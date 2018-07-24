/**
 * \file test_blackhole.c
 * \brief drop messages
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2017
 */
/*
 * Copyright (C) 2017 CESNET
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

#include <libtrap/trap.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>


int main(int argc, char **argv)
{
   uint64_t i;
   trap_ctx_t *ctx = trap_ctx_init3("testmodule", "test description", 0, 1, "b:", "test-service-ifc");
   if (ctx == NULL || trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
      fprintf(stderr, "Failed trap_ctx_init.\n");
      return 1;
   }
   sleep(1);
   trap_ctx_set_data_fmt(ctx, 0, TRAP_FMT_RAW);

   for (i = 0; i <= 200; i++) {
      trap_ctx_send(ctx, 0, &i, sizeof(i));
   }

   trap_ctx_finalize(&ctx);

   return 0;
}


