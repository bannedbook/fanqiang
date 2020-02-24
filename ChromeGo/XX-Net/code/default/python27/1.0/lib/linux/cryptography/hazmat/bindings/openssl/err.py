# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/err.h>
"""

TYPES = """
static const int Cryptography_HAS_REMOVE_THREAD_STATE;
static const int Cryptography_HAS_098H_ERROR_CODES;
static const int Cryptography_HAS_098C_CAMELLIA_CODES;
static const int Cryptography_HAS_EC_CODES;
static const int Cryptography_HAS_RSA_R_PKCS_DECODING_ERROR;

struct ERR_string_data_st {
    unsigned long error;
    const char *string;
};
typedef struct ERR_string_data_st ERR_STRING_DATA;

static const int ERR_LIB_DH;
static const int ERR_LIB_EVP;
static const int ERR_LIB_EC;
static const int ERR_LIB_PEM;
static const int ERR_LIB_ASN1;
static const int ERR_LIB_RSA;
static const int ERR_LIB_PKCS12;

static const int ASN1_F_ASN1_ENUMERATED_TO_BN;
static const int ASN1_F_ASN1_EX_C2I;
static const int ASN1_F_ASN1_FIND_END;
static const int ASN1_F_ASN1_GENERALIZEDTIME_SET;
static const int ASN1_F_ASN1_GENERATE_V3;
static const int ASN1_F_ASN1_GET_OBJECT;
static const int ASN1_F_ASN1_ITEM_I2D_FP;
static const int ASN1_F_ASN1_ITEM_PACK;
static const int ASN1_F_ASN1_ITEM_SIGN;
static const int ASN1_F_ASN1_ITEM_UNPACK;
static const int ASN1_F_ASN1_ITEM_VERIFY;
static const int ASN1_F_ASN1_MBSTRING_NCOPY;
static const int ASN1_F_ASN1_TEMPLATE_EX_D2I;
static const int ASN1_F_ASN1_TEMPLATE_NEW;
static const int ASN1_F_ASN1_TEMPLATE_NOEXP_D2I;
static const int ASN1_F_ASN1_TIME_SET;
static const int ASN1_F_ASN1_TYPE_GET_INT_OCTETSTRING;
static const int ASN1_F_ASN1_TYPE_GET_OCTETSTRING;
static const int ASN1_F_ASN1_UNPACK_STRING;
static const int ASN1_F_ASN1_UTCTIME_SET;
static const int ASN1_F_ASN1_VERIFY;
static const int ASN1_F_BITSTR_CB;
static const int ASN1_F_BN_TO_ASN1_ENUMERATED;
static const int ASN1_F_BN_TO_ASN1_INTEGER;
static const int ASN1_F_D2I_ASN1_TYPE_BYTES;
static const int ASN1_F_D2I_ASN1_UINTEGER;
static const int ASN1_F_D2I_ASN1_UTCTIME;
static const int ASN1_F_D2I_NETSCAPE_RSA;
static const int ASN1_F_D2I_NETSCAPE_RSA_2;
static const int ASN1_F_D2I_PRIVATEKEY;
static const int ASN1_F_D2I_X509;
static const int ASN1_F_D2I_X509_CINF;
static const int ASN1_F_D2I_X509_PKEY;
static const int ASN1_F_I2D_ASN1_SET;
static const int ASN1_F_I2D_ASN1_TIME;
static const int ASN1_F_I2D_DSA_PUBKEY;
static const int ASN1_F_LONG_C2I;
static const int ASN1_F_OID_MODULE_INIT;
static const int ASN1_F_PARSE_TAGGING;
static const int ASN1_F_PKCS5_PBE_SET;
static const int ASN1_F_X509_CINF_NEW;

