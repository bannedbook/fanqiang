/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <unistd.h>

#include <check.h>
#include <libcork/core.h>

#include "ipset/ipset.h"


#define DESCRIBE_TEST  fprintf(stderr, "---\n%s\n", __func__)


/*-----------------------------------------------------------------------
 * Temporary file helper
 */

#define TEMP_FILE_TEMPLATE "/tmp/bdd-XXXXXX"

struct temp_file {
    char  *filename;
    FILE  *stream;
};

static struct temp_file *
temp_file_new(void)
{
    struct temp_file  *temp_file = cork_new(struct temp_file);
    temp_file->filename = (char *) cork_strdup(TEMP_FILE_TEMPLATE);
    temp_file->stream = NULL;
    return temp_file;
}

static void
temp_file_free(struct temp_file *temp_file)
{
    if (temp_file->stream != NULL) {
        fclose(temp_file->stream);
    }

    unlink(temp_file->filename);
    cork_strfree(temp_file->filename);
    free(temp_file);
}

static void
temp_file_open_stream(struct temp_file *temp_file)
{
    int  fd = mkstemp(temp_file->filename);
    temp_file->stream = fdopen(fd, "rb+");
}


/*-----------------------------------------------------------------------
 * Helper functions
 */

static void
test_round_trip(struct ip_map *map)
{
    struct ip_map  *read_map;

    struct temp_file  *temp_file = temp_file_new();
    temp_file_open_stream(temp_file);

    fail_unless(ipmap_save(temp_file->stream, map) == 0,
                "Could not save map");

    fflush(temp_file->stream);
    fseek(temp_file->stream, 0, SEEK_SET);

    read_map = ipmap_load(temp_file->stream);
    fail_if(read_map == NULL,
            "Could not read map");

    fail_unless(ipmap_is_equal(map, read_map),
                "Map not same after saving/loading");

    temp_file_free(temp_file);
    ipmap_free(read_map);
}


/*-----------------------------------------------------------------------
 * General tests
 */

START_TEST(test_map_starts_empty)
{
    DESCRIBE_TEST;
    struct ip_map  map;

    ipmap_init(&map, 0);
    fail_unless(ipmap_is_empty(&map),
                "Map should start empty");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_empty_maps_equal)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;

    ipmap_init(&map1, 0);
    ipmap_init(&map2, 0);
    fail_unless(ipmap_is_equal(&map1, &map2),
                "Empty maps should be equal");
    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_different_defaults_unequal)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;

    ipmap_init(&map1, 0);
    ipmap_init(&map2, 1);
    fail_if(ipmap_is_equal(&map1, &map2),
            "Empty maps with different defaults "
            "should be unequal");
    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_store_empty_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;

    ipmap_init(&map, 0);
    test_round_trip(&map);
    ipmap_done(&map);
}
END_TEST

START_TEST(test_store_empty_02)
{
    DESCRIBE_TEST;
    struct ip_map  map;

    ipmap_init(&map, 1);
    test_round_trip(&map);
    ipmap_done(&map);
}
END_TEST


/*-----------------------------------------------------------------------
 * IPv4 tests
 */

