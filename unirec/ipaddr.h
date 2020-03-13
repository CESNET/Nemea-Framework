/**
 * \file ipaddr.h
 * \brief Structure to store both IPv4 and IPv6 addresses and associated functions.
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
 * \date 2013
 * \date 2014
 *
 * \defgroup uripaddr IP addresses API
 * @{
 *
 * \page ipaddr
 * @{
 * Structure and functions to handle generic IP addresses (IPv4 or IPv6).
 * IP addresses are stored on 128 bits. IPv6 addresses are stored directly.
 * IPv4 addresses are converted to 128 bit is this way:
 * \code
 * 0000:0000:0000:0000:<ipv4_addr>:ffff:ffff
 * \endcode
 * No valid IPv6 address should look like this so it's possible to determine
 * IP address version without explicitly storing any flag.
 *
 * Addresses are stored in big endian (network byte order).
 *
 * This implementation assumes the platform uses little endian (true for x86
 * architectures).
 *
 * \code
 * Layout of ip_addr_t union:
 *  MSB                                 LSB
 *  xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx
 * |      i64[0]       |       i64[1]      |
 * | i32[0]  | i32[1]  | i32[2]  | i32[3]  |
 * |bytes[0] ...              ... bytes[15]|
 * \endcode
 *
 * @}
 */
/*
 * Copyright (C) 2013,2014 CESNET
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

#ifndef _IPADDR_H_
#define _IPADDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "inline.h"

#include <stdint.h>
#include <string.h>
#ifndef __WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#else
#define ntohl(x) ((x & 0x0000000ff) << 24) | ((x & 0x0000ff00) << 8) | \
                 ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24)
#define htonl(x) ntohl(x)
#endif

/**
 * IP address data type.
 *
 * It is 128b data type.  User can access to parts of the IP address in several ways.
 */
typedef union ip_addr_u {
   /**
    * 16x 8b numbers
    */
   uint8_t  bytes[16];
   /**
    * same as bytes
    */
   uint8_t  ui8[16];
   /**
    * 8x 16b numbers
    */
   uint16_t ui16[8];
   /**
    * 4x 32b numbers
    */
   uint32_t ui32[4];
   /**
    * 2x 64b numbers
    */
   uint64_t ui64[2];
} ip_addr_t;


/**
 * Check if the address is IPv4.
 *
 * \param[in] addr   IP address that is checked.
 * \returns 1 if the address is IPv4, 0 otherwise.
 */
INLINE int ip_is4(const ip_addr_t *addr)
{
   return (addr->ui64[0] == 0 && addr->ui32[3] == 0xffffffff);
}

/**
 * Check if the address is IPv6.
 *
 * \param[in] addr   IP address that is checked.
 * \returns 1 if the address is IPv6, 0 otherwise.
 * \note It is a negation of ip_is4().
 */
INLINE int ip_is6(const ip_addr_t *addr)
{
   return !ip_is4(addr);
}

/**
 * Get IPv4 address as one 32b number.
 *
 * The function takes the appropriate 32b part of ip_addr_t and converts it
 * into host byte order.
 *
 * \param[in] addr   IP address for conversion.
 * \returns IP address as one 32b number in host byte order.
 */
INLINE uint32_t ip_get_v4_as_int(ip_addr_t *addr)
{
   return ntohl(addr->ui32[2]);
}

/**
 * Get pointer to byte array of IPv4 address.
 *
 * The function returns pointer to the first byte of IP address data structure.
 * Data is in network byte order.
 *
 * \param[in] addr   IP address for conversion.
 * \returns Pointer to the first byte of IPv4 address.
 */
INLINE char *ip_get_v4_as_bytes(const ip_addr_t *addr)
{
   return (char *) &addr->bytes[8];
}


/**
 * Convert 32b number into IPv4 address encoded as ip_addr_t.
 *
 * \param[in] i   32b number in host byte order for conversion.
 * \returns IPv4 address stored as ip_addr_t.
 */
INLINE ip_addr_t ip_from_int(uint32_t i)
{
   ip_addr_t a;
   a.ui64[0] = 0;
   a.ui32[2] = htonl(i);
   a.ui32[3] = 0xffffffff;
   return a;
}

/**
 * Convert 4B array (in network byte order) into IPv4 address encoded as ip_addr_t.
 *
 * \param[in] b   4 bytes array in network byte order for conversion.
 * \returns IPv4 address stored as ip_addr_t.
 * \see ip_from_int()
 */