static const int ASN1_R_BOOLEAN_IS_WRONG_LENGTH;
static const int ASN1_R_BUFFER_TOO_SMALL;
static const int ASN1_R_CIPHER_HAS_NO_OBJECT_IDENTIFIER;
static const int ASN1_R_DATA_IS_WRONG;
static const int ASN1_R_DECODE_ERROR;
static const int ASN1_R_DECODING_ERROR;
static const int ASN1_R_DEPTH_EXCEEDED;
static const int ASN1_R_ENCODE_ERROR;
static const int ASN1_R_ERROR_GETTING_TIME;
static const int ASN1_R_ERROR_LOADING_SECTION;
static const int ASN1_R_MSTRING_WRONG_TAG;
static const int ASN1_R_NESTED_ASN1_STRING;
static const int ASN1_R_NO_MATCHING_CHOICE_TYPE;
static const int ASN1_R_UNKNOWN_MESSAGE_DIGEST_ALGORITHM;
static const int ASN1_R_UNKNOWN_OBJECT_TYPE;
static const int ASN1_R_UNKNOWN_PUBLIC_KEY_TYPE;
static const int ASN1_R_UNKNOWN_TAG;
static const int ASN1_R_UNKOWN_FORMAT;
static const int ASN1_R_UNSUPPORTED_ANY_DEFINED_BY_TYPE;
static const int ASN1_R_UNSUPPORTED_ENCRYPTION_ALGORITHM;
static const int ASN1_R_UNSUPPORTED_PUBLIC_KEY_TYPE;
static const int ASN1_R_UNSUPPORTED_TYPE;
static const int ASN1_R_WRONG_TAG;
static const int ASN1_R_WRONG_TYPE;

static const int DH_F_COMPUTE_KEY;

static const int DH_R_INVALID_PUBKEY;

static const int EVP_F_AES_INIT_KEY;
static const int EVP_F_D2I_PKEY;
static const int EVP_F_DSA_PKEY2PKCS8;
static const int EVP_F_DSAPKEY2PKCS8;
static const int EVP_F_ECDSA_PKEY2PKCS8;
static const int EVP_F_ECKEY_PKEY2PKCS8;
static const int EVP_F_EVP_CIPHER_CTX_CTRL;
static const int EVP_F_EVP_CIPHER_CTX_SET_KEY_LENGTH;
static const int EVP_F_EVP_CIPHERINIT_EX;
static const int EVP_F_EVP_DECRYPTFINAL_EX;
static const int EVP_F_EVP_DIGESTINIT_EX;
static const int EVP_F_EVP_ENCRYPTFINAL_EX;
static const int EVP_F_EVP_MD_CTX_COPY_EX;
static const int EVP_F_EVP_OPENINIT;
static const int EVP_F_EVP_PBE_ALG_ADD;
static const int EVP_F_EVP_PBE_CIPHERINIT;
static const int EVP_F_EVP_PKCS82PKEY;
static const int EVP_F_EVP_PKEY2PKCS8_BROKEN;
static const int EVP_F_EVP_PKEY_COPY_PARAMETERS;
static const int EVP_F_EVP_PKEY_DECRYPT;
static const int EVP_F_EVP_PKEY_ENCRYPT;
static const int EVP_F_EVP_PKEY_GET1_DH;
static const int EVP_F_EVP_PKEY_GET1_DSA;
static const int EVP_F_EVP_PKEY_GET1_ECDSA;
static const int EVP_F_EVP_PKEY_GET1_EC_KEY;
static const int EVP_F_EVP_PKEY_GET1_RSA;
static const int EVP_F_EVP_PKEY_NEW;
static const int EVP_F_EVP_RIJNDAEL;
static const int EVP_F_EVP_SIGNFINAL;
static const int EVP_F_EVP_VERIFYFINAL;
static const int EVP_F_PKCS5_PBE_KEYIVGEN;
static const int EVP_F_PKCS5_V2_PBE_KEYIVGEN;
static const int EVP_F_PKCS8_SET_BROKEN;
static const int EVP_F_RC2_MAGIC_TO_METH;
static const int EVP_F_RC5_CTRL;

