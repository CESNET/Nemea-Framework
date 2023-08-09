/**
 * \file ifc_tcpip.c
 * \brief TRAP TCP/IP interfaces
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \author Pavel Siska <siska@cesnet.cz>
 * \date 2018
 * \date 2023
 */
/*
 * Copyright (C) 2023 CESNET
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <assert.h>
#include <poll.h>

#include "../include/libtrap/trap.h"
#include "trap_internal.h"
#include "trap_ifc.h"
#include "trap_error.h"
#include "ifc_tcpip.h"
#include "ifc_tcpip_internal.h"
#include "ifc_socket_common.h"

/**
 * \addtogroup trap_ifc TRAP communication module interface
 * @{
 */
/**
 * \addtogroup tcpip_ifc TCP/IP and UNIX socket communication interface module
 * @{
 */

#define MAX_RECOVERY_TRY   10
/* must be smaller than 1000000 */
#define RECOVERY_WAIT_USEC 500000
#define USEC_IN_SEC        1000000
#define ACK_MESS_SIZE      1
#define CRIT_1VS2SEND      10000
#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif
#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif

/***** TCPIP server *****/

/**
 * Internal union for host address storage, common for tcpip & unix
 */
union tcpip_socket_addr {
   struct addrinfo tcpip_addr; ///< used for TCPIP socket
   struct sockaddr_un unix_addr; ///< used for path of UNIX socket
};

/**
 * \brief Structure passed to send thread.
 */
struct thread_data {
    tcpip_sender_private_t *arg;
    client_t *client;
};

/**
 * Basic interface statistics
 */
struct tcp_trap_stats {
   uint64_t total_missed; /** Missed receive messages */
   uint64_t total_msg; /** Total messages */
   uint64_t total_containers; /** Total containers */
   float percentage_missed;
} tcp_trap_stats;

static int client_socket_connect(void *priv, const char *dest_addr, const char *dest_port, int *socket_descriptor, struct timeval *tv);
static void client_socket_disconnect(void *priv);
static int server_socket_open(void *priv);

/**
 * \brief Get sockaddr, IPv4 or IPv6
 * \param[in] sa  structure with input socket address
 * \return converted ponter to address
 */
static void *get_in_addr(struct sockaddr *sa)
{
   if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }

   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
 * \brief Check if the given port is a correct port number.
 *
 * Port number for TCP socket must be a number in the range from 1 to 65535.
 * It can also be a service name that is translated by getaddrinfo().
 *
 * \param[in] port  Port to check.
 * \return EXIT_FAILURE if port is not given or it is a number < 1 or > 65535;
 * EXIT_SUCCESS when port is a valid number or it is a service name.
 */
static int check_portrange(const char *port)
{
   uint32_t portnum = 0;
   int ret;

   if (port == NULL) {
      return EXIT_FAILURE;
   }

   ret = sscanf(port, "%" SCNu32, &portnum);
   if (ret == 1) {
      if (portnum < 1 || portnum > 65535) {
         VERBOSE(CL_ERROR, "Given port (%" PRIu32 ") number is out of the allowed range (1-65535).", portnum);
         return EXIT_FAILURE;
      }
   }

   /* port is not number (it is a service name) or it is correct */
   return EXIT_SUCCESS;
}

/**
 * \addtogroup tcpip_receiver
 * @{
 */
/* Receiver (client socket) */
// Receiver is a client that connects itself to the source of data (to sender) = server

/**
 * Receive chunk of data.
 *
 * Caller is responsible for checking elapsed time, since this function
 * may finished before the given timeout without having data.
 *
 * \param[in] priv      private IFC data
 * \param[out] data     received data
 * \param[in,out] size  expected size to wait for, it is used to return size that was not read
 * \param[in] tm        timeout
 */
static int receive_part(void *priv, void **data, uint32_t *size, struct timeval *tm)
{
   void *data_p = (*data);
   tcpip_receiver_private_t *config = (tcpip_receiver_private_t *) priv;
   ssize_t numbytes = *size;
   int recvb, retval;
   struct pollfd pfds;
   struct timespec ts, *tempts = NULL;
   if (tm != NULL) {
      ts.tv_sec = tm->tv_sec;
      ts.tv_nsec = tm->tv_usec * 1000l;
      tempts = &ts;
   }

   assert(data_p != NULL);

   while (config->is_terminated == 0) {
      DEBUG_IFC(if (tm) {VERBOSE(CL_VERBOSE_LIBRARY, "Try to receive data in timeout %" PRIu64
                        "s%"PRIu64"us", tm->tv_sec, tm->tv_usec)});

      /*
       * Blocking or with timeout?
       * With timeout 0,0 - non-blocking
       */
      pfds = (struct pollfd) {.fd = config->sd, .events = POLLIN};
      retval = ppoll(&pfds, 1, tempts, NULL);
      if (retval > 0 && pfds.revents & POLLIN) {
         do {
            recvb = recv(config->sd, data_p, numbytes, 0);
            if (recvb < 1) {
               if (recvb == 0) {
                  errno = EPIPE;
               }
               switch (errno) {
               case EINTR:
                  if (config->is_terminated == 1) {
                     client_socket_disconnect(priv);
                     return TRAP_E_TERMINATED;
                  }
                  break;
               case ECONNRESET:
               case EBADF:
               case EPIPE:
                  client_socket_disconnect(priv);
                  return TRAP_E_IO_ERROR;
               case EAGAIN:
                  /* This should never happen with blocking socket. */
                  (*size) = numbytes;
                  (*data) = data_p;
                  return TRAP_E_TIMEOUT;
               }
            }
            numbytes -= recvb;
            data_p += recvb;
            DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "receive_part got %" PRId32 "B", recvb));
         } while (numbytes > 0);
         (*size) = numbytes;
         (*data) = data_p;
         return TRAP_E_OK;
      } else if ((retval == 0) || (retval < 0 && errno == EINTR)) {
         /* Timeout expired or signal received.  Caller of this function
          * has to decide to call this function again or not according
          * to elapsed time from the calling. */
         (*size) = numbytes;
         return TRAP_E_TIMEOUT;
      } else { // some error has occured
         VERBOSE(CL_VERBOSE_OFF, "ppoll() returned %i (%s)", retval, strerror(errno));
         client_socket_disconnect(priv);
         return TRAP_E_IO_ERROR;
      }
   }
   return TRAP_E_TERMINATED;
}

/**
 * Return current time in microseconds.
 *
 * This is used to get current timestamp in tcpip_receiver_recv() and tcpip_sender_send().
 *
 * \return current timestamp
 */
static inline uint64_t get_cur_timestamp()
{
   struct timespec spec_time;

   clock_gettime(CLOCK_MONOTONIC, &spec_time);
   /* time in microseconds seconds -> secs * microsends + nanoseconds */
   return spec_time.tv_sec * 1000000 + (spec_time.tv_nsec / 1000);
}

/**
 * \brief Receive data from interface.
 *
 * It is expected that data is always the same pointer because it is buffer given by trap.c.
 *
 * This function contains finite state machine that controls receiving messages (header
 * and payload), handles timeouts and sleep (to offload CPU during waiting for connection).
 * The transition graph is:
 * \dot
 * digraph fsm { label="tcpip_receiver_recv()";labelloc=t;
 *      init -> conn_wait;
 *      init -> head_wait;
 *      init -> mess_wait;
 *      discard -> reset;
 *      reset -> init;
 *      reset -> init;
 *      reset -> reset;
 *      reset -> init;
 *      conn_wait -> reset;
 *      conn_wait -> head_wait;
 *      head_wait -> discard;
 *      head_wait -> reset;
 *      head_wait -> reset;
 *      head_wait -> mess_wait;
 *      mess_wait -> discard;
 *      mess_wait -> reset;
 * }
 * \enddot
 *
 * \param [in,out] priv  private configuration structure
 * \param [out] data  where received data are stored
 * \param [out] size  size of received data
 * \param [in] timeout  timeout in usec, can be TRAP_WAIT, TRAP_HALFWAIT, or TRAP_NO_WAIT
 * \return TRAP_E_OK (0) on success
 */
