/**
 * \file trap_internal.c
 * \brief Internal functions and macros for libtrap
 * Verbose and debug macros from libcommlbr
 * \author Tomas Konir <Tomas.Konir@liberouter.org>
 * \author Milan Kovacik <xkovaci1@fi.muni.cz>
 * \author Vojtech Krmicek <xkrmicek@fi.muni.cz>
 * \author Juraj Blaho <xblaho00@stud.fit.vutbr.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2006-2011
 * \date 2013
 * \date 2014
 *
 * Copyright (C) 2006-2014 CESNET
 *
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in
 *	the documentation and/or other materials provided with the
 *	distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *	may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
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
#include <stdio.h>
#include "trap_internal.h"

/**
 * Verbose level storage
 */
int trap_debug = 0;
int trap_verbose = -1;

/**
 * \brief Return syslog and output level based on given verbose level
 *
 * \param level Verbose level
 * \param str_level String that is set according to given verbose level
 */
static void get_level(int level, char **str_level)
{
   static char *error="ERROR";
   static char *warning="WARNING";
   static char *notice="NOTICE";
   static char *verbose="VERBOSE";
   static char *adv_verbose="ADVANCED VERBOSE";
   static char *library="LIBRARY VERBOSE";

   switch (level) {
   case CL_ERROR:
      *str_level = error;
      break;
   case CL_WARNING:
      *str_level= warning;
      break;
   case CL_VERBOSE_OFF:
      *str_level= notice;
      break;
   case CL_VERBOSE_BASIC:
      *str_level= verbose;
      break;
   case CL_VERBOSE_ADVANCED:
      *str_level= adv_verbose;
      break;
   case CL_VERBOSE_LIBRARY:
      *str_level= library;
      break;
   default:
      *str_level= notice;
   }
}

/**
 * \brief send verbose message to stderr
 *
 * send verbose message to stderr. may change in future. don't use it directly
 *
 * \param level importance level
 * \param string format string, like printf function
 */
void trap_verbose_msg(int level, char *string)
{
   char *strl;
   get_level(level, &strl);
   fprintf(stderr, "%s: %s\n", strl, string);
   fflush(stderr);
   string[0] = 0;
}

#ifndef ATOMICOPS
static pthread_mutex_t atomic_mutex = PTHREAD_MUTEX_INITIALIZER;

_Bool __sync_bool_compare_and_swap_8(int64_t *ptr, int64_t oldvar, int64_t newval)
{
   pthread_mutex_lock(&atomic_mutex);
   int64_t tmp = *ptr;
   *ptr = newval;
   pthread_mutex_unlock(&atomic_mutex);
   return tmp == oldvar;
}

uint64_t __sync_fetch_and_add_8(uint64_t *ptr, uint64_t value)
{
   pthread_mutex_lock(&atomic_mutex);
   uint64_t tmp = *ptr;
   *ptr += value;
   pthread_mutex_unlock(&atomic_mutex);
   return tmp;
}

uint64_t __sync_add_and_fetch_8(uint64_t *ptr, uint64_t value)
{
   pthread_mutex_lock(&atomic_mutex);
   uint64_t tmp = *ptr;
   *ptr += value;
   pthread_mutex_unlock(&atomic_mutex);
   return tmp;
}

uint64_t __sync_and_and_fetch_8(uint64_t *ptr, uint64_t value)
{
   pthread_mutex_lock(&atomic_mutex);
   uint64_t tmp = *ptr;
   *ptr &= value;
   pthread_mutex_unlock(&atomic_mutex);
   return tmp;
}

uint64_t __sync_or_and_fetch_8(uint64_t *ptr, uint64_t value)
{
   pthread_mutex_lock(&atomic_mutex);
   uint64_t tmp = *ptr;
   *ptr |= value;
   pthread_mutex_unlock(&atomic_mutex);
   return tmp;
}

#endif

