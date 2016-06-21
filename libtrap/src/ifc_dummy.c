/**
 * \file ifc_dummy.c
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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#include "../include/libtrap/trap.h"
#include "trap_ifc.h"
#include "trap_error.h"

/***** Generator *****/

typedef struct generator_private_s {
   trap_ctx_priv_t *ctx;
   char *data_to_send;
   int data_size;
   char is_terminated;
} generator_private_t;

static void create_dump(void *priv, uint32_t idx, const char *path)
{
   VERBOSE(CL_ERROR, "Unimplemented. (%s:%d)", __FILE__, __LINE__);
   return;
}

int generator_recv(void *priv, void *data, uint32_t *size, int timeout)
{
   assert(data != NULL);
   assert(size != NULL);

   uint16_t *mh = data;
   void *p = (void *) (mh + 1);

   generator_private_t *config = (generator_private_t*) priv;
   if (config->is_terminated) {
      return trap_error(config->ctx, TRAP_E_TERMINATED);
   }
   *mh = config->data_size;
   memcpy(p, config->data_to_send, config->data_size);
   *size = config->data_size;
   return TRAP_E_OK;
}

void generator_terminate(void *priv)
{
   if (priv) {
      ((generator_private_t*)priv)->is_terminated = 1;
   }
}


void generator_destroy(void *priv)
{
   // Free private data
   if (priv) {
      free(((generator_private_t*)priv)->data_to_send);
      free(priv);
   }
}

char *generator_ifc_get_id(void *priv)
{
   return NULL;
}

int create_generator_ifc(trap_ctx_priv_t *ctx, char *params, trap_input_ifc_t *ifc)
{
   generator_private_t *priv = NULL;
   char *param_iterator = NULL;
   char *n_str = NULL;
   int n, ret;

   // Check parameter
   if (params == NULL) {
      VERBOSE(CL_ERROR, "parameter is null pointer");
      return TRAP_E_BADPARAMS;
   }

   /* Parsing params */
   param_iterator = trap_get_param_by_delimiter(params, &n_str, TRAP_IFC_PARAM_DELIMITER);
   if (n_str == NULL) {
      VERBOSE(CL_ERROR, "Missing parameter of generator IFC.");
      ret = TRAP_E_BADPARAMS;
      goto failure;
   }
   ret = sscanf(n_str, "%d", &n);
   free(n_str);
   if ((ret != 1) || (n <= 0) || (n > 255)) {
      VERBOSE(CL_ERROR, "Generator IFC expects a number from 1 to 255 as the 1st parameter.");
      ret = TRAP_E_BADPARAMS;
      goto failure;
   }

   // Create structure to store private data
   priv = calloc(1, sizeof(generator_private_t));
   if (!priv) {
      ret = TRAP_E_MEMORY;
      goto failure;
   }
   param_iterator = trap_get_param_by_delimiter(param_iterator, &priv->data_to_send, TRAP_IFC_PARAM_DELIMITER);

   // Store data to send (param) into private data
   if (!priv->data_to_send) {
      VERBOSE(CL_ERROR, "Generator IFC expects %d bytes as the 2nd parameter.", priv->data_size);
      ret = TRAP_E_MEMORY;
      goto failure;
   }
   if (strlen(priv->data_to_send) != n) {
      VERBOSE(CL_ERROR, "Bad length of the 2nd parameter of generator IFC.");
      ret = TRAP_E_BADPARAMS;
      goto failure;
   }

   priv->ctx = ctx;
   priv->is_terminated = 0;
   priv->data_size = n;

   // Fill struct defining the interface
   ifc->recv = generator_recv;
   ifc->terminate = generator_terminate;
   ifc->destroy = generator_destroy;
   ifc->create_dump = create_dump;
   ifc->priv = priv;
   ifc->get_id = generator_ifc_get_id;

   return TRAP_E_OK;
failure:
   if (priv != NULL) {
      free(priv->data_to_send);
   }
   free(priv);
   return ret;
}



/***** Blackhole *****/
// Everything sent to blackhole is dropped

int blackhole_send(void *priv, const void *data, uint32_t size, int timeout)
{
   return TRAP_E_OK;
}

void blackhole_terminate(void *priv)
{
   return;
}


void blackhole_destroy(void *priv)
{
   return;
}

int32_t blackhole_get_client_count(void *priv)
{
   /* this interface does not support multiple clients */
   return 1;
}

char *blackhole_ifc_get_id(void *priv)
{
   return NULL;
}

int create_blackhole_ifc(trap_ctx_priv_t *ctx, char *params, trap_output_ifc_t *ifc)
{
   ifc->send = blackhole_send;
   ifc->terminate = blackhole_terminate;
   ifc->destroy = blackhole_destroy;
   ifc->get_client_count = blackhole_get_client_count;
   ifc->create_dump = create_dump;
   ifc->priv = NULL;
   ifc->get_id = blackhole_ifc_get_id;
   return TRAP_E_OK;
}

