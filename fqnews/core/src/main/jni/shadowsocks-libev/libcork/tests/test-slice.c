/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "libcork/ds/slice.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Static slices
 */

START_TEST(test_static_slice)
{
    static char  SRC[] = "Here is some text.";
    size_t  SRC_LEN = sizeof(SRC) - 1;

    struct cork_slice  slice;
    struct cork_slice  copy1;
    struct cork_slice  lcopy1;
    cork_slice_init_static(&slice, SRC, SRC_LEN);
    fail_if_error(cork_slice_copy(&copy1, &slice, 8, 4));
    fail_if_error(cork_slice_light_copy(&lcopy1, &slice, 8, 4));
    fail_if_error(cork_slice_slice(&slice, 8, 4));
    fail_unless(cork_slice_equal(&slice, &copy1), "Slices should be equal");
    fail_unless(cork_slice_equal(&slice, &lcopy1), "Slices should be equal");
    /* We have to finish lcopy1 first, since it's a light copy. */
    cork_slice_finish(&lcopy1);
    cork_slice_finish(&slice);
    cork_slice_finish(&copy1);
}
END_TEST


/*-----------------------------------------------------------------------
 * Copy-once slices
 */

START_TEST(test_copy_once_slice)
{
    static char  SRC[] = "Here is some text.";
    size_t  SRC_LEN = sizeof(SRC) - 1;

    struct cork_slice  slice;
    struct cork_slice  copy1;
    struct cork_slice  copy2;
    struct cork_slice  lcopy1;
    struct cork_slice  lcopy2;

    cork_slice_init_copy_once(&slice, SRC, SRC_LEN);
    fail_unless(slice.buf == SRC, "Unexpected slice buffer");

    fail_if_error(cork_slice_light_copy(&lcopy1, &slice, 8, 4));
    /* We should still be using the original SRC buffer directly, since we only
     * created a light copy. */
    fail_unless(slice.buf == SRC, "Unexpected slice buffer");
    fail_unless(slice.buf + 8 == lcopy1.buf, "Unexpected slice buffer");

    fail_if_error(cork_slice_copy(&copy1, &slice, 8, 4));
    fail_if_error(cork_slice_slice(&slice, 8, 4));
    /* Once we create a full copy, the content should have been moved into a
     * managed buffer, which will exist somewhere else in memory than the
     * original SRC pointer. */
    fail_unless(slice.buf != SRC, "Unexpected slice buffer");
    fail_unless(slice.buf == copy1.buf, "Unexpected slice buffer");
    /* The light copy that we made previously won't have been moved over to the
     * new managed buffer, though. */
    fail_unless(cork_slice_equal(&slice, &copy1), "Slices should be equal");

    /* Once we've switched over to the managed buffer, a new light copy should
     * still point into the managed buffer. */
    fail_if_error(cork_slice_light_copy(&lcopy2, &slice, 0, 4));
    fail_unless(slice.buf != SRC, "Unexpected slice buffer");
    fail_unless(slice.buf == lcopy2.buf, "Unexpected slice buffer");

    fail_if_error(cork_slice_copy(&copy2, &slice, 0, 4));
    /* The second full copy should not create a new managed buffer, it should
     * just increment the existing managed buffer's refcount. */
    fail_unless(slice.buf == copy2.buf, "Unexpected slice buffer");
    fail_unless(copy1.buf == copy2.buf, "Unexpected slice buffer");
    fail_unless(cork_slice_equal(&slice, &copy2), "Slices should be equal");
    fail_unless(cork_slice_equal(&copy1, &copy2), "Slices should be equal");

    /* We have to finish the light copies first. */
    cork_slice_finish(&lcopy1);
    cork_slice_finish(&lcopy2);
    cork_slice_finish(&slice);
    cork_slice_finish(&copy1);
    cork_slice_finish(&copy2);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("slice");

    TCase  *tc_slice = tcase_create("slice");
    tcase_add_test(tc_slice, test_static_slice);
    tcase_add_test(tc_slice, test_copy_once_slice);
    suite_add_tcase(s, tc_slice);

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
