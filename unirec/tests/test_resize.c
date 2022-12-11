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
   ur_template_t *tmplt, *tmplt2;

   /* Create small template with undefined fields */
   tmplt = ur_create_template_from_ifc_spec("time TIME");
   if (tmplt != NULL) {
      fprintf(stderr, "Creating template with undefined field should fail.\n");
      return 1;
   }

   /* define new field */
   if (ur_define_set_of_fields("time TIME") != UR_OK) {
      fprintf(stderr, "Error when defining 1st UniRec field.\n");
      return 1;
   }

   /* repeat creating template */
   tmplt = ur_create_template_from_ifc_spec("time TIME");
   if (tmplt == NULL) {
      fprintf(stderr, "Error when creating UniRec template.\n");
      return 1;
   }
   ur_print_template(tmplt);

   /* define fields and expand template */
   if (ur_define_set_of_fields("uint64 baseStore_cacheline_max_index,uint64 baseStore_cacheline_max_index2") != UR_OK) {
      fprintf(stderr, "Error when defining 1st UniRec field.\n");
      return 1;
   }
   tmplt2 = ur_expand_template("uint64 baseStore_cacheline_max_index,uint64 baseStore_cacheline_max_index2", tmplt);
   if (tmplt2 == NULL) {
      fprintf(stderr, "Error expanding template.\n");
      return 1;
   }
   ur_print_template(tmplt2);

   /* extend tamplete with auto defining */
   tmplt2 = ur_define_fields_and_update_template("time TIME,uint64 baseStore_cacheline_max_index,uint64 baseStore_cacheline_max_index2,int32 test,ipaddr IP", tmplt2);
   if (tmplt2 == NULL) {
      fprintf(stderr, "Error when creating extended UniRec template.\n");
      return 1;
   }
   ur_print_template(tmplt2);

   ur_free_template(tmplt2);

   ur_finalize();

   return 0;
}
