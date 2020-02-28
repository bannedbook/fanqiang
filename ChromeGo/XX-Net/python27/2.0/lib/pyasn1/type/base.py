#
# This file is part of pyasn1 software.
#
# Copyright (c) 2005-2018, Ilya Etingof <etingof@gmail.com>
# License: http://snmplabs.com/pyasn1/license.html
#
import sys

from pyasn1 import error
from pyasn1.compat import calling
from pyasn1.type import constraint
from pyasn1.type import tag
from pyasn1.type import tagmap

__all__ = ['Asn1Item', 'Asn1ItemBase', 'AbstractSimpleAsn1Item', 'AbstractConstructedAsn1Item']


class Asn1Item(object):
    @classmethod
    def getTypeId(cls, increment=1):
        try:
            Asn1Item._typeCounter += increment
        except AttributeError:
            Asn1Item._typeCounter = increment
        return Asn1Item._typeCounter


class Asn1ItemBase(Asn1Item):
    #: Set or return a :py:class:`~pyasn1.type.tag.TagSet` object representing
    #: ASN.1 tag(s) associated with |ASN.1| type.
    tagSet = tag.TagSet()

    #: Default :py:class:`~pyasn1.type.constraint.ConstraintsIntersection`
    #: object imposing constraints on initialization values.
    subtypeSpec = constraint.ConstraintsIntersection()

    # Disambiguation ASN.1 types identification
    typeId = None

    def __init__(self, **kwargs):
        readOnly = {
            'tagSet': self.tagSet,
            'subtypeSpec': self.subtypeSpec
        }

        readOnly.update(kwargs)

        self.__dict__.update(readOnly)

        self._readOnly = readOnly

    def __setattr__(self, name, value):
        if name[0] != '_' and name in self._readOnly:
            raise error.PyAsn1Error('read-only instance attribute "%s"' % name)

        self.__dict__[name] = value

    def __str__(self):
        return self.prettyPrint()

    @property
    def readOnly(self):
        return self._readOnly

    @property
    def effectiveTagSet(self):
        """For |ASN.1| type is equivalent to *tagSet*
        """
        return self.tagSet  # used by untagged types

    @property
    def tagMap(self):
        """Return a :class:`~pyasn1.type.tagmap.TagMap` object mapping ASN.1 tags to ASN.1 objects within callee object.
        """
        return tagmap.TagMap({self.tagSet: self})

    def isSameTypeWith(self, other, matchTags=True, matchConstraints=True):
        """Examine |ASN.1| type for equality with other ASN.1 type.

        ASN.1 tags (:py:mod:`~pyasn1.type.tag`) and constraints
        (:py:mod:`~pyasn1.type.constraint`) are examined when carrying
        out ASN.1 types comparison.

        Python class inheritance relationship is NOT considered.

        Parameters
        ----------
        other: a pyasn1 type object
            Class instance representing ASN.1 type.

        Returns
        -------
        : :class:`bool`
            :class:`True` if *other* is |ASN.1| type,
            :class:`False` otherwise.
        """
        return (self is other or
                (not matchTags or self.tagSet == other.tagSet) and
                (not matchConstraints or self.subtypeSpec == other.subtypeSpec))

    def isSuperTypeOf(self, other, matchTags=True, matchConstraints=True):
        """Examine |ASN.1| type for subtype relationship with other ASN.1 type.

        ASN.1 tags (:py:mod:`~pyasn1.type.tag`) and constraints
        (:py:mod:`~pyasn1.type.constraint`) are examined when carrying
        out ASN.1 types comparison.

        Python class inheritance relationship is NOT considered.

        Parameters
        ----------
            other: a pyasn1 type object
                Class instance representing ASN.1 type.

        Returns
        -------
            : :class:`bool`
                :class:`True` if *other* is a subtype of |ASN.1| type,
                :class:`False` otherwise.
        """
        return (not matchTags or
                (self.tagSet.isSuperTagSetOf(other.tagSet)) and
                 (not matchConstraints or self.subtypeSpec.isSuperTypeOf(other.subtypeSpec)))

    @staticmethod
    def isNoValue(*values):
        for value in values:
            if value is not noValue:
                return False
        return True

    def prettyPrint(self, scope=0):
        raise NotImplementedError()

    # backward compatibility

    def getTagSet(self):
        return self.tagSet

    def getEffectiveTagSet(self):
        return self.effectiveTagSet

    def getTagMap(self):
        return self.tagMap

    def getSubtypeSpec(self):
        return self.subtypeSpec

    def hasValue(self):
        return self.isValue


