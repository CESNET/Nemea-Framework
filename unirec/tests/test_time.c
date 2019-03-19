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
   const char *badstr2 = "2018-06-27T16:52:54.222222222000";
   const char *tzstr = "2018-06-27T16:52:54Z";
   const char *tzstr2 = "2018-06-27T16:52:54.123Z";
   const char *str = "2018-06-27T16:52:54";

   res = ur_time_from_string(&ut, NULL);
   if (res != 2) {
      fprintf(stderr, "1. Passing NULL didn't cause error.\n");
      return 1;
   }

   res = ur_time_from_string(NULL, str);
   if (res != 2) {
      fprintf(stderr, "2. Passing NULL didn't cause error.\n");
      return 1;
   }

   res = ur_time_from_string(&ut, badstr);
   if (res != 1) {
      fprintf(stderr, "3. Parsing succeeded while it should fail.\n");
      return 1;
   }

   res = ur_time_from_string(&ut, strmsec);
   if (res != 0) {
      fprintf(stderr, "4. Parsing failed while it should succeed.\n");
      errors++;
   }

   if (ur_time_get_msec(ut) != 123) {
      fprintf(stderr, "5. Number of milliseconds (%" PRIu32 ") is not the expected value (123).\n", ur_time_get_msec(ut));
      errors++;
   }

   res = ur_time_from_string(&ut, str);
   if (res != 0) {
      fprintf(stderr, "6. Parsing failed while it should succeed.\n");
      errors++;
   }

   if (ur_time_get_sec(ut) != 1530118374) {
      fprintf(stderr, "7. Number of seconds is not the expected value.\n");
      errors++;
   }

   if (ur_time_get_msec(ut) != 0) {
      fprintf(stderr, "8. Number of milliseconds is not the expected value.\n");
      errors++;
   }

   ut2 = ur_time_from_sec_msec(1530118373, 500);

   if (ur_timediff(ut, ut2) != 500) {
      fprintf(stderr, "9. Timediff returned unexpected result.\n");
      errors++;
   }

   ut = ur_time_from_sec_usec(1234567890, 123456);

   if (ur_time_get_usec(ut) != 123456) {
      fprintf(stderr, "10. Number of microseconds (%" PRIu32 "us / %" PRIu32 "ns) is not the expected value (123456).\n", ur_time_get_usec(ut), ur_time_get_nsec(ut));
      errors++;
   }
   if (ur_time_get_sec(ut) != 1234567890) {
      fprintf(stderr, "11. Number of seconds (%" PRIu32 ") is not the expected value (1234567890).\n", ur_time_get_sec(ut));
      errors++;
   }

   ut2 = ur_time_from_sec_usec(1234567890, 123456);

   if (ur_time_get_nsec(ut2) != 123456000) {
      fprintf(stderr, "12. Number of nanoseconds (%" PRIu32 ") is not the expected value (123456000).\n", ur_time_get_nsec(ut2));
      errors++;
   }
   if (ur_time_get_usec(ut2) != 123456) {
      fprintf(stderr, "13. Number of microseconds (%" PRIu32 ") is not the expected value (123456).\n", ur_time_get_usec(ut2));
      errors++;
   }
   if (ur_time_get_msec(ut2) != 123) {
      fprintf(stderr, "14. Number of milliseconds (%" PRIu32 ") is not the expected value (123).\n", ur_time_get_msec(ut2));
      errors++;
   }
   if (ur_time_get_sec(ut2) != 1234567890) {
      fprintf(stderr, "15. Number of seconds (%" PRIu32 ") is not the expected value (1234567890).\n", ur_time_get_sec(ut2));
      errors++;
   }

   /* Check that conversion to ur_time_t and back returns always the same value */
   int i = 0;
   uint32_t values_ms[] = {0, 1, 123, 567, 999};
   uint32_t values_us[] = {0, 1, 123456, 567890, 999999};
   uint32_t values_ns[] = {0, 1, 123456789, 555666777, 999999999};
   /* ms */
   for (i = 0; i < sizeof values_ms / sizeof values_ms[0]; i++) {
      /* write as msec */
      ut2 = ur_time_from_sec_msec(1551234567, values_ms[i]);
      /* read as all ms, us, ns */
      if (ur_time_get_msec(ut2) != values_ms[i]) {
         fprintf(stderr, "16(ms->ms,%i). Number of milliseconds (%" PRIu32 ") is not the expected value (%" PRIu32 ").\n", i, ur_time_get_msec(ut2), values_ms[i]);
         errors++;
      }
      if (ur_time_get_usec(ut2) != values_ms[i] * 1000) {
         fprintf(stderr, "16(ms->us,%i). Number of microseconds (%" PRIu32 ") is not the expected value (%" PRIu32 ").\n", i, ur_time_get_usec(ut2), values_ms[i] * 1000);
         errors++;
      }
      if (ur_time_get_nsec(ut2) != values_ms[i] * 1000000) {
         fprintf(stderr, "16(ms->ns,%i). Number of nanoseconds (%" PRIu32 ") is not the expected value (%" PRIu32 ").\n", i, ur_time_get_nsec(ut2), values_ms[i] * 1000000);
         errors++;
      }
      /* also check the seconds part */
      if (ur_time_get_sec(ut2) != 1551234567) {
         fprintf(stderr, "16(ms->s,%i). Number of seconds (%" PRIu32 ") is not the expected value (1551234567).\n", i, ur_time_get_sec(ut2));
         errors++;
      }
   }
   /* us */
   for (i = 0; i < sizeof values_us / sizeof values_us[0]; i++) {
      /* write as usec */
      ut2 = ur_time_from_sec_usec(1551234567, values_us[i]);
      /* read as all ms, us, ns */
      if (ur_time_get_msec(ut2) != values_us[i] / 1000) {
         fprintf(stderr, "16(us->ms,%i). Number of milliseconds (%" PRIu32 ") is not the expected value (%" PRIu32 ").\n", i, ur_time_get_msec(ut2), values_us[i] / 1000);
         errors++;
      }
      if (ur_time_get_usec(ut2) != values_us[i]) {
         fprintf(stderr, "16(us->us,%i). Number of microseconds (%" PRIu32 ") is not the expected value (%" PRIu32 ").\n", i, ur_time_get_usec(ut2), values_us[i]);
         errors++;
      }
      if (ur_time_get_nsec(ut2) != values_us[i] * 1000) {
         fprintf(stderr, "16(us->ns,%i). Number of nanoseconds (%" PRIu32 ") is not the expected value (%" PRIu32 ").\n", i, ur_time_get_nsec(ut2), values_us[i] * 1000);
         errors++;
      }
      /* also check the seconds part */
      if (ur_time_get_sec(ut2) != 1551234567) {
         fprintf(stderr, "16(us->s,%i). Number of seconds (%" PRIu32 ") is not the expected value (1551234567).\n", i, ur_time_get_sec(ut2));
         errors++;
      }
   }
   /* ns */
   for (i = 0; i < sizeof values_ns / sizeof values_ns[0]; i++) {
      /* write as nsec */
      ut2 = ur_time_from_sec_nsec(1551234567, values_ns[i]);
      /* read as all ms, us, ns */
      if (ur_time_get_msec(ut2) != values_ns[i] / 1000000) {
         fprintf(stderr, "16(ns->ms,%i). Number of milliseconds (%" PRIu32 ") is not the expected value (%" PRIu32 ").\n", i, ur_time_get_msec(ut2), values_ns[i] / 1000000);
         errors++;
      }
      if (ur_time_get_usec(ut2) != values_ns[i] / 1000) {
         fprintf(stderr, "16(ns->us,%i). Number of microseconds (%" PRIu32 ") is not the expected value (%" PRIu32 ").\n", i, ur_time_get_usec(ut2), values_ns[i] / 1000);
         errors++;
      }
      if (ur_time_get_nsec(ut2) != values_ns[i]) {
         fprintf(stderr, "16(ns->ns,%i). Number of nanoseconds (%" PRIu32 ") is not the expected value (%" PRIu32 ").\n", i, ur_time_get_nsec(ut2), values_ns[i]);
         errors++;
      }
      /* also check the seconds part */
      if (ur_time_get_sec(ut2) != 1551234567) {
         fprintf(stderr, "16(ns->s,%i). Number of seconds (%" PRIu32 ") is not the expected value (1551234567).\n", i, ur_time_get_sec(ut2));
         errors++;
      }
   }

   if (ur_timediff_us(ut, ut2) != 316666677876543) {
      printf("ur_timediff_us: %" PRIu64 ", %" PRIu32 ".%" PRIu32 " - %" PRIu32 ".%" PRIu32 "\n",
             ur_timediff_us(ut, ut2), ur_time_get_sec(ut), ur_time_get_usec(ut), ur_time_get_sec(ut2), ur_time_get_usec(ut2));
   }

   if (ur_timediff_ns(ut, ut2) != 316666677876543999) {
      printf("ur_timediff_ns: %" PRIu64 ", %" PRIu32 ".%" PRIu32 " - %" PRIu32 ".%" PRIu32 "\n",
             ur_timediff_ns(ut, ut2), ur_time_get_sec(ut), ur_time_get_nsec(ut), ur_time_get_sec(ut2), ur_time_get_nsec(ut2));
   }

   res = ur_time_from_string(&ut, strusec);
   if (res != 0) {
      fprintf(stderr, "17. Parsing failed while it should succeed.\n");
      errors++;
   }
   if (ur_time_get_usec(ut) != 123456) {
      fprintf(stderr, "18. Number of microseconds (%" PRIu32 ") is not the expected value (123456).\n", ur_time_get_usec(ut));
      errors++;
   }

   res = ur_time_from_string(&ut, strnsec);
   if (res != 0) {
      fprintf(stderr, "19. Parsing failed while it should succeed.\n");
      errors++;
   }
   if (ur_time_get_nsec(ut) != 122456789) {
      fprintf(stderr, "20. Number of nanoseconds (%" PRIu32 ") is not the expected value (122456789).\n", ur_time_get_nsec(ut));
      errors++;
   }

   res = ur_time_from_string(&ut, badstr2);
   if (res != 0) {
      fprintf(stderr, "21. Parsing failed while it should succeed.\n");
      errors++;
   }
   if (ur_time_get_nsec(ut) != 222222222) {
      fprintf(stderr, "22. Number of nanoseconds (%" PRIu32 ") is not the expected value (222222222).\n", ur_time_get_nsec(ut));
      errors++;
   }

   // string with timezone
   res = ur_time_from_string(&ut, tzstr);
   if (res != 0) {
      fprintf(stderr, "23. Parsing failed while it should succeed.\n");
      errors++;
   }
   res = ur_time_from_string(&ut, tzstr2);
   if (res != 0) {
      fprintf(stderr, "24. Parsing failed while it should succeed.\n");
      errors++;
   }

   return errors;
}

