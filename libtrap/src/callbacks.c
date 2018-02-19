/**
 * \file callbacks.c
 * \brief Callbacks setters for TRAP library.
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

#include "../include/libtrap/trap.h"
#include "trap_internal.h"


void trap_ctx_clb_in_negotiation(trap_ctx_t *ctx, clb_in_negotiation_t clb, void *caller_data)
{
   trap_ctx_priv_t *c = (trap_ctx_priv_t *) ctx;
   /* Set pointer to callback function if it was passed. */
   if (c != NULL) {
      c->clb_in_negotiation = clb;
   }
   /* Set pointer to user data (caller_data) if it was passed. */
   if (caller_data != NULL) {
      c->clb_in_negotiation_arg = caller_data;
   }
}

void trap_clb_in_negotiation(clb_in_negotiation_t clb, void *caller_data)
{
   /* call ctx version, this is only a wrapper */
   trap_ctx_clb_in_negotiation(trap_glob_ctx, clb, caller_data);
}