int tcpip_receiver_recv(void *priv, void *data, uint32_t *size, int timeout)
{
#ifdef LIMITED_RECOVERY
   uint32_t recovery = 0;
#endif
   /** messageframe contains header that is read (even partially) in HEAD_WAIT */
   trap_buffer_header_t messageframe;
   tcpip_receiver_private_t *config = (tcpip_receiver_private_t *) priv;
   void *p = &messageframe;
   struct timeval tm, *temptm;
   int retval;
   uint64_t entry_time = get_cur_timestamp();
   uint64_t curr_time = 0;

   /* sleeptime (in usec) with sleeptimespec are used to wait
    * for a while before next connecting when non-blocking. */
   uint64_t sleeptime;
   struct timespec sleeptimespec;

   /* correct module will pass only possitive timeout or TRAP_WAIT.
    * TRAP_HALFWAIT is not valid value */
   assert(timeout > TRAP_HALFWAIT);

   if ((config == NULL) || (data == NULL) || (size == NULL)) {
      return TRAP_E_BAD_FPARAMS;
   }
   (*size) = 0;

   DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv trap_recv() was called"));

   /* convert libtrap timeout into timespec and timeval */
   trap_set_timeouts(timeout, &tm, NULL);
   temptm = (timeout == TRAP_WAIT) ? NULL : &tm;

   while (config->is_terminated == 0) {
init:
      p = &messageframe;
      DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv INIT"));
      if (config->connected == 0) {
         goto conn_wait;
      } else {
         if (config->data_pointer == NULL) {
            goto head_wait;
         } else {
            /* continue where we timedout earlier */
            p = config->data_pointer;
            goto mess_wait;
         }
      }
discard:
      DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv DISCARD"));
      config->data_pointer = NULL;
      goto reset;
reset:
      config->seq = -1;
      if (config->is_terminated != 0) {
         /* TRAP_E_TERMINATED is returned outside the loop */
         break;
      }
      /* failure, next state is exit when we are non-blocking or INIT on blocking,
         this state is a great place for handling timeouts. */
      DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv RESET"));
      if (timeout == TRAP_WAIT) {
#ifdef LIMITED_RECOVERY
         if (++recovery > MAX_RECOVERY_TRY) {
            goto init;
         } else {
            return TRAP_E_TIMEOUT;
         }
#else
         goto init;
#endif
      } else {
         /* non-blocking mode, let's check elapsed time */
         curr_time =  get_cur_timestamp();
         if ((curr_time - entry_time) >= timeout) {
            return TRAP_E_TIMEOUT;
         } else {
            if (config->connected == 0) {
               /* wait at most 1 second before return to INIT */

               /* sleeptime is in usec */
               sleeptime = timeout - (curr_time - entry_time);
               /* if remaining sleeptime is higher than 1s, use 1s */
               if (sleeptime < 1000000) {
                  sleeptimespec.tv_sec = sleeptime / 1000000;
                  sleeptimespec.tv_nsec = (sleeptime % 1000000) * 1000;
                  DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "sleep time set %" PRIu64
                            " us: %"PRIu64"s%"PRIu64"ns", sleeptime,
                            sleeptimespec.tv_sec, sleeptimespec.tv_nsec));
               } else {
                  sleeptimespec.tv_sec = 1;
                  sleeptimespec.tv_nsec = 0;
               }
               /* We are not interested in reminder, because timeout will be
                * checked again later. */
               if (nanosleep(&sleeptimespec, NULL) == -1) {
                  if (errno == EINTR) {
                     goto reset;
                  } else {
                     VERBOSE(CL_ERROR, "recv nanosleep(): %s", strerror(errno));
                  }
               }
               DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv nanosleep finished"));
            }

            /* update timeout that is used for recv after successful connection */
            curr_time =  get_cur_timestamp();
            sleeptime = timeout - (int) (curr_time - entry_time);
            if ((int) sleeptime > 0) {
               trap_set_timeouts(sleeptime, &tm, NULL);
            } else {
               return TRAP_E_TIMEOUT;
            }

            goto init;
         }
      }
conn_wait:
      /* check if connected -> try to connect -> check if connected; next state is RESET or HEAD_WAIT */
      /* expected next state is waiting for header */
      DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv CONN_WAIT"));
      if (config->connected == 0) {
         /* we don't have connection, we must try to connect before accepting header */
         retval = client_socket_connect(priv, config->dest_addr, config->dest_port, &config->sd, temptm);
         if (retval == TRAP_E_FIELDS_MISMATCH) {
            config->connected = 1;
            return TRAP_E_FORMAT_MISMATCH;
         } else if (retval == TRAP_E_NEGOTIATION_FAILED) {
            config->connected = 1;
            return TRAP_E_NEGOTIATION_FAILED;
         } else if (retval == TRAP_E_OK) {
            config->connected = 1;
            /* ok, wait for header as we planned */
         } else {
            /* failed, reseting... */
            if (timeout == TRAP_WAIT) {
               /* Create a delay when blocking...
                * This is specific situation, many attempts would be unpleasant */
               sleep(1);
            }
            goto reset;
         }
      }
      goto head_wait;
head_wait:
      /* get and check header of message, next state can be MESS_WAIT or RESET */
      DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv HEAD_WAIT (%p)", p));
      config->data_wait_size = sizeof(trap_buffer_header_t);
      retval = receive_part(config, &p, &config->data_wait_size, temptm);
      if (retval != TRAP_E_OK) {
         /* receiving failed */
         DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv failed HEAD (%p) waiting %d B", p, config->data_wait_size));
         if (retval == TRAP_E_IO_ERROR) {
            /* disconnected -> drop data */
            goto discard;
         }

         goto reset;
      } else {
         /* we expect to receive data */
         messageframe.data_length = ntohl(messageframe.data_length);

         config->total_msg += messageframe.size;
         if (config->seq != -1 && config->seq + config->cur_size != messageframe.seq_num) {
         	config->total_missed += (messageframe.seq_num - config->seq + config->cur_size);
         	VERBOSE(CL_VERBOSE_BASIC, "Recv: missed %" PRIu64 " messages. %.1f%% of total seen messages %" 
               PRIu64 " has been missed", 
         		messageframe.seq_num - config->seq + config->cur_size,
         		(100.0 / (config->total_msg + config->total_missed)) * (config->total_missed), 
         		config->total_missed + config->total_msg);
         }
         tcp_trap_stats.total_containers++;
         tcp_trap_stats.total_missed = config->total_missed;
         tcp_trap_stats.total_msg = config->total_msg;
         tcp_trap_stats.percentage_missed = (100.0 / (config->total_msg + config->total_missed)) * (config->total_missed);

         config->seq = messageframe.seq_num;
         config->cur_size = messageframe.size;


         config->data_wait_size = messageframe.data_length;
         config->ext_buffer_size = messageframe.data_length;
#ifdef ENABLE_CHECK_HEADER
         /* check if header is ok: */
         if (tcpip_check_header(&messageframe) == 0) {
            goto reset;
         }
#endif
         /* we got header, now we can start receiving payload */
         p = data;
         config->ext_buffer = data;
         goto mess_wait;
      }
mess_wait:
      /* get and check payload of message, next state can be RESET or success exit */
      /* receive payload */
      DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv waiting MESS (%p) %d B", p, config->data_wait_size));
      retval = receive_part(config, &p, &config->data_wait_size, temptm);
      if (retval == TRAP_E_OK) {
         /* Success! Data was already set by recv */
         config->data_pointer = NULL;
         (*size) = messageframe.data_length;
         DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv get MESS (%p) remains: %d B", p, config->data_wait_size));
         return TRAP_E_OK;
      } else {
         DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv get MESS (%p) still waiting for %d B", p, config->data_wait_size));
         if (retval == TRAP_E_IO_ERROR) {
            /* disconnected -> drop data */
            goto discard;
         }
         config->data_pointer = p;
         goto reset;
      }
   }
   return TRAP_E_TERMINATED;
}

/**
 * \brief Set interface state as terminated.
 * \param[in] priv  pointer to module private data
 */
void tcpip_receiver_terminate(void *priv)
{
   tcpip_receiver_private_t *config = (tcpip_receiver_private_t *) priv;

   if (config != NULL) {
      config->is_terminated = 1;
   } else {
      VERBOSE(CL_ERROR, "Bad parameter of tcpip_receiver_terminate()!");
   }
   return;
}


/**
 * \brief Destructor of TCPIP receiver (input ifc)
 * \param[in] priv  pointer to module private data
 */
void tcpip_receiver_destroy(void *priv)
{
#define X(p) if (p != NULL) { \
free(p); \
p = NULL; \
}
   tcpip_receiver_private_t *config = (tcpip_receiver_private_t *) priv;
   if (config != NULL) {
      if (config->connected == 1) {
         close(config->sd);
      }
      X(config->dest_addr);
      X(config->dest_port);
      X(config);
   } else {
      VERBOSE(CL_ERROR, "Destroying IFC that is probably not initialized.");
   }
   return;
#undef X
}

