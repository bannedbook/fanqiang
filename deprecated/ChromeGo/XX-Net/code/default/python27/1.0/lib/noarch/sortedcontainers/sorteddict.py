# -*- coding: utf-8 -*-
#
# Sorted dict implementation.

from .sortedset import SortedSet
from .sortedlist import SortedList, recursive_repr
from .sortedlistwithkey import SortedListWithKey
from collections import Set, Sequence
from collections import KeysView as AbstractKeysView
from collections import ValuesView as AbstractValuesView
from collections import ItemsView as AbstractItemsView

from functools import wraps
from sys import hexversion

_NotGiven = object()

def not26(func):
    """Function decorator for methods not implemented in Python 2.6."""

    @wraps(func)
    def errfunc(*args, **kwargs):
        raise NotImplementedError

    if hexversion < 0x02070000:
        return errfunc
    else:
        return func

class _IlocWrapper:
    def __init__(self, _dict):
        self._dict = _dict
    def __len__(self):
        return len(self._dict)
    def __getitem__(self, index):
        """
        Very efficiently return the key at index *index* in iteration. Supports
        negative indices and slice notation. Raises IndexError on invalid
        *index*.
        """
        return self._dict._list[index]
    def __delitem__(self, index):
        """
        Remove the ``sdict[sdict.iloc[index]]`` from *sdict*. Supports negative
        indices and slice notation. Raises IndexError on invalid *index*.
        """
        _temp = self._dict
        _list = _temp._list
        _delitem = _temp._delitem

        if isinstance(index, slice):
            keys = _list[index]
            del _list[index]
            for key in keys:
                _delitem(key)
        else:
            key = _list[index]
            del _list[index]
            _delitem(key)