class NoValue(object):
    """Create a singleton instance of NoValue class.

    The *NoValue* sentinel object represents an instance of ASN.1 schema
    object as opposed to ASN.1 value object.

    Only ASN.1 schema-related operations can be performed on ASN.1
    schema objects.

    Warning
    -------
    Any operation attempted on the *noValue* object will raise the
    *PyAsn1Error* exception.
    """
    skipMethods = set(
        ('__slots__',
         # attributes
         '__getattribute__',
         '__getattr__',
         '__setattr__',
         '__delattr__',
         # class instance
         '__class__',
         '__init__',
         '__del__',
         '__new__',
         '__repr__',
         '__qualname__',
         '__objclass__',
         'im_class',
         '__sizeof__',
         # pickle protocol
         '__reduce__',
         '__reduce_ex__',
         '__getnewargs__',
         '__getinitargs__',
         '__getstate__',
         '__setstate__')
    )

    _instance = None

    def __new__(cls):
        if cls._instance is None:
            def getPlug(name):
                def plug(self, *args, **kw):
                    raise error.PyAsn1Error('Attempted "%s" operation on ASN.1 schema object' % name)
                return plug

            op_names = [name
                        for typ in (str, int, list, dict)
                        for name in dir(typ)
                        if (name not in cls.skipMethods and
                            name.startswith('__') and
                            name.endswith('__') and
                            calling.callable(getattr(typ, name)))]

            for name in set(op_names):
                setattr(cls, name, getPlug(name))

            cls._instance = object.__new__(cls)

        return cls._instance

    def __getattr__(self, attr):
        if attr in self.skipMethods:
            raise AttributeError('Attribute %s not present' % attr)

        raise error.PyAsn1Error('Attempted "%s" operation on ASN.1 schema object' % attr)

    def __repr__(self):
        return '<%s object at 0x%x>' % (self.__class__.__name__, id(self))


noValue = NoValue()