static void tcpip_receiver_create_dump(void *priv, uint32_t idx, const char *path)
{
   tcpip_receiver_private_t *c = (tcpip_receiver_private_t *) priv;
   /* return value */
   int r;
   /* config file trap-i<number>-config.txt */
   char *conf_file = NULL;
   /* config file trap-i<number>-buffer.dat */
   char *buf_file = NULL;
   FILE *f = NULL;
   trap_buffer_header_t aux = { 0 };
   aux.data_length = htonl(c->ext_buffer_size);

   r = asprintf(&conf_file, "%s/trap-i%02"PRIu32"-config.txt", path, idx);
   if (r == -1) {
      VERBOSE(CL_ERROR, "Not enough memory, dump failed. (%s:%d)", __FILE__, __LINE__);
      conf_file = NULL;
      goto exit;
   }
   f = fopen(conf_file, "w");
   fprintf(f, "Dest addr: %s\nDest port: %s\nConnected: %d\n"
           "Terminated: %d\nSocket descriptor: %d\nSocket type: %d\n"
           "Data pointer: %p\nData wait size: %"PRIu32"\nMessage header: %"PRIu32"\n"
           "Extern buffer pointer: %p\nExtern buffer data size: %"PRIu32"\n"
           "Timeout: %"PRId32"us (%s)\n",
           c->dest_addr, c->dest_port, c->connected, c->is_terminated, c->sd, c->socket_type,
           c->data_pointer, c->data_wait_size, c->int_mess_header.data_length,
           c->ext_buffer, c->ext_buffer_size,
           c->ctx->in_ifc_list[idx].datatimeout,
           TRAP_TIMEOUT_STR(c->ctx->in_ifc_list[idx].datatimeout));
   fclose(f);
   f = NULL;

   r = asprintf(&buf_file, "%s/trap-i%02"PRIu32"-buffer.dat", path, idx);
   if (r == -1) {
      buf_file = NULL;
      VERBOSE(CL_ERROR, "Not enough memory, dump failed. (%s:%d)", __FILE__, __LINE__);
      goto exit;
   }
   f = fopen(buf_file, "w");
   if (fwrite(&aux, sizeof(c->ext_buffer_size), 1, f) != 1) {
      VERBOSE(CL_ERROR, "Writing buffer header failed. (%s:%d)", __FILE__, __LINE__);
      goto exit;
   }
   if (fwrite(c->ext_buffer, c->ext_buffer_size, 1, f) != 1) {
      VERBOSE(CL_ERROR, "Writing buffer content failed. (%s:%d)", __FILE__, __LINE__);
      goto exit;
   }
exit:
   if (f != NULL) {
      fclose(f);
   }
   free(conf_file);
   free(buf_file);
   return;
}

char *tcpip_recv_ifc_get_id(void *priv)
{
   if (priv == NULL) {
      return NULL;
   }

   tcpip_receiver_private_t *config = (tcpip_receiver_private_t *) priv;
   if (config->dest_port == NULL) {
      return NULL;
   }
   return config->dest_port;
}

uint8_t tcpip_recv_ifc_is_conn(void *priv)
{
   if (priv == NULL) {
      return 0;
   }
   tcpip_receiver_private_t *config = (tcpip_receiver_private_t *) priv;
   if (config->connected == 1) {
      return 1;
   }
   return 0;
}

/**
 * \brief Constructor of input TCP/IP IFC module.
 * This function is called by TRAP library to initialize one input interface.
 *
 * \param[in,out] ctx   Pointer to the private libtrap context data (trap_ctx_init()).
 * \param[in] params    Configuration string containing space separated values of these parameters (in this exact order): *dest_addr* *dest_port*,
 * where dest_addr is destination address of output TCP/IP IFC module and
 * dest_port is the port where sender is listening.
 * \param[in,out] ifc   IFC interface used for calling TCP/IP module.
 * \param[in] idx       Index of IFC that is created.
 * \param [in] type     Select the type of socket (see #tcpip_ifc_sockettype for options).
 * \return 0 on success (TRAP_E_OK)
 */
int create_tcpip_receiver_ifc(trap_ctx_priv_t *ctx, char *params, trap_input_ifc_t *ifc, uint32_t idx, enum tcpip_ifc_sockettype type)
{
#define X(pointer) free(pointer); \
   pointer = NULL;

   int result = TRAP_E_OK;
   char *param_iterator = NULL;
   char *dest_addr = NULL;
   char *dest_port = NULL;
   tcpip_receiver_private_t *config = NULL;

   if (params == NULL) {
      VERBOSE(CL_ERROR, "No parameters found for input IFC.");
      return TRAP_E_BADPARAMS;
   }

   config = (tcpip_receiver_private_t *) calloc(1, sizeof(tcpip_receiver_private_t));
   if (config == NULL) {
      VERBOSE(CL_ERROR, "Failed to allocate internal memory for input IFC.");
      return TRAP_E_MEMORY;
   }
   config->ctx = ctx;
   config->is_terminated = 0;
   config->socket_type = type;
   config->ifc_idx = idx;
   config->seq = -1;
   config->cur_size = 0;
   config->total_missed = 0;
   config->total_msg = 0;

   /* Parsing params */
   param_iterator = trap_get_param_by_delimiter(params, &dest_addr, TRAP_IFC_PARAM_DELIMITER);
   if (param_iterator == NULL) {
      /* error! we expect 2 parameters */
      if ((dest_addr == NULL) || (strlen(dest_addr) == 0)) {
         VERBOSE(CL_ERROR, "Missing 'destination address' for TCPIP IFC.");
         result = TRAP_E_BADPARAMS;
         goto failsafe_cleanup;
      }
   }
   param_iterator = trap_get_param_by_delimiter(param_iterator, &dest_port, TRAP_IFC_PARAM_DELIMITER);
   if ((dest_port == NULL) || (strlen(dest_port) == 0)) {
      /* if 2nd param is missing, use localhost as addr and 1st param as "port" */
      free(dest_port);
      dest_port = dest_addr;
      dest_addr = strdup("localhost");
      VERBOSE(CL_VERBOSE_BASIC, "Using the only parameter as 'destination port' and \"localhost\" as 'destination address' for TCPIP IFC.");
   }

   /* set global buffer size */
   config->int_mess_header.data_length = DEFAULT_MAX_DATA_LENGTH;
   /* Parsing params ended */

   config->dest_addr = dest_addr;
   config->dest_port = dest_port;

   if ((config->dest_addr == NULL) || (config->dest_port == NULL)) {
      /* no delimiter found even if we expect two parameters */
      VERBOSE(CL_ERROR, "Malformed params for input IFC, missing destination address and port.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }

   VERBOSE(CL_VERBOSE_ADVANCED, "config:\ndest_addr=\"%s\"\ndest_port=\"%s\"\n"
           "TDU size: %u\n", config->dest_addr, config->dest_port,
           config->int_mess_header.data_length);

   /*
    * In constructor, we do not know timeout yet.
    * Use 5 seconds to wait for connection to output interface.
    */
#ifndef ENABLE_NEGOTIATION
   int retval = 0;
   struct timeval tv = {5, 0};
   retval = client_socket_connect((void *) config, config->dest_addr, config->dest_port, &config->sd, &tv);
   if (retval != TRAP_E_OK) {
      config->connected = 0;
      if ((retval == TRAP_E_BAD_FPARAMS) || (retval == TRAP_E_IO_ERROR)) {
         VERBOSE(CL_VERBOSE_BASIC, "Could not connect to sender due to bad parameters.");
         result = TRAP_E_BADPARAMS;
         goto failsafe_cleanup;
      }
   } else {
      config->connected = 1;
   }
#endif

   /* hook functions and store priv */
   ifc->recv = tcpip_receiver_recv;
   ifc->destroy = tcpip_receiver_destroy;
   ifc->terminate = tcpip_receiver_terminate;
   ifc->create_dump = tcpip_receiver_create_dump;
   ifc->priv = config;
   ifc->get_id = tcpip_recv_ifc_get_id;
   ifc->is_conn = tcpip_recv_ifc_is_conn;

#ifndef ENABLE_NEGOTIATION
   if (config->connected == 0) {
      VERBOSE(CL_VERBOSE_BASIC, "Could not connect to sender.");
      if ((retval == TRAP_E_BAD_FPARAMS) || (retval == TRAP_E_IO_ERROR)) {
        result = retval;
        goto failsafe_cleanup;
      }
   }
#endif
   return TRAP_E_OK;
failsafe_cleanup:
   X(dest_addr);
   X(dest_port);
   X(config);
   return result;
#undef X
}

/**
 * Disconnect from output IFC.
 *
 * \param[in,out] priv  pointer to private structure of input IFC (client)
 */
static void client_socket_disconnect(void *priv)
{
   tcpip_receiver_private_t *config = (tcpip_receiver_private_t *) priv;
   DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv Disconnected."));
   if (config->connected == 1) {
      VERBOSE(CL_VERBOSE_BASIC, "TCPIP ifc client disconnecting");
      close(config->sd);
      config->connected = 0;
   }
}

/**
 * Function waits for non-blocking connect().
 *
 * \param[in] sock socket descriptor of client
 * \param[in] tv  timeout
 * \return TRAP_E_OK on success, TRAP_E_TIMEOUT on error (can be caused by interrupt)
 */
static int wait_for_connection(int sock, struct timeval *tv)
{
   int rv;
   struct pollfd pfds = {.fd = sock, .events = POLLOUT};
   struct timespec ts, *tempts = NULL;
   if (tv != NULL) {
      ts.tv_sec = tv->tv_sec;
      ts.tv_nsec = tv->tv_usec * 1000l;
      tempts = &ts;
   }
   VERBOSE(CL_VERBOSE_LIBRARY, "wait for connection");
   rv = ppoll(&pfds, 1, tempts, NULL);
   if (rv == 1 && pfds.revents & POLLOUT) {
      int so_error = 0;
      socklen_t len = sizeof so_error;

      if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) != -1 &&
          so_error == 0) {
         return TRAP_E_OK;
      }
   }
   return TRAP_E_TIMEOUT;
}