class SortedDict(dict):
    """
    A SortedDict provides the same methods as a dict.  Additionally, a
    SortedDict efficiently maintains its keys in sorted order. Consequently, the
    keys method will return the keys in sorted order, the popitem method will
    remove the item with the highest key, etc.
    """
    def __init__(self, *args, **kwargs):
        """
        A SortedDict provides the same methods as a dict.  Additionally, a
        SortedDict efficiently maintains its keys in sorted order. Consequently,
        the keys method will return the keys in sorted order, the popitem method
        will remove the item with the highest key, etc.

        An optional *key* argument defines a callable that, like the `key`
        argument to Python's `sorted` function, extracts a comparison key from
        each dict key. If no function is specified, the default compares the
        dict keys directly. The `key` argument must be provided as a positional
        argument and must come before all other arguments.

        An optional *load* argument defines the load factor of the internal list
        used to maintain sort order. If present, this argument must come before
        an iterable. The default load factor of '1000' works well for lists from
        tens to tens of millions of elements.  Good practice is to use a value
        that is the cube root of the list size.  With billions of elements, the
        best load factor depends on your usage.  It's best to leave the load
        factor at the default until you start benchmarking.

        An optional *iterable* argument provides an initial series of items to
        populate the SortedDict.  Each item in the series must itself contain
        two items.  The first is used as a key in the new dictionary, and the
        second as the key's value. If a given key is seen more than once, the
        last value associated with it is retained in the new dictionary.

        If keyword arguments are given, the keywords themselves with their
        associated values are added as items to the dictionary. If a key is
        specified both in the positional argument and as a keyword argument, the
        value associated with the keyword is retained in the dictionary. For
        example, these all return a dictionary equal to ``{"one": 2, "two":
        3}``:

        * ``SortedDict(one=2, two=3)``
        * ``SortedDict({'one': 2, 'two': 3})``
        * ``SortedDict(zip(('one', 'two'), (2, 3)))``
        * ``SortedDict([['two', 3], ['one', 2]])``

        The first example only works for keys that are valid Python
        identifiers; the others work with any valid keys.
        """
        if len(args) > 0 and (args[0] is None or callable(args[0])):
            self._key = args[0]
            args = args[1:]
        else:
            self._key = None

        if len(args) > 0 and type(args[0]) == int:
            self._load = args[0]
            args = args[1:]
        else:
            self._load = 1000

        if self._key is None:
            self._list = SortedList(load=self._load)
        else:
            self._list = SortedListWithKey(key=self._key, load=self._load)

        # Cache function pointers to dict methods.

        _dict = super(SortedDict, self)
        self._dict = _dict
        self._clear = _dict.clear
        self._delitem = _dict.__delitem__
        self._iter = _dict.__iter__
        self._pop = _dict.pop
        self._setdefault = _dict.setdefault
        self._setitem = _dict.__setitem__
        self._update = _dict.update

        # Cache function pointers to SortedList methods.

        self._list_add = self._list.add
        self._list_bisect_left = self._list.bisect_left
        self._list_bisect_right = self._list.bisect_right
        self._list_clear = self._list.clear
        self._list_index = self._list.index
        self._list_pop = self._list.pop
        self._list_remove = self._list.remove
        self._list_update = self._list.update

        self.iloc = _IlocWrapper(self)

        self.update(*args, **kwargs)

    def clear(self):
        """Remove all elements from the dictionary."""
        self._clear()
        self._list_clear()

    def __delitem__(self, key):
        """
        Remove ``d[key]`` from *d*.  Raises a KeyError if *key* is not in the
        dictionary.
        """
        self._delitem(key)
        self._list_remove(key)

    def __iter__(self):
        """Create an iterator over the sorted keys of the dictionary."""
        return iter(self._list)

    def __reversed__(self):
        """
        Create a reversed iterator over the sorted keys of the dictionary.
        """
        return reversed(self._list)

    def __setitem__(self, key, value):
        """Set `d[key]` to *value*."""
        if key not in self:
            self._list_add(key)
        self._setitem(key, value)

    def copy(self):
        """Return a shallow copy of the sorted dictionary."""
        return self.__class__(self._key, self._load, self.iteritems())

    __copy__ = copy

    @classmethod
    def fromkeys(cls, seq, value=None):
        """
        Create a new dictionary with keys from *seq* and values set to *value*.
        """
        return cls((key, value) for key in seq)

    if hexversion < 0x03000000:
        def items(self):
            """
            Return a list of the dictionary's items (``(key, value)`` pairs).
            """
            return list(self.iteritems())
    else:
        def items(self):
            """
            Return a new ItemsView of the dictionary's items.  In addition to
            the methods provided by the built-in `view` the ItemsView is
            indexable (e.g. ``d.items()[5]``).
            """
            return ItemsView(self)

    def iteritems(self):
        """Return an iterable over the items (``(key, value)`` pairs)."""
        return iter((key, self[key]) for key in self._list)

    if hexversion < 0x03000000:
        def keys(self):
            """Return a SortedSet of the dictionary's keys."""
            return SortedSet(self._list, key=self._key, load=self._load)
    else:
        def keys(self):
            """
            Return a new KeysView of the dictionary's keys.  In addition to the
            methods provided by the built-in `view` the KeysView is indexable
            (e.g. ``d.keys()[5]``).
            """
            return KeysView(self)

    def iterkeys(self):
        """Return an iterable over the keys of the dictionary."""
        return iter(self._list)

    if hexversion < 0x03000000:
        def values(self):
            """Return a list of the dictionary's values."""
            return list(self.itervalues())
    else:
        def values(self):
            """
            Return a new :class:`ValuesView` of the dictionary's values.
            In addition to the methods provided by the built-in `view` the
            ValuesView is indexable (e.g., ``d.values()[5]``).
            """
            return ValuesView(self)

    def itervalues(self):
        """Return an iterable over the values of the dictionary."""
        return iter(self[key] for key in self._list)

    def pop(self, key, default=_NotGiven):
        """
        If *key* is in the dictionary, remove it and return its value,
        else return *default*. If *default* is not given and *key* is not in
        the dictionary, a KeyError is raised.
        """
        if key in self:
            self._list_remove(key)
            return self._pop(key)
        else:
            if default is _NotGiven:
                raise KeyError(key)
            else:
                return default

    def popitem(self):
        """
        Remove and return the ``(key, value)`` pair with the greatest *key*
        from the dictionary.

        If the dictionary is empty, calling `popitem` raises a
        KeyError`.
        """
        if not len(self):
            raise KeyError('popitem(): dictionary is empty')

        key = self._list_pop()
        value = self._pop(key)

        return (key, value)

    def setdefault(self, key, default=None):
        """
        If *key* is in the dictionary, return its value.  If not, insert *key*
        with a value of *default* and return *default*.  *default* defaults to
        ``None``.
        """
        if key in self:
            return self[key]
        else:
            self._setitem(key, default)
            self._list_add(key)
            return default

    def update(self, *args, **kwargs):
        """
        Update the dictionary with the key/value pairs from *other*, overwriting
        existing keys.

        *update* accepts either another dictionary object or an iterable of
        key/value pairs (as a tuple or other iterable of length two).  If
        keyword arguments are specified, the dictionary is then updated with
        those key/value pairs: ``d.update(red=1, blue=2)``.
        """
        if not len(self):
            self._update(*args, **kwargs)
            self._list_update(self._iter())
            return

        if (len(kwargs) == 0 and len(args) == 1 and isinstance(args[0], dict)):
            pairs = args[0]
        else:
            pairs = dict(*args, **kwargs)

        if (10 * len(pairs)) > len(self):
            self._update(pairs)
            self._list_clear()
            self._list_update(self._iter())
        else:
            for key in pairs:
                self[key] = pairs[key]

    def index(self, key, start=None, stop=None):
        """
        Return the smallest *k* such that `d.iloc[k] == key` and `i <= k < j`.
        Raises `ValueError` if *key* is not present.  *stop* defaults to the end
        of the set.  *start* defaults to the beginning.  Negative indexes are
        supported, as for slice indices.
        """
        return self._list_index(key, start, stop)

    def bisect_left(self, key):
        """
        Similar to the ``bisect`` module in the standard library, this returns
        an appropriate index to insert *key* in SortedDict. If *key* is
        already present in SortedDict, the insertion point will be before (to
        the left of) any existing entries.
        """
        return self._list_bisect_left(key)

    def bisect(self, key):
        """Same as bisect_right."""
        return self._list_bisect_right(key)

    def bisect_right(self, key):
        """
        Same as `bisect_left`, but if *key* is already present in SortedDict,
        the insertion point will be after (to the right of) any existing
        entries.
        """
        return self._list_bisect_right(key)

    @not26
    def viewkeys(self):
        """
        In Python 2.7 and later, return a new `KeysView` of the dictionary's
        keys.

        In Python 2.6, raise a NotImplementedError.
        """
        return KeysView(self)

    @not26
    def viewvalues(self):
        """
        In Python 2.7 and later, return a new `ValuesView` of the dictionary's
        values.

        In Python 2.6, raise a NotImplementedError.
        """
        return ValuesView(self)

    @not26
    def viewitems(self):
        """
        In Python 2.7 and later, return a new `ItemsView` of the dictionary's
        items.

        In Python 2.6, raise a NotImplementedError.
        """
        return ItemsView(self)

    def __reduce__(self):
        return (self.__class__, (self._key, self._load, list(self.iteritems())))

    @recursive_repr
    def __repr__(self):
        temp = '{0}({1}, {2}, {{{3}}})'
        items = ', '.join('{0}: {1}'.format(repr(key), repr(self[key]))
                          for key in self._list)
        return temp.format(
            self.__class__.__name__,
            repr(self._key),
            repr(self._load),
            items
        )

    def _check(self):
        self._list._check()
        assert len(self) == len(self._list)
        assert all(val in self for val in self._list)

