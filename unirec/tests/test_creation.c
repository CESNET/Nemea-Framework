/**
 * \file test_creation.c
 * \brief Test for UniRec setting values in a cycle
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
 * \author Zdenek Rosa <rosazden@fit.cvut.cz>
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
#include <stdint.h>
#include <time.h>

#include "fields.h"

UR_FIELDS(
   ipaddr SRC_IP,
   ipaddr DST_IP,
   uint16 SRC_PORT,
   uint16 DST_PORT,
   uint8 PROTOCOL,
   uint32 PACKETS,
   uint64 BYTES,
   string URL,
)
int main(int argc, char **argv)
{
   // Create a template
   ur_template_t *tmplt = ur_create_template("SRC_IP, DST_IP ,SRC_PORT,DST_PORT,PROTOCOL,PACKETS,BYTES,URL", NULL);
   if (tmplt == NULL) {
      fprintf(stderr, "Error when creating UniRec template.\n");
      return 1;
   }

   // Allocate memory for a record
   void *rec = ur_create_record(tmplt, 128); // allocates size of static fields + 128 bytes for URL

   ip_addr_t src_addr, dst_addr;
   ip_from_str("1.0.0.0", &src_addr);
   ip_from_str("255.0.0.0", &dst_addr);
   int x = 0;
   uint16_t y = 0;
   uint64_t z = 0;
   const char *url1 = "www.example.com";
   const char *url2 = "something.example.com/index.html";

   time_t start_time = time(NULL);

   for (int i = 0; i < 1000000000; i++) {
      ur_set(tmplt, rec, F_SRC_IP, src_addr);
      ur_set(tmplt, rec, F_DST_IP, dst_addr);
      src_addr.ui32[2] += 1;
      dst_addr.ui32[2] += 7;
      ur_set(tmplt, rec, F_SRC_PORT, y++);
      ur_set(tmplt, rec, F_DST_PORT, y*=2);
      ur_set(tmplt, rec, F_PROTOCOL, x++);
      ur_set(tmplt, rec, F_PACKETS, x++);
      ur_set(tmplt, rec, F_BYTES, z+=100);
      // setting of dynamic fieds is not solved yet, this solution works for one dynamic field only
      if (i & 1) {
         ur_set_string(tmplt, rec, F_URL,url1);
      } else {
         ur_set_string(tmplt, rec, F_URL,url2);
      }
   }

   printf("Time: %fs\n", difftime(time(NULL), start_time));

   ur_free_record(rec);

   ur_free_template(tmplt);

   return 0;
}
