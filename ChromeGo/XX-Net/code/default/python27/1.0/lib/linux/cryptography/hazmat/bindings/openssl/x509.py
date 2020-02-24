# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/ssl.h>

/*
 * This is part of a work-around for the difficulty cffi has in dealing with
 * `STACK_OF(foo)` as the name of a type.  We invent a new, simpler name that
 * will be an alias for this type and use the alias throughout.  This works
 * together with another opaque typedef for the same name in the TYPES section.
 * Note that the result is an opaque type.
 */
typedef STACK_OF(X509) Cryptography_STACK_OF_X509;
typedef STACK_OF(X509_CRL) Cryptography_STACK_OF_X509_CRL;
typedef STACK_OF(X509_REVOKED) Cryptography_STACK_OF_X509_REVOKED;
"""

TYPES = """
typedef ... Cryptography_STACK_OF_X509;
typedef ... Cryptography_STACK_OF_X509_CRL;
typedef ... Cryptography_STACK_OF_X509_REVOKED;

typedef struct {
    ASN1_OBJECT *algorithm;
    ...;
} X509_ALGOR;

typedef ... X509_ATTRIBUTE;

typedef struct {
    X509_ALGOR *signature;
    ...;
} X509_CINF;

typedef struct {
    ASN1_OBJECT *object;
    ASN1_BOOLEAN critical;
    ASN1_OCTET_STRING *value;
} X509_EXTENSION;

typedef ... X509_EXTENSIONS;

typedef ... X509_REQ;

typedef struct {
    ASN1_INTEGER *serialNumber;
    ASN1_TIME *revocationDate;
    X509_EXTENSIONS *extensions;
    int sequence;
    ...;
} X509_REVOKED;

typedef struct {
    Cryptography_STACK_OF_X509_REVOKED *revoked;
    ...;
} X509_CRL_INFO;

typedef struct {
    X509_CRL_INFO *crl;
    ...;
} X509_CRL;

typedef struct {
    X509_ALGOR *sig_alg;
    X509_CINF *cert_info;
    ...;
} X509;

typedef ... NETSCAPE_SPKI;

typedef ... PKCS8_PRIV_KEY_INFO;

static const int X509_FLAG_COMPAT;
static const int X509_FLAG_NO_HEADER;
static const int X509_FLAG_NO_VERSION;
static const int X509_FLAG_NO_SERIAL;
static const int X509_FLAG_NO_SIGNAME;
static const int X509_FLAG_NO_ISSUER;
static const int X509_FLAG_NO_VALIDITY;
static const int X509_FLAG_NO_SUBJECT;
static const int X509_FLAG_NO_PUBKEY;
static const int X509_FLAG_NO_EXTENSIONS;
static const int X509_FLAG_NO_SIGDUMP;
static const int X509_FLAG_NO_AUX;
static const int X509_FLAG_NO_ATTRIBUTES;

static const int XN_FLAG_SEP_MASK;
static const int XN_FLAG_COMPAT;
static const int XN_FLAG_SEP_COMMA_PLUS;
static const int XN_FLAG_SEP_CPLUS_SPC;
static const int XN_FLAG_SEP_SPLUS_SPC;
static const int XN_FLAG_SEP_MULTILINE;
static const int XN_FLAG_DN_REV;
static const int XN_FLAG_FN_MASK;
static const int XN_FLAG_FN_SN;
static const int XN_FLAG_FN_LN;
static const int XN_FLAG_FN_OID;
static const int XN_FLAG_FN_NONE;
static const int XN_FLAG_SPC_EQ;
static const int XN_FLAG_DUMP_UNKNOWN_FIELDS;
static const int XN_FLAG_FN_ALIGN;
static const int XN_FLAG_RFC2253;
static const int XN_FLAG_ONELINE;
static const int XN_FLAG_MULTILINE;
"""

FUNCTIONS = """
X509 *X509_new(void);
void X509_free(X509 *);
X509 *X509_dup(X509 *);

int X509_print_ex(BIO *, X509 *, unsigned long, unsigned long);

int X509_set_version(X509 *, long);

EVP_PKEY *X509_get_pubkey(X509 *);
int X509_set_pubkey(X509 *, EVP_PKEY *);

unsigned char *X509_alias_get0(X509 *, int *);
int X509_sign(X509 *, EVP_PKEY *, const EVP_MD *);

int X509_digest(const X509 *, const EVP_MD *, unsigned char *, unsigned int *);

ASN1_TIME *X509_gmtime_adj(ASN1_TIME *, long);

unsigned long X509_subject_name_hash(X509 *);

X509_NAME *X509_get_subject_name(X509 *);
int X509_set_subject_name(X509 *, X509_NAME *);

X509_NAME *X509_get_issuer_name(X509 *);
int X509_set_issuer_name(X509 *, X509_NAME *);

