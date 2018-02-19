/**
 * \file callbacks.h
 * \brief Interface of callbacks that can be set in TRAP library.
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2018
 */
/*
 * Copyright (C) 2018 CESNET
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

#ifndef _TRAP_CALLBACKS_H_
#define _TRAP_CALLBACKS_H_

#include "trap.h"

/**
 * Callback function that is called at the end of every negotiation of an input IFC.
 *
 * \param[in] negotiation_result Result of receiving and evaluation of a
 *      `hello` message. It might be set to any of the following values:
 *      #NEG_RES_CONT, #NEG_RES_FAILED, #NEG_RES_FMT_CHANGED, #NEG_RES_FMT_MISMATCH,
 *      #NEG_RES_FMT_UNKNOWN, #NEG_RES_RECEIVER_FMT_SUBSET, #NEG_RES_SENDER_FMT_SUBSET
 * \param[in] req_data_type      Required data type that was set for the IFC, see #trap_ctx_vset_required_fmt()
 * \param[in] req_data_fmt       Required data specifier that was set for the IFC.
 * \param[in] recv_data_type     Data type that was sent by output IFC where this IFC is connected.
 * \param[in] recv_data_fmt      Data specifier that was sent by output IFC where this IFC is connected.
 * \param[in] caller_data        Optional user data that was set by callback setter (#trap_ctx_clb_in_negotiation() or #trap_clb_in_negotiation()).
 *
 * \return Return value has currently no effect, it is reserved for possible future purposes.
 */
typedef int (*clb_in_negotiation_t)(int negotiation_result, uint8_t req_data_type, const char *req_data_fmt, uint8_t recv_data_type, const char *recv_data_fmt, void *caller_data);

/**
 * Set callback function for negotiation of the input IFC.
 *
 * \param[in,out] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] clb    Pointer to callback function.
 * \param[in] caller_data    Optional pointer to user data that is passed into callback function. It can be NULL.
 */
void trap_ctx_clb_in_negotiation(trap_ctx_t *ctx, clb_in_negotiation_t clb, void *caller_data);

/**
 * Set callback function for negotiation of the input IFC.
 *
 * \param[in] clb    Pointer to callback function.
 * \param[in] caller_data    Optional pointer to user data that is passed into callback function. It can be NULL.
 */
void trap_clb_in_negotiation(clb_in_negotiation_t clb, void *caller_data);

#endif

