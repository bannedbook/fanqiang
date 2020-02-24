# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#ifndef OPENSSL_NO_ECDH
#include <openssl/ecdh.h>
#endif
"""

TYPES = """
static const int Cryptography_HAS_ECDH;
"""

FUNCTIONS = """
"""

MACROS = """
int ECDH_compute_key(void *, size_t, const EC_POINT *, EC_KEY *,
                     void *(*)(const void *, size_t, void *, size_t *));

int ECDH_get_ex_new_index(long, void *, CRYPTO_EX_new *, CRYPTO_EX_dup *,
                          CRYPTO_EX_free *);

int ECDH_set_ex_data(EC_KEY *, int, void *);

void *ECDH_get_ex_data(EC_KEY *, int);
"""

CUSTOMIZATIONS = """
#ifdef OPENSSL_NO_ECDH
static const long Cryptography_HAS_ECDH = 0;

int (*ECDH_compute_key)(void *, size_t, const EC_POINT *, EC_KEY *,
                        void *(*)(const void *, size_t, void *,
                        size_t *)) = NULL;

int (*ECDH_get_ex_new_index)(long, void *, CRYPTO_EX_new *, CRYPTO_EX_dup *,
                             CRYPTO_EX_free *) = NULL;

int (*ECDH_set_ex_data)(EC_KEY *, int, void *) = NULL;

void *(*ECDH_get_ex_data)(EC_KEY *, int) = NULL;

#else
static const long Cryptography_HAS_ECDH = 1;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_ECDH": [
        "ECDH_compute_key",
        "ECDH_get_ex_new_index",
        "ECDH_set_ex_data",
        "ECDH_get_ex_data",
    ],
}
