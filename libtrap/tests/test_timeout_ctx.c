/**
 * \file test_timeout_ctx.c
 * \brief Test of non-blocking functions
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2014
 */
/*
 * Copyright (C) 2014 CESNET
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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <inttypes.h>
#include <libtrap/trap.h>

#define ERRARG -1

// Struct with information about module
trap_module_info_t module_info_input = {
   "libtrap timeouts test", // Module name
   // Module description
   "",
   1, // Number of input interfaces
   0, // Number of output interfaces
};

trap_module_info_t module_info_output = {
   "libtrap timeouts test", // Module name
   // Module description
   "",
   0, // Number of input interfaces
   1, // Number of output interfaces
};

int main(int argc, char **argv)
{
   int ret;
   trap_ifc_spec_t ifc_spec;
   char *ifc_params[] = {"libtraptimeouttest"};
   ifc_spec.types = "u";
   ifc_spec.params = ifc_params;

   time_t duration, t2;

   uint16_t payload_size = 1000;
   char payload[payload_size];

   // Important change CONTEXT structure
   trap_ctx_t *ctx_input = NULL, *ctx_output = NULL;

   // Initialize TRAP library (create and init all interfaces)
   ctx_input = (trap_ctx_t *) trap_ctx_init(&module_info_input, ifc_spec);
   if (ctx_input == NULL) {
      fprintf(stderr, "Trap_ctx_init failed.\n");
      return 1;
   }
   if (trap_ctx_get_last_error(ctx_input) != TRAP_E_OK) {
      fprintf(stderr, "Trap_ctx_init returned error.\n");
      return 1;
   }
   trap_ctx_ifcctl(ctx_input, TRAPIFC_INPUT, 0, TRAPCTL_SETTIMEOUT, 2500000);


   duration = time(NULL);
   ret = trap_ctx_recv(ctx_input, 0, (void *) payload, &payload_size);
   t2 = time(NULL);
   printf("timeout in %"PRId64", t1: %"PRId64" t2: %"PRId64"\n", t2 - duration, duration, t2);
   if ((t2 - duration) < 2) {
      goto error;
   }
   if (ret != TRAP_E_TIMEOUT) {
      goto error;
   }
   trap_ctx_terminate(ctx_input);
   
   // Initialize TRAP library (create and init all interfaces)
   ctx_output = (trap_ctx_t *) trap_ctx_init(&module_info_output, ifc_spec);
   if (ctx_output == NULL) {
      fprintf(stderr, "Trap_ctx_init failed.\n");
      return 1;
   }
   if (trap_ctx_get_last_error(ctx_output) != TRAP_E_OK) {
      fprintf(stderr, "Trap_ctx_init returned error.\n");
      return 1;
   }

   trap_ctx_ifcctl(ctx_output, TRAPIFC_OUTPUT, 0, TRAPCTL_AUTOFLUSH_TIMEOUT, TRAP_NO_AUTO_FLUSH);
   trap_ctx_ifcctl(ctx_output, TRAPIFC_OUTPUT, 0, TRAPCTL_BUFFERSWITCH, 0);
   trap_ctx_ifcctl(ctx_output, TRAPIFC_OUTPUT, 0, TRAPCTL_SETTIMEOUT, 2500000);

   duration = time(NULL);
   ret = trap_ctx_send(ctx_output, 0, (void *) payload, payload_size);
   t2 = time(NULL);
   printf("timeout in %"PRId64", t1: %"PRId64" t2: %"PRId64"\n", t2 - duration, duration, t2);
   if ((t2 - duration) > 1) {
      printf("expected duration <= 1, measured %d.\n", (int) (t2 - duration));
      goto error;
   }
   if (ret != TRAP_E_TIMEOUT) {
      printf("expected ret %d but received %d.\n", TRAP_E_TIMEOUT, ret);
      goto error;
   }
   trap_ctx_terminate(ctx_output);


   trap_ctx_finalize(&ctx_input);
   trap_ctx_finalize(&ctx_output);
   return 0;
error:
   if (ctx_input != NULL) {
      trap_ctx_finalize(&ctx_input);
   }
   if (ctx_output != NULL) {
      trap_ctx_finalize(&ctx_output);
   }
   return 1;
}