int X509_get_ext_count(X509 *);
int X509_add_ext(X509 *, X509_EXTENSION *, int);
X509_EXTENSION *X509_EXTENSION_dup(X509_EXTENSION *);
X509_EXTENSION *X509_get_ext(X509 *, int);
int X509_EXTENSION_get_critical(X509_EXTENSION *);
ASN1_OBJECT *X509_EXTENSION_get_object(X509_EXTENSION *);
void X509_EXTENSION_free(X509_EXTENSION *);

int i2d_X509(X509 *, unsigned char **);

int X509_REQ_set_version(X509_REQ *, long);
X509_REQ *X509_REQ_new(void);
void X509_REQ_free(X509_REQ *);
int X509_REQ_set_pubkey(X509_REQ *, EVP_PKEY *);
int X509_REQ_sign(X509_REQ *, EVP_PKEY *, const EVP_MD *);
int X509_REQ_verify(X509_REQ *, EVP_PKEY *);
int X509_REQ_digest(const X509_REQ *, const EVP_MD *,
                    unsigned char *, unsigned int *);
EVP_PKEY *X509_REQ_get_pubkey(X509_REQ *);
int X509_REQ_print_ex(BIO *, X509_REQ *, unsigned long, unsigned long);

int X509V3_EXT_print(BIO *, X509_EXTENSION *, unsigned long, int);
ASN1_OCTET_STRING *X509_EXTENSION_get_data(X509_EXTENSION *);

X509_REVOKED *X509_REVOKED_new(void);
void X509_REVOKED_free(X509_REVOKED *);

int X509_REVOKED_set_serialNumber(X509_REVOKED *, ASN1_INTEGER *);

int X509_REVOKED_add1_ext_i2d(X509_REVOKED *, int, void *, int, unsigned long);

X509_CRL *d2i_X509_CRL_bio(BIO *, X509_CRL **);
X509_CRL *X509_CRL_new(void);
void X509_CRL_free(X509_CRL *);
int X509_CRL_add0_revoked(X509_CRL *, X509_REVOKED *);
int i2d_X509_CRL_bio(BIO *, X509_CRL *);
int X509_CRL_print(BIO *, X509_CRL *);
int X509_CRL_set_issuer_name(X509_CRL *, X509_NAME *);
int X509_CRL_sign(X509_CRL *, EVP_PKEY *, const EVP_MD *);

int NETSCAPE_SPKI_verify(NETSCAPE_SPKI *, EVP_PKEY *);
int NETSCAPE_SPKI_sign(NETSCAPE_SPKI *, EVP_PKEY *, const EVP_MD *);
char *NETSCAPE_SPKI_b64_encode(NETSCAPE_SPKI *);
NETSCAPE_SPKI *NETSCAPE_SPKI_b64_decode(const char *, int);
EVP_PKEY *NETSCAPE_SPKI_get_pubkey(NETSCAPE_SPKI *);
int NETSCAPE_SPKI_set_pubkey(NETSCAPE_SPKI *, EVP_PKEY *);
NETSCAPE_SPKI *NETSCAPE_SPKI_new(void);
void NETSCAPE_SPKI_free(NETSCAPE_SPKI *);

/*  ASN1 serialization */
int i2d_X509_bio(BIO *, X509 *);
X509 *d2i_X509_bio(BIO *, X509 **);

int i2d_X509_REQ_bio(BIO *, X509_REQ *);
X509_REQ *d2i_X509_REQ_bio(BIO *, X509_REQ **);

int i2d_PrivateKey_bio(BIO *, EVP_PKEY *);
EVP_PKEY *d2i_PrivateKey_bio(BIO *, EVP_PKEY **);
int i2d_PUBKEY_bio(BIO *, EVP_PKEY *);
EVP_PKEY *d2i_PUBKEY_bio(BIO *, EVP_PKEY **);

ASN1_INTEGER *X509_get_serialNumber(X509 *);
int X509_set_serialNumber(X509 *, ASN1_INTEGER *);

const char *X509_verify_cert_error_string(long);

const char *X509_get_default_cert_area(void);
const char *X509_get_default_cert_dir(void);
const char *X509_get_default_cert_file(void);
const char *X509_get_default_cert_dir_env(void);
const char *X509_get_default_cert_file_env(void);
const char *X509_get_default_private_dir(void);

int i2d_RSA_PUBKEY(RSA *, unsigned char **);
RSA *d2i_RSA_PUBKEY(RSA **, const unsigned char **, long);
RSA *d2i_RSAPublicKey(RSA **, const unsigned char **, long);
RSA *d2i_RSAPrivateKey(RSA **, const unsigned char **, long);
int i2d_DSA_PUBKEY(DSA *, unsigned char **);
DSA *d2i_DSA_PUBKEY(DSA **, const unsigned char **, long);
DSA *d2i_DSAPublicKey(DSA **, const unsigned char **, long);
DSA *d2i_DSAPrivateKey(DSA **, const unsigned char **, long);