# Base class for "simple" ASN.1 objects. These are immutable.
class AbstractSimpleAsn1Item(Asn1ItemBase):
    #: Default payload value
    defaultValue = noValue

    def __init__(self, value=noValue, **kwargs):
        Asn1ItemBase.__init__(self, **kwargs)
        if value is noValue:
            value = self.defaultValue
        else:
            value = self.prettyIn(value)
            try:
                self.subtypeSpec(value)

            except error.PyAsn1Error:
                exType, exValue, exTb = sys.exc_info()
                raise exType('%s at %s' % (exValue, self.__class__.__name__))

        self._value = value

    def __repr__(self):
        representation = '%s %s object at 0x%x' % (
            self.__class__.__name__, self.isValue and 'value' or 'schema', id(self)
        )

        for attr, value in self.readOnly.items():
            if value:
                representation += ' %s %s' % (attr, value)

        if self.isValue:
            value = self.prettyPrint()
            if len(value) > 32:
                value = value[:16] + '...' + value[-16:]
            representation += ' payload [%s]' % value

        return '<%s>' % representation

    def __eq__(self, other):
        return self is other and True or self._value == other

    def __ne__(self, other):
        return self._value != other

    def __lt__(self, other):
        return self._value < other

    def __le__(self, other):
        return self._value <= other

    def __gt__(self, other):
        return self._value > other

    def __ge__(self, other):
        return self._value >= other

    if sys.version_info[0] <= 2:
        def __nonzero__(self):
            return self._value and True or False
    else:
        def __bool__(self):
            return self._value and True or False

    def __hash__(self):
        return hash(self._value)

    @property
    def isValue(self):
        """Indicate that |ASN.1| object represents ASN.1 value.

        If *isValue* is `False` then this object represents just ASN.1 schema.

        If *isValue* is `True` then, in addition to its ASN.1 schema features,
        this object can also be used like a Python built-in object (e.g. `int`,
        `str`, `dict` etc.).

        Returns
        -------
        : :class:`bool`
            :class:`False` if object represents just ASN.1 schema.
            :class:`True` if object represents ASN.1 schema and can be used as a normal value.

        Note
        ----
        There is an important distinction between PyASN1 schema and value objects.
        The PyASN1 schema objects can only participate in ASN.1 schema-related
        operations (e.g. defining or testing the structure of the data). Most
        obvious uses of ASN.1 schema is to guide serialisation codecs whilst
        encoding/decoding serialised ASN.1 contents.

        The PyASN1 value objects can **additionally** participate in many operations
        involving regular Python objects (e.g. arithmetic, comprehension etc).
        """
        return self._value is not noValue

    def clone(self, value=noValue, **kwargs):
        """Create a modified version of |ASN.1| schema or value object.

        The `clone()` method accepts the same set arguments as |ASN.1|
        class takes on instantiation except that all arguments
        of the `clone()` method are optional.

        Whatever arguments are supplied, they are used to create a copy
        of `self` taking precedence over the ones used to instantiate `self`.

        Note
        ----
        Due to the immutable nature of the |ASN.1| object, if no arguments
        are supplied, no new |ASN.1| object will be created and `self` will
        be returned instead.
        """
        if value is noValue:
            if not kwargs:
                return self

            value = self._value

        initilaizers = self.readOnly.copy()
        initilaizers.update(kwargs)

        return self.__class__(value, **initilaizers)

    def subtype(self, value=noValue, **kwargs):
        """Create a specialization of |ASN.1| schema or value object.

        The subtype relationship between ASN.1 types has no correlation with
        subtype relationship between Python types. ASN.1 type is mainly identified
        by its tag(s) (:py:class:`~pyasn1.type.tag.TagSet`) and value range
        constraints (:py:class:`~pyasn1.type.constraint.ConstraintsIntersection`).
        These ASN.1 type properties are implemented as |ASN.1| attributes.  

        The `subtype()` method accepts the same set arguments as |ASN.1|
        class takes on instantiation except that all parameters
        of the `subtype()` method are optional.

        With the exception of the arguments described below, the rest of
        supplied arguments they are used to create a copy of `self` taking
        precedence over the ones used to instantiate `self`.

        The following arguments to `subtype()` create a ASN.1 subtype out of
        |ASN.1| type:

        Other Parameters
        ----------------
        implicitTag: :py:class:`~pyasn1.type.tag.Tag`
            Implicitly apply given ASN.1 tag object to `self`'s
            :py:class:`~pyasn1.type.tag.TagSet`, then use the result as
            new object's ASN.1 tag(s).

        explicitTag: :py:class:`~pyasn1.type.tag.Tag`
            Explicitly apply given ASN.1 tag object to `self`'s
            :py:class:`~pyasn1.type.tag.TagSet`, then use the result as
            new object's ASN.1 tag(s).

        subtypeSpec: :py:class:`~pyasn1.type.constraint.ConstraintsIntersection`
            Add ASN.1 constraints object to one of the `self`'s, then
            use the result as new object's ASN.1 constraints.

        Returns
        -------
        :
            new instance of |ASN.1| schema or value object

        Note
        ----
        Due to the immutable nature of the |ASN.1| object, if no arguments
        are supplied, no new |ASN.1| object will be created and `self` will
        be returned instead.
        """
        if value is noValue:
            if not kwargs:
                return self

            value = self._value

        initializers = self.readOnly.copy()

        implicitTag = kwargs.pop('implicitTag', None)
        if implicitTag is not None:
            initializers['tagSet'] = self.tagSet.tagImplicitly(implicitTag)

        explicitTag = kwargs.pop('explicitTag', None)
        if explicitTag is not None:
            initializers['tagSet'] = self.tagSet.tagExplicitly(explicitTag)

        for arg, option in kwargs.items():
            initializers[arg] += option

        return self.__class__(value, **initializers)

    def prettyIn(self, value):
        return value

    def prettyOut(self, value):
        return str(value)

    def prettyPrint(self, scope=0):
        return self.prettyOut(self._value)

    # noinspection PyUnusedLocal
    def prettyPrintType(self, scope=0):
        return '%s -> %s' % (self.tagSet, self.__class__.__name__)

#
# Constructed types:
# * There are five of them: Sequence, SequenceOf/SetOf, Set and Choice
# * ASN1 types and values are represened by Python class instances
# * Value initialization is made for defaulted components only
# * Primary method of component addressing is by-position. Data model for base
#   type is Python sequence. Additional type-specific addressing methods
#   may be implemented for particular types.
# * SequenceOf and SetOf types do not implement any additional methods
# * Sequence, Set and Choice types also implement by-identifier addressing
# * Sequence, Set and Choice types also implement by-asn1-type (tag) addressing
# * Sequence and Set types may include optional and defaulted
#   components
# * Constructed types hold a reference to component types used for value
#   verification and ordering.
# * Component type is a scalar type for SequenceOf/SetOf types and a list
#   of types for Sequence/Set/Choice.
#


