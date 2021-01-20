#
# This file is part of pyasn1 software.
#
# Copyright (c) 2005-2018, Ilya Etingof <etingof@gmail.com>
# License: http://snmplabs.com/pyasn1/license.html
#
# Original concept and code by Mike C. Fletcher.
#
import sys

from pyasn1.type import error

__all__ = ['SingleValueConstraint', 'ContainedSubtypeConstraint',
           'ValueRangeConstraint', 'ValueSizeConstraint',
           'PermittedAlphabetConstraint', 'InnerTypeConstraint',
           'ConstraintsExclusion', 'ConstraintsIntersection',
           'ConstraintsUnion']


class AbstractConstraint(object):

    def __init__(self, *values):
        self._valueMap = set()
        self._setValues(values)
        self.__hash = hash((self.__class__.__name__, self._values))

    def __call__(self, value, idx=None):
        if not self._values:
            return

        try:
            self._testValue(value, idx)

        except error.ValueConstraintError:
            raise error.ValueConstraintError(
                '%s failed at: %r' % (self, sys.exc_info()[1])
            )

    def __repr__(self):
        representation = '%s object at 0x%x' % (self.__class__.__name__, id(self))

        if self._values:
            representation += ' consts %s' % ', '.join([repr(x) for x in self._values])

        return '<%s>' % representation

    def __eq__(self, other):
        return self is other and True or self._values == other

    def __ne__(self, other):
        return self._values != other

    def __lt__(self, other):
        return self._values < other

    def __le__(self, other):
        return self._values <= other

    def __gt__(self, other):
        return self._values > other

    def __ge__(self, other):
        return self._values >= other

    if sys.version_info[0] <= 2:
        def __nonzero__(self):
            return self._values and True or False
    else:
        def __bool__(self):
            return self._values and True or False

    def __hash__(self):
        return self.__hash

    def _setValues(self, values):
        self._values = values

    def _testValue(self, value, idx):
        raise error.ValueConstraintError(value)

    # Constraints derivation logic
    def getValueMap(self):
        return self._valueMap

    def isSuperTypeOf(self, otherConstraint):
        # TODO: fix possible comparison of set vs scalars here
        return (otherConstraint is self or
                not self._values or
                otherConstraint == self or
                self in otherConstraint.getValueMap())

    def isSubTypeOf(self, otherConstraint):
        return (otherConstraint is self or
                not self or
                otherConstraint == self or
                otherConstraint in self._valueMap)


class SingleValueConstraint(AbstractConstraint):
    """Create a SingleValueConstraint object.

    The SingleValueConstraint satisfies any value that
    is present in the set of permitted values.

    The SingleValueConstraint object can be applied to
    any ASN.1 type.

    Parameters
    ----------
    \*values: :class:`int`
        Full set of values permitted by this constraint object.

    Examples
    --------
    .. code-block:: python

        class DivisorOfSix(Integer):
            '''
            ASN.1 specification:

            Divisor-Of-6 ::= INTEGER (1 | 2 | 3 | 6)
            '''
            subtypeSpec = SingleValueConstraint(1, 2, 3, 6)

        # this will succeed
        divisor_of_six = DivisorOfSix(1)

        # this will raise ValueConstraintError
        divisor_of_six = DivisorOfSix(7)
    """
    def _setValues(self, values):
        self._values = values
        self._set = set(values)

    def _testValue(self, value, idx):
        if value not in self._set:
            raise error.ValueConstraintError(value)


class ContainedSubtypeConstraint(AbstractConstraint):
    """Create a ContainedSubtypeConstraint object.

    The ContainedSubtypeConstraint satisfies any value that
    is present in the set of permitted values and also
    satisfies included constraints.

    The ContainedSubtypeConstraint object can be applied to
    any ASN.1 type.

    Parameters
    ----------
    \*values:
        Full set of values and constraint objects permitted
        by this constraint object.

    Examples
    --------
    .. code-block:: python

        class DivisorOfEighteen(Integer):
            '''
            ASN.1 specification:

            Divisors-of-18 ::= INTEGER (INCLUDES Divisors-of-6 | 9 | 18)
            '''
            subtypeSpec = ContainedSubtypeConstraint(
                SingleValueConstraint(1, 2, 3, 6), 9, 18
            )

        # this will succeed
        divisor_of_eighteen = DivisorOfEighteen(9)

        # this will raise ValueConstraintError
        divisor_of_eighteen = DivisorOfEighteen(10)
    """
    def _testValue(self, value, idx):
        for constraint in self._values:
            if isinstance(constraint, AbstractConstraint):
                constraint(value, idx)
            elif value not in self._set:
                raise error.ValueConstraintError(value)


