/**
 * \file test_macaddr.c
 * \brief Test for MAC address structure
 * \author Jiri Havranek <havraji6@fit.cvut.cz>
 * \date 2017
 */
/*
 * Copyright (C) 2017 CESNET
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

#include "../macaddr.h"
#include <stdio.h>

int main()
{
   mac_addr_t addr1;
   mac_addr_t addr2;

   char tmp_str[MAC_STR_LEN];
   uint8_t tmp_bytes[6];

   char mac_str[] = "AA:BB:CC:DD:EE:FF";
   uint8_t mac_bytes[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

   /* addr1 STR -> MAC check. */
   if (!mac_from_str(mac_str, &addr1)) {
      fprintf(stderr, "Error: STR -> MAC test failed\n");
      return 1;
   }

   addr2 = mac_from_bytes(mac_bytes);

   /* addr1 MAC -> BYTES check. */
   mac_to_bytes(&addr1, tmp_bytes);
   if (memcmp(tmp_bytes, mac_bytes, 6)) {
      fprintf(stderr, "Error: MAC -> BYTES test failed\n");
      return 1;
   }

   /* addr2 MAC -> BYTES check. */
   mac_to_bytes(&addr2, tmp_bytes);
   if (memcmp(tmp_bytes, mac_bytes, 6)) {
      fprintf(stderr, "Error: MAC -> BYTES test failed\n");
      return 1;
   }

   /* addr1 and addr2 check. */
   if (mac_cmp(&addr1, &addr2)) {
      fprintf(stderr, "Error: MAC and MAC compare check failed\n");
      return 1;
   }

   /* MAC -> STR check. */
   mac_to_str(&addr1, tmp_str);
   mac_from_str(tmp_str, &addr2);

   /* addr1 and addr2 check. */
   if (mac_cmp(&addr1, &addr2)) {
      fprintf(stderr, "Error: MAC -> STR test failed\n");
      return 1;
   }

   return 0;
}
