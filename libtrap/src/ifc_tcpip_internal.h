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

#include "ifc_socket_common.h"
#include "trap_mbuf.h"

#include <sys/queue.h>

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


/**
 * \brief Structure for TCP/IP IFC client information.
 */
typedef struct client_s {
	int sd;                    /**< Client socket descriptor */
	uint64_t cont_id;          /**< ID of current container. */
	uint32_t id;               /**< Client identification - PID for unix socket, port number for TCP socket */

	uint64_t sent_containers;  /**< Container counter */
	uint64_t sent_messages;    /**< Messages counter */

	pthread_t tid;   

	int pfds_index;            /**< Client pfds struct array index. */
	LIST_ENTRY(client_s) entries; 
} client_t __attribute__ ((aligned (64)));

struct client_container_s {
   LIST_ENTRY(client_container_s) entries; 
};

LIST_HEAD(clients_head_s, client_s);

/**
 * \brief Structure for service IFC client information.
 */
typedef struct service_client_s {
	int sd;  /**< Client socket descriptor */
} service_client_t;

/**
 * \brief Structure for service IFC private information.
 */
typedef struct tcpip_sender_service_private_s {
	size_t max_clients;         /**< Maximum number of clients */
	int server_sd;              /**< Server socket descriptor */
	char *server_port;          /**< UNIX socket path */
	char initialized;           /**< Initialization flag */
	char is_terminated;         /**< Termination flag */
	int term_pipe[2];           /**< File descriptor pair for select() termination */
	service_client_t *clients;  /**< Array of client structures */
} tcpip_sender_service_private_t;


/**
 * \brief Structure for TCP/IP IFC private information.
 */
typedef struct tcpip_sender_private_s {
	trap_ctx_priv_t *ctx;                   /**< Libtrap context */

	enum tcpip_ifc_sockettype socket_type;  /**< Socket type (TCPIP / UNIX) */
	size_t connected_clients;
	size_t max_clients;

   bool is_blocking_mode;
	bool is_client_waiting_for_connection;

	struct clients_head_s clients_list_head; /**< clients container list */

	uint32_t ifc_idx;                       /**< Index of interface in 'out_ifc_list' array */
	struct trap_mbuf_s t_mbuf;
	char *server_port;                      /**< TCPIP port number / UNIX socket path */
	char is_terminated;                     /**< Termination flag */
	char initialized;                       /**< Initialization flag */
	int server_sd;                          /**< Server socket descriptor */
	uint64_t autoflush_timestamp;           /**< Time when the last buffer was finished - used for autoflush */

	int term_pipe[2];                       /**< File descriptor pair for select() termination */
	uint64_t max_cont_id;
	
	struct pollfd *clients_pfds;            /**< Array of clients pfds for poll */

	pthread_mutex_t client_list_mtx;
	pthread_t accept_thr;                   /**< Pthread structure containing info about accept thread */
	pthread_t autoflush_thr;
} tcpip_sender_private_t;

/**
 * @}
 */

#define TERMINATE_IMMEDIATELY 1
#define TERMINATE_AFTER_JOB_DONE 2

/**
 * \defgroup tcpip_receiver TCPIP input IFC
 * @{
 */
typedef struct tcpip_receiver_private_s {
	 trap_ctx_priv_t *ctx;                   /**< Libtrap context */
	 char *dest_addr;
	 char *dest_port;
	 char connected;
	// char is_terminated;

	 int is_terminated;

	 uint64_t total_msg;
	 uint64_t total_missed;
	 uint64_t seq;
	 uint16_t c_size;

	 int sd;
	 enum tcpip_ifc_sockettype socket_type;
	 void *data_pointer;                     /**< Pointer to next free byte, if NULL, we ended in header */
	 uint32_t data_wait_size;                /**< Missing data to accept in the next function call */
	 void *ext_buffer;                       /**< Pointer to buffer that was passed by higher layer - this is the place we write */
	 uint32_t ext_buffer_size;               /**< size of content of the extbuffer */
	 trap_buffer_header_t int_mess_header;   /**< Internal message header - used for message_buffer payload size \note message_buffer size is sizeof(tcpip_tdu_header_t) + payload size */
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

