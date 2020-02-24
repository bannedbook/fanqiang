# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/opensslv.h>
"""

TYPES = """
/* Note that these will be resolved when cryptography is compiled and are NOT
   guaranteed to be the version that it actually loads. */
static const int OPENSSL_VERSION_NUMBER;
static const char *const OPENSSL_VERSION_TEXT;
"""

FUNCTIONS = """
"""

MACROS = """
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
