/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "libcork/core/types.h"
#include "libcork/ds/bitset.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Bit sets
 */

static void
test_bitset_of_size(size_t bit_count)
{
    size_t  i;
    struct cork_bitset  *set1 = cork_bitset_new(bit_count);
    struct cork_bitset set2;

    for (i = 0; i < bit_count; i++) {
        cork_bitset_set(set1, i, true);
        fail_unless(cork_bitset_get(set1, i), "Unexpected value for bit %zu", i);
    }

    for (i = 0; i < bit_count; i++) {
        cork_bitset_set(set1, i, false);
        fail_if(cork_bitset_get(set1, i), "Unexpected value for bit %zu", i);
    }
    cork_bitset_free(set1);

    cork_bitset_init(&set2, bit_count);
    for (i = 0; i < bit_count; i++) {
        cork_bitset_set(&set2, i, true);
        fail_unless(cork_bitset_get(&set2, i), "Unexpected value for bit %zu", i);
    }

    for (i = 0; i < bit_count; i++) {
        cork_bitset_set(&set2, i, false);
        fail_if(cork_bitset_get(&set2, i), "Unexpected value for bit %zu", i);
    }
    cork_bitset_done(&set2);
}

START_TEST(test_bitset)
{
    DESCRIBE_TEST;
    /* Test a variety of sizes, with and without spillover bits. */
    test_bitset_of_size(1);
    test_bitset_of_size(2);
    test_bitset_of_size(3);
    test_bitset_of_size(4);
    test_bitset_of_size(5);
    test_bitset_of_size(6);
    test_bitset_of_size(7);
    test_bitset_of_size(8);
    test_bitset_of_size(9);
    test_bitset_of_size(10);
    test_bitset_of_size(11);
    test_bitset_of_size(12);
    test_bitset_of_size(13);
    test_bitset_of_size(14);
    test_bitset_of_size(15);
    test_bitset_of_size(16);
    test_bitset_of_size(65535);
    test_bitset_of_size(65536);
    test_bitset_of_size(65537);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("bits");

    TCase  *tc_ds = tcase_create("bits");
    tcase_set_timeout(tc_ds, 20.0);
    tcase_add_test(tc_ds, test_bitset);
    suite_add_tcase(s, tc_ds);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    setup_allocator();
    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
