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
#include <unistd.h>
#include <string.h>
#include <libtrap/trap.h>
#include <trap_internal.h>

// Struct with information about module
trap_module_info_t module_info = {
   "TCPIP Example client module", // Module name
   // Module description
   "This module serves as an example of module implementation using TCPIP IFC in TRAP platform.\n"
   "It does nothing, just receives data from sender and prints it."
   "Interfaces:\n"
   "   Inputs: 1 (any format)\n"
   "   Outputs: 0",
   1, // Number of input interfaces
   0, // Number of output interfaces
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

#define CONTINUOUS

int main(int argc, char **argv)
{
   int ret;
   char *recvdata = NULL;

   trap_ifc_spec_t ifc_spec;
   char *ifc_params[] = {"localhost,12345,4096"};
   ifc_spec.types = "t";
   ifc_spec.params = ifc_params;

   int timeout;
#ifndef CONTINUOUS
   int i;
#endif

   //verbose = CL_VERBOSE_LIBRARY;
   trap_verbose = CL_VERBOSE_OFF;

   // Initialize TRAP library (create and init all interfaces)
   ret = trap_init(&module_info, ifc_spec);
   if (ret != TRAP_E_OK) {
      VERBOSE(CL_ERROR, "TRAP initialization: %s", trap_last_error_msg);
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
   trap_ifcctl(TRAPIFC_INPUT, 0, TRAPCTL_SETTIMEOUT, timeout);

   VERBOSE(CL_VERBOSE_OFF, "Receiving...");
   // Read data from input, process them and write to output
#ifndef CONTINUOUS
   for (i=0; i<100; ++i) {
#else
   while (!stop) {
#endif
      const void *data_ptr;
      uint16_t data_size;

#ifndef CONTINUOUS
      printf("%03i: ", i);
#endif

      // Receive data from any interface, wait until data are available
      ret = trap_recv(0, &data_ptr, &data_size);
      if (ret == TRAP_E_OK) {
         recvdata = (char *) calloc(1, data_size + 1);
         memcpy(recvdata, data_ptr, data_size);
#ifndef DEBUG
         if (data_size > 10) {
            printf("recv(%hu): %.5s..%.5s\n", data_size, (char *) recvdata, (char *) (recvdata + data_size - 5));
         } else {
            printf("recv(%u): %s\n", (unsigned int) data_size, (char *) recvdata);
         }
#endif
         free(recvdata);
      } else if ((ret == TRAP_E_TERMINATED) || (ret == TRAP_E_IO_ERROR)) {
         printf("terminated or IO error\n");
         break;
      } else if (ret == TRAP_E_TIMEOUT) {
         printf("trap_recv() timeouted, this shouldn't happen!\n");
#ifdef WAITING
         sleep(6);
#endif
      } else {
         printf("Error: trap_recv() returned %i (%s)\n", ret, trap_last_error_msg);
      }
   }

   // Do all necessary cleanup before exiting
   // (close interfaces and free allocated memory)
   trap_finalize();

   return 0;
}