class KeysView(AbstractKeysView, Set, Sequence):
    """
    A KeysView object is a dynamic view of the dictionary's keys, which
    means that when the dictionary's keys change, the view reflects
    those changes.

    The KeysView class implements the Set and Sequence Abstract Base Classes.
    """
    if hexversion < 0x03000000:
        def __init__(self, sorted_dict):
            """
            Initialize a KeysView from a SortedDict container as *sorted_dict*.
            """
            self._list = sorted_dict._list
            self._view = sorted_dict._dict.viewkeys()
    else:
        def __init__(self, sorted_dict):
            """
            Initialize a KeysView from a SortedDict container as *sorted_dict*.
            """
            self._list = sorted_dict._list
            self._view = sorted_dict._dict.keys()
    def __len__(self):
        """Return the number of entries in the dictionary."""
        return len(self._view)
    def __contains__(self, key):
        """
        Return True if and only if *key* is one of the underlying dictionary's
        keys.
        """
        return key in self._view
    def __iter__(self):
        """
        Return an iterable over the keys in the dictionary. Keys are iterated
        over in their sorted order.

        Iterating views while adding or deleting entries in the dictionary may
        raise a RuntimeError or fail to iterate over all entries.
        """
        return iter(self._list)
    def __getitem__(self, index):
        """Return the key at position *index*."""
        return self._list[index]
    def __reversed__(self):
        """
        Return a reversed iterable over the keys in the dictionary. Keys are
        iterated over in their reverse sort order.

        Iterating views while adding or deleting entries in the dictionary may
        raise a RuntimeError or fail to iterate over all entries.
        """
        return reversed(self._list)
    def index(self, value, start=None, stop=None):
        """
        Return the smallest *k* such that `keysview[k] == value` and `start <= k
        < end`.  Raises `KeyError` if *value* is not present.  *stop* defaults
        to the end of the set.  *start* defaults to the beginning.  Negative
        indexes are supported, as for slice indices.
        """
        return self._list.index(value, start, stop)
    def count(self, value):
        """Return the number of occurrences of *value* in the set."""
        return 1 if value in self._view else 0
    def __eq__(self, that):
        """Test set-like equality with *that*."""
        return self._view == that
    def __ne__(self, that):
        """Test set-like inequality with *that*."""
        return self._view != that
    def __lt__(self, that):
        """Test whether self is a proper subset of *that*."""
        return self._view < that
    def __gt__(self, that):
        """Test whether self is a proper superset of *that*."""
        return self._view > that
    def __le__(self, that):
        """Test whether self is contained within *that*."""
        return self._view <= that
    def __ge__(self, that):
        """Test whether *that* is contained within self."""
        return self._view >= that
    def __and__(self, that):
        """Return a SortedSet of the intersection of self and *that*."""
        return SortedSet(self._view & that)
    def __or__(self, that):
        """Return a SortedSet of the union of self and *that*."""
        return SortedSet(self._view | that)
    def __sub__(self, that):
        """Return a SortedSet of the difference of self and *that*."""
        return SortedSet(self._view - that)
    def __xor__(self, that):
        """Return a SortedSet of the symmetric difference of self and *that*."""
        return SortedSet(self._view ^ that)
    if hexversion < 0x03000000:
        def isdisjoint(self, that):
            """Return True if and only if *that* is disjoint with self."""
            return not any(key in self._list for key in that)
    else:
        def isdisjoint(self, that):
            """Return True if and only if *that* is disjoint with self."""
            return self._view.isdisjoint(that)
    @recursive_repr
    def __repr__(self):
        return 'SortedDict_keys({0})'.format(repr(list(self)))

