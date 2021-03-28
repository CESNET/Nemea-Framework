/**
 * \file ur_time.h
 * \brief Types, macros and function for UniRec timestamp format.
 * \author Erik Sabik <xsabik02@stud.fit.vutbr.cz>
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2013
 * \date 2014
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
#ifndef _UR_TIME_H
#define _UR_TIME_H

#include <stdint.h>

/**
 * \defgroup urtime Timestamps API
 * @{
 */

/** \brief Type of timestamps used in UniRec
 * Timestamps in UniRec are stored as number of seconds from Unix epoch in
 * 64bit fixed point number (32bit integral part, 32bit fraction part).
 */
typedef uint64_t ur_time_t;

/** Constant used to convert nanoseconds to fraction of second on 32 bits.
 * Its value is 2^32/1e9 in fixed-point notation,
 * which is (2^32/1e9)*2^32 = 2^64/1e9 (rounded).
 */
#define UR_TIME_NSEC_TO_FRAC 0x44B82FA0AULL
//#define UR_TIME_USEC_TO_FRAC 0x10C6F7A0B5EEULL
//#define UR_TIME_MSEC_TO_FRAC 0x4189374BC6A7F0ULL

/*
Note: We could use spearate constants for direct conversion from ms, us
and ns, but each such constant would have a different rounding error.
It would result in incorrect results when a value would be encoded from
one precision and decoded in another one (e.g. store as ms -> read as us
wouldn't result in a number like "x.xxx000").
The only consistent way is to always convert to fixed-point using the
highest precision (ns) and do additional conversion to/from ms/us by
multiplying/dividing it by 1000 or 1000000, i.e. in decadic form.

The current implementation satisfies these rules:
(everything is related to the fractional part only, the integer part is easy)
- Converting the time to unirec and back using the same precision always results
  in exactly the same number. Example:
    123456us -> ur_time -> 123456us
- Setting the time in lower precision and reading it in higer precision
  results in a number ending with zeros. Example:
    123ms -> ur_time -> 123000000ns
- Seting the time in higher precision and reading it in lower precision
  results in a floored value (i.e. rounded down). Example:
    199999us -> ur_time -> 199ms
*/


/** \brief Convert seconds and nanoseconds to ur_time_t.
 * \param sec seconds
 * \param nsec nanoseconds
 * \return UniRec timestamp (ur_time_t)
 */
#define ur_time_from_sec_nsec(sec, nsec) \
   (ur_time_t) (((uint64_t) (sec) << 32) | (((uint64_t) (nsec) * UR_TIME_NSEC_TO_FRAC) >> 32))

/** \brief Convert seconds and microseconds to ur_time_t.
 * \param sec seconds
 * \param usec microseconds
 * \return UniRec timestamp (ur_time_t)
 */
#define ur_time_from_sec_usec(sec, usec) \
   (ur_time_t) (((uint64_t) (sec) << 32) | (((uint64_t) (usec) * 1000 * UR_TIME_NSEC_TO_FRAC) >> 32))

/** \brief Convert seconds and milliseconds to ur_time_t.
 * \param sec seconds
 * \param msec milliseconds
 * \return UniRec timestamp (ur_time_t)
 */
#define ur_time_from_sec_msec(sec, msec) \
   (ur_time_t) (((uint64_t) (sec) << 32) | (((uint64_t) (msec) * 1000000 * UR_TIME_NSEC_TO_FRAC) >> 32))


/** \brief Get number of seconds from ur_time_t
 * \param time UniRec timestamp
 * \return seconds
 */
#define ur_time_get_sec(time) \
   (uint32_t) ((uint64_t) (time) >> 32)


/** \brief Get number of nanoseconds from ur_time_t
 * \param time UniRec timestamp
 * \return nanoseconds
 */
#define ur_time_get_nsec(time) \
   (uint32_t) ((((uint64_t) (time) & 0xffffffff) * 1000000000ULL + 0xffffffff) >> 32)

/** \brief Get number of microeconds from ur_time_t
 * \param time UniRec timestamp
 * \return microseconds
 */
#define ur_time_get_usec(time) \
   (uint32_t) (ur_time_get_nsec(time) / 1000)

/** \brief Get number of milliseconds from ur_time_t
 * \param time UniRec timestamp
 * \return milliseconds
 */
#define ur_time_get_msec(time) \
   (uint32_t) (ur_time_get_nsec(time) / 1000000)


/**
 * Return a time difference between A and B in miliseconds.
 *
 * \param [in] a  Timestamp A
 * \param [in] b  Timestamp B
 * \returns abs(A - B), the result is in miliseconds.
 */
static inline uint64_t ur_timediff(ur_time_t a, ur_time_t b)
{
   ur_time_t c = (a > b) ? a - b : b - a;
   return (uint64_t) ur_time_get_sec(c) * 1000 + ur_time_get_msec(c);
}

/**
 * Return a time difference between A and B in microseconds.
 *
 * \param [in] a  Timestamp A
 * \param [in] b  Timestamp B
 * \returns abs(A - B), the result is in microseconds.
 */
static inline uint64_t ur_timediff_us(ur_time_t a, ur_time_t b)
{
   ur_time_t c = (a > b) ? a - b : b - a;
   return (uint64_t) ur_time_get_sec(c) * 1000000 + ur_time_get_usec(c);
}

/**
 * Return a time difference between A and B in nanoseconds.
 *
 * \param [in] a  Timestamp A
 * \param [in] b  Timestamp B
 * \returns abs(A - B), the result is in nanoseconds.
 */
static inline uint64_t ur_timediff_ns(ur_time_t a, ur_time_t b)
{
   ur_time_t c = (a > b) ? a - b : b - a;
   return (uint64_t) ur_time_get_sec(c) * 1000000000 + ur_time_get_nsec(c);
}

/**
 * Convert string value str into UniRec time ur.
 *
 * \param [out] ur   Target pointer to store result.
 * \param [in] str   String in the following format: 2018-06-27T16:52:54 or
 *                   2018-06-27T16:52:54.500, str may end with "Z" (2018-06-27T16:52:54Z or
 *                   2018-06-27T16:52:54.500Z) indicating UTC timezone explicitly. UTC is
 *                   recommended and should be used in any case.
 * \return 0 on success, 1 is returned on parsing error (malformed format of
 * str) and ur is set to 0, 2 on bad parameter (NULL was passed).
 */
uint8_t ur_time_from_string(ur_time_t *ur, const char *str);
// hint: implementation is in unirec.c

/**
 * @}
 */

#endif