INLINE ip_addr_t ip_from_4_bytes_be(char b[4])
{
   ip_addr_t a;
   a.ui64[0] = 0;
   a.bytes[8] = b[0];
   a.bytes[9] = b[1];
   a.bytes[10] = b[2];
   a.bytes[11] = b[3];
   a.ui32[3] = 0xffffffff;
   return a;
}

/**
 * Convert 4B array (in little-endian byte order) into IPv4 address encoded as ip_addr_t.
 *
 * \param[in] b   4 bytes array in little-endian byte order for conversion.
 * \returns IPv4 address stored as ip_addr_t.
 * \see ip_from_int()
 */
INLINE ip_addr_t ip_from_4_bytes_le(char b[4])
{
   ip_addr_t a;
   a.ui64[0] = 0;
   a.bytes[8]  = b[3];
   a.bytes[9]  = b[2];
   a.bytes[10] = b[1];
   a.bytes[11] = b[0];
   a.ui32[3] = 0xffffffff;
   return a;
}

/**
 * Convert 16B array (in network byte order) into IPv6 address encoded as ip_addr_t.
 *
 * \param[in] b   16 bytes array in network byte order for conversion.
 * \returns IPv6 address stored as ip_addr_t.
 */
INLINE ip_addr_t ip_from_16_bytes_be(char b[16])
{
   ip_addr_t a;
   memcpy(&a, b, 16);
   return a;
}

/**
 * Convert 16B array (in little-endian byte order) into IPv6 address encoded as ip_addr_t.
 *
 * \param[in] b   16 bytes array in little-endian byte order for conversion.
 * \returns IPv6 address stored as ip_addr_t.
 */
INLINE ip_addr_t ip_from_16_bytes_le(char b[16])
{
   ip_addr_t a;
   int i;
   for (i = 0; i < 16; i++) {
      a.bytes[i] = b[15-i];
   }
   return a;
}

/**
 * Compare two IP addresses.
 *
 * \param[in] addr1  IP address as ip_addr_t
 * \param[in] addr2  IP address as ip_addr_t
 * \returns Positive number (>0) if addr1 > addr2, negative number (<0) if addr1 < addr2, and zero (=0) if addr1 == addr2.
 */
INLINE int ip_cmp(const ip_addr_t *addr1, const ip_addr_t *addr2)
{
   return memcmp((const char *)addr1, (const char *)addr2, 16);
}

/**
 * Check if IP address is NULL/invalid/unspecified?
 *
 * There are two variants being used as invalid/unspecified address:
 * \code
 *   00000000 00000000 00000000 ffffffff (i.e. "0.0.0.0", zero IPv4)
 *   00000000 00000000 00000000 00000000 (i.e. "::", zero IPv6)
 * \endcode
 *
 * \param[in] addr   IP address to check.
 * \returns 1 if given address matches any of the ones above, otherwise returns 0.
 */
INLINE int ip_is_null(const ip_addr_t *addr)
{
   if (addr->ui64[0] == 0) {
      if (addr->ui64[1] == 0 || (addr->ui32[2] == 0 && addr->ui32[3] == 0xffffffff)) {
         return 1;
      }
   }
   return 0;
}

#ifndef __WIN32
/**
 * Convert string into ip_addr_t.
 *
 * \param[in] str    String for conversion.
 * \param[out] addr  Pointer to memory where to store converted IP address.
 * \returns 1 on success, 0 on error i.e. string is not a valid IP address.
 */
INLINE int ip_from_str(const char *str, ip_addr_t *addr)
{
   char tmp[16];
   if (strchr(str, ':') == NULL) { // IPv4
      if (inet_pton(AF_INET, str, (void *) tmp) != 1) {
         return 0; // err
      }
      *addr = ip_from_4_bytes_be(tmp);
      return 1;
   } else { // IPv6
      if (inet_pton(AF_INET6, str, (void *) tmp) != 1) {
         return 0; // err
      }
      *addr = ip_from_16_bytes_be(tmp);
      return 1;
   }
}

/**
 * Convert ip_addr_t into string.
 *
 * \param[in] addr   Pointer to IP address.
 * \param[out] str   Pointer to memory where to store converted IP address.  It must be of at least INET6_ADDRSTRLEN size.
 */
INLINE void ip_to_str(const ip_addr_t *addr, char *str)
{
   if (ip_is4(addr)) { // IPv4
      inet_ntop(AF_INET, ip_get_v4_as_bytes(addr), str, INET6_ADDRSTRLEN);
   } else { // IPv6
      inet_ntop(AF_INET6, addr, str, INET6_ADDRSTRLEN);
   }
}
#endif

#ifdef __cplusplus
}
#endif

#endif
/**
 * @}
 */