class ValuesView(AbstractValuesView, Sequence):
    """
    A ValuesView object is a dynamic view of the dictionary's values, which
    means that when the dictionary's values change, the view reflects those
    changes.

    The ValuesView class implements the Sequence Abstract Base Class.
    """
    if hexversion < 0x03000000:
        def __init__(self, sorted_dict):
            """
            Initialize a ValuesView from a SortedDict container as
            *sorted_dict*.
            """
            self._dict = sorted_dict
            self._list = sorted_dict._list
            self._view = sorted_dict._dict.viewvalues()
    else:
        def __init__(self, sorted_dict):
            """
            Initialize a ValuesView from a SortedDict container as
            *sorted_dict*.
            """
            self._dict = sorted_dict
            self._list = sorted_dict._list
            self._view = sorted_dict._dict.values()
    def __len__(self):
        """Return the number of entries in the dictionary."""
        return len(self._dict)
    def __contains__(self, value):
        """
        Return True if and only if *value* is on the underlying dictionary's
        values.
        """
        return value in self._view
    def __iter__(self):
        """
        Return an iterator over the values in the dictionary.  Values are
        iterated over in sorted order of the keys.

        Iterating views while adding or deleting entries in the dictionary may
        raise a `RuntimeError` or fail to iterate over all entries.
        """
        _dict = self._dict
        return iter(_dict[key] for key in self._list)
    def __getitem__(self, index):
        """
        Efficiently return value at *index* in iteration.

        Supports slice notation and negative indexes.
        """
        _dict, _list = self._dict, self._list
        if isinstance(index, slice):
            return [_dict[key] for key in _list[index]]
        else:
            return _dict[_list[index]]
    def __reversed__(self):
        """
        Return a reverse iterator over the values in the dictionary.  Values are
        iterated over in reverse sort order of the keys.

        Iterating views while adding or deleting entries in the dictionary may
        raise a `RuntimeError` or fail to iterate over all entries.
        """
        _dict = self._dict
        return iter(_dict[key] for key in reversed(self._list))
    def index(self, value):
        """
        Return index of *value* in self.

        Raises ValueError if *value* is not found.
        """
        for idx, val in enumerate(self):
            if value == val:
                return idx
        else:
            raise ValueError('{0} is not in dict'.format(repr(value)))
    if hexversion < 0x03000000:
        def count(self, value):
            """Return the number of occurrences of *value* in self."""
            return sum(1 for val in self._dict.itervalues() if val == value)
    else:
        def count(self, value):
            """Return the number of occurrences of *value* in self."""
            return sum(1 for val in _dict.values() if val == value)
    def __lt__(self, that):
        raise TypeError
    def __gt__(self, that):
        raise TypeError
    def __le__(self, that):
        raise TypeError
    def __ge__(self, that):
        raise TypeError
    def __and__(self, that):
        raise TypeError
    def __or__(self, that):
        raise TypeError
    def __sub__(self, that):
        raise TypeError
    def __xor__(self, that):
        raise TypeError
    @recursive_repr
    def __repr__(self):
        return 'SortedDict_values({0})'.format(repr(list(self)))

