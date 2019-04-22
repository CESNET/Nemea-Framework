/**
 * \file macaddr.h
 * \brief Structure to store MAC address and associated functions.
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

#ifndef _MACADDR_H_
#define _MACADDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "inline.h"

#define MAC_STR_LEN 18

#ifndef PRIx8
#define PRIx8 "x"
#endif

#ifndef SCNx8
#define SCNx8 "hhx"
#endif

#define MAC_ADD_FORMAT_SCN "%02" SCNx8 ":%02" SCNx8 ":%02" SCNx8 ":%02" SCNx8 ":%02" SCNx8 ":%02" SCNx8 ""
#define MAC_ADD_FORMAT_PRI "%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ""

/**
 * Structure containing MAC address bytes.
 */
typedef struct __attribute__((packed)) mac_addr_s {
   uint8_t bytes[6];
} mac_addr_t;

/**
 * Convert 6B array into mac_addr_t.
 *
 * \param[in] array 6B array containing MAC address bytes.
 * \returns MAC address stored as mac_addr_t.
 */
INLINE mac_addr_t mac_from_bytes(uint8_t *array)
{
   mac_addr_t tmp;

   memcpy(&tmp.bytes, array, 6);

   return tmp;
}

/**
 * Convert string into mac_addr_t.
 *
 * \param[in] str    String for conversion.
 * \param[out] addr  Pointer to memory where to store MAC address.
 * \returns 1 on success, 0 on error i.e. string is not a valid MAC address.
 */
INLINE int mac_from_str(const char *str, mac_addr_t *addr)
{
   int res = sscanf(str, MAC_ADD_FORMAT_SCN, &addr->bytes[0], &addr->bytes[1], &addr->bytes[2],
                    &addr->bytes[3], &addr->bytes[4], &addr->bytes[5]);
   if (res == 6) {
      return 1;
   } else {
      memset(addr->bytes, 0, 6);
      return 0;
   }
}

/**
 * Compare two MAC addresses.
 *
 * \param[in] addr1 MAC address as mac_addr_t
 * \param[in] addr2 MAC address as mac_addr_t
 * \returns Positive number (>0) if addr1 > addr2, negative number (<0) if addr1 < addr2, and zero (=0) if addr1 == addr2.
 */
INLINE int mac_cmp(const mac_addr_t *addr1, const mac_addr_t *addr2)
{
   return memcmp(addr1->bytes, addr2->bytes, 6);
}

/**
 * Convert mac_addr_t into string.
 *
 * \param[in] addr Pointer to MAC address.
 * \param[out] str Pointer to memory where to store converted MAC address. It must be of at least MAC_STR_LEN size.
 */
INLINE void mac_to_str(const mac_addr_t *addr, char *str)
{
   if (str != NULL) {
      snprintf(str, MAC_STR_LEN, MAC_ADD_FORMAT_PRI,
               addr->bytes[0], addr->bytes[1], addr->bytes[2],
               addr->bytes[3], addr->bytes[4], addr->bytes[5]);
   }
}

/**
 * Convert mac_addr_t into bytes array.
 *
 * \param[in] addr Pointer to MAC address.
 * \param[out] array Pointer to memory where to store MAC address bytes. It must be of at least 6B size.
 */
INLINE void mac_to_bytes(const mac_addr_t *addr, uint8_t *array)
{
   memcpy(array, (void *) addr->bytes, 6);
}

#ifdef __cplusplus
}
#endif

#endif
