# -*- coding: utf-8 -*-
#
# Sorted list implementation.

from __future__ import print_function
from sys import hexversion

from bisect import bisect_left, bisect_right, insort
from itertools import chain, repeat, starmap
from collections import MutableSequence
from operator import iadd, add
from functools import wraps
from math import log

if hexversion < 0x03000000:
    from itertools import izip as zip
    from itertools import imap as map
    try:
        from thread import get_ident
    except ImportError:
        from dummy_thread import get_ident
else:
    from functools import reduce
    try:
        from _thread import get_ident
    except ImportError:
        from _dummy_thread import get_ident

def recursive_repr(func):
    """Decorator to prevent infinite repr recursion."""
    repr_running = set()

    @wraps(func)
    def wrapper(self):
        key = id(self), get_ident()

        if key in repr_running:
            return '...'

        repr_running.add(key)

        try:
            return func(self)
        finally:
            repr_running.discard(key)

    return wrapper

class SortedList(MutableSequence):
    """
    SortedList provides most of the same methods as a list but keeps the items
    in sorted order.
    """

    def __init__(self, iterable=None, load=1000):
        """
        SortedList provides most of the same methods as a list but keeps the
        items in sorted order.

        An optional *iterable* provides an initial series of items to populate
        the SortedList.

        An optional *load* specifies the load-factor of the list. The default
        load factor of '1000' works well for lists from tens to tens of millions
        of elements.  Good practice is to use a value that is the cube root of
        the list size.  With billions of elements, the best load factor depends
        on your usage.  It's best to leave the load factor at the default until
        you start benchmarking.
        """
        self._len, self._maxes, self._lists, self._index = 0, [], [], []
        self._load, self._twice, self._half = load, load * 2, load >> 1
        self._offset = 0

        if iterable is not None:
            self.update(iterable)

    def clear(self):
        """Remove all the elements from the list."""
        self._len = 0
        del self._maxes[:]
        del self._lists[:]
        del self._index[:]

    def add(self, val):
        """Add the element *val* to the list."""
        _maxes, _lists = self._maxes, self._lists

        if _maxes:
            pos = bisect_right(_maxes, val)

            if pos == len(_maxes):
                pos -= 1
                _maxes[pos] = val
                _lists[pos].append(val)
            else:
                insort(_lists[pos], val)

            self._expand(pos)
        else:
            _maxes.append(val)
            _lists.append([val])

        self._len += 1

    def _expand(self, pos):
        """Splits sublists that are more than double the load level.

        Updates the index when the sublist length is less than double the load
        level. This requires incrementing the nodes in a traversal from the leaf
        node to the root. For an example traversal see self._loc.
        """
        _lists, _index = self._lists, self._index

        if len(_lists[pos]) > self._twice:
            _maxes, _load = self._maxes, self._load
            half = _lists[pos][_load:]
            del _lists[pos][_load:]
            _maxes[pos] = _lists[pos][-1]
            _maxes.insert(pos + 1, half[-1])
            _lists.insert(pos + 1, half)
            del _index[:]
        else:
            if len(_index) > 0:
                child = self._offset + pos
                while child > 0:
                    _index[child] += 1
                    child = (child - 1) >> 1
                _index[0] += 1

    def update(self, iterable):
        """Update the list by adding all elements from *iterable*."""
        _maxes, _lists = self._maxes, self._lists
        values = sorted(iterable)

        if _maxes:
            if len(values) * 4 >= self._len:
                values.extend(chain.from_iterable(_lists))
                values.sort()
                self.clear()
            else:
                _add = self.add
                for val in values:
                    _add(val)
                return

        _load, _index = self._load, self._index
        _lists.extend(values[pos:(pos + _load)]
                      for pos in range(0, len(values), _load))
        _maxes.extend(sublist[-1] for sublist in _lists)
        self._len = len(values)
        del _index[:]

    def __contains__(self, val):
        """Return True if and only if *val* is an element in the list."""
        _maxes = self._maxes

        if not _maxes:
            return False

        pos = bisect_left(_maxes, val)

        if pos == len(_maxes):
            return False

        _lists = self._lists
        idx = bisect_left(_lists[pos], val)
        return _lists[pos][idx] == val

    def discard(self, val):
        """
        Remove the first occurrence of *val*.

        If *val* is not a member, does nothing.
        """
        _maxes = self._maxes

        if not _maxes:
            return

        pos = bisect_left(_maxes, val)

        if pos == len(_maxes):
            return

        _lists = self._lists
        idx = bisect_left(_lists[pos], val)
        if _lists[pos][idx] == val:
            self._delete(pos, idx)

    def remove(self, val):
        """
        Remove first occurrence of *val*.

        Raises ValueError if *val* is not present.
        """
        _maxes = self._maxes

        if not _maxes:
            raise ValueError('{0} not in list'.format(repr(val)))

        pos = bisect_left(_maxes, val)

        if pos == len(_maxes):
            raise ValueError('{0} not in list'.format(repr(val)))

        _lists = self._lists
        idx = bisect_left(_lists[pos], val)
        if _lists[pos][idx] == val:
            self._delete(pos, idx)
        else:
            raise ValueError('{0} not in list'.format(repr(val)))

    def _delete(self, pos, idx):
        """Delete the item at the given (pos, idx).

        Combines lists that are less than half the load level.

        Updates the index when the sublist length is more than half the load
        level. This requires decrementing the nodes in a traversal from the leaf
        node to the root. For an example traversal see self._loc.
        """
        _maxes, _lists, _index = self._maxes, self._lists, self._index

        lists_pos = _lists[pos]

        del lists_pos[idx]
        self._len -= 1

        len_lists_pos = len(lists_pos)

        if len_lists_pos > self._half:

            _maxes[pos] = lists_pos[-1]

            if len(_index) > 0:
                child = self._offset + pos
                while child > 0:
                    _index[child] -= 1
                    child = (child - 1) >> 1
                _index[0] -= 1

        elif len(_lists) > 1:

            if not pos:
                pos += 1

            prev = pos - 1
            _lists[prev].extend(_lists[pos])
            _maxes[prev] = _lists[prev][-1]

            del _maxes[pos]
            del _lists[pos]
            del _index[:]

            self._expand(prev)

        elif len_lists_pos:

            _maxes[pos] = lists_pos[-1]

        else:

            del _maxes[pos]
            del _lists[pos]
            del _index[:]

    def _loc(self, pos, idx):
        """Convert an index pair (alpha, beta) into a single index that corresponds to
        the position of the value in the sorted list.

        Most queries require the index be built. Details of the index are
        described in self._build_index.

        Indexing requires traversing the tree from a leaf node to the root. The
        parent of each node is easily computable at (pos - 1) // 2.

        Left-child nodes are always at odd indices and right-child nodes are
        always at even indices.

        When traversing up from a right-child node, increment the total by the
        left-child node.

        The final index is the sum from traversal and the index in the sublist.

        For example, using the index from self._build_index:

        _index = 14 5 9 3 2 4 5
        _offset = 3

        Tree:

                 14
              5      9
            3   2  4   5

        Converting index pair (2, 3) into a single index involves iterating like
        so:

        1. Starting at the leaf node: offset + alpha = 3 + 2 = 5. We identify
           the node as a left-child node. At such nodes, we simply traverse to
           the parent.

        2. At node 9, position 2, we recognize the node as a right-child node
           and accumulate the left-child in our total. Total is now 5 and we
           traverse to the parent at position 0.

        3. Iteration ends at the root.

        Computing the index is the sum of the total and beta: 5 + 3 = 8.
        """
        if not pos:
            return idx

        _index = self._index

        if not len(_index):
            self._build_index()

        total = 0

        # Increment pos to point in the index to len(self._lists[pos]).

        pos += self._offset

        # Iterate until reaching the root of the index tree at pos = 0.

        while pos:

            # Right-child nodes are at odd indices. At such indices
            # account the total below the left child node.

            if not (pos & 1):
                total += _index[pos - 1]

            # Advance pos to the parent node.

            pos = (pos - 1) >> 1

        return total + idx

    def _pos(self, idx):
        """Convert an index into a pair (alpha, beta) that can be used to access
        the corresponding _lists[alpha][beta] position.

        Most queries require the index be built. Details of the index are
        described in self._build_index.

        Indexing requires traversing the tree to a leaf node. Each node has
        two children which are easily computable. Given an index, pos, the
        left-child is at pos * 2 + 1 and the right-child is at pos * 2 + 2.

        When the index is less than the left-child, traversal moves to the
        left sub-tree. Otherwise, the index is decremented by the left-child
        and traversal moves to the right sub-tree.

        At a child node, the indexing pair is computed from the relative
        position of the child node as compared with the offset and the remaining
        index.

        For example, using the index from self._build_index:

        _index = 14 5 9 3 2 4 5
        _offset = 3

        Tree:

                 14
              5      9
            3   2  4   5

        Indexing position 8 involves iterating like so:

        1. Starting at the root, position 0, 8 is compared with the left-child
           node (5) which it is greater than. When greater the index is
           decremented and the position is updated to the right child node.

        2. At node 9 with index 3, we again compare the index to the left-child
           node with value 4. Because the index is the less than the left-child
           node, we simply traverse to the left.

        3. At node 4 with index 3, we recognize that we are at a leaf node and
           stop iterating.

        4. To compute the sublist index, we subtract the offset from the index
           of the leaf node: 5 - 3 = 2. To compute the index in the sublist, we
           simply use the index remaining from iteration. In this case, 3.

        The final index pair from our example is (2, 3) which corresponds to
        index 8 in the sorted list.
        """
        _len, _lists = self._len, self._lists

        if idx < 0:
            last_len = len(_lists[-1])
            if (-idx) <= last_len:
                return len(_lists) - 1, last_len + idx
            idx += _len
            if idx < 0:
                raise IndexError('list index out of range')
        elif idx >= _len:
            raise IndexError('list index out of range')

        if idx < len(_lists[0]):
            return 0, idx

        _index = self._index

        if not len(_index):
            self._build_index()

        pos = 0
        len_index = len(_index)
        child = (pos << 1) + 1

        while child < len_index:
            index_child = _index[child]

            if idx < index_child:
                pos = child
            else:
                idx -= index_child
                pos = child + 1

            child = (pos << 1) + 1

        return (pos - self._offset, idx)

    def _build_index(self):
        """Build an index for indexing the sorted list.

        Indexes are represented as binary trees in a dense array notation
        similar to a binary heap.

        For example, given a _lists representation storing integers:

        [0]: 1 2 3
        [1]: 4 5
        [2]: 6 7 8 9
        [3]: 10 11 12 13 14

        The first transformation maps the sub-lists by their length. The
        first row of the index is the length of the sub-lists.

        [0]: 3 2 4 5

        Each row after that is the sum of consecutive pairs of the previous row:

        [1]: 5 9
        [2]: 14

        Finally, the index is built by concatenating these lists together:

        _index = 14 5 9 3 2 4 5

        An offset storing the start of the first row is also stored:

        _offset = 3

        When built, the index can be used for efficient indexing into the list.
        See the comment and notes on self._pos for details.
        """
        row0 = list(map(len, self._lists))

        if len(row0) == 1:
            self._index[:] = row0
            self._offset = 0
            return

        head = iter(row0)
        tail = iter(head)
        row1 = list(starmap(add, zip(head, tail)))

        if len(row0) & 1:
            row1.append(row0[-1])

        if len(row1) == 1:
            self._index[:] = row1 + row0
            self._offset = 1
            return

        size = 2 ** (int(log(len(row1) - 1, 2)) + 1)
        row1.extend(repeat(0, size - len(row1)))
        tree = [row0, row1]

        while len(tree[-1]) > 1:
            head = iter(tree[-1])
            tail = iter(head)
            row = list(starmap(add, zip(head, tail)))
            tree.append(row)

        reduce(iadd, reversed(tree), self._index)
        self._offset = size * 2 - 1

    def _slice(self, slc):
        start, stop, step = slc.start, slc.stop, slc.step

        if step == 0:
            raise ValueError('slice step cannot be zero')

        # Set defaults for missing values.

        if step is None:
            step = 1

        if step > 0:
            if start is None:
                start = 0

            if stop is None:
                stop = len(self)
            elif stop < 0:
                stop += len(self)
        else:
            if start is None:
                start = len(self) - 1

            if stop is None:
                stop = -1
            elif stop < 0:
                stop += len(self)

        if start < 0:
            start += len(self)

        # Fix indices that are too big or too small.
        # Slice notation is surprisingly permissive
        # where normal indexing would raise IndexError.

        if step > 0:
            if start < 0:
                start = 0
            elif start > len(self):
                start = len(self)

            if stop < 0:
                stop = 0
            elif stop > len(self):
                stop = len(self)
        else:
            if start < 0:
                start = -1
            elif start >= len(self):
                start = len(self) - 1

            if stop < 0:
                stop = -1
            elif stop > len(self):
                stop = len(self)

        return start, stop, step

    def __delitem__(self, idx):
        """Remove the element at *idx*. Supports slicing."""
        if isinstance(idx, slice):
            start, stop, step = self._slice(idx)

            if ((step == 1) and (start < stop)
                    and ((stop - start) * 8 >= self._len)):

                values = self[:start]
                if stop < self._len:
                    values += self[stop:]
                self.clear()
                self.update(values)
                return

            indices = range(start, stop, step)

            # Delete items from greatest index to least so
            # that the indices remain valid throughout iteration.

            if step > 0:
                indices = reversed(indices)

            _pos, _delete = self._pos, self._delete

            for index in indices:
                pos, idx = _pos(index)
                _delete(pos, idx)
        else:
            pos, idx = self._pos(idx)
            self._delete(pos, idx)

    def __getitem__(self, idx):
        """Return the element at *idx*. Supports slicing."""
        _lists = self._lists

        if isinstance(idx, slice):
            start, stop, step = self._slice(idx)

            if step == 1 and start < stop:
                if start == 0 and stop == self._len:
                    return self.as_list()

                start_pos, start_idx = self._pos(start)

                if stop == self._len:
                    stop_pos = len(_lists) - 1
                    stop_idx = len(_lists[stop_pos])
                else:
                    stop_pos, stop_idx = self._pos(stop)

                if start_pos == stop_pos:
                    return _lists[start_pos][start_idx:stop_idx]

                prefix = _lists[start_pos][start_idx:]
                middle = _lists[(start_pos + 1):stop_pos]
                result = reduce(iadd, middle, prefix)
                result += _lists[stop_pos][:stop_idx]

                return result

            if step == -1 and start > stop:
                result = self[(stop + 1):(start + 1)]
                result.reverse()
                return result

            # Return a list because a negative step could
            # reverse the order of the items and this could
            # be the desired behavior.

            indices = range(start, stop, step)
            return list(self[index] for index in indices)
        else:
            pos, idx = self._pos(idx)
            return _lists[pos][idx]

    def _check_order(self, idx, val):
        _lists, _len = self._lists, self._len

        pos, loc = self._pos(idx)

        if idx < 0:
            idx += _len

        # Check that the inserted value is not less than the
        # previous value.

        if idx > 0:
            idx_prev = loc - 1
            pos_prev = pos

            if idx_prev < 0:
                pos_prev -= 1
                idx_prev = len(_lists[pos_prev]) - 1

            if _lists[pos_prev][idx_prev] > val:
                msg = '{0} not in sort order at index {1}'.format(repr(val), idx)
                raise ValueError(msg)

        # Check that the inserted value is not greater than
        # the previous value.

        if idx < (_len - 1):
            idx_next = loc + 1
            pos_next = pos

            if idx_next == len(_lists[pos_next]):
                pos_next += 1
                idx_next = 0

            if _lists[pos_next][idx_next] < val:
                msg = '{0} not in sort order at index {1}'.format(repr(val), idx)
                raise ValueError(msg)

    def __setitem__(self, index, value):
        """
        Replace the item at position *index* with *value*.

        Supports slice notation. Raises a :exc:`ValueError` if the sort order
        would be violated. When used with a slice and iterable, the
        :exc:`ValueError` is raised before the list is mutated if the sort order
        would be violated by the operation.
        """
        _maxes, _lists, _pos = self._maxes, self._lists, self._pos
        _check_order = self._check_order

        if isinstance(index, slice):
            start, stop, step = self._slice(index)
            indices = range(start, stop, step)

            if step != 1:
                if not hasattr(value, '__len__'):
                    value = list(value)

                indices = list(indices)

                if len(value) != len(indices):
                    raise ValueError(
                        'attempt to assign sequence of size {0}'
                        ' to extended slice of size {1}'
                        .format(len(value), len(indices)))

                # Keep a log of values that are set so that we can
                # roll back changes if ordering is violated.

                log = []
                _append = log.append

                for idx, val in zip(indices, value):
                    pos, loc = _pos(idx)
                    _append((idx, _lists[pos][loc], val))
                    _lists[pos][loc] = val
                    if len(_lists[pos]) == (loc + 1):
                        _maxes[pos] = val

                try:
                    # Validate ordering of new values.

                    for idx, oldval, newval in log:
                        _check_order(idx, newval)

                except ValueError:

                    # Roll back changes from log.

                    for idx, oldval, newval in log:
                        pos, loc = _pos(idx)
                        _lists[pos][loc] = oldval
                        if len(_lists[pos]) == (loc + 1):
                            _maxes[pos] = oldval

                    raise
            else:
                # Test ordering using indexing. If the value given
                # doesn't support getitem, convert it to a list.

                if not hasattr(value, '__getitem__'):
                    value = list(value)

                # Check that the given values are ordered properly.

                ordered = all(value[pos - 1] <= value[pos]
                              for pos in range(1, len(value)))

                if not ordered:
                    raise ValueError('given sequence not in sort order')

                # Check ordering in context of sorted list.

                if not start or not len(value):
                    # Nothing to check on the lhs.
                    pass
                else:
                    if self[start - 1] > value[0]:
                        msg = '{0} not in sort order at index {1}'.format(repr(value[0]), start)
                        raise ValueError(msg)

                if stop == len(self) or not len(value):
                    # Nothing to check on the rhs.
                    pass
                else:
                    # "stop" is exclusive so we don't need
                    # to add one for the index.
                    if self[stop] < value[-1]:
                        msg = '{0} not in sort order at index {1}'.format(repr(value[-1]), stop)
                        raise ValueError(msg)

                # Delete the existing values.

                del self[index]

                # Insert the new values.

                _insert = self.insert
                for idx, val in enumerate(value):
                    _insert(start + idx, val)
        else:
            pos, loc = _pos(index)
            _check_order(index, value)
            _lists[pos][loc] = value
            if len(_lists[pos]) == (loc + 1):
                _maxes[pos] = value

    def __iter__(self):
        """Create an iterator over the list."""
        return chain.from_iterable(self._lists)

    def __reversed__(self):
        """Create an iterator to traverse the list in reverse."""
        return chain.from_iterable(map(reversed, reversed(self._lists)))

    def __len__(self):
        """Return the number of elements in the list."""
        return self._len

    def bisect_left(self, val):
        """
        Similar to the *bisect* module in the standard library, this returns an
        appropriate index to insert *val*. If *val* is already present, the
        insertion point will be before (to the left of) any existing entries.
        """
        _maxes = self._maxes

        if not _maxes:
            return 0

        pos = bisect_left(_maxes, val)

        if pos == len(_maxes):
            return self._len

        idx = bisect_left(self._lists[pos], val)

        return self._loc(pos, idx)

    def bisect_right(self, val):
        """
        Same as *bisect_left*, but if *val* is already present, the insertion
        point will be after (to the right of) any existing entries.
        """
        _maxes = self._maxes

        if not _maxes:
            return 0

        pos = bisect_right(_maxes, val)

        if pos == len(_maxes):
            return self._len

        idx = bisect_right(self._lists[pos], val)

        return self._loc(pos, idx)

    bisect = bisect_right

    def count(self, val):
        """Return the number of occurrences of *val* in the list."""
        _maxes = self._maxes

        if not _maxes:
            return 0

        pos_left = bisect_left(_maxes, val)

        if pos_left == len(_maxes):
            return 0

        _lists = self._lists
        idx_left = bisect_left(_lists[pos_left], val)
        pos_right = bisect_right(_maxes, val)

        if pos_right == len(_maxes):
            return self._len - self._loc(pos_left, idx_left)

        idx_right = bisect_right(_lists[pos_right], val)

        if pos_left == pos_right:
            return idx_right - idx_left

        right = self._loc(pos_right, idx_right)
        left = self._loc(pos_left, idx_left)

        return right - left

    def copy(self):
        """Return a shallow copy of the sorted list."""
        return self.__class__(self, load=self._load)

    __copy__ = copy

    def append(self, val):
        """
        Append the element *val* to the list. Raises a ValueError if the *val*
        would violate the sort order.
        """
        _maxes, _lists = self._maxes, self._lists

        if not _maxes:
            _maxes.append(val)
            _lists.append([val])
            self._len = 1
            return

        pos = len(_lists) - 1

        if val < _lists[pos][-1]:
            msg = '{0} not in sort order at index {1}'.format(repr(val), self._len)
            raise ValueError(msg)

        _maxes[pos] = val
        _lists[pos].append(val)
        self._len += 1
        self._expand(pos)

    def extend(self, values):
        """
        Extend the list by appending all elements from the *values*. Raises a
        ValueError if the sort order would be violated.
        """
        _maxes, _lists, _load = self._maxes, self._lists, self._load

        if not isinstance(values, list):
            values = list(values)

        if any(values[pos - 1] > values[pos]
               for pos in range(1, len(values))):
            raise ValueError('given sequence not in sort order')

        offset = 0

        if _maxes:
            if values[0] < _lists[-1][-1]:
                msg = '{0} not in sort order at index {1}'.format(repr(values[0]), self._len)
                raise ValueError(msg)

            if len(_lists[-1]) < self._half:
                _lists[-1].extend(values[:_load])
                _maxes[-1] = _lists[-1][-1]
                offset = _load

        len_lists = len(_lists)

        for idx in range(offset, len(values), _load):
            _lists.append(values[idx:(idx + _load)])
            _maxes.append(_lists[-1][-1])

        _index = self._index

        if len_lists == len(_lists):
            len_index = len(_index)
            if len_index > 0:
                len_values = len(values)
                child = len_index - 1
                while child:
                    _index[child] += len_values
                    child = (child - 1) >> 1
                _index[0] += len_values
        else:
            del _index[:]

        self._len += len(values)

    def insert(self, idx, val):
        """
        Insert the element *val* into the list at *idx*. Raises a ValueError if
        the *val* at *idx* would violate the sort order.
        """
        _maxes, _lists, _len = self._maxes, self._lists, self._len

        if idx < 0:
            idx += _len
        if idx < 0:
            idx = 0
        if idx > _len:
            idx = _len

        if not _maxes:
            # The idx must be zero by the inequalities above.
            _maxes.append(val)
            _lists.append([val])
            self._len = 1
            return

        if not idx:
            if val > _lists[0][0]:
                msg = '{0} not in sort order at index {1}'.format(repr(val), 0)
                raise ValueError(msg)
            else:
                _lists[0].insert(0, val)
                self._expand(0)
                self._len += 1
                return

        if idx == _len:
            pos = len(_lists) - 1
            if _lists[pos][-1] > val:
                msg = '{0} not in sort order at index {1}'.format(repr(val), _len)
                raise ValueError(msg)
            else:
                _lists[pos].append(val)
                _maxes[pos] = _lists[pos][-1]
                self._expand(pos)
                self._len += 1
                return

        pos, idx = self._pos(idx)
        idx_before = idx - 1
        if idx_before < 0:
            pos_before = pos - 1
            idx_before = len(_lists[pos_before]) - 1
        else:
            pos_before = pos

        before = _lists[pos_before][idx_before]
        if before <= val <= _lists[pos][idx]:
            _lists[pos].insert(idx, val)
            self._expand(pos)
            self._len += 1
        else:
            msg = '{0} not in sort order at index {1}'.format(repr(val), idx)
            raise ValueError(msg)

    def pop(self, idx=-1):
        """
        Remove and return item at *idx* (default last).  Raises IndexError if
        list is empty or index is out of range.  Negative indices are supported,
        as for slice indices.
        """
        if (idx < 0 and -idx > self._len) or (idx >= self._len):
            raise IndexError('pop index out of range')

        pos, idx = self._pos(idx)
        val = self._lists[pos][idx]
        self._delete(pos, idx)

        return val

    def index(self, val, start=None, stop=None):
        """
        Return the smallest *k* such that L[k] == val and i <= k < j`.  Raises
        ValueError if *val* is not present.  *stop* defaults to the end of the
        list. *start* defaults to the beginning. Negative indices are supported,
        as for slice indices.
        """
        _len, _maxes = self._len, self._maxes

        if not _maxes:
            raise ValueError('{0} is not in list'.format(repr(val)))

        if start is None:
            start = 0
        if start < 0:
            start += _len
        if start < 0:
            start = 0

        if stop is None:
            stop = _len
        if stop < 0:
            stop += _len
        if stop > _len:
            stop = _len

        if stop <= start:
            raise ValueError('{0} is not in list'.format(repr(val)))

        stop -= 1
        pos_left = bisect_left(_maxes, val)

        if pos_left == len(_maxes):
            raise ValueError('{0} is not in list'.format(repr(val)))

        _lists = self._lists
        idx_left = bisect_left(_lists[pos_left], val)

        if _lists[pos_left][idx_left] != val:
            raise ValueError('{0} is not in list'.format(repr(val)))

        left = self._loc(pos_left, idx_left)

        if start <= left:
            if left <= stop:
                return left
        else:
            right = self.bisect_right(val) - 1

            if start <= right:
                return start

        raise ValueError('{0} is not in list'.format(repr(val)))

    def as_list(self):
        """Very efficiently convert the SortedList to a list."""
        return reduce(iadd, self._lists, [])

    def __add__(self, that):
        """
        Return a new sorted list containing all the elements in *self* and
        *that*. Elements in *that* do not need to be properly ordered with
        respect to *self*.
        """
        values = self.as_list()
        values.extend(that)
        return self.__class__(values, load=self._load)

    def __iadd__(self, that):
        """
        Update *self* to include all values in *that*. Elements in *that* do not
        need to be properly ordered with respect to *self*.
        """
        self.update(that)
        return self

    def __mul__(self, that):
        """
        Return a new sorted list containing *that* shallow copies of each item
        in SortedList.
        """
        values = self.as_list() * that
        return self.__class__(values, load=self._load)

    def __imul__(self, that):
        """
        Increase the length of the list by appending *that* shallow copies of
        each item.
        """
        values = self.as_list() * that
        self.clear()
        self.update(values)
        return self

    def __eq__(self, that):
        """Compare two Sequences for equality."""
        return ((self._len == len(that))
                and all(lhs == rhs for lhs, rhs in zip(self, that)))

    def __ne__(self, that):
        """Compare two Sequences for inequality."""
        return ((self._len != len(that))
                or any(lhs != rhs for lhs, rhs in zip(self, that)))

    def __lt__(self, that):
        """Compare two Sequences for less than."""
        return ((self._len <= len(that))
                and all(lhs < rhs for lhs, rhs in zip(self, that)))

    def __le__(self, that):
        """Compare two Sequences for less than equal."""
        return ((self._len <= len(that))
                and all(lhs <= rhs for lhs, rhs in zip(self, that)))

    def __gt__(self, that):
        """Compare two Sequences for greater than."""
        return ((self._len >= len(that))
                and all(lhs > rhs for lhs, rhs in zip(self, that)))

    def __ge__(self, that):
        """Compare two Sequences for greater than equal."""
        return ((self._len >= len(that))
                and all(lhs >= rhs for lhs, rhs in zip(self, that)))

    @recursive_repr
    def __repr__(self):
        """Return string representation of SortedList."""
        temp = '{0}({1}, load={2})'
        return temp.format(
            self.__class__.__name__,
            repr(list(self)),
            repr(self._load)
        )

    def _check(self):
        try:
            # Check load parameters.

            assert self._load >= 4
            assert self._half == (self._load >> 1)
            assert self._twice == (self._load * 2)

            # Check empty sorted list case.

            if self._maxes == []:
                assert self._lists == []
                return

            assert len(self._maxes) > 0 and len(self._lists) > 0

            # Check all sublists are sorted.

            assert all(sublist[pos - 1] <= sublist[pos]
                       for sublist in self._lists
                       for pos in range(1, len(sublist)))

            # Check beginning/end of sublists are sorted.

            for pos in range(1, len(self._lists)):
                assert self._lists[pos - 1][-1] <= self._lists[pos][0]

            # Check length of _maxes and _lists match.

            assert len(self._maxes) == len(self._lists)

            # Check _maxes is a map of _lists.

            assert all(self._maxes[pos] == self._lists[pos][-1]
                       for pos in range(len(self._maxes)))

            # Check load level is less than _twice.

            assert all(len(sublist) <= self._twice for sublist in self._lists)

            # Check load level is greater than _half for all
            # but the last sublist.

            assert all(len(self._lists[pos]) >= self._half
                       for pos in range(0, len(self._lists) - 1))

            # Check length.

            assert self._len == sum(len(sublist) for sublist in self._lists)

            # Check index.

            if len(self._index):
                assert len(self._index) == self._offset + len(self._lists)
                assert self._len == self._index[0]

                def test_offset_pos(pos):
                    from_index = self._index[self._offset + pos]
                    return from_index == len(self._lists[pos])

                assert all(test_offset_pos(pos)
                           for pos in range(len(self._lists)))

                for pos in range(self._offset):
                    child = (pos << 1) + 1
                    if self._index[pos] == 0:
                        assert child >= len(self._index)
                    elif child + 1 == len(self._index):
                        assert self._index[pos] == self._index[child]
                    else:
                        child_sum = self._index[child] + self._index[child + 1]
                        assert self._index[pos] == child_sum

        except:
            import sys
            import traceback

            traceback.print_exc(file=sys.stdout)

            print('len', self._len)
            print('load', self._load, self._half, self._twice)
            print('offset', self._offset)
            print('len_index', len(self._index))
            print('index', self._index)
            print('len_maxes', len(self._maxes))
            print('maxes', self._maxes)
            print('len_lists', len(self._lists))
            print('lists', self._lists)

            raise