/**
 * \brief client_socket is used as a receiver
 * \param[in] priv  pointer to module private data
 * \param[in] dest_addr  destination address where to connect and where receive
 * \param[in] dest_port  destination port where to connect and where receive
 * \param[out] socket_descriptor  socket descriptor of established connection
 * \param[in] tv  timeout
 * \return TRAP_E_OK on success
 */
static int client_socket_connect(void *priv, const char *dest_addr, const char *dest_port, int *socket_descriptor, struct timeval *tv)
{
   tcpip_receiver_private_t *config = (tcpip_receiver_private_t *) priv;
   int sockfd = -1, options;
   union tcpip_socket_addr addr;
   struct addrinfo *servinfo, *p = NULL;
   int rv = 0, addr_count = 0;
   char s[INET6_ADDRSTRLEN];

   if ((config == NULL) || (dest_addr == NULL) || (dest_port == NULL) || (socket_descriptor == NULL)) {
      return TRAP_E_BAD_FPARAMS;
   }
   if (check_portrange(dest_port) == EXIT_FAILURE) {
      return TRAP_E_BADPARAMS;
   }

   memset(&addr, 0, sizeof(addr));

   if (config->socket_type == TRAP_IFC_TCPIP) {
      addr.tcpip_addr.ai_family = AF_UNSPEC;
      addr.tcpip_addr.ai_socktype = SOCK_STREAM;

      if ((rv = getaddrinfo(dest_addr, dest_port, &addr.tcpip_addr, &servinfo)) != 0) {
         VERBOSE(CL_ERROR, "getaddrinfo: %s", gai_strerror(rv));
         return TRAP_E_IO_ERROR;
      }

      DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv Try to connect"));

      if (tv != NULL) {
         /* compute uniform intervals for all possible address */
         for (p = servinfo; p != NULL; p = p->ai_next) {
            addr_count++;
         }
         tv->tv_sec = (tv->tv_sec * 1000000 + tv->tv_usec) / addr_count;
         tv->tv_usec = tv->tv_sec % 1000000;
         tv->tv_sec /= 1000000;
         VERBOSE(CL_VERBOSE_LIBRARY, "Every address will be tried for timeout: %"PRId64"s%"PRId64"us",
               tv->tv_sec, tv->tv_usec);
      }

      // loop through all the results and connect to the first we can
      for (p = servinfo; p != NULL; p = p->ai_next) {
         if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
         }

         /* Change the socket to be non-blocking if required by user. */
         if (tv != NULL) {
            if ((options = fcntl(sockfd, F_GETFL)) != -1) {
               if (fcntl(sockfd, F_SETFL, O_NONBLOCK | options) == -1) {
                  VERBOSE(CL_ERROR, "Could not set socket to non-blocking.");
               }
            }
         }

         if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            if (errno != EINPROGRESS && errno != EAGAIN) {
               DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv TCPIP ifc connect error %d (%s)", errno,
                                 strerror(errno)));
               close(sockfd);
               continue;
            } else {
               rv = wait_for_connection(sockfd, tv);
               if (rv == TRAP_E_TIMEOUT) {
                  close(sockfd);
                  if (config->is_terminated) {
                     rv = TRAP_E_TERMINATED;
                     break;
                  }
                  /* try another address */
                  continue;
               } else {
                  /* success */
                  rv = TRAP_E_OK;
                  break;
               }
            }
         }
         break;
      }

      if (p != NULL) {
         if (inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s) != NULL) {
            VERBOSE(CL_VERBOSE_LIBRARY, "recv client: connected to %s", s);
         }
      }
      freeaddrinfo(servinfo); // all done with this structure
   } else if (config->socket_type == TRAP_IFC_TCPIP_UNIX) {
      /* UNIX socket */
      addr.unix_addr.sun_family = AF_UNIX;
      snprintf(addr.unix_addr.sun_path, sizeof(addr.unix_addr.sun_path) - 1, trap_default_socket_path_format, dest_port);
      sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
      if (sockfd != -1) {
         if (connect(sockfd, (struct sockaddr *) &addr.unix_addr, sizeof(addr.unix_addr)) < 0) {
            VERBOSE(CL_VERBOSE_LIBRARY, "recv UNIX domain socket connect error %d (%s)", errno, strerror(errno));
            close(sockfd);
            sockfd = -1;
         } else {
            p = (struct addrinfo *) &addr.unix_addr;
         }
        rv = TRAP_E_OK;
      } else {
         rv = TRAP_E_IO_ERROR;
      }
   }

   if (p == NULL) {
      VERBOSE(CL_VERBOSE_LIBRARY, "recv client: Connection failed.");
      rv = TRAP_E_TIMEOUT;
   }

   if (rv != TRAP_E_OK) { /*something went wrong while setting up connection */
      if (sockfd >= 0) {
         close(sockfd);
      }
      return rv;
   }

   *socket_descriptor = sockfd;


   /** Input interface negotiation */
#ifdef ENABLE_NEGOTIATION
   switch (input_ifc_negotiation(priv, TRAP_IFC_TYPE_TCPIP)) {
   case NEG_RES_FMT_UNKNOWN:
      VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: failed (unknown data format of the output interface).");
      close(sockfd);
      return TRAP_E_TIMEOUT;

   case NEG_RES_CONT:
      VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success.");
      return TRAP_E_OK;

   case NEG_RES_FMT_CHANGED: // used on format change with JSON
      VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success (format has changed; it was not first negotiation).");
      return TRAP_E_OK;

   case NEG_RES_RECEIVER_FMT_SUBSET: // used on format change with UniRec
      VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success (required set of fields of the input interface is subset of the recevied format).");
      return TRAP_E_OK;

   case NEG_RES_SENDER_FMT_SUBSET: // used on format change with UniRec
      VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success (new recevied format specifier is subset of the old one; it was not first negotiation).");
      return TRAP_E_OK;

   case NEG_RES_FAILED:
      VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: failed (error while receiving hello message from output interface).");
      return TRAP_E_NEGOTIATION_FAILED;

   case NEG_RES_FMT_MISMATCH:
      VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: failed (data type or data format specifier mismatch).");
      return TRAP_E_FIELDS_MISMATCH;

   default:
      VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: default case");
      break;
   }
#endif


   return rv;
}

/**
 * @}
 *//* tcpip_receiver */

/**
 * \addtogroup tcpip_sender
 * @{
 */

static uint64_t
calculate_sleep(uint64_t current_sleep)
{
	uint64_t sleep = current_sleep * 2;
	return sleep > 5000 ? 5000 : sleep;
}

static uint64_t
find_lowest_container_id(tcpip_sender_private_t *c)
{
   struct client_s *cl;
   uint64_t lowest = -1;

   pthread_mutex_lock(&c->client_list_mtx);
   LIST_FOREACH(cl, &c->clients_list_head, entries) {
      if (cl->container_id < lowest) {
         lowest = cl->container_id;
      }
   }
   pthread_mutex_unlock(&c->client_list_mtx);
   return lowest == -1 ? 0: lowest;
}

/**
 * \brief This function is called when a client was/is being disconnected.
 *
 * \param[in] c Pointer to interface's private data structure.
 * \param[in] cl Client to disconnect
 */
static void
disconnect_client(tcpip_sender_private_t *c, client_t *cl)
{
   pthread_mutex_lock(&c->client_list_mtx);
   client_t *next, *cl_iterator = LIST_FIRST(&c->clients_list_head);
   while (cl_iterator != NULL) {
      next = LIST_NEXT(cl_iterator, entries);
      if (cl_iterator == cl) {
         __sync_sub_and_fetch(&c->connected_clients, 1);
         LIST_REMOVE(cl, entries);
         shutdown(cl->sd, SHUT_RDWR);
         close(cl->sd);
         free(cl);
         break;
      }
      cl_iterator = next;
   }
   pthread_mutex_unlock(&c->client_list_mtx);
}

