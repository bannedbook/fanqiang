# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/evp.h>
"""

TYPES = """
typedef ... EVP_CIPHER;
typedef struct {
    const EVP_CIPHER *cipher;
    ENGINE *engine;
    int encrypt;
    ...;
} EVP_CIPHER_CTX;
typedef ... EVP_MD;
typedef struct env_md_ctx_st {
    ...;
} EVP_MD_CTX;

typedef struct evp_pkey_st {
    int type;
    ...;
} EVP_PKEY;
typedef ... EVP_PKEY_CTX;
static const int EVP_PKEY_RSA;
static const int EVP_PKEY_DSA;
static const int EVP_PKEY_EC;
static const int EVP_MAX_MD_SIZE;
static const int EVP_CTRL_GCM_SET_IVLEN;
static const int EVP_CTRL_GCM_GET_TAG;
static const int EVP_CTRL_GCM_SET_TAG;

static const int Cryptography_HAS_GCM;
static const int Cryptography_HAS_PBKDF2_HMAC;
static const int Cryptography_HAS_PKEY_CTX;
"""

FUNCTIONS = """
const EVP_CIPHER *EVP_get_cipherbyname(const char *);
int EVP_EncryptInit_ex(EVP_CIPHER_CTX *, const EVP_CIPHER *, ENGINE *,
                       const unsigned char *, const unsigned char *);
int EVP_CIPHER_CTX_set_padding(EVP_CIPHER_CTX *, int);
int EVP_EncryptUpdate(EVP_CIPHER_CTX *, unsigned char *, int *,
                      const unsigned char *, int);
int EVP_EncryptFinal_ex(EVP_CIPHER_CTX *, unsigned char *, int *);
int EVP_DecryptInit_ex(EVP_CIPHER_CTX *, const EVP_CIPHER *, ENGINE *,
                       const unsigned char *, const unsigned char *);
int EVP_DecryptUpdate(EVP_CIPHER_CTX *, unsigned char *, int *,
                      const unsigned char *, int);
int EVP_DecryptFinal_ex(EVP_CIPHER_CTX *, unsigned char *, int *);
int EVP_CipherInit_ex(EVP_CIPHER_CTX *, const EVP_CIPHER *, ENGINE *,
                      const unsigned char *, const unsigned char *, int);
int EVP_CipherUpdate(EVP_CIPHER_CTX *, unsigned char *, int *,
                     const unsigned char *, int);
int EVP_CipherFinal_ex(EVP_CIPHER_CTX *, unsigned char *, int *);
int EVP_CIPHER_CTX_cleanup(EVP_CIPHER_CTX *);
void EVP_CIPHER_CTX_init(EVP_CIPHER_CTX *);
EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void);
void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *);
int EVP_CIPHER_CTX_set_key_length(EVP_CIPHER_CTX *, int);

EVP_MD_CTX *EVP_MD_CTX_create(void);
int EVP_MD_CTX_copy_ex(EVP_MD_CTX *, const EVP_MD_CTX *);
int EVP_DigestInit_ex(EVP_MD_CTX *, const EVP_MD *, ENGINE *);
int EVP_DigestUpdate(EVP_MD_CTX *, const void *, size_t);
int EVP_DigestFinal_ex(EVP_MD_CTX *, unsigned char *, unsigned int *);
int EVP_MD_CTX_cleanup(EVP_MD_CTX *);
void EVP_MD_CTX_destroy(EVP_MD_CTX *);
const EVP_MD *EVP_get_digestbyname(const char *);

EVP_PKEY *EVP_PKEY_new(void);
void EVP_PKEY_free(EVP_PKEY *);
int EVP_PKEY_type(int);
int EVP_PKEY_bits(EVP_PKEY *);
int EVP_PKEY_size(EVP_PKEY *);
RSA *EVP_PKEY_get1_RSA(EVP_PKEY *);
DSA *EVP_PKEY_get1_DSA(EVP_PKEY *);
DH *EVP_PKEY_get1_DH(EVP_PKEY *);

int EVP_SignInit(EVP_MD_CTX *, const EVP_MD *);
int EVP_SignUpdate(EVP_MD_CTX *, const void *, size_t);
int EVP_SignFinal(EVP_MD_CTX *, unsigned char *, unsigned int *, EVP_PKEY *);

int EVP_VerifyInit(EVP_MD_CTX *, const EVP_MD *);
int EVP_VerifyUpdate(EVP_MD_CTX *, const void *, size_t);
int EVP_VerifyFinal(EVP_MD_CTX *, const unsigned char *, unsigned int,
                    EVP_PKEY *);

const EVP_MD *EVP_md5(void);
const EVP_MD *EVP_sha1(void);
const EVP_MD *EVP_ripemd160(void);
const EVP_MD *EVP_sha224(void);
const EVP_MD *EVP_sha256(void);
const EVP_MD *EVP_sha384(void);
const EVP_MD *EVP_sha512(void);

int PKCS5_PBKDF2_HMAC_SHA1(const char *, int, const unsigned char *, int, int,
                           int, unsigned char *);

int EVP_PKEY_set1_RSA(EVP_PKEY *, struct rsa_st *);
int EVP_PKEY_set1_DSA(EVP_PKEY *, struct dsa_st *);
int EVP_PKEY_set1_DH(EVP_PKEY *, DH *);

int EVP_PKEY_get_attr_count(const EVP_PKEY *);
int EVP_PKEY_get_attr_by_NID(const EVP_PKEY *, int, int);
int EVP_PKEY_get_attr_by_OBJ(const EVP_PKEY *, ASN1_OBJECT *, int);
X509_ATTRIBUTE *EVP_PKEY_get_attr(const EVP_PKEY *, int);
X509_ATTRIBUTE *EVP_PKEY_delete_attr(EVP_PKEY *, int);
int EVP_PKEY_add1_attr(EVP_PKEY *, X509_ATTRIBUTE *);
int EVP_PKEY_add1_attr_by_OBJ(EVP_PKEY *, const ASN1_OBJECT *, int,
                              const unsigned char *, int);
int EVP_PKEY_add1_attr_by_NID(EVP_PKEY *, int, int,
                              const unsigned char *, int);
int EVP_PKEY_add1_attr_by_txt(EVP_PKEY *, const char *, int,
                              const unsigned char *, int);

int EVP_PKEY_cmp(const EVP_PKEY *, const EVP_PKEY *);

EVP_PKEY *EVP_PKCS82PKEY(PKCS8_PRIV_KEY_INFO *);
"""

MACROS = """
void OpenSSL_add_all_algorithms(void);
int EVP_PKEY_assign_RSA(EVP_PKEY *, RSA *);
int EVP_PKEY_assign_DSA(EVP_PKEY *, DSA *);

int EVP_PKEY_assign_EC_KEY(EVP_PKEY *, EC_KEY *);
EC_KEY *EVP_PKEY_get1_EC_KEY(EVP_PKEY *);
int EVP_PKEY_set1_EC_KEY(EVP_PKEY *, EC_KEY *);

int EVP_CIPHER_CTX_block_size(const EVP_CIPHER_CTX *);
int EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *, int, int, void *);

int PKCS5_PBKDF2_HMAC(const char *, int, const unsigned char *, int, int,
                      const EVP_MD *, int, unsigned char *);

int EVP_PKEY_CTX_set_signature_md(EVP_PKEY_CTX *, const EVP_MD *);

/* These aren't macros, but must be in this section because they're not
   available in 0.9.8. */
EVP_PKEY_CTX *EVP_PKEY_CTX_new(EVP_PKEY *, ENGINE *);
EVP_PKEY_CTX *EVP_PKEY_CTX_new_id(int, ENGINE *);
EVP_PKEY_CTX *EVP_PKEY_CTX_dup(EVP_PKEY_CTX *);
void EVP_PKEY_CTX_free(EVP_PKEY_CTX *);
int EVP_PKEY_sign_init(EVP_PKEY_CTX *);
int EVP_PKEY_sign(EVP_PKEY_CTX *, unsigned char *, size_t *,
                  const unsigned char *, size_t);
int EVP_PKEY_verify_init(EVP_PKEY_CTX *);
int EVP_PKEY_verify(EVP_PKEY_CTX *, const unsigned char *, size_t,
                    const unsigned char *, size_t);
int EVP_PKEY_encrypt_init(EVP_PKEY_CTX *);
int EVP_PKEY_decrypt_init(EVP_PKEY_CTX *);

/* The following were macros in 0.9.8e. Once we drop support for RHEL/CentOS 5
   we should move these back to FUNCTIONS. */
const EVP_CIPHER *EVP_CIPHER_CTX_cipher(const EVP_CIPHER_CTX *);
int EVP_CIPHER_block_size(const EVP_CIPHER *);
const EVP_MD *EVP_MD_CTX_md(const EVP_MD_CTX *);
int EVP_MD_size(const EVP_MD *);

/* Must be in macros because EVP_PKEY_CTX is undefined in 0.9.8 */
int Cryptography_EVP_PKEY_encrypt(EVP_PKEY_CTX *ctx, unsigned char *out,
                                  size_t *outlen, const unsigned char *in,
                                  size_t inlen);
int Cryptography_EVP_PKEY_decrypt(EVP_PKEY_CTX *ctx, unsigned char *out,
                                  size_t *outlen, const unsigned char *in,
                                  size_t inlen);
