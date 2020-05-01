/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "libcork/core/allocator.h"
#include "libcork/core/types.h"
#include "libcork/threads/atomics.h"
#include "libcork/threads/basics.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Atomics
 */

#define test_atomic_op(name, type, fmt, op, expected) \
    do { \
        type  actual = cork_##name##_atomic_##op(&val, 1); \
        fail_unless_equal(#name, fmt, expected, actual); \
    } while (0)

#define test_cas(name, type, fmt, ov, nv) \
    do { \
        type  actual = cork_##name##_cas(&val, ov, nv); \
        fail_unless_equal(#name, fmt, ov, actual); \
    } while (0)

#define test_atomic(name, type, fmt) \
START_TEST(test_atomic_##name) \
{ \
    DESCRIBE_TEST; \
    volatile type  val = 0; \
    test_atomic_op(name, type, fmt, add, 1); \
    test_atomic_op(name, type, fmt, pre_add, 1); \
    test_atomic_op(name, type, fmt, add, 3); \
    test_atomic_op(name, type, fmt, pre_add, 3); \
    fail_unless_equal(#name, fmt, 4, val); \
    test_atomic_op(name, type, fmt, sub, 3); \
    test_atomic_op(name, type, fmt, pre_sub, 3); \
    test_atomic_op(name, type, fmt, sub, 1); \
    test_atomic_op(name, type, fmt, pre_sub, 1); \
    fail_unless_equal(#name, fmt, 0, val); \
    \
    test_cas(name, type, fmt, 0, 1); \
    test_cas(name, type, fmt, 1, 10); \
    test_cas(name, type, fmt, 10, 2); \
    test_cas(name, type, fmt, 2, 0); \
    fail_unless_equal(#name, fmt, 0, val); \
} \
END_TEST

test_atomic(int,  int,          "%d");
test_atomic(uint, unsigned int, "%u");
test_atomic(size, size_t,       "%zu");

START_TEST(test_atomic_ptr)
{
    DESCRIBE_TEST;

    uint64_t  v0 = 0;
    uint64_t  v1 = 0;
    uint64_t  v2 = 0;
    uint64_t  v3 = 0;
    uint64_t * volatile  val = &v0;

    test_cas(ptr, uint64_t *, "%p", &v0, &v1);
    test_cas(ptr, uint64_t *, "%p", &v1, &v2);
    test_cas(ptr, uint64_t *, "%p", &v2, &v3);
    test_cas(ptr, uint64_t *, "%p", &v3, &v0);
    fail_unless_equal("ptr", "%p", &v0, val);
}
END_TEST


/*-----------------------------------------------------------------------
 * Once
 */

START_TEST(test_once)
{
    DESCRIBE_TEST;

    cork_once_barrier(once);
    static size_t  call_count = 0;
    static int  value = 0;

#define go \
    do { \
        call_count++; \
        value = 1; \
    } while (0)

    cork_once(once, go);
    fail_unless_equal("Value", "%d", 1, value);
    cork_once(once, go);
    fail_unless_equal("Value", "%d", 1, value);
    cork_once(once, go);
    fail_unless_equal("Value", "%d", 1, value);
    cork_once(once, go);
    fail_unless_equal("Value", "%d", 1, value);

    fail_unless_equal("Call count", "%zu", 1, call_count);
}
END_TEST

START_TEST(test_once_recursive)
{
    DESCRIBE_TEST;

    cork_once_barrier(once);
    static size_t  call_count = 0;
    static int  value = 0;

#define go \
    do { \
        call_count++; \
        value = 1; \
    } while (0)

    cork_once_recursive(once, go);
    fail_unless_equal("Value", "%d", 1, value);
    cork_once_recursive(once, go);
    fail_unless_equal("Value", "%d", 1, value);
    cork_once_recursive(once, go);
    fail_unless_equal("Value", "%d", 1, value);
    cork_once_recursive(once, go);
    fail_unless_equal("Value", "%d", 1, value);

    fail_unless_equal("Call count", "%zu", 1, call_count);
}
END_TEST


/*-----------------------------------------------------------------------
 * Thread IDs
 */

START_TEST(test_thread_ids)
{
    DESCRIBE_TEST;
    cork_thread_id  id = cork_current_thread_get_id();
    fail_if(id == CORK_THREAD_NONE, "Expected a valid thread ID");
}
END_TEST


/*-----------------------------------------------------------------------
 * Threads
 */

struct cork_test_thread {
    int  *dest;
    int  value;
};

static int
cork_test_thread__run(void *vself)
{
    struct cork_test_thread  *self = vself;
    *self->dest = self->value;
    return 0;
}

static void
cork_test_thread__free(void *vself)
{
    struct cork_test_thread  *self = vself;
    cork_delete(struct cork_test_thread, self);
}

static struct cork_thread *
cork_test_thread_new(const char *name, int *dest, int value)
{
    struct cork_test_thread  *self = cork_new(struct cork_test_thread);
    self->dest = dest;
    self->value = value;
    return cork_thread_new
        (name, self, cork_test_thread__free, cork_test_thread__run);
}


static int
cork_error_thread__run(void *vself)
{
    /* The particular error doesn't matter; just want to make sure it gets
     * propagated from the cork_thread_join call. */
    cork_system_error_set_explicit(ENOMEM);
    return -1;
}


START_TEST(test_threads_01)
{
    struct cork_thread  *t1;
    int  v1 = -1;

    DESCRIBE_TEST;

    fail_if_error(t1 = cork_test_thread_new("test", &v1, 1));
    fail_unless_equal("Values", "%d", -1, v1);
    cork_thread_free(t1);
}
END_TEST

START_TEST(test_threads_02)
{
    struct cork_thread  *t1;
    int  v1 = -1;

    DESCRIBE_TEST;

    fail_if_error(t1 = cork_test_thread_new("test", &v1, 1));
    fail_if_error(cork_thread_start(t1));
    fail_if_error(cork_thread_join(t1));
    fail_unless_equal("Values", "%d", 1, v1);
}
END_TEST

START_TEST(test_threads_03)
{
    struct cork_thread  *t1;
    struct cork_thread  *t2;
    int  v1 = -1;
    int  v2 = -1;

    DESCRIBE_TEST;

    fail_if_error(t1 = cork_test_thread_new("test1", &v1, 1));
    fail_if_error(t2 = cork_test_thread_new("test2", &v2, 2));
    fail_if_error(cork_thread_start(t1));
    fail_if_error(cork_thread_start(t2));
    fail_if_error(cork_thread_join(t1));
    fail_if_error(cork_thread_join(t2));
    fail_unless_equal("Values", "%d", 1, v1);
    fail_unless_equal("Values", "%d", 2, v2);
}
END_TEST

START_TEST(test_threads_04)
{
    struct cork_thread  *t1;
    struct cork_thread  *t2;
    int  v1 = -1;
    int  v2 = -1;

    DESCRIBE_TEST;

    fail_if_error(t1 = cork_test_thread_new("test1", &v1, 1));
    fail_if_error(t2 = cork_test_thread_new("test2", &v2, 2));
    fail_if_error(cork_thread_start(t1));
    fail_if_error(cork_thread_start(t2));
    fail_if_error(cork_thread_join(t2));
    fail_if_error(cork_thread_join(t1));
    fail_unless_equal("Values", "%d", 1, v1);
    fail_unless_equal("Values", "%d", 2, v2);
}
END_TEST

START_TEST(test_threads_error_01)
{
    DESCRIBE_TEST;
    struct cork_thread  *t1;

    fail_if_error(t1 = cork_thread_new
                  ("test", NULL, NULL, cork_error_thread__run));
    fail_if_error(cork_thread_start(t1));
    fail_unless_error(cork_thread_join(t1));
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("threads");

    TCase  *tc_atomic = tcase_create("atomic");
    tcase_add_test(tc_atomic, test_atomic_int);
    tcase_add_test(tc_atomic, test_atomic_uint);
    tcase_add_test(tc_atomic, test_atomic_size);
    tcase_add_test(tc_atomic, test_atomic_ptr);
    suite_add_tcase(s, tc_atomic);

    TCase  *tc_basics = tcase_create("basics");
    tcase_add_test(tc_basics, test_once);
    tcase_add_test(tc_basics, test_once_recursive);
    tcase_add_test(tc_basics, test_thread_ids);
    suite_add_tcase(s, tc_basics);

    TCase  *tc_threads = tcase_create("threads");
    tcase_add_test(tc_threads, test_threads_01);
    tcase_add_test(tc_threads, test_threads_02);
    tcase_add_test(tc_threads, test_threads_03);
    tcase_add_test(tc_threads, test_threads_04);
    tcase_add_test(tc_threads, test_threads_error_01);
    suite_add_tcase(s, tc_threads);

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
