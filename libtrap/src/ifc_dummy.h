/**
 * \file ifc_dummy.h
 * \brief TRAP dummy interfaces (generator and blackhole)
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
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
#ifndef _TRAP_IFC_DUMMY_H_
#define _TRAP_IFC_DUMMY_H_

#include "trap_ifc.h"

/** Create Generator interface (input ifc).
 *  Receive function of this interface returns always the same data. These data
 *  are given in params on creation.
 *  @param[in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 *  @param[in] params Array of n+1 bytes. First byte is equal to n, other bytes
 *                    are data the generator should generate.
 *  @param[out] ifc Created interface.
 *  @return Error code (0 on success). Generated interface is returned in ifc.
 */
int create_generator_ifc(trap_ctx_priv_t *ctx, char *params, trap_input_ifc_t *ifc);


/** Create Blackhole interface (output ifc).
 *  Send function of this interface does nothing, so everything sent to
 *  a blackhole is dropped.
 *  @param[in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 *  @param[in] params Ignored.
 *  @param[out] ifc Created interface.
 *  @return Always returns 0.
 */
int create_blackhole_ifc(trap_ctx_priv_t *ctx, char *params, trap_output_ifc_t *ifc);

#endif
