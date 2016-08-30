/**
 * \file test_trap_buffer.c
 * \brief Unit tests for TRAP ring buffer data structure
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2016
 */
/*
 * Copyright (C) 2016 CESNET
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

#include "../src/trap_buffer.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define DEBUG_PRINTS 0

#if DEBUG_PRINTS
#define DBG_PRINT(...) printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif


void *__real__test_malloc(const size_t size, const char* file, const int line);

void *__wrap__test_malloc(size_t size)
{
   int fail = (int) mock();
   if (fail) {
      return NULL;
   } else {
      return __real__test_malloc(size, __FILE__, __LINE__);
   }
}

static void test_create_destroy(void **state)
{
   trap_buffer_t *b = NULL;
   (void) state; /* unused */

   b = tb_init(0, 0);
   assert_null(b);
   b = tb_init(0, 1);
   assert_null(b);
   b = tb_init(1, 0);
   assert_null(b);

   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   b = tb_init(10, 100000);
   assert_non_null(b);
   tb_destroy(&b);
   tb_destroy(&b);
   tb_destroy(NULL);
}

static void test_create_fail(void **state)
{
   trap_buffer_t *b = NULL;
   (void) state; /* unused */
   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 1);
   b = tb_init(10, 100000);
   assert_null(b);

   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 1);
   b = tb_init(10, 100000);
   assert_null(b);

   will_return(__wrap__test_malloc, 1);
   b = tb_init(10, 100000);
   assert_null(b);
}

static void test_insert_message(void **state)
{
   int res;
   const uint64_t START_DATA = 0xA0B0C0D001020304;
   uint64_t data = START_DATA;
   uint16_t data_size = sizeof(data);
   const void *data_pointer = (void *) &data;
   uint16_t i, bi, nblocks = 3, sblock = 100, iter;
   (void) state; /* unused */
   trap_buffer_t *b = NULL;

   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   b = tb_init(nblocks, sblock);

   iter = (sblock - sizeof(struct tb_block_data_s)) / (sizeof(data) + sizeof(data_size));

   assert_non_null(b);
   assert_true(tb_isblockfree(b->cur_wr_block) == TB_SUCCESS);

   for (bi = 0; bi < nblocks + 1; bi++) {
      for (i = 0; i < iter; i++) {
         res = tb_pushmess(b, (void *) &data, data_size);
         data++;
         DBG_PRINT("iter %" PRIu16 " res: %i\n", i, res);
         if ((i != 0 || bi == 0) && (bi < nblocks)) {
            assert_true(res == TB_SUCCESS);
         } else {
            if (bi >= nblocks) {
               assert_true(res == TB_FULL);
            } else {
               assert_true(res == TB_USED_NEWBLOCK);
            }
         }
      }
   }

   assert_true(tb_isblockfree(b->cur_wr_block) == TB_FULL);

   DBG_PRINT("-----------\nREADING\n------------\n");

   data = START_DATA;
   for (bi = 0; bi < nblocks + 1; bi++) {
      for (i = 0; i < iter; i++) {
         res = tb_getmess(b, &data_pointer, &data_size);

         DBG_PRINT("iter %" PRIu16 " res %i should get %" PRIx64 " and got %" PRIx64 "\n", i, res,
               data, *((uint64_t *) data_pointer));
         if ((i != 0 || bi == 0) && (bi < nblocks)) {
            assert_true(res == TB_SUCCESS);
            assert_true(*((uint64_t *) data_pointer) == data);
         } else {
            if (bi >= nblocks) {
               assert_true(res == TB_EMPTY);
            } else {
               assert_true(res == TB_USED_NEWBLOCK);
            }
         }
         data++;
      }
   }

   tb_destroy(&b);
}

static void test_insert_bigmessage(void **state)
{
   int res;
   const uint64_t START_DATA = 0xA0B0C0D001020304;
   uint64_t data = START_DATA;
   uint16_t data_size = sizeof(data);
   const void *data_pointer = (void *) &data;
   (void) state; /* unused */
   trap_buffer_t *b = NULL;

   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   b = tb_init(1, 20);

   /* not enough space for message bigger than blocksize: */
   res = tb_pushmess(b, data_pointer, b->blocksize * sizeof(data));
   assert_true(res == TB_ERROR);

   DBG_PRINT("-----------\nREADING\n------------\n");

   res = tb_getmess(b, &data_pointer, &data_size);
   assert_true(res == TB_EMPTY);

   tb_destroy(&b);
}

