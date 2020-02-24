# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <CommonCrypto/CommonCryptor.h>
"""

TYPES = """
enum {
    kCCAlgorithmAES128 = 0,
    kCCAlgorithmDES,
    kCCAlgorithm3DES,
    kCCAlgorithmCAST,
    kCCAlgorithmRC4,
    kCCAlgorithmRC2,
    kCCAlgorithmBlowfish
};
typedef uint32_t CCAlgorithm;
enum {
    kCCSuccess = 0,
    kCCParamError = -4300,
    kCCBufferTooSmall = -4301,
    kCCMemoryFailure = -4302,
    kCCAlignmentError = -4303,
    kCCDecodeError = -4304,
    kCCUnimplemented = -4305
};
typedef int32_t CCCryptorStatus;
typedef uint32_t CCOptions;
enum {
    kCCEncrypt = 0,
    kCCDecrypt,
};
typedef uint32_t CCOperation;
typedef ... *CCCryptorRef;

enum {
    kCCModeOptionCTR_LE = 0x0001,
    kCCModeOptionCTR_BE = 0x0002
};

typedef uint32_t CCModeOptions;

enum {
    kCCModeECB = 1,
    kCCModeCBC = 2,
    kCCModeCFB = 3,
    kCCModeCTR = 4,
    kCCModeF8 = 5,
    kCCModeLRW = 6,
    kCCModeOFB = 7,
    kCCModeXTS = 8,
    kCCModeRC4 = 9,
    kCCModeCFB8 = 10,
    kCCModeGCM = 11
};
typedef uint32_t CCMode;
enum {
    ccNoPadding = 0,
    ccPKCS7Padding = 1,
};
typedef uint32_t CCPadding;
"""

FUNCTIONS = """
CCCryptorStatus CCCryptorCreateWithMode(CCOperation, CCMode, CCAlgorithm,
                                        CCPadding, const void *, const void *,
                                        size_t, const void *, size_t, int,
                                        CCModeOptions, CCCryptorRef *);
CCCryptorStatus CCCryptorCreate(CCOperation, CCAlgorithm, CCOptions,
                                const void *, size_t, const void *,
                                CCCryptorRef *);
CCCryptorStatus CCCryptorUpdate(CCCryptorRef, const void *, size_t, void *,
                                size_t, size_t *);
CCCryptorStatus CCCryptorFinal(CCCryptorRef, void *, size_t, size_t *);
CCCryptorStatus CCCryptorRelease(CCCryptorRef);

CCCryptorStatus CCCryptorGCMAddIV(CCCryptorRef, const void *, size_t);
CCCryptorStatus CCCryptorGCMAddAAD(CCCryptorRef, const void *, size_t);
CCCryptorStatus CCCryptorGCMEncrypt(CCCryptorRef, const void *, size_t,
                                    void *);
CCCryptorStatus CCCryptorGCMDecrypt(CCCryptorRef, const void *, size_t,
                                    void *);
CCCryptorStatus CCCryptorGCMFinal(CCCryptorRef, const void *, size_t *);
CCCryptorStatus CCCryptorGCMReset(CCCryptorRef);
"""

MACROS = """
"""

CUSTOMIZATIONS = """
/* Not defined in the public header */
enum {
    kCCModeGCM = 11
};
"""

CONDITIONAL_NAMES = {}
