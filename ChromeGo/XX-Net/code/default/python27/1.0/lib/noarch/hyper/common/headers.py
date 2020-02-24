# -*- coding: utf-8 -*-
"""
hyper/common/headers
~~~~~~~~~~~~~~~~~~~~~

Contains hyper's structures for storing and working with HTTP headers.
"""
import collections

from hyper.common.util import to_bytestring, to_bytestring_tuple


class HTTPHeaderMap(collections.MutableMapping):
    """
    A structure that contains HTTP headers.

    HTTP headers are a curious beast. At the surface level they look roughly
    like a name-value set, but in practice they have many variations that
    make them tricky:

    - duplicate keys are allowed
    - keys are compared case-insensitively
    - duplicate keys are isomorphic to comma-separated values, *except when
      they aren't*!
    - they logically contain a form of ordering

    This data structure is an attempt to preserve all of that information
    while being as user-friendly as possible. It retains all of the mapping
    convenience methods (allowing by-name indexing), while avoiding using a
    dictionary for storage.

    When iterated over, this structure returns headers in 'canonical form'.
    This form is a tuple, where the first entry is the header name (in
    lower-case), and the second entry is a list of header values (in original
    case).

    The mapping always emits both names and values in the form of bytestrings:
    never unicode strings. It can accept names and values in unicode form, and
    will automatically be encoded to bytestrings using UTF-8. The reason for
    what appears to be a user-unfriendly decision here is primarily to allow
    the broadest-possible compatibility (to make it possible to send headers in
    unusual encodings) while ensuring that users are never confused about what
    type of data they will receive.

    .. warning:: Note that this data structure makes none of the performance
                 guarantees of a dictionary. Lookup and deletion is not an O(1)
                 operation. Inserting a new value *is* O(1), all other
                 operations are O(n), including *replacing* a header entirely.
    """
    def __init__(self, *args, **kwargs):
        # The meat of the structure. In practice, headers are an ordered list
        # of tuples. This early version of the data structure simply uses this
        # directly under the covers.
        #
        # An important curiosity here is that the headers are not stored in
        # 'canonical form', but are instead stored in the form they were
        # provided in. This is to ensure that it is always possible to
        # reproduce the original header structure if necessary. This leads to
        # some unfortunate performance costs on structure access where it is
        # often necessary to transform the data into canonical form on access.
        # This cost is judged acceptable in low-level code like `hyper`, but
        # higher-level abstractions should consider if they really require this
        # logic.
        self._items = []

        for arg in args:
            self._items.extend(map(lambda x: to_bytestring_tuple(*x), arg))

        for k, v in kwargs.items():
            self._items.append(to_bytestring_tuple(k, v))

    def __getitem__(self, key):
        """
        Unlike the dict __getitem__, this returns a list of items in the order
        they were added. These items are returned in 'canonical form', meaning
        that comma-separated values are split into multiple values.
        """
        key = to_bytestring(key)
        values = []

        for k, v in self._items:
            if _keys_equal(k, key):
                values.extend(x[1] for x in canonical_form(k, v))

        if not values:
            raise KeyError("Nonexistent header key: {}".format(key))

        return values

    def __setitem__(self, key, value):
        """
        Unlike the dict __setitem__, this appends to the list of items.
        """
        self._items.append(to_bytestring_tuple(key, value))

    def __delitem__(self, key):
        """
        Sadly, __delitem__ is kind of stupid here, but the best we can do is
        delete all headers with a given key. To correctly achieve the 'KeyError
        on missing key' logic from dictionaries, we need to do this slowly.
        """
        key = to_bytestring(key)
        indices = []
        for (i, (k, v)) in enumerate(self._items):
            if _keys_equal(k, key):
                indices.append(i)

        if not indices:
            raise KeyError("Nonexistent header key: {}".format(key))

        for i in indices[::-1]:
            self._items.pop(i)

    def __iter__(self):
        """
        This mapping iterates like the list of tuples it is. The headers are
        returned in canonical form.
        """
        for pair in self._items:
            for value in canonical_form(*pair):
                yield value

    def __len__(self):
        """
        The length of this mapping is the number of individual headers in
        canonical form. Sadly, this is a somewhat expensive operation.
        """
        size = 0
        for _ in self:
            size += 1

        return size

    def __contains__(self, key):
        """
        If any header is present with this key, returns True.
        """
        key = to_bytestring(key)
        return any(_keys_equal(key, k) for k, _ in self._items)

    def keys(self):
        """
        Returns an iterable of the header keys in the mapping. This explicitly
        does not filter duplicates, ensuring that it's the same length as
        len().
        """
        for n, _ in self:
            yield n

    def items(self):
        """
        This mapping iterates like the list of tuples it is.
        """
        return self.__iter__()

    def values(self):
        """
        This is an almost nonsensical query on a header dictionary, but we
        satisfy it in the exact same way we satisfy 'keys'.
        """
        for _, v in self:
            yield v

    def get(self, name, default=None):
        """
        Unlike the dict get, this returns a list of items in the order
        they were added.
        """
        try:
            return self[name]
        except KeyError:
            return default

    def iter_raw(self):
        """
        Allows iterating over the headers in 'raw' form: that is, the form in
        which they were added to the structure. This iteration is in order,
        and can be used to rebuild the original headers (e.g. to determine
        exactly what a server sent).
        """
        for item in self._items:
            yield item

    def replace(self, key, value):
        """
        Replace existing header with new value. If header doesn't exist this
        method work like ``__setitem__``. Replacing leads to deletion of all
        existing headers with the same name.
        """
        key = to_bytestring(key)
        indices = []
        for (i, (k, v)) in enumerate(self._items):
            if _keys_equal(k, key):
                indices.append(i)

        # If the key isn't present, this is easy: just append and abort early.
        if not indices:
            self._items.append((key, value))
            return

        # Delete all but the first. I swear, this is the correct slicing
        # syntax!
        base_index = indices[0]
        for i in indices[:0:-1]:
            self._items.pop(i)

        del self._items[base_index]
        self._items.insert(base_index, (key, value))

    def merge(self, other):
        """
        Merge another header set or any other dict-like into this one.
        """
        # Short circuit to avoid infinite loops in case we try to merge into
        # ourselves.
        if other is self:
            return

        if isinstance(other, HTTPHeaderMap):
            self._items.extend(other.iter_raw())
            return

        for k, v in other.items():
            self._items.append(to_bytestring_tuple(k, v))

    def __eq__(self, other):
        return self._items == other._items

    def __ne__(self, other):
        return self._items != other._items

    def __str__(self):  # pragma: no cover
        return 'HTTPHeaderMap(%s)' % self._items

    def __repr__(self):  # pragma: no cover
        return str(self)


def canonical_form(k, v):
    """
    Returns an iterable of key-value-pairs corresponding to the header in
    canonical form. This means that the header is split on commas unless for
    any reason it's a super-special snowflake (I'm looking at you Set-Cookie).
    """
    SPECIAL_SNOWFLAKES = set([b'set-cookie', b'set-cookie2'])

    k = k.lower()

    if k in SPECIAL_SNOWFLAKES:
        yield k, v
    else:
        for sub_val in v.split(b','):
            yield k, sub_val.strip()


def _keys_equal(x, y):
    """
    Returns 'True' if the two keys are equal by the laws of HTTP headers.
    """
    return x.lower() == y.lower()
