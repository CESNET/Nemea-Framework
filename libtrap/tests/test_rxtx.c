/**
 * \file test_rxtx.c
 * \brief Test: send 1 message recv 1 message
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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <libtrap/trap.h>

#define ERRARG -1

// Struct with information about module
trap_module_info_t module_info = {
   "TCPIP Example client module", // Module name
   // Module description
   "\n",
   1, // Number of input interfaces
   1, // Number of output interfaces
};

static char stop = 0;
static char start_tx_first = 0;
static char enable_buffering = 0;
trap_ctx_t *ctx = NULL;

void signal_handler(int signal)
{
   if ((signal == SIGTERM) || (signal == SIGINT)) {
      printf("Signal TERM received");
      stop = 1;
      trap_ctx_terminate(ctx);
   }
}

void help(const char *progname)
{
   printf("%s -i ifcspec [-bhs] -n [number]\n"
          "\t-i\tlibtrap IFC spec\n"
          "\t-n\tnumber - size of data to send for testing\n"
          "\t-s\toptional parameter to start sending at first.\n"
          "\t-b\tenable buffering.\n", progname);
}

int main(int argc, char **argv)
{
   int ret;

   uint64_t counter = 0, rxcounter = 0;
   uint64_t iteration = 0, limit = 100;
   time_t duration;
   uint16_t payload_size = sizeof(counter);

   const char *options = "hbn:s";
   signed char opt;

   char *payload = NULL;
   const void *recv_payload;
   uint16_t recv_payload_size;
   char send = 0;

   trap_ifc_spec_t ifc_spec;
   ret = trap_parse_params(&argc, argv, &ifc_spec);
   if (ret != TRAP_E_OK) {
      if (ret == TRAP_E_HELP) { // "-h" was found
         help(argv[0]);
         //trap_print_help(&module_info);
         return 0;
      }
      fprintf(stderr, "ERROR in parsing of parameters for TRAP: %s\n", trap_last_error_msg);
      return 1;
   }


   // Initialize TRAP library (create and init all interfaces)
   ctx = trap_ctx_init(&module_info, ifc_spec);
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
   duration = time(NULL);

   // Parses arguments
   if (argc > 1) {
      while ((opt = getopt(argc, argv, options)) != ERRARG) {
         switch (opt) {
         case 'n':
            sscanf(optarg, "%hu", &payload_size);
            payload = (char *) calloc(1, payload_size);
            break;
         case 's':
            send = start_tx_first = 1;
            break;
         case 'h':
            help(argv[0]);
            return 0;
         case 'b':
            enable_buffering = 1;
            break;
         }
      }
   }
   trap_ctx_set_data_fmt(ctx, 0, TRAP_FMT_RAW);
   trap_ctx_set_required_fmt(ctx, 0, TRAP_FMT_RAW);

   trap_ctx_ifcctl(ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_AUTOFLUSH_TIMEOUT, TRAP_NO_AUTO_FLUSH);
   trap_ctx_ifcctl(ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_SETTIMEOUT, TRAP_WAIT);

   if (enable_buffering == 1) {
      trap_ctx_ifcctl(ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_BUFFERSWITCH, 1);
   } else {
      trap_ctx_ifcctl(ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_BUFFERSWITCH, 0);
   }

   if (payload == NULL) {
      fprintf(stderr, "Allocation of payload buffer failed.\n");
      return 1;
   }

   // Read data from input, process them and write to output
   while(!stop) {
      if (send == 1) {
         *((uint64_t *) payload) = counter;
         ret = trap_ctx_send(ctx, 0, (void *) payload, payload_size);
         if (enable_buffering == 1) {
            trap_ctx_send_flush(ctx, 0);
         }
         if (ret == TRAP_E_OK) {
            counter++;
         } else {
            fprintf(stderr, "ERROR in getting data. %d\n", ret);
            break;
         }
      } else {
         send = 1;
      }
      ret = trap_ctx_recv(ctx, 0, &recv_payload, &recv_payload_size);
      if (ret == TRAP_E_OK) {
         rxcounter++;
      } else {
         fprintf(stderr, "ERROR in getting data. %d\n", ret);
         break;
      }
      iteration++;
      if ((iteration == limit) && (start_tx_first == 1)) {
         printf("reached maximal message limit, ending...\n");
         break;
      }
      if (recv_payload_size <= 1) {
         printf("ending message received\n");
         break;
      }
   }
   if (start_tx_first == 1) {
      printf("sending termination message\n");
      ret = trap_ctx_send(ctx, 0, (void *) payload, 1);
      if (enable_buffering == 1) {
         trap_ctx_send_flush(ctx, 0);
      }
   }
   duration = time(NULL) - duration;

   printf("Number of iterations: %"PRIu64"\nLast sent: %"PRIu64"\n"
          "Counter recv: %"PRIu64"\nTime: %"PRIu64"s\n",
      iteration, counter-1, rxcounter-1, duration);

   // Do all necessary cleanup before exiting
   // (close interfaces and free allocated memory)
   trap_ctx_finalize(&ctx);
   free(payload);

   return 0;
}

