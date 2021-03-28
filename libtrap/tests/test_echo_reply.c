/**
 * \file test_echo_reply.c
 * \brief Measuring tool for performance test for libtrap-0.4
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \author Jan Neuzil <neuzija1@fit.cvut.cz>
 * \author Katerina Pilatova <xpilat05@cesnet.cz>
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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <libtrap/trap.h>
#include "trap_internal.h"

// Struct with information about module
trap_module_info_t module_info = {
   "TCPIP Example client module", // Module name
   // Module description
   "",
   1, // Number of input interfaces
   0, // Number of output interfaces
};

static char stop = 0;

void signal_handler(int signal)
{
   if ((signal == SIGTERM) || (signal == SIGINT)) {
      VERBOSE(CL_VERBOSE_OFF, "Signal TERM received");
      stop = 1;
   }
}

//#define CONTINUOUS

int main(int argc, char **argv)
{
   int ret;

   uint16_t read_size;
   uint16_t last_size;
   uint64_t last_value;
   uint64_t *cur_value;
   uint64_t counter = 0;
   uint64_t iteration = 0;
   uint64_t errors = 0;
   uint64_t size_changes = -1;
   time_t duration;

   trap_verbose = CL_VERBOSE_OFF;

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
   ret = trap_init(&module_info, ifc_spec);
   if (ret != TRAP_E_OK) {
      VERBOSE(CL_ERROR, "TRAP initialization: %s", trap_last_error_msg);
      return 2;
   }
   signal(SIGTERM, signal_handler);
   signal(SIGINT, signal_handler);

   // Read data from input, process them and write to output
   duration = 0;
   last_value = -1;
   last_size = -1;
   cur_value = NULL;
   trap_set_required_fmt(0, TRAP_FMT_RAW);
   //trap_ifcctl(TRAPIFC_INPUT, 0, TRAPCTL_SETTIMEOUT, 2000);
   trap_ifcctl(TRAPIFC_INPUT, 0, TRAPCTL_SETTIMEOUT, TRAP_WAIT);
   while (!stop) {
      ret = trap_recv(0, (const void **) &cur_value, &read_size);
      if (duration == 0) {
	      duration = time(NULL);
      }
      if (read_size == 1) {
         break;
      }
      if (ret == TRAP_E_OK) {
         counter++;
         if ((*cur_value) != (last_value + 1)) {
            printf("%"SCNu64": (cur) %"SCNu64" - %"SCNu64" (last)\n", counter, *cur_value, last_value+1);
            errors++;
         }
         if (last_size != read_size) {
            size_changes++;
            last_size = read_size;
         }
         last_value = (*cur_value);
      } else {
         stop = 1;
      }
      iteration++;
      //usleep(1000);
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
   trap_finalize();

   return 0;
}