/**
 * \brief Function disconnects all clients of the output interface whose private structure is passed via "priv" parameter.
 *
 * \param[in] priv Pointer to output interface private structure.
 */
void tcpip_server_disconnect_all_clients(void *priv)
{
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;
   pthread_mutex_lock(&c->client_list_mtx);
   client_t *next, *cl = LIST_FIRST(&c->clients_list_head);
   while (cl != NULL) {
      next = LIST_NEXT(cl, entries);
      __sync_sub_and_fetch(&c->connected_clients, 1);
      LIST_REMOVE(cl, entries);
      shutdown(cl->sd, SHUT_RDWR);
      close(cl->sd);
      free(cl);
      cl = next;
   }
   pthread_mutex_unlock(&c->client_list_mtx);
}

static bool
is_next_container_ready(tcpip_sender_private_t *c, client_t *cl)
{
	uint64_t head = __sync_fetch_and_add(&c->t_mbuf.to_send.head_, 0); 
   if (head == 0 || cl->container_id >= head) {
      return false;
   } 
   return true;
}

/**
 * \brief This function runs in a separate thread. It handles sending data
          to connected clients for TCPIP and UNIX interfaces (this function is blocking).
 * \param[in] arg pointer to thread_data
 */
static void *
send_blocking_mode(void *arg)
{
   tcpip_sender_private_t *c = ((struct thread_data *) arg)->arg;
   client_t *cl = ((struct thread_data *) arg)->client;
   struct trap_container_s *t_cont;

   uint64_t sleep_time = 1;
   size_t pending_bytes;
   char *buffer;
   int send_ret_code;

   while (!c->is_terminated) {
      // is next container ready
      while (!is_next_container_ready(c, cl)) {
         if (c->is_terminated) {
            break;
         }
        	sleep_time = calculate_sleep(sleep_time);
        	usleep(sleep_time);
      }

      sleep_time = 1;

      // get next container
      t_cont = t_rb_at(&c->t_mbuf.to_send, cl->container_id);
          
      buffer = t_cont->buffer;
      pending_bytes = t_cont->used_bytes;

again:
      send_ret_code = send(cl->sd, &buffer[t_cont->used_bytes - pending_bytes], pending_bytes, MSG_NOSIGNAL);
      if (send_ret_code < 0) { // Send failed
         if (c->is_terminated) {
            break;
         }
         switch (errno) {
         case EBADF:
         case EPIPE:
         case EFAULT:
            goto cleanup;
         case EAGAIN:
            goto again;
         default:    
            VERBOSE(CL_VERBOSE_OFF, "Unhandled error from send in send_blocking_mode (errno: %i)", errno);
            goto cleanup;
         }
      } else {
         pending_bytes -= send_ret_code;
         if (pending_bytes) { // buffer was not send completely. Try again send pending bytes.
            goto again;
         }
      }

      // increase container ID and statistics
      cl->sent_containers++;
      cl->sent_messages += t_cont->size; 
      __sync_add_and_fetch(&cl->container_id, 1);
   }

   pthread_exit(NULL);

cleanup:
   disconnect_client(c, cl);
   free(arg);
   return NULL;
}

static void *
send_non_blocking_mode(void *arg)
{
   tcpip_sender_private_t *c = ((struct thread_data *) arg)->arg;
   client_t *cl = ((struct thread_data *) arg)->client;
   struct trap_container_s *t_cont;

   uint64_t sleep_time = 1;
   uint64_t next_seq_number = -1;
   size_t pending_bytes;
   char *buffer;
   int send_ret_code;

   while (!c->is_terminated) {
again_set_container:
      while (!is_next_container_ready(c, cl)) {
        	sleep_time = calculate_sleep(sleep_time);
         usleep(sleep_time);
      }

      sleep_time = 1;
          
      // get next container
      t_cont = t_rb_at(&c->t_mbuf.to_send, cl->container_id);

      // container is no longer available
      if (t_cont_acquiere(t_cont) < 1 || __sync_fetch_and_add(&t_cont->idx, 0) != cl->container_id) {
         t_cont_release(t_cont);
         uint64_t head = __sync_fetch_and_add(&c->t_mbuf.to_send.head_, 0); 
         __sync_add_and_fetch(&cl->container_id, head - cl->container_id);
         goto again_set_container;
      }

      buffer = t_cont->buffer;
      pending_bytes = t_cont->used_bytes;

again:
      send_ret_code = send(cl->sd, &buffer[t_cont->used_bytes - pending_bytes], pending_bytes, MSG_NOSIGNAL);
      if (send_ret_code < 0) { // Send failed
         if (c->is_terminated) {
            break;
         }
         switch (errno) {
         case EBADF:
         case EPIPE:
         case EFAULT:
            t_cont_release(t_cont);
            goto cleanup;
         case EAGAIN:
            goto again;
         default:    
            VERBOSE(CL_VERBOSE_OFF, "Unhandled error from send in send_non_blocking_mode (errno: %i)", errno);
            t_cont_release(t_cont);
            goto cleanup;
         }
      } else {
         pending_bytes -= send_ret_code;
         if (pending_bytes) { // buffer was not send completely. Try again send pending bytes.
            goto again;
         }
      }

      // calculate skipped messages 
      if (next_seq_number == -1) {
         next_seq_number = t_cont->seq_num + t_cont->size;
      } else {
         if (next_seq_number != t_cont->seq_num) {
            cl->skipped_messages += t_cont->seq_num - next_seq_number;
         }
         next_seq_number = t_cont->seq_num + t_cont->size;
      }

      cl->timeouts = cl->skipped_messages;

      // increase container ID and statistics
      cl->sent_containers++;
      cl->sent_messages += t_cont->size; 
      
      t_cont_release(t_cont);
      uint64_t tail = __sync_fetch_and_add(&c->t_mbuf.to_send.tail_, 0);

      if (cl->container_id < tail) {
         uint64_t head = __sync_fetch_and_add(&c->t_mbuf.to_send.head_, 0); 
         __sync_add_and_fetch(&cl->container_id, head - cl->container_id);
      } else {
         __sync_add_and_fetch(&cl->container_id, 1);
      }
   }

   pthread_exit(NULL);

cleanup:
   disconnect_client(c, cl);
   free(arg);
   return NULL;
}

/**
 * \brief This function runs in a separate thread and handles new client's connection requests.
 *
 * \param[in] arg Pointer to interface's private data structure.
 */
static void *accept_clients_thread(void *arg)
{
   char remoteIP[INET6_ADDRSTRLEN];
   struct sockaddr_storage remoteaddr; // client address
   struct client_s *cl;
   socklen_t addrlen;
   int newclient;
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) arg;
   struct sockaddr *tmpaddr;
   struct ucred ucred;
   uint32_t ucredlen = sizeof(struct ucred);
   uint32_t client_id = 0;
   struct pollfd pfds;

   /* handle new connections */
   addrlen = sizeof(remoteaddr);
   while (1) {
      if (c->is_terminated != 0) {
         break;
      }
      pfds = (struct pollfd) {.fd = c->server_sd, .events = POLLIN};

      if (poll(&pfds, 1, -1) == -1) {
         if (errno == EINTR) {
            if (c->is_terminated != 0) {
               break;
            }
            continue;
         } else {
            VERBOSE(CL_ERROR, "%s:%d unexpected error code %d", __func__, __LINE__, errno);
         }
      }

      cl = NULL;

      if (pfds.revents & POLLIN) {
         newclient = accept(c->server_sd, (struct sockaddr *) &remoteaddr, &addrlen);
         if (newclient == -1) {
            VERBOSE(CL_ERROR, "Accepting new client failed.");
         } else {
            if (c->socket_type == TRAP_IFC_TCPIP) {
               tmpaddr = (struct sockaddr *) &remoteaddr;
               switch (((struct sockaddr *) tmpaddr)->sa_family) {
                  case AF_INET:
                     client_id = ntohs(((struct sockaddr_in *) tmpaddr)->sin_port);
                     break;
                  case AF_INET6:
                     client_id = ntohs(((struct sockaddr_in6 *) tmpaddr)->sin6_port);
                     break;
               }
               VERBOSE(CL_VERBOSE_ADVANCED, "Client connected via TCP socket, port=%u", client_id);
            } else {
               if (getsockopt(newclient, SOL_SOCKET, SO_PEERCRED, &ucred, &ucredlen) == -1) {
                  goto refuse_client;
               }
               client_id = (uint32_t) ucred.pid;
               VERBOSE(CL_VERBOSE_ADVANCED, "Client connected via UNIX socket, pid=%ld", (long) ucred.pid);
            }

            VERBOSE(CL_VERBOSE_ADVANCED, "New connection from %s on socket %d",
                    inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*) &remoteaddr), remoteIP, INET6_ADDRSTRLEN),
                    newclient);

            cl = calloc(1, sizeof(struct client_s));
            if (cl == NULL) {
                VERBOSE(CL_VERBOSE_LIBRARY, "Client's memory allocation failed. Refuse connection.");
               goto refuse_client;
            }

            if (c->connected_clients < c->max_clients) {
               // lock interface to connect new client

               __sync_add_and_fetch(&c->clients_waiting_for_connection, 1);

               pthread_mutex_lock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);

               __sync_sub_and_fetch(&c->clients_waiting_for_connection, 1);

               cl->container_id = t_rb_head_id(&c->t_mbuf.to_send);
               cl->sd = newclient;
               cl->pfds_index = -1;
               cl->id = client_id;

