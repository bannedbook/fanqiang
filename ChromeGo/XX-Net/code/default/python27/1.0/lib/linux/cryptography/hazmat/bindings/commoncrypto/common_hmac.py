# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <CommonCrypto/CommonHMAC.h>
"""

TYPES = """
typedef struct {
    ...;
} CCHmacContext;
enum {
    kCCHmacAlgSHA1,
    kCCHmacAlgMD5,
    kCCHmacAlgSHA256,
    kCCHmacAlgSHA384,
    kCCHmacAlgSHA512,
    kCCHmacAlgSHA224
};
typedef uint32_t CCHmacAlgorithm;
"""

FUNCTIONS = """
void CCHmacInit(CCHmacContext *, CCHmacAlgorithm, const void *, size_t);
void CCHmacUpdate(CCHmacContext *, const void *, size_t);
void CCHmacFinal(CCHmacContext *, void *);

"""

MACROS = """
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
