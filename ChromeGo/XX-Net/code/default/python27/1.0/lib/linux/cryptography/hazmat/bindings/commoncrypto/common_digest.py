# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <CommonCrypto/CommonDigest.h>
"""

TYPES = """
typedef uint32_t CC_LONG;
typedef uint64_t CC_LONG64;
typedef struct CC_MD5state_st {
    ...;
} CC_MD5_CTX;
typedef struct CC_SHA1state_st {
    ...;
} CC_SHA1_CTX;
typedef struct CC_SHA256state_st {
    ...;
} CC_SHA256_CTX;
typedef struct CC_SHA512state_st {
    ...;
} CC_SHA512_CTX;
"""

FUNCTIONS = """
int CC_MD5_Init(CC_MD5_CTX *);
int CC_MD5_Update(CC_MD5_CTX *, const void *, CC_LONG);
int CC_MD5_Final(unsigned char *, CC_MD5_CTX *);

int CC_SHA1_Init(CC_SHA1_CTX *);
int CC_SHA1_Update(CC_SHA1_CTX *, const void *, CC_LONG);
int CC_SHA1_Final(unsigned char *, CC_SHA1_CTX *);

int CC_SHA224_Init(CC_SHA256_CTX *);
int CC_SHA224_Update(CC_SHA256_CTX *, const void *, CC_LONG);
int CC_SHA224_Final(unsigned char *, CC_SHA256_CTX *);

int CC_SHA256_Init(CC_SHA256_CTX *);
int CC_SHA256_Update(CC_SHA256_CTX *, const void *, CC_LONG);
int CC_SHA256_Final(unsigned char *, CC_SHA256_CTX *);

int CC_SHA384_Init(CC_SHA512_CTX *);
int CC_SHA384_Update(CC_SHA512_CTX *, const void *, CC_LONG);
int CC_SHA384_Final(unsigned char *, CC_SHA512_CTX *);

int CC_SHA512_Init(CC_SHA512_CTX *);
int CC_SHA512_Update(CC_SHA512_CTX *, const void *, CC_LONG);
int CC_SHA512_Final(unsigned char *, CC_SHA512_CTX *);
"""

MACROS = """
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
