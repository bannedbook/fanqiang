# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2010, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

"""
Defines an object-oriented interface for the IP set and map functions
in the ipset library.
"""

__all__ = (
    "IPMap",
    "IPSet",
)


from ipset.c import *


class IPSet(object):
    def __init__(self, set_ptr=None):
        """
        Create a new, empty IP set.
        """

        if set_ptr is None:
            self.set = ipset.ipset_new()
        else:
            self.set = set_ptr


    def __del__(self):
        """
        Free the memory uses by an IP set.
        """

        ipset.ipset_free(self.set)


    def __eq__(self, other):
        """
        Return whether two IP sets contain exactly the same addresses.
        """

        if not isinstance(other, IPSet):
            return False

        return ipset.ipset_is_equal(self.set, other.set)


    def __nonzero__(self):
        """
        Return whether an IP set contains any elements.
        """

        return not ipset.ipset_is_empty(self.set)


    def is_empty(self):
        """
        Return whether an IP set is empty.
        """

        return ipset.ipset_is_empty(self.set)


    def memory_size(self):
        """
        Return the amount of memory that this IP set consumes.
        """

        return ipset.ipset_memory_size(self.set)


    def add(self, addr):
        """
        Add the given IP address to the set.  The address must be
        given as a str or buffer that is exactly 4 or 16 bytes long.
        If it's 4 bytes, it's interpreted as an IPv4 address; if it's
        16 bytes, it's interpreted as an IPv6 address.

        We return a bool indicating whether the address was already in
        the set.
        """

        if len(addr) == 4:
            return ipset.ipset_ipv4_add(self.set, addr)

        elif len(addr) == 16:
            return ipset.ipset_ipv6_add(self.set, addr)

        else:
            raise ValueError("Invalid address")


    def add_network(self, addr, netmask):
        """
        Add the given IP CIDR network block to the set.  The address
        must be given as a str or buffer that is exactly 4 or 16 bytes
        long.  If it's 4 bytes, it's interpreted as an IPv4 address;
        if it's 16 bytes, it's interpreted as an IPv6 address.  The
        netmask should be an integer between 1 and 32 (inclusive) for
        IPv4, or between 1 and 128 (inclusive) for IPv6.

        We return a bool indicating whether the address was already in
        the set.
        """

        if len(addr) == 4:
            return ipset.ipset_ipv4_add_network(self.set, addr, netmask)

        elif len(addr) == 16:
            return ipset.ipset_ipv6_add_network(self.set, addr, netmask)

        else:
            raise ValueError("Invalid address")


    @classmethod
    def from_file(cls, filename):
        """
        Load an IP set from a file.
        """

        f = libc.fopen(filename, "r")
        if f == 0:
            raise IOError("No such file")

        try:
            set_ptr = ipset.ipset_load(f)
            if set_ptr == 0:
                raise IOError("Could not read IP set")

            return cls(set_ptr)

        finally:
            libc.fclose(f)


    def save(self, filename):
        """
        Save an IP set to a file.
        """

        f = libc.fopen(filename, "w")
        if f == 0:
            raise IOError("Cannot open file for writing")

        try:
            if not ipset.ipset_save(self.set, f):
                raise IOError("Could not write IP set")

        finally:
            libc.fclose(f)



class IPMap(object):
    def __init__(self, default_value, map_ptr=None):
        """
        Create a new, empty IP map.
        """

        if map_ptr is None:
            self.map = ipset.ipmap_new(default_value)
        else:
            self.map = map_ptr


    def __del__(self):
        """
        Free the memory uses by an IP map.
        """

        ipset.ipmap_free(self.map)


    def __eq__(self, other):
        """
        Return whether two IP maps contain exactly the same addresses,
        mapped to the same values.
        """

        if not isinstance(other, IPMap):
            return False

        return ipset.ipmap_is_equal(self.map, other.map)


    def __nonzero__(self):
        """
        Return whether an IP map contains any elements.
        """

        return not ipset.ipmap_is_empty(self.map)


    def is_empty(self):
        """
        Return whether an IP map is empty.
        """

        return ipset.ipmap_is_empty(self.map)


    def memory_size(self):
        """
        Return the amount of memory that this IP map consumes.
        """

        return ipset.ipmap_memory_size(self.map)


    def get(self, addr):
        """
        Return the value that an IP address is mapped to in this map.
        If the address (or a network it belongs to) hasn't been
        explicitly added to the map, the default value is returned.
        The address must be given as a str or buffer that is exactly 4
        or 16 bytes long.  If it's 4 bytes, it's interpreted as an
        IPv4 address; if it's 16 bytes, it's interpreted as an IPv6
        address.
        """

        if len(addr) == 4:
            return ipset.ipmap_ipv4_get(self.map, addr)

        elif len(addr) == 16:
            return ipset.ipmap_ipv6_get(self.map, addr)

        else:
            raise ValueError("Invalid address")

    __getitem__ = get


    def set(self, addr, value):
        """
        Add the given IP address to the map.  The address must be
        given as a str or buffer that is exactly 4 or 16 bytes long.
        If it's 4 bytes, it's interpreted as an IPv4 address; if it's
        16 bytes, it's interpreted as an IPv6 address.
        """

        if len(addr) == 4:
            ipset.ipmap_ipv4_set(self.map, addr, value)
            return

        elif len(addr) == 16:
            ipset.ipmap_ipv6_set(self.map, addr, value)
            return

        else:
            raise ValueError("Invalid address")

    __setitem__ = set


    def set_network(self, addr, netmask, value):
        """
        Add the given IP address to the map.  The address must be
        given as a str or buffer that is exactly 4 or 16 bytes long.
        If it's 4 bytes, it's interpreted as an IPv4 address; if it's
        16 bytes, it's interpreted as an IPv6 address.  The netmask
        should be an integer between 1 and 32 (inclusive) for IPv4, or
        between 1 and 128 (inclusive) for IPv6.
        """

        if len(addr) == 4:
            ipset.ipmap_ipv4_set_network(self.map, addr, netmask, value)
            return

        elif len(addr) == 16:
            ipset.ipmap_ipv6_set_network(self.map, addr, netmask, value)
            return

        else:
            raise ValueError("Invalid address")


    @classmethod
    def from_file(cls, filename):
        """
        Load an IP map from a file.
        """

        f = libc.fopen(filename, "r")
        if f == 0:
            raise IOError("No such file")

        try:
            map_ptr = ipset.ipmap_load(f)
            if map_ptr == 0:
                raise IOError("Could not read IP map")

            return cls(None, map_ptr)

        finally:
            libc.fclose(f)


    def save(self, filename):
        """
        Save an IP map to a file.
        """

        f = libc.fopen(filename, "w")
        if f == 0:
            raise IOError("Cannot open file for writing")

        try:
            if not ipset.ipmap_save(self.map, f):
                raise IOError("Could not write IP map")

        finally:
            libc.fclose(f)
