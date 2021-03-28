/**
 * \file test_fileifc.c
 * \brief Store messages and read them again.
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
#include <string.h>

#define NO_MESSAGES 1000
#define DATAFILE "/tmp/testoutputfile"

typedef struct message_s {
   uint64_t a;
   uint32_t b;
   uint8_t d1;
   uint8_t d2;
   uint8_t d3;
   uint8_t d4;
} message_t;

void compute_values(message_t *m, uint64_t index)
{
   m->a = (index & 0xFFFFFFFF) | (index << 32);
   m->b = (uint32_t) index * 100;
   m->d1 = (uint8_t) index;
   m->d2 = (uint8_t) index + 1;
   m->d3 = (uint8_t) index + 2;
   m->d4 = (uint8_t) index + 3;
}

int main(int argc, char **argv)
{
   uint64_t i;
   message_t m;
   const void *read_m;
   uint16_t read_size;
   int ret = 0;

   trap_ctx_t *ctx = trap_ctx_init3("testmodule", "test description", 0, 1, "f:" DATAFILE ":w", NULL);
   if (ctx == NULL || trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
      fprintf(stderr, "Failed trap_ctx_init.\n");
      return 1;
   }
   trap_ctx_set_data_fmt(ctx, 0, TRAP_FMT_JSON, "test");

   for (i = 0; i < NO_MESSAGES; i++) {
      /* compute values in the message */
      compute_values(&m, i);

      /* send the message */
      trap_ctx_send(ctx, 0, &m, sizeof(m));
   }

   trap_ctx_finalize(&ctx);

   ctx = trap_ctx_init3("testmodule", "test description", 1, 0, "f:" DATAFILE, NULL);
   if (ctx == NULL || trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
      fprintf(stderr, "Failed trap_ctx_init.\n");
      return 1;
   }
   trap_ctx_set_required_fmt(ctx, 0, TRAP_FMT_JSON, "test");

   for (i = 0; i < NO_MESSAGES; i++) {
      trap_ctx_recv(ctx, 0, &read_m, &read_size);

      /* compute and check values in the message */
      compute_values(&m, i);
      if (read_size != sizeof(m)) {
         fprintf(stderr, "Size of stored and read messages (#%" PRIu64 ") don't match (%" PRIu16 " and %" PRIu64 ").\n", i, read_size, sizeof(m));
         ret = 1;
         break;
      }
      if (memcmp((void *) &m, read_m, read_size) != 0) {
         fprintf(stderr, "Stored and read messages (#%" PRIu64 ") don't match.\n", i);
         ret = 1;
         break;
      }
   }

   trap_ctx_finalize(&ctx);

   unlink(DATAFILE);

   return ret;
}