#ifdef ENABLE_NEGOTIATION
               int ret_val = output_ifc_negotiation(c, TRAP_IFC_TYPE_TCPIP, newclient, NULL);
               if (ret_val == NEG_RES_OK) {
                  VERBOSE(CL_VERBOSE_LIBRARY, "Output_ifc_negotiation result: success.");
               } else if (ret_val == NEG_RES_FMT_UNKNOWN) {
                  VERBOSE(CL_VERBOSE_LIBRARY, "Output_ifc_negotiation result: failed (unknown data format of this output interface -> refuse client).");
                  pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
                  goto refuse_client;
               } else { // ret_val == NEG_RES_FAILED, sending the data to input interface failed, refuse client
                  VERBOSE(CL_VERBOSE_LIBRARY, "Output_ifc_negotiation result: failed (error while sending hello message to input interface).");
                  pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
                  goto refuse_client;
               }
#endif
               pthread_mutex_lock(&c->client_list_mtx);
               LIST_INSERT_HEAD(&c->clients_list_head, cl, entries);
               pthread_mutex_unlock(&c->client_list_mtx);
               struct thread_data *client_thread_data = malloc(sizeof(struct thread_data));
               if (client_thread_data == NULL) {
                	VERBOSE(CL_VERBOSE_LIBRARY, "Client's memory allocation failed. Refuse connection.");
                	pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
                	goto refuse_client;
               }

               client_thread_data->arg = c;
               client_thread_data->client = cl;

               if (c->timeout == TRAP_WAIT || c->timeout == TRAP_HALFWAIT) {
                  pthread_create(&cl->sender_thread_id, NULL, send_blocking_mode, client_thread_data);
               } else {
                  pthread_create(&cl->sender_thread_id, NULL, send_non_blocking_mode, client_thread_data);
               }
               
               __sync_add_and_fetch(&c->connected_clients, 1);
               pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
            } else {
               VERBOSE(CL_VERBOSE_LIBRARY, "Shutting down client we do not have additional resources (%u/%u)", 
                  c->connected_clients, c->max_clients);
refuse_client:
               shutdown(newclient, SHUT_RDWR);
               close(newclient);
               free(cl);
            }
         }
      }
   }

   pthread_exit(NULL);
}

static void 
finish_container(tcpip_sender_private_t *c, struct trap_mbuf_s *t_mbuf)
{
   t_cond_write_header(t_mbuf->active, t_mbuf->to_send.head_);
   uint64_t current_sleep = 1;
    
   if ((c->timeout == TRAP_WAIT || (c->timeout == TRAP_HALFWAIT && c->connected_clients))  
      && c->lowest_container_id <= t_mbuf->to_send.tail_ 
      && (t_mbuf->to_send.head_ - t_mbuf->to_send.tail_ >= t_mbuf->to_send.size - 1) ) {
repeat:
      c->lowest_container_id = find_lowest_container_id(c); 
      if (c->lowest_container_id <= t_mbuf->to_send.tail_) {
         if (c->is_terminated) {
            return;
         }
         if (__sync_add_and_fetch(&c->clients_waiting_for_connection, 0)) {
            pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
            while(__sync_add_and_fetch(&c->clients_waiting_for_connection, 0)) {
               usleep(1000);
            }
            pthread_mutex_lock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
            goto repeat;
         }
         current_sleep = calculate_sleep(current_sleep);
         usleep(current_sleep);
         goto repeat;
      }
   }        

    struct trap_container_s *old_container = t_rb_get_old_write_new(&t_mbuf->to_send, t_mbuf->active);
    if (old_container != NULL) {
      uint8_t ref = __sync_sub_and_fetch(&old_container->ref_counter, 0);
    	if (ref == 0) {
         t_cont_clear(old_container);
         t_stack_push(&t_mbuf->empty, old_container);
      } else {
	      ref = __sync_sub_and_fetch(&old_container->ref_counter, 1);
	      if (ref != 0) {
	         t_stack_push(&t_mbuf->deferred, old_container);
	      } else {
            __sync_sub_and_fetch(&old_container->idx, 1);
	         t_cont_clear(old_container);
	         t_stack_push(&t_mbuf->empty, old_container);
	      }
      }
   }

   c->autoflush_timestamp = get_cur_timestamp();
}

/**
 * \brief Force flush of active buffer
 *
 * \param[in] priv pointer to interface private data
 */
void tcpip_sender_flush(void *priv)
{
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;

   pthread_mutex_lock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
   struct trap_mbuf_s *t_mbuf = &c->t_mbuf;

   // buffer is empty, only header inside
   if (t_mbuf->active->used_bytes == TRAP_HEADER_SIZE) {
      pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
      return;
   }

   finish_container(c, t_mbuf);
   c->max_container_id++;
   struct trap_container_s *t_cont = t_mbuf_get_empty_container(t_mbuf);
   t_cond_set_seq_num(t_cont, t_mbuf->processed_messages);

   pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
   __sync_add_and_fetch(&c->ctx->counter_autoflush[c->ifc_idx], 1);
}

static void *autoflush_thread(void *priv)
{
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;
   int64_t time_since_flush;

   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

   while (!c->is_terminated) {
      if (c->connected_clients == 0) {
         usleep(c->ctx->out_ifc_list[c->ifc_idx].timeout);
         continue;
      }
      time_since_flush = get_cur_timestamp() - c->autoflush_timestamp;
      if (time_since_flush >= c->ctx->out_ifc_list[c->ifc_idx].timeout) {
         tcpip_sender_flush(c);
         usleep(c->ctx->out_ifc_list[c->ifc_idx].timeout);
      } else {
         usleep(c->ctx->out_ifc_list[c->ifc_idx].timeout - time_since_flush);    
      }
   }
   pthread_exit(NULL);
}

/**
 * \brief Store message into buffer.
 *
 * \param[in] priv      pointer to module private data
 * \param[in] data      pointer to data to write
 * \param[in] size      size of data to write
 * \param[in] timeout   maximum time spent waiting for the message to be stored [microseconds]
 *
 * \return TRAP_E_OK         Success.
 * \return TRAP_E_TIMEOUT    Message was not stored into buffer and the attempt should be repeated.
 * \return TRAP_E_TERMINATED Libtrap was terminated during the process.
 */
int tcpip_sender_send(void *priv, const void *data, uint16_t size, int timeout)
{
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;

   // Can we put message at least into empty buffer? 
   if (t_cont_has_capacity(size + sizeof(size)) == false) {
      VERBOSE(CL_ERROR, "Container is too small for message of size [%u B]. Skipping...", size);
      return TRAP_E_OK;
   }

repeat:
   if (c->is_terminated) {
      return TRAP_E_TERMINATED;
   }

   if (timeout == TRAP_WAIT && c->connected_clients == 0) {
      usleep(NO_CLIENTS_SLEEP);
      goto repeat;
   }

   // lock critical section
   pthread_mutex_lock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);      

   struct trap_mbuf_s *t_mbuf = &c->t_mbuf;
   struct trap_container_s *t_cont = t_mbuf->active;

   // check if container has enough space to insert new message
   // if not finish current container and get new empty one.
   if (t_cont_has_space(t_cont, size + sizeof(size))) {
      t_cont_insert(t_cont, data, size);
   } else {
      finish_container(c, t_mbuf);
      c->max_container_id++;
      t_cont = t_mbuf_get_empty_container(t_mbuf);
      t_cond_set_seq_num(t_cont, t_mbuf->processed_messages);
      t_cont_insert(t_cont, data, size);
   }

   /* If bufferswitch is 0, only 1 message is allowed to be stored in buffer */
   if (c->ctx->out_ifc_list[c->ifc_idx].bufferswitch == 0) {
      finish_container(c, t_mbuf);
      c->max_container_id++;
      t_cont = t_mbuf_get_empty_container(t_mbuf);
      t_cond_set_seq_num(t_cont, t_mbuf->processed_messages);
   }

   t_mbuf->processed_messages++;
   pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
   
   return TRAP_E_OK;
}