class ValueRangeConstraint(AbstractConstraint):
    """Create a ValueRangeConstraint object.

    The ValueRangeConstraint satisfies any value that
    falls in the range of permitted values.

    The ValueRangeConstraint object can only be applied
    to :class:`~pyasn1.type.univ.Integer` and
    :class:`~pyasn1.type.univ.Real` types.

    Parameters
    ----------
    start: :class:`int`
        Minimum permitted value in the range (inclusive)

    end: :class:`int`
        Maximum permitted value in the range (inclusive)

    Examples
    --------
    .. code-block:: python

        class TeenAgeYears(Integer):
            '''
            ASN.1 specification:

            TeenAgeYears ::= INTEGER (13 .. 19)
            '''
            subtypeSpec = ValueRangeConstraint(13, 19)

        # this will succeed
        teen_year = TeenAgeYears(18)

        # this will raise ValueConstraintError
        teen_year = TeenAgeYears(20)
    """
    def _testValue(self, value, idx):
        if value < self.start or value > self.stop:
            raise error.ValueConstraintError(value)

    def _setValues(self, values):
        if len(values) != 2:
            raise error.PyAsn1Error(
                '%s: bad constraint values' % (self.__class__.__name__,)
            )
        self.start, self.stop = values
        if self.start > self.stop:
            raise error.PyAsn1Error(
                '%s: screwed constraint values (start > stop): %s > %s' % (
                    self.__class__.__name__,
                    self.start, self.stop
                )
            )
        AbstractConstraint._setValues(self, values)


class ValueSizeConstraint(ValueRangeConstraint):
    """Create a ValueSizeConstraint object.

    The ValueSizeConstraint satisfies any value for
    as long as its size falls within the range of
    permitted sizes.

    The ValueSizeConstraint object can be applied
    to :class:`~pyasn1.type.univ.BitString`,
    :class:`~pyasn1.type.univ.OctetString` (including
    all :ref:`character ASN.1 types <type.char>`),
    :class:`~pyasn1.type.univ.SequenceOf`
    and :class:`~pyasn1.type.univ.SetOf` types.

    Parameters
    ----------
    minimum: :class:`int`
        Minimum permitted size of the value (inclusive)

    maximum: :class:`int`
        Maximum permitted size of the value (inclusive)

    Examples
    --------
    .. code-block:: python

        class BaseballTeamRoster(SetOf):
            '''
            ASN.1 specification:

            BaseballTeamRoster ::= SET SIZE (1..25) OF PlayerNames
            '''
            componentType = PlayerNames()
            subtypeSpec = ValueSizeConstraint(1, 25)

        # this will succeed
        team = BaseballTeamRoster()
        team.extend(['Jan', 'Matej'])
        encode(team)

        # this will raise ValueConstraintError
        team = BaseballTeamRoster()
        team.extend(['Jan'] * 26)
        encode(team)

    Note
    ----
    Whenever ValueSizeConstraint is applied to mutable types
    (e.g. :class:`~pyasn1.type.univ.SequenceOf`,
    :class:`~pyasn1.type.univ.SetOf`), constraint
    validation only happens at the serialisation phase rather
    than schema instantiation phase (as it is with immutable
    types).
    """
    def _testValue(self, value, idx):
        valueSize = len(value)
        if valueSize < self.start or valueSize > self.stop:
            raise error.ValueConstraintError(value)


class PermittedAlphabetConstraint(SingleValueConstraint):
    """Create a PermittedAlphabetConstraint object.

    The PermittedAlphabetConstraint satisfies any character
    string for as long as all its characters are present in
    the set of permitted characters.

    The PermittedAlphabetConstraint object can only be applied
    to the :ref:`character ASN.1 types <type.char>` such as
    :class:`~pyasn1.type.char.IA5String`.

    Parameters
    ----------
    \*alphabet: :class:`str`
        Full set of characters permitted by this constraint object.

    Examples
    --------
    .. code-block:: python

        class BooleanValue(IA5String):
            '''
            ASN.1 specification:

            BooleanValue ::= IA5String (FROM ('T' | 'F'))
            '''
            subtypeSpec = PermittedAlphabetConstraint('T', 'F')

        # this will succeed
        truth = BooleanValue('T')
        truth = BooleanValue('TF')

        # this will raise ValueConstraintError
        garbage = BooleanValue('TAF')
    """
    def _setValues(self, values):
        self._values = values
        self._set = set(values)

    def _testValue(self, value, idx):
        if not self._set.issuperset(value):
            raise error.ValueConstraintError(value)


# This is a bit kludgy, meaning two op modes within a single constraint
class InnerTypeConstraint(AbstractConstraint):
    """Value must satisfy the type and presence constraints"""

    def _testValue(self, value, idx):
        if self.__singleTypeConstraint:
            self.__singleTypeConstraint(value)
        elif self.__multipleTypeConstraint:
            if idx not in self.__multipleTypeConstraint:
                raise error.ValueConstraintError(value)
            constraint, status = self.__multipleTypeConstraint[idx]
            if status == 'ABSENT':  # XXX presense is not checked!
                raise error.ValueConstraintError(value)
            constraint(value)

    def _setValues(self, values):
        self.__multipleTypeConstraint = {}
        self.__singleTypeConstraint = None
        for v in values:
            if isinstance(v, tuple):
                self.__multipleTypeConstraint[v[0]] = v[1], v[2]
            else:
                self.__singleTypeConstraint = v
        AbstractConstraint._setValues(self, values)


