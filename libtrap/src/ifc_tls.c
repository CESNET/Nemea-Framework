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

#define DEFAULT_MAX_DATA_LENGTH  (sizeof(trap_buffer_header_t) + 1024)
#define MAX_CLIENTS_ARR_SIZE     10

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
 * \brief Destructor of TCPIP receiver (input ifc)
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
      VERBOSE(CL_VERBOSE_BASIC, "TCPIP ifc client disconnecting");
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
            DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "recv TCPIP ifc connect error %d (%s)", errno,
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

static void server_disconnected_client(tls_sender_private_t *c, int cl_id)
{
   struct tlsclient_s *cl = &c->clients[cl_id];
   pthread_mutex_lock(&c->lock);
   SSL_free(cl->ssl);
   cl->ssl = NULL;
   close(cl->sd);
   cl->sd = -1;
   cl->client_state = TLSCURRENT_IDLE;
   c->connected_clients--;
   pthread_mutex_unlock(&c->lock);
}

/**
 * \brief Try to send data block at once
 *
 * \param [in] c  private data
 * \param [in] cl  pointer to client structure
 * \param [in,out] data pointer to beginning of data
 * \param [in,out] size size of data to send and the rest unsent size of data
 * \param [in] block       1 if blocking, 0 if non-blocking
 * \return TRAP_E_OK, TRAP_E_TIMEOUT, TRAP_E_TERMINATED, TRAP_E_IO_ERROR
 */
static int send_all_data(tls_sender_private_t *c, struct tlsclient_s *cl, void **data, uint32_t *size, char block)
{
   void *p = (*data);
   ssize_t numbytes = (*size), sent_b;
   int res = TRAP_E_TERMINATED;

again:
   sent_b = SSL_write(cl->ssl, p, numbytes);
   if (sent_b <= 0) {
      switch (SSL_get_error(cl->ssl, sent_b)) {
      case SSL_ERROR_ZERO_RETURN:
      case SSL_ERROR_SYSCALL:
         VERBOSE(CL_VERBOSE_OFF, "Disconnected client (%i)", errno);
         res = TRAP_E_IO_ERROR;
         goto failure;

      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
         if (block == 1) {
            usleep(NONBLOCKING_MINWAIT);
            goto again;
         }
         break;
      }
      if (c->is_terminated == 1) {
         res = TRAP_E_TERMINATED;
         goto failure;
      }
   } else if (sent_b > 0) {
      numbytes -= sent_b;
      p += sent_b;
      DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "send sent: %"PRId64" B remaining: "
                "%"PRIu64" B from %p (errno %"PRId32")",
                sent_b, numbytes, p, errno));
   }
   assert(numbytes>=0);
   (*size) = numbytes;
   if (numbytes > 0) {
      if (block == 1) {
         goto again;
      }
      (*data) = p;
      return TRAP_E_TIMEOUT;
   } else if (numbytes == 0) {
      (*data) = NULL;
      return TRAP_E_OK;
   }
failure:
   (*data) = NULL;
   (*size) = 0;
   return res;
}

/**
 * Check if any client is connected.
 * \return non-zero if there is a connected client
 */
static inline char check_connected_clients(tls_sender_private_t *config)
{
   pthread_mutex_lock(&config->lock);
   if (config->connected_clients == 0) {
      pthread_mutex_unlock(&config->lock);
      return 0;
   }
   pthread_mutex_unlock(&config->lock);
   return 1;
}

typedef enum tls_sender_result {
   EVERYBODY_PASSED,
   EVERYBODY_TIMEDOUT,
   SOMEBODY_TIMEDOUT
} tls_sender_result_t;

/**
 * \brief Check if we have connected clients and wait for them.
 *
 * The function is blocking or non-blocking.
 * \param [in] config   private data
 * \param [in] t        pointer to timeout for sem_timedwait(), NULL when blocking
 * \return TRAP_E_OK - we have clients, TRAP_E_TIMEOUT - we don't have clients, TRAP_E_TERMINATED - terminating
 */
