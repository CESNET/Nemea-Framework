/**
 * \file test_basic.c
 * \brief Test for basic unirec functions
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

struct flow_rec_s {
   ip_addr_t dst_ip;
   ip_addr_t src_ip;
   uint64_t  bytes;
   uint32_t  packets;
   uint16_t  dst_port;
   uint16_t  src_port;
   uint8_t   protocol;
   uint16_t  url_offset;
   uint16_t  url_len;
   char      url[20];
} __attribute__((packed));
typedef struct flow_rec_s flow_rec_t;


int main(int argc, char **argv)
{
#ifdef UNIREC
   ur_template_t *tmplt = ur_create_template("SRC_IP,DST_IP,SRC_PORT,DST_PORT,PROTOCOL,PACKETS,BYTES,URL", NULL);
   if (tmplt == NULL) {
      fprintf(stderr, "Error when creating UniRec template.\n");
      return 1;
   }

   char *rec =
#else
   flow_rec_t *rec = (flow_rec_t*)
#endif
      "asdfqwerASDFQWER"
      "asdfqwerASDFQWER"
      "bytesxyz"
      "pkts"
      "dp"
      "sp"
      "p"
      "\00\x00\x13\x00"
      "http://example.com/";

   int x = 0;
   uint16_t y = 0;
   uint64_t z = 0;
   char tmp_str[20];

   time_t start_time = time(NULL);

   for (int i = 0; i < 1000000000; i++) {
#ifdef UNIREC
      if (ip_is4(ur_get_ptr(tmplt, rec, F_SRC_IP)))
         x += 1;
      if (ip_is4(ur_get_ptr(tmplt, rec, F_DST_IP)))
         x += 1;
      y += ur_get(tmplt, rec, F_SRC_PORT);
      y += ur_get(tmplt, rec, F_DST_PORT);
      y += ur_get(tmplt, rec, F_PROTOCOL);
      z += ur_get(tmplt, rec, F_PACKETS);
      z += ur_get(tmplt, rec, F_BYTES);
      memcpy(tmp_str, ur_get_ptr(tmplt, rec, F_URL), ur_get_var_len(tmplt, rec, F_URL));
#else
      if (ip_is4(&rec->src_ip))
         x += 1;
      if (ip_is4(&rec->dst_ip))
         x += 1;
      y += rec->src_port;
      y += rec->dst_port;
      y += rec->protocol;
      z += rec->packets;
      z += rec->bytes;
      memcpy(tmp_str, rec->url, rec->url_len);
#endif
   }

   printf("%i %hu %lu\n", x, y, z);

   printf("Time: %fs\n", difftime(time(NULL), start_time));

#ifdef UNIREC
   ur_free_template(tmplt);
#endif

   return 0;
}
