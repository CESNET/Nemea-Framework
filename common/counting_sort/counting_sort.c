/*!
 * \file counting_sort.c
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

#include "../include/counting_sort.h"
#include <stdlib.h>
#include <string.h>

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
                          uint32_t key_max, cs_order order, uint32_t (*get_key) (const void *))
{
   uint32_t *keys;
   uint32_t keys_size = key_max - key_min + 1;

   /* Check input */
   if (!input || !output || !count || !size || (key_max <= key_min)) {
      return CS_BAD_PARAM;
   }

   /* Allocate memory for array of keys */
   keys = (uint32_t *) calloc(keys_size, sizeof(uint32_t));
   if (!keys) {
      return CS_MEMORY;
   }

   /* Create histogram */
   for (uint32_t i = 0; i < count; i++) {
      uint32_t key = get_key(((char *) input) + i * size) - key_min;
      /* Check if the provided key is legitimate */
      if (key >= keys_size) {
         free(keys);
         return CS_BAD_INDEX;
      }

      ++keys[key];
   }

   /* Sort items in descending order */
   if (order == CS_ORDER_DSC) {
      /* Compute prefix sum array */
      for (uint32_t i = keys_size - 1; i > 0; i--) {
         keys[i - 1] += keys[i];
      }

      /* Copy data from the input array to appropriate places of the output array */
      for (uint32_t i = count - 1;; i--) {
         uint32_t key = get_key(((char *) input) + i * size) - key_min;
         uint32_t out_index = (keys[key] - 1) * size;

         memcpy(((char *) output) + out_index, ((char *) input) + i * size, size);
         --keys[key];
         if (i == 0) {
            break;
         }
      }
   /* Sort items in ascending order */
   } else {
      /* Compute prefix sum array */
      for (uint32_t i = 1; i < keys_size; i++) {
         keys[i] += keys[i - 1];
      }

      /* Copy data from the input array to appropriate places of the output array */
      for (uint32_t i = count - 1;; i--) {
         uint32_t key = get_key(((char *) input) + i * size) - key_min;
         uint32_t out_index = (keys[key] - 1) * size;

         memcpy(((char *) output) + out_index, ((char *) input) + i * size, size);
         --keys[key];
         if (i == 0) {
            break;
         }
      }
   }

   free(keys);
   return CS_SUCCESS;
}