/**
 * \brief Set interface state as terminated.
 * \param[in] priv  pointer to module private data
 */
void tcpip_sender_terminate(void *priv)
{
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;

   /* Wait for connected clients to receive all finished buffers before terminating */
   if (c == NULL) {
      VERBOSE(CL_ERROR, "Destroying IFC that is probably not initialized.");
      return;
   }
   
   do {
      size_t lowest_container_id = find_lowest_container_id(c);
      if (lowest_container_id == c->t_mbuf.to_send.head_) {
         break;
      }

      if (c->connected_clients == 0) {
         break;
      }
      usleep(10000); //prevents busy waiting
   } while (true);
   c->is_terminated = 1;
   close(c->term_pipe[1]);
   VERBOSE(CL_VERBOSE_LIBRARY, "Closed term_pipe, it should break poll()");
   return;
}

/**
 * \brief Destructor of TCP sender (output ifc)
 * \param[in] priv  pointer to module private data
 */
void tcpip_sender_destroy(void *priv)
{
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;
   char *unix_socket_path = NULL;
   void *res;

   if (c == NULL) {
      return;
   }

#define X(x) free(x); x = NULL;
   // Free private data
   if (c->socket_type == TRAP_IFC_TCPIP_UNIX) {
      if (asprintf(&unix_socket_path, trap_default_socket_path_format, c->server_port) != -1) {
         if (unix_socket_path != NULL) {
            unlink(unix_socket_path);
            X(unix_socket_path);
         }
      }
   }
   if (c->server_port != NULL) {
      X(c->server_port);
   }
 
   if (c->initialized) {
      pthread_cancel(c->accept_thr);
      pthread_join(c->accept_thr, &res);
   }

   /* close server socket */
   close(c->server_sd);

   if (c->clients_pfds != NULL) {
      X(c->clients_pfds);
   }

   /* disconnect all clients */
   tcpip_server_disconnect_all_clients(priv);
   X(c)
#undef X
}

int32_t tcpip_sender_get_client_count(void *priv)
{
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;

   if (c == NULL) {
      return 0;
   }

   return c->connected_clients;
}

int8_t tcpip_sender_get_client_stats_json(void *priv, json_t *client_stats_arr)
{
   json_t *client_stats = NULL;
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;
   struct client_s *cl;

   if (c == NULL) {
      return 0;
   }

   char total_container_buf[40];
   char total_messages_buf[40];
   char total_missed_buf[40];

   sprintf(total_container_buf, "%" PRIu64, tcp_trap_stats.total_containers);
   sprintf(total_messages_buf, "%" PRIu64, tcp_trap_stats.total_msg);
   sprintf(total_missed_buf, "%" PRIu64, tcp_trap_stats.total_missed);

   client_stats = json_pack("{sssssssssf}", 
      "id", "extended_input_interface_info", 
      "sent_containers", total_container_buf, 
      "sent_messages", total_messages_buf, 
      "skipped_messages", total_missed_buf,
      "skipped_percentage", tcp_trap_stats.percentage_missed);
	if (client_stats == NULL) {
	   return 0;
	}

	if (json_array_append_new(client_stats_arr, client_stats) == -1) {
	   return 0;
	}


  	pthread_mutex_lock(&c->client_list_mtx);
   LIST_FOREACH(cl, &c->clients_list_head, entries) {
      char sent_container_buf[40];
      char sent_messages_buf[40];
      char skipped_messages_buf[40];

      sprintf(sent_container_buf, "%" PRIu64, cl->sent_containers);
      sprintf(sent_messages_buf, "%" PRIu64, cl->sent_messages);
      sprintf(skipped_messages_buf, "%" PRIu64, cl->skipped_messages);

      float skipped_percentage = 0;
      if (cl->skipped_messages) {
         skipped_percentage = ((100.0 / (cl->sent_messages + cl->skipped_messages)) * (cl->skipped_messages));
      }

    	client_stats = json_pack("{sisssssssf}", 
         "id", cl->id, 
         "sent_containers", sent_container_buf, 
         "sent_messages", sent_messages_buf, 
         "skipped_messages", skipped_messages_buf,
         "skipped_percentage", skipped_percentage);
	   if (client_stats == NULL) {
         pthread_mutex_unlock(&c->client_list_mtx);
	      return 0;
	   }

	   if (json_array_append_new(client_stats_arr, client_stats) == -1) {
         pthread_mutex_unlock(&c->client_list_mtx);
	      return 0;
	   }
   }
   pthread_mutex_unlock(&c->client_list_mtx);
   return 1;
}

static void tcpip_sender_create_dump(void *priv, uint32_t idx, const char *path)
{
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;
   /* return value */
   int r;
   /* config file trap-i<number>-config.txt */
   char *conf_file = NULL;
   FILE *f = NULL;
   client_t *cl;

   r = asprintf(&conf_file, "%s/trap-o%02"PRIu32"-config.txt", path, idx);
   if (r == -1) {
      VERBOSE(CL_ERROR, "Not enough memory, dump failed. (%s:%d)", __FILE__, __LINE__);
      conf_file = NULL;
      goto exit;
   }
   f = fopen(conf_file, "w");
   fprintf(f, "Server port: %s\n"
              "Server socket descriptor: %d\n"
              "Connected clients: %d\n"
              "Max clients: %d\n"
              "Buffer count: %u\n"
              "Buffer size: %u\n"
              "Terminated: %d\n"
              "Initialized: %d\n"
              "Socket type: %s\n"
              "Timeout: %u us\n",
              c->server_port,
              c->server_sd,
              c->connected_clients,
              c->max_clients,
              c->buffer_size,
              c->buffer_size,
              c->is_terminated,
              c->initialized,
              TCPIP_SOCKETTYPE_STR(c->socket_type),
              c->ctx->out_ifc_list[idx].datatimeout);
   fprintf(f, "Clients:\n");
   fprintf(f, "SD, Sent containers, sent messages, skipped messages, current container id:\n");
   pthread_mutex_lock(&c->client_list_mtx);
   LIST_FOREACH(cl, &c->clients_list_head, entries) {
      fprintf(f, "\t{%d, %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 "}\n", 
         cl->sd, cl->sent_containers, cl->sent_messages, cl->skipped_messages, cl->container_id);
   }
   pthread_mutex_unlock(&c->client_list_mtx);

   fclose(f);
exit:
   free(conf_file);
   return;
}

char *tcpip_send_ifc_get_id(void *priv)
{
   if (priv == NULL) {
      return NULL;
   }

   tcpip_sender_private_t *config = (tcpip_sender_private_t *) priv;
   if (config->server_port == NULL) {
      return NULL;
   }
   return config->server_port;
}

/**
 * \brief Constructor of output TCP/IP IFC module.
 * This function is called by TRAP library to initialize one output interface.
 *
 * \param[in,out] ctx  Pointer to the private libtrap context data (trap_ctx_init()).
 * \param[in] params   Configuration string containing interface specific parameters -
 * - tcp port/unix socket, max number of clients, buffer size, buffer count.
 * \param[in,out] ifc  IFC interface used for calling TCP/IP module.
 * \param[in] idx      Index of IFC that is created.
 * \param [in] type select the type of socket (see #tcpip_ifc_sockettype for options)
 * \return 0 on success (TRAP_E_OK)
 */
