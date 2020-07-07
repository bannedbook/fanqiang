#
# This file is part of pyasn1 software.
#
# Copyright (c) 2005-2018, Ilya Etingof <etingof@gmail.com>
# License: http://snmplabs.com/pyasn1/license.html
#


class PyAsn1Error(Exception):
    """Create pyasn1 exception object

    The `PyAsn1Error` exception represents generic, usually fatal, error.
    """


class ValueConstraintError(PyAsn1Error):
    """Create pyasn1 exception object

    The `ValueConstraintError` exception indicates an ASN.1 value
    constraint violation.
    """


class SubstrateUnderrunError(PyAsn1Error):
    """Create pyasn1 exception object

    The `SubstrateUnderrunError` exception indicates insufficient serialised
    data on input of a deserialisation routine.
    """
