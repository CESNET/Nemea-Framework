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

/**
 * How long to sleep between two non-blocking attempts for send()?
 */
#define NONBLOCKING_MINWAIT   1000

/** \addtogroup tls_ifc
 * @{
 */

 /**
 * \defgroup tls_sender TLS Output IFC
 * @{
 */

/**
 * \brief Structure for tls client send state.
 */
enum tlsclient_send_state {
   TLSCURRENT_IDLE, /**< waiting for a message in current buffer */
   TLSCURRENT_HEAD, /**< timeout in header */
   TLSCURRENT_PAYLOAD, /**< timeout in payload */
   TLSCURRENT_COMPLETE, /**< message sent */
   TLSBACKUP_BUFFER /**< timeout in backup buffer */
};

/**
 * \brief Structure for TLS IFC client information.
 */
struct tlsclient_s {
   int sd; /**< Socket descriptor */
   SSL *ssl; /**< Each client individual SSL info. */
   void *sending_pointer; /**< Array of pointers into buffer */
   void *buffer; /**< separate message buffer */
   uint32_t pending_bytes; /**< The size of data that must be sent */
   enum tlsclient_send_state client_state; /**< State of sending */
};

/**
 * \brief Structure for TLS IFC client information.
 */
typedef struct tls_sender_private_s {
   trap_ctx_priv_t *ctx; /**< Libtrap context */
   char *server_port; /**< Server port */

   char *keyfile; /**< Path to private key file in PEM format. */
   char *certfile; /**< Path to certificate in PEM format. */
   char *cafile; /**< Path to trusted CAs (can be chain file) file in PEM format. */

   int server_sd; /**< Server socket descriptor. */
   SSL_CTX *sslctx; /**< Server SSL context. */

   struct tlsclient_s *clients; /**< Array of clients. */

   int32_t connected_clients; /**< Number of currently connected clients. */
   int32_t clients_arr_size; /**< Size of connected clients clients. */
   sem_t have_clients; /**< Semaphore indicating whether server has clients. */
   trap_buffer_header_t int_mess_header; /**< Internal message header. */

   void *backup_buffer; /**< Internal backup buffer for message */

   const void *ext_buffer; /**< Pointer to buffer that was passed by higher layer - this is the place we write */
   uint32_t ext_buffer_size; /** size of content of the extbuffer */

   char is_terminated; /**< Flag to show whether server is terminated. */

   char initialized; /**< Flag to show whether server is initialized. */

   /**
    * File descriptor pair for select() termination.
    *
    * Using python wrapper, it is not possible to terminate module
    * when no receiver is connected to output IFC.  Therefore,
    * this file descriptor will be used to signal termination to
    * select().
    */
   int term_pipe[2];

   pthread_mutex_t  lock; /**< Lock used while working with clients array. */
   pthread_mutex_t  sending_lock; /**< Lock used while working with whole structure. */
   pthread_t        accept_thread; /**< Thread for accepting clients. */
   uint32_t ifc_idx; /**< Index of IFC. */
} tls_sender_private_t;

#define tls_SENDER_STATE_STR(st) (st == TLSCURRENT_IDLE ? "TLSCURRENT_IDLE": \
(st == TLSCURRENT_HEAD     ? "TLSCURRENT_HEAD": \
(st == TLSCURRENT_PAYLOAD  ? "TLSCURRENT_PAYLOAD": \
(st == TLSCURRENT_COMPLETE ? "TLSCURRENT_COMPLETE": \
("BACKUP_BUFFER")))))

/**
 * @}
 */

/**
 * \defgroup tls_receiver TLS Input IFC
 * @{
 */
typedef struct tls_receiver_private_s {
   trap_ctx_priv_t *ctx; /**< Libtrap context */
   char *dest_addr; /**< Destination address */
   char *dest_port; /**< Destination port */

   char *keyfile; /**< Path to private key file in PEM format. */
   char *certfile; /**< Path to certificate in PEM format. */
   char *cafile; /**< Path to trusted CAs (can be chain file) file in PEM format. */

   SSL_CTX *sslctx; /**< Whole client SSL context. */
   SSL *ssl; /**< SSL conection info of client */

   char connected; /**< Indicates whether client is connected to server. */
   char is_terminated; /**< Indicates whether client should be destroyed. */
   int sd; /**< Socket descriptor */
   void *data_pointer; /**< Pointer to next free byte, if NULL, we ended in header */
   uint32_t data_wait_size; /** Missing data to accept in the next function call */
   void *ext_buffer; /**< Pointer to buffer that was passed by higher layer - this is the place we write */
   uint32_t ext_buffer_size; /** size of content of the extbuffer */
   trap_buffer_header_t int_mess_header; /**< Internal message header - used for message_buffer payload size \note message_buffer size is sizeof(tls_tdu_header_t) + payload size */
   uint32_t ifc_idx; /**< Index of IFC */
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