int create_tcpip_sender_ifc(trap_ctx_priv_t *ctx, const char *params, trap_output_ifc_t *ifc, uint32_t idx, enum tcpip_ifc_sockettype type)
{
   int result = TRAP_E_OK;
   char *param_iterator = NULL;
   char *param_str = NULL;
   char *server_port = NULL;
   tcpip_sender_private_t *priv = NULL;
   unsigned int max_clients = DEFAULT_MAX_CLIENTS;
   unsigned int buffer_count = DEFAULT_BUFFER_COUNT;
   unsigned int buffer_size = DEFAULT_BUFFER_SIZE;

#define X(pointer) free(pointer); \
   pointer = NULL;

   // Check parameter
   if (params == NULL) {
      VERBOSE(CL_ERROR, "IFC requires at least one parameter (%s).",
              type == TRAP_IFC_TCPIP ? "TCP port" : "UNIX socket name");
      return TRAP_E_BADPARAMS;
   }

   // Create structure to store private data
   priv = (tcpip_sender_private_t *) calloc(1, sizeof(tcpip_sender_private_t));
   if (priv == NULL) {
      result = TRAP_E_MEMORY;
      goto failsafe_cleanup;
   }

   /* Parsing params */
   param_iterator = trap_get_param_by_delimiter(params, &server_port, TRAP_IFC_PARAM_DELIMITER);
   if ((server_port == NULL) || (strlen(server_port) == 0)) {
      VERBOSE(CL_ERROR, "Missing 'port' for %s IFC.", (type == TRAP_IFC_TCPIP ? "TCPIP" : "UNIX socket"));
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }

   /* Optional params */
   while (param_iterator != NULL) {
      param_iterator = trap_get_param_by_delimiter(param_iterator, &param_str, TRAP_IFC_PARAM_DELIMITER);
      if (param_str == NULL)
         continue;
      if (strncmp(param_str, "buffer_count=x", BUFFER_COUNT_PARAM_LENGTH) == 0) {
         if (sscanf(param_str + BUFFER_COUNT_PARAM_LENGTH, "%u", &buffer_count) != 1) {
            VERBOSE(CL_ERROR, "Optional buffer count given, but it is probably in wrong format.");
            buffer_count = DEFAULT_BUFFER_COUNT;
         }
      } else if (strncmp(param_str, "buffer_size=x", BUFFER_SIZE_PARAM_LENGTH) == 0) {
         if (sscanf(param_str + BUFFER_SIZE_PARAM_LENGTH, "%u", &buffer_size) != 1) {
            VERBOSE(CL_ERROR, "Optional buffer size  given, but it is probably in wrong format.");
            buffer_size = DEFAULT_BUFFER_SIZE;
         }
      } else if (strncmp(param_str, "max_clients=x", MAX_CLIENTS_PARAM_LENGTH) == 0) {
         if (sscanf(param_str + MAX_CLIENTS_PARAM_LENGTH, "%u", &max_clients) != 1 || max_clients > 64) {
            VERBOSE(CL_ERROR, "Optional max clients number given, but it is probably in wrong format.");
            max_clients = DEFAULT_MAX_CLIENTS;
         }
      } else {
         VERBOSE(CL_ERROR, "Unknown parameter \"%s\".", param_str);
      }
      X(param_str);
   }
   /* Parsing params ended */

   if (t_mbuf_init(&priv->t_mbuf, buffer_count, max_clients)) {
      VERBOSE(CL_ERROR, "Trap mbuf initialization failed");
      result = TRAP_E_MEMORY;
      goto failsafe_cleanup;	
   }

   t_cont_set_len(buffer_size);

   priv->ctx = ctx;
   priv->timeout = ifc->datatimeout;
   priv->socket_type = type;
   priv->ifc_idx = idx;
   priv->server_port = server_port;
   priv->max_clients = max_clients;
   priv->connected_clients = 0;
   priv->is_terminated = 0;
   priv->autoflush_timestamp = get_cur_timestamp();

   VERBOSE(CL_VERBOSE_ADVANCED, "config:\nserver_port:\t%s\nmax_clients:\t%u\nactive_containers:\t%u\nbuffer size:\t%uB\n",
      priv->server_port, priv->max_clients, buffer_count, buffer_size);

   result = server_socket_open(priv);
   if (result != TRAP_E_OK) {
      VERBOSE(CL_ERROR, "Socket could not be opened on given port '%s'.", server_port);
      goto failsafe_cleanup;
   }

   if (pipe(priv->term_pipe) != 0) {
      VERBOSE(CL_ERROR, "Opening of pipe failed. Using stdin as a fall back.");
      priv->term_pipe[0] = 0;
   }

   // Fill struct defining the interface
   ifc->disconn_clients = tcpip_server_disconnect_all_clients;
   ifc->send = tcpip_sender_send;
   ifc->flush = tcpip_sender_flush;
   ifc->terminate = tcpip_sender_terminate;
   ifc->destroy = tcpip_sender_destroy;
   ifc->get_client_count = tcpip_sender_get_client_count;
   ifc->get_client_stats_json = tcpip_sender_get_client_stats_json;
   ifc->create_dump = tcpip_sender_create_dump;
   ifc->priv = priv;
   ifc->get_id = tcpip_send_ifc_get_id;
   return result;

failsafe_cleanup:
   X(server_port);
   X(param_str);
   if (priv != NULL) {
      if (priv->clients_pfds != NULL) {
         X(priv->clients_pfds);
      }
      t_mbuf_clear(&priv->t_mbuf);
      X(priv);
   }
#undef X
   return result;
}

/**
 * \brief Open TCPIP socket for sender module
 * \param[in] priv  tcpip_sender_private_t structure (private data)
 * \return 0 on success (TRAP_E_OK), TRAP_E_IO_ERROR on error
 */
static int server_socket_open(void *priv)
{
   int yes = 1;        // for setsockopt() SO_REUSEADDR, below
   int rv;

   union tcpip_socket_addr addr;
   struct addrinfo *ai, *p = NULL;
   tcpip_sender_private_t *c = (tcpip_sender_private_t *) priv;
   if (c->server_port == NULL) {
      return TRAP_E_BAD_FPARAMS;
   }

   memset(&addr, 0, sizeof(addr));

   if (c->socket_type == TRAP_IFC_TCPIP) {
      if (check_portrange(c->server_port) == EXIT_FAILURE) {
         return TRAP_E_BADPARAMS;
      }

      // get us a socket and bind it
      addr.tcpip_addr.ai_family = AF_UNSPEC;
      addr.tcpip_addr.ai_socktype = SOCK_STREAM;
      addr.tcpip_addr.ai_flags = AI_PASSIVE;
      if ((rv = getaddrinfo(NULL, c->server_port, &addr.tcpip_addr, &ai)) != 0) {
         return trap_errorf(c->ctx, TRAP_E_IO_ERROR, "selectserver: %s\n", gai_strerror(rv));
      }

      for (p = ai; p != NULL; p = p->ai_next) {
         c->server_sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
         if (c->server_sd < 0) {
            continue;
         }

         // lose the pesky "address already in use" error message
         if (setsockopt(c->server_sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            VERBOSE(CL_ERROR, "Failed to set socket to reuse address. (%d)", errno);
         }

         if (bind(c->server_sd, p->ai_addr, p->ai_addrlen) < 0) {
            close(c->server_sd);
            continue;
         }
         break; /* found socket to bind */
      }
      freeaddrinfo(ai); // all done with this
   } else if (c->socket_type == TRAP_IFC_TCPIP_UNIX) {
      /* UNIX socket */
      addr.unix_addr.sun_family = AF_UNIX;
      snprintf(addr.unix_addr.sun_path, sizeof(addr.unix_addr.sun_path) - 1, trap_default_socket_path_format, c->server_port);
      /* if socket file exists, it could be hard to create new socket and bind */
      unlink(addr.unix_addr.sun_path); /* error when file does not exist is not a problem */
      c->server_sd = socket(AF_UNIX, SOCK_STREAM, 0);
      if (c->server_sd != -1) {
         if (bind(c->server_sd, (struct sockaddr *) &addr.unix_addr, sizeof(addr.unix_addr)) != -1) {
            p = (struct addrinfo *) &addr.unix_addr;
            if (chmod(addr.unix_addr.sun_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == -1) {
               VERBOSE(CL_ERROR, "Failed to set permissions to socket (%s).", addr.unix_addr.sun_path);
            }
         } else {
            /* error bind() failed */
            p = NULL;
            VERBOSE(CL_ERROR, "Failed bind() with the following socket path: %s", addr.unix_addr.sun_path);
         }
      } else {
         VERBOSE(CL_ERROR, "Failed to create socket.");
         p = NULL;
      }
   }

   if (p == NULL) {
      // if we got here, it means we didn't get bound
      VERBOSE(CL_VERBOSE_LIBRARY, "selectserver: failed to bind");
      return TRAP_E_IO_ERROR;
   }

   // listen
   if (listen(c->server_sd, c->max_clients) == -1) {
      VERBOSE(CL_ERROR, "Listen failed");
      return TRAP_E_IO_ERROR;
   }

   if (pthread_create(&c->accept_thr, NULL, accept_clients_thread, priv) != 0) {
      VERBOSE(CL_ERROR, "Failed to create accept clients thread.");
      return TRAP_E_IO_ERROR;
   }

   if (pthread_create(&c->autoflush_thr, NULL, autoflush_thread, priv) != 0) {
      VERBOSE(CL_ERROR, "Failed to create autoflush thread.");
      return TRAP_E_IO_ERROR;
   }


   c->initialized = 1;
   return 0;
}

/**
 * @}
 *//* tcpip_sender */


/**
 * @}
 *//* tcpip module */

/**
 * @}
 *//* ifc modules */


// Local variables:
// c-basic-offset: 3
// End:
