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
test_round_trip(struct ip_set *set)
{
    struct ip_set  *read_set;

    struct temp_file  *temp_file = temp_file_new();
    temp_file_open_stream(temp_file);

    fail_unless(ipset_save(temp_file->stream, set) == 0,
                "Could not save set");

    fflush(temp_file->stream);
    fseek(temp_file->stream, 0, SEEK_SET);

    read_set = ipset_load(temp_file->stream);
    fail_if(read_set == NULL,
            "Could not read set");

    fail_unless(ipset_is_equal(set, read_set),
                "Set not same after saving/loading");

    temp_file_free(temp_file);
    ipset_free(read_set);
}


/*-----------------------------------------------------------------------
 * General tests
 */

START_TEST(test_set_starts_empty)
{
    DESCRIBE_TEST;
    struct ip_set  set;

    ipset_init(&set);
    fail_unless(ipset_is_empty(&set),
                "Set should start empty");
    ipset_done(&set);
}
END_TEST

START_TEST(test_empty_sets_equal)
{
    DESCRIBE_TEST;
    struct ip_set  set1, set2;

    ipset_init(&set1);
    ipset_init(&set2);
    fail_unless(ipset_is_equal(&set1, &set2),
                "Empty sets should be equal");
    ipset_done(&set1);
    ipset_done(&set2);
}
END_TEST

START_TEST(test_store_empty)
{
    DESCRIBE_TEST;
    struct ip_set  set;

    ipset_init(&set);
    test_round_trip(&set);
    ipset_done(&set);
}
END_TEST


/*-----------------------------------------------------------------------
 * IPv4 tests
 */

START_TEST(test_ipv4_insert_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    fail_if(ipset_ipv4_add(&set, &addr),
            "Element should not be present");
    fail_unless(ipset_ipv4_add(&set, &addr),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_remove_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    fail_if(ipset_ipv4_add(&set, &addr),
            "Element should not be present");
    fail_if(ipset_ipv4_remove(&set, &addr),
            "Element should be present");
    fail_unless(ipset_ipv4_remove(&set, &addr),
                "Element should not be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_insert_network_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    fail_if(ipset_ipv4_add_network(&set, &addr, 24),
            "Element should not be present");
    fail_unless(ipset_ipv4_add_network(&set, &addr, 24),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_remove_network_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    fail_if(ipset_ipv4_add_network(&set, &addr, 24),
            "Element should not be present");
    fail_if(ipset_ipv4_remove_network(&set, &addr, 24),
            "Element should be present");
    fail_unless(ipset_ipv4_remove_network(&set, &addr, 24),
                "Element should not be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_bad_cidr_prefix_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipset_ipv4_add_network(&set, &addr, 33);
    fail_unless(ipset_is_empty(&set),
                "Bad CIDR prefix shouldn't change set");
    cork_error_clear();
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_contains_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    fail_if(ipset_ipv4_add(&set, &addr),
            "Element should not be present");
    fail_unless(ipset_contains_ipv4(&set, &addr),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_contains_02)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;
    struct cork_ip  ip;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    cork_ip_from_ipv4(&ip, &addr);
    fail_if(ipset_ipv4_add(&set, &addr),
            "Element should not be present");
    fail_unless(ipset_contains_ip(&set, &ip),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_network_contains_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    fail_if(ipset_ipv4_add_network(&set, &addr, 24),
            "Element should not be present");
    fail_unless(ipset_contains_ipv4(&set, &addr),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_equality_1)
{
    DESCRIBE_TEST;
    struct ip_set  set1, set2;
    struct cork_ipv4  addr;

    ipset_init(&set1);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipset_ipv4_add(&set1, &addr);

    ipset_init(&set2);
    ipset_ipv4_add(&set2, &addr);

    fail_unless(ipset_is_equal(&set1, &set2),
                "Expected {x} == {x}");

    ipset_done(&set1);
    ipset_done(&set2);
}
END_TEST

START_TEST(test_ipv4_equality_2)
{
    DESCRIBE_TEST;
    struct ip_set  set1, set2;
    struct cork_ipv4  addr;

    ipset_init(&set1);
    cork_ipv4_init(&addr, "192.168.1.0");
    ipset_ipv4_add(&set1, &addr);
    ipset_ipv4_add_network(&set1, &addr, 24);

    ipset_init(&set2);
    ipset_ipv4_add_network(&set2, &addr, 24);

    fail_unless(ipset_is_equal(&set1, &set2),
                "Expected {x} == {x}");

    ipset_done(&set1);
    ipset_done(&set2);
}
END_TEST

START_TEST(test_ipv4_equality_3)
{
    DESCRIBE_TEST;
    struct ip_set  set1, set2;
    struct cork_ipv4  addr;

    ipset_init(&set1);
    cork_ipv4_init(&addr, "192.168.1.0");
    ipset_ipv4_add(&set1, &addr);
    ipset_ipv4_add_network(&set1, &addr, 24);
    cork_ipv4_init(&addr, "192.168.2.0");
    ipset_ipv4_add(&set1, &addr);

    ipset_init(&set2);
    cork_ipv4_init(&addr, "192.168.1.0");
    ipset_ipv4_add_network(&set2, &addr, 24);
    cork_ipv4_init(&addr, "192.168.2.0");
    ipset_ipv4_add(&set2, &addr);

    fail_unless(ipset_is_equal(&set1, &set2),
                "Expected {x} == {x}");

    ipset_done(&set1);
    ipset_done(&set2);
}
END_TEST

START_TEST(test_ipv4_inequality_1)
{
    DESCRIBE_TEST;
    struct ip_set  set1, set2;
    struct cork_ipv4  addr;

    ipset_init(&set1);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipset_ipv4_add(&set1, &addr);

    ipset_init(&set2);
    ipset_ipv4_add_network(&set2, &addr, 24);

    fail_unless(!ipset_is_equal(&set1, &set2),
                "Expected {x} != {x}");

    ipset_done(&set1);
    ipset_done(&set2);
}
END_TEST

START_TEST(test_ipv4_memory_size_1)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;
    size_t  expected, actual;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipset_ipv4_add(&set, &addr);

    expected = 33 * sizeof(struct ipset_node);
    actual = ipset_memory_size(&set);

    fail_unless(expected == actual,
                "Expected set to be %zu bytes, got %zu bytes",
                expected, actual);

    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_memory_size_2)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;
    size_t  expected, actual;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipset_ipv4_add_network(&set, &addr, 24);

    expected = 25 * sizeof(struct ipset_node);
    actual = ipset_memory_size(&set);

    fail_unless(expected == actual,
                "Expected set to be %zu bytes, got %zu bytes",
                expected, actual);

    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_store_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipset_ipv4_add(&set, &addr);
    test_round_trip(&set);
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_store_02)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipset_ipv4_add_network(&set, &addr, 24);
    test_round_trip(&set);
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv4_store_03)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv4  addr;

    ipset_init(&set);
    cork_ipv4_init(&addr, "192.168.1.100");
    ipset_ipv4_add(&set, &addr);
    cork_ipv4_init(&addr, "192.168.1.101");
    ipset_ipv4_add(&set, &addr);
    cork_ipv4_init(&addr, "192.168.2.100");
    ipset_ipv4_add_network(&set, &addr, 24);
    test_round_trip(&set);
    ipset_done(&set);
}
END_TEST


/*-----------------------------------------------------------------------
 * IPv6 tests
 */

START_TEST(test_ipv6_insert_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    fail_if(ipset_ipv6_add(&set, &addr),
            "Element should not be present");
    fail_unless(ipset_ipv6_add(&set, &addr),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_remove_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    fail_if(ipset_ipv6_add(&set, &addr),
            "Element should not be present");
    fail_if(ipset_ipv6_remove(&set, &addr),
            "Element should be present");
    fail_unless(ipset_ipv6_remove(&set, &addr),
                "Element should not be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_insert_network_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    fail_if(ipset_ipv6_add_network(&set, &addr, 32),
            "Element should not be present");
    fail_unless(ipset_ipv6_add_network(&set, &addr, 32),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_remove_network_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    fail_if(ipset_ipv6_add_network(&set, &addr, 32),
            "ELement should not be present");
    fail_if(ipset_ipv6_remove_network(&set, &addr, 32),
            "Element should be present");
    fail_unless(ipset_ipv6_remove_network(&set, &addr, 32),
                "Element should not be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_bad_cidr_prefix_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipset_ipv6_add_network(&set, &addr, 129);
    fail_unless(ipset_is_empty(&set),
                "Bad CIDR prefix shouldn't change set");
    cork_error_clear();
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_contains_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    fail_if(ipset_ipv6_add(&set, &addr),
            "Element should not be present");
    fail_unless(ipset_contains_ipv6(&set, &addr),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_contains_02)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;
    struct cork_ip  ip;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    cork_ip_from_ipv6(&ip, &addr);
    fail_if(ipset_ipv6_add(&set, &addr),
            "Element should not be present");
    fail_unless(ipset_contains_ip(&set, &ip),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_network_contains_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    fail_if(ipset_ipv6_add_network(&set, &addr, 32),
            "Element should not be present");
    fail_unless(ipset_contains_ipv6(&set, &addr),
                "Element should be present");
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_equality_1)
{
    DESCRIBE_TEST;
    struct ip_set  set1, set2;
    struct cork_ipv6  addr;

    ipset_init(&set1);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipset_ipv6_add(&set1, &addr);

    ipset_init(&set2);
    ipset_ipv6_add(&set2, &addr);

    fail_unless(ipset_is_equal(&set1, &set2),
                "Expected {x} == {x}");

    ipset_done(&set1);
    ipset_done(&set2);
}
END_TEST

START_TEST(test_ipv6_equality_2)
{
    DESCRIBE_TEST;
    struct ip_set  set1, set2;
    struct cork_ipv6  addr;

    ipset_init(&set1);
    cork_ipv6_init(&addr, "fe80::");
    ipset_ipv6_add(&set1, &addr);
    ipset_ipv6_add_network(&set1, &addr, 64);

    ipset_init(&set2);
    ipset_ipv6_add_network(&set2, &addr, 64);

    fail_unless(ipset_is_equal(&set1, &set2),
                "Expected {x} == {x}");

    ipset_done(&set1);
    ipset_done(&set2);
}
END_TEST

START_TEST(test_ipv6_equality_3)
{
    DESCRIBE_TEST;
    struct ip_set  set1, set2;
    struct cork_ipv6  addr;

    ipset_init(&set1);
    cork_ipv6_init(&addr, "fe80::");
    ipset_ipv6_add(&set1, &addr);
    ipset_ipv6_add_network(&set1, &addr, 64);
    cork_ipv6_init(&addr, "fe80:1::");
    ipset_ipv6_add(&set1, &addr);

    ipset_init(&set2);
    cork_ipv6_init(&addr, "fe80::");
    ipset_ipv6_add_network(&set2, &addr, 64);
    cork_ipv6_init(&addr, "fe80:1::");
    ipset_ipv6_add(&set2, &addr);

    fail_unless(ipset_is_equal(&set1, &set2),
                "Expected {x} == {x}");

    ipset_done(&set1);
    ipset_done(&set2);
}
END_TEST

START_TEST(test_ipv6_inequality_1)
{
    DESCRIBE_TEST;
    struct ip_set  set1, set2;
    struct cork_ipv6  addr;

    ipset_init(&set1);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipset_ipv6_add(&set1, &addr);

    ipset_init(&set2);
    ipset_ipv6_add_network(&set2, &addr, 32);

    fail_unless(!ipset_is_equal(&set1, &set2),
                "Expected {x} != {x}");

    ipset_done(&set1);
    ipset_done(&set2);
}
END_TEST

START_TEST(test_ipv6_memory_size_1)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;
    size_t  expected, actual;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipset_ipv6_add(&set, &addr);

    expected = 129 * sizeof(struct ipset_node);
    actual = ipset_memory_size(&set);

    fail_unless(expected == actual,
                "Expected set to be %zu bytes, got %zu bytes",
                expected, actual);

    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_memory_size_2)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;
    size_t  expected, actual;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipset_ipv6_add_network(&set, &addr, 24);

    expected = 25 * sizeof(struct ipset_node);
    actual = ipset_memory_size(&set);

    fail_unless(expected == actual,
                "Expected set to be %zu bytes, got %zu bytes",
                expected, actual);

    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_store_01)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipset_ipv6_add(&set, &addr);
    test_round_trip(&set);
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_store_02)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipset_ipv6_add_network(&set, &addr, 24);
    test_round_trip(&set);
    ipset_done(&set);
}
END_TEST

START_TEST(test_ipv6_store_03)
{
    DESCRIBE_TEST;
    struct ip_set  set;
    struct cork_ipv6  addr;

    ipset_init(&set);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e1");
    ipset_ipv6_add(&set, &addr);
    cork_ipv6_init(&addr, "fe80::21e:c2ff:fe9f:e8e2");
    ipset_ipv6_add(&set, &addr);
    cork_ipv6_init(&addr, "fe80:1::21e:c2ff:fe9f:e8e1");
    ipset_ipv6_add_network(&set, &addr, 24);
    test_round_trip(&set);
    ipset_done(&set);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
ipset_suite()
{
    Suite  *s = suite_create("ipset");
    TCase  *tc_general = tcase_create("general");
    tcase_add_test(tc_general, test_set_starts_empty);
    tcase_add_test(tc_general, test_empty_sets_equal);
    tcase_add_test(tc_general, test_store_empty);
    suite_add_tcase(s, tc_general);

    TCase  *tc_ipv4 = tcase_create("ipv4");
    tcase_add_test(tc_ipv4, test_ipv4_insert_01);
    tcase_add_test(tc_ipv4, test_ipv4_remove_01);
    tcase_add_test(tc_ipv4, test_ipv4_insert_network_01);
    tcase_add_test(tc_ipv4, test_ipv4_remove_network_01);
    tcase_add_test(tc_ipv4, test_ipv4_bad_cidr_prefix_01);
    tcase_add_test(tc_ipv4, test_ipv4_contains_01);
    tcase_add_test(tc_ipv4, test_ipv4_contains_02);
    tcase_add_test(tc_ipv4, test_ipv4_network_contains_01);
    tcase_add_test(tc_ipv4, test_ipv4_equality_1);
    tcase_add_test(tc_ipv4, test_ipv4_equality_2);
    tcase_add_test(tc_ipv4, test_ipv4_equality_3);
    tcase_add_test(tc_ipv4, test_ipv4_inequality_1);
    tcase_add_test(tc_ipv4, test_ipv4_memory_size_1);
    tcase_add_test(tc_ipv4, test_ipv4_memory_size_2);
    tcase_add_test(tc_ipv4, test_ipv4_store_01);
    tcase_add_test(tc_ipv4, test_ipv4_store_02);
    tcase_add_test(tc_ipv4, test_ipv4_store_03);
    suite_add_tcase(s, tc_ipv4);

    TCase  *tc_ipv6 = tcase_create("ipv6");
    tcase_add_test(tc_ipv6, test_ipv6_insert_01);
    tcase_add_test(tc_ipv6, test_ipv6_remove_01);
    tcase_add_test(tc_ipv6, test_ipv6_insert_network_01);
    tcase_add_test(tc_ipv6, test_ipv6_remove_network_01);
    tcase_add_test(tc_ipv6, test_ipv6_bad_cidr_prefix_01);
    tcase_add_test(tc_ipv6, test_ipv6_contains_01);
    tcase_add_test(tc_ipv6, test_ipv6_contains_02);
    tcase_add_test(tc_ipv6, test_ipv6_network_contains_01);
    tcase_add_test(tc_ipv6, test_ipv6_equality_1);
    tcase_add_test(tc_ipv6, test_ipv6_equality_2);
    tcase_add_test(tc_ipv6, test_ipv6_equality_3);
    tcase_add_test(tc_ipv6, test_ipv6_inequality_1);
    tcase_add_test(tc_ipv6, test_ipv6_memory_size_1);
    tcase_add_test(tc_ipv6, test_ipv6_memory_size_2);
    tcase_add_test(tc_ipv6, test_ipv6_store_01);
    tcase_add_test(tc_ipv6, test_ipv6_store_02);
    tcase_add_test(tc_ipv6, test_ipv6_store_03);
    suite_add_tcase(s, tc_ipv6);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = ipset_suite();
    SRunner  *runner = srunner_create(suite);

    ipset_init_library();

    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
