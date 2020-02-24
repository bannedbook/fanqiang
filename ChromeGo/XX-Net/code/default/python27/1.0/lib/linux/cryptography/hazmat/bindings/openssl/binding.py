# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import os
import sys
import threading

from cryptography.hazmat.bindings.utils import (
    build_ffi_for_binding, load_library_for_binding,
)


_OSX_PRE_INCLUDE = """
#ifdef __APPLE__
#include <AvailabilityMacros.h>
#define __ORIG_DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER \
    DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#undef DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#endif
"""

_OSX_POST_INCLUDE = """
#ifdef __APPLE__
#undef DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER \
    __ORIG_DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#endif
"""


def _get_libraries(platform):
    # OpenSSL goes by a different library name on different operating systems.
    if platform != "win32":
        # In some circumstances, the order in which these libs are
        # specified on the linker command-line is significant;
        # libssl must come before libcrypto
        # (http://marc.info/?l=openssl-users&m=135361825921871)
        return ["ssl", "crypto"]
    else:
        link_type = os.environ.get("PYCA_WINDOWS_LINK_TYPE", "static")
        return _get_windows_libraries(link_type)


def _get_windows_libraries(link_type):
    if link_type == "dynamic":
        return ["libeay32", "ssleay32", "advapi32"]
    elif link_type == "static" or link_type == "":
        return ["libeay32mt", "ssleay32mt", "advapi32",
                "crypt32", "gdi32", "user32", "ws2_32"]
    else:
        raise ValueError(
            "PYCA_WINDOWS_LINK_TYPE must be 'static' or 'dynamic'"
        )


class Binding(object):
    """
    OpenSSL API wrapper.
    """
    _module_prefix = "cryptography.hazmat.bindings.openssl."
    _modules = [
        "aes",
        "asn1",
        "bignum",
        "bio",
        "cmac",
        "cms",
        "conf",
        "crypto",
        "dh",
        "dsa",
        "ec",
        "ecdh",
        "ecdsa",
        "engine",
        "err",
        "evp",
        "hmac",
        "nid",
        "objects",
        "opensslv",
        "osrandom_engine",
        "pem",
        "pkcs7",
        "pkcs12",
        "rand",
        "rsa",
        "ssl",
        "x509",
        "x509name",
        "x509v3",
        "x509_vfy"
    ]

    _locks = None
    _lock_cb_handle = None
    _init_lock = threading.Lock()
    _lock_init_lock = threading.Lock()

    ffi = build_ffi_for_binding(
        module_prefix=_module_prefix,
        modules=_modules,
        pre_include=_OSX_PRE_INCLUDE,
        post_include=_OSX_POST_INCLUDE,
        libraries=_get_libraries(sys.platform)
    )
    lib = None

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
                    cls._module_prefix,
                    cls._modules,
                )

                res = cls.lib.Cryptography_add_osrandom_engine()
                assert res != 0

    @classmethod
    def init_static_locks(cls):
        with cls._lock_init_lock:
            cls._ensure_ffi_initialized()

            if not cls._lock_cb_handle:
                cls._lock_cb_handle = cls.ffi.callback(
                    "void(int, int, const char *, int)",
                    cls._lock_cb
                )

            # Use Python's implementation if available, importing _ssl triggers
            # the setup for this.
            __import__("_ssl")

            if cls.lib.CRYPTO_get_locking_callback() != cls.ffi.NULL:
                return

            # If nothing else has setup a locking callback already, we set up
            # our own
            num_locks = cls.lib.CRYPTO_num_locks()
            cls._locks = [threading.Lock() for n in range(num_locks)]

            cls.lib.CRYPTO_set_locking_callback(cls._lock_cb_handle)

    @classmethod
    def _lock_cb(cls, mode, n, file, line):
        lock = cls._locks[n]

        if mode & cls.lib.CRYPTO_LOCK:
            lock.acquire()
        elif mode & cls.lib.CRYPTO_UNLOCK:
            lock.release()
        else:
            raise RuntimeError(
                "Unknown lock mode {0}: lock={1}, file={2}, line={3}.".format(
                    mode, n, file, line
                )
            )
