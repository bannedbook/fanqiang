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

#include <check.h>
#include <libcork/core.h>

#include "ipset/ipset.h"


#define IPV4_BIT_SIZE  32
#define IPV6_BIT_SIZE  128


/*-----------------------------------------------------------------------
 * Iterators
 */

START_TEST(test_iterate_empty)
{
    struct ip_set  set;
    ipset_init(&set);
    struct ipset_iterator  *it = ipset_iterate(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");
    fail_unless(it->finished,
                "IP set should be empty");
    ipset_iterator_free(it);
    ipset_done(&set);
}
END_TEST


START_TEST(test_ipv4_iterate_01)
{
    struct ip_set  set;
    ipset_init(&set);

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "192.168.0.1");

    fail_if(ipset_ip_add(&set, &ip1),
            "Element should not be present");

    struct ipset_iterator  *it = ipset_iterate(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == IPV4_BIT_SIZE,
                "IP CIDR prefix 0 doesn't match");

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 1 element");

    ipset_iterator_free(it);

    ipset_done(&set);
}
END_TEST


START_TEST(test_ipv4_iterate_network_01)
{
    struct ip_set  set;
    ipset_init(&set);

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "192.168.0.0");

    fail_if(ipset_ip_add_network(&set, &ip1, 31),
            "Element should not be present");

    struct ipset_iterator  *it = ipset_iterate_networks(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == 31,
                "IP CIDR prefix 0 doesn't match");

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 1 elements");

    ipset_iterator_free(it);

    ipset_done(&set);
}
END_TEST


START_TEST(test_ipv4_iterate_network_02)
{
    struct ip_set  set;
    ipset_init(&set);

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "192.168.0.0");

    fail_if(ipset_ip_add_network(&set, &ip1, 16),
            "Element should not be present");

    struct ipset_iterator  *it = ipset_iterate_networks(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == 16,
                "IP CIDR prefix 0 doesn't match");

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 1 elements");

    ipset_iterator_free(it);

    ipset_done(&set);
}
END_TEST


START_TEST(test_ipv4_iterate_network_03)
{
    struct ip_set  set;
    ipset_init(&set);

    /*
     * If we add all of the IP addresses in a network individually, we
     * should still get the network as a whole from the iterator.
     */

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "192.168.0.0");

    struct cork_ip  ip2;
    cork_ip_init(&ip2, "192.168.0.1");

    fail_if(ipset_ip_add(&set, &ip1),
            "Element should not be present");

    fail_if(ipset_ip_add(&set, &ip2),
            "Element should not be present");

    struct ipset_iterator  *it = ipset_iterate_networks(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == 31,
                "IP CIDR prefix 0 doesn't match");

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 1 elements");

    ipset_iterator_free(it);

    ipset_done(&set);
}
END_TEST


START_TEST(test_ipv6_iterate_01)
{
    struct ip_set  set;
    ipset_init(&set);

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "fe80::1");

    fail_if(ipset_ip_add(&set, &ip1),
            "Element should not be present");

    struct ipset_iterator  *it = ipset_iterate(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == IPV6_BIT_SIZE,
                "IP CIDR prefix 0 doesn't match");

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 1 element");

    ipset_iterator_free(it);

    ipset_done(&set);
}
END_TEST


START_TEST(test_ipv6_iterate_network_01)
{
    struct ip_set  set;
    ipset_init(&set);

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "fe80::");

    fail_if(ipset_ip_add_network(&set, &ip1, 127),
            "Element should not be present");

    struct ipset_iterator  *it = ipset_iterate_networks(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == 127,
                "IP CIDR prefix 0 doesn't match (%u)", it->cidr_prefix);

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 1 element");

    ipset_iterator_free(it);

    ipset_done(&set);
}
END_TEST


START_TEST(test_ipv6_iterate_network_02)
{
    struct ip_set  set;
    ipset_init(&set);

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "fe80::");

    fail_if(ipset_ip_add_network(&set, &ip1, 16),
            "Element should not be present");

    struct ipset_iterator  *it = ipset_iterate_networks(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == 16,
                "IP CIDR prefix 0 doesn't match (%u)", it->cidr_prefix);

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 1 element");

    ipset_iterator_free(it);

    ipset_done(&set);
}
END_TEST


