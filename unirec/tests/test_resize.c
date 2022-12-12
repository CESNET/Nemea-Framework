/**
 * \file test_resize.c
 * \brief Test for UniRec setting values in a cycle
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2022
 */
/*
 * Copyright (C) 2022 CESNET
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
#include <stdint.h>
#include <time.h>

#include "fields.h"

int main(int argc, char **argv)
{
   ur_template_t *tmplt;
   int retval = 0;

   /* Create small template with undefined fields */
   //ur_define_set_of_fields("time TIME");
   tmplt = ur_create_template("", NULL);
   if (tmplt == NULL) {
      fprintf(stderr, "Creating template failed.\n");
      retval = 1;
      goto cleanup;
   }

   /* define fields and expand template */
   if (ur_define_set_of_fields("uint64 baseStore_cacheline_max_index,uint64 baseStore_cacheline_max_index2") != UR_OK) {
      fprintf(stderr, "1. Error when defining 1st UniRec field.\n");
      retval = 1;
      goto cleanup;
   }
   tmplt = ur_expand_template("uint64 baseStore_cacheline_max_index,uint64 baseStore_cacheline_max_index2", tmplt);
   if (tmplt == NULL) {
      fprintf(stderr, "2. Error expanding template.\n");
      retval = 1;
      goto cleanup;
   }
   ur_print_template(tmplt);
   if (ur_define_set_of_fields("uint8 a") != UR_OK) {
      fprintf(stderr, "3. Error when defining 1st UniRec field a.\n");
      retval = 1;
      goto cleanup;
   }
   tmplt = ur_expand_template("uint8 a", tmplt);
   if (tmplt == NULL) {
      fprintf(stderr, "4. Error expanding template a.\n");
      retval = 1;
      goto cleanup;
   }
   if (ur_define_set_of_fields("uint8 b") != UR_OK) {
      fprintf(stderr, "5. Error when defining 1st UniRec field.\n");
      retval = 1;
      goto cleanup;
   }
   tmplt = ur_expand_template("uint8 b", tmplt);
   if (tmplt == NULL) {
      fprintf(stderr, "6. Error expanding template.\n");
      retval = 1;
      goto cleanup;
   }
   if (ur_define_set_of_fields("uint8 bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb") != UR_OK) {
      fprintf(stderr, "7. Error when defining 1st UniRec field.\n");
      retval = 1;
      goto cleanup;
   }
   tmplt = ur_expand_template("uint8 bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", tmplt);
   if (tmplt == NULL) {
      fprintf(stderr, "8. Error expanding template.\n");
      retval = 1;
      goto cleanup;
   }
   ur_print_template(tmplt);

   /* extend tamplete with auto defining */
   tmplt = ur_define_fields_and_update_template("time TIME,uint64 baseStore_cacheline_max_index,uint64 baseStore_cacheline_max_index2,int32 test,ipaddr IP", tmplt);
   if (tmplt == NULL) {
      fprintf(stderr, "9. Error when creating extended UniRec template.\n");
      retval = 1;
      goto cleanup;
   }
   ur_print_template(tmplt);

cleanup:
   ur_free_template(tmplt);

   ur_finalize();

   return retval;
}