static const int EVP_R_AES_KEY_SETUP_FAILED;
static const int EVP_R_ASN1_LIB;
static const int EVP_R_BAD_BLOCK_LENGTH;
static const int EVP_R_BAD_DECRYPT;
static const int EVP_R_BAD_KEY_LENGTH;
static const int EVP_R_BN_DECODE_ERROR;
static const int EVP_R_BN_PUBKEY_ERROR;
static const int EVP_R_CIPHER_PARAMETER_ERROR;
static const int EVP_R_CTRL_NOT_IMPLEMENTED;
static const int EVP_R_CTRL_OPERATION_NOT_IMPLEMENTED;
static const int EVP_R_DATA_NOT_MULTIPLE_OF_BLOCK_LENGTH;
static const int EVP_R_DECODE_ERROR;
static const int EVP_R_DIFFERENT_KEY_TYPES;
static const int EVP_R_ENCODE_ERROR;
static const int EVP_R_INITIALIZATION_ERROR;
static const int EVP_R_INPUT_NOT_INITIALIZED;
static const int EVP_R_INVALID_KEY_LENGTH;
static const int EVP_R_IV_TOO_LARGE;
static const int EVP_R_KEYGEN_FAILURE;
static const int EVP_R_MISSING_PARAMETERS;
static const int EVP_R_NO_CIPHER_SET;
static const int EVP_R_NO_DIGEST_SET;
static const int EVP_R_NO_DSA_PARAMETERS;
static const int EVP_R_NO_SIGN_FUNCTION_CONFIGURED;
static const int EVP_R_NO_VERIFY_FUNCTION_CONFIGURED;
static const int EVP_R_PKCS8_UNKNOWN_BROKEN_TYPE;
static const int EVP_R_PUBLIC_KEY_NOT_RSA;
static const int EVP_R_UNKNOWN_PBE_ALGORITHM;
static const int EVP_R_UNSUPORTED_NUMBER_OF_ROUNDS;
static const int EVP_R_UNSUPPORTED_CIPHER;
static const int EVP_R_UNSUPPORTED_KEY_DERIVATION_FUNCTION;
static const int EVP_R_UNSUPPORTED_KEYLENGTH;
static const int EVP_R_UNSUPPORTED_SALT_TYPE;
static const int EVP_R_UNSUPPORTED_PRIVATE_KEY_ALGORITHM;
static const int EVP_R_WRONG_FINAL_BLOCK_LENGTH;
static const int EVP_R_WRONG_PUBLIC_KEY_TYPE;

static const int EC_F_EC_GROUP_NEW_BY_CURVE_NAME;

static const int EC_R_UNKNOWN_GROUP;

static const int PEM_F_D2I_PKCS8PRIVATEKEY_BIO;
static const int PEM_F_D2I_PKCS8PRIVATEKEY_FP;
static const int PEM_F_DO_PK8PKEY;
static const int PEM_F_DO_PK8PKEY_FP;
static const int PEM_F_LOAD_IV;
static const int PEM_F_PEM_ASN1_READ;
static const int PEM_F_PEM_ASN1_READ_BIO;
static const int PEM_F_PEM_ASN1_WRITE;
static const int PEM_F_PEM_ASN1_WRITE_BIO;
static const int PEM_F_PEM_DEF_CALLBACK;
static const int PEM_F_PEM_DO_HEADER;
static const int PEM_F_PEM_F_PEM_WRITE_PKCS8PRIVATEKEY;
static const int PEM_F_PEM_GET_EVP_CIPHER_INFO;
static const int PEM_F_PEM_PK8PKEY;
static const int PEM_F_PEM_READ;
static const int PEM_F_PEM_READ_BIO;
static const int PEM_F_PEM_READ_BIO_PRIVATEKEY;
static const int PEM_F_PEM_READ_PRIVATEKEY;
static const int PEM_F_PEM_SEALFINAL;
static const int PEM_F_PEM_SEALINIT;
static const int PEM_F_PEM_SIGNFINAL;
static const int PEM_F_PEM_WRITE;
static const int PEM_F_PEM_WRITE_BIO;
static const int PEM_F_PEM_X509_INFO_READ;
static const int PEM_F_PEM_X509_INFO_READ_BIO;
static const int PEM_F_PEM_X509_INFO_WRITE_BIO;

