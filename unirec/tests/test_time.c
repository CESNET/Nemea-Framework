/**
 * \file test_time.c
 * \brief Test for ur_time_t structure
 * \author Tomas Cejka <cejkat@cesnet.cz>
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

#include "../ur_time.h"
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

int main()
{
   uint8_t errors = 0;
   uint8_t res = 0;
   ur_time_t ut = 0, ut2 = 0;
   const char *badstr = "1.1.2018 16:52:54";
   const char *strmsec = "2018-06-27T16:52:54.123";
   const char *strusec = "2018-06-27T16:52:54.123456";
   const char *strnsec = "2018-06-27T16:52:54.122456789";
   const char *badstr2 = "2018-06-27T16:52:54.122456789000";
   const char *str = "2018-06-27T16:52:54";

   res = ur_time_from_string(&ut, NULL);
   if (res != 2) {
      fprintf(stderr, "Passing NULL didn't cause error.\n");
      return 1;
   }

   res = ur_time_from_string(NULL, str);
   if (res != 2) {
      fprintf(stderr, "Passing NULL didn't cause error.\n");
      return 1;
   }

   res = ur_time_from_string(&ut, badstr);
   if (res != 1) {
      fprintf(stderr, "Parsing succeeded while it should fail.\n");
      return 1;
   }

   res = ur_time_from_string(&ut, strmsec);
   if (res != 0) {
      fprintf(stderr, "Parsing failed while it should succeed.\n");
      errors++;
   }

   if (ur_time_get_msec(ut) != 122) {
      fprintf(stderr, "Number of miliseconds (%" PRIu32 ") is not the expected value (122).\n", ur_time_get_msec(ut));
      errors++;
   }

   res = ur_time_from_string(&ut, str);
   if (res != 0) {
      fprintf(stderr, "Parsing failed while it should succeed.\n");
      errors++;
   }

   if (ur_time_get_sec(ut) != 1530118374) {
      fprintf(stderr, "Number of seconds is not the expected value.\n");
      errors++;
   }

   if (ur_time_get_msec(ut) != 0) {
      fprintf(stderr, "Number of miliseconds is not the expected value.\n");
      errors++;
   }

   ut2 = ur_time_from_sec_msec(1530118373, 500);

   if (ur_timediff(ut, ut2) != 500) {
      fprintf(stderr, "Timediff returned unexpected result.\n");
      errors++;
   }

   ut = ur_time_from_sec_usec(1234567890, 123456);

   if (ur_time_get_usec(ut) != 123456) {
      fprintf(stderr, "Number of microseconds (%" PRIu32 "us / %" PRIu32 "ns) is not the expected value (123456).\n", ur_time_get_usec(ut), ur_time_get_nsec(ut));
      errors++;
   }
   if (ur_time_get_sec(ut) != 1234567890) {
      fprintf(stderr, "Number of seconds (%" PRIu32 ") is not the expected value (1234567890).\n", ur_time_get_sec(ut));
      errors++;
   }

   ut2 = ur_time_from_sec_usec(1234567890, 123456789);

   if (ur_time_get_nsec(ut) != 123456789) {
      fprintf(stderr, "Number of nanoseconds (%" PRIu32 ") is not the expected value (123456789).\n", ur_time_get_nsec(ut));
      errors++;
   }
   if (ur_time_get_sec(ut) != 1234567890) {
      fprintf(stderr, "Number of seconds (%" PRIu32 ") is not the expected value (1234567890).\n", ur_time_get_sec(ut));
      errors++;
   }

   printf("ur_timediff_us: %" PRIu64 "\n", ur_timediff_us(ut, ut2));
   printf("ur_timediff_ns: %" PRIu64 "\n", ur_timediff_ns(ut, ut2));

   res = ur_time_from_string(&ut, strusec);
   if (res != 0) {
      fprintf(stderr, "Parsing failed while it should succeed.\n");
      errors++;
   }
   if (ur_time_get_usec(ut) != 123456) {
      fprintf(stderr, "Number of microseconds (%" PRIu32 ") is not the expected value (123456).\n", ur_time_get_usec(ut));
      errors++;
   }

   res = ur_time_from_string(&ut, strnsec);
   if (res != 0) {
      fprintf(stderr, "Parsing failed while it should succeed.\n");
      errors++;
   }
   if (ur_time_get_usec(ut) != 122456789) {
      fprintf(stderr, "Number of nanoseconds (%" PRIu32 ") is not the expected value (122456789).\n", ur_time_get_nsec(ut));
      errors++;
   }

   res = ur_time_from_string(&ut, badstr2);
   if (res != 0) {
      fprintf(stderr, "Parsing failed while it should succeed.\n");
      errors++;
   }
   if (ur_time_get_usec(ut) != 122456789) {
      fprintf(stderr, "Number of nanoseconds (%" PRIu32 ") is not the expected value (122456789).\n", ur_time_get_nsec(ut));
      errors++;
   }

   return errors;
}

