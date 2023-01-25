/**
 * \file ifc_tls_internal.h
 * \brief TRAP TCP/IP interfaces private structures
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2014
 */
/*
 * Copyright (C) 2013 CESNET
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


#include <openssl/ssl.h>
#include <openssl/err.h>

#include "ifc_socket_common.h"
#include "trap_mbuf.h"

#include <sys/queue.h>


/** \addtogroup trap_ifc
 * @{
 */

/** \addtogroup tls_ifc
 * @{
 */

 /**
 * \defgroup tls_sender TLS Output IFC
 * @{
 */

/**
 * \brief Structure for TLS IFC client information.
 */
typedef struct tlsclient_s {
   SSL *ssl;                                /**< Client SSL info. */

   int sd;                                  /**< Client socket descriptor */

   uint64_t timeouts;                       /**< Number of messages dropped (since connection) due to client blocking active buffer */
   uint32_t id;                             /**< Client identification - PID for unix socket, port number for TCP socket */

   pthread_t sender_thread_id;
   uint64_t sent_containers; /**< Sent containers counter */
   uint64_t sent_messages; /**< Sent messages counter */
   uint64_t skipped_messages; /**< Skipped messages counter */
   uint64_t container_id; /**< ID of current container. */
   LIST_ENTRY(tlsclient_s) entries;
} tlsclient_t __attribute__((aligned(64)));

struct tlsclient_container_s {
   LIST_ENTRY(tlsclient_container_s)
   entries;
};

LIST_HEAD(tlsclients_head_s, tlsclient_s);

/**
 * \brief Structure for TLS IFC private information.
 */
typedef struct tls_sender_private_s {
    trap_ctx_priv_t *ctx;                   /**< Libtrap context */

    SSL_CTX *sslctx;                        /**< Server SSL context. */

    char *keyfile;                          /**< Path to private key file in PEM format. */
    char *certfile;                         /**< Path to certificate in PEM format. */
    char *cafile;                           /**< Path to trusted CAs (can be chain file) file in PEM format. */

    int term_pipe[2];                       /**< File descriptor pair for select() termination */
    int server_sd;                          /**< Server socket descriptor */

    char *server_port;                      /**< TCPIP port number / UNIX socket path */
    char is_terminated;                     /**< Termination flag */
    char initialized;                       /**< Initialization flag */

    uint64_t autoflush_timestamp;           /**< Time when the last buffer was finished - used for autoflush */
    uint32_t ifc_idx;                       /**< Index of interface in 'out_ifc_list' array */
    uint32_t connected_clients;             /**< Number of currently connected clients */
    uint32_t buffer_count;                  /**< Number of buffers used */
    uint32_t buffer_size;                   /**< Buffer size [bytes] */

    tlsclient_t *clients;                   /**< Array of client structures */

    pthread_t accept_thr;                   /**< Pthread structure containing info about accept thread */
    pthread_t autoflush_thr;                /**< Pthread to autoflush messages */

    int timeout;
    uint32_t max_clients;
    struct trap_mbuf_s t_mbuf;
    uint64_t max_container_id;

    uint32_t clients_waiting_for_connection;
    uint64_t lowest_container_id;
    struct tlsclients_head_s tlsclients_list_head; /**< clients container list */
    pthread_mutex_t client_list_mtx;
} tls_sender_private_t;

/**
 * @}
 */

/**
 * \defgroup tls_receiver TLS Input IFC
 * @{
 */
typedef struct tls_receiver_private_s {
   trap_ctx_priv_t *ctx;                    /**< Libtrap context */
   char *dest_addr;                         /**< Destination address */
   char *dest_port;                         /**< Destination port */

   char *keyfile;                           /**< Path to private key file in PEM format. */
   char *certfile;                          /**< Path to certificate in PEM format. */
   char *cafile;                            /**< Path to trusted CAs (can be chain file) file in PEM format. */

   SSL_CTX *sslctx;                         /**< Whole client SSL context. */
   SSL *ssl;                                /**< SSL conection info of client */

   uint64_t total_msg;                      /**< Total messages seen on interface. */
   uint64_t total_missed;                   /**< Total messages missed on interface. */
   uint64_t seq;                            /**< Sequence number. */
   uint16_t cur_size;                       /**< received size */

   char connected;                          /**< Indicates whether client is connected to server. */
   char is_terminated;                      /**< Indicates whether client should be destroyed. */
   int sd;                                  /**< Socket descriptor */
   void *data_pointer;                      /**< Pointer to next free byte, if NULL, we ended in header */
   uint32_t data_wait_size;                 /** Missing data to accept in the next function call */
   void *ext_buffer;                        /**< Pointer to buffer that was passed by higher layer - this is the place we write */
   uint32_t ext_buffer_size;                /** size of content of the extbuffer */
   trap_buffer_header_t int_mess_header;    /**< Internal message header - used for message_buffer payload size \note message_buffer size is sizeof(tls_tdu_header_t) + payload size */
   uint32_t ifc_idx;                        /**< Index of IFC */
} tls_receiver_private_t;

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

