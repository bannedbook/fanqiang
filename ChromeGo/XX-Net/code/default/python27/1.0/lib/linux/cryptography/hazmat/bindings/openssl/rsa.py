# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/rsa.h>
"""

TYPES = """
typedef struct rsa_st {
    BIGNUM *n;
    BIGNUM *e;
    BIGNUM *d;
    BIGNUM *p;
    BIGNUM *q;
    BIGNUM *dmp1;
    BIGNUM *dmq1;
    BIGNUM *iqmp;
    ...;
} RSA;
typedef ... BN_GENCB;
static const int RSA_PKCS1_PADDING;
static const int RSA_SSLV23_PADDING;
static const int RSA_NO_PADDING;
static const int RSA_PKCS1_OAEP_PADDING;
static const int RSA_X931_PADDING;
static const int RSA_PKCS1_PSS_PADDING;
static const int RSA_F4;

static const int Cryptography_HAS_PSS_PADDING;
static const int Cryptography_HAS_MGF1_MD;
"""

FUNCTIONS = """
RSA *RSA_new(void);
void RSA_free(RSA *);
int RSA_size(const RSA *);
int RSA_generate_key_ex(RSA *, int, BIGNUM *, BN_GENCB *);
int RSA_check_key(const RSA *);
RSA *RSAPublicKey_dup(RSA *);
int RSA_blinding_on(RSA *, BN_CTX *);
void RSA_blinding_off(RSA *);
int RSA_public_encrypt(int, const unsigned char *, unsigned char *,
                       RSA *, int);
int RSA_private_encrypt(int, const unsigned char *, unsigned char *,
                        RSA *, int);
int RSA_public_decrypt(int, const unsigned char *, unsigned char *,
                       RSA *, int);
int RSA_private_decrypt(int, const unsigned char *, unsigned char *,
                        RSA *, int);
int RSA_print(BIO *, const RSA *, int);
int RSA_verify_PKCS1_PSS(RSA *, const unsigned char *, const EVP_MD *,
                         const unsigned char *, int);
int RSA_padding_add_PKCS1_PSS(RSA *, unsigned char *, const unsigned char *,
                              const EVP_MD *, int);
int RSA_padding_add_PKCS1_OAEP(unsigned char *, int, const unsigned char *,
                               int, const unsigned char *, int);
int RSA_padding_check_PKCS1_OAEP(unsigned char *, int, const unsigned char *,
                                 int, int, const unsigned char *, int);
"""

MACROS = """
int EVP_PKEY_CTX_set_rsa_padding(EVP_PKEY_CTX *, int);
int EVP_PKEY_CTX_set_rsa_pss_saltlen(EVP_PKEY_CTX *, int);
int EVP_PKEY_CTX_set_rsa_mgf1_md(EVP_PKEY_CTX *, EVP_MD *);
"""

CUSTOMIZATIONS = """
#if OPENSSL_VERSION_NUMBER >= 0x10000000
static const long Cryptography_HAS_PSS_PADDING = 1;
#else
/* see evp.py for the definition of Cryptography_HAS_PKEY_CTX */
static const long Cryptography_HAS_PSS_PADDING = 0;
int (*EVP_PKEY_CTX_set_rsa_padding)(EVP_PKEY_CTX *, int) = NULL;
int (*EVP_PKEY_CTX_set_rsa_pss_saltlen)(EVP_PKEY_CTX *, int) = NULL;
static const long RSA_PKCS1_PSS_PADDING = 0;
#endif
#if OPENSSL_VERSION_NUMBER >= 0x1000100f
static const long Cryptography_HAS_MGF1_MD = 1;
#else
static const long Cryptography_HAS_MGF1_MD = 0;
int (*EVP_PKEY_CTX_set_rsa_mgf1_md)(EVP_PKEY_CTX *, EVP_MD *) = NULL;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_PKEY_CTX": [
        "EVP_PKEY_CTX_set_rsa_padding",
        "EVP_PKEY_CTX_set_rsa_pss_saltlen",
    ],
    "Cryptography_HAS_PSS_PADDING": [
        "RSA_PKCS1_PSS_PADDING",
    ],
    "Cryptography_HAS_MGF1_MD": [
        "EVP_PKEY_CTX_set_rsa_mgf1_md",
    ],
}
