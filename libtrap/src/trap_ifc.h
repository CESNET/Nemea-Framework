/**
 * \file trap_ifc.h
 * \brief Interface of TRAP interfaces.
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \author Jan Neuzil <neuzija1@fit.cvut.cz>
 * \author Marek Svepes <svepemar@fit.cvut.cz>
 * \date 2013
 * \date 2014
 * \date 2015
 */
/*
 * Copyright (C) 2013-2015 CESNET
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
#ifndef _TRAP_IFC_H_
#define _TRAP_IFC_H_

#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

/** \defgroup trap_ifc TRAP communication module interface
 * @{
 */

/**
 * Default max number of clients that can connect to output interface.
 * It takes effect when no value is given during interface initialization.
 */
#define TRAP_IFC_DEFAULT_MAX_CLIENTS 64

/**
 * \defgroup trap_ifc_api IFC API
 *
 * The set of function that must be implemented for communication interface.
 * @{
 */
/**
 * Receive one message via this IFC.
 *
 * This function is called from trap_read_from_buffer() when there is a
 * need to get new data.
 *
 * \param[in] p   pointer to IFC's private memory allocated by constructor
 * \param[out] d  pointer to memory where this IFC can write received message
 * \param[out] s  size (in bytes) of received message (must be set by this IFC)
 * \param[in] t   timeout, see \ref trap_timeout
 * \returns TRAP_E_OK on success
 */
typedef int (*ifc_recv_func_t)(void *p, void *d, uint32_t *s, int t);

/**
 * Send one message via this IFC.
 *
 * This function is called from trap_store_into_buffer() when there is a
 * need to send new data.
 *
 * \param[in] p   pointer to IFC's private memory allocated by constructor
 * \param[in] d   pointer to message that will be sent
 * \param[in] s   size (in bytes) of message that will be sent
 * \param[in] t   timeout, see \ref trap_timeout
 * \returns TRAP_E_OK on success
 */
typedef int (*ifc_send_func_t)(void *p, const void *d, uint32_t s, int t);

/**
 * Disconnect all connected clients to output IFC.
 *
 * \param[in] p   pointer to IFC's private memory allocated by constructor
 */
typedef void (*ifc_disconn_clients_func_t)(void *p);

/**
 * Terminate IFC - stop sending/receiving.
 *
 * \param[in] p   pointer to IFC's private memory allocated by constructor
 */
typedef void (*ifc_terminate_func_t)(void *p);

/**
 * Destructor, called to free allocated memory.
 *
 * \param[in,out] p   pointer to IFC's private memory allocated by constructor
 */
typedef void (*ifc_destroy_func_t)(void *p);

/**
 * Create dump of interface for debug purposes.
 *
 * \param[in] p   pointer to IFC's private memory allocated by constructor
 * \param[in] i   index of interface (index into the IFC array in the context
 * #trap_ctx_priv_s)
 * \param[in] d   directory path to generate output
 */
typedef void (*ifc_create_dump_func_t)(void *p, uint32_t i, const char *d);

/**
 * Get number of connected clients.
 *
 * \param[in] p   pointer to IFC's private memory allocated by constructor
 * \returns number of clients that are connected to this IFC
 */
typedef int32_t (*ifc_get_client_count_func_t)(void *p);

/**
 * Get identifier of the interface
 *
 * \param[in] priv   pointer to IFC's private memory allocated by constructor
 * \returns pointer to char (private memory of the ifc - do not free!).
 * TCP and UNIXSOCKET ifces return port and name of socket, file ifc returns name of the file,
 * GENERATOR and BLACKHOLE ifces return NULL.
 */
typedef char * (*ifc_get_id_func_t)(void *priv);


/**
 * @}
 */

