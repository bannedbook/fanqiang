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

#include "libcork/core/allocator.h"
#include "libcork/core/hash.h"
#include "libcork/core/types.h"
#include "libcork/ds/buffer.h"
#include "libcork/ds/hash-table.h"

#include "helpers.h"

/*-----------------------------------------------------------------------
 * Integer hash tables
 */

static void
uint64__free(void *vi)
{
    uint64_t  *i = vi;
    cork_delete(uint64_t, i);
}

static bool
uint64__equals(void *user_data, const void *va, const void *vb)
{
    const uint64_t  *a = va;
    const uint64_t  *b = vb;
#if 0
    printf("Testing %p (%" PRIu64 ") and %p (%" PRIu64 ")\n",
           a, *a, b, *b);
#endif
    return *a == *b;
}

static cork_hash
uint64__hash(void *user_data, const void *velement)
{
    const uint64_t  *element = velement;
#if 0
    printf("Hashing %p (%" PRIu64 ")\n", element, *element);
#endif
    return (cork_hash) *element;
}

static enum cork_hash_table_map_result
uint64_sum(void *vsum, struct cork_hash_table_entry *entry)
{
    uint64_t  *sum = vsum;
    uint64_t  *value = entry->value;
    *sum += *value;
    return CORK_HASH_TABLE_MAP_CONTINUE;
}

static void
test_map_sum(struct cork_hash_table *table, uint64_t expected)
{
    uint64_t  sum = 0;
    cork_hash_table_map(table, &sum, uint64_sum);
    fail_unless(sum == expected,
                "Unexpected map sum, got %" PRIu64
                ", expected %" PRIu64,
                sum, expected);
}

static void
test_iterator_sum(struct cork_hash_table *table, uint64_t expected)
{
    uint64_t  sum = 0;
    struct cork_hash_table_iterator  iterator;
    struct cork_hash_table_entry  *entry;
    cork_hash_table_iterator_init(table, &iterator);
    while ((entry = cork_hash_table_iterator_next(&iterator)) != NULL) {
        uint64_t  *value_ptr = entry->value;
        sum += *value_ptr;
    }
    fail_unless(sum == expected,
                "Unexpected iterator sum, got %" PRIu64
                ", expected %" PRIu64 "",
                sum, expected);
}

static enum cork_hash_table_map_result
uint64_to_string(void *vdest, struct cork_hash_table_entry *entry)
{
    struct cork_buffer  *dest = vdest;
    uint64_t  *key = entry->key;
    uint64_t  *value = entry->value;
    if (dest->size > 1) {
        cork_buffer_append(dest, ", ", 2);
    }
    cork_buffer_append_printf(dest, "%" PRIu64 ":%" PRIu64, *key, *value);
    return CORK_HASH_TABLE_MAP_CONTINUE;
}

static void
test_map_to_string(struct cork_hash_table *table, const char *expected)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    cork_buffer_set(&buf, "[", 1);
    cork_hash_table_map(table, &buf, uint64_to_string);
    cork_buffer_append(&buf, "]", 1);
    fail_unless_streq("Integer arrays", expected, buf.buf);
    cork_buffer_done(&buf);
}

static void
test_iterator_to_string(struct cork_hash_table *table, const char *expected)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    struct cork_hash_table_iterator  iterator;
    struct cork_hash_table_entry  *entry;
    cork_buffer_set(&buf, "[", 1);
    cork_hash_table_iterator_init(table, &iterator);
    while ((entry = cork_hash_table_iterator_next(&iterator)) != NULL) {
        uint64_t  *key = entry->key;
        uint64_t  *value = entry->value;
        if (buf.size > 1) {
            cork_buffer_append(&buf, ", ", 2);
        }
        cork_buffer_append_printf(&buf, "%" PRIu64 ":%" PRIu64, *key, *value);
    }
    cork_buffer_append(&buf, "]", 1);
    fail_unless_streq("Integer arrays", expected, buf.buf);
    cork_buffer_done(&buf);
}

