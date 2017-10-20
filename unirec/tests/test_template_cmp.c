/**
 * \file test_template_cmp.c
 * \brief Test for comparison of two UniRec templates
 * \author Pavel Krobot <Pavel.Krobot@cesnet.cz>
 * \date 2017
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
#include <stdint.h>

#include "../unirec.h"
#include "fields.h"

UR_FIELDS(
   ipaddr SRC_IP,
   ipaddr DST_IP,
   uint8 PROTOCOL,
   uint32 PACKETS,
   uint64 BYTES,
   string URL,
   string MESSAGE_CMP,
)
int main(int argc, char **argv)
{
   // Create a templates
   ur_template_t *tmpltA1 = ur_create_template("SRC_IP,DST_IP,PROTOCOL,PACKETS,BYTES,URL", NULL);
   if (tmpltA1 == NULL) {
      fprintf(stderr, "Error when creating UniRec template.\n");
      return 1;
   }

   ur_template_t *tmpltA2 = ur_create_template("PACKETS,URL,BYTES,PROTOCOL,SRC_IP,DST_IP", NULL);
   if (tmpltA2 == NULL) {
      fprintf(stderr, "Error when creating UniRec template.\n");
      return 1;
   }

   ur_template_t *tmpltB = ur_create_template("SRC_IP,DST_IP,PROTOCOL,PACKETS,BYTES,MESSAGE_CMP", NULL);
   if (tmpltB == NULL) {
      fprintf(stderr, "Error when creating UniRec template.\n");
      return 1;
   }

   // Comparison check
   if (ur_template_compare(tmpltA1, tmpltA2) == 0) {
      fprintf(stderr, "ur_template_compare() returned \"not-equal\" on templates with same fields.\n");
      return 2;
   }

   if (ur_template_compare(tmpltA1, tmpltB) != 0) {
      fprintf(stderr, "ur_template_compare() returned \"equal\" on templates with different fields.\n");
      return 2;
   }

   ur_free_template(tmpltA1);
   ur_free_template(tmpltA2);
   ur_free_template(tmpltB);

   return 0;
}
