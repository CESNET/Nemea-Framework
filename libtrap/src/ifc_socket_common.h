/**
 * \file ifc_socket_common.h
 * \brief This file contains common functions and structures used in socket based interfaces (tcp-ip / tls).
 * \author Matej Barnat <barnama1@fit.cvut.cz>
 * \date 2019
 */
/*
 * Copyright (C) 2013-2019 CESNET
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

#ifndef _ifc_socket_common_h_
#define _ifc_socket_common_h_

#define BUFFER_COUNT_PARAM_LENGTH    13   /**< Used for parsing ifc params */
#define BUFFER_SIZE_PARAM_LENGTH     12   /**< Used for parsing ifc params */
#define MAX_CLIENTS_PARAM_LENGTH     12   /**< Used for parsing ifc params */

#define DEFAULT_MAX_DATA_LENGTH  (sizeof(trap_buffer_header_t) + 1024) /**< Obsolete? */

#ifndef DEFAULT_BUFFER_COUNT
#define DEFAULT_BUFFER_COUNT     50       /**< Default buffer count */
#endif

#ifndef DEFAULT_BUFFER_SIZE
#define DEFAULT_BUFFER_SIZE      100000   /**< Default buffer size [bytes] */
#endif

#ifndef DEFAULT_MAX_CLIENTS
#define DEFAULT_MAX_CLIENTS      64       /**< Default size of client array */
#endif

#define NO_CLIENTS_SLEEP         100000   /**< Value used in usleep() when waiting for a client to connect */

/**
 * \brief Output buffer structure.
 */
typedef struct buffer_s {
    uint32_t wr_index;                    /**< Pointer to first free byte in buffer */
    uint64_t clients_bit_arr;             /**< Bit array of clients that have not yet received the buffer */

    uint8_t *header;                      /**< Pointer to first byte in buffer */
    uint8_t *data;                        /**< Pointer to first byte of buffer payload */
} buffer_t;

/**
 * \brief Array containing constants used for operations with bit arrays.
 */
static uint64_t mask[64] = {
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288,
        1048576, 2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728, 268435456, 536870912, 1073741824,
        2147483648, 4294967296, 8589934592, 17179869184, 34359738368, 68719476736, 137438953472, 274877906944,
        549755813888, 1099511627776, 2199023255552, 4398046511104, 8796093022208, 17592186044416, 35184372088832,
        70368744177664, 140737488355328, 281474976710656, 562949953421312, 1125899906842624, 2251799813685248,
        4503599627370496, 9007199254740992, 18014398509481984, 36028797018963968, 72057594037927936, 144115188075855872,
        288230376151711744, 576460752303423488, 1152921504606846976, 2305843009213693952, 4611686018427387904, 9223372036854775808ULL
};

/**
 * \brief Set i-th element (one bit) of 'bits' to 1.
 *
 * \param[in] bits Pointer to the bit array.
 * \param[in] i Target element's index in the 'bits' array.
 */
static inline void set_index(uint64_t *bits, int i)
{
   *bits = __sync_or_and_fetch(bits, mask[i]);
}

/**
 * \brief Set i-th element (one bit) of 'bits' to 0.
 *
 * \param[in] bits Pointer to the bit array.
 * \param[in] i Target element's index in the 'bits' array.
 */
static inline void del_index(uint64_t *bits, int i)
{
   *bits = __sync_and_and_fetch(bits, (0xffffffffffffffff - mask[i]));
}

/**
 * \brief Return value of i-th element (one bit) in the 'bits' array.
 *
 * \param[in] bits Pointer to the bit array.
 * \param[in] i Target element's index in the 'bits' array.
 *
 * \return Value of i-th element (one bit) in the 'bits' array.
 */
static inline uint64_t check_index(uint64_t bits, int i)
{
   return (bits & mask[i]);
}

/**
 * \brief Write data into buffer
 *
 * \param[in] buffer Pointer to the buffer.
 * \param[in] data Pointer to data to write.
 * \param[in] size Size of the data.
 */
static inline void insert_into_buffer(buffer_t *buffer, const void *data, uint16_t size)
{
   uint16_t *msize = (uint16_t *)(buffer->data + buffer->wr_index);
   (*msize) = htons(size);
   memcpy((void *)(msize + 1), data, size);
   buffer->wr_index += (size + sizeof(size));
}

#endif