# Logic operations on constraints

class ConstraintsExclusion(AbstractConstraint):
    """Create a ConstraintsExclusion logic operator object.

    The ConstraintsExclusion logic operator succeeds when the
    value does *not* satisfy the operand constraint.

    The ConstraintsExclusion object can be applied to
    any constraint and logic operator object.

    Parameters
    ----------
    constraint:
        Constraint or logic operator object.

    Examples
    --------
    .. code-block:: python

        class Lipogramme(IA5STRING):
            '''
            ASN.1 specification:

            Lipogramme ::=
                IA5String (FROM (ALL EXCEPT ("e"|"E")))
            '''
            subtypeSpec = ConstraintsExclusion(
                PermittedAlphabetConstraint('e', 'E')
            )

        # this will succeed
        lipogramme = Lipogramme('A work of fiction?')

        # this will raise ValueConstraintError
        lipogramme = Lipogramme('Eel')

    Warning
    -------
    The above example involving PermittedAlphabetConstraint might
    not work due to the way how PermittedAlphabetConstraint works.
    The other constraints might work with ConstraintsExclusion
    though.
    """
    def _testValue(self, value, idx):
        try:
            self._values[0](value, idx)
        except error.ValueConstraintError:
            return
        else:
            raise error.ValueConstraintError(value)

    def _setValues(self, values):
        if len(values) != 1:
            raise error.PyAsn1Error('Single constraint expected')

        AbstractConstraint._setValues(self, values)


class AbstractConstraintSet(AbstractConstraint):

    def __getitem__(self, idx):
        return self._values[idx]

    def __iter__(self):
        return iter(self._values)

    def __add__(self, value):
        return self.__class__(*(self._values + (value,)))

    def __radd__(self, value):
        return self.__class__(*((value,) + self._values))

    def __len__(self):
        return len(self._values)

    # Constraints inclusion in sets

    def _setValues(self, values):
        self._values = values
        for constraint in values:
            if constraint:
                self._valueMap.add(constraint)
                self._valueMap.update(constraint.getValueMap())


class ConstraintsIntersection(AbstractConstraintSet):
    """Create a ConstraintsIntersection logic operator object.

    The ConstraintsIntersection logic operator only succeeds
    if *all* its operands succeed.

    The ConstraintsIntersection object can be applied to
    any constraint and logic operator objects.

    The ConstraintsIntersection object duck-types the immutable
    container object like Python :py:class:`tuple`.

    Parameters
    ----------
    \*constraints:
        Constraint or logic operator objects.

    Examples
    --------
    .. code-block:: python

        class CapitalAndSmall(IA5String):
            '''
            ASN.1 specification:

            CapitalAndSmall ::=
                IA5String (FROM ("A".."Z"|"a".."z"))
            '''
            subtypeSpec = ConstraintsIntersection(
                PermittedAlphabetConstraint('A', 'Z'),
                PermittedAlphabetConstraint('a', 'z')
            )

        # this will succeed
        capital_and_small = CapitalAndSmall('Hello')

        # this will raise ValueConstraintError
        capital_and_small = CapitalAndSmall('hello')
    """
    def _testValue(self, value, idx):
        for constraint in self._values:
            constraint(value, idx)


class ConstraintsUnion(AbstractConstraintSet):
    """Create a ConstraintsUnion logic operator object.

    The ConstraintsUnion logic operator only succeeds if
    *at least a single* operand succeeds.

    The ConstraintsUnion object can be applied to
    any constraint and logic operator objects.

    The ConstraintsUnion object duck-types the immutable
    container object like Python :py:class:`tuple`.

    Parameters
    ----------
    \*constraints:
        Constraint or logic operator objects.

    Examples
    --------
    .. code-block:: python

        class CapitalOrSmall(IA5String):
            '''
            ASN.1 specification:

            CapitalOrSmall ::=
                IA5String (FROM ("A".."Z") | FROM ("a".."z"))
            '''
            subtypeSpec = ConstraintsIntersection(
                PermittedAlphabetConstraint('A', 'Z'),
                PermittedAlphabetConstraint('a', 'z')
            )

        # this will succeed
        capital_or_small = CapitalAndSmall('Hello')

        # this will raise ValueConstraintError
        capital_or_small = CapitalOrSmall('hello!')
    """
    def _testValue(self, value, idx):
        for constraint in self._values:
            try:
                constraint(value, idx)
            except error.ValueConstraintError:
                pass
            else:
                return

        raise error.ValueConstraintError(
            'all of %s failed for "%s"' % (self._values, value)
        )

# TODO:
# refactor InnerTypeConstraint
# add tests for type check
# implement other constraint types
# make constraint validation easy to skip
