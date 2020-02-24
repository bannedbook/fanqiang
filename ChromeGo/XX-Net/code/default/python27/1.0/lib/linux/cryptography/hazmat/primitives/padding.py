# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc

import os

import six

from cryptography import utils
from cryptography.exceptions import AlreadyFinalized
from cryptography.hazmat.bindings.utils import LazyLibrary, build_ffi


with open(os.path.join(os.path.dirname(__file__), "src/padding.h")) as f:
    TYPES = f.read()

with open(os.path.join(os.path.dirname(__file__), "src/padding.c")) as f:
    FUNCTIONS = f.read()


_ffi = build_ffi(cdef_source=TYPES, verify_source=FUNCTIONS)
_lib = LazyLibrary(_ffi)


@six.add_metaclass(abc.ABCMeta)
class PaddingContext(object):
    @abc.abstractmethod
    def update(self, data):
        """
        Pads the provided bytes and returns any available data as bytes.
        """

    @abc.abstractmethod
    def finalize(self):
        """
        Finalize the padding, returns bytes.
        """


class PKCS7(object):
    def __init__(self, block_size):
        if not (0 <= block_size < 256):
            raise ValueError("block_size must be in range(0, 256).")

        if block_size % 8 != 0:
            raise ValueError("block_size must be a multiple of 8.")

        self.block_size = block_size

    def padder(self):
        return _PKCS7PaddingContext(self.block_size)

    def unpadder(self):
        return _PKCS7UnpaddingContext(self.block_size)


@utils.register_interface(PaddingContext)
class _PKCS7PaddingContext(object):
    def __init__(self, block_size):
        self.block_size = block_size
        # TODO: more copies than necessary, we should use zero-buffer (#193)
        self._buffer = b""

    def update(self, data):
        if self._buffer is None:
            raise AlreadyFinalized("Context was already finalized.")

        if not isinstance(data, bytes):
            raise TypeError("data must be bytes.")

        self._buffer += data

        finished_blocks = len(self._buffer) // (self.block_size // 8)

        result = self._buffer[:finished_blocks * (self.block_size // 8)]
        self._buffer = self._buffer[finished_blocks * (self.block_size // 8):]

        return result

    def finalize(self):
        if self._buffer is None:
            raise AlreadyFinalized("Context was already finalized.")

        pad_size = self.block_size // 8 - len(self._buffer)
        result = self._buffer + six.int2byte(pad_size) * pad_size
        self._buffer = None
        return result


@utils.register_interface(PaddingContext)
class _PKCS7UnpaddingContext(object):
    def __init__(self, block_size):
        self.block_size = block_size
        # TODO: more copies than necessary, we should use zero-buffer (#193)
        self._buffer = b""

    def update(self, data):
        if self._buffer is None:
            raise AlreadyFinalized("Context was already finalized.")

        if not isinstance(data, bytes):
            raise TypeError("data must be bytes.")

        self._buffer += data

        finished_blocks = max(
            len(self._buffer) // (self.block_size // 8) - 1,
            0
        )

        result = self._buffer[:finished_blocks * (self.block_size // 8)]
        self._buffer = self._buffer[finished_blocks * (self.block_size // 8):]

        return result

    def finalize(self):
        if self._buffer is None:
            raise AlreadyFinalized("Context was already finalized.")

        if len(self._buffer) != self.block_size // 8:
            raise ValueError("Invalid padding bytes.")

        valid = _lib.Cryptography_check_pkcs7_padding(
            self._buffer, self.block_size // 8
        )

        if not valid:
            raise ValueError("Invalid padding bytes.")

        pad_size = six.indexbytes(self._buffer, -1)
        res = self._buffer[:-pad_size]
        self._buffer = None
        return res
