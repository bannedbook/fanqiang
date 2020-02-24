# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/dsa.h>
"""

TYPES = """
typedef struct dsa_st {
    /* Prime number (public) */
    BIGNUM *p;
    /* Subprime (160-bit, q | p-1, public) */
    BIGNUM *q;
    /* Generator of subgroup (public) */
    BIGNUM *g;
    /* Private key x */
    BIGNUM *priv_key;
    /* Public key y = g^x */
    BIGNUM *pub_key;
    ...;
} DSA;
typedef struct {
    BIGNUM *r;
    BIGNUM *s;
} DSA_SIG;
"""

FUNCTIONS = """
DSA *DSA_generate_parameters(int, unsigned char *, int, int *, unsigned long *,
                             void (*)(int, int, void *), void *);
int DSA_generate_key(DSA *);
DSA *DSA_new(void);
void DSA_free(DSA *);
DSA_SIG *DSA_SIG_new(void);
void DSA_SIG_free(DSA_SIG *);
int i2d_DSA_SIG(const DSA_SIG *, unsigned char **);
DSA_SIG *d2i_DSA_SIG(DSA_SIG **, const unsigned char **, long);
int DSA_size(const DSA *);
int DSA_sign(int, const unsigned char *, int, unsigned char *, unsigned int *,
             DSA *);
int DSA_verify(int, const unsigned char *, int, const unsigned char *, int,
               DSA *);
"""

MACROS = """
int DSA_generate_parameters_ex(DSA *, int, unsigned char *, int,
                               int *, unsigned long *, BN_GENCB *);
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
