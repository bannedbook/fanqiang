# -*- coding: utf-8 -*-

"""
    Bimap - bidirectional mapping between code/value
"""

class BimapError(Exception):
    pass

class Bimap(object):

    """
        Bi-directional mapping between code/value.

        Initialised using:

            name:   Used for exceptions
            dict:   Dict mapping from value (numeric) to code (text)
            error:  Error type to raise if key not found

        The class provides:

            * A 'forward' map (code->text) which is accessed through
              __getitem__ (bimap[code])
            * A 'reverse' map (code>value) which is accessed through
              __getattr__ (bimap.text)
            * A 'get' method which does a forward lookup (code->text)
              and returns a textual version of code if there is no
              explicit mapping (or default provided)

        >>> class TestError(Exception):
        ...     pass

        >>> TEST = Bimap('TEST',{1:'A', 2:'B', 3:'C'},TestError)
        >>> TEST[1]
        'A'
        >>> TEST.A
        1
        >>> TEST.X
        Traceback (most recent call last):
        ...
        TestError: TEST: Invalid reverse lookup: [X]
        >>> TEST[99]
        Traceback (most recent call last):
        ...
        TestError: TEST: Invalid forward lookup: [99]
        >>> TEST.get(99)
        '99'

    """

    def __init__(self,name,forward,error=KeyError):
        self.name = name
        self.error = error
        self.forward = forward.copy()
        self.reverse = dict([(v,k) for (k,v) in list(forward.items())])

    def get(self,k,default=None):
        try:
            return self.forward[k]
        except KeyError as e:
            return default or str(k)

    def __getitem__(self,k):
        try:
            return self.forward[k]
        except KeyError as e:
            raise self.error("%s: Invalid forward lookup: [%s]" % (self.name,k))

    def __getattr__(self,k):
        try:
            return self.reverse[k]
        except KeyError as e:
            raise self.error("%s: Invalid reverse lookup: [%s]" % (self.name,k))

if __name__ == '__main__':
    import doctest
    doctest.testmod()
