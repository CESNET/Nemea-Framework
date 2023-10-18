/**
 * \file links.h
 * \brief Definition of structures and functions for handling LINK_BIT_FIELD.
 * Implementation is in unirec.c.
 * It uses link mask (hexadecimal, 64bit long number) to determine which links
 * are used and how they are used. It means, that one can use e.g. 9 links,
 * which do not have to fill ones in LINK_BIT_FIELD on positions 1 - 9 necessary
 * but on positions specified by link mask.
 * \author Pavel Krobot <xkrobo01@stud.fit.vutbr.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2013
 * \date 2015
 */
 /*
 * Copyright (C) 2013-2015 CESNET
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
#ifndef _UNIREC_LINKS_H_
#define _UNIREC_LINKS_H_

/**
 * \defgroup urlinks Links API
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "inline.h"

#define MAX_LINK_COUNT 64 //since link mask is held by uint64_t
/** \brief Links structure.
 * It contains a link count, link mask and link indexes. Array link_indexes
 * stores positions in LINK_BIT_FIELD for used links (1-n), so one can easily
 * compare LINK_BIT_FIELD with some link, indexed by common indexes 0 - (1-n).
 */
typedef struct {
   unsigned int link_count;
   uint64_t link_mask;
   uint64_t *link_indexes;
} ur_links_t;

/** \brief Create and initialize links structure.
 * Create new links structure and initialize it from link mask in string format
 * passed by parameter. String link mask is stored in uint64_t, thne link count
 * and link_indexes arrray is determined from it.
 * Structure created by this function should be destroyed by ur_free_links.
 * \param[in] mask String with link mask in hexadecimal format.
 * \return Pointer to newly created links structure or NULL on error.
 */
ur_links_t *ur_create_links(const char *mask);

/** \brief Destroy links structure.
 * Free all memory allocated for a links structure created previously by
 * ur_create_links.
 * \param[in] links Pointer to the links structure.
 */
void ur_free_links(ur_links_t *links);

/** \brief Get index of link (0 - (n-1))
 * Function gets search link_indexes array for value corresponding to passed
 * LINK_BIT_FIELD, which should contains only one "1" value. If more ones are
 * filled in LINK_BIT_FIELD, first from right is taken. Returns index to
 * link_indexes array (from interval 0 - (link_count-1)) or negative value if
 * correspondig value was not found.
 * \param[in] links Pointer to the links structure.
 * \param[in] link_bit_field Link bit field of given link.
 * \return Index of given link from interval 0 - (link_count-1).
 */
INLINE int ur_get_link_index(ur_links_t *links, uint64_t link_bit_field)
{
   unsigned int i;
   for (i = 0; i < links->link_count; ++i) {
      /* search for corresponding value */
      if ((link_bit_field >> links->link_indexes[i]) & 1L) {
         return i;
      }
   }
   /* ERROR */
   return -1;
}

/** \brief Get position in link_bit_field of link.
 * Get position in link_bit_field of link specified by index of link (from
 * interval 0 - (link_count-1)). This function is inversion to get_link_index.
 * Returns zero if invalid index is passed.
 * \param[in] links Pointer to the links structure.
 * \param[in] index Index of link from interval 0 - (link_count-1).
 * \return Position in link_bit_field.
 */
INLINE uint64_t ur_get_link_bit_field_position(ur_links_t *links, unsigned int index)
{
   if (index < links->link_count) {//index have to be from interval 0 - (n-1)
      return links->link_indexes[index];
   } else {
      return 0;//returns 0 on error since no link can possibly have value 1 on
               //position 0 in LINK_BIT_FIELD
   }
}

/** \brief Get link mask.
 * \param[in] links Pointer to the links structure.
 * \return Link mask in 64bit unsigned int.
 */
INLINE uint64_t ur_get_link_mask(ur_links_t *links)
{
   return links->link_mask;
}

/** \brief Get link count.
 * \param[in] links Pointer to the links structure.
 * \return Count of used links.
 */
INLINE unsigned int ur_get_link_count(ur_links_t *links)
{
   return links->link_count;
}

#ifdef __cplusplus
} // extern "C"
#endif

/**
 * @}
 */

#endif
// END OF links.h