START_TEST(test_ipv4_insert_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map, &addr, 1);
    fail_unless(ipmap_ipv4_get(&map, &addr) == 1,
                "Element should be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_insert_02)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map, &addr, 1);
    cork_ipv4_init(&addr, "192.168.1.101");
    fail_unless(ipmap_ipv4_get(&map, &addr) == 0,
                "Element should not be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_insert_03)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map, &addr, 1);
    cork_ipv4_init(&addr, "192.168.2.100");
    fail_unless(ipmap_ipv4_get(&map, &addr) == 0,
                "Element should not be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_insert_network_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set_network(&map, &addr, 24, 1);
    fail_unless(ipmap_ipv4_get(&map, &addr) == 1,
                "Element should be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_insert_network_02)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set_network(&map, &addr, 24, 1);
    cork_ipv4_init(&addr, "192.168.1.101");
    fail_unless(ipmap_ipv4_get(&map, &addr) == 1,
                "Element should be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_insert_network_03)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set_network(&map, &addr, 24, 1);
    cork_ipv4_init(&addr, "192.168.2.100");
    fail_unless(ipmap_ipv4_get(&map, &addr) == 0,
                "Element should not be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_overwrite_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.0");
    ipmap_ipv4_set_network(&map, &addr, 24, 1);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map, &addr, 0);
    fail_unless(ipmap_ipv4_get(&map, &addr) == 0,
                "Element should be overwritten");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_overwrite_02)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.0");
    ipmap_ipv4_set_network(&map, &addr, 24, 1);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map, &addr, 2);
    fail_unless(ipmap_ipv4_get(&map, &addr) == 2,
                "Element should be overwritten");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_bad_cidr_prefix_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set_network(&map, &addr, 33, 1);
    fail_unless(ipmap_is_empty(&map),
                "Bad CIDR prefix shouldn't change map");
    cork_error_clear();
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_equality_1)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv4  addr;

    ipmap_init(&map1, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map1, &addr, 1);

    ipmap_init(&map2, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map2, &addr, 1);

    fail_unless(ipmap_is_equal(&map1, &map2),
                "Expected {x} == {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv4_equality_2)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv4  addr;

    ipmap_init(&map1, 0);
    cork_ipv4_init(&addr, "192.168.1.0");
    ipmap_ipv4_set(&map1, &addr, 1);
    ipmap_ipv4_set_network(&map1, &addr, 24, 1);

    ipmap_init(&map2, 0);
    cork_ipv4_init(&addr, "192.168.1.0");
    ipmap_ipv4_set_network(&map2, &addr, 24, 1);

    fail_unless(ipmap_is_equal(&map1, &map2),
                "Expected {x} == {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv4_equality_3)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv4  addr;

    ipmap_init(&map1, 0);
    cork_ipv4_init(&addr, "192.168.0.0");
    ipmap_ipv4_set(&map1, &addr, 1);
    ipmap_ipv4_set_network(&map1, &addr, 23, 1);
    ipmap_ipv4_set_network(&map1, &addr, 24, 2);

    ipmap_init(&map2, 0);
    cork_ipv4_init(&addr, "192.168.0.0");
    ipmap_ipv4_set_network(&map2, &addr, 24, 2);
    cork_ipv4_init(&addr, "192.168.1.0");
    ipmap_ipv4_set_network(&map2, &addr, 24, 1);

    fail_unless(ipmap_is_equal(&map1, &map2),
                "Expected {x} == {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv4_inequality_1)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv4  addr;

    ipmap_init(&map1, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map1, &addr, 1);

    ipmap_init(&map2, 0);
    ipmap_ipv4_set_network(&map2, &addr, 24, 1);

    fail_unless(!ipmap_is_equal(&map1, &map2),
                "Expected {x} != {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv4_inequality_2)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv4  addr;

    ipmap_init(&map1, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map1, &addr, 1);

    ipmap_init(&map2, 0);
    ipmap_ipv4_set(&map2, &addr, 2);

    fail_unless(!ipmap_is_equal(&map1, &map2),
                "Expected {x} != {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv4_memory_size_1)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;
    size_t  expected, actual;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map, &addr, 1);

    expected = 33 * sizeof(struct ipset_node);
    actual = ipmap_memory_size(&map);

    fail_unless(expected == actual,
                "Expected map to be %zu bytes, got %zu bytes",
                expected, actual);

    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_memory_size_2)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;
    size_t  expected, actual;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set_network(&map, &addr, 24, 1);

    expected = 25 * sizeof(struct ipset_node);
    actual = ipmap_memory_size(&map);

    fail_unless(expected == actual,
                "Expected map to be %zu bytes, got %zu bytes",
                expected, actual);

    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv4_store_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv4  addr;

    ipmap_init(&map, 0);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipmap_ipv4_set(&map, &addr, 1);
    cork_ipv4_init(&addr, "192.168.1.101");
    ipmap_ipv4_set(&map, &addr, 2);
    cork_ipv4_init(&addr, "192.168.2.100");
    ipmap_ipv4_set_network(&map, &addr, 24, 2);
    test_round_trip(&map);
    ipmap_done(&map);
}
END_TEST


/*-----------------------------------------------------------------------
 * IPv6 tests
 */

START_TEST(test_ipv6_insert_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set(&map, &addr, 1);
    fail_unless(ipmap_ipv6_get(&map, &addr) == 1,
                "Element should be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv6_insert_02)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set(&map, &addr, 1);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e2");
    fail_unless(ipmap_ipv6_get(&map, &addr) == 0,
                "Element should not be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv6_insert_03)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set(&map, &addr, 1);
    cork_ipv6_init(&addr, "fe80:1::21e:c2ff:fe9f:e8e1");
    fail_unless(ipmap_ipv6_get(&map, &addr) == 0,
                "Element should not be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv6_insert_network_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set_network(&map, &addr, 32, 1);
    fail_unless(ipmap_ipv6_get(&map, &addr) == 1,
                "Element should be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv6_insert_network_02)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set_network(&map, &addr, 32, 1);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e2");
    fail_unless(ipmap_ipv6_get(&map, &addr) == 1,
                "Element should be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv6_insert_network_03)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set_network(&map, &addr, 32, 1);
    cork_ipv6_init(&addr, "fe80:1::21e:c2ff:fe9f:e8e1");
    fail_unless(ipmap_ipv6_get(&map, &addr) == 0,
                "Element should not be present");
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv6_bad_cidr_prefix_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set_network(&map, &addr, 129, 1);
    fail_unless(ipmap_is_empty(&map),
                "Bad CIDR prefix shouldn't change map");
    cork_error_clear();
    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv6_equality_1)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv6  addr;

    ipmap_init(&map1, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set(&map1, &addr, 1);

    ipmap_init(&map2, 0);
    ipmap_ipv6_set(&map2, &addr, 1);

    fail_unless(ipmap_is_equal(&map1, &map2),
                "Expected {x} == {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv6_equality_2)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv6  addr;

    ipmap_init(&map1, 0);
    cork_ipv6_init(&addr, "fe80::");
    ipmap_ipv6_set(&map1, &addr, 1);
    ipmap_ipv6_set_network(&map1, &addr, 64, 1);

    ipmap_init(&map2, 0);
    cork_ipv6_init(&addr, "fe80::");
    ipmap_ipv6_set_network(&map2, &addr, 64, 1);

    fail_unless(ipmap_is_equal(&map1, &map2),
                "Expected {x} == {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv6_equality_3)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv6  addr;

    ipmap_init(&map1, 0);
    cork_ipv6_init(&addr, "fe80::");
    ipmap_ipv6_set(&map1, &addr, 1);
    ipmap_ipv6_set_network(&map1, &addr, 111, 1);
    ipmap_ipv6_set_network(&map1, &addr, 112, 2);

    ipmap_init(&map2, 0);
    cork_ipv6_init(&addr, "fe80::");
    ipmap_ipv6_set_network(&map2, &addr, 112, 2);
    cork_ipv6_init(&addr, "fe80::1:0");
    ipmap_ipv6_set_network(&map2, &addr, 112, 1);

    fail_unless(ipmap_is_equal(&map1, &map2),
                "Expected {x} == {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv6_inequality_1)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv6  addr;

    ipmap_init(&map1, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set(&map1, &addr, 1);

    ipmap_init(&map2, 0);
    ipmap_ipv6_set_network(&map2, &addr, 32, 1);

    fail_unless(!ipmap_is_equal(&map1, &map2),
                "Expected {x} != {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv6_inequality_2)
{
    DESCRIBE_TEST;
    struct ip_map  map1, map2;
    struct cork_ipv6  addr;

    ipmap_init(&map1, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set(&map1, &addr, 1);

    ipmap_init(&map2, 0);
    ipmap_ipv6_set(&map2, &addr, 2);

    fail_unless(!ipmap_is_equal(&map1, &map2),
                "Expected {x} != {x}");

    ipmap_done(&map1);
    ipmap_done(&map2);
}
END_TEST

START_TEST(test_ipv6_memory_size_1)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;
    size_t  expected, actual;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set(&map, &addr, 1);

    expected = 129 * sizeof(struct ipset_node);
    actual = ipmap_memory_size(&map);

    fail_unless(expected == actual,
                "Expected map to be %zu bytes, got %zu bytes",
                expected, actual);

    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv6_memory_size_2)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;
    size_t  expected, actual;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set_network(&map, &addr, 32, 1);

    expected = 33 * sizeof(struct ipset_node);
    actual = ipmap_memory_size(&map);

    fail_unless(expected == actual,
                "Expected map to be %zu bytes, got %zu bytes",
                expected, actual);

    ipmap_done(&map);
}
END_TEST

START_TEST(test_ipv6_store_01)
{
    DESCRIBE_TEST;
    struct ip_map  map;
    struct cork_ipv6  addr;

    ipmap_init(&map, 0);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set(&map, &addr, 1);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e2");
    ipmap_ipv6_set(&map, &addr, 2);
    cork_ipv6_init(&addr, "fe80:1::21e:c2ff:fe9f:e8e1");
    ipmap_ipv6_set_network(&map, &addr, 32, 2);
    test_round_trip(&map);
    ipmap_done(&map);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
ipmap_suite()
{
    Suite  *s = suite_create("ipmap");

    TCase  *tc_general = tcase_create("general");
    tcase_add_test(tc_general, test_map_starts_empty);
    tcase_add_test(tc_general, test_empty_maps_equal);
    tcase_add_test(tc_general, test_different_defaults_unequal);
    tcase_add_test(tc_general, test_store_empty_01);
    tcase_add_test(tc_general, test_store_empty_02);
    suite_add_tcase(s, tc_general);

    TCase  *tc_ipv4 = tcase_create("ipv4");
    tcase_add_test(tc_ipv4, test_ipv4_insert_01);
    tcase_add_test(tc_ipv4, test_ipv4_insert_02);
    tcase_add_test(tc_ipv4, test_ipv4_insert_03);
    tcase_add_test(tc_ipv4, test_ipv4_insert_network_01);
    tcase_add_test(tc_ipv4, test_ipv4_insert_network_02);
    tcase_add_test(tc_ipv4, test_ipv4_insert_network_03);
    tcase_add_test(tc_ipv4, test_ipv4_overwrite_01);
    tcase_add_test(tc_ipv4, test_ipv4_overwrite_02);
    tcase_add_test(tc_ipv4, test_ipv4_bad_cidr_prefix_01);
    tcase_add_test(tc_ipv4, test_ipv4_equality_1);
    tcase_add_test(tc_ipv4, test_ipv4_equality_2);
    tcase_add_test(tc_ipv4, test_ipv4_equality_3);
    tcase_add_test(tc_ipv4, test_ipv4_inequality_1);
    tcase_add_test(tc_ipv4, test_ipv4_inequality_2);
    tcase_add_test(tc_ipv4, test_ipv4_memory_size_1);
    tcase_add_test(tc_ipv4, test_ipv4_memory_size_2);
    tcase_add_test(tc_ipv4, test_ipv4_store_01);
    suite_add_tcase(s, tc_ipv4);

    TCase  *tc_ipv6 = tcase_create("ipv6");
    tcase_add_test(tc_ipv6, test_ipv6_insert_01);
    tcase_add_test(tc_ipv6, test_ipv6_insert_02);
    tcase_add_test(tc_ipv6, test_ipv6_insert_03);
    tcase_add_test(tc_ipv6, test_ipv6_insert_network_01);
    tcase_add_test(tc_ipv6, test_ipv6_insert_network_02);
    tcase_add_test(tc_ipv6, test_ipv6_insert_network_03);
    tcase_add_test(tc_ipv6, test_ipv6_bad_cidr_prefix_01);
    tcase_add_test(tc_ipv6, test_ipv6_equality_1);
    tcase_add_test(tc_ipv6, test_ipv6_equality_2);
    tcase_add_test(tc_ipv6, test_ipv6_equality_3);
    tcase_add_test(tc_ipv6, test_ipv6_inequality_1);
    tcase_add_test(tc_ipv6, test_ipv6_inequality_2);
    tcase_add_test(tc_ipv6, test_ipv6_memory_size_1);
    tcase_add_test(tc_ipv6, test_ipv6_memory_size_2);
    tcase_add_test(tc_ipv6, test_ipv6_store_01);
    suite_add_tcase(s, tc_ipv6);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = ipmap_suite();
    SRunner  *runner = srunner_create(suite);

    ipset_init_library();

    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