static inline int tls_sender_conn_phase(tls_sender_private_t *config, struct timespec *t)
{
   int res;
   assert(t != NULL);

   if (check_connected_clients(config) == 0) {
      /* there is no connected client */
      res = sem_timedwait(&config->have_clients, t);
      if (res == -1) {
         if (errno == EINTR) {
            DEBUG_IFC(VERBOSE(CL_ERROR, "interrupt."));
            if (config->is_terminated != 0) {
               return TRAP_E_TERMINATED;
            }
         } else if (errno == ETIMEDOUT) {
            return TRAP_E_TIMEOUT; /* no client after timeout */
         } else {
            VERBOSE(CL_ERROR, "sem_timedwait failed (%d): %s", errno, strerror(errno));
         }
      }
      /* timeout or connected client */
      if (check_connected_clients(config) == 0) {
         return TRAP_E_TIMEOUT; /* no client after timeout */
      }
   }
   /* connected client */
   return TRAP_E_OK;
}

/**
 * \brief Send data to all connected clients.
 *
 * All clients are in 'current' buffer or in 'backup' buffer.
 * All clients get messages from current buffer at first.
 * * When all buffer is successfully sent to all clients
 * in 'current' buffer, everything is alright.
 *
 * * When everybody fails due to timeout, everything
 * is alright, everybody stays in 'current' buffer.
 *
 * * When somebody fails and somebody is successful, failing clients are moved into 'backup' buffer and they have chance to receive the rest of message later. We disconnect all clients that were in 'backup' buffer before moving new ones from 'current' buffer.
 *
 * * All successful clients from 'backup' buffer are moved back into 'current' buffer.
 *
 * \param[in] priv  pointer to module private data
 * \param[in] data  pointer to data to send
 * \param[in] size  size of data to send
 * \param[in] timeout  timeout in microseconds
 * \return 0 on success (TRAP_E_OK), TRAP_E_BAD_FPARAMS if sender was not properly initialized, TRAP_E_TERMINATED if interface was terminated.
 */
