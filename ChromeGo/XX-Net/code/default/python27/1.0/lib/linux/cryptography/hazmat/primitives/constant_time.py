# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import hmac
import os

from cryptography.hazmat.bindings.utils import LazyLibrary, build_ffi


with open(os.path.join(os.path.dirname(__file__), "src/constant_time.h")) as f:
    TYPES = f.read()

with open(os.path.join(os.path.dirname(__file__), "src/constant_time.c")) as f:
    FUNCTIONS = f.read()


_ffi = build_ffi(cdef_source=TYPES, verify_source=FUNCTIONS)
_lib = LazyLibrary(_ffi)


if hasattr(hmac, "compare_digest"):
    def bytes_eq(a, b):
        if not isinstance(a, bytes) or not isinstance(b, bytes):
            raise TypeError("a and b must be bytes.")

        return hmac.compare_digest(a, b)

else:
    def bytes_eq(a, b):
        if not isinstance(a, bytes) or not isinstance(b, bytes):
            raise TypeError("a and b must be bytes.")

        return _lib.Cryptography_constant_time_bytes_eq(
            a, len(a), b, len(b)
        ) == 1
