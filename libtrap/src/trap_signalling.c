/**
 * \file trap_signalling.c
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

#include <arpa/inet.h>
#include "trap_internal.h"
#include "../include/libtrap/trap_signalling.h"

void trap_ctx_sig_flag(trap_ctx_t *ctx, uint32_t ifcidx, trap_sig_flag_t flag)
{
   trap_ctx_priv_t *c = (trap_ctx_priv_t *) ctx;
   trap_sig_mess_t *sp;
   trap_output_ifc_t *priv;
   // XXX
   (void) priv;
   (void) sp;


   if ((c != NULL) && (ifcidx < c->num_ifc_out)) {
      priv = &c->out_ifc_list[ifcidx];
      pthread_mutex_lock(&c->out_ifc_list[ifcidx].ifc_mtx);
      // XXX
      //if (!(priv->buffer_index <= (TRAP_IFC_MESSAGEQ_SIZE - sizeof(trap_buffer_header_t)))) {
      //   VERBOSE(CL_ERROR, "Not enough space for signalling message.");
      //   pthread_mutex_unlock(&c->out_ifc_list[ifcidx].ifc_mtx);
      //   return;
      //}
      //sp = (trap_sig_mess_t *) &priv->buffer[priv->buffer_index];
      //sp->size = TRAP_SIGNAL_MAGICSIZE;
      //sp->flag = htons(flag);
      //priv->buffer_index += sizeof(trap_sig_mess_t);

      pthread_mutex_unlock(&c->out_ifc_list[ifcidx].ifc_mtx);
    }
}

