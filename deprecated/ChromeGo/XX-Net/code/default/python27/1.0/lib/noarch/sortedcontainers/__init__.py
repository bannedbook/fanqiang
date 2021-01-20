# -*- coding: utf-8 -*-

"""
sortedcontainers Sorted Container Types Library
===============================================

SortedContainers is an Apache2 licensed containers library, written in
pure-Python, and fast as C-extensions.

Python's standard library is great until you need a sorted container type. Many
will attest that you can get really far without one, but the moment you
**really need** a sorted list, dict, or set, you're faced with a dozen
different implementations, most using C-extensions without great documentation
and benchmarking.

Things shouldn't be this way. Not in Python.

::

    >>> from sortedcontainers import SortedList, SortedDict, SortedSet
    >>> sl = SortedList(xrange(10000000))
    >>> 1234567 in sl
    True
    >>> sl[7654321]
    7654321
    >>> sl.add(1234567)
    >>> sl.count(1234567)
    2
    >>> sl *= 3
    >>> len(sl)
    30000003

SortedContainers takes all of the work out of Python sorted types - making your
deployment and use of Python easy. There's no need to install a C compiler or
pre-build and distribute custom extensions. Performance is a feature and
testing has 100% coverage with unit tests and hours of stress.

:copyright: (c) 2014 by Grant Jenks.
:license: Apache 2.0, see LICENSE for more details.

"""

__title__ = 'sortedcontainers'
__version__ = '0.9.4'
__build__ = 0x000904
__author__ = 'Grant Jenks'
__license__ = 'Apache 2.0'
__copyright__ = 'Copyright 2014 Grant Jenks'

from .sortedlist import SortedList
from .sortedset import SortedSet
from .sorteddict import SortedDict
from .sortedlistwithkey import SortedListWithKey

__all__ = ['SortedList', 'SortedSet', 'SortedDict', 'SortedListWithKey']
