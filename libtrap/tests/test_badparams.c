/**
 * \file test_badparams.c
 * \brief Test of IFC parameters given via -i
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2016
 */
/*
 * Copyright (C) 2016 CESNET
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include <libtrap/trap.h>

trap_module_info_t *module_info = NULL;

#define MODULE_BASIC_INFO(BASIC) \
  BASIC("Example module", \
        "test", 1, 1)

#define MODULE_PARAMS(PARAM)

static int local_init(int argc, char **argv)
{
   INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)
   TRAP_DEFAULT_INITIALIZATION(argc, argv, *module_info);
   FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)
   return 0;
}

static int local_init2(int argc, char **argv)
{
   INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)
   TRAP_DEFAULT_INITIALIZATION(argc, argv, *module_info);
   FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)
   return 0;
}

int main(int argc, char **argv)
{
   int ret;
   int argc_test = 3;
   char *argv_test[] = { argv[0], "-i", "u:test-missing-ifc" };
   char *argv_test2[] = { argv[0], "-i", "u:test-ifc1,u:test-ifc2" };

   ret = local_init(argc_test, argv_test);
   if (ret == 0) {
      printf("Test 1 failed %d\n", ret);
      return 1;
   } else {
      printf("Test 1 succeded %d\n", ret);
   }
   TRAP_DEFAULT_FINALIZATION();

   ret = local_init2(argc_test, argv_test2);
   if (ret != 0) {
      printf("Test 2 failed %d\n", ret);
      return 1;
   } else {
      printf("Test 2 succeded %d\n", ret);
   }
   TRAP_DEFAULT_FINALIZATION();

   return 0;
}