RSA *d2i_RSAPrivateKey_bio(BIO *, RSA **);
int i2d_RSAPrivateKey_bio(BIO *, RSA *);
RSA *d2i_RSAPublicKey_bio(BIO *, RSA **);
int i2d_RSAPublicKey_bio(BIO *, RSA *);
RSA *d2i_RSA_PUBKEY_bio(BIO *, RSA **);
int i2d_RSA_PUBKEY_bio(BIO *, RSA *);
DSA *d2i_DSA_PUBKEY_bio(BIO *, DSA **);
int i2d_DSA_PUBKEY_bio(BIO *, DSA *);
DSA *d2i_DSAPrivateKey_bio(BIO *, DSA **);
int i2d_DSAPrivateKey_bio(BIO *, DSA *);

PKCS8_PRIV_KEY_INFO *d2i_PKCS8_PRIV_KEY_INFO_bio(BIO *,
                                                 PKCS8_PRIV_KEY_INFO **);
void PKCS8_PRIV_KEY_INFO_free(PKCS8_PRIV_KEY_INFO *);
"""

MACROS = """
long X509_get_version(X509 *);

ASN1_TIME *X509_get_notBefore(X509 *);
ASN1_TIME *X509_get_notAfter(X509 *);

long X509_REQ_get_version(X509_REQ *);
X509_NAME *X509_REQ_get_subject_name(X509_REQ *);

Cryptography_STACK_OF_X509 *sk_X509_new_null(void);
void sk_X509_free(Cryptography_STACK_OF_X509 *);
int sk_X509_num(Cryptography_STACK_OF_X509 *);
int sk_X509_push(Cryptography_STACK_OF_X509 *, X509 *);
X509 *sk_X509_value(Cryptography_STACK_OF_X509 *, int);

X509_EXTENSIONS *sk_X509_EXTENSION_new_null(void);
int sk_X509_EXTENSION_num(X509_EXTENSIONS *);
X509_EXTENSION *sk_X509_EXTENSION_value(X509_EXTENSIONS *, int);
int sk_X509_EXTENSION_push(X509_EXTENSIONS *, X509_EXTENSION *);
X509_EXTENSION *sk_X509_EXTENSION_delete(X509_EXTENSIONS *, int);
void sk_X509_EXTENSION_free(X509_EXTENSIONS *);

int sk_X509_REVOKED_num(Cryptography_STACK_OF_X509_REVOKED *);
X509_REVOKED *sk_X509_REVOKED_value(Cryptography_STACK_OF_X509_REVOKED *, int);

int i2d_RSAPublicKey(RSA *, unsigned char **);
int i2d_RSAPrivateKey(RSA *, unsigned char **);
int i2d_DSAPublicKey(DSA *, unsigned char **);
int i2d_DSAPrivateKey(DSA *, unsigned char **);

/* These aren't macros these arguments are all const X on openssl > 1.0.x */
int X509_CRL_set_lastUpdate(X509_CRL *, ASN1_TIME *);
int X509_CRL_set_nextUpdate(X509_CRL *, ASN1_TIME *);
int X509_set_notBefore(X509 *, ASN1_UTCTIME *);
int X509_set_notAfter(X509 *, ASN1_UTCTIME *);

/* These use STACK_OF(X509_EXTENSION) in 0.9.8e. Once we drop support for
   RHEL/CentOS 5 we should move these back to FUNCTIONS. */
int X509_REQ_add_extensions(X509_REQ *, X509_EXTENSIONS *);
X509_EXTENSIONS *X509_REQ_get_extensions(X509_REQ *);

int i2d_EC_PUBKEY(EC_KEY *, unsigned char **);
EC_KEY *d2i_EC_PUBKEY(EC_KEY **, const unsigned char **, long);
EC_KEY *d2i_EC_PUBKEY_bio(BIO *, EC_KEY **);
int i2d_EC_PUBKEY_bio(BIO *, EC_KEY *);
EC_KEY *d2i_ECPrivateKey_bio(BIO *, EC_KEY **);
int i2d_ECPrivateKey_bio(BIO *, EC_KEY *);
"""

CUSTOMIZATIONS = """
/* OpenSSL 0.9.8e does not have this definition. */
#if OPENSSL_VERSION_NUMBER <= 0x0090805fL
typedef STACK_OF(X509_EXTENSION) X509_EXTENSIONS;
#endif
#ifdef OPENSSL_NO_EC
int (*i2d_EC_PUBKEY)(EC_KEY *, unsigned char **) = NULL;
EC_KEY *(*d2i_EC_PUBKEY)(EC_KEY **, const unsigned char **, long) = NULL;
EC_KEY *(*d2i_EC_PUBKEY_bio)(BIO *, EC_KEY **) = NULL;
int (*i2d_EC_PUBKEY_bio)(BIO *, EC_KEY *) = NULL;
EC_KEY *(*d2i_ECPrivateKey_bio)(BIO *, EC_KEY **) = NULL;
int (*i2d_ECPrivateKey_bio)(BIO *, EC_KEY *) = NULL;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_EC": [
        "i2d_EC_PUBKEY",
        "d2i_EC_PUBKEY",
        "d2i_EC_PUBKEY_bio",
        "i2d_EC_PUBKEY_bio",
        "d2i_ECPrivateKey_bio",
        "i2d_ECPrivateKey_bio",
    ]
}
