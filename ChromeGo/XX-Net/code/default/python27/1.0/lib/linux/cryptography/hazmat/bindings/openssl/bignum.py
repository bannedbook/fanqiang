# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/bn.h>
"""

TYPES = """
typedef ... BN_CTX;
typedef ... BIGNUM;
/*
 * TODO: This typedef is wrong.
 *
 * This is due to limitations of cffi.
 * See https://bitbucket.org/cffi/cffi/issue/69
 *
 * For another possible work-around (not used here because it involves more
 * complicated use of the cffi API which falls outside the general pattern used
 * by this package), see
 * http://paste.pound-python.org/show/iJcTUMkKeBeS6yXpZWUU/
 *
 * The work-around used here is to just be sure to declare a type that is at
 * least as large as the real type.  Maciej explains:
 *
 * <fijal> I think you want to declare your value too large (e.g. long)
 * <fijal> that way you'll never pass garbage
 */
typedef uintptr_t BN_ULONG;
"""

FUNCTIONS = """
BIGNUM *BN_new(void);
void BN_free(BIGNUM *);

BN_CTX *BN_CTX_new(void);
void BN_CTX_free(BN_CTX *);

void BN_CTX_start(BN_CTX *);
BIGNUM *BN_CTX_get(BN_CTX *);
void BN_CTX_end(BN_CTX *);

BIGNUM *BN_copy(BIGNUM *, const BIGNUM *);
BIGNUM *BN_dup(const BIGNUM *);

int BN_set_word(BIGNUM *, BN_ULONG);
BN_ULONG BN_get_word(const BIGNUM *);

const BIGNUM *BN_value_one(void);

char *BN_bn2hex(const BIGNUM *);
int BN_hex2bn(BIGNUM **, const char *);
int BN_dec2bn(BIGNUM **, const char *);

int BN_bn2bin(const BIGNUM *, unsigned char *);
BIGNUM *BN_bin2bn(const unsigned char *, int, BIGNUM *);

int BN_num_bits(const BIGNUM *);

int BN_cmp(const BIGNUM *, const BIGNUM *);
int BN_add(BIGNUM *, const BIGNUM *, const BIGNUM *);
int BN_sub(BIGNUM *, const BIGNUM *, const BIGNUM *);
int BN_mul(BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
int BN_sqr(BIGNUM *, const BIGNUM *, BN_CTX *);
int BN_div(BIGNUM *, BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
int BN_nnmod(BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
int BN_mod_add(BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *,
               BN_CTX *);
int BN_mod_sub(BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *,
               BN_CTX *);
int BN_mod_mul(BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *,
               BN_CTX *);
int BN_mod_sqr(BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
int BN_exp(BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
int BN_mod_exp(BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *,
               BN_CTX *);
int BN_gcd(BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
BIGNUM *BN_mod_inverse(BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);

int BN_set_bit(BIGNUM *, int);
int BN_clear_bit(BIGNUM *, int);

int BN_is_bit_set(const BIGNUM *, int);

int BN_mask_bits(BIGNUM *, int);
"""

MACROS = """
int BN_zero(BIGNUM *);
int BN_one(BIGNUM *);
int BN_mod(BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);

int BN_lshift(BIGNUM *, const BIGNUM *, int);
int BN_lshift1(BIGNUM *, BIGNUM *);

int BN_rshift(BIGNUM *, BIGNUM *, int);
int BN_rshift1(BIGNUM *, BIGNUM *);
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
