# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <Security/SecImportExport.h>
"""

TYPES = """
typedef ... *SecAccessRef;

CFStringRef kSecImportExportPassphrase;
CFStringRef kSecImportExportKeychain;
CFStringRef kSecImportExportAccess;

typedef uint32_t SecExternalItemType;
enum {
    kSecItemTypeUnknown,
    kSecItemTypePrivateKey,
    kSecItemTypePublicKey,
    kSecItemTypeSessionKey,
    kSecItemTypeCertificate,
    kSecItemTypeAggregate
};


typedef uint32_t SecExternalFormat;
enum {
    kSecFormatUnknown = 0,
    kSecFormatOpenSSL,
    kSecFormatSSH,
    kSecFormatBSAFE,
    kSecFormatRawKey,
    kSecFormatWrappedPKCS8,
    kSecFormatWrappedOpenSSL,
    kSecFormatWrappedSSH,
    kSecFormatWrappedLSH,
    kSecFormatX509Cert,
    kSecFormatPEMSequence,
    kSecFormatPKCS7,
    kSecFormatPKCS12,
    kSecFormatNetscapeCertSequence,
    kSecFormatSSHv2
};

typedef uint32_t SecItemImportExportFlags;
enum {
    kSecKeyImportOnlyOne        = 0x00000001,
    kSecKeySecurePassphrase     = 0x00000002,
    kSecKeyNoAccessControl      = 0x00000004
};
typedef uint32_t SecKeyImportExportFlags;

typedef struct {
    /* for import and export */
    uint32_t version;
    SecKeyImportExportFlags  flags;
    CFTypeRef                passphrase;
    CFStringRef              alertTitle;
    CFStringRef              alertPrompt;

    /* for import only */
    SecAccessRef             accessRef;
    CFArrayRef               keyUsage;

    CFArrayRef               keyAttributes;
} SecItemImportExportKeyParameters;
"""

FUNCTIONS = """
OSStatus SecItemImport(CFDataRef, CFStringRef, SecExternalFormat *,
                       SecExternalItemType *, SecItemImportExportFlags,
                       const SecItemImportExportKeyParameters *,
                       SecKeychainRef, CFArrayRef *);
OSStatus SecPKCS12Import(CFDataRef, CFDictionaryRef, CFArrayRef *);
OSStatus SecItemExport(CFTypeRef, SecExternalFormat, SecItemImportExportFlags,
                       const SecItemImportExportKeyParameters *, CFDataRef *);
"""

MACROS = """
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
