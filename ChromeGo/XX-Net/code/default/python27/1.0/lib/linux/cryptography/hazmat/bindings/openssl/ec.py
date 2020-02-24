# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#ifndef OPENSSL_NO_EC
#include <openssl/ec.h>
#endif

#include <openssl/obj_mac.h>
"""

TYPES = """
static const int Cryptography_HAS_EC;
static const int Cryptography_HAS_EC_1_0_1;
static const int Cryptography_HAS_EC_NISTP_64_GCC_128;
static const int Cryptography_HAS_EC2M;

static const int OPENSSL_EC_NAMED_CURVE;

typedef ... EC_KEY;
typedef ... EC_GROUP;
typedef ... EC_POINT;
typedef ... EC_METHOD;
typedef struct {
    int nid;
    const char *comment;
} EC_builtin_curve;
typedef enum { ... } point_conversion_form_t;
"""

FUNCTIONS = """
"""

MACROS = """
EC_GROUP *EC_GROUP_new(const EC_METHOD *);
void EC_GROUP_free(EC_GROUP *);
void EC_GROUP_clear_free(EC_GROUP *);

EC_GROUP *EC_GROUP_new_curve_GFp(
    const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
EC_GROUP *EC_GROUP_new_curve_GF2m(
    const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
EC_GROUP *EC_GROUP_new_by_curve_name(int);

int EC_GROUP_set_curve_GFp(
    EC_GROUP *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
int EC_GROUP_get_curve_GFp(
    const EC_GROUP *, BIGNUM *, BIGNUM *, BIGNUM *, BN_CTX *);
int EC_GROUP_set_curve_GF2m(
    EC_GROUP *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
int EC_GROUP_get_curve_GF2m(
    const EC_GROUP *, BIGNUM *, BIGNUM *, BIGNUM *, BN_CTX *);

int EC_GROUP_get_degree(const EC_GROUP *);

const EC_METHOD *EC_GROUP_method_of(const EC_GROUP *);
const EC_POINT *EC_GROUP_get0_generator(const EC_GROUP *);
int EC_GROUP_get_curve_name(const EC_GROUP *);

size_t EC_get_builtin_curves(EC_builtin_curve *, size_t);

void EC_KEY_free(EC_KEY *);

int EC_KEY_get_flags(const EC_KEY *);
void EC_KEY_set_flags(EC_KEY *, int);
void EC_KEY_clear_flags(EC_KEY *, int);
EC_KEY *EC_KEY_new_by_curve_name(int);
EC_KEY *EC_KEY_copy(EC_KEY *, const EC_KEY *);
EC_KEY *EC_KEY_dup(const EC_KEY *);
int EC_KEY_up_ref(EC_KEY *);
const EC_GROUP *EC_KEY_get0_group(const EC_KEY *);
int EC_GROUP_get_order(const EC_GROUP *, BIGNUM *, BN_CTX *);
int EC_KEY_set_group(EC_KEY *, const EC_GROUP *);
const BIGNUM *EC_KEY_get0_private_key(const EC_KEY *);
int EC_KEY_set_private_key(EC_KEY *, const BIGNUM *);
const EC_POINT *EC_KEY_get0_public_key(const EC_KEY *);
int EC_KEY_set_public_key(EC_KEY *, const EC_POINT *);
unsigned int EC_KEY_get_enc_flags(const EC_KEY *);
void EC_KEY_set_enc_flags(EC_KEY *eckey, unsigned int);
point_conversion_form_t EC_KEY_get_conv_form(const EC_KEY *);
void EC_KEY_set_conv_form(EC_KEY *, point_conversion_form_t);
void *EC_KEY_get_key_method_data(
    EC_KEY *,
    void *(*)(void *),
    void (*)(void *),
    void (*)(void *)
);
void EC_KEY_insert_key_method_data(
    EC_KEY *,
    void *,
    void *(*)(void *),
    void (*)(void *),
    void (*)(void *)
);
void EC_KEY_set_asn1_flag(EC_KEY *, int);
int EC_KEY_precompute_mult(EC_KEY *, BN_CTX *);
int EC_KEY_generate_key(EC_KEY *);
int EC_KEY_check_key(const EC_KEY *);
int EC_KEY_set_public_key_affine_coordinates(EC_KEY *, BIGNUM *, BIGNUM *);

EC_POINT *EC_POINT_new(const EC_GROUP *);
void EC_POINT_free(EC_POINT *);
void EC_POINT_clear_free(EC_POINT *);
int EC_POINT_copy(EC_POINT *, const EC_POINT *);
EC_POINT *EC_POINT_dup(const EC_POINT *, const EC_GROUP *);
const EC_METHOD *EC_POINT_method_of(const EC_POINT *);

int EC_POINT_set_to_infinity(const EC_GROUP *, EC_POINT *);

int EC_POINT_set_Jprojective_coordinates_GFp(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);

int EC_POINT_get_Jprojective_coordinates_GFp(const EC_GROUP *,
    const EC_POINT *, BIGNUM *, BIGNUM *, BIGNUM *, BN_CTX *);

int EC_POINT_set_affine_coordinates_GFp(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, const BIGNUM *, BN_CTX *);

int EC_POINT_get_affine_coordinates_GFp(const EC_GROUP *,
    const EC_POINT *, BIGNUM *, BIGNUM *, BN_CTX *);

int EC_POINT_set_compressed_coordinates_GFp(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, int, BN_CTX *);

int EC_POINT_set_affine_coordinates_GF2m(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, const BIGNUM *, BN_CTX *);

int EC_POINT_get_affine_coordinates_GF2m(const EC_GROUP *,
    const EC_POINT *, BIGNUM *, BIGNUM *, BN_CTX *);

int EC_POINT_set_compressed_coordinates_GF2m(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, int, BN_CTX *);

size_t EC_POINT_point2oct(const EC_GROUP *, const EC_POINT *,
    point_conversion_form_t,
    unsigned char *, size_t, BN_CTX *);

int EC_POINT_oct2point(const EC_GROUP *, EC_POINT *,
    const unsigned char *, size_t, BN_CTX *);

BIGNUM *EC_POINT_point2bn(const EC_GROUP *, const EC_POINT *,
    point_conversion_form_t form, BIGNUM *, BN_CTX *);

EC_POINT *EC_POINT_bn2point(const EC_GROUP *, const BIGNUM *,
    EC_POINT *, BN_CTX *);

char *EC_POINT_point2hex(const EC_GROUP *, const EC_POINT *,
    point_conversion_form_t form, BN_CTX *);

EC_POINT *EC_POINT_hex2point(const EC_GROUP *, const char *,
    EC_POINT *, BN_CTX *);

int EC_POINT_add(const EC_GROUP *, EC_POINT *, const EC_POINT *,
    const EC_POINT *, BN_CTX *);

int EC_POINT_dbl(const EC_GROUP *, EC_POINT *, const EC_POINT *, BN_CTX *);
int EC_POINT_invert(const EC_GROUP *, EC_POINT *, BN_CTX *);
int EC_POINT_is_at_infinity(const EC_GROUP *, const EC_POINT *);
int EC_POINT_is_on_curve(const EC_GROUP *, const EC_POINT *, BN_CTX *);

int EC_POINT_cmp(
    const EC_GROUP *, const EC_POINT *, const EC_POINT *, BN_CTX *);

int EC_POINT_make_affine(const EC_GROUP *, EC_POINT *, BN_CTX *);
int EC_POINTs_make_affine(const EC_GROUP *, size_t, EC_POINT *[], BN_CTX *);

int EC_POINTs_mul(
    const EC_GROUP *, EC_POINT *, const BIGNUM *,
    size_t, const EC_POINT *[], const BIGNUM *[], BN_CTX *);

int EC_POINT_mul(const EC_GROUP *, EC_POINT *, const BIGNUM *,
    const EC_POINT *, const BIGNUM *, BN_CTX *);

int EC_GROUP_precompute_mult(EC_GROUP *, BN_CTX *);
int EC_GROUP_have_precompute_mult(const EC_GROUP *);

const EC_METHOD *EC_GFp_simple_method();
const EC_METHOD *EC_GFp_mont_method();
const EC_METHOD *EC_GFp_nist_method();

const EC_METHOD *EC_GFp_nistp224_method();
const EC_METHOD *EC_GFp_nistp256_method();
const EC_METHOD *EC_GFp_nistp521_method();

const EC_METHOD *EC_GF2m_simple_method();

int EC_METHOD_get_field_type(const EC_METHOD *);
"""

CUSTOMIZATIONS = """
#ifdef OPENSSL_NO_EC
static const long Cryptography_HAS_EC = 0;

typedef void EC_KEY;
typedef void EC_GROUP;
typedef void EC_POINT;
typedef void EC_METHOD;
typedef struct {
    int nid;
    const char *comment;
} EC_builtin_curve;
typedef long point_conversion_form_t;

static const int OPENSSL_EC_NAMED_CURVE = 0;

void (*EC_KEY_free)(EC_KEY *) = NULL;
size_t (*EC_get_builtin_curves)(EC_builtin_curve *, size_t) = NULL;
EC_KEY *(*EC_KEY_new_by_curve_name)(int) = NULL;
EC_KEY *(*EC_KEY_copy)(EC_KEY *, const EC_KEY *) = NULL;
EC_KEY *(*EC_KEY_dup)(const EC_KEY *) = NULL;
int (*EC_KEY_up_ref)(EC_KEY *) = NULL;
const EC_GROUP *(*EC_KEY_get0_group)(const EC_KEY *) = NULL;
int (*EC_GROUP_get_order)(const EC_GROUP *, BIGNUM *, BN_CTX *) = NULL;
int (*EC_KEY_set_group)(EC_KEY *, const EC_GROUP *) = NULL;
const BIGNUM *(*EC_KEY_get0_private_key)(const EC_KEY *) = NULL;
int (*EC_KEY_set_private_key)(EC_KEY *, const BIGNUM *) = NULL;
const EC_POINT *(*EC_KEY_get0_public_key)(const EC_KEY *) = NULL;
int (*EC_KEY_set_public_key)(EC_KEY *, const EC_POINT *) = NULL;
unsigned int (*EC_KEY_get_enc_flags)(const EC_KEY *) = NULL;
void (*EC_KEY_set_enc_flags)(EC_KEY *eckey, unsigned int) = NULL;
point_conversion_form_t (*EC_KEY_get_conv_form)(const EC_KEY *) = NULL;
void (*EC_KEY_set_conv_form)(EC_KEY *, point_conversion_form_t) = NULL;
void *(*EC_KEY_get_key_method_data)(
    EC_KEY *, void *(*)(void *), void (*)(void *), void (*)(void *)) = NULL;
void (*EC_KEY_insert_key_method_data)(
    EC_KEY *, void *,
    void *(*)(void *), void (*)(void *), void (*)(void *)) = NULL;
void (*EC_KEY_set_asn1_flag)(EC_KEY *, int) = NULL;
int (*EC_KEY_precompute_mult)(EC_KEY *, BN_CTX *) = NULL;
int (*EC_KEY_generate_key)(EC_KEY *) = NULL;
int (*EC_KEY_check_key)(const EC_KEY *) = NULL;

EC_GROUP *(*EC_GROUP_new)(const EC_METHOD *);
void (*EC_GROUP_free)(EC_GROUP *);
void (*EC_GROUP_clear_free)(EC_GROUP *);

EC_GROUP *(*EC_GROUP_new_curve_GFp)(
    const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);

EC_GROUP *(*EC_GROUP_new_by_curve_name)(int);

int (*EC_GROUP_set_curve_GFp)(
    EC_GROUP *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);

int (*EC_GROUP_get_curve_GFp)(
    const EC_GROUP *, BIGNUM *, BIGNUM *, BIGNUM *, BN_CTX *);

int (*EC_GROUP_get_degree)(const EC_GROUP *) = NULL;

const EC_METHOD *(*EC_GROUP_method_of)(const EC_GROUP *) = NULL;
const EC_POINT *(*EC_GROUP_get0_generator)(const EC_GROUP *) = NULL;
int (*EC_GROUP_get_curve_name)(const EC_GROUP *) = NULL;

EC_POINT *(*EC_POINT_new)(const EC_GROUP *) = NULL;
void (*EC_POINT_free)(EC_POINT *) = NULL;
void (*EC_POINT_clear_free)(EC_POINT *) = NULL;
int (*EC_POINT_copy)(EC_POINT *, const EC_POINT *) = NULL;
EC_POINT *(*EC_POINT_dup)(const EC_POINT *, const EC_GROUP *) = NULL;
const EC_METHOD *(*EC_POINT_method_of)(const EC_POINT *) = NULL;
int (*EC_POINT_set_to_infinity)(const EC_GROUP *, EC_POINT *) = NULL;
int (*EC_POINT_set_Jprojective_coordinates_GFp)(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *) = NULL;

int (*EC_POINT_get_Jprojective_coordinates_GFp)(const EC_GROUP *,
    const EC_POINT *, BIGNUM *, BIGNUM *, BIGNUM *, BN_CTX *) = NULL;

int (*EC_POINT_set_affine_coordinates_GFp)(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, const BIGNUM *, BN_CTX *) = NULL;

int (*EC_POINT_get_affine_coordinates_GFp)(const EC_GROUP *,
    const EC_POINT *, BIGNUM *, BIGNUM *, BN_CTX *) = NULL;

int (*EC_POINT_set_compressed_coordinates_GFp)(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, int, BN_CTX *) = NULL;

size_t (*EC_POINT_point2oct)(const EC_GROUP *, const EC_POINT *,
    point_conversion_form_t,
    unsigned char *, size_t, BN_CTX *) = NULL;

int (*EC_POINT_oct2point)(const EC_GROUP *, EC_POINT *,
    const unsigned char *, size_t, BN_CTX *) = NULL;

BIGNUM *(*EC_POINT_point2bn)(const EC_GROUP *, const EC_POINT *,
    point_conversion_form_t form, BIGNUM *, BN_CTX *) = NULL;

EC_POINT *(*EC_POINT_bn2point)(const EC_GROUP *, const BIGNUM *,
    EC_POINT *, BN_CTX *) = NULL;

char *(*EC_POINT_point2hex)(const EC_GROUP *, const EC_POINT *,
    point_conversion_form_t form, BN_CTX *) = NULL;

EC_POINT *(*EC_POINT_hex2point)(const EC_GROUP *, const char *,
    EC_POINT *, BN_CTX *) = NULL;

int (*EC_POINT_add)(const EC_GROUP *, EC_POINT *, const EC_POINT *,
    const EC_POINT *, BN_CTX *) = NULL;

int (*EC_POINT_dbl)(const EC_GROUP *, EC_POINT *, const EC_POINT *,
    BN_CTX *) = NULL;

int (*EC_POINT_invert)(const EC_GROUP *, EC_POINT *, BN_CTX *) = NULL;
int (*EC_POINT_is_at_infinity)(const EC_GROUP *, const EC_POINT *) = NULL;

int (*EC_POINT_is_on_curve)(const EC_GROUP *, const EC_POINT *,
    BN_CTX *) = NULL;

int (*EC_POINT_cmp)(
    const EC_GROUP *, const EC_POINT *, const EC_POINT *, BN_CTX *) = NULL;

int (*EC_POINT_make_affine)(const EC_GROUP *, EC_POINT *, BN_CTX *) = NULL;

int (*EC_POINTs_make_affine)(const EC_GROUP *, size_t, EC_POINT *[],
    BN_CTX *) = NULL;

int (*EC_POINTs_mul)(
    const EC_GROUP *, EC_POINT *, const BIGNUM *,
    size_t, const EC_POINT *[], const BIGNUM *[], BN_CTX *) = NULL;

int (*EC_POINT_mul)(const EC_GROUP *, EC_POINT *, const BIGNUM *,
    const EC_POINT *, const BIGNUM *, BN_CTX *) = NULL;

int (*EC_GROUP_precompute_mult)(EC_GROUP *, BN_CTX *) = NULL;
int (*EC_GROUP_have_precompute_mult)(const EC_GROUP *) = NULL;

const EC_METHOD *(*EC_GFp_simple_method)() = NULL;
const EC_METHOD *(*EC_GFp_mont_method)() = NULL;
const EC_METHOD *(*EC_GFp_nist_method)() = NULL;

int (*EC_METHOD_get_field_type)(const EC_METHOD *) = NULL;

#else
static const long Cryptography_HAS_EC = 1;
#endif

#if defined(OPENSSL_NO_EC) || OPENSSL_VERSION_NUMBER < 0x1000100f
static const long Cryptography_HAS_EC_1_0_1 = 0;

int (*EC_KEY_get_flags)(const EC_KEY *) = NULL;
void (*EC_KEY_set_flags)(EC_KEY *, int) = NULL;
void (*EC_KEY_clear_flags)(EC_KEY *, int) = NULL;

int (*EC_KEY_set_public_key_affine_coordinates)(
    EC_KEY *, BIGNUM *, BIGNUM *) = NULL;
#else
static const long Cryptography_HAS_EC_1_0_1 = 1;
#endif


#if defined(OPENSSL_NO_EC) || OPENSSL_VERSION_NUMBER < 0x1000100f || \
    defined(OPENSSL_NO_EC_NISTP_64_GCC_128)
static const long Cryptography_HAS_EC_NISTP_64_GCC_128 = 0;

const EC_METHOD *(*EC_GFp_nistp224_method)(void) = NULL;
const EC_METHOD *(*EC_GFp_nistp256_method)(void) = NULL;
const EC_METHOD *(*EC_GFp_nistp521_method)(void) = NULL;
#else
static const long Cryptography_HAS_EC_NISTP_64_GCC_128 = 1;
#endif

#if defined(OPENSSL_NO_EC) || defined(OPENSSL_NO_EC2M)
static const long Cryptography_HAS_EC2M = 0;

const EC_METHOD *(*EC_GF2m_simple_method)() = NULL;

int (*EC_POINT_set_affine_coordinates_GF2m)(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, const BIGNUM *, BN_CTX *) = NULL;

int (*EC_POINT_get_affine_coordinates_GF2m)(const EC_GROUP *,
    const EC_POINT *, BIGNUM *, BIGNUM *, BN_CTX *) = NULL;

int (*EC_POINT_set_compressed_coordinates_GF2m)(const EC_GROUP *, EC_POINT *,
    const BIGNUM *, int, BN_CTX *) = NULL;

int (*EC_GROUP_set_curve_GF2m)(
    EC_GROUP *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);

int (*EC_GROUP_get_curve_GF2m)(
    const EC_GROUP *, BIGNUM *, BIGNUM *, BIGNUM *, BN_CTX *);

EC_GROUP *(*EC_GROUP_new_curve_GF2m)(
    const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
#else
static const long Cryptography_HAS_EC2M = 1;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_EC": [
        "OPENSSL_EC_NAMED_CURVE",
        "EC_GROUP_new",
        "EC_GROUP_free",
        "EC_GROUP_clear_free",
        "EC_GROUP_new_curve_GFp",
        "EC_GROUP_new_by_curve_name",
        "EC_GROUP_set_curve_GFp",
        "EC_GROUP_get_curve_GFp",
        "EC_GROUP_method_of",
        "EC_GROUP_get0_generator",
        "EC_GROUP_get_curve_name",
        "EC_GROUP_get_degree",
        "EC_KEY_free",
        "EC_get_builtin_curves",
        "EC_KEY_new_by_curve_name",
        "EC_KEY_copy",
        "EC_KEY_dup",
        "EC_KEY_up_ref",
        "EC_KEY_set_group",
        "EC_KEY_get0_private_key",
        "EC_KEY_set_private_key",
        "EC_KEY_set_public_key",
        "EC_KEY_get_enc_flags",
        "EC_KEY_set_enc_flags",
        "EC_KEY_set_conv_form",
        "EC_KEY_get_key_method_data",
        "EC_KEY_insert_key_method_data",
        "EC_KEY_set_asn1_flag",
        "EC_KEY_precompute_mult",
        "EC_KEY_generate_key",
        "EC_KEY_check_key",
        "EC_POINT_new",
        "EC_POINT_free",
        "EC_POINT_clear_free",
        "EC_POINT_copy",
        "EC_POINT_dup",
        "EC_POINT_method_of",
        "EC_POINT_set_to_infinity",
        "EC_POINT_set_Jprojective_coordinates_GFp",
        "EC_POINT_get_Jprojective_coordinates_GFp",
        "EC_POINT_set_affine_coordinates_GFp",
        "EC_POINT_get_affine_coordinates_GFp",
        "EC_POINT_set_compressed_coordinates_GFp",
        "EC_POINT_point2oct",
        "EC_POINT_oct2point",
        "EC_POINT_point2bn",
        "EC_POINT_bn2point",
        "EC_POINT_point2hex",
        "EC_POINT_hex2point",
        "EC_POINT_add",
        "EC_POINT_dbl",
        "EC_POINT_invert",
        "EC_POINT_is_at_infinity",
        "EC_POINT_is_on_curve",
        "EC_POINT_cmp",
        "EC_POINT_make_affine",
        "EC_POINTs_make_affine",
        "EC_POINTs_mul",
        "EC_POINT_mul",
        "EC_GROUP_precompute_mult",
        "EC_GROUP_have_precompute_mult",
        "EC_GFp_simple_method",
        "EC_GFp_mont_method",
        "EC_GFp_nist_method",
        "EC_METHOD_get_field_type",
    ],

    "Cryptography_HAS_EC_1_0_1": [
        "EC_KEY_get_flags",
        "EC_KEY_set_flags",
        "EC_KEY_clear_flags",
        "EC_KEY_set_public_key_affine_coordinates",
    ],

    "Cryptography_HAS_EC_NISTP_64_GCC_128": [
        "EC_GFp_nistp224_method",
        "EC_GFp_nistp256_method",
        "EC_GFp_nistp521_method",
    ],

    "Cryptography_HAS_EC2M": [
        "EC_GF2m_simple_method",
        "EC_POINT_set_affine_coordinates_GF2m",
        "EC_POINT_get_affine_coordinates_GF2m",
        "EC_POINT_set_compressed_coordinates_GF2m",
        "EC_GROUP_set_curve_GF2m",
        "EC_GROUP_get_curve_GF2m",
        "EC_GROUP_new_curve_GF2m",
    ],
}
