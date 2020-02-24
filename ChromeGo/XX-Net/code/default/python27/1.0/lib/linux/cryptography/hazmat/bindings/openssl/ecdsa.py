# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#ifndef OPENSSL_NO_ECDSA
#include <openssl/ecdsa.h>
#endif
"""

TYPES = """
static const int Cryptography_HAS_ECDSA;

typedef struct {
    BIGNUM *r;
    BIGNUM *s;
} ECDSA_SIG;

typedef ... CRYPTO_EX_new;
typedef ... CRYPTO_EX_dup;
typedef ... CRYPTO_EX_free;
"""

FUNCTIONS = """
"""

MACROS = """
ECDSA_SIG *ECDSA_SIG_new();
void ECDSA_SIG_free(ECDSA_SIG *);
int i2d_ECDSA_SIG(const ECDSA_SIG *, unsigned char **);
ECDSA_SIG *d2i_ECDSA_SIG(ECDSA_SIG **s, const unsigned char **, long);
ECDSA_SIG *ECDSA_do_sign(const unsigned char *, int, EC_KEY *);
ECDSA_SIG *ECDSA_do_sign_ex(const unsigned char *, int, const BIGNUM *,
                            const BIGNUM *, EC_KEY *);
int ECDSA_do_verify(const unsigned char *, int, const ECDSA_SIG *, EC_KEY *);
int ECDSA_sign_setup(EC_KEY *, BN_CTX *, BIGNUM **, BIGNUM **);
int ECDSA_sign(int, const unsigned char *, int, unsigned char *,
               unsigned int *, EC_KEY *);
int ECDSA_sign_ex(int, const unsigned char *, int dgstlen, unsigned char *,
                  unsigned int *, const BIGNUM *, const BIGNUM *, EC_KEY *);
int ECDSA_verify(int, const unsigned char *, int, const unsigned char *, int,
                 EC_KEY *);
int ECDSA_size(const EC_KEY *);

const ECDSA_METHOD *ECDSA_OpenSSL();
void ECDSA_set_default_method(const ECDSA_METHOD *);
const ECDSA_METHOD *ECDSA_get_default_method();
int ECDSA_get_ex_new_index(long, void *, CRYPTO_EX_new *,
                           CRYPTO_EX_dup *, CRYPTO_EX_free *);
int ECDSA_set_method(EC_KEY *, const ECDSA_METHOD *);
int ECDSA_set_ex_data(EC_KEY *, int, void *);
void *ECDSA_get_ex_data(EC_KEY *, int);
"""

CUSTOMIZATIONS = """
#ifdef OPENSSL_NO_ECDSA
static const long Cryptography_HAS_ECDSA = 0;

typedef struct {
    BIGNUM *r;
    BIGNUM *s;
} ECDSA_SIG;

ECDSA_SIG* (*ECDSA_SIG_new)() = NULL;
void (*ECDSA_SIG_free)(ECDSA_SIG *) = NULL;
int (*i2d_ECDSA_SIG)(const ECDSA_SIG *, unsigned char **) = NULL;
ECDSA_SIG* (*d2i_ECDSA_SIG)(ECDSA_SIG **s, const unsigned char **,
                            long) = NULL;
ECDSA_SIG* (*ECDSA_do_sign)(const unsigned char *, int, EC_KEY *eckey) = NULL;
ECDSA_SIG* (*ECDSA_do_sign_ex)(const unsigned char *, int, const BIGNUM *,
                               const BIGNUM *, EC_KEY *) = NULL;
int (*ECDSA_do_verify)(const unsigned char *, int, const ECDSA_SIG *,
                       EC_KEY *) = NULL;
int (*ECDSA_sign_setup)(EC_KEY *, BN_CTX *, BIGNUM **, BIGNUM **) = NULL;
int (*ECDSA_sign)(int, const unsigned char *, int, unsigned char *,
                  unsigned int *, EC_KEY *) = NULL;
int (*ECDSA_sign_ex)(int, const unsigned char *, int dgstlen, unsigned char *,
                     unsigned int *, const BIGNUM *, const BIGNUM *,
                     EC_KEY *) = NULL;
int (*ECDSA_verify)(int, const unsigned char *, int, const unsigned char *,
                    int, EC_KEY *) = NULL;
int (*ECDSA_size)(const EC_KEY *) = NULL;

const ECDSA_METHOD* (*ECDSA_OpenSSL)() = NULL;
void (*ECDSA_set_default_method)(const ECDSA_METHOD *) = NULL;
const ECDSA_METHOD* (*ECDSA_get_default_method)() = NULL;
int (*ECDSA_set_method)(EC_KEY *, const ECDSA_METHOD *) = NULL;
int (*ECDSA_get_ex_new_index)(long, void *, CRYPTO_EX_new *,
                              CRYPTO_EX_dup *, CRYPTO_EX_free *) = NULL;
int (*ECDSA_set_ex_data)(EC_KEY *, int, void *) = NULL;
void* (*ECDSA_get_ex_data)(EC_KEY *, int) = NULL;
#else
static const long Cryptography_HAS_ECDSA = 1;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_ECDSA": [
        "ECDSA_SIG_new",
        "ECDSA_SIG_free",
        "i2d_ECDSA_SIG",
        "d2i_ECDSA_SIG",
        "ECDSA_do_sign",
        "ECDSA_do_sign_ex",
        "ECDSA_do_verify",
        "ECDSA_sign_setup",
        "ECDSA_sign",
        "ECDSA_sign_ex",
        "ECDSA_verify",
        "ECDSA_size",
        "ECDSA_OpenSSL",
        "ECDSA_set_default_method",
        "ECDSA_get_default_method",
        "ECDSA_set_method",
        "ECDSA_get_ex_new_index",
        "ECDSA_set_ex_data",
        "ECDSA_get_ex_data",
    ],
}
