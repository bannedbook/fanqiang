# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import threading

from cryptography.hazmat.bindings.utils import (
    build_ffi_for_binding, load_library_for_binding,
)


class Binding(object):
    """
    CommonCrypto API wrapper.
    """
    _module_prefix = "cryptography.hazmat.bindings.commoncrypto."
    _modules = [
        "cf",
        "common_digest",
        "common_hmac",
        "common_key_derivation",
        "common_cryptor",
        "secimport",
        "secitem",
        "seckey",
        "seckeychain",
        "sectransform",
    ]

    ffi = build_ffi_for_binding(
        module_prefix=_module_prefix,
        modules=_modules,
        extra_link_args=[
            "-framework", "Security", "-framework", "CoreFoundation"
        ],
    )
    lib = None
    _init_lock = threading.Lock()

    def __init__(self):
        self._ensure_ffi_initialized()

    @classmethod
    def _ensure_ffi_initialized(cls):
        if cls.lib is not None:
            return

        with cls._init_lock:
            if cls.lib is None:
                cls.lib = load_library_for_binding(
                    cls.ffi,
                    module_prefix=cls._module_prefix,
                    modules=cls._modules,
                )
