# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/pem.h>
"""

TYPES = """
typedef int pem_password_cb(char *buf, int size, int rwflag, void *userdata);
"""

FUNCTIONS = """
X509 *PEM_read_bio_X509(BIO *, X509 **, pem_password_cb *, void *);
int PEM_write_bio_X509(BIO *, X509 *);

int PEM_write_bio_PrivateKey(BIO *, EVP_PKEY *, const EVP_CIPHER *,
                             unsigned char *, int, pem_password_cb *, void *);

EVP_PKEY *PEM_read_bio_PrivateKey(BIO *, EVP_PKEY **, pem_password_cb *,
                                 void *);

int PEM_write_bio_PKCS8PrivateKey(BIO *, EVP_PKEY *, const EVP_CIPHER *,
                                  char *, int, pem_password_cb *, void *);
int PEM_write_bio_PKCS8PrivateKey_nid(BIO *, EVP_PKEY *, int, char *, int,
                                      pem_password_cb *, void *);

int i2d_PKCS8PrivateKey_bio(BIO *, EVP_PKEY *, const EVP_CIPHER *,
                            char *, int, pem_password_cb *, void *);
int i2d_PKCS8PrivateKey_nid_bio(BIO *, EVP_PKEY *, int,
                                char *, int, pem_password_cb *, void *);

PKCS7 *d2i_PKCS7_bio(BIO *, PKCS7 **);
EVP_PKEY *d2i_PKCS8PrivateKey_bio(BIO *, EVP_PKEY **, pem_password_cb *,
                                  void *);

int PEM_write_bio_X509_REQ(BIO *, X509_REQ *);

X509_REQ *PEM_read_bio_X509_REQ(BIO *, X509_REQ **, pem_password_cb *, void *);

X509_CRL *PEM_read_bio_X509_CRL(BIO *, X509_CRL **, pem_password_cb *, void *);

int PEM_write_bio_X509_CRL(BIO *, X509_CRL *);

PKCS7 *PEM_read_bio_PKCS7(BIO *, PKCS7 **, pem_password_cb *, void *);
DH *PEM_read_bio_DHparams(BIO *, DH **, pem_password_cb *, void *);

DSA *PEM_read_bio_DSAPrivateKey(BIO *, DSA **, pem_password_cb *, void *);

RSA *PEM_read_bio_RSAPrivateKey(BIO *, RSA **, pem_password_cb *, void *);

int PEM_write_bio_DSAPrivateKey(BIO *, DSA *, const EVP_CIPHER *,
                                unsigned char *, int,
                                pem_password_cb *, void *);

int PEM_write_bio_RSAPrivateKey(BIO *, RSA *, const EVP_CIPHER *,
                                unsigned char *, int,
                                pem_password_cb *, void *);

DSA *PEM_read_bio_DSA_PUBKEY(BIO *, DSA **, pem_password_cb *, void *);

RSA *PEM_read_bio_RSAPublicKey(BIO *, RSA **, pem_password_cb *, void *);

int PEM_write_bio_DSA_PUBKEY(BIO *, DSA *);

int PEM_write_bio_RSAPublicKey(BIO *, const RSA *);

EVP_PKEY *PEM_read_bio_PUBKEY(BIO *, EVP_PKEY **, pem_password_cb *, void *);
int PEM_write_bio_PUBKEY(BIO *, EVP_PKEY *);
"""

MACROS = """
int PEM_write_bio_ECPrivateKey(BIO *, EC_KEY *, const EVP_CIPHER *,
                               unsigned char *, int, pem_password_cb *,
                               void *);
"""

CUSTOMIZATIONS = """
// Cryptography_HAS_EC is provided by ec.py so we don't need to define it here
#ifdef OPENSSL_NO_EC
int (*PEM_write_bio_ECPrivateKey)(BIO *, EC_KEY *, const EVP_CIPHER *,
                                  unsigned char *, int, pem_password_cb *,
                                  void *) = NULL;
#endif

"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_EC": [
        "PEM_write_bio_ECPrivateKey"
    ]
}
