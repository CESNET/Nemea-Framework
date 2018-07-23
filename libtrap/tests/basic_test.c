/**
 * \file basic_test.c
 * \brief Init and finalize
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

#include <libtrap/trap.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

trap_module_info_t module_info = {
  "test module", // Module name
  // Module description
  "test module.\n"
    "Interfaces:\n"
    "   Inputs: 1 (flow records)\n"
    "   Outputs: 0\n",
  1, // Number of input interfaces
  0, // Number of output interfaces
};

int check_socket(const char *socketident)
{
   char filepath[255];
   struct stat buf;
   int result = 1;

   snprintf(filepath, 255, trap_default_socket_path_format, socketident);
   if (stat(filepath, &buf) == -1) {
      fprintf(stderr, "Socket file does not exist (%s).\n", filepath);
      result = 0;
   }
   /* success */
   return result;
}

int main(int argc, char **argv)
{
  // ***** TRAP initialization *****

  trap_ifc_spec_t ifc_spec;

  fprintf(stderr, "Test 01:\n");
  trap_parse_params(&argc, argv, &ifc_spec);
  trap_ctx_t *ctx = trap_ctx_init(&module_info, ifc_spec);
  if (ctx == NULL || trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
     fprintf(stderr, "Failed trap_ctx_init.\n");
     return 1;
  }
  trap_ctx_finalize(&ctx);

  fprintf(stderr, "Test 02:\n");
  ctx = trap_ctx_init2(&module_info, ifc_spec, "abcdef");
  if (ctx == NULL || trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
     fprintf(stderr, "Failed trap_ctx_init.\n");
     return 1;
  }
  sleep(1);
  if (check_socket("abcdef") == 0) {
     trap_ctx_finalize(&ctx);
     return 1;
  }
  trap_ctx_finalize(&ctx);

  fprintf(stderr, "Test 03:\n");
  ctx = trap_ctx_init3("testmodule", "test description", 1, 1, "u:test-input-ifc,u:test-output-ifc", "test-service-ifc");
  if (ctx == NULL || trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
     fprintf(stderr, "Failed trap_ctx_init.\n");
     return 1;
  }
  sleep(1);
  if (check_socket("test-output-ifc") == 0) {
     trap_ctx_finalize(&ctx);
     return 1;
  }
  if (check_socket("test-service-ifc") == 0) {
     trap_ctx_finalize(&ctx);
     return 1;
  }

  trap_ctx_finalize(&ctx);

  fprintf(stderr, "Test 04: (illegal character in service IFC name))\n");
  ctx = trap_ctx_init2(&module_info, ifc_spec, "a/b");
  if (ctx == NULL || trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
     fprintf(stderr, "Failed trap_ctx_init.\n");
     return 1;
  }
  sleep(1);
  if (check_socket("a/b") == 1) {
     trap_ctx_finalize(&ctx);
     return 1;
  }
  trap_ctx_finalize(&ctx);

  fprintf(stderr, "Test 05: (disabled service IFC))\n");
  ctx = trap_ctx_init2(&module_info, ifc_spec, NULL);
  if (ctx == NULL || trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
     fprintf(stderr, "Failed trap_ctx_init.\n");
     return 1;
  }
  sleep(1);
  trap_ctx_finalize(&ctx);

  return 0;
}