int tls_sender_send(void *priv, const void *data, uint32_t size, int timeout)
{
   uint8_t buffer[DEFAULT_MAX_DATA_LENGTH];
   int result = TRAP_E_TIMEOUT;
   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   /* timeout for select */
   struct timeval tv;
   /* timeout for sem_timedwait */
   struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
   fd_set set, disset;
   int maxsd = -1;
   struct tlsclient_s *cl;
   uint32_t i, j, failed, passed;
   int retval;
   ssize_t readbytes;

   char block = ((timeout == TRAP_WAIT || timeout == TRAP_HALFWAIT) ? 1 : 0);

   /* pointer to timeout for select() */
   struct timeval *tv_p = ((block != 0) ? NULL : &tv);
   /* pointer to timeout for sem_timedwait() */
   struct timespec *ts_p = &ts;

   /* correct module will pass only possitive timeout or TRAP_WAIT, TRAP_HALFWAIT */
   assert(timeout >= TRAP_HALFWAIT);

   /* I. Init phase: set timeout and double-send switch */

blocking_repeat:

   if (c->is_terminated) {
      return TRAP_E_TERMINATED;
   }

   switch (timeout) {
   case TRAP_WAIT:
      trap_set_timeouts(1000000, &tv, &ts);
      break;
   case TRAP_HALFWAIT:
      /*
       * wait 1s in a loop for select(),
       * do not change timeout for sem_timedwait in connphase
       */
      trap_set_timeouts(1000000, &tv, NULL);
      break;
   default:
      /*
       * set timeout (can be 0 - nowait or any positive number) for select(),
       * do not change timeout for sem_timedwait in connphase
       */
      trap_set_timeouts(timeout, &tv, NULL);
      break;
   }

   /* Check connected clients and wait for them when blocking */
   result = tls_sender_conn_phase(c, ts_p);
   if (result != TRAP_E_OK) {
      /* it is useless to send when nobody receives */
      goto exit;
   }

   FD_ZERO(&disset);
   FD_ZERO(&set);

   /*
    * add term_pipe for reading into the disconnect client set
    */
   FD_SET(c->term_pipe[0], &disset);
   if (maxsd < c->term_pipe[0]) {
      maxsd = c->term_pipe[0];
   }

   for (i = 0, j = 0; i < c->clients_arr_size; ++i) {
      cl = &c->clients[i];
      if (j == c->connected_clients) {
         break;
      }
      if (cl->sd <= 0) {
         /* not connected client */
         continue;
      }
      j++;
      if (maxsd < cl->sd) {
         maxsd = cl->sd;
      }
      /* check if client is still connected */
      FD_SET(cl->sd, &disset);
      if (cl->client_state != TLSCURRENT_COMPLETE) {
         /* check if client is ready for message */
         FD_SET(cl->sd, &set);
      }
   }

   retval = select(maxsd + 1, &disset, &set, NULL, tv_p);
   if (retval == 0) {
      if (block == 0) {
         /* non-blocking mode */
         result = TRAP_E_TIMEOUT;
         goto exit;
      } else {
         /* blocking mode */
         goto blocking_repeat;
      }
   } else if (retval < 0) {
      if (c->is_terminated != 0) {
         goto exit;
      } else if (errno == EBADF) {
         assert(0);
         result = TRAP_E_IO_ERROR;
         goto exit;
      }
   }

   passed = failed = 0;

   pthread_mutex_lock(&c->sending_lock);
   for (i = 0, j = 0; i < c->clients_arr_size; ++i) {
      if (j == c->connected_clients) {
         break;
      }
      cl = &c->clients[i];
      if(cl->sd == -1) {
         continue;
      }
      if (FD_ISSET(cl->sd, &disset)) {
         /* client disconnects */
         readbytes = recv(cl->sd, buffer, DEFAULT_MAX_DATA_LENGTH, MSG_NOSIGNAL | MSG_DONTWAIT);
         if (readbytes < 1) {
            VERBOSE(CL_VERBOSE_LIBRARY, "Disconnected client.");
            result = TRAP_E_IO_ERROR;
            server_disconnected_client(c, i);
            continue;
         }
      }

      if (FD_ISSET(cl->sd, &set)) {
         if (j == c->connected_clients) {
            break;
         }
         /* we added only clients whose sending is not TLSCURRENT_COMPLETE */
         if ((cl->sending_pointer == NULL) || (cl->pending_bytes == 0)) {
            cl->sending_pointer = (void *) data;
            cl->pending_bytes = size;
         }
         result = send_all_data(c, cl, &cl->sending_pointer, &cl->pending_bytes, block);
         switch (result) {
         case TRAP_E_IO_ERROR:
            server_disconnected_client(c, i);
            failed++;
            break;
         case TRAP_E_OK:
            passed++;
            cl->client_state = TLSCURRENT_COMPLETE;
            break;
         case TRAP_E_TIMEOUT:
            failed++;
            break;
         }
         j++;
      }
      if (c->connected_clients == 0) {
         /* there is no client to send to */
         result = TRAP_E_IO_ERROR;
         break;
      }
   }
   pthread_mutex_unlock(&c->sending_lock);

   if (FD_ISSET(c->term_pipe[0], &disset)) {
      /* Sending was interrupted by terminate(), exit even from TRAP_WAIT function call. */
      return TRAP_E_TERMINATED;
   }

   if (failed != 0) {
      result = TRAP_E_TIMEOUT;
   } else {
      for (i = 0, passed = 0; i < c->clients_arr_size; ++i) {
         cl = &c->clients[i];
         if ((cl->sd > 0) && (cl->client_state == TLSCURRENT_COMPLETE) && (cl->sending_pointer == NULL)) {
            passed++;
            if (passed == c->connected_clients) {
               break;
            }
         }
      }
      if (passed == c->connected_clients) {
         /* there is no client that failed */
         for (i = 0, j = 0; i < c->clients_arr_size; ++i) {
            cl = &c->clients[i];
            if ((cl->sd > 0) && (cl->client_state == TLSCURRENT_COMPLETE)) {
               cl->client_state = TLSCURRENT_IDLE;
               j++;
               if (j == passed) {
                  break;
               }
            }
         }
         result = TRAP_E_OK;
      } else {
         if (block != 0) {
            goto blocking_repeat;
         }
      }
   }

exit:
   /*
    * Return to blocking_repeat ONLY when the timeout is TRAP_WAIT.
    * TRAP_HALFWAIT is handled before.
    */
   if ((timeout == TRAP_WAIT) && ((result != TRAP_E_OK) || (c->connected_clients == 0))) {
      goto blocking_repeat;
   }
   return result;
}


