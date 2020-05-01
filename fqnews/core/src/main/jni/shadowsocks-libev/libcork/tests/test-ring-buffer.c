/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
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
#include "libcork/ds/ring-buffer.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Ring buffers
 */

START_TEST(test_ring_buffer_1)
{
    struct cork_ring_buffer  buf;
    cork_ring_buffer_init(&buf, 4);

    fail_unless(cork_ring_buffer_add(&buf, (void *) 1) == 0,
                "Cannot add to ring buffer");
    fail_unless(cork_ring_buffer_add(&buf, (void *) 2) == 0,
                "Cannot add to ring buffer");
    fail_unless(cork_ring_buffer_add(&buf, (void *) 3) == 0,
                "Cannot add to ring buffer");
    fail_unless(cork_ring_buffer_add(&buf, (void *) 4) == 0,
                "Cannot add to ring buffer");
    fail_if(cork_ring_buffer_add(&buf, (void *) 5) == 0,
            "Shouldn't be able to add to ring buffer");

    fail_unless(((intptr_t) cork_ring_buffer_peek(&buf)) == 1,
                "Unexpected head of ring buffer (peek)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(&buf)) == 1,
                "Unexpected head of ring buffer (pop)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(&buf)) == 2,
                "Unexpected head of ring buffer (pop)");

    fail_unless(cork_ring_buffer_add(&buf, (void *) 5) == 0,
                "Cannot add to ring buffer");
    fail_unless(cork_ring_buffer_add(&buf, (void *) 6) == 0,
                "Cannot add to ring buffer");
    fail_if(cork_ring_buffer_add(&buf, (void *) 7) == 0,
            "Shouldn't be able to add to ring buffer");

    fail_unless(((intptr_t) cork_ring_buffer_pop(&buf)) == 3,
                "Unexpected head of ring buffer (pop)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(&buf)) == 4,
                "Unexpected head of ring buffer (pop)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(&buf)) == 5,
                "Unexpected head of ring buffer (pop)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(&buf)) == 6,
                "Unexpected head of ring buffer (pop)");
    fail_unless(cork_ring_buffer_pop(&buf) == NULL,
                "Shouldn't be able to pop from ring buffer");

    cork_ring_buffer_done(&buf);
}
END_TEST


START_TEST(test_ring_buffer_2)
{
    struct cork_ring_buffer  *buf = cork_ring_buffer_new(4);

    fail_unless(cork_ring_buffer_add(buf, (void *) 1) == 0,
                "Cannot add to ring buffer");
    fail_unless(cork_ring_buffer_add(buf, (void *) 2) == 0,
                "Cannot add to ring buffer");
    fail_unless(cork_ring_buffer_add(buf, (void *) 3) == 0,
                "Cannot add to ring buffer");
    fail_unless(cork_ring_buffer_add(buf, (void *) 4) == 0,
                "Cannot add to ring buffer");
    fail_if(cork_ring_buffer_add(buf, (void *) 5) == 0,
            "Shouldn't be able to add to ring buffer");

    fail_unless(((intptr_t) cork_ring_buffer_peek(buf)) == 1,
                "Unexpected head of ring buffer (peek)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(buf)) == 1,
                "Unexpected head of ring buffer (pop)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(buf)) == 2,
                "Unexpected head of ring buffer (pop)");

    fail_unless(cork_ring_buffer_add(buf, (void *) 5) == 0,
                "Cannot add to ring buffer");
    fail_unless(cork_ring_buffer_add(buf, (void *) 6) == 0,
                "Cannot add to ring buffer");
    fail_if(cork_ring_buffer_add(buf, (void *) 7) == 0,
            "Shouldn't be able to add to ring buffer");

    fail_unless(((intptr_t) cork_ring_buffer_pop(buf)) == 3,
                "Unexpected head of ring buffer (pop)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(buf)) == 4,
                "Unexpected head of ring buffer (pop)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(buf)) == 5,
                "Unexpected head of ring buffer (pop)");
    fail_unless(((intptr_t) cork_ring_buffer_pop(buf)) == 6,
                "Unexpected head of ring buffer (pop)");
    fail_unless(cork_ring_buffer_pop(buf) == NULL,
                "Shouldn't be able to pop from ring buffer");

    cork_ring_buffer_free(buf);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("ring_buffer");

    TCase  *tc_ds = tcase_create("ring_buffer");
    tcase_add_test(tc_ds, test_ring_buffer_1);
    tcase_add_test(tc_ds, test_ring_buffer_2);
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