/** Struct to hold an instance of some input interface. */
typedef struct trap_input_ifc_s {
   ifc_get_id_func_t get_id;       ///< Pointer to get_id function
   ifc_recv_func_t recv;           ///< Pointer to receive function
   ifc_terminate_func_t terminate; ///< Pointer to terminate function
   ifc_destroy_func_t destroy;     ///< Pointer to destructor function
   ifc_create_dump_func_t create_dump; ///< Pointer to function for generating of dump
   void *priv;                     ///< Pointer to instance's private data
   char *buffer;                   ///< Internal pointer to buffer for messages
   char *buffer_pointer;           ///< Internal pointer to current message in buffer
   uint32_t buffer_full;           ///< Internal used space in message buffer (0 for empty buffer)
   int32_t datatimeout;            ///< Timeout for *_recv() calls

   /**
    * If 1 do not allow to change timeout by module, it is used to force
    * timeout of IFC by module's parameter.  If 0 - timeout can be changed
    * by standard way using trap_ctx_ifcctl().
    */
   char datatimeout_fixed;
   char ifc_type;                  ///< Type of interface

   pthread_mutex_t ifc_mtx;        ///< Locking mutex for interface.

   /**
    * Negotiation state defined as #trap_in_ifc_state_t.
    */
   trap_in_ifc_state_t client_state;

   /**
    * Message format defined by trap_data_format_t.
    */
   uint8_t data_type;

   /**
    * Message format specifier.
    *
    * if data_type is TRAP_FMT_RAW, no data_fmt_spec is expected.  Otherwise,
    * data_fmt_spec contains e.g. UniRec template specifier (string representation)
    */
   char *data_fmt_spec;

   /**
    * Required message format defined by trap_data_format_t
    */
   uint8_t req_data_type;

   /**
    * Required message format specifier.
    *
    * if data_type is TRAP_FMT_RAW, no data_fmt_spec is expected.  Otherwise,
    * data_fmt_spec contains e.g. UniRec template specifier (string representation)
    */
   char *req_data_fmt_spec;
} trap_input_ifc_t;

/** Struct to hold an instance of some output interface. */
typedef struct trap_output_ifc_s {
   ifc_get_id_func_t get_id;       ///< Pointer to get_id function
   ifc_disconn_clients_func_t disconn_clients; ///< Pointer to disconnect_clients function
   ifc_send_func_t send;           ///< Pointer to send function
   ifc_terminate_func_t terminate; ///< Pointer to terminate function
   ifc_destroy_func_t destroy;     ///< Pointer to destructor function
   ifc_create_dump_func_t create_dump; ///< Pointer to function for generating of dump
   ifc_get_client_count_func_t get_client_count;  ///< Pointer to get_client_count function
   void *priv;                     ///< Pointer to instance's private data
   unsigned char *buffer;          ///< Internal pointer to buffer for messages
   unsigned char *buffer_header;   ///< Internal pointer to header of buffer followed by payload
   uint32_t buffer_index;          ///< Internal index in buffer for new message
   uint8_t buffer_occupied;        ///< If 0, buffer can be modified, otherwise drop message and don't move with buffer.
   pthread_mutex_t ifc_mtx;        ///< Locking mutex for interface.
   int64_t timeout;                ///< Internal structure to send partial data after timeout (autoflush).

   /**
    * If 1 do not allow to change autoflush timeout by module.  If 0 - autoflush can
    * be changed by standard way using trap_ctx_ifcctl().
    */
   char timeout_fixed;

   char bufferswitch;              ///< Enable (1) or Disable (0) buffering, default is Enabled (1).

   /**
    * If 1 do not allow to change bufferswitch by module.  If 0 - bufferswitch can
    * be changed by standard way using trap_ctx_ifcctl().
    */
   char bufferswitch_fixed;

   char bufferflush;               ///< Flag (1) whether the buffer was sent before timeout has elapsed or not (0)
   int32_t datatimeout;            ///< Timeout for *_send() calls

   /**
    * If 1 do not allow to change timeout by module, it is used to force
    * timeout of IFC by module's parameter.  If 0 - timeout can be changed
    * by standard way using trap_ctx_ifcctl().
    */
   char datatimeout_fixed;
   char ifc_type;                  ///< Type of interface

   /**
    * Message format defined by trap_data_format_t
    */
   uint8_t data_type;

   /**
    * Message format specifier.
    *
    * if data_type is TRAP_FMT_RAW, no data_fmt_spec is expected.  Otherwise,
    * data_fmt_spec contains e.g. UniRec template specifier (string representation)
    */
   char *data_fmt_spec;
} trap_output_ifc_t;

/**
 * \brief Internal function for setting of timeout structs according to libtrap timeout.
 *
 * \param[in] timeout  timeout given to trap_get_data() or trap_send_data()
 * \param[out] tm      used for select() call when non-blocking
 * \param[out] tmnblk  used for sem_timedwait() call to block on semaphore.
 */
void trap_set_timeouts(int timeout, struct timeval *tm, struct timespec *tmnblk);

/**
 * \brief Internal function for setting of timeout structs according to libtrap timeout.
 *
 * \param[in] timeout  Amount of time / timeout that is being converted.
 * \param[in] tm       Precomputed timeval, set using e.g. trap_set_timeouts().
 * \param[out] tmnblk  Used for sem_timedwait() call to block on semaphore.
 */
void trap_set_abs_timespec(int timeout, struct timeval *tm, struct timespec *tmnblk);

/**
 * @}
 */
#endif
