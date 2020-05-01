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
#include "libcork/ds/buffer.h"
#include "libcork/ds/dllist.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Doubly-linked lists
 */

struct int64_item {
    int64_t  value;
    struct cork_dllist_item  element;
};

static int
int64_sum(void *user_data, struct cork_dllist_item *element)
{
    int64_t  *sum = user_data;
    struct int64_item  *item =
        cork_container_of(element, struct int64_item, element);
    *sum += item->value;
    return 0;
}

static void
check_int64_list(struct cork_dllist *list, const char *expected)
{
    bool  first = true;
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    struct cork_dllist_item  *curr;
    struct cork_dllist_item  *next;
    struct int64_item  *item;
    cork_buffer_append_literal(&buf, "");
    cork_dllist_foreach(list, curr, next, struct int64_item, item, element) {
        if (first) {
            first = false;
        } else {
            cork_buffer_append_literal(&buf, ",");
        }
        cork_buffer_append_printf(&buf, "%" PRId64, item->value);
    }
    fail_unless_streq("List contents", expected, buf.buf);
    cork_buffer_done(&buf);
}

START_TEST(test_dllist)
{
    struct cork_dllist  list;
    struct cork_dllist_item  *curr;
    struct cork_dllist_item  *next;
    struct int64_item  *item;
    struct int64_item  item1;
    struct int64_item  item2;
    struct int64_item  item3;
    int64_t  sum;

    cork_dllist_init(&list);
    fail_unless(cork_dllist_size(&list) == 0,
                "Unexpected size of list: got %zu, expected 0",
                cork_dllist_size(&list));
    fail_unless(cork_dllist_is_empty(&list),
                "Expected empty list");
    check_int64_list(&list, "");

    item1.value = 1;
    cork_dllist_add(&list, &item1.element);
    fail_unless(cork_dllist_size(&list) == 1,
                "Unexpected size of list: got %zu, expected 1",
                cork_dllist_size(&list));
    check_int64_list(&list, "1");

    item2.value = 2;
    cork_dllist_add(&list, &item2.element);
    fail_unless(cork_dllist_size(&list) == 2,
                "Unexpected size of list: got %zu, expected 2",
                cork_dllist_size(&list));
    check_int64_list(&list, "1,2");

    item3.value = 3;
    cork_dllist_add(&list, &item3.element);
    fail_unless(cork_dllist_size(&list) == 3,
                "Unexpected size of list: got %zu, expected 3",
                cork_dllist_size(&list));
    check_int64_list(&list, "1,2,3");

    sum = 0;
    fail_if(cork_dllist_visit(&list, &sum, int64_sum));
    fail_unless(sum == 6,
                "Unexpected sum, got %ld, expected 6",
                (long) sum);

    sum = 0;
    cork_dllist_foreach(&list, curr, next, struct int64_item, item, element) {
        sum += item->value;
    }
    fail_unless(sum == 6,
                "Unexpected sum, got %ld, expected 6",
                (long) sum);

    cork_dllist_remove(&item2.element);
    fail_unless(cork_dllist_size(&list) == 2,
                "Unexpected size of list: got %zu, expected 2",
                cork_dllist_size(&list));
}
END_TEST

START_TEST(test_dllist_append)
{
    struct cork_dllist  list1 = CORK_DLLIST_INIT(list1);
    struct cork_dllist  list2 = CORK_DLLIST_INIT(list2);
    struct int64_item  item1;
    struct int64_item  item2;
    struct int64_item  item3;
    struct int64_item  item4;
    struct int64_item  item5;
    struct int64_item  item6;
    struct int64_item  item7;

    item1.value = 1;
    cork_dllist_add_to_head(&list1, &item1.element);
    check_int64_list(&list1, "1");

    item2.value = 2;
    cork_dllist_add_to_head(&list1, &item2.element);
    check_int64_list(&list1, "2,1");

    item3.value = 3;
    cork_dllist_add_to_tail(&list1, &item3.element);
    check_int64_list(&list1, "2,1,3");

    item4.value = 4;
    cork_dllist_add_to_tail(&list2, &item4.element);
    item5.value = 5;
    cork_dllist_add_to_tail(&list2, &item5.element);
    cork_dllist_add_list_to_head(&list1, &list2);
    check_int64_list(&list1, "4,5,2,1,3");

    item6.value = 6;
    cork_dllist_add_to_tail(&list2, &item6.element);
    item7.value = 7;
    cork_dllist_add_to_tail(&list2, &item7.element);
    cork_dllist_add_list_to_tail(&list1, &list2);
    check_int64_list(&list1, "4,5,2,1,3,6,7");
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("dllist");

    TCase  *tc_ds = tcase_create("dllist");
    tcase_add_test(tc_ds, test_dllist);
    tcase_add_test(tc_ds, test_dllist_append);
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
