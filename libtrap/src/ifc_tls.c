/**
 * \file ifc_tls.c
 * \brief TRAP TCP with TLS interfaces
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \author Jaroslav Hlavac <hlavaj20@fit.cvut.cz>
 * \date 2018
 */
/*
 * Copyright (C) 2013-2018 CESNET
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
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>


#include "../include/libtrap/trap.h"
#include "trap_internal.h"
#include "trap_ifc.h"
#include "trap_error.h"
#include "ifc_tls.h"
#include "ifc_tls_internal.h"
#include "ifc_socket_common.h"

/**
 * \addtogroup trap_ifc TRAP communication module interface
 * @{
 */
/**
 * \addtogroup tls_ifc TLS communication interface module
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

static SSL_CTX *tlsserver_create_context()
{
   const SSL_METHOD *method;
   SSL_CTX *ctx;

   method = SSLv23_server_method();

   ctx = SSL_CTX_new(method);
   if (!ctx) {
      perror("Unable to create SSL context");
      ERR_print_errors_fp(stderr);
      return NULL;
   }

#if defined(SSL_CTX_set_ecdh_auto)
   SSL_CTX_set_ecdh_auto(ctx, 1);
#else
   SSL_CTX_set_tmp_ecdh(ctx, EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
#endif

   return ctx;
}

static SSL_CTX *tlsclient_create_context()
{
   const SSL_METHOD *method;
   SSL_CTX *ctx = NULL;

   method = SSLv23_client_method();

   ctx = SSL_CTX_new(method);
   if (!ctx) {
      perror("Unable to create SSL context");
      ERR_print_errors_fp(stderr);
   }

   return ctx;
}

/**
 * \brief Verify context of ssl.
 * \param[in] arg  pointer to SSL (usually stored in tls_receiver_private_t resp. in an array of tlsclient_s inside tls_sender_private_t for input resp. output IFC)
 * \return 1 on failure, 0 on success
 * Disabling undesired versions of TLS/SSL and adding supported CAs to SSL_CTX.
 */
static int verify_certificate(SSL *arg)
{
   X509 *cert = NULL;
   int ret = 0;

   cert = SSL_get_peer_certificate(arg);
   if (cert == NULL) {
      VERBOSE(CL_ERROR, "Could not retrieve peer certificate file.");
      return EXIT_FAILURE;
   }
   
   if (SSL_get_verify_result(arg) == X509_V_OK) {
      ret = EXIT_SUCCESS;
   } else {
      ret = EXIT_FAILURE;
   }
   
   X509_free(cert);
   return ret;
}

/**
 * \brief Configure context of ssl server.
 * \param[in] cert  path to certfile
 * \param[in] ctx  ssl context to be configured
 * \return 1 on failure, 0 on success
 * Disabling undesired versions of TLS/SSL and adding supported CAs to SSL_CTX.
 */
static int tls_server_configure_ctx(const char *cert, SSL_CTX *ctx)
{
   X509* certificate = X509_new();
   BIO* bio_cert = BIO_new_file(cert, "r");

   PEM_read_bio_X509(bio_cert, &certificate, NULL, NULL);
   if (certificate == NULL) {
      VERBOSE(CL_ERROR, "Could not load certificate file.");
      return EXIT_FAILURE;
   }
   if (SSL_CTX_add_client_CA(ctx, certificate) != 1) {
      VERBOSE(CL_ERROR, "Could not add certificate to SSL_CTX.");
      return EXIT_FAILURE;
   }
   /* disabling undesired versions of TLS */
   SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
   SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3);
   SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1);
   SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1_1);

   X509_free(certificate);
   BIO_free_all(bio_cert);
   return EXIT_SUCCESS;
}

/**
 * \brief Configure ssl context of new connection.
 * \param[in] ctx  ssl context to be configured
 * \param[in] key  path to keyfile
 * \param[in] crt  path to certfile
 * \param[in] ca  path to CA file
 * \return 1 on failure, 0 on success
 * Loading certificate and key to SSL_CTX. Setting location of CA that is used for verification of
 * incomming certificates. Also forcing peer to send it's certificate.
 */
static int tls_configure_ctx(SSL_CTX *ctx, const char *key, const char *crt, const char *ca)
{
   int ret;

   /* Set the key and cert */
   ret = SSL_CTX_use_certificate_chain_file(ctx, crt);
   if (ret != 1) {
      VERBOSE(CL_ERROR, "Loading certificate (%s) failed. %s",
            crt, ERR_reason_error_string(ERR_get_error()));
      return EXIT_FAILURE;
   }

   ret = SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM);
   if (ret != 1) {
      VERBOSE(CL_ERROR, "Loading private key (%s) failed: %s",
            key, ERR_reason_error_string(ERR_get_error()));
      return EXIT_FAILURE;
   }

   if (SSL_CTX_check_private_key(ctx) == 0) {
      VERBOSE(CL_ERROR, "Private key does not match the certificate public key.");
      return EXIT_FAILURE;
   }
      

   if (SSL_CTX_load_verify_locations(ctx, ca, NULL) != 1) {
      VERBOSE(CL_ERROR, "Could not load CA location used for verification.");
      return EXIT_FAILURE;
   }
   SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

   return EXIT_SUCCESS;
}

/***** TCPIP server *****/

/**
 * Internal union for host address storage, common for tcpip & unix
 */
union tls_socket_addr {
   struct addrinfo tls_addr; ///< used for TCPIP socket
   struct sockaddr_un unix_addr; ///< used for path of UNIX socket
};

