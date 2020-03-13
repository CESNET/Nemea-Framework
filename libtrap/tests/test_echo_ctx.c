/**
 * \file test_echo_ctx.c
 * \brief Measuring tool for performance test for libtrap-0.5
 * \author Jan Neuzil <neuzija1@fit.cvut.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \author Katerina Pilatova <xpilat05@cesnet.cz>
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
#include <inttypes.h>
#include <getopt.h>
#include <unistd.h>
#include <libtrap/trap.h>

#define ERRARG -1

// Struct with information about module
trap_module_info_t module_info = {
   "TCPIP Example client module", // Module name
   // Module description
   "Parameters: \n"
   " -n X   X = size of data to send for testing \n",
   0, // Number of input interfaces
   1, // Number of output interfaces
};

static char stop = 0;
trap_ctx_t *ctx = NULL;

void signal_handler(int signal)
{
   if ((signal == SIGTERM) || (signal == SIGINT)) {
      printf("Signal TERM received");
      stop = 1;
   }
}

int main(int argc, char **argv)
{
   int ret;

   uint64_t counter = 0;
   uint64_t iteration = 0;
   time_t duration;
   uint16_t payload_size = sizeof(counter);

   const char *options = "n:";
   signed char opt;

   char *payload = NULL;

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

   // Important change CONTEXT structure

   printf("%s [number]\nNumber - size of data to send for testing\n", argv[0]);

   // Initialize TRAP library (create and init all interfaces)
   ctx = trap_ctx_init(&module_info, ifc_spec);
   if (ctx == NULL) {
      fprintf(stderr, "Trap_ctx_init failed.\n");
      goto cleanup;
   }
   if (trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
      fprintf(stderr, "Trap_ctx_init returned error. %s\n", trap_ctx_get_last_error_msg(ctx));
      goto cleanup;
   }

   // Set the data format of the output interface
   trap_ctx_set_data_fmt(ctx, 0, TRAP_FMT_RAW, NULL);

   signal(SIGTERM, signal_handler);
   signal(SIGINT, signal_handler);
   duration = time(NULL);
   trap_ctx_ifcctl(ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_AUTOFLUSH_TIMEOUT, TRAP_NO_AUTO_FLUSH);
   trap_ctx_ifcctl(ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_SETTIMEOUT, TRAP_WAIT);
   trap_ctx_ifcctl(ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_BUFFERSWITCH, 1);

   // Parses arguments
   if (argc > 1) {
      while ((opt = getopt(argc, argv, options)) != ERRARG) {
         switch (opt) {
         case 'n':
            sscanf(optarg, "%hu", &payload_size);
            payload = (char *) calloc(1, payload_size);
         }
      }
   }

   if (payload == NULL) {
      fprintf(stderr, "Allocation of payload buffer failed.\n");
      return 1;
   }

   // Read data from input, process them and write to output
   while(!stop) {
      *((uint64_t *) payload) = counter;
      ret = trap_ctx_send(ctx, 0, (void *) payload, payload_size);
      if (ret == TRAP_E_OK) {
         counter++;
      } else {
         // CANNOT GET LAST ERROR MSG NOW
         fprintf(stderr, "ERROR in getting data. %d\n", ret);
         //printf("%s", ctx->trap_last_error_msg);
         break;
      }
      iteration++;
      if (counter == 1234568000) {
         printf("reached maximal message count\n");
         break;
      }
   }
   ret = trap_ctx_send(ctx, 0, (void *) payload, 1);
   trap_ctx_send_flush(ctx, 0);
   duration = time(NULL) - duration;

   printf("Number of iterations: %"PRIu64"\nLast sent: %"PRIu64"\nTime: %"PRIu64"s\n",
      (uint64_t) iteration,
      (uint64_t) counter-1,
      (uint64_t) duration);

cleanup:
   // Do all necessary cleanup before exiting
   // (close interfaces and free allocated memory)
   trap_ctx_finalize(&ctx);
   trap_free_ifc_spec(ifc_spec);
   free(payload);

   return 0;
}

