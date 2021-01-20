#
# This file is part of pyasn1 software.
#
# Copyright (c) 2005-2018, Ilya Etingof <etingof@gmail.com>
# License: http://snmplabs.com/pyasn1/license.html
#

__all__ = ['OpenType']


class OpenType(object):
    """Create ASN.1 type map indexed by a value

    The *DefinedBy* object models the ASN.1 *DEFINED BY* clause which maps
    values to ASN.1 types in the context of the ASN.1 SEQUENCE/SET type.

    OpenType objects are duck-type a read-only Python :class:`dict` objects,
    however the passed `typeMap` is stored by reference.

    Parameters
    ----------
    name: :py:class:`str`
        Field name

    typeMap: :py:class:`dict`
        A map of value->ASN.1 type. It's stored by reference and can be
        mutated later to register new mappings.

    Examples
    --------
    .. code-block:: python

        openType = OpenType(
            'id',
            {1: Integer(),
             2: OctetString()}
        )
        Sequence(
            componentType=NamedTypes(
                NamedType('id', Integer()),
                NamedType('blob', Any(), openType=openType)
            )
        )
    """

    def __init__(self, name, typeMap=None):
        self.__name = name
        if typeMap is None:
            self.__typeMap = {}
        else:
            self.__typeMap = typeMap

    @property
    def name(self):
        return self.__name

    # Python dict protocol

    def values(self):
        return self.__typeMap.values()

    def keys(self):
        return self.__typeMap.keys()

    def items(self):
        return self.__typeMap.items()

    def __contains__(self, key):
        return key in self.__typeMap

    def __getitem__(self, key):
        return self.__typeMap[key]

    def __iter__(self):
        return iter(self.__typeMap)
