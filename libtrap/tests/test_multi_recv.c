/**
 * \file test_multi_recv.c
 * \brief Test of trap_ctx_multi_recv()
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2015
 */
/*
 * Copyright (C) 2015 CESNET
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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <libtrap/trap.h>

// Struct with information about module
trap_module_info_t module_info = {
   "TCPIP Example client module", // Module name
   // Module description
   "",
   2, // Number of input interfaces
   0, // Number of output interfaces
};

static char stop = 0;
// Important change CONTEXT structure
trap_ctx_t *ctx = NULL;

void signal_handler(int signal)
{
   if ((signal == SIGTERM) || (signal == SIGINT)) {
      printf("Signal TERM received");
      stop = 1;
      trap_ctx_terminate(ctx);
   }
}

//#define CONTINUOUS

int main(int argc, char **argv)
{
   int ret, i;

   uint16_t read_size;
   uint16_t last_size;
   uint64_t last_value;
   uint64_t counter = 0;
   uint64_t iteration = 0;
   uint64_t errors = 0;
   uint64_t size_changes = -1;
   time_t duration;
   uint32_t mask = 0x03;

   trap_multi_result_t *mr;

   trap_ifc_spec_t ifc_spec;
   ret = trap_parse_params(&argc, argv, &ifc_spec);
   if (ret != TRAP_E_OK) {
      if (ret == TRAP_E_HELP) { // "-h" was found
         trap_print_help(&module_info);
         return 0;
      }
      fprintf(stderr, "ERROR in parsing of parameters for TRAP: %s\n", trap_last_error_msg);
      return 1;
   }


   // Initialize TRAP library (create and init all interfaces)
   ctx = (trap_ctx_t *) trap_ctx_init(&module_info, ifc_spec);
   if (ctx == NULL) {
      fprintf(stderr, "Trap_ctx_init failed.\n");
      return 1;
   }
   if (trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
      fprintf(stderr, "Trap_ctx_init returned error.\n");
      return 1;
   }

   signal(SIGTERM, signal_handler);
   signal(SIGINT, signal_handler);

   // Read data from input, process them and write to output
   duration = 0;
   last_value = -1;
   last_size = -1;
   trap_ctx_ifcctl(ctx, TRAPIFC_INPUT, 0, TRAPCTL_SETTIMEOUT, TRAP_WAIT);
   while (mask && !stop) {
      ret = trap_ctx_multi_recv(ctx, mask, (const void **) &mr, &read_size);
      if (duration == 0) {
         duration = time(NULL);
      }
      if (ret == TRAP_E_OK) {
         for (i = 0; i < 2; i++) {
            if (mr[i].result_code == TRAP_E_OK) {
               if (mr[i].message_size <= 1) {
                  mask ^= (1 << i);
                  printf("%d: message_size: %"PRIu16" new mask %"PRIx32"\n",
                         i, mr[i].message_size, mask);
               }
               counter++;
            } else {
               errors++;
            }
         }
      } else {
         printf("error: %s\n", trap_ctx_get_last_error_msg(ctx));
      }
      iteration++;
   }
   printf("\n");
   duration = time(NULL) - duration;

   printf("Number of iterations: %"PRIu64"\nNumber of sent messages: %"PRIu64"\nErrors: %"PRIu64"\nSize changes: %"PRIu64"\nLast size: %hu\nTime: %"PRIu64"s\n",
      (uint64_t) iteration,
      (uint64_t) counter,
      (uint64_t) errors,
      (uint64_t) size_changes,
      last_size,
      (uint64_t) duration);
   printf("Last received value: %"PRIu64"\n", (uint64_t) last_value);

   // Do all necessary cleanup before exiting
   // (close interfaces and free allocated memory)
   trap_ctx_finalize(&ctx);

   return 0;
}




