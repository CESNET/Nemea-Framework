/**
 * \file ifc_socket_common.h
 * \brief This file contains common functions and structures used in socket based interfaces (tcp-ip / tls).
 * \author Matej Barnat <barnama1@fit.cvut.cz>
 * \date 2019
 */
/*
 * Copyright (C) 2013-2019 CESNET
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

#ifndef _ifc_socket_common_h_
#define _ifc_socket_common_h_

#define BUFFER_COUNT_PARAM_LENGTH 13 /**< Used for parsing ifc params */
#define BUFFER_SIZE_PARAM_LENGTH 12 /**< Used for parsing ifc params */
#define MAX_CLIENTS_PARAM_LENGTH 12 /**< Used for parsing ifc params */

#define DEFAULT_MAX_DATA_LENGTH (sizeof(trap_buffer_header_t) + 1024) /**< Obsolete? */

#ifndef DEFAULT_BUFFER_COUNT
#define DEFAULT_BUFFER_COUNT 50 /**< Default buffer count */
#endif

#ifndef DEFAULT_BUFFER_SIZE
#define DEFAULT_BUFFER_SIZE 100000 /**< Default buffer size [bytes] */
#endif

#ifndef DEFAULT_MAX_CLIENTS
#define DEFAULT_MAX_CLIENTS 64 /**< Default size of client array */
#endif

#define NO_CLIENTS_SLEEP 100000 /**< Value used in usleep() when waiting for a client to connect */

/**
 * \brief Output buffer structure.
 */
typedef struct buffer_s {
    uint32_t wr_index; /**< Pointer to first free byte in buffer */

    uint8_t* header; /**< Pointer to first byte in buffer */
    uint8_t* data; /**< Pointer to first byte of buffer payload */
} buffer_t;

#endif
