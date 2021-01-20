# -*- coding: utf-8 -*-

"""
    Wrapper around property builtin to restrict attribute to defined
    integer value range (throws ValueError).

    Intended to ensure that values packed with struct are in the
    correct range

    >>> class T(object):
    ...     a = range_property('a',-100,100)
    ...     b = B('b')
    ...     c = H('c')
    ...     d = I('d')
    >>> t = T()
    >>> for i in [0,100,-100]:
    ...     t.a = i
    ...     assert t.a == i
    >>> t.a = 101
    Traceback (most recent call last):
    ...
    ValueError: Attribute 'a' must be between -100-100 [101]
    >>> t.a = -101
    Traceback (most recent call last):
    ...
    ValueError: Attribute 'a' must be between -100-100 [-101]
    >>> t.a = 'blah'
    Traceback (most recent call last):
    ...
    ValueError: Attribute 'a' must be between -100-100 [blah]

"""

import sys
if sys.version < '3':
    int_types = (int, long,)
else:
    int_types = (int,)

def range_property(attr,min,max):
    def getter(obj):
        return getattr(obj,"_%s" % attr)
    def setter(obj,val):
        if isinstance(val,int_types) and min <= val <= max:
            setattr(obj,"_%s" % attr,val)
        else:
            raise ValueError("Attribute '%s' must be between %d-%d [%s]" %
                                        (attr,min,max,val))
    return property(getter,setter)

def B(attr):
    """
        Unsigned Byte
    """
    return range_property(attr,0,255)

def H(attr):
    """
        Unsigned Short
    """
    return range_property(attr,0,65535)

def I(attr):
    """
        Unsigned Long
    """
    return range_property(attr,0,4294967295)

def ntuple_range(attr,n,min,max):
    f = lambda x : isinstance(x,int_types) and min <= x <= max
    def getter(obj):
        return getattr(obj,"_%s" % attr)
    def setter(obj,val):
        if len(val) != n:
            raise ValueError("Attribute '%s' must be tuple with %d elements [%s]" %
                                        (attr,n,val))
        if all(map(f,val)):
            setattr(obj,"_%s" % attr,val)
        else:
            raise ValueError("Attribute '%s' elements must be between %d-%d [%s]" %
                                        (attr,min,max,val))
    return property(getter,setter)

def IP4(attr):
    return ntuple_range(attr,4,0,255)

def IP6(attr):
    return ntuple_range(attr,16,0,255)

if __name__ == '__main__':
    import doctest
    doctest.testmod()

