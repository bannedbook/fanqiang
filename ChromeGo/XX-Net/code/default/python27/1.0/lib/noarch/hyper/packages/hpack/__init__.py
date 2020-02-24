# -*- coding: utf-8 -*-
"""
hpack
~~~~~

HTTP/2 header encoding for Python.
"""
from .hpack import Encoder, Decoder
from .struct import HeaderTuple, NeverIndexedHeaderTuple
from .exceptions import (
    HPACKError, HPACKDecodingError, InvalidTableIndex, OversizedHeaderListError
)

__all__ = [
    'Encoder', 'Decoder', 'HPACKError', 'HPACKDecodingError',
    'InvalidTableIndex', 'HeaderTuple', 'NeverIndexedHeaderTuple',
    'OversizedHeaderListError'
]

__version__ = '3.0.0'