static int client_socket_connect(tls_receiver_private_t *priv, struct timeval *tv);
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
 * \addtogroup tls_receiver TLS Input IFC
 * @{
 */
/* Receiver (client socket) */
// Receiver is a client that connects itself to the source of data (to sender) = server

/**
 * Receive data
 * \param[in] priv      private IFC data
 * \param[out] data     received data
 * \param[in,out] size  expected size to wait for, it is used to return size that was not read
 * \param[in] tm        timeout
 */
static int receive_part(void *priv, void **data, uint32_t *size, struct timeval *tm)
{
   void *data_p = (*data);
   tls_receiver_private_t *config = (tls_receiver_private_t *) priv;
   ssize_t numbytes = *size;
   int recvb, retval;
   fd_set set;

   assert(data_p != NULL);

   while (config->is_terminated == 0) {
      DEBUG_IFC(if (tm) {VERBOSE(CL_VERBOSE_LIBRARY, "Try to receive data in timeout %" PRIu64
                        "s%"PRIu64"us", tm->tv_sec, tm->tv_usec)});

      FD_ZERO(&set);
      FD_SET(config->sd, &set);
      /*
       * Blocking or with timeout?
       * With timeout 0,0 - non-blocking
       */
      retval = select(config->sd + 1, &set, NULL, NULL, tm);
      if (retval > 0) {
         if (FD_ISSET(config->sd, &set)) {
            do {
               recvb = SSL_read(config->ssl, data_p, numbytes);
               if (recvb < 1) {
                  if (recvb == 0) {
                     errno = EPIPE;
                  }
                  switch (errno) {
                  case EINTR:
                     VERBOSE(CL_ERROR, "EINTR occured");
                     if (config->is_terminated == 1) {
                        client_socket_disconnect(priv);
                        return TRAP_E_TERMINATED;
                     }
                     break;
                  case EBADF:
                  case EPIPE:
                     client_socket_disconnect(priv);
                     return TRAP_E_IO_ERROR;
                  case EAGAIN:
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
         } else {
            continue;
         }
      } else if (retval == 0) {
         VERBOSE(CL_VERBOSE_LIBRARY, "Timeout elapsed - non-blocking call used.");
         (*size) = numbytes;
         return TRAP_E_TIMEOUT;
      } else if (retval < 0 && errno == EINTR) { /* signal received */
         /** \todo continue with timeout minus time already waited */
         VERBOSE(CL_VERBOSE_BASIC, "select interrupted");
         continue;
      } else { /* some error has occured */
         VERBOSE(CL_VERBOSE_OFF, "select() returned %i (%s)", retval, strerror(errno));
         client_socket_disconnect(priv);
         return TRAP_E_IO_ERROR;
      }
   }
   return TRAP_E_TERMINATED;
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
 * digraph fsm { label="tls_receiver_recv()";labelloc=t;
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
int tls_receiver_recv(void *priv, void *data, uint32_t *size, int timeout)
{
#ifdef LIMITED_RECOVERY
   uint32_t recovery = 0;
#endif
   /** messageframe contains header that is read (even partially) in HEAD_WAIT */
   trap_buffer_header_t messageframe;
   tls_receiver_private_t *config = (tls_receiver_private_t *) priv;
   void *p = &messageframe;
   struct timeval tm, *temptm;
   int retval;
   /* first timestamp for global timeout in this function...
    * in the RESET state, we should check the timeout given by caller
    * with elapsed time from entry_time.
    * Timeout is in microseconds... */
   struct timespec spec_time;

   clock_gettime(CLOCK_MONOTONIC, &spec_time);
   /* entry_time is in microseconds seconds -> secs * microsends + nanoseconds */
   uint64_t entry_time = spec_time.tv_sec * 1000000 + (spec_time.tv_nsec / 1000);
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
   temptm = (timeout==TRAP_WAIT?NULL:&tm);

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
         clock_gettime(CLOCK_MONOTONIC, &spec_time);
         curr_time =  spec_time.tv_sec * 1000000 + (spec_time.tv_nsec / 1000);
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
            clock_gettime(CLOCK_MONOTONIC, &spec_time);
            curr_time =  spec_time.tv_sec * 1000000 + (spec_time.tv_nsec / 1000);
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
         retval = client_socket_connect(config, temptm);
         if (retval == TRAP_E_FIELDS_MISMATCH) {
            config->connected = 1;
            return TRAP_E_FORMAT_MISMATCH;
         } else if (retval == TRAP_E_OK) {
            config->connected = 1;
            /* ok, wait for header as we planned */
         } else if (TRAP_E_BAD_CERT) {
            /* TODO replace with TRAP_E_IO_ERROR and change it in test_echo_reply.c */
            return TRAP_E_TERMINATED;
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
         config->data_wait_size = messageframe.data_length;
         config->ext_buffer_size = messageframe.data_length;
#ifdef ENABLE_CHECK_HEADER
         /* check if header is ok: */
         if (tls_check_header(&messageframe) == 0) {
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
void tls_receiver_terminate(void *priv)
{
   tls_receiver_private_t *config = (tls_receiver_private_t *) priv;
   if (config != NULL) {
      config->is_terminated = 1;
   } else {
      VERBOSE(CL_ERROR, "Bad parameter of tls_receiver_terminate()!");
   }
   return;
}


/**
 * \brief Destructor of TLS receiver (input ifc)
 * \param[in] priv  pointer to module private data
 */
void tls_receiver_destroy(void *priv)
{
   tls_receiver_private_t *config = (tls_receiver_private_t *) priv;
   if (config != NULL) {
      if (config->connected == 1) {
         close(config->sd);
      }
      free(config->ssl);
      free(config->sslctx);
      free(config->dest_addr);
      free(config->dest_port);
      free(config->keyfile);
      free(config->certfile);
      free(config->cafile);
      free(config);
   } else {
      VERBOSE(CL_ERROR, "Destroying IFC that is probably not initialized.");
   }
   return;
}

static void tls_receiver_create_dump(void *priv, uint32_t idx, const char *path)
{
   tls_receiver_private_t *c = (tls_receiver_private_t *) priv;
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
           "Terminated: %d\nSocket descriptor: %d\n"
           "Data pointer: %p\nData wait size: %"PRIu32"\nMessage header: %"PRIu32"\n"
           "Extern buffer pointer: %p\nExtern buffer data size: %"PRIu32"\n"
           "Timeout: %"PRId32"us (%s)\nPrivate key: %s\nCertificate: %s\n",
           c->dest_addr, c->dest_port, c->connected, c->is_terminated, c->sd,
           c->data_pointer, c->data_wait_size, c->int_mess_header.data_length,
           c->ext_buffer, c->ext_buffer_size,
           c->ctx->in_ifc_list[idx].datatimeout,
           TRAP_TIMEOUT_STR(c->ctx->in_ifc_list[idx].datatimeout),
           c->keyfile, c->certfile);
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

char *tls_recv_ifc_get_id(void *priv)
{
   if (priv == NULL) {
      return NULL;
   }

   tls_receiver_private_t *config = (tls_receiver_private_t *) priv;
   if (config->dest_port == NULL) {
      return NULL;
   }
   return config->dest_port;
}

uint8_t tls_recv_ifc_is_conn(void *priv)
{
   if (priv == NULL) {
      return 0;
   }
   tls_receiver_private_t *config = (tls_receiver_private_t *) priv;
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
 * \return 0 on success (TRAP_E_OK)
 */
int create_tls_receiver_ifc(trap_ctx_priv_t *ctx, char *params, trap_input_ifc_t *ifc, uint32_t idx)
{
   int result = TRAP_E_OK;
   char *param_iterator = NULL;
   char *dest_addr = NULL;
   char *dest_port = NULL;
   char *keyfile = NULL;
   char *certfile = NULL;
   char *cafile = NULL;
   tls_receiver_private_t *config = NULL;

   if (params == NULL) {
      VERBOSE(CL_ERROR, "IFC requires at least three parameters (port:keyfile:certfile).");
      return TRAP_E_BADPARAMS;
   }

   config = (tls_receiver_private_t *) calloc(1, sizeof(tls_receiver_private_t));
   if (config == NULL) {
      VERBOSE(CL_ERROR, "Failed to allocate internal memory for input IFC.");
      return TRAP_E_MEMORY;
   }
   config->ctx = ctx;
   config->is_terminated = 0;
   config->ifc_idx = idx;

   /* Parsing params */
   param_iterator = trap_get_param_by_delimiter(params, &dest_addr, TRAP_IFC_PARAM_DELIMITER);
   /* error! we expect 2 parameters */
   if ((dest_addr == NULL) || (strlen(dest_addr) == 0)) {
      VERBOSE(CL_ERROR, "Expected parameters: 'destination address:port:keyfile:certfile' are missing.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }
   if (param_iterator != NULL) {
      param_iterator = trap_get_param_by_delimiter(param_iterator, &dest_port, TRAP_IFC_PARAM_DELIMITER);
   } else {
      VERBOSE(CL_ERROR, "Missing 'dest_port', 'keyfile', 'certfile' and trusted 'CAfile' parameters.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }
   if (param_iterator != NULL) {
      param_iterator = trap_get_param_by_delimiter(param_iterator, &keyfile, TRAP_IFC_PARAM_DELIMITER);
   } else {
      VERBOSE(CL_ERROR, "Missing 'keyfile', 'certfile' and trusted 'CAfile' parameters.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }
   if (param_iterator != NULL) {
      param_iterator = trap_get_param_by_delimiter(param_iterator, &certfile, TRAP_IFC_PARAM_DELIMITER);
   } else {
      VERBOSE(CL_ERROR, "Missing 'certfile' and trusted 'CAfile' parameters.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }
   if (param_iterator != NULL) {
      param_iterator = trap_get_param_by_delimiter(param_iterator, &cafile, TRAP_IFC_PARAM_DELIMITER);
   } else {
      /* dest_addr skipped, move parameters */
      cafile = certfile;
      certfile = keyfile;
      keyfile = dest_port;
      dest_port = dest_addr;
      dest_addr = strdup("localhost");
      VERBOSE(CL_ERROR, "Only 3 parameters given, using 'localhost' as a destination address.");
   }

   /* set global buffer size */
   config->int_mess_header.data_length = DEFAULT_MAX_DATA_LENGTH;
   /* Parsing params ended */

   config->dest_addr = dest_addr;
   config->dest_port = dest_port;
   config->keyfile = keyfile;
   config->certfile = certfile;
   config->cafile = cafile;

   if ((config->dest_addr == NULL) || (config->dest_port == NULL)) {
      /* no delimiter found even if we expect two parameters */
      VERBOSE(CL_ERROR, "Malformed params for input IFC, missing destination address and port.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }

   VERBOSE(CL_VERBOSE_ADVANCED, "config:\ndest_addr=\"%s\"\ndest_port=\"%s\"\n"
           "TDU size: %u\n", config->dest_addr, config->dest_port,
           config->int_mess_header.data_length);

   config->sslctx = tlsclient_create_context();
   if (config->sslctx == NULL) {
      result = TRAP_E_MEMORY;
      goto failsafe_cleanup;
   }
   if (tls_configure_ctx(config->sslctx, keyfile, certfile, cafile) == EXIT_FAILURE) {
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }

   /*
    * In constructor, we do not know timeout yet.
    * Use 5 seconds to wait for connection to output interface.
    */
#ifndef ENABLE_NEGOTIATION
   int retval = 0;
   struct timeval tv = {5, 0};
   retval = client_socket_connect(config, &tv);
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
   ifc->recv = tls_receiver_recv;
   ifc->destroy = tls_receiver_destroy;
   ifc->terminate = tls_receiver_terminate;
   ifc->create_dump = tls_receiver_create_dump;
   ifc->priv = config;
   ifc->get_id = tls_recv_ifc_get_id;
   ifc->is_conn = tls_recv_ifc_is_conn;

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
   free(dest_addr);
   free(dest_port);
   free(keyfile);
   free(certfile);
   free(cafile);
   if (config != NULL && config->sslctx != NULL) {
      SSL_CTX_free(config->sslctx);
   }
   free(config);
   return result;
}

/**
 * Disconnect from output IFC.
 *
 * \param[in,out] priv  pointer to private structure of input IFC (client)
 */
static void client_socket_disconnect(void *priv)
{
   tls_receiver_private_t *config = (tls_receiver_private_t *) priv;
   DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv Disconnected."));
   if (config->connected == 1) {
      VERBOSE(CL_VERBOSE_BASIC, "TLS ifc client disconnecting");
      SSL_free(config->ssl);
      config->ssl = NULL;
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
   fd_set fdset;
   FD_ZERO(&fdset);
   FD_SET(sock, &fdset);
   VERBOSE(CL_VERBOSE_LIBRARY, "wait for connection");

   rv = select(sock + 1, NULL, &fdset, NULL, tv);
   if (rv == 1) {
      int so_error;
      socklen_t len = sizeof so_error;

      getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);

      if (so_error == 0) {
         return TRAP_E_OK;
      }
   }
   return TRAP_E_TIMEOUT;
}


/**
 * \brief client_socket is used as a receiver
 * \param[in] c  pointer to module private data
 * \param[in] tv  timeout
 * \return TRAP_E_OK on success
 */
static int client_socket_connect(tls_receiver_private_t *c, struct timeval *tv)
{
   int sockfd = -1, options;
   union tls_socket_addr addr;
   struct addrinfo *servinfo, *p = NULL;
   int rv, addr_count = 0;
   char s[INET6_ADDRSTRLEN];

   if ((c == NULL) || (c->dest_addr == NULL) || (c->dest_port == NULL)) {
      return TRAP_E_BAD_FPARAMS;
   }

   memset(&addr, 0, sizeof(addr));

   addr.tls_addr.ai_family = AF_UNSPEC;
   addr.tls_addr.ai_socktype = SOCK_STREAM;

   if ((rv = getaddrinfo(c->dest_addr, c->dest_port, &addr.tls_addr, &servinfo)) != 0) {
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

   /* loop through all the results and connect to the first we can */
   for (p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         continue;
      }
      if ((options = fcntl(sockfd, F_GETFL)) != -1) {
         if (fcntl(sockfd, F_SETFL, O_NONBLOCK | options) == -1) {
            VERBOSE(CL_ERROR, "Could not set socket to non-blocking.");
         }
      }
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         if (errno != EINPROGRESS && errno != EAGAIN) {
            DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv TLS ifc connect error %d (%s)", errno,
                     strerror(errno)));
            close(sockfd);
            sockfd = -1;
            continue;
         } else {
            rv = wait_for_connection(sockfd, tv);
            if (rv == TRAP_E_TIMEOUT) {
               if (c->is_terminated) {
                  rv = TRAP_E_TERMINATED;
                  break;
               }
               /* try another address */
               close(sockfd);
               sockfd = -1;
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
   /* there was no successfull connection for whole servinfo struct */
   if (p == NULL) {
      VERBOSE(CL_VERBOSE_LIBRARY, "recv client: Connection failed.");
      rv = TRAP_E_TIMEOUT;
   }

   /* catching all possible errors from setting up socket before atempting tls handshake */
   if (rv != TRAP_E_OK) {
      freeaddrinfo(servinfo);
      close(sockfd);
      return rv;
   }

   if (p != NULL) {
      if (inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s) != NULL) {
         VERBOSE(CL_VERBOSE_LIBRARY, "recv client: connected to %s", s);
      }
   }
   freeaddrinfo(servinfo);
   servinfo = NULL;

   c->sd = sockfd;
   c->ssl = SSL_new(c->sslctx);
   if (c->ssl == NULL) {
      VERBOSE(CL_ERROR, "Creating SSL structure failed: %s", ERR_reason_error_string(ERR_get_error()));
      return TRAP_E_MEMORY;
   }

   /* setting tcp socket to be used for ssl connection */
   if (SSL_set_fd(c->ssl, c->sd) != 1) {
      VERBOSE(CL_ERROR, "Setting SSL file descriptor to tcp socket failed: %s",
            ERR_reason_error_string(ERR_get_error()));
      return TRAP_E_IO_ERROR;
   }
   SSL_set_connect_state(c->ssl);

   do {
      rv = SSL_connect(c->ssl);
      if (rv < 1) {
         rv = ERR_get_error();
         switch (rv) {
         case SSL_ERROR_NONE:
         case SSL_ERROR_WANT_CONNECT:
         case SSL_ERROR_WANT_X509_LOOKUP:
         case SSL_ERROR_WANT_READ:
         case SSL_ERROR_WANT_WRITE:
            break;
         default:
            VERBOSE(CL_ERROR, "SSL connection failed, could be wrong certificate. %s",
                  ERR_reason_error_string(ERR_get_error()));
            SSL_free(c->ssl);
            c->ssl = NULL;
            close(c->sd);
            return TRAP_E_IO_ERROR;
         }
      }
   } while (rv < 1);
   VERBOSE(CL_VERBOSE_BASIC, "SSL successfully connected")

   int ret_ver = verify_certificate(c->ssl); /* server certificate verification */
   if (ret_ver != 0){
      VERBOSE(CL_VERBOSE_LIBRARY, "verify_certificate: failed to verify server's certificate");
      SSL_free(c->ssl);
      c->ssl = NULL;
      return TRAP_E_BAD_CERT;
   }

      /** Input interface negotiation */
#ifdef ENABLE_NEGOTIATION
      switch(input_ifc_negotiation(c, TRAP_IFC_TYPE_TLS)) {
      case NEG_RES_FMT_UNKNOWN:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: failed (unknown data format of the output interface).");
         close(sockfd);
         return TRAP_E_TIMEOUT;

      case NEG_RES_CONT:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success.");
         return TRAP_E_OK;

      case NEG_RES_FMT_CHANGED: /* used on format change with JSON */
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success (format has changed; it was not first negotiation).");
         return TRAP_E_OK;

      case NEG_RES_RECEIVER_FMT_SUBSET: /* used on format change with UniRec */
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success (required set of fields of the input interface is subset of the recevied format).");
         return TRAP_E_OK;

      case NEG_RES_SENDER_FMT_SUBSET: /* used on format change with UniRec */
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success (new recevied format specifier is subset of the old one; it was not first negotiation).");
         return TRAP_E_OK;

      case NEG_RES_FAILED:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: failed (error while receiving hello message from output interface).");
         return TRAP_E_FIELDS_MISMATCH;

      case NEG_RES_FMT_MISMATCH:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: failed (data type or data format specifier mismatch).");
         return TRAP_E_FIELDS_MISMATCH;

      default:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: default case");
         break;
      }
#endif


   return TRAP_E_OK;
}

/**
 * @}
 *//* tls_receiver */

/**
 * \addtogroup tls_sender TLS Output IFC
 * @{
 */

/**
 * \brief This function is called when a client was/is being disconnected.
 *
 * \param[in] priv Pointer to interface's private data structure.
 * \param[in] cl_id Index of the client in 'clients' array.
 */
static inline void disconnect_client(tls_sender_private_t *priv, int cl_id)
{
   int i;
   tlsclient_t *c = &priv->clients[cl_id];

   for (i = 0; i < priv->buffer_count; ++i) {
      del_index(&priv->buffers[i].clients_bit_arr, cl_id);
      if (priv->buffers[i].clients_bit_arr == 0) {
         pthread_cond_broadcast(&priv->cond_full_buffer);
      }
   }
   del_index(&priv->clients_bit_arr, cl_id);
   __sync_sub_and_fetch(&priv->connected_clients, 1);

   shutdown(c->sd, SHUT_RDWR);
   close(c->sd);
   SSL_free(c->ssl);
   c->sd = -1;
   c->ssl = NULL;
   c->pending_bytes = 0;
   c->sending_pointer = NULL;
}

/**
 * \brief Function disconnects all clients of the output interface whose private structure is passed via "priv" parameter.
 *
 * \param[in] priv Pointer to output interface private structure.
 */
void tls_server_disconnect_all_clients(void *priv)
{
   uint32_t i;
   tls_sender_private_t *c = (tls_sender_private_t *) priv;

   for (i = 0; i<c->clients_arr_size; i++) {
      if (c->clients[i].sd > 0) {
         disconnect_client(priv, i);
      }
   }
}

/**
 * Return current time in microseconds.
 *
 * This is used to get current timestamp in tls_sender_send().
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
 * \brief This function runs in a separate thread and handles new client's connection requests.
 *
 * \param[in] arg Pointer to interface's private data structure.
 */
static void *accept_clients_thread(void *arg)
{
   char remoteIP[INET6_ADDRSTRLEN];
   struct sockaddr_storage remoteaddr; /* client address */
   struct tlsclient_s *cl;
   socklen_t addrlen;
   int newclient, fdmax;
   fd_set scset;
   tls_sender_private_t *c = (tls_sender_private_t *) arg;
   int i;
   struct sockaddr *tmpaddr;
   uint32_t client_id = 0;

   /* handle new connections */
   addrlen = sizeof(remoteaddr);
   while (1) {
      if (c->is_terminated != 0) {
         break;
      }
      FD_ZERO(&scset);
      FD_SET(c->server_sd, &scset);
      fdmax = c->server_sd;

      if (select(fdmax + 1, &scset, NULL, NULL, NULL) == -1) {
         if (errno == EINTR) {
            if (c->is_terminated != 0) {
               break;
            }
            continue;
         } else {
            VERBOSE(CL_ERROR, "%s:%d unexpected error code %d", __func__, __LINE__, errno);
         }
      }

      if (FD_ISSET(c->server_sd, &scset)) {
         newclient = accept(c->server_sd, (struct sockaddr *) &remoteaddr, &addrlen);
         if (newclient == -1) {
            VERBOSE(CL_ERROR, "Accepting new client failed.");
         } else {
            tmpaddr = (struct sockaddr *) &remoteaddr;
            switch(((struct sockaddr *) tmpaddr)->sa_family) {
               case AF_INET:
                  client_id = ntohs(((struct sockaddr_in *) tmpaddr)->sin_port);
                  break;
               case AF_INET6:
                  client_id = ntohs(((struct sockaddr_in6 *) tmpaddr)->sin6_port);
                  break;
            }
            VERBOSE(CL_VERBOSE_ADVANCED, "New connection from %s on socket %d",
                    inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*) &remoteaddr), remoteIP, INET6_ADDRSTRLEN),
                    newclient);

            if (c->connected_clients < c->clients_arr_size) {
               cl = NULL;
               for (i = 0; i < c->clients_arr_size; ++i) {
                  if (c->clients[i].sd < 1) {
                     cl = &c->clients[i];
                     break;
                  }
               }
               if (cl == NULL) {
                  goto refuse_client;
               }
               cl->ssl = SSL_new(c->sslctx);
               if (cl->ssl == NULL) {
                  VERBOSE(CL_ERROR, "Creating SSL structure failed: %s", ERR_reason_error_string(ERR_get_error()));
                  goto refuse_client;
               }
               if (SSL_set_fd(cl->ssl, newclient) != 1) {
                  VERBOSE(CL_ERROR, "Setting SSL file descriptor to tcp socket failed: %s",
                          ERR_reason_error_string(ERR_get_error()));
                  SSL_free(cl->ssl);
                  cl->ssl = NULL;
                  goto refuse_client;
               }

               if (SSL_accept(cl->ssl) <= 0) {
                  ERR_print_errors_fp(stderr);
                  SSL_free(cl->ssl);
                  cl->ssl = NULL;
                  goto refuse_client;
               }

               /** Verifying SSL certificate of client. */
               int ret_ver = verify_certificate(cl->ssl);
               if (ret_ver != 0){
                  VERBOSE(CL_VERBOSE_LIBRARY, "verify_certificate: failed to verify client's certificate");
                  goto refuse_client;
               }

               cl->sd = newclient;
               cl->sending_pointer = NULL;
               cl->pending_bytes = 0;
               cl->timer_total = 0;
               cl->id = client_id;
               cl->assigned_buffer = c->active_buffer;
               cl->timeouts = 0;

#ifdef ENABLE_NEGOTIATION
               int ret_val = output_ifc_negotiation(c, TRAP_IFC_TYPE_TLS, i);
               if (ret_val == NEG_RES_OK) {
                  VERBOSE(CL_VERBOSE_LIBRARY, "Output_ifc_negotiation result: success.");
               } else if (ret_val == NEG_RES_FMT_UNKNOWN) {
                  VERBOSE(CL_VERBOSE_LIBRARY, "Output_ifc_negotiation result: failed (unknown data format of this output interface -> refuse client).");
                  cl->sd = -1;
                  goto refuse_client;
               } else { // ret_val == NEG_RES_FAILED, sending the data to input interface failed, refuse client
                  VERBOSE(CL_VERBOSE_LIBRARY, "Output_ifc_negotiation result: failed (error while sending hello message to input interface).");
                  cl->sd = -1;
                  goto refuse_client;
               }
#endif

               set_index(&c->clients_bit_arr, i);
               __sync_add_and_fetch(&c->connected_clients, 1);
            } else {
refuse_client:
               VERBOSE(CL_VERBOSE_LIBRARY, "Shutting down client we do not have additional resources (%u/%u)",
                       c->connected_clients, c->clients_arr_size);
               shutdown(newclient, SHUT_RDWR);
               close(newclient);
            }
         }
      }
   }
   pthread_exit(NULL);
}

/**
 * \brief Write buffer size to its header and shift active index.
 *
 * \param[in] priv Pointer to output interface private structure.
 * \param[in] buffer Pointer to the buffer.
 */
static inline void finish_buffer(tls_sender_private_t *priv, buffer_t *buffer)
{
   priv->autoflush_timestamp = get_cur_timestamp();

   if (buffer->clients_bit_arr == 0 && buffer->wr_index != 0) {
      uint32_t header = htonl(buffer->wr_index);
      memcpy(buffer->header, &header, sizeof(header));

      priv->active_buffer = (priv->active_buffer + 1) % priv->buffer_count;

      buffer->clients_bit_arr = priv->clients_bit_arr;
      buffer->wr_index = 0;
   }

   pthread_mutex_lock(&priv->mtx_no_data);
   pthread_cond_broadcast(&priv->cond_no_data);
   pthread_mutex_unlock(&priv->mtx_no_data);
}

/**
 * \brief Force flush of active buffer
 *
 * \param[in] priv pointer to interface private data
 */
void tls_sender_flush(void *priv)
{
   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   c->autoflush_timestamp = get_cur_timestamp();

   pthread_mutex_lock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);

   buffer_t *buffer = &c->buffers[c->active_buffer];
   if (buffer->clients_bit_arr == 0 && buffer->wr_index != 0) {
      finish_buffer(c, buffer);
      __sync_add_and_fetch(&c->ctx->counter_autoflush[c->ifc_idx], 1);
   }

   pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
}

/**
 * \brief Send data to client from his assigned buffer.
 *
 * \param[in] priv Pointer to iterface's private data structure.
 * \param[in] c Pointer to the client's structure.
 * \param[in] cl_id Client's index in the 'clients' array.
 *
 * \return TRAP_E_OK successfully sent.
 * \return TRAP_E_TERMINATED TRAP was terminated.
 * \return TRAP_E_IO_ERROR send failed although TRAP was not terminated.
 */
static inline int send_data(tls_sender_private_t *priv, tlsclient_t *c, uint32_t cl_id)
{
   int sent;
   /* Pointer to client's assigned buffer */
   buffer_t *buffer = &priv->buffers[c->assigned_buffer];

again:
   sent = SSL_write(c->ssl, c->sending_pointer, c->pending_bytes);

   if (sent < 0) {
      /* Send failed */
      if (priv->is_terminated != 0) {
         return TRAP_E_TERMINATED;
      }
      switch (SSL_get_error(c->ssl, sent)) {
      case SSL_ERROR_ZERO_RETURN:
      case SSL_ERROR_SYSCALL:
         return TRAP_E_IO_ERROR;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
         goto again;
      default:
         VERBOSE(CL_VERBOSE_OFF, "Unhandled error from ssl_write in send_data");
         return TRAP_E_IO_ERROR;
      }
   } else {
      c->pending_bytes -= sent;
      c->sending_pointer = (uint8_t *) c->sending_pointer + sent;

      /* Client received whole buffer */
      if (c->pending_bytes <= 0) {
         del_index(&buffer->clients_bit_arr, cl_id);
         if (buffer->clients_bit_arr == 0) {
            __sync_add_and_fetch(&priv->ctx->counter_send_buffer[priv->ifc_idx], 1);
            pthread_cond_broadcast(&priv->cond_full_buffer);
         }

         /* Assign client the next buffer in sequence */
         c->assigned_buffer = (c->assigned_buffer + 1) % priv->buffer_count;
      }
   }
   return TRAP_E_OK;
}

/**
 * \brief This function runs in a separate thread. It handles sending data
          to connected clients for TLS interface.
 * \param[in] priv pointer to interface private data
 */
static void *sending_thread_func(void *priv)
{
   uint32_t i, j;
   int res;
   int maxsd = -1;
   fd_set set, disset;
   tlsclient_t *cl;
   buffer_t *assigned_buffer;
   uint8_t buffer[DEFAULT_MAX_DATA_LENGTH];
   uint64_t send_entry_time;
   uint64_t send_exit_time;
   uint8_t waiting_clients;
   struct timeval select_timeout;

   tls_sender_private_t *c = (tls_sender_private_t *) priv;

   while (1) {
      if (c->is_terminated != 0) {
         pthread_exit(NULL);
      }
      if (c->connected_clients == 0) {
         usleep(NO_CLIENTS_SLEEP);
         continue;
      }

      if ((get_cur_timestamp() - c->autoflush_timestamp) > c->ctx->out_ifc_list[c->ifc_idx].timeout) {
         tls_sender_flush(c);
      }

      FD_ZERO(&disset);
      FD_ZERO(&set);
      waiting_clients = 0;
      select_timeout.tv_sec = 1;
      select_timeout.tv_usec = 0;

      /* Add term_pipe for reading into the disconnect client set */
      FD_SET(c->term_pipe[0], &disset);
      if (maxsd < c->term_pipe[0]) {
         maxsd = c->term_pipe[0];
      }

      /* Check whether clients are connected and there is data for them to receive. */
      for (i = j = 0; i < c->clients_arr_size; ++i) {
         if (j == c->connected_clients) {
            break;
         }

         if (check_index(c->clients_bit_arr, i) == 0) {
            continue;
         }

         ++j;

         cl = &(c->clients[i]);
         assigned_buffer = &c->buffers[cl->assigned_buffer];

         FD_SET(cl->sd, &disset);
         if (maxsd < cl->sd) {
            maxsd = cl->sd;
         }

         if (check_index(assigned_buffer->clients_bit_arr, i) == 0) {
            ++waiting_clients;
            continue;
         }

         if (cl->pending_bytes <= 0) {
            cl->sending_pointer = assigned_buffer->header;
            cl->pending_bytes = ntohl(*((uint32_t *) assigned_buffer->header)) + sizeof(uint32_t);
         }

         FD_SET(cl->sd, &set);
      }

      if (waiting_clients == c->connected_clients) {
         pthread_mutex_lock(&c->mtx_no_data);
         pthread_cond_wait(&c->cond_no_data, &c->mtx_no_data);
         pthread_mutex_unlock(&c->mtx_no_data);
         continue;
      }

      res = select(maxsd + 1, &disset, &set, NULL, &select_timeout);
      if (res < 0) {
         /* Select returned with an error */
         if (c->is_terminated == 0) {
            switch (errno) {
               case EINTR:
                  continue;
               default:
                  VERBOSE(CL_ERROR, "Sending thread: unexpected error in select (errno: %i)", errno);
                  pthread_exit(NULL);
            }
         } else {
            VERBOSE(CL_VERBOSE_ADVANCED, "Sending thread: terminating...");
            pthread_exit(NULL);
         }
      } else if (res == 0) {
         /* Select timed out - no client will be receiving */
         continue;
      }

      if (FD_ISSET(c->term_pipe[0], &disset)) {
         /* Sending was interrupted by terminate(), exit even from TRAP_WAIT function call. */
         VERBOSE(CL_VERBOSE_ADVANCED, "Sending was interrupted by terminate()");
         pthread_exit(NULL);
      }

      /* Check file descriptors. Disconnect "inactive" clients and send data to those designated by select */
      for (i = j = 0; i < c->clients_arr_size; ++i) {
         if (j == c->connected_clients) {
            break;
         }

         cl = &(c->clients[i]);
         if (cl->sd < 1) {
            continue;
         }

         ++j;

         /* Check if client is still connected */
         if (FD_ISSET(cl->sd, &disset)) {
            res = recv(cl->sd, buffer, DEFAULT_MAX_DATA_LENGTH, 0);
            if (res < 1) {
               disconnect_client(c, i);
               VERBOSE(CL_VERBOSE_LIBRARY, "Client %u disconnected", cl->id);
               continue;
            }
         }

         /* Check if client is ready for data */
         if (FD_ISSET(cl->sd, &set)) {
            send_entry_time = get_cur_timestamp();
            res = send_data(c, cl, i);
            send_exit_time = get_cur_timestamp();

            /* Measure how much time we spent sending to this client (in microseconds) */
            cl->timer_last = (send_exit_time - send_entry_time);
            cl->timer_total += cl->timer_last;

            if (res != TRAP_E_OK) {
               VERBOSE(CL_VERBOSE_OFF, "Disconnected client %d (ret val: %d)", cl->id, res);
               disconnect_client(c, i);
            }
         }
      }
   }
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
int tls_sender_send(void *priv, const void *data, uint16_t size, int timeout)
{
   int res, i;
   uint32_t free_bytes;
   struct timespec ts;
   buffer_t *buffer;

   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   uint8_t block = (timeout == TRAP_WAIT || (timeout == TRAP_HALFWAIT && c->connected_clients != 0)) ? 1 : 0;

   /* Can we put message at least into empty buffer? In the worst case, we could end up with SEGFAULT -> rather skip with error */
   if ((size + sizeof(size)) > c->buffer_size) {
      VERBOSE(CL_ERROR, "Buffer is too small for this message. Skipping...");
      goto timeout;
   }

   /* If timeout is wait or half wait, we need to set some valid timeout value (>= 0)*/
   if (timeout == TRAP_WAIT || timeout == TRAP_HALFWAIT) {
      timeout = 10000;
   }

repeat:
   if (c->is_terminated != 0) {
      return TRAP_E_TERMINATED;
   }
   if (block && c->connected_clients == 0) {
      usleep(NO_CLIENTS_SLEEP);
      goto repeat;
   }

   pthread_mutex_lock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
   buffer = &c->buffers[c->active_buffer];
   while (buffer->clients_bit_arr != 0) {
      clock_gettime(CLOCK_REALTIME, &ts);

      ts.tv_nsec += (ts.tv_sec * 1000000000L) + (timeout * 1000L);
      ts.tv_sec = (ts.tv_nsec / 1000000000L);
      ts.tv_nsec %= 1000000000L;

      /* Wait until woken up by sending thread or until timeout elapses */
      res = pthread_cond_timedwait(&c->cond_full_buffer, &c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx, &ts);
      switch (res) {
         case 0:
            /* Succesfully locked, buffer can be used */
            break;
         case ETIMEDOUT:
            /* Desired buffer is still full after timeout */
            if (block) {
               /* Blocking send, wait until buffer is free to use */
               pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
               goto repeat;
            } else {
               /* Non-blocking send, drop message or force buffer reset (not implemented) */
               goto timeout;
            }
         default:
            VERBOSE(CL_ERROR, "Unexpected error in pthread_mutex_timedlock()");
            goto timeout;
      }
   }

   /* Check if there is enough space in buffer */
   free_bytes = c->buffer_size - buffer->wr_index;
   if (free_bytes >= (size + sizeof(size))) {
      /* Store message into buffer */
      insert_into_buffer(buffer, data, size);

      /* If bufferswitch is 0, only 1 message is allowed to be stored in buffer */
      if (c->ctx->out_ifc_list[c->ifc_idx].bufferswitch == 0) {
         finish_buffer(c, buffer);
      }

      pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
      return TRAP_E_OK;
   } else {
      /* Not enough space for message, finish current buffer and try to store message into next buffer */
      finish_buffer(c, buffer);
      buffer = &c->buffers[c->active_buffer];

      pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
      goto repeat;
   }

timeout:
   for (i = 0; i < c->clients_arr_size; i++) {
      if (c->clients[i].sd > 0 && c->clients[i].assigned_buffer == c->active_buffer) {
         c->clients[i].timeouts++;
      }
   }
   pthread_mutex_unlock(&c->ctx->out_ifc_list[c->ifc_idx].ifc_mtx);
   return TRAP_E_TIMEOUT;
}

/**
 * \brief Set interface state as terminated.
 * \param[in] priv  pointer to module private data
 */
void tls_sender_terminate(void *priv)
{
   tls_sender_private_t *c = (tls_sender_private_t *) priv;

   uint32_t i;
   uint64_t sum;

   /* Wait for connected clients to receive all finished buffers before terminating */
   if (c != NULL) {
      do {
         usleep(10000); //prevents busy waiting
         sum = 0;
         for (i = 0; i < c->buffer_count; i++) {
            sum |= c->buffers[i].clients_bit_arr;
         }
      } while (sum != 0);

      c->is_terminated = 1;
      close(c->term_pipe[1]);
      VERBOSE(CL_VERBOSE_LIBRARY, "Closed term_pipe, it should break select()");
   } else {
      VERBOSE(CL_ERROR, "Destroying IFC that is probably not initialized.");
   }
   return;
}

/**
 * \brief Destructor of TCP sender (output ifc)
 * \param[in] priv  pointer to module private data
 */
void tls_sender_destroy(void *priv)
{
   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   void *res;
   int32_t i;

   /* free private data */
   if (c != NULL) {
      SSL_CTX_free(c->sslctx);
      free(c->server_port);
      free(c->keyfile);
      free(c->certfile);
      free(c->cafile);

      if (c->initialized) {
         pthread_cancel(c->send_thr);
         pthread_cancel(c->accept_thr);
         pthread_join(c->send_thr, &res);
         pthread_join(c->accept_thr, &res);
      }

      /* close server socket */
      close(c->server_sd);

      /* disconnect all clients */
      if (c->clients != NULL) {
         tls_server_disconnect_all_clients(priv);
         free(c->clients);
      }

      if (c->buffers != NULL) {
         for (i = 0; i < c->buffer_count; i++) {
            free(c->buffers[i].header);
         }
         free(c->buffers);
      }

      pthread_mutex_destroy(&c->mtx_no_data);
      pthread_cond_destroy(&c->cond_no_data);
      pthread_cond_destroy(&c->cond_full_buffer);
      free(c);
   }
}

int32_t tls_sender_get_client_count(void *priv)
{
   tls_sender_private_t *c = (tls_sender_private_t *) priv;

   if (c == NULL) {
      return 0;
   }

   return c->connected_clients;
}

int8_t tls_sender_get_client_stats_json(void* priv, json_t *client_stats_arr)
{
   int i;
   json_t *client_stats = NULL;
   tls_sender_private_t *c = (tls_sender_private_t *) priv;

   if (c == NULL) {
      return 0;
   }

   for (i = 0; i < c->clients_arr_size; ++i) {
      if (check_index(c->clients_bit_arr, i) == 0) {
         continue;
      }

      client_stats = json_pack("{sisisisi}", "id", c->clients[i].id, "timer_total", c->clients[i].timer_total, "timer_last", c->clients[i].timer_last, "timeouts", c->clients[i].timeouts);
      if (client_stats == NULL) {
         return 0;
      }

      if (json_array_append_new(client_stats_arr, client_stats) == -1) {
         return 0;
      }
   }
   return 1;
}

static void tls_sender_create_dump(void *priv, uint32_t idx, const char *path)
{
   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   /* return value */
   int r;
   /* config file trap-i<number>-config.txt */
   char *conf_file = NULL;
   FILE *f = NULL;
   int32_t i;
   tlsclient_t *cl;

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
              "Active buffer: %d\n"
              "Buffer count: %u\n"
              "Buffer size: %u\n"
              "Terminated: %d\n"
              "Initialized: %d\n"
              "Timeout: %u us\n",
           c->server_port,
           c->server_sd,
           c->connected_clients,
           c->clients_arr_size,
           c->active_buffer,
           c->buffer_size,
           c->buffer_size,
           c->is_terminated,
           c->initialized,
           c->ctx->out_ifc_list[idx].datatimeout);
   fprintf(f, "Clients:\n");
   for (i = 0; i < c->clients_arr_size; i++) {
      cl = &c->clients[i];
      fprintf(f, "\t{%d, %d, %p, %d}\n", cl->sd, cl->assigned_buffer, cl->sending_pointer, cl->pending_bytes);
   }
   fclose(f);
exit:
   free(conf_file);
   return;
}

char *tls_send_ifc_get_id(void *priv)
{
   if (priv == NULL) {
      return NULL;
   }

   tls_sender_private_t *config = (tls_sender_private_t *) priv;
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
 * \param[in] params   Configuration string containing space separated values of these parameters (in this exact order): *server_port* *max_clients*,
 * where dest_addr is destination address of output TCP/IP IFC module and
 * dest_port is the port where sender is listening.
 * \param[in,out] ifc  IFC interface used for calling TCP/IP module.
 * \param[in] idx      Index of IFC that is created.
 * \return 0 on success (TRAP_E_OK)
 */
int create_tls_sender_ifc(trap_ctx_priv_t *ctx, const char *params, trap_output_ifc_t *ifc, uint32_t idx)
{
   int result = TRAP_E_OK;
   char *param_iterator = NULL;
   char *param_str = NULL;
   char *server_port = NULL;
   char *keyfile = NULL;
   char *certfile = NULL;
   char *cafile = NULL;
   tls_sender_private_t *priv = NULL;
   unsigned int max_clients = DEFAULT_MAX_CLIENTS;
   unsigned int buffer_count = DEFAULT_BUFFER_COUNT;
   unsigned int buffer_size = DEFAULT_BUFFER_SIZE;
   uint32_t i;

#define X(pointer) free(pointer); \
   pointer = NULL;

   /* Check parameters */
   if (params == NULL) {
      VERBOSE(CL_ERROR, "IFC requires at least three parameters (port:keyfile:certfile).");
      return TRAP_E_BADPARAMS;
   }

   /* Create structure to store private data */
   priv = (tls_sender_private_t *) calloc(1, sizeof(tls_sender_private_t));
   if (priv == NULL) {
      result = TRAP_E_MEMORY;
      goto failsafe_cleanup;
   }

   /* Parsing params */
   param_iterator = trap_get_param_by_delimiter(params, &server_port, TRAP_IFC_PARAM_DELIMITER);
   if ((server_port == NULL) || (strlen(server_port) == 0)) {
      VERBOSE(CL_ERROR, "Missing 'port' for TLS IFC.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }
   if (param_iterator != NULL) {
      param_iterator = trap_get_param_by_delimiter(param_iterator, &keyfile, TRAP_IFC_PARAM_DELIMITER);
   } else {
      VERBOSE(CL_ERROR, "Missing 'keyfile', 'certfile' and trusted 'CAfile' for TLS IFC.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }
   if (param_iterator != NULL) {
      param_iterator = trap_get_param_by_delimiter(param_iterator, &certfile, TRAP_IFC_PARAM_DELIMITER);
   } else {
      VERBOSE(CL_ERROR, "Missing 'certfile' and trusted 'CAfile' for TLS IFC.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }
   if (param_iterator != NULL) {
      param_iterator = trap_get_param_by_delimiter(param_iterator, &cafile, TRAP_IFC_PARAM_DELIMITER);
   } else {
      VERBOSE(CL_ERROR, "Missing trusted 'CAfile' for TLS IFC.");
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
         if (sscanf(param_str + MAX_CLIENTS_PARAM_LENGTH, "%u", &max_clients) != 1) {
            VERBOSE(CL_ERROR, "Optional max clients number given, but it is probably in wrong format.");
            max_clients = DEFAULT_MAX_CLIENTS;
         }
      } else {
         VERBOSE(CL_ERROR, "Unknown parameter \"%s\".", param_str);
      }
      X(param_str);
   }
   /* Parsing params ended */

   priv->buffers = calloc(buffer_count, sizeof(buffer_t));
   if (priv->buffers == NULL) {
      /* if some memory could not have been allocated, we cannot continue */
      goto failsafe_cleanup;
   }
   for (i = 0; i < buffer_count; ++i) {
      buffer_t *b = &(priv->buffers[i]);

      b->header = malloc(buffer_size + sizeof(buffer_size));
      b->data = b->header + sizeof(buffer_size);
      b->wr_index = 0;
      b->clients_bit_arr = 0;
   }

   priv->clients = calloc(max_clients, sizeof(tlsclient_t));
   if (priv->clients == NULL) {
      /* if some memory could not have been allocated, we cannot continue */
      goto failsafe_cleanup;
   }
   for (i = 0; i < max_clients; ++i) {
      tlsclient_t *client = &(priv->clients[i]);

      client->assigned_buffer = 0;
      client->sd = -1;
      client->timer_total = 0;
      client->pending_bytes = 0;
      client->sending_pointer = NULL;
   }

   priv->keyfile = keyfile;
   priv->certfile = certfile;
   priv->cafile = cafile;
   priv->ctx = ctx;
   priv->ifc_idx = idx;
   priv->server_port = server_port;
   priv->buffer_size = buffer_size;
   priv->buffer_count = buffer_count;
   priv->clients_arr_size = max_clients;
   priv->clients_bit_arr = 0;
   priv->connected_clients = 0;
   priv->is_terminated = 0;
   priv->active_buffer = 0;
   priv->autoflush_timestamp = get_cur_timestamp();

   pthread_mutex_init(&priv->mtx_no_data, NULL);
   pthread_cond_init(&priv->cond_no_data, NULL);
   pthread_cond_init(&priv->cond_full_buffer, NULL);

   VERBOSE(CL_VERBOSE_ADVANCED, "config:\nserver_port:\t%s\nmax_clients:\t%u\nbuffer count:\t%u\nbuffer size:\t%uB\n",
                                priv->server_port, priv->clients_arr_size,priv->buffer_count, priv->buffer_size);

   result = server_socket_open(priv);
   if (result != TRAP_E_OK) {
      VERBOSE(CL_ERROR, "Socket could not be opened on given port '%s'.", server_port);
      goto failsafe_cleanup;
   }

   if (pipe(priv->term_pipe) != 0) {
      VERBOSE(CL_ERROR, "Opening of pipe failed. Using stdin as a fall back.");
      priv->term_pipe[0] = 0;
   }

   priv->sslctx = tlsserver_create_context();
   if (priv->sslctx == NULL) {
      result = TRAP_E_MEMORY;
      goto failsafe_cleanup;
   }

   if (tls_server_configure_ctx(priv->certfile, priv->sslctx) != 0) {
      VERBOSE(CL_ERROR, "Configuring server context failed.");
      goto failsafe_cleanup;
   }

   if (tls_configure_ctx(priv->sslctx, keyfile, certfile, cafile) == EXIT_FAILURE) {
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }

   /* Fill struct defining the interface */
   ifc->disconn_clients = tls_server_disconnect_all_clients;
   ifc->send = tls_sender_send;
   ifc->flush = tls_sender_flush;
   ifc->terminate = tls_sender_terminate;
   ifc->destroy = tls_sender_destroy;
   ifc->get_client_count = tls_sender_get_client_count;
   ifc->get_client_stats_json = tls_sender_get_client_stats_json;
   ifc->create_dump = tls_sender_create_dump;
   ifc->priv = priv;
   ifc->get_id = tls_send_ifc_get_id;
   return result;

failsafe_cleanup:
   X(server_port);
   X(param_str);
   X(certfile);
   X(cafile);
   X(keyfile);
   if (priv != NULL) {
      if (priv->buffers != NULL) {
         for (i = 0; i < priv->buffer_count; i++) {
            X(priv->buffers[i].header);
         }
         X(priv->buffers)
      }
      if (priv->clients != NULL) {
         X(priv->clients);
      }
      pthread_mutex_destroy(&priv->mtx_no_data);
      pthread_cond_destroy(&priv->cond_no_data);
      pthread_cond_destroy(&priv->cond_full_buffer);
      X(priv);
   }
#undef X
   return result;
}

/**
 * \brief Open TLS socket for sender module
 * \param[in] priv  tls_sender_private_t structure (private data)
 * \return 0 on success (TRAP_E_OK), TRAP_E_IO_ERROR on error
 */
static int server_socket_open(void *priv)
{
   int yes = 1;        /* for setsockopt() SO_REUSEADDR, below */
   int rv;

   union tls_socket_addr addr;
   struct addrinfo *ai, *p = NULL;
   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   if (c->server_port == NULL) {
      return TRAP_E_BAD_FPARAMS;
   }

   memset(&addr, 0, sizeof(addr));

   /* get us a socket and bind it */
   addr.tls_addr.ai_family = AF_UNSPEC;
   addr.tls_addr.ai_socktype = SOCK_STREAM;
   addr.tls_addr.ai_flags = AI_PASSIVE;
   if ((rv = getaddrinfo(NULL, c->server_port, &addr.tls_addr, &ai)) != 0) {
      return trap_errorf(c->ctx, TRAP_E_IO_ERROR, "selectserver: %s\n", gai_strerror(rv));
   }

   for (p = ai; p != NULL; p = p->ai_next) {
      c->server_sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (c->server_sd < 0) {
         continue;
      }

      /* lose the pesky "address already in use" error message */
      if (setsockopt(c->server_sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
         VERBOSE(CL_ERROR, "Failed to set socket to reuse address. (%d)", errno);
      }

      if (bind(c->server_sd, p->ai_addr, p->ai_addrlen) < 0) {
         close(c->server_sd);
         continue;
      }
      break; /* found socket to bind */
   }
   freeaddrinfo(ai);

   if (p == NULL) {
      /* if we got here, it means we didn't get bound */
      VERBOSE(CL_VERBOSE_LIBRARY, "selectserver: failed to bind");
      return TRAP_E_IO_ERROR;
   }

   /* listen */
   if (listen(c->server_sd, c->clients_arr_size) == -1) {
      VERBOSE(CL_ERROR, "Listen failed");
      return TRAP_E_IO_ERROR;
   }

   if (pthread_create(&c->send_thr, NULL, sending_thread_func, priv) != 0) {
      VERBOSE(CL_ERROR, "Failed to create sending thread.");
      return TRAP_E_IO_ERROR;
   }

   if (pthread_create(&c->accept_thr, NULL, accept_clients_thread, priv) != 0) {
      VERBOSE(CL_ERROR, "Failed to create accept_thread.");
      return TRAP_E_IO_ERROR;
   }

   c->initialized = 1;
   return 0;
}

/**
 * @}
 *//* tls_sender */


/**
 * @}
 *//* tls_ifc module */

/**
 * @}
 *//* ifc modules */

