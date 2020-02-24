# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/conf.h>
"""

TYPES = """
typedef ... CONF;
"""

FUNCTIONS = """
void OPENSSL_config(const char *);
void OPENSSL_no_config(void);
"""

MACROS = """
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