START_TEST(test_uint64_hash_table)
{
    struct cork_hash_table  *table;
    uint64_t  key, *key_ptr, *old_key;
    void  *v_key, *v_value;
    uint64_t  *value_ptr, *old_value;
    bool  is_new;
    struct cork_hash_table_entry  *entry;

    table = cork_hash_table_new(0, 0);
    cork_hash_table_set_hash(table, uint64__hash);
    cork_hash_table_set_equals(table, uint64__equals);
    cork_hash_table_set_free_key(table, uint64__free);
    cork_hash_table_set_free_value(table, uint64__free);
    fail_unless(cork_hash_table_size(table) == 0,
                "Hash table should start empty");

    key = 0;
    fail_unless(cork_hash_table_get(table, &key) == NULL,
                "Shouldn't get value pointer from empty hash table");

    test_map_sum(table, 0);
    test_map_to_string(table, "[]");
    test_iterator_sum(table, 0);
    test_iterator_to_string(table, "[]");

    key_ptr = cork_new(uint64_t);
    *key_ptr = 0;
    value_ptr = cork_new(uint64_t);
    *value_ptr = 32;
    fail_if_error(cork_hash_table_put
                  (table, key_ptr, value_ptr,
                   &is_new, &v_key, &v_value));
    fail_unless(is_new, "Couldn't append {0=>32} to hash table");
    old_key = v_key;
    old_value = v_value;

    fail_unless(old_key == NULL,
                "Unexpected previous key");
    fail_unless(old_value == NULL,
                "Unexpected previous value");

    fail_unless(cork_hash_table_size(table) == 1,
                "Unexpected size after adding {0->32}");

    fail_if_error(entry = cork_hash_table_get_or_create
                  (table, &key, &is_new));
    fail_if(is_new, "Shouldn't create new {0=>X} entry");
    value_ptr = entry->value;
    fail_unless(*value_ptr == 32,
                "Unexpected value for {0=>X} entry");

    fail_unless(cork_hash_table_size(table) == 1,
                "Unexpected size after retrieving {0->32}");

    key = 1;
    fail_if_error(entry = cork_hash_table_get_or_create
                  (table, &key, &is_new));
    fail_unless(is_new, "Should create new {1=>X} entry");
    key_ptr = cork_new(uint64_t);
    *key_ptr = key;
    entry->key = key_ptr;
    value_ptr = cork_new(uint64_t);
    *value_ptr = 2;
    entry->value = value_ptr;

    fail_unless(cork_hash_table_size(table) == 2,
                "Unexpected size after adding {1=>2}");

    test_map_sum(table, 34);
    test_map_to_string(table, "[0:32, 1:2]");
    test_iterator_sum(table, 34);
    test_iterator_to_string(table, "[0:32, 1:2]");

    key = 0;
    fail_unless(cork_hash_table_delete(table, &key, NULL, NULL),
                "Couldn't delete {0=>32}");

    fail_unless(cork_hash_table_size(table) == 1,
                "Unexpected size after deleting entry");

    test_map_to_string(table, "[1:2]");
    test_iterator_to_string(table, "[1:2]");

    key = 3;
    fail_if(cork_hash_table_delete(table, &key, NULL, NULL),
            "Shouldn't be able to delete nonexistent {3=>X}");

    cork_hash_table_delete_entry(table, entry);

    fail_unless(cork_hash_table_size(table) == 0,
                "Unexpected size after deleting last entry");

    /*
     * Add the entries back so that we can try deleting them using
     * cork_hash_table_map.
     */

    key_ptr = cork_new(uint64_t);
    *key_ptr = 0;
    value_ptr = cork_new(uint64_t);
    *value_ptr = 32;
    fail_if_error(cork_hash_table_put
                  (table, key_ptr, value_ptr,
                   &is_new, &v_key, &v_value));
    fail_unless(is_new, "Couldn't append {0=>32} to hash table");
    old_key = v_key;
    old_value = v_value;

    key_ptr = cork_new(uint64_t);
    *key_ptr = 1;
    value_ptr = cork_new(uint64_t);
    *value_ptr = 2;
    fail_if_error(cork_hash_table_put
                  (table, key_ptr, value_ptr,
                   &is_new, &v_key, &v_value));
    fail_unless(is_new, "Couldn't append {1=>2} to hash table");
    old_key = v_key;
    old_value = v_value;

    cork_hash_table_clear(table);
    fail_unless(cork_hash_table_size(table) == 0,
                "Unexpected size after deleting entries using map");

    /* And we're done, so let's free everything. */
    cork_hash_table_free(table);
}
END_TEST


/*-----------------------------------------------------------------------
 * String hash tables
 */

START_TEST(test_string_hash_table)
{
    struct cork_hash_table  *table;
    char  key[256];
    void  *value;

    table = cork_string_hash_table_new(0, 0);

    fail_if_error(cork_hash_table_put
                  (table, "key1", (void *) (uintptr_t) 1, NULL, NULL, NULL));
    fail_unless(cork_hash_table_size(table) == 1,
                "Unexpected size after adding {key1->1}");

    strncpy(key, "key1", sizeof(key));
    fail_if((value = cork_hash_table_get(table, key)) == NULL,
            "No entry for key1");

    fail_unless(value == (void *) (uintptr_t) 1,
                "Unexpected value for key1");

    strncpy(key, "key2", sizeof(key));
    fail_unless((value = cork_hash_table_get(table, key)) == NULL,
                "Unexpected entry for key2");

    cork_hash_table_free(table);
}
END_TEST


/*-----------------------------------------------------------------------
 * Pointer hash tables
 */

START_TEST(test_pointer_hash_table)
{
    struct cork_hash_table  *table;
    int  key1;
    int  key2;
    void  *value;

    table = cork_pointer_hash_table_new(0, 0);

    fail_if_error(cork_hash_table_put
                  (table, &key1, (void *) (uintptr_t) 1, NULL, NULL, NULL));
    fail_unless(cork_hash_table_size(table) == 1,
                "Unexpected size after adding {key1->1}");

    fail_if((value = cork_hash_table_get(table, &key1)) == NULL,
            "No entry for key1");

    fail_unless(value == (void *) (uintptr_t) 1,
                "Unexpected value for key1");

    fail_unless((value = cork_hash_table_get(table, &key2)) == NULL,
                "Unexpected entry for key2");

    cork_hash_table_free(table);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("hash_table");

    TCase  *tc_ds = tcase_create("hash_table");
    tcase_add_test(tc_ds, test_uint64_hash_table);
    tcase_add_test(tc_ds, test_string_hash_table);
    tcase_add_test(tc_ds, test_pointer_hash_table);
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
