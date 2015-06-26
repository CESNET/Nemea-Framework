/**
 * \file   super_fast_hash.h
 * \brief  SuperFastHash (http://www.azillionmonkeys.com/qed/hash.html)
 * \author Paul Hsieh
 * \date 2013
 */

#include <stdlib.h>
#include <stdint.h>

#ifndef SUPER_FAST_HASH_H
#define SUPER_FAST_HASH_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \brief Function for fast computing hash from data.
 * \param data Pointer to data from which will be computed hash.
 * \param len  Size of data.
 * \return Computed 32bit hash value.
 */
uint32_t SuperFastHash (const char * data, int len);



#ifdef __cplusplus
}
#endif


#endif
