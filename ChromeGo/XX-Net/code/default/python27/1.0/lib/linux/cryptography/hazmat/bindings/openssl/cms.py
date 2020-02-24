# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#if !defined(OPENSSL_NO_CMS) && OPENSSL_VERSION_NUMBER >= 0x0090808fL
/* The next define should really be in the OpenSSL header, but it is missing.
   Failing to include this on Windows causes compilation failures. */
#if defined(OPENSSL_SYS_WINDOWS)
#include <windows.h>
#endif
#include <openssl/cms.h>
#endif
"""

TYPES = """
static const long Cryptography_HAS_CMS;

typedef ... CMS_ContentInfo;
typedef ... CMS_SignerInfo;
typedef ... CMS_CertificateChoices;
typedef ... CMS_RevocationInfoChoice;
typedef ... CMS_RecipientInfo;
typedef ... CMS_ReceiptRequest;
typedef ... CMS_Receipt;

static const int CMS_TEXT;
static const int CMS_NOCERTS;
static const int CMS_NO_CONTENT_VERIFY;
static const int CMS_NO_ATTR_VERIFY;
static const int CMS_NOSIGS;
static const int CMS_NOINTERN;
static const int CMS_NO_SIGNER_CERT_VERIFY;
static const int CMS_NOVERIFY;
static const int CMS_DETACHED;
static const int CMS_BINARY;
static const int CMS_NOATTR;
static const int CMS_NOSMIMECAP;
static const int CMS_NOOLDMIMETYPE;
static const int CMS_CRLFEOL;
static const int CMS_STREAM;
static const int CMS_NOCRL;
static const int CMS_PARTIAL;
static const int CMS_REUSE_DIGEST;
static const int CMS_USE_KEYID;
static const int CMS_DEBUG_DECRYPT;
"""

FUNCTIONS = """
"""

MACROS = """
BIO *BIO_new_CMS(BIO *, CMS_ContentInfo *);
int i2d_CMS_bio_stream(BIO *, CMS_ContentInfo *, BIO *, int);
int PEM_write_bio_CMS_stream(BIO *, CMS_ContentInfo *, BIO *, int);
int CMS_final(CMS_ContentInfo *, BIO *, BIO *, unsigned int);
CMS_ContentInfo *CMS_sign(X509 *, EVP_PKEY *, Cryptography_STACK_OF_X509 *,
                          BIO *, unsigned int);
int CMS_verify(CMS_ContentInfo *, Cryptography_STACK_OF_X509 *, X509_STORE *,
               BIO *, BIO *, unsigned int);
CMS_ContentInfo *CMS_encrypt(Cryptography_STACK_OF_X509 *, BIO *,
                             const EVP_CIPHER *, unsigned int);
int CMS_decrypt(CMS_ContentInfo *, EVP_PKEY *, X509 *, BIO *, BIO *,
                unsigned int);
CMS_SignerInfo *CMS_add1_signer(CMS_ContentInfo *, X509 *, EVP_PKEY *,
                                const EVP_MD *, unsigned int);
"""

CUSTOMIZATIONS = """
#if !defined(OPENSSL_NO_CMS) && OPENSSL_VERSION_NUMBER >= 0x0090808fL
static const long Cryptography_HAS_CMS = 1;
#else
static const long Cryptography_HAS_CMS = 0;
typedef void CMS_ContentInfo;
typedef void CMS_SignerInfo;
typedef void CMS_CertificateChoices;
typedef void CMS_RevocationInfoChoice;
typedef void CMS_RecipientInfo;
typedef void CMS_ReceiptRequest;
typedef void CMS_Receipt;
const long CMS_TEXT = 0;
const long CMS_NOCERTS = 0;
const long CMS_NO_CONTENT_VERIFY = 0;
const long CMS_NO_ATTR_VERIFY = 0;
const long CMS_NOSIGS = 0;
const long CMS_NOINTERN = 0;
const long CMS_NO_SIGNER_CERT_VERIFY = 0;
const long CMS_NOVERIFY = 0;
const long CMS_DETACHED = 0;
const long CMS_BINARY = 0;
const long CMS_NOATTR = 0;
const long CMS_NOSMIMECAP = 0;
const long CMS_NOOLDMIMETYPE = 0;
const long CMS_CRLFEOL = 0;
const long CMS_STREAM = 0;
const long CMS_NOCRL = 0;
const long CMS_PARTIAL = 0;
const long CMS_REUSE_DIGEST = 0;
const long CMS_USE_KEYID = 0;
const long CMS_DEBUG_DECRYPT = 0;
BIO *(*BIO_new_CMS)(BIO *, CMS_ContentInfo *) = NULL;
int (*i2d_CMS_bio_stream)(BIO *, CMS_ContentInfo *, BIO *, int) = NULL;
int (*PEM_write_bio_CMS_stream)(BIO *, CMS_ContentInfo *, BIO *, int) = NULL;
int (*CMS_final)(CMS_ContentInfo *, BIO *, BIO *, unsigned int) = NULL;
CMS_ContentInfo *(*CMS_sign)(X509 *, EVP_PKEY *, Cryptography_STACK_OF_X509 *,
                             BIO *, unsigned int) = NULL;
int (*CMS_verify)(CMS_ContentInfo *, Cryptography_STACK_OF_X509 *,
                  X509_STORE *, BIO *, BIO *, unsigned int) = NULL;
CMS_ContentInfo *(*CMS_encrypt)(Cryptography_STACK_OF_X509 *, BIO *,
                                const EVP_CIPHER *, unsigned int) = NULL;
int (*CMS_decrypt)(CMS_ContentInfo *, EVP_PKEY *, X509 *, BIO *, BIO *,
                   unsigned int) = NULL;
CMS_SignerInfo *(*CMS_add1_signer)(CMS_ContentInfo *, X509 *, EVP_PKEY *,
                                   const EVP_MD *, unsigned int) = NULL;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_CMS": [
        "BIO_new_CMS",
        "i2d_CMS_bio_stream",
        "PEM_write_bio_CMS_stream",
        "CMS_final",
        "CMS_sign",
        "CMS_verify",
        "CMS_encrypt",
        "CMS_decrypt",
        "CMS_add1_signer",
        "CMS_TEXT",
        "CMS_NOCERTS",
        "CMS_NO_CONTENT_VERIFY",
        "CMS_NO_ATTR_VERIFY",
        "CMS_NOSIGS",
        "CMS_NOINTERN",
        "CMS_NO_SIGNER_CERT_VERIFY",
        "CMS_NOVERIFY",
        "CMS_DETACHED",
        "CMS_BINARY",
        "CMS_NOATTR",
        "CMS_NOSMIMECAP",
        "CMS_NOOLDMIMETYPE",
        "CMS_CRLFEOL",
        "CMS_STREAM",
        "CMS_NOCRL",
        "CMS_PARTIAL",
        "CMS_REUSE_DIGEST",
        "CMS_USE_KEYID",
        "CMS_DEBUG_DECRYPT",
    ]
}
