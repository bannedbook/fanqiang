# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2010, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

import unittest

from ipset.c import *


IPV4_ADDR_1 = \
    "\xc0\xa8\x01\x64"

IPV6_ADDR_1 = \
    "\xfe\x80\x00\x00\x00\x00\x00\x00\x02\x1e\xc2\xff\xfe\x9f\xe8\xe1"


class TestSet(unittest.TestCase):
    def test_set_starts_empty(self):
        s = ipset.ipset_new()
        self.assert_(ipset.ipset_is_empty(s))
        ipset.ipset_free(s)

    def test_empty_sets_equal(self):
        s1 = ipset.ipset_new()
        s2 = ipset.ipset_new()
        self.assert_(ipset.ipset_is_equal(s1, s2))
        ipset.ipset_free(s1)
        ipset.ipset_free(s2)

    def test_ipv4_insert(self):
        s = ipset.ipset_new()
        ipset.ipset_ipv4_add(s, IPV4_ADDR_1)
        self.assertFalse(ipset.ipset_is_empty(s))
        ipset.ipset_free(s)

    def test_ipv4_insert_network(self):
        s = ipset.ipset_new()
        ipset.ipset_ipv4_add_network(s, IPV4_ADDR_1, 24)
        self.assertFalse(ipset.ipset_is_empty(s))
        ipset.ipset_free(s)

    def test_ipv6_insert(self):
        s = ipset.ipset_new()
        ipset.ipset_ipv6_add(s, IPV6_ADDR_1)
        self.assertFalse(ipset.ipset_is_empty(s))
        ipset.ipset_free(s)

    def test_ipv6_insert_network(self):
        s = ipset.ipset_new()
        ipset.ipset_ipv6_add_network(s, IPV6_ADDR_1, 32)
        self.assertFalse(ipset.ipset_is_empty(s))
        ipset.ipset_free(s)


class TestMap(unittest.TestCase):
    def test_map_starts_empty(self):
        s = ipset.ipmap_new(0)
        self.assert_(ipset.ipmap_is_empty(s))
        ipset.ipmap_free(s)

    def test_empty_maps_equal(self):
        s1 = ipset.ipmap_new(0)
        s2 = ipset.ipmap_new(0)
        self.assert_(ipset.ipmap_is_equal(s1, s2))
        ipset.ipmap_free(s1)
        ipset.ipmap_free(s2)

    def test_ipv4_insert(self):
        s = ipset.ipmap_new(0)
        ipset.ipmap_ipv4_set(s, IPV4_ADDR_1, 1)
        self.assertFalse(ipset.ipmap_is_empty(s))
        ipset.ipmap_free(s)

    def test_ipv4_insert_network(self):
        s = ipset.ipmap_new(0)
        ipset.ipmap_ipv4_set_network(s, IPV4_ADDR_1, 24, 1)
        self.assertFalse(ipset.ipmap_is_empty(s))
        ipset.ipmap_free(s)

    def test_ipv6_insert(self):
        s = ipset.ipmap_new(0)
        ipset.ipmap_ipv6_set(s, IPV6_ADDR_1, 1)
        self.assertFalse(ipset.ipmap_is_empty(s))
        ipset.ipmap_free(s)

    def test_ipv6_insert_network(self):
        s = ipset.ipmap_new(0)
        ipset.ipmap_ipv6_set_network(s, IPV6_ADDR_1, 32, 1)
        self.assertFalse(ipset.ipmap_is_empty(s))
        ipset.ipmap_free(s)
