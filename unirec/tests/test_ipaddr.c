/**
 * \file test_ipaddr.c
 * \brief Test for IP address structure
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

#include "../ipaddr.h"
#include <stdio.h>

int main()
{
   ip_addr_t addr;
   char str[46];

   addr = ip_from_int(0x80ff7f01); // 128.255.127.1
   ip_to_str(&addr, str);
   printf("128.255.127.1 int - %s\n", str);
   for (int i = 0; i < 16; i++)
      printf("%02x ", addr.bytes[i]);
   printf("\n\n");

   char bytes_le[4] = {1,127,255,128}; // 128.255.127.1
   addr = ip_from_4_bytes_le(bytes_le);
   ip_to_str(&addr, str);
   printf("128.255.127.1 bytes LE - %s\n", str);
   for (int i = 0; i < 16; i++)
      printf("%02x ", addr.bytes[i]);
   printf("\n\n");

   char bytes_be[4] = {128,255,127,1}; // 128.255.127.1
   addr = ip_from_4_bytes_be(bytes_be);
   ip_to_str(&addr, str);
   printf("128.255.127.1 bytes BE - %s\n", str);
   for (int i = 0; i < 16; i++)
      printf("%02x ", addr.bytes[i]);
   printf("\n\n");

   char bytes2_le[16] = {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0}; // 0:1:2:3:4:5:6:7:8:9:a:b:c:d:e:f
   addr = ip_from_16_bytes_le(bytes2_le);
   ip_to_str(&addr, str);
   printf("0001:0203:0405:0607:0809:0a0b:0c0d:0e0f bytes LE - %s\n", str);
   for (int i = 0; i < 16; i++)
      printf("%02x ", addr.bytes[i]);
   printf("\n\n");

   char bytes2_be[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}; // 0:1:2:3:4:5:6:7:8:9:a:b:c:d:e:f
   addr = ip_from_16_bytes_be(bytes2_be);
   ip_to_str(&addr, str);
   printf("0001:0203:0405:0607:0809:0a0b:0c0d:0e0f bytes BE - %s\n", str);
   for (int i = 0; i < 16; i++)
      printf("%02x ", addr.bytes[i]);
   printf("\n\n");

   ip_from_str("1.2.3.4", &addr);
   ip_to_str(&addr, str);
   printf("1.2.3.4 str - %s\n", str);
   for (int i = 0; i < 16; i++)
      printf("%02x ", addr.bytes[i]);
   printf("\n\n");

   ip_from_str("2001:aa:bb:cc::1234:5678", &addr);
   ip_to_str(&addr, str);
   printf("2001:aa:bb:cc::1234:5678 str - %s\n", str);
   for (int i = 0; i < 16; i++)
      printf("%02x ", addr.bytes[i]);
   printf("\n\n");

   return 0;
}