class ItemsView(AbstractItemsView, Set, Sequence):
    """
    An ItemsView object is a dynamic view of the dictionary's ``(key,
    value)`` pairs, which means that when the dictionary changes, the
    view reflects those changes.

    The ItemsView class implements the Set and Sequence Abstract Base Classes.
    However, the set-like operations (``&``, ``|``, ``-``, ``^``) will only
    operate correctly if all of the dictionary's values are hashable.
    """
    if hexversion < 0x03000000:
        def __init__(self, sorted_dict):
            """
            Initialize an ItemsView from a SortedDict container as
            *sorted_dict*.
            """
            self._dict = sorted_dict
            self._list = sorted_dict._list
            self._view = sorted_dict._dict.viewitems()
    else:
        def __init__(self, sorted_dict):
            """
            Initialize an ItemsView from a SortedDict container as
            *sorted_dict*.
            """
            self._dict = sorted_dict
            self._list = sorted_dict._list
            self._view = sorted_dict._dict.items()
    def __len__(self):
        """Return the number of entries in the dictionary."""
        return len(self._view)
    def __contains__(self, key):
        """
        Return True if and only if *key* is one of the underlying dictionary's
        items.
        """
        return key in self._view
    def __iter__(self):
        """
        Return an iterable over the items in the dictionary. Items are iterated
        over in their sorted order.

        Iterating views while adding or deleting entries in the dictionary may
        raise a RuntimeError or fail to iterate over all entries.
        """
        _dict = self._dict
        return iter((key, _dict[key]) for key in self._list)
    def __getitem__(self, index):
        """Return the item as position *index*."""
        _dict, _list = self._dict, self._list
        if isinstance(index, slice):
            return [(key, _dict[key]) for key in _list[index]]
        else:
            key = _list[index]
            return (key, _dict[key])
    def __reversed__(self):
        """
        Return a reversed iterable over the items in the dictionary. Items are
        iterated over in their reverse sort order.

        Iterating views while adding or deleting entries in the dictionary may
        raise a RuntimeError or fail to iterate over all entries.
        """
        _dict = self._dict
        return iter((key, _dict[key]) for key in reversed(self._list))
    def index(self, key, start=None, stop=None):
        """
        Return the smallest *k* such that `itemssview[k] == key` and `start <= k
        < end`.  Raises `KeyError` if *key* is not present.  *stop* defaults
        to the end of the set.  *start* defaults to the beginning.  Negative
        indexes are supported, as for slice indices.
        """
        temp, value = key
        pos = self._list.index(temp, start, stop)
        if value == self._dict[temp]:
            return pos
        else:
            raise ValueError('{0} is not in dict'.format(repr(key)))
    def count(self, item):
        """Return the number of occurrences of *item* in the set."""
        key, value = item
        return 1 if key in self._dict and self._dict[key] == value else 0
    def __eq__(self, that):
        """Test set-like equality with *that*."""
        return self._view == that
    def __ne__(self, that):
        """Test set-like inequality with *that*."""
        return self._view != that
    def __lt__(self, that):
        """Test whether self is a proper subset of *that*."""
        return self._view < that
    def __gt__(self, that):
        """Test whether self is a proper superset of *that*."""
        return self._view > that
    def __le__(self, that):
        """Test whether self is contained within *that*."""
        return self._view <= that
    def __ge__(self, that):
        """Test whether *that* is contained within self."""
        return self._view >= that
    def __and__(self, that):
        """Return a SortedSet of the intersection of self and *that*."""
        return SortedSet(self._view & that)
    def __or__(self, that):
        """Return a SortedSet of the union of self and *that*."""
        return SortedSet(self._view | that)
    def __sub__(self, that):
        """Return a SortedSet of the difference of self and *that*."""
        return SortedSet(self._view - that)
    def __xor__(self, that):
        """Return a SortedSet of the symmetric difference of self and *that*."""
        return SortedSet(self._view ^ that)
    if hexversion < 0x03000000:
        def isdisjoint(self, that):
            """Return True if and only if *that* is disjoint with self."""
            _dict = self._dict
            for key, value in that:
                if key in _dict and _dict[key] == value:
                    return False
            return True
    else:
        def isdisjoint(self, that):
            """Return True if and only if *that* is disjoint with self."""
            return self._view.isdisjoint(that)
    @recursive_repr
    def __repr__(self):
        return 'SortedDict_items({0})'.format(repr(list(self)))
