/**
 * \file test_ur2csv.c
 * \brief Test of of UniRec API to create CSV-like representation of UniRec data
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2019
 */
/*
 * Copyright (C) 2019 CESNET
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../unirec.h"
#include "../unirec2csv.h"
#include "fields.h"

UR_FIELDS(
   ipaddr SRC_IP,
   ipaddr DST_IP,
   uint8 PROTOCOL,
   uint32 PACKETS,
   uint64 BYTES,
   string URL,
   bytes MESSAGE,
)

static void test_constructor(void **state)
{
   urcsv_t *ucsv = urcsv_init(NULL, ',');
   assert_null(ucsv);
}

static void test_header(void **state)
{
   char *str;
   ur_template_t *tmplt = ur_create_template("SRC_IP,DST_IP,PROTOCOL,PACKETS,BYTES,URL,MESSAGE", NULL);
   urcsv_t *ucsv = urcsv_init(tmplt, ',');
   str = urcsv_header(ucsv);
   assert_string_equal(str, "ipaddr DST_IP,ipaddr SRC_IP,uint64 BYTES,uint32 PACKETS,uint8 PROTOCOL,bytes MESSAGE,string URL");
   free(str);
   
   ur_free_template(tmplt);

   urcsv_free(&ucsv);
   assert_true(ucsv == NULL);
}

static void test_record(void **state)
{
   char *str;

   // Create a templates
   ur_template_t *tmplt = ur_create_template("SRC_IP,DST_IP,PROTOCOL,PACKETS,BYTES,URL,MESSAGE", NULL);

   void *rec = ur_create_record(tmplt, 100);

   ur_set_from_string(tmplt, rec, F_SRC_IP, "10.0.0.1");
   ur_set_from_string(tmplt, rec, F_DST_IP, "10.0.0.2");
   ur_set_from_string(tmplt, rec, F_PROTOCOL, "6");
   ur_set_from_string(tmplt, rec, F_PACKETS, "6");
   ur_set_from_string(tmplt, rec, F_BYTES, "6");
   ur_set_from_string(tmplt, rec, F_URL, "www.example.com");
   ur_set_var(tmplt, rec, F_MESSAGE, "0\x00\x01\x02\x09\x0A1", 7);


   urcsv_t *ucsv = urcsv_init(tmplt, ',');

   str = urcsv_header(ucsv);
   assert_string_equal(str, "ipaddr DST_IP,ipaddr SRC_IP,uint64 BYTES,uint32 PACKETS,uint8 PROTOCOL,bytes MESSAGE,string URL");
   free(str);

   str = urcsv_record(ucsv, rec);
   assert_string_equal(str, "10.0.0.2,10.0.0.1,6,6,6,3000010209a100,\"www.example.com\"");
   free(str);

   ur_set_from_string(tmplt, rec, F_URL, "www.\"example\".com");
   str = urcsv_record(ucsv, rec);
   assert_string_equal(str, "10.0.0.2,10.0.0.1,6,6,6,3000010209a100,\"www.\"\"example\"\".com\"");
   free(str);

   urcsv_free(&ucsv);
   assert_null(ucsv);

   ur_free_record(rec);
   ur_free_template(tmplt);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
       cmocka_unit_test(test_constructor),
       cmocka_unit_test(test_header),
       cmocka_unit_test(test_record)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