"""

CUSTOMIZATIONS = """
#ifdef EVP_CTRL_GCM_SET_TAG
const long Cryptography_HAS_GCM = 1;
#else
const long Cryptography_HAS_GCM = 0;
const long EVP_CTRL_GCM_GET_TAG = -1;
const long EVP_CTRL_GCM_SET_TAG = -1;
const long EVP_CTRL_GCM_SET_IVLEN = -1;
#endif
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
const long Cryptography_HAS_PBKDF2_HMAC = 1;
const long Cryptography_HAS_PKEY_CTX = 1;

/* OpenSSL 0.9.8 defines EVP_PKEY_encrypt and EVP_PKEY_decrypt functions,
   but they are a completely different signature from the ones in 1.0.0+.
   These wrapper functions allows us to safely declare them on any version and
   conditionally remove them on 0.9.8. */
int Cryptography_EVP_PKEY_encrypt(EVP_PKEY_CTX *ctx, unsigned char *out,
                                  size_t *outlen, const unsigned char *in,
                                  size_t inlen) {
    return EVP_PKEY_encrypt(ctx, out, outlen, in, inlen);
}
int Cryptography_EVP_PKEY_decrypt(EVP_PKEY_CTX *ctx, unsigned char *out,
                                  size_t *outlen, const unsigned char *in,
                                  size_t inlen) {
    return EVP_PKEY_decrypt(ctx, out, outlen, in, inlen);
}
#else
const long Cryptography_HAS_PBKDF2_HMAC = 0;
int (*PKCS5_PBKDF2_HMAC)(const char *, int, const unsigned char *, int, int,
                         const EVP_MD *, int, unsigned char *) = NULL;
const long Cryptography_HAS_PKEY_CTX = 0;
typedef void EVP_PKEY_CTX;
int (*EVP_PKEY_CTX_set_signature_md)(EVP_PKEY_CTX *, const EVP_MD *) = NULL;
int (*EVP_PKEY_sign_init)(EVP_PKEY_CTX *) = NULL;
int (*EVP_PKEY_sign)(EVP_PKEY_CTX *, unsigned char *, size_t *,
                     const unsigned char *, size_t) = NULL;
int (*EVP_PKEY_verify_init)(EVP_PKEY_CTX *) = NULL;
int (*EVP_PKEY_verify)(EVP_PKEY_CTX *, const unsigned char *, size_t,
                       const unsigned char *, size_t) = NULL;
EVP_PKEY_CTX *(*EVP_PKEY_CTX_new)(EVP_PKEY *, ENGINE *) = NULL;
EVP_PKEY_CTX *(*EVP_PKEY_CTX_new_id)(int, ENGINE *) = NULL;
EVP_PKEY_CTX *(*EVP_PKEY_CTX_dup)(EVP_PKEY_CTX *) = NULL;
void (*EVP_PKEY_CTX_free)(EVP_PKEY_CTX *) = NULL;
int (*EVP_PKEY_encrypt_init)(EVP_PKEY_CTX *) = NULL;
int (*EVP_PKEY_decrypt_init)(EVP_PKEY_CTX *) = NULL;
int (*Cryptography_EVP_PKEY_encrypt)(EVP_PKEY_CTX *, unsigned char *, size_t *,
                                     const unsigned char *, size_t) = NULL;
int (*Cryptography_EVP_PKEY_decrypt)(EVP_PKEY_CTX *, unsigned char *, size_t *,
                                     const unsigned char *, size_t) = NULL;
#endif
#ifdef OPENSSL_NO_EC
int (*EVP_PKEY_assign_EC_KEY)(EVP_PKEY *, EC_KEY *) = NULL;
EC_KEY *(*EVP_PKEY_get1_EC_KEY)(EVP_PKEY *) = NULL;
int (*EVP_PKEY_set1_EC_KEY)(EVP_PKEY *, EC_KEY *) = NULL;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_GCM": [
        "EVP_CTRL_GCM_GET_TAG",
        "EVP_CTRL_GCM_SET_TAG",
        "EVP_CTRL_GCM_SET_IVLEN",
    ],
    "Cryptography_HAS_PBKDF2_HMAC": [
        "PKCS5_PBKDF2_HMAC"
    ],
    "Cryptography_HAS_PKEY_CTX": [
        "EVP_PKEY_CTX_new",
        "EVP_PKEY_CTX_new_id",
        "EVP_PKEY_CTX_dup",
        "EVP_PKEY_CTX_free",
        "EVP_PKEY_sign",
        "EVP_PKEY_sign_init",
        "EVP_PKEY_verify",
        "EVP_PKEY_verify_init",
        "Cryptography_EVP_PKEY_encrypt",
        "EVP_PKEY_encrypt_init",
        "Cryptography_EVP_PKEY_decrypt",
        "EVP_PKEY_decrypt_init",
        "EVP_PKEY_CTX_set_signature_md",
    ],
    "Cryptography_HAS_EC": [
        "EVP_PKEY_assign_EC_KEY",
        "EVP_PKEY_get1_EC_KEY",
        "EVP_PKEY_set1_EC_KEY",
    ]
}
