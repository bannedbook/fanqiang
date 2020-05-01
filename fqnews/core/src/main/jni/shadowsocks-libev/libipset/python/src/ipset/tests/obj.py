# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2010, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

import unittest

from ipset import *
from ipset.tests import *


IPV4_ADDR_1 = \
    "\xc0\xa8\x01\x64"

IPV6_ADDR_1 = \
    "\xfe\x80\x00\x00\x00\x00\x00\x00\x02\x1e\xc2\xff\xfe\x9f\xe8\xe1"


class TestSet(unittest.TestCase):
    def test_set_starts_empty(self):
        s = IPSet()
        self.assertFalse(s)

    def test_empty_sets_equal(self):
        s1 = IPSet()
        s2 = IPSet()
        self.assertEqual(s1, s2)

    def test_ipv4_insert(self):
        s = IPSet()
        s.add(IPV4_ADDR_1)
        self.assert_(s)

    def test_ipv4_insert_network(self):
        s = IPSet()
        s.add_network(IPV4_ADDR_1, 24)
        self.assert_(s)

    def test_ipv6_insert(self):
        s = IPSet()
        s.add(IPV6_ADDR_1)
        self.assert_(s)

    def test_ipv6_insert_network(self):
        s = IPSet()
        s.add_network(IPV6_ADDR_1, 32)
        self.assert_(s)

    def test_file_empty(self):
        s1 = IPSet.from_file(test_file("empty.set"))
        s2 = IPSet()
        self.assertEqual(s1, s2)

    def test_file_just1_v4(self):
        s1 = IPSet.from_file(test_file("just1-v4.set"))
        s2 = IPSet()
        s2.add(IPV4_ADDR_1)
        self.assertEqual(s1, s2)

    def test_file_just1_v6(self):
        s1 = IPSet.from_file(test_file("just1-v6.set"))
        s2 = IPSet()
        s2.add(IPV6_ADDR_1)
        self.assertEqual(s1, s2)


class TestMap(unittest.TestCase):
    def test_map_starts_empty(self):
        s = IPMap(0)
        self.assertFalse(s)

    def test_empty_maps_equal(self):
        s1 = IPMap(0)
        s2 = IPMap(0)
        self.assertEqual(s1, s2)

    def test_ipv4_insert(self):
        s = IPMap(0)
        s[IPV4_ADDR_1] = 1
        self.assert_(s)

    def test_ipv4_insert_network(self):
        s = IPMap(0)
        s.set_network(IPV4_ADDR_1, 24, 1)
        self.assert_(s)

    def test_ipv6_insert(self):
        s = IPMap(0)
        s[IPV6_ADDR_1] = 1
        self.assert_(s)

    def test_ipv6_insert_network(self):
        s = IPMap(0)
        s.set_network(IPV6_ADDR_1, 32, 1)
        self.assert_(s)

    def test_file_empty(self):
        s1 = IPMap.from_file(test_file("empty.map"))
        s2 = IPMap(0)
        self.assertEqual(s1, s2)

    def test_file_just1_v4(self):
        s1 = IPMap.from_file(test_file("just1-v4.map"))
        s2 = IPMap(0)
        s2[IPV4_ADDR_1] = 1
        self.assertEqual(s1, s2)

    def test_file_just1_v6(self):
        s1 = IPMap.from_file(test_file("just1-v6.map"))
        s2 = IPMap(0)
        s2[IPV6_ADDR_1] = 1
        self.assertEqual(s1, s2)
