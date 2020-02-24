# -*- coding: utf-8 -*-
"""
hyper/common/decoder
~~~~~~~~~~~~~~~~~~~~

Contains hyper's code for handling compressed bodies.
"""
import zlib


class DeflateDecoder(object):
    """
    This is a decoding object that wraps ``zlib`` and is used for decoding
    deflated content.

    This rationale for the existence of this object is pretty unpleasant.
    The HTTP RFC specifies that 'deflate' is a valid content encoding. However,
    the spec _meant_ the zlib encoding form. Unfortunately, people who didn't
    read the RFC very carefully actually implemented a different form of
    'deflate'. Insanely, ``zlib`` handles them using two wbits values. This is
    such a mess it's hard to adequately articulate.

    This class was lovingly borrowed from the excellent urllib3 library under
    license: see NOTICES. If you ever see @shazow, you should probably buy him
    a drink or something.
    """
    def __init__(self):
        self._first_try = True
        self._data = b''
        self._obj = zlib.decompressobj(zlib.MAX_WBITS)

    def __getattr__(self, name):
        return getattr(self._obj, name)

    def decompress(self, data):
        if not self._first_try:
            return self._obj.decompress(data)

        self._data += data
        try:
            return self._obj.decompress(data)
        except zlib.error:
            self._first_try = False
            self._obj = zlib.decompressobj(-zlib.MAX_WBITS)
            try:
                return self.decompress(self._data)
            finally:
                self._data = None
