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

int main()
{
   uint8_t res = 0;
   ur_time_t ut = 0, ut2 = 0;
   const char *badstr = "1.1.2018 16:52:54";
   const char *strmsec = "2018-06-27T16:52:54.123";
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
      return 1;
   }

   if (ur_time_get_msec(ut) != 123) {
      fprintf(stderr, "Number of miliseconds is not the expected value.\n");
      return 2;
   }

   res = ur_time_from_string(&ut, str);
   if (res != 0) {
      fprintf(stderr, "Parsing failed while it should succeed.\n");
      return 1;
   }

   if (ur_time_get_sec(ut) != 1530118374) {
      fprintf(stderr, "Number of seconds is not the expected value.\n");
      return 3;
   }

   if (ur_time_get_msec(ut) != 0) {
      fprintf(stderr, "Number of miliseconds is not the expected value.\n");
      return 3;
   }

   ut2 = ur_time_from_sec_msec(1530118373, 500);

   if (ur_timediff(ut, ut2) != 500) {
      fprintf(stderr, "Timediff returned unexpected result.\n");
      return 4;
   }
   return 0;
}