/**
 * \brief Set interface state as terminated.
 * \param[in] priv  pointer to module private data
 */
void tls_sender_terminate(void *priv)
{
   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   if (c != NULL) {
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
   struct tlsclient_s *cl;
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
         /* cancel accepting new clients */
         pthread_cancel(c->accept_thread);
         pthread_join(c->accept_thread, &res);
      }

      /* close server socket */
      close(c->server_sd);

      /* disconnect all clients */
      pthread_mutex_lock(&c->lock);
      if (c->clients != NULL) {
         for (i = 0; i < c->clients_arr_size; i++) {
            cl = &c->clients[i];
            if (cl->sd > 0) {
               if (cl->ssl) {
                  SSL_free(cl->ssl);
               }
               close(cl->sd);
               cl->sd = -1;
               c->connected_clients--;
            }
            free(cl->buffer);
         }
         free(c->clients);
      }
      pthread_mutex_unlock(&c->lock);
      pthread_mutex_destroy(&c->lock);
      pthread_mutex_destroy(&c->sending_lock);
      sem_destroy(&c->have_clients);

      free(c->backup_buffer);
      free(c);
   }
}

int32_t tls_sender_get_client_count(void *priv)
{
   int32_t client_count = 0;
   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   if (c == NULL) {
      return 0;
   }
   pthread_mutex_lock(&c->lock);
   client_count = c->connected_clients;
   pthread_mutex_unlock(&c->lock);
   return client_count;
}

