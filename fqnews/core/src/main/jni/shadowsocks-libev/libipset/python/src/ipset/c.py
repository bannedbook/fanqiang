# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2010, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

"""
Contains a ctypes interface to the underlying ipset C library.

Python code probably wants to use the object-oriented interface in the
ipset package, instead.
"""

__all__ = (
    "ipset",
    "libc",
)

from ctypes import *
import ctypes.util

libc = CDLL(ctypes.util.find_library("c"))
ipset = CDLL(ctypes.util.find_library("ipset"))

# A restype function that converts a 1-byte integer into a Python
# boolean.

def c_bool(value):
    return ((value & 0xff) != 0)


ipset.ipset_init_library.restype = c_int

# Call ipset_init_library when we load this Python module.

ipset.ipset_init_library()


ipset.ipset_init.argtypes = [c_void_p]
ipset.ipset_done.argtypes = [c_void_p]
ipset.ipset_new.restype = c_void_p
ipset.ipset_free.argtypes = [c_void_p]

ipset.ipset_is_empty.argtypes = [c_void_p]
ipset.ipset_is_empty.restype = c_bool
ipset.ipset_is_equal.argtypes = [c_void_p, c_void_p]
ipset.ipset_is_equal.restype = c_bool
ipset.ipset_is_not_equal.argtypes = [c_void_p, c_void_p]
ipset.ipset_is_not_equal.restype = c_bool
ipset.ipset_memory_size.argtypes = [c_void_p]
ipset.ipset_memory_size.restype = c_long

ipset.ipset_save.argtypes = [c_void_p, c_void_p]
ipset.ipset_save.restype = c_bool
ipset.ipset_load.argtypes = [c_void_p]
ipset.ipset_load.restype = c_void_p

ipset.ipset_ipv4_add.argtypes = [c_void_p, c_void_p]
ipset.ipset_ipv4_add.restype = c_bool
ipset.ipset_ipv4_add_network.argtypes = [c_void_p, c_void_p, c_int]
ipset.ipset_ipv4_add_network.restype = c_bool

ipset.ipset_ipv6_add.argtypes = [c_void_p, c_void_p]
ipset.ipset_ipv6_add.restype = c_bool
ipset.ipset_ipv6_add_network.argtypes = [c_void_p, c_void_p, c_int]
ipset.ipset_ipv6_add_network.restype = c_bool


ipset.ipmap_init.argtypes = [c_void_p, c_long]
ipset.ipmap_init_ptr.argtypes = [c_void_p, c_void_p]
ipset.ipmap_done.argtypes = [c_void_p]
ipset.ipmap_new.argtypes = [c_long]
ipset.ipmap_new.restype = c_void_p
ipset.ipmap_new.argtypes = [c_void_p]
ipset.ipmap_new_ptr.restype = c_void_p
ipset.ipmap_free.argtypes = [c_void_p]

ipset.ipmap_is_empty.argtypes = [c_void_p]
ipset.ipmap_is_empty.restype = c_bool
ipset.ipmap_is_equal.argtypes = [c_void_p, c_void_p]
ipset.ipmap_is_equal.restype = c_bool
ipset.ipmap_is_not_equal.argtypes = [c_void_p, c_void_p]
ipset.ipmap_is_not_equal.restype = c_bool
ipset.ipmap_memory_size.argtypes = [c_void_p]
ipset.ipmap_memory_size.restype = c_long

ipset.ipmap_save.argtypes = [c_void_p, c_void_p]
ipset.ipmap_save.restype = c_bool
ipset.ipmap_load.argtypes = [c_void_p]
ipset.ipmap_load.restype = c_void_p

ipset.ipmap_ipv4_set.argtypes = [c_void_p, c_void_p, c_long]
ipset.ipmap_ipv4_set_network.argtypes = [c_void_p, c_void_p, c_int, c_long]
ipset.ipmap_ipv4_set_ptr.argtypes = [c_void_p, c_void_p, c_void_p]
ipset.ipmap_ipv4_set_network_ptr.argtypes = [c_void_p, c_void_p, c_int, c_void_p]
ipset.ipmap_ipv4_get.argtypes = [c_void_p, c_void_p]
ipset.ipmap_ipv4_get.restype = c_long
ipset.ipmap_ipv4_get_ptr.argtypes = [c_void_p, c_void_p]
ipset.ipmap_ipv4_get_ptr.restype = c_void_p

ipset.ipmap_ipv6_set.argtypes = [c_void_p, c_void_p, c_long]
ipset.ipmap_ipv6_set_network.argtypes = [c_void_p, c_void_p, c_int, c_long]
ipset.ipmap_ipv6_set_ptr.argtypes = [c_void_p, c_void_p, c_void_p]
ipset.ipmap_ipv6_set_network_ptr.argtypes = [c_void_p, c_void_p, c_int, c_void_p]
ipset.ipmap_ipv6_get.argtypes = [c_void_p, c_void_p]
ipset.ipmap_ipv6_get.restype = c_long
ipset.ipmap_ipv6_get_ptr.argtypes = [c_void_p, c_void_p]
ipset.ipmap_ipv6_get_ptr.restype = c_void_p