static void test_insert_message2(void **state)
{
   int res;
   const uint64_t START_DATA = 0xA0B0C0D001020304;
   uint64_t data = START_DATA;
   uint16_t data_size = sizeof(data);
   const void *data_pointer = (void *) &data;
   (void) state; /* unused */
   trap_buffer_t *b = NULL;

   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   b = tb_init(1, 20);
   uint32_t *p1 = (uint32_t *) &START_DATA;
   uint32_t *p2 = p1 + 1;


   res = tb_pushmess2(b, (void *) p1, sizeof(*p1), (void *) p2, sizeof(*p2));
   assert_true(res == TB_SUCCESS);

   /* not enough space for message bigger than blocksize: */
   res = tb_pushmess2(b, (void *) p1, sizeof(*p1), (void *) p2, b->blocksize * sizeof(*p2));
   DBG_PRINT("res: %i\n", res);
   assert_true(res == TB_ERROR);

   DBG_PRINT("-----------\nREADING\n------------\n");

   res = tb_getmess(b, &data_pointer, &data_size);
   assert_true(*((uint64_t *) data_pointer) == START_DATA);

   tb_destroy(&b);
}



static void test_ifcapproach(void **state)
{
   const uint64_t START_DATA = 0xA0B0C0D001020304;
   const uint16_t nblocks = 10;
   const uint32_t bsize = 200;

   trap_buffer_t *wrb = NULL, *rdb = NULL;
   (void) state; /* unused */
   uint16_t i;

   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   wrb = tb_init(nblocks, bsize);
   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   will_return(__wrap__test_malloc, 0);
   rdb = tb_init(nblocks, bsize);


   uint64_t data = START_DATA;
   uint16_t data_size = sizeof(data);
   for (i = 0; i < nblocks * bsize * sizeof(data); i++) {
      if (tb_pushmess(wrb, &data, sizeof(data)) == TB_FULL) {
         break;
      }
      DBG_PRINT("%" PRIx64 "\n", data);
      data++;
   }

   DBG_PRINT("#1 FILLING BUFFER as IFC DOES\n");
   tb_block_t *bl = NULL;
   int res;
   for (i = 0; i <= nblocks; i++) {
      TB_FILL_START(rdb, &bl, res);
      if (i >= nblocks) {
         assert_true(res == TB_FULL);
      } else {
         assert_true(res == TB_SUCCESS);
      }
      if (res == TB_SUCCESS) {
         memcpy(bl->data->data, wrb->blocks[i]->data->data, wrb->blocks[i]->data->size);

         TB_FILL_END(rdb, bl, wrb->blocks[i]->data->size);
      } else {
         DBG_PRINT("Filling full buffer.\n");
      }
   }

   data = START_DATA;
   const void *data_pointer = NULL;
   for (i = 0; i < nblocks * bsize * sizeof(data); i++) {
      if (tb_getmess(rdb, &data_pointer, &data_size) == TB_EMPTY) {
         break;
      }
      DBG_PRINT("%" PRIx64 " == %" PRIx64 "\n", *((uint64_t *) data_pointer), data);
      assert_true(*((uint64_t *) data_pointer) == data);
      data++;
   }

   tb_clear_unused(rdb);

   DBG_PRINT("#2 FILLING BUFFER as IFC DOES\n");
   for (i = 0; i <= nblocks; i++) {
      TB_FILL_START(rdb, &bl, res);
      if (i >= nblocks) {
         assert_true(res == TB_FULL);
      } else {
         DBG_PRINT("res %i\n", res);
         assert_true(res == TB_SUCCESS);
      }
      if (res == TB_SUCCESS) {
         memcpy(bl->data->data, wrb->blocks[i]->data->data, wrb->blocks[i]->data->size);

         TB_FILL_END(rdb, bl, wrb->blocks[i]->data->size);
      } else {
         DBG_PRINT("Filling full buffer.\n");
      }
   }

   for (i = 0; i <= nblocks; i++) {
      TB_FLUSH_START(wrb, &bl, res);

      if (i >= nblocks) {
         assert_true(bl->data->size == 0);
      } else {
         assert_memory_equal(bl->data->data, rdb->blocks[i]->data->data, bl->data->size);
      }

      if (res == TB_FULL) {
         bl->refcount = 0;

         TB_FLUSH_END(rdb, bl, 0);
      }
   }

   tb_destroy(&wrb);
   tb_destroy(&rdb);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
       cmocka_unit_test(test_create_destroy),
       cmocka_unit_test(test_create_fail),
       cmocka_unit_test(test_insert_message),
       cmocka_unit_test(test_insert_message2),
       cmocka_unit_test(test_insert_bigmessage),
       cmocka_unit_test(test_ifcapproach)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