static const int PEM_R_BAD_BASE64_DECODE;
static const int PEM_R_BAD_DECRYPT;
static const int PEM_R_BAD_END_LINE;
static const int PEM_R_BAD_IV_CHARS;
static const int PEM_R_BAD_PASSWORD_READ;
static const int PEM_R_ERROR_CONVERTING_PRIVATE_KEY;
static const int PEM_R_NO_START_LINE;
static const int PEM_R_NOT_DEK_INFO;
static const int PEM_R_NOT_ENCRYPTED;
static const int PEM_R_NOT_PROC_TYPE;
static const int PEM_R_PROBLEMS_GETTING_PASSWORD;
static const int PEM_R_PUBLIC_KEY_NO_RSA;
static const int PEM_R_READ_KEY;
static const int PEM_R_SHORT_HEADER;
static const int PEM_R_UNSUPPORTED_CIPHER;
static const int PEM_R_UNSUPPORTED_ENCRYPTION;

static const int PKCS12_F_PKCS12_PBE_CRYPT;

static const int PKCS12_R_PKCS12_CIPHERFINAL_ERROR;

static const int RSA_R_DATA_TOO_LARGE_FOR_KEY_SIZE;
static const int RSA_R_DIGEST_TOO_BIG_FOR_RSA_KEY;
static const int RSA_R_BLOCK_TYPE_IS_NOT_01;
static const int RSA_R_BLOCK_TYPE_IS_NOT_02;
static const int RSA_R_PKCS_DECODING_ERROR;
"""

FUNCTIONS = """
void ERR_load_crypto_strings(void);
void ERR_load_SSL_strings(void);
void ERR_free_strings(void);
char *ERR_error_string(unsigned long, char *);
void ERR_error_string_n(unsigned long, char *, size_t);
const char *ERR_lib_error_string(unsigned long);
const char *ERR_func_error_string(unsigned long);
const char *ERR_reason_error_string(unsigned long);
void ERR_print_errors(BIO *);
void ERR_print_errors_fp(FILE *);
unsigned long ERR_get_error(void);
unsigned long ERR_peek_error(void);
unsigned long ERR_peek_last_error(void);
unsigned long ERR_get_error_line(const char **, int *);
unsigned long ERR_peek_error_line(const char **, int *);
unsigned long ERR_peek_last_error_line(const char **, int *);
unsigned long ERR_get_error_line_data(const char **, int *,
                                      const char **, int *);
unsigned long ERR_peek_error_line_data(const char **,
                                       int *, const char **, int *);
unsigned long ERR_peek_last_error_line_data(const char **,
                                            int *, const char **, int *);
void ERR_put_error(int, int, int, const char *, int);
void ERR_add_error_data(int, ...);
int ERR_get_next_error_library(void);
"""

MACROS = """
unsigned long ERR_PACK(int, int, int);
int ERR_GET_LIB(unsigned long);
int ERR_GET_FUNC(unsigned long);
int ERR_GET_REASON(unsigned long);
int ERR_FATAL_ERROR(unsigned long);
/* introduced in 1.0.0 so we have to handle this specially to continue
 * supporting 0.9.8
 */
void ERR_remove_thread_state(const CRYPTO_THREADID *);

/* These were added in OpenSSL 0.9.8h. When we drop support for RHEL/CentOS 5
   we should be able to move these back to TYPES. */
