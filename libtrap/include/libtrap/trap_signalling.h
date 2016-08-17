/**
 * \file ifc_tcpip.h
 * \brief TRAP signalling API for internal control of data stream
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2016
 */
/*
 * Copyright (C) 2016 CESNET
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

#ifndef TRAP_SIGNALLING_H
#define TRAP_SIGNALLING_H

#include <config.h>
#include "trap.h"

/** \defgroup trap_signal Internal signalization protocol
 *
 * Message HEADER    Data          Message HEADER  Data
 * +-----------+-----------------+ +--------------+--------+
 * | 0x00 0x00 | trap_sig_flag_t | | size_of_data | DATA   |
 * +-----------+-----------------+ +--------------+--------+
 *
 * @{
 */

typedef uint16_t trap_sig_flag_t;

typedef struct trap_sig_mess_s {
    uint16_t size;
    trap_sig_flag_t flag;
} trap_sig_mess_t;

/**
 * Pre-defined constant message size to signalize that the message is trap_sig_mess_t.
 */
#define TRAP_SIGNAL_MAGICSIZE 0x0000

/**
 * The following message is a negotiation (i.e. hello message)
 */
#define TRAP_SIGNAL_NEGOTIATE ((trap_sig_flag_t) (1 << 0))

/**
 * Signal End of Transmission
 */
#define TRAP_SIGNAL_EOT       ((trap_sig_flag_t) (1 << 1))

/**
 * Signal End of Buffer/Burst
 *
 * Can be used e.g. to separate different time windows.
 */
#define TRAP_SIGNAL_EOB       ((trap_sig_flag_t) (1 << 2))

/**
 * Signal next fragment of the previous message
 */
#define TRAP_SIGNAL_FRG       ((trap_sig_flag_t) (1 << 3))

/**
 * Check if the message is a signalling.
 * \return 1 if the message is a signalling one
 */
#define TRAP_SIG_CHECKMESS(data) (((trap_sig_mess_t *) data)->size == 0 ? 1 : 0)


void trap_ctx_sig_flag(trap_ctx_t *ctx, uint32_t ifcidx, trap_sig_flag_t flag);

/**
 * \brief
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifcidx    Index of interface to write into.
 */
#define TRAP_SIG_NEGOTIATE(ifcidx)   trap_ctx_sig_flag(trap_glob_ctx, ifcidx, TRAP_SIGNAL_NEGOTIATE)

/**
 * \brief Signal End of Transmission
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifcidx    Index of interface to write into.
 */
#define TRAP_SIG_EOT(ifcidx)   trap_ctx_sig_flag(trap_glob_ctx, ifcidx, TRAP_SIGNAL_EOT)

/**
 * \brief
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifcidx    Index of interface to write into.
 */
#define TRAP_SIG_EOB(ifcidx)   trap_ctx_sig_flag(trap_glob_ctx, ifcidx, TRAP_SIGNAL_EOB)

/**
 * \brief
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifcidx    Index of interface to write into.
 */
#define TRAP_SIG_FRG(ifcidx)   trap_ctx_sig_flag(trap_glob_ctx, ifcidx, TRAP_SIGNAL_FRG)

/**
 * \brief
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifcidx    Index of interface to write into.
 */
#define TRAP_CTX_SIG_NEGOTIATE(ctx, ifcidx)   trap_ctx_sig_flag(ctx, ifcidx, TRAP_SIGNAL_NEGOTIATE)

/**
 * \brief Signal End of Transmission
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifcidx    Index of interface to write into.
 */
#define TRAP_CTX_SIG_EOT(ctx, ifcidx)   trap_ctx_sig_flag(ctx, ifcidx, TRAP_SIGNAL_EOT)

/**
 * \brief
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifcidx    Index of interface to write into.
 */
#define TRAP_CTX_SIG_EOB(ctx, ifcidx)   trap_ctx_sig_flag(ctx, ifcidx, TRAP_SIGNAL_EOB)

/**
 * \brief
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifcidx    Index of interface to write into.
 */
#define TRAP_CTX_SIG_FRG(ctx, ifcidx)   trap_ctx_sig_flag(ctx, ifcidx, TRAP_SIGNAL_FRG)

#define TRAP_SIG_IS_EOT(flag) (flag & TRAP_SIGNAL_EOT)
#define TRAP_SIG_IS_EOB(flag) (flag & TRAP_SIGNAL_EOB)
#define TRAP_SIG_IS_FRG(flag) (flag & TRAP_SIGNAL_FRG)
#define TRAP_SIG_IS_NEGOTIATE(flag) (flag & TRAP_SIGNAL_NEGOTIATE)

/**
 * @}
 */

#endif

