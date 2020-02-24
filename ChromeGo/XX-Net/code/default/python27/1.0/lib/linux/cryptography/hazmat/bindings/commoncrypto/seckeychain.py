# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <Security/SecKeychain.h>
"""

TYPES = """
typedef ... *SecKeychainRef;
"""

FUNCTIONS = """
OSStatus SecKeychainCreate(const char *, UInt32, const void *, Boolean,
                           SecAccessRef, SecKeychainRef *);
OSStatus SecKeychainDelete(SecKeychainRef);
"""

MACROS = """
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