static const int ASN1_F_B64_READ_ASN1;
static const int ASN1_F_B64_WRITE_ASN1;
static const int ASN1_F_SMIME_READ_ASN1;
static const int ASN1_F_SMIME_TEXT;
static const int ASN1_R_NO_CONTENT_TYPE;
static const int ASN1_R_NO_MULTIPART_BODY_FAILURE;
static const int ASN1_R_NO_MULTIPART_BOUNDARY;
/* These were added in OpenSSL 0.9.8c. */
static const int EVP_F_CAMELLIA_INIT_KEY;
static const int EVP_R_CAMELLIA_KEY_SETUP_FAILED;
"""

CUSTOMIZATIONS = """
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
static const long Cryptography_HAS_REMOVE_THREAD_STATE = 1;
#else
static const long Cryptography_HAS_REMOVE_THREAD_STATE = 0;
typedef uint32_t CRYPTO_THREADID;
void (*ERR_remove_thread_state)(const CRYPTO_THREADID *) = NULL;
#endif

/* OpenSSL 0.9.8h+ */
#if OPENSSL_VERSION_NUMBER >= 0x0090808fL
static const long Cryptography_HAS_098H_ERROR_CODES = 1;
#else
static const long Cryptography_HAS_098H_ERROR_CODES = 0;
static const int ASN1_F_B64_READ_ASN1 = 0;
static const int ASN1_F_B64_WRITE_ASN1 = 0;
static const int ASN1_F_SMIME_READ_ASN1 = 0;
static const int ASN1_F_SMIME_TEXT = 0;
static const int ASN1_R_NO_CONTENT_TYPE = 0;
static const int ASN1_R_NO_MULTIPART_BODY_FAILURE = 0;
static const int ASN1_R_NO_MULTIPART_BOUNDARY = 0;
#endif

/* OpenSSL 0.9.8c+ */
#ifdef EVP_F_CAMELLIA_INIT_KEY
static const long Cryptography_HAS_098C_CAMELLIA_CODES = 1;
#else
static const long Cryptography_HAS_098C_CAMELLIA_CODES = 0;
static const int EVP_F_CAMELLIA_INIT_KEY = 0;
static const int EVP_R_CAMELLIA_KEY_SETUP_FAILED = 0;
#endif

// OpenSSL without EC. e.g. RHEL
#ifndef OPENSSL_NO_EC
static const long Cryptography_HAS_EC_CODES = 1;
#else
static const long Cryptography_HAS_EC_CODES = 0;
static const int EC_R_UNKNOWN_GROUP = 0;
static const int EC_F_EC_GROUP_NEW_BY_CURVE_NAME = 0;
#endif

#ifdef RSA_R_PKCS_DECODING_ERROR
static const long Cryptography_HAS_RSA_R_PKCS_DECODING_ERROR = 1;
#else
static const long Cryptography_HAS_RSA_R_PKCS_DECODING_ERROR = 0;
static const long RSA_R_PKCS_DECODING_ERROR = 0;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_REMOVE_THREAD_STATE": [
        "ERR_remove_thread_state"
    ],
    "Cryptography_HAS_098H_ERROR_CODES": [
        "ASN1_F_B64_READ_ASN1",
        "ASN1_F_B64_WRITE_ASN1",
        "ASN1_F_SMIME_READ_ASN1",
        "ASN1_F_SMIME_TEXT",
        "ASN1_R_NO_CONTENT_TYPE",
        "ASN1_R_NO_MULTIPART_BODY_FAILURE",
        "ASN1_R_NO_MULTIPART_BOUNDARY",
    ],
    "Cryptography_HAS_098C_CAMELLIA_CODES": [
        "EVP_F_CAMELLIA_INIT_KEY",
        "EVP_R_CAMELLIA_KEY_SETUP_FAILED"
    ],
    "Cryptography_HAS_EC_CODES": [
        "EC_R_UNKNOWN_GROUP",
        "EC_F_EC_GROUP_NEW_BY_CURVE_NAME"
    ],
    "Cryptography_HAS_RSA_R_PKCS_DECODING_ERROR": [
        "RSA_R_PKCS_DECODING_ERROR"
    ]
}