static void tls_sender_create_dump(void *priv, uint32_t idx, const char *path)
{
   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   /* return value */
   int r;
   /* config file trap-i<number>-config.txt */
   char *conf_file = NULL;
   /* config file trap-i<number>-buffer.dat */
   char *buf_file = NULL;
   FILE *f = NULL;
   trap_buffer_header_t aux = { 0 };
   int32_t i;
   struct tlsclient_s *cl;


   r = asprintf(&conf_file, "%s/trap-o%02"PRIu32"-config.txt", path, idx);
   if (r == -1) {
      VERBOSE(CL_ERROR, "Not enough memory, dump failed. (%s:%d)", __FILE__, __LINE__);
      conf_file = NULL;
      goto exit;
   }
   f = fopen(conf_file, "w");
   fprintf(f, "Server port: %s\nServer socket descriptor: %d\n"
           "Connected clients: %d\nMax clients: %d\nBuffering layer buffer: %p\n"
           "Buffering layer buffer size: %"PRIu32"\n"
           "Backup buffer: %p\nTerminated: %d\nInitialized: %d\n"
           "Message size: %"PRIu32"\nTimeout: %"PRId32"us (%s)\n"
           "Clients:\n",
           c->server_port, c->server_sd, c->connected_clients, c->clients_arr_size,
           c->ctx->out_ifc_list[idx].buffer,
           c->ctx->out_ifc_list[idx].buffer_index,
           c->backup_buffer, c->is_terminated,
           c->initialized,
           c->int_mess_header.data_length,
           c->ctx->out_ifc_list[idx].datatimeout,
           TRAP_TIMEOUT_STR(c->ctx->out_ifc_list[idx].datatimeout));
   for (i = 0; i < c->clients_arr_size; i++) {
      cl = &c->clients[i];
      fprintf(f, "\t{%"PRId32", %s, %p, %"PRIu32"}\n",
              cl->sd, tls_SENDER_STATE_STR(cl->client_state),
              cl->sending_pointer, cl->pending_bytes);
   }

   fclose(f);
   f = NULL;

   r = asprintf(&buf_file, "%s/trap-o%02"PRIu32"-buffer.dat", path, idx);
   if (r == -1) {
      buf_file = NULL;
      VERBOSE(CL_ERROR, "Not enough memory, dump failed. (%s:%d)", __FILE__, __LINE__);
      goto exit;
   }
   f = fopen(buf_file, "w");
   aux.data_length = htonl(c->ctx->out_ifc_list[idx].buffer_index);
   if (fwrite(&aux, sizeof(c->int_mess_header), 1, f) != 1) {
      VERBOSE(CL_ERROR, "Writing buffer header failed. (%s:%d)", __FILE__, __LINE__);
      goto exit;
   }
   if (fwrite(c->ctx->out_ifc_list[idx].buffer, c->ctx->out_ifc_list[idx].buffer_index, 1, f) != 1) {
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
   VERBOSE(CL_ERROR, "Unimplemented. (%s:%d)", __FILE__, __LINE__);
   return;
}


/**
 * \brief Function disconnects all clients of the output interface whose private structure is passed via "priv" parameter.
 *
 * \param[in] priv Pointer to output interface private structure.
 */
void tlsserver_disconnect_all_clients(void *priv)
{
   tls_sender_private_t *c = (tls_sender_private_t *) priv;
   struct tlsclient_s *cl;
   int32_t i;

   pthread_mutex_lock(&c->lock);
   if (c->clients != NULL) {
      for (i = 0; i < c->clients_arr_size; i++) {
         cl = &c->clients[i];
         if (cl->sd > 0) {
            if (cl->ssl) {
               SSL_free(cl->ssl);
               cl->ssl = NULL;
            }
            close(cl->sd);
            cl->sd = -1;
            c->connected_clients--;
         }
      }
   }
   pthread_mutex_unlock(&c->lock);
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
   char *server_port = NULL;
   char *max_clients = NULL;
   char *keyfile = NULL;
   char *certfile = NULL;
   char *cafile = NULL;
   tls_sender_private_t *priv = NULL;
   unsigned int max_num_client = 10;
   uint32_t i;

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

   priv->ctx = ctx;
   priv->ifc_idx = idx;

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
   if (param_iterator != NULL) {
      /* still having something to parse... Rotate all parameters, the first one is probably max_clients*/
      max_clients = keyfile;
      keyfile = certfile;
      certfile = cafile;
      param_iterator = trap_get_param_by_delimiter(param_iterator, &cafile, TRAP_IFC_PARAM_DELIMITER);
   }
   if (max_clients == NULL) {
      /* 2nd parameter became optional, set default value when missing */
      max_num_client = TRAP_IFC_DEFAULT_MAX_CLIENTS;
   } else {
      if (sscanf(max_clients, "%u", &max_num_client) != 1) {
         VERBOSE(CL_ERROR, "Optional max client number given, but it is probably in wrong format.");
         max_num_client = TRAP_IFC_DEFAULT_MAX_CLIENTS;
      }
   }

   /* set global buffer size */
   priv->int_mess_header.data_length = TRAP_IFC_MESSAGEQ_SIZE;
   /* Parsing params ended */

   priv->clients_arr_size = max_num_client;

   priv->clients = (struct tlsclient_s *) calloc(max_num_client, sizeof(struct tlsclient_s));
   if (priv->clients == NULL) {
      result = TRAP_E_MEMORY;
      goto failsafe_cleanup;
   }

   /* allocate buffer according to TRAP_IFC_MESSAGEQ_SIZE with additional space for message header */
   priv->backup_buffer = (void *) calloc(1, priv->int_mess_header.data_length +
                           sizeof(trap_buffer_header_t));

   if (priv->backup_buffer == NULL) {
      /* if some memory could not have been allocated, we cannot continue */
      result = TRAP_E_MEMORY;
      goto failsafe_cleanup;
   }
   for (i = 0; i < max_num_client; i++) {
      /* all clients are IDLE after connection */
      priv->clients[i].client_state = TLSCURRENT_IDLE;
      /* all clients are disconnected */
      priv->clients[i].sd = -1;
      priv->clients[i].buffer = (void *) calloc(TRAP_IFC_MESSAGEQ_SIZE + 4, 1);
      if (priv->clients[i].buffer == NULL) {
         result = TRAP_E_MEMORY;
         goto failsafe_cleanup;
      }
   }

   priv->connected_clients = 0;
   priv->server_port = server_port;
   priv->keyfile = keyfile;
   priv->certfile = certfile;
   priv->cafile = cafile;
   priv->is_terminated = 0;
   pthread_mutex_init(&priv->lock, NULL);
   pthread_mutex_init(&priv->sending_lock, NULL);

   VERBOSE(CL_VERBOSE_ADVANCED, "config:\nserver_port=\"%s\"\nmax_clients=\"%s\"\n"
      "TDU size: %u\nKey file: %s\nCert file: %s\nCA file: %s\n(max_clients_num=\"%u\")",
      priv->server_port, max_clients,
      priv->int_mess_header.data_length, priv->keyfile, priv->certfile,
      priv->cafile, priv->clients_arr_size);
   free(max_clients);

   if (sem_init(&priv->have_clients, 0, 0) == -1) {
      VERBOSE(CL_ERROR, "Initialization of semaphore failed.");
      goto failsafe_cleanup;
   }

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
   ifc->disconn_clients = tlsserver_disconnect_all_clients;
   ifc->send = tls_sender_send;
   ifc->terminate = tls_sender_terminate;
   ifc->destroy = tls_sender_destroy;
   ifc->get_client_count = tls_sender_get_client_count;
   ifc->create_dump = tls_sender_create_dump;
   ifc->priv = priv;
   ifc->get_id = tls_send_ifc_get_id;

   return result;

failsafe_cleanup:
   free(server_port);
   free(max_clients);
   free(certfile);
   free(cafile);
   free(keyfile);
   if (priv != NULL) {
      free(priv->backup_buffer);
      if (priv->clients != NULL) {
         for (i = 0; i < max_num_client; i++) {
            free(priv->clients[i].buffer);
         }
      }
      free(priv->clients);
      pthread_mutex_destroy(&priv->lock);
      pthread_mutex_destroy(&priv->sending_lock);
      free(priv);
   }

   return result;
}

/**
 * \brief Function for server thread - accepts incoming clients and disconnects them.
 * \param[in] arg  tls_sender_private_t structure (private data)
 * \return NULL
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

   /* handle new connections */
   addrlen = sizeof remoteaddr;
   while (1) {
      pthread_mutex_lock(&c->lock);
      if (c->is_terminated != 0) {
         pthread_mutex_unlock(&c->lock);
         break;
      }
      pthread_mutex_unlock(&c->lock);
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
            VERBOSE(CL_VERBOSE_ADVANCED, "New connection from %s on socket %d",
               inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*) &remoteaddr), remoteIP, INET6_ADDRSTRLEN),
                  newclient);

            pthread_mutex_lock(&c->lock);

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

               /** Output interface negotiation */
               int ret_val = output_ifc_negotiation(c, TRAP_IFC_TYPE_TLS, i);
               if (ret_val == NEG_RES_OK) {
                  VERBOSE(CL_VERBOSE_LIBRARY, "Output_ifc_negotiation result: success.");
               } else if (ret_val == NEG_RES_FMT_UNKNOWN) {
                  VERBOSE(CL_VERBOSE_LIBRARY, "Output_ifc_negotiation result: failed (unknown data format of this output interface -> refuse client).");
                  SSL_free(cl->ssl);
                  cl->ssl = NULL;
                  goto refuse_client;
               } else { /* ret_val == NEG_RES_FAILED, sending the data to input interface failed, refuse client */
                  VERBOSE(CL_VERBOSE_LIBRARY, "Output_ifc_negotiation result: failed (error while sending hello message to input interface).");
                  SSL_free(cl->ssl);
                  cl->ssl = NULL;
                  goto refuse_client;
               }

               cl->sd = newclient;
               cl->client_state = TLSCURRENT_IDLE;
               cl->sending_pointer = NULL;
               cl->pending_bytes = 0;
               c->connected_clients++;

               if (sem_post(&c->have_clients) == -1) {
                  VERBOSE(CL_ERROR, "Semaphore post failed.");
               }
            } else {
refuse_client:
               VERBOSE(CL_VERBOSE_LIBRARY, "Shutting down client we do not have additional resources (%u/%u)",
                     c->connected_clients, c->clients_arr_size);
               shutdown(newclient, SHUT_RDWR);
               close(newclient);
            }
            pthread_mutex_unlock(&c->lock);
         }
      }
   }
   pthread_exit(NULL);
}

/**
 * \brief Open TCPIP socket for sender module
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

   if (pthread_create(&c->accept_thread, NULL, accept_clients_thread, priv) != 0) {
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

