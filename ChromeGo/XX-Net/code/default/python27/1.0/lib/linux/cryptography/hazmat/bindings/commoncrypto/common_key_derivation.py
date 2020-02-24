# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <CommonCrypto/CommonKeyDerivation.h>
"""

TYPES = """
enum {
    kCCPBKDF2 = 2,
};
typedef uint32_t CCPBKDFAlgorithm;
enum {
    kCCPRFHmacAlgSHA1 = 1,
    kCCPRFHmacAlgSHA224 = 2,
    kCCPRFHmacAlgSHA256 = 3,
    kCCPRFHmacAlgSHA384 = 4,
    kCCPRFHmacAlgSHA512 = 5,
};
typedef uint32_t CCPseudoRandomAlgorithm;
typedef unsigned int uint;
"""

FUNCTIONS = """
int CCKeyDerivationPBKDF(CCPBKDFAlgorithm, const char *, size_t,
                         const uint8_t *, size_t, CCPseudoRandomAlgorithm,
                         uint, uint8_t *, size_t);
uint CCCalibratePBKDF(CCPBKDFAlgorithm, size_t, size_t,
                      CCPseudoRandomAlgorithm, size_t, uint32_t);
"""

MACROS = """
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
