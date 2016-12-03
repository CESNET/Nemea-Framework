/**
 * \file ifc_tcpip.h
 * \brief TRAP TCP/IP interfaces
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2013
 * \date 2014
 */
/*
 * Copyright (C) 2013,2014 CESNET
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
#ifndef _TRAP_IFC_TCPIP_H_
#define _TRAP_IFC_TCPIP_H_

#include "trap_ifc.h"

/**
 * Delimiter used between *params* in the *create_tcpip_sender_ifc* and *create_tcpip_receiver_ifc* functions.
 */
#define TCPIP_IFC_PARAMS_DELIMITER  (',')

#ifndef DEFAULT_SOCKET_FORMAT
#define DEFAULT_SOCKET_FORMAT "/trap-%s.sock"
#endif

#ifndef UNIX_PATH_FILENAME_FORMAT
/**
 * Communication via UNIX socket needs to specify path to socket file.
 * It is currently placed according to this format, where %s is replaced by
 * port given as an argument of TCPIP IFC.
 */
#define UNIX_PATH_FILENAME_FORMAT   DEFAULTSOCKETDIR DEFAULT_SOCKET_FORMAT
#endif

/**
 * Type of socket that is used for the TRAP interface.
 */
enum tcpip_ifc_sockettype {
   TRAP_IFC_TCPIP, ///< use TCP/IP connection
   TRAP_IFC_TCPIP_UNIX, ///< use UNIX socket for local communication
   TRAP_IFC_TCPIP_SERVICE ///< use UNIX socket as a service interface
};
#define TCPIP_SOCKETTYPE_STR(st) (st == TRAP_IFC_TCPIP?"TCP":(st == TRAP_IFC_TCPIP_UNIX ? "UNIX": "SERVICE"))
/** Create TCP/IP output interface.
 *  \param [in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 *  \param [in] params  format not decided yet
 *  \param [out] ifc Created interface for library purposes
 *  \param [in] type select the type of socket (see #tcpip_ifc_sockettype for options)
 *  \return 0 on success (TRAP_E_OK)
 */
int create_tcpip_sender_ifc(trap_ctx_priv_t *ctx, const char *params, trap_output_ifc_t *ifc, uint32_t idx, enum tcpip_ifc_sockettype type);


/** Create TCP/IP input interface.
 *  \param [in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 *  \param [in] params  format not decided yet
 *  \param [out] ifc Created interface for library purposes
 *  \param [in] type select the type of socket (see #tcpip_ifc_sockettype for options)
 *  \return 0 on success (TRAP_E_OK)
 */
int create_tcpip_receiver_ifc(trap_ctx_priv_t *ctx, char *params, trap_input_ifc_t *ifc, uint32_t itx, enum tcpip_ifc_sockettype type);

#endif
