# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <Security/SecDigestTransform.h>
#include <Security/SecSignVerifyTransform.h>
#include <Security/SecEncryptTransform.h>
"""

TYPES = """
typedef ... *SecTransformRef;

CFStringRef kSecImportExportPassphrase;
CFStringRef kSecImportExportKeychain;
CFStringRef kSecImportExportAccess;

CFStringRef kSecEncryptionMode;
CFStringRef kSecEncryptKey;
CFStringRef kSecIVKey;
CFStringRef kSecModeCBCKey;
CFStringRef kSecModeCFBKey;
CFStringRef kSecModeECBKey;
CFStringRef kSecModeNoneKey;
CFStringRef kSecModeOFBKey;
CFStringRef kSecOAEPEncodingParametersAttributeName;
CFStringRef kSecPaddingKey;
CFStringRef kSecPaddingNoneKey;
CFStringRef kSecPaddingOAEPKey;
CFStringRef kSecPaddingPKCS1Key;
CFStringRef kSecPaddingPKCS5Key;
CFStringRef kSecPaddingPKCS7Key;

const CFStringRef kSecTransformInputAttributeName;
const CFStringRef kSecTransformOutputAttributeName;
const CFStringRef kSecTransformDebugAttributeName;
const CFStringRef kSecTransformTransformName;
const CFStringRef kSecTransformAbortAttributeName;

CFStringRef kSecInputIsAttributeName;
CFStringRef kSecInputIsPlainText;
CFStringRef kSecInputIsDigest;
CFStringRef kSecInputIsRaw;

const CFStringRef kSecDigestTypeAttribute;
const CFStringRef kSecDigestLengthAttribute;
const CFStringRef kSecDigestMD5;
const CFStringRef kSecDigestSHA1;
const CFStringRef kSecDigestSHA2;
"""

FUNCTIONS = """
Boolean SecTransformSetAttribute(SecTransformRef, CFStringRef, CFTypeRef,
                                 CFErrorRef *);
SecTransformRef SecDecryptTransformCreate(SecKeyRef, CFErrorRef *);
SecTransformRef SecEncryptTransformCreate(SecKeyRef, CFErrorRef *);
SecTransformRef SecVerifyTransformCreate(SecKeyRef, CFDataRef, CFErrorRef *);
SecTransformRef SecSignTransformCreate(SecKeyRef, CFErrorRef *) ;
CFTypeRef SecTransformExecute(SecTransformRef, CFErrorRef *);
"""

MACROS = """
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