class AbstractConstructedAsn1Item(Asn1ItemBase):

    #: If `True`, requires exact component type matching,
    #: otherwise subtype relation is only enforced
    strictConstraints = False

    componentType = None
    sizeSpec = None

    def __init__(self, **kwargs):
        readOnly = {
            'componentType': self.componentType,
            'sizeSpec': self.sizeSpec
        }
        readOnly.update(kwargs)

        Asn1ItemBase.__init__(self, **readOnly)

        self._componentValues = []

    def __repr__(self):
        representation = '%s %s object at 0x%x' % (
            self.__class__.__name__, self.isValue and 'value' or 'schema', id(self)
        )

        for attr, value in self.readOnly.items():
            if value is not noValue:
                representation += ' %s=%r' % (attr, value)

        if self.isValue and self._componentValues:
            representation += ' payload [%s]' % ', '.join([repr(x) for x in self._componentValues])

        return '<%s>' % representation

    def __eq__(self, other):
        return self is other and True or self._componentValues == other

    def __ne__(self, other):
        return self._componentValues != other

    def __lt__(self, other):
        return self._componentValues < other

    def __le__(self, other):
        return self._componentValues <= other

    def __gt__(self, other):
        return self._componentValues > other

    def __ge__(self, other):
        return self._componentValues >= other

    if sys.version_info[0] <= 2:
        def __nonzero__(self):
            return self._componentValues and True or False
    else:
        def __bool__(self):
            return self._componentValues and True or False

    def __len__(self):
        return len(self._componentValues)

    def _cloneComponentValues(self, myClone, cloneValueFlag):
        pass

    def clone(self, **kwargs):
        """Create a modified version of |ASN.1| schema object.

        The `clone()` method accepts the same set arguments as |ASN.1|
        class takes on instantiation except that all arguments
        of the `clone()` method are optional.

        Whatever arguments are supplied, they are used to create a copy
        of `self` taking precedence over the ones used to instantiate `self`.

        Possible values of `self` are never copied over thus `clone()` can
        only create a new schema object.

        Returns
        -------
        :
            new instance of |ASN.1| type/value

        Note
        ----
        Due to the mutable nature of the |ASN.1| object, even if no arguments
        are supplied, new |ASN.1| object will always be created as a shallow
        copy of `self`.
        """
        cloneValueFlag = kwargs.pop('cloneValueFlag', False)

        initilaizers = self.readOnly.copy()
        initilaizers.update(kwargs)

        clone = self.__class__(**initilaizers)

        if cloneValueFlag:
            self._cloneComponentValues(clone, cloneValueFlag)

        return clone

    def subtype(self, **kwargs):
        """Create a specialization of |ASN.1| schema object.

        The `subtype()` method accepts the same set arguments as |ASN.1|
        class takes on instantiation except that all parameters
        of the `subtype()` method are optional.

        With the exception of the arguments described below, the rest of
        supplied arguments they are used to create a copy of `self` taking
        precedence over the ones used to instantiate `self`.

        The following arguments to `subtype()` create a ASN.1 subtype out of
        |ASN.1| type.

        Other Parameters
        ----------------
        implicitTag: :py:class:`~pyasn1.type.tag.Tag`
            Implicitly apply given ASN.1 tag object to `self`'s
            :py:class:`~pyasn1.type.tag.TagSet`, then use the result as
            new object's ASN.1 tag(s).

        explicitTag: :py:class:`~pyasn1.type.tag.Tag`
            Explicitly apply given ASN.1 tag object to `self`'s
            :py:class:`~pyasn1.type.tag.TagSet`, then use the result as
            new object's ASN.1 tag(s).

        subtypeSpec: :py:class:`~pyasn1.type.constraint.ConstraintsIntersection`
            Add ASN.1 constraints object to one of the `self`'s, then
            use the result as new object's ASN.1 constraints.


        Returns
        -------
        :
            new instance of |ASN.1| type/value

        Note
        ----
        Due to the immutable nature of the |ASN.1| object, if no arguments
        are supplied, no new |ASN.1| object will be created and `self` will
        be returned instead.
        """

        initializers = self.readOnly.copy()

        cloneValueFlag = kwargs.pop('cloneValueFlag', False)

        implicitTag = kwargs.pop('implicitTag', None)
        if implicitTag is not None:
            initializers['tagSet'] = self.tagSet.tagImplicitly(implicitTag)

        explicitTag = kwargs.pop('explicitTag', None)
        if explicitTag is not None:
            initializers['tagSet'] = self.tagSet.tagExplicitly(explicitTag)

        for arg, option in kwargs.items():
            initializers[arg] += option

        clone = self.__class__(**initializers)

        if cloneValueFlag:
            self._cloneComponentValues(clone, cloneValueFlag)

        return clone

    def verifySizeSpec(self):
        self.sizeSpec(self)

    def getComponentByPosition(self, idx):
        raise error.PyAsn1Error('Method not implemented')

    def setComponentByPosition(self, idx, value, verifyConstraints=True):
        raise error.PyAsn1Error('Method not implemented')

    def setComponents(self, *args, **kwargs):
        for idx, value in enumerate(args):
            self[idx] = value
        for k in kwargs:
            self[k] = kwargs[k]
        return self

    def clear(self):
        self._componentValues = []

    # backward compatibility

    def setDefaultComponents(self):
        pass

    def getComponentType(self):
        return self.componentType
