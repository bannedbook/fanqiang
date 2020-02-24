# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
#include <openssl/cmac.h>
#endif
"""

TYPES = """
static const int Cryptography_HAS_CMAC;
typedef ... CMAC_CTX;
"""

FUNCTIONS = """
"""

MACROS = """
CMAC_CTX *CMAC_CTX_new(void);
int CMAC_Init(CMAC_CTX *, const void *, size_t, const EVP_CIPHER *, ENGINE *);
int CMAC_Update(CMAC_CTX *, const void *, size_t);
int CMAC_Final(CMAC_CTX *, unsigned char *, size_t *);
int CMAC_CTX_copy(CMAC_CTX *, const CMAC_CTX *);
void CMAC_CTX_free(CMAC_CTX *);
"""

CUSTOMIZATIONS = """
#if OPENSSL_VERSION_NUMBER < 0x10001000L

static const long Cryptography_HAS_CMAC = 0;
typedef void CMAC_CTX;
CMAC_CTX *(*CMAC_CTX_new)(void) = NULL;
int (*CMAC_Init)(CMAC_CTX *, const void *, size_t, const EVP_CIPHER *,
    ENGINE *) = NULL;
int (*CMAC_Update)(CMAC_CTX *, const void *, size_t) = NULL;
int (*CMAC_Final)(CMAC_CTX *, unsigned char *, size_t *) = NULL;
int (*CMAC_CTX_copy)(CMAC_CTX *, const CMAC_CTX *) = NULL;
void (*CMAC_CTX_free)(CMAC_CTX *) = NULL;
#else
static const long Cryptography_HAS_CMAC = 1;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_CMAC": [
        "CMAC_CTX_new",
        "CMAC_Init",
        "CMAC_Update",
        "CMAC_Final",
        "CMAC_CTX_copy",
        "CMAC_CTX_free",
    ],
}
