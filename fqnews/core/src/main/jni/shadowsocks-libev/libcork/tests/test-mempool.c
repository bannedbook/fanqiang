/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2015, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "libcork/core/mempool.h"
#include "libcork/core/types.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Memory pools
 */


START_TEST(test_mempool_01)
{
#define OBJECT_COUNT  16
    DESCRIBE_TEST;
    struct cork_mempool  *mp;
    /* Small enough that we'll have to allocate a couple of blocks */
    mp = cork_mempool_new_ex(int64_t, 64);

    size_t  i;
    int64_t  *objects[OBJECT_COUNT];
    for (i = 0; i < OBJECT_COUNT; i++) {
        fail_if((objects[i] = cork_mempool_new_object(mp)) == NULL,
                "Cannot allocate object #%zu", i);
    }

    for (i = 0; i < OBJECT_COUNT; i++) {
        cork_mempool_free_object(mp, objects[i]);
    }

    for (i = 0; i < OBJECT_COUNT; i++) {
        fail_if((objects[i] = cork_mempool_new_object(mp)) == NULL,
                "Cannot reallocate object #%zu", i);
    }

    for (i = 0; i < OBJECT_COUNT; i++) {
        cork_mempool_free_object(mp, objects[i]);
    }

    cork_mempool_free(mp);
}
END_TEST

START_TEST(test_mempool_fail_01)
{
    DESCRIBE_TEST;
    struct cork_mempool  *mp;
    mp = cork_mempool_new(int64_t);

    int64_t  *obj;
    fail_if((obj = cork_mempool_new_object(mp)) == NULL,
            "Cannot allocate object");

    /* This should raise an assertion since we never freed obj. */
    cork_mempool_free(mp);
}
END_TEST


static void
int64_init(void *user_data, void *vobj)
{
    int64_t  *obj = vobj;
    *obj = 12;
}

static void
int64_done(void *user_data, void *vobj)
{
    size_t  *done_call_count = user_data;
    (*done_call_count)++;
}

/* This is based on our knowledge of the internal structure of a memory
 * pool's blocks and objects. */

#define OBJECTS_PER_BLOCK(block_size, obj_size) \
    (((block_size) - CORK_SIZEOF_POINTER) / (obj_size + CORK_SIZEOF_POINTER))

START_TEST(test_mempool_reuse_01)
{
#define BLOCK_SIZE  64
    DESCRIBE_TEST;
    size_t  done_call_count = 0;
    struct cork_mempool  *mp;
    mp = cork_mempool_new_ex(int64_t, BLOCK_SIZE);
    cork_mempool_set_user_data(mp, &done_call_count, NULL);
    cork_mempool_set_init_object(mp, int64_init);
    cork_mempool_set_done_object(mp, int64_done);

    int64_t  *obj;
    fail_if((obj = cork_mempool_new_object(mp)) == NULL,
            "Cannot allocate object");

    /* The init_object function sets the value to 12 */
    fail_unless(*obj == 12, "Unexpected value %" PRId64, *obj);

    /* Set the value to something new, free the object, then reallocate.
     * Since we know memory pools are LIFO, we should get back the same
     * object, unchanged. */
    *obj = 42;
    cork_mempool_free_object(mp, obj);
    fail_if((obj = cork_mempool_new_object(mp)) == NULL,
            "Cannot allocate object");
    fail_unless(*obj == 42, "Unexpected value %" PRId64, *obj);

    cork_mempool_free_object(mp, obj);
    cork_mempool_free(mp);

    fail_unless(done_call_count ==
                OBJECTS_PER_BLOCK(BLOCK_SIZE, sizeof(int64_t)),
                "done_object called an unexpected number of times: %zu",
                done_call_count);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("mempool");

    TCase  *tc_mempool = tcase_create("mempool");
    tcase_add_test(tc_mempool, test_mempool_01);
#if NDEBUG
    /* If we're not compiling assertions then this test won't abort */
    tcase_add_test(tc_mempool, test_mempool_fail_01);
#else
    tcase_add_test_raise_signal(tc_mempool, test_mempool_fail_01, SIGABRT);
#endif
    tcase_add_test(tc_mempool, test_mempool_reuse_01);
    suite_add_tcase(s, tc_mempool);

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

