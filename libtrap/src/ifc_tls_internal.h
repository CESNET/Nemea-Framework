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

#define TLS_DEFAULT_TIMEOUT_ACCEPT   0         /**< Default timeout used in accept_new_client() [microseconds] */
#define TLS_DEFAULT_BUFFER_COUNT     1         /**< Default buffer count */
#define TLS_DEFAULT_BUFFER_SIZE      100000    /**< Default buffer size [bytes] */
#define TLS_DEFAULT_MAX_CLIENTS      10        /**< Default size of client array */
#define TLS_DEFAULT_TIMEOUT_FLUSH    1000000   /**< Default timeout for autoflush [microseconds]*/

typedef struct tlsbuffer_s {
    uint32_t wr_index;                      /**< Pointer to first free byte in buffer */
    uint32_t sent_to;                       /**< Number of clients buffer was successfully sent to */

    uint8_t finished;                       /**< Flag indicating whether buffer is full and ready to be sent */

    uint8_t *header;                        /**< Pointer to first byte in buffer */
    uint8_t *data;                          /**< Pointer to first byte of buffer payload */

    pthread_mutex_t lock;
} tlsbuffer_t;

/**
 * \brief Structure for TLS IFC client information.
 */
typedef struct tlsclient_s {
   SSL *ssl;                                /**< Client SSL info. */

   int sd;                                  /**< Client socket descriptor */
   void *sending_pointer;                   /**< Pointer to data in client's assigned buffer */

   uint64_t timer_total;                    /**< Total time spent sending (microseconds) since client connection */
   uint64_t received_buffers;               /**< Total number of received buffers since client connection */

   uint32_t timer_last;                     /**< Time spent on last send call [microseconds] */
   uint32_t pending_bytes;                  /**< The size of data that must be sent */
   uint32_t id;                             /**< Client identification - PID for unix socket, port number for TCP socket */
   uint32_t assigned_buffer;                /**< Index of assigned buffer in array of buffers */
} tlsclient_t;

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
    int timeout_accept;                     /**< Timeout used in accept_new_client() [microseconds] */
    int timeout_autoflush;                  /**< Timeout used for autoflush [microseconds] */

    char *server_port;                      /**< TCPIP port number / UNIX socket path */
    char is_terminated;                     /**< Termination flag */
    char initialized;                       /**< Initialization flag */

    uint64_t finished_buffers;              /**< Counter of 'finished' buffers since trap initialization */
    uint64_t autoflush_timestamp;           /**< Time when the last buffer was finished. Used for autoflush. */

    uint32_t ifc_idx;                       /**< Index of interface in 'ctx->out_ifc_list' array */
    uint32_t connected_clients;             /**< Number of currently connected clients */
    uint32_t clients_arr_size;              /**< Maximum number of clients */
    uint32_t buffer_count;                  /**< Number of buffers used */
    uint32_t buffer_size;                   /**< Buffer size [bytes] */
    uint32_t active_buffer;                 /**< Index of active buffer in 'buffers' array */

    tlsbuffer_t *buffers;                   /**< Array of buffer structures */
    tlsclient_t *clients;                   /**< Array of client structures */

    pthread_t send_thr;                     /**< Pthread structure containing info about sending thread */
    pthread_mutex_t lock;                   /**< Interface lock. Used for autoflush. */
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

