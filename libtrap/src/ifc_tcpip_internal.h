/**
 * \file ifc_tcpip_internal.h
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


/** \addtogroup trap_ifc
 * @{
 */

/** \addtogroup tcpip_ifc
 * @{
 */

 /**
 * \defgroup tcpip_sender TCPIP output IFC
 * @{
 */

typedef struct buffer_s {
   uint32_t seq_num;
   uint32_t wr_index;
   uint32_t ref_count;
   uint32_t sent_to;

   uint8_t finished;
   uint8_t *header;
   uint8_t *data;

   pthread_mutex_t lock;
} buffer_t;

typedef struct client_s {
   int sd; /**< Socket descriptor */
   void *sending_pointer; /**< Pointer to data in client's assigned buffer */

   uint64_t timer_total; /**< Total time spent sending (in microseconds) */

   uint32_t timer_last; /**< Time spent on last send call (in microseconds) */
   uint32_t pending_bytes; /**< The size of data that must be sent */
   uint32_t id; /** Client identification - PID for unix socket, port number for TCP socket */
   uint32_t received_seq_num;

   buffer_t *assigned_buffer;
} client_t;

typedef struct tcpip_sender_private_s {
   trap_ctx_priv_t *ctx; /**< Libtrap context */
   /**
    * File descriptor pair for select() termination.
    *
    * Using python wrapper, it is not possible to terminate module
    * when no receiver is connected to output IFC.  Therefore,
    * this file descriptor will be used to signal termination to
    * select().
    */
   int term_pipe[2];
   int server_sd;
   char *server_port;
   char is_terminated;
   char initialized;
   enum tcpip_ifc_sockettype socket_type;

   uint64_t dropped_buffers;
   uint64_t active_buffer_reset_timestamp;

   uint32_t ifc_idx;
   uint32_t connected_clients;
   uint32_t clients_arr_size;
   uint32_t next_seq_num;
   uint32_t buffer_count;
   uint32_t buffer_size;
   uint32_t timeout_accept;
   uint32_t timeout_send;
   uint32_t timeout_store;
   uint32_t timeout_autoflush;

   uint8_t flush;
   uint8_t block;

   buffer_t *buffers;
   buffer_t *active_buffer;

   client_t *clients;

   pthread_t send_thr;
   pthread_t accept_thr;
   pthread_mutex_t lock;
} tcpip_sender_private_t;

/**
 * @}
 */

/**
 * \defgroup tcpip_receiver TCPIP input IFC
 * @{
 */
typedef struct tcpip_receiver_private_s {
   trap_ctx_priv_t *ctx; /**< Libtrap context */
   char *dest_addr;
   char *dest_port;
   char connected;
   char is_terminated;
   int sd;
   enum tcpip_ifc_sockettype socket_type;
   void *data_pointer; /**< Pointer to next free byte, if NULL, we ended in header */
   uint32_t data_wait_size; /** Missing data to accept in the next function call */
   void *ext_buffer; /**< Pointer to buffer that was passed by higher layer - this is the place we write */
   uint32_t ext_buffer_size; /** size of content of the extbuffer */
   trap_buffer_header_t int_mess_header; /**< Internal message header - used for message_buffer payload size \note message_buffer size is sizeof(tcpip_tdu_header_t) + payload size */
   uint32_t ifc_idx;
} tcpip_receiver_private_t;

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