START_TEST(test_ipv6_iterate_network_03)
{
    struct ip_set  set;
    ipset_init(&set);

    /*
     * If we add all of the IP addresses in a network individually, we
     * should still get the network as a whole from the iterator.
     */

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "fe80::");

    struct cork_ip  ip2;
    cork_ip_init(&ip2, "fe80::1");

    fail_if(ipset_ip_add(&set, &ip1),
            "Element should not be present");

    fail_if(ipset_ip_add(&set, &ip2),
            "Element should not be present");

    struct ipset_iterator  *it = ipset_iterate_networks(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == 127,
                "IP CIDR prefix 0 doesn't match");

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 1 elements");

    ipset_iterator_free(it);

    ipset_done(&set);
}
END_TEST


START_TEST(test_generic_ip_iterate_01)
{
    struct ip_set  set;
    ipset_init(&set);

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "0.0.0.0");

    struct cork_ip  ip2;
    cork_ip_init(&ip2, "::");

    struct ipset_iterator  *it = ipset_iterate_networks(&set, false);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == 0,
                "IP CIDR prefix 0 doesn't match");

    ipset_iterator_advance(it);
    fail_if(it->finished,
            "IP set should have more than 1 element");
    fail_unless(cork_ip_equal(&ip2, &it->addr),
                "IP address 1 doesn't match");
    fail_unless(it->cidr_prefix == 0,
                "IP CIDR prefix 1 doesn't match");

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 2 elements");

    ipset_iterator_free(it);

    ipset_done(&set);
}
END_TEST


START_TEST(test_generic_ip_iterate_02)
{
    struct ip_set  set;
    ipset_init(&set);

    /*
     * These addresses are carefully constructed so that the same BDD
     * variable assignments are used to store both, apart from the
     * IPv4/v6 discriminator variable.  The goal is get a BDD that has
     * EITHER in the assignment for variable 0, but isn't simply the
     * empty or full set.
     */

    struct cork_ip  ip1;
    cork_ip_init(&ip1, "192.168.0.1"); /* 0xc0a80001 */

    struct cork_ip  ip2;
    cork_ip_init(&ip2, "c0a8:0001::");

    fail_if(ipset_ip_add(&set, &ip1),
            "Element should not be present");
    fail_if(ipset_ip_add_network(&set, &ip2, 32),
            "Element should not be present");

    struct ipset_iterator  *it = ipset_iterate_networks(&set, true);
    fail_if(it == NULL,
            "IP set iterator is NULL");

    fail_if(it->finished,
            "IP set shouldn't be empty");
    fail_unless(cork_ip_equal(&ip1, &it->addr),
                "IP address 0 doesn't match");
    fail_unless(it->cidr_prefix == 32,
                "IP CIDR prefix 0 doesn't match");

    ipset_iterator_advance(it);
    fail_if(it->finished,
            "IP set should have more than 1 element");
    fail_unless(cork_ip_equal(&ip2, &it->addr),
                "IP address 1 doesn't match");
    fail_unless(it->cidr_prefix == 32,
                "IP CIDR prefix 1 doesn't match");

    ipset_iterator_advance(it);
    fail_unless(it->finished,
                "IP set should contain 2 elements");

    ipset_iterator_free(it);

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

    TCase  *tc_iterator = tcase_create("iterator");
    tcase_add_test(tc_iterator, test_iterate_empty);
    tcase_add_test(tc_iterator, test_ipv4_iterate_01);
    tcase_add_test(tc_iterator, test_ipv4_iterate_network_01);
    tcase_add_test(tc_iterator, test_ipv4_iterate_network_02);
    tcase_add_test(tc_iterator, test_ipv4_iterate_network_03);
    tcase_add_test(tc_iterator, test_ipv6_iterate_01);
    tcase_add_test(tc_iterator, test_ipv6_iterate_network_01);
    tcase_add_test(tc_iterator, test_ipv6_iterate_network_02);
    tcase_add_test(tc_iterator, test_ipv6_iterate_network_03);
    tcase_add_test(tc_iterator, test_generic_ip_iterate_01);
    tcase_add_test(tc_iterator, test_generic_ip_iterate_02);
    suite_add_tcase(s, tc_iterator);

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
