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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <libtrap/trap.h>
#include "trap_internal.h"

// Struct with information about module
trap_module_info_t module_info = {
   "TCPIP Example server module", // Module name
   // Module description
   ""
   "Interfaces:\n"
   "   Inputs: 0\n"
   "   Outputs: 1\n",
   0, // Number of input interfaces
   1, // Number of output interfaces
};

static char stop = 0;

void signal_handler(int signal)
{
   if ((signal == SIGTERM) || (signal == SIGINT)) {
      VERBOSE(CL_VERBOSE_OFF, "Signal TERM received");
      stop = 1;
      trap_terminate();
   }
}


int main(int argc, char **argv)
{
   int ret;

   trap_ifc_spec_t ifc_spec;
   char *ifc_params[] = {"12345,5"};
   ifc_spec.types = "t";
   ifc_spec.params = ifc_params;

   char *data_to_send = argv[0];
   uint16_t mess_size = 0;
   int timeout;
   int i = 0;

   //verbose = CL_VERBOSE_LIBRARY;
   trap_verbose = CL_VERBOSE_OFF;
   VERBOSE(CL_VERBOSE_OFF, "%s [number]\nnumber - size of data to send for testing", argv[0]);

   // Initialize TRAP library (create and init all interfaces)
   ret = trap_init(&module_info, ifc_spec);
   if (ret != TRAP_E_OK) {
      fprintf(stderr, "ERROR in TRAP initialization: %s\n", trap_last_error_msg);
      return 1;
   }

   signal(SIGTERM, signal_handler);
   signal(SIGINT, signal_handler);

   //#define BLOCKING
#ifdef NONBLOCKING
   timeout = TRAP_NO_WAIT;
#elif defined(BLOCKING)
   timeout = TRAP_WAIT;
#else
   timeout = 5000000; /* wait for 5 secs */
#endif

   if (argc > 1) {
      if (sscanf(argv[1], "%hu", &mess_size) != 1) {
         mess_size = strlen(argv[0]);
      } else {
         /* send mess_size - 1 x '*', '\0' */
         data_to_send = calloc(1, mess_size);
#ifdef SENDASTERIX
         memset(data_to_send, '*', mess_size - 1);
#else
         for (i=0; i<mess_size; ++i) {
            data_to_send[i] = '0' + (i % 10);
         }
#endif
      }
   } else {
      mess_size = strlen(argv[0]);
   }
   i = 0;
   uint64_t countok = 0;
   uint64_t countto = 0;
   // Read data from input, process them and write to output
   while(!stop) {
      ret = trap_send_data(0, data_to_send, mess_size, timeout);
      if (ret == TRAP_E_TERMINATED) {
         break;
      } else if (ret == TRAP_E_OK) {
         countok++;
         //printf("%02u ", i++);
      } else {
         VERBOSE(CL_ERROR, "%s", trap_last_error_msg);
         i = 0;
         countto++;
         printf("T\n");
         //sleep(1);
      }
#ifdef WAITING
      sleep(6);
#endif
   }

   printf("ok: %"PRIu64"\ntimeout: %"PRIu64"\n", (uint64_t) countok, (uint64_t) countto);

   if (argc > 1) {
      free(data_to_send);
   }

   // Do all necessary cleanup before exiting
   // (close interfaces and free allocated memory)
   trap_finalize();

   return 0;
}


