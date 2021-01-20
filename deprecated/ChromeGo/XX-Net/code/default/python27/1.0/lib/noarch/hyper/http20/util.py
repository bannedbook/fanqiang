# -*- coding: utf-8 -*-
"""
hyper/http20/util
~~~~~~~~~~~~~~~~~

Utility functions for use with hyper.
"""
from collections import defaultdict


def combine_repeated_headers(kvset):
    """
    Given a list of key-value pairs (like for HTTP headers!), combines pairs
    with the same key together, separating the values with NULL bytes. This
    function maintains the order of input keys, because it's awesome.
    """
    def set_pop(set, item):
        set.remove(item)
        return item

    headers = defaultdict(list)
    keys = set()

    for key, value in kvset:
        headers[key].append(value)
        keys.add(key)

    return [(set_pop(keys, k), b'\x00'.join(headers[k])) for k, v in kvset
            if k in keys]

def split_repeated_headers(kvset):
    """
    Given a set of key-value pairs (like for HTTP headers!), finds values that
    have NULL bytes in them and splits them into a dictionary whose values are
    lists.
    """
    headers = defaultdict(list)

    for key, value in kvset:
        headers[key] = value.split(b'\x00')

    return dict(headers)


def h2_safe_headers(headers):
    """
    This method takes a set of headers that are provided by the user and
    transforms them into a form that is safe for emitting over HTTP/2.

    Currently, this strips the Connection header and any header it refers to.
    """
    stripped = {i.lower().strip() for k, v in headers if k == 'connection'
                                  for i in v.split(',')}
    stripped.add('connection')

    return [header for header in headers if header[0] not in stripped]
