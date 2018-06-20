/*!
 * \file counting_sort.h
 * \brief Counting sort - A stable sorting algorithm which is not based on compare and exchange principle
 * \author Tomas Jansky <jansky@cesnet.cz>
 * \date 2018
 */

/*
 * Copyright (C) 2018 CESNET
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

#ifndef _NEMEA_COMMON_COUNTING_SORT_
#define _NEMEA_COMMON_COUNTING_SORT_

#include <stdint.h>

/**
 * \brief Possible orders - ascending and descending.
 */
typedef enum {
   CS_ORDER_ASC = 0,
   CS_ORDER_DSC
} cs_order;

/**
 * \brief Possible return codes of counting sort function.
 */
typedef enum {
   CS_SUCCESS = 0, /**< Success. */
   CS_BAD_PARAM,   /**< Indicates wrong input parameter (NULL pointers, zero number of items, zero item size, minimal key >= maximal key value... */
   CS_MEMORY,      /**< Indicates that internally used calloc function was unable to allocate memory. */
   CS_BAD_INDEX    /**< Indicates that one or more of keys does not fit into the specified range of possible key values. */
} cs_ret_code;

/**
 * Counting sort. Sorts an array o N items with known range of possible values (keys) K.
 * Only usable for items which keys are known to be in some limited range which is a small constant and not a linear function of the number of items.
 * Does not (and by its nature can not) work for items with negative key values or for items with key values represented by a floating point number.
 * Asymptotic time complexity: O(N + K)
 * Outplace - exact space complexity: 2N + K
 * Stable (two objects with equal keys appear in the same order in sorted output as they appear in the input array)
 *
 * \param[in]     input     Pointer to the first item of the array of items which need to be sorted.
 * \param[out]    output    Pointer to the array where should this function copy sorted items (must be at least same length as the input array!).
 * \param[in]     count     Number of items to sort.
 * \param[in]     size      Size of each sorted item.
 * \param[in]     key_min   Minimal possible value of an item key.
 * \param[in]     key_max   Maximal possible value of an item key.
 * \param[in]     order     Specified order in which the items should be sorted (CS_ORDER_ASC | CS_ORDER_DSC).
 * \param[in]     get_key   Pointer to a user-defined function which dereferences an item and returns its key.
 * \return        CS_SUCCESS on success,
                  error code otherwise (see details of cs_ret_code enum) and content of the output array is undefined
 */
cs_ret_code counting_sort(const void *input, void *output, uint32_t count, uint32_t size, uint32_t key_min,
                          uint32_t key_max, cs_order order, uint32_t (*get_key) (const void *));

#endif            /* _NEMEA_COMMON_COUNTING_SORT_ */
