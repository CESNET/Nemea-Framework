/**
 * \file test_parse_params.c
 * \brief Module for testing of trap_parse_params function.
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
#include <stdio.h>
#include <string.h>
#include <libtrap/trap.h>

int main(int argc, char **argv)
{
   int ret, i;
   trap_ifc_spec_t ifc_spec;

   // Initialize TRAP library (create and init all interfaces)
   ret = trap_parse_params(&argc, argv, &ifc_spec);
   if (ret != TRAP_E_OK) {
      if (ret == TRAP_E_HELP) {
         fprintf(stderr, "Help.\n");
         return 0;
      }
      fprintf(stderr, "ERROR in parsing of parameters for TRAP: %s\n", trap_last_error_msg);
      return 1;
   }

   printf("ifc_types: \"%s\"\n", ifc_spec.types);
   printf("ifc_params:\n");
   for (i = 0; i < strlen(ifc_spec.types); i++) {
      printf("\t\"%s\"\n", ifc_spec.params[i]);
   }
   printf("argc: %i\n", argc);
   printf("argv:\n");
   for (i = 0; i < argc; i++) {
      printf("\t\"%s\"\n", argv[i]);
   }

   // Cleanup
   trap_free_ifc_spec(ifc_spec);

   return 0;
}

