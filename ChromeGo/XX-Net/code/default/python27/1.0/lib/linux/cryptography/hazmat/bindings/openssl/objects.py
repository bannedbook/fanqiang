# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/objects.h>
"""

TYPES = """
"""

FUNCTIONS = """
ASN1_OBJECT *OBJ_nid2obj(int);
const char *OBJ_nid2ln(int);
const char *OBJ_nid2sn(int);
int OBJ_obj2nid(const ASN1_OBJECT *);
int OBJ_ln2nid(const char *);
int OBJ_sn2nid(const char *);
int OBJ_txt2nid(const char *);
ASN1_OBJECT *OBJ_txt2obj(const char *, int);
int OBJ_obj2txt(char *, int, const ASN1_OBJECT *, int);
int OBJ_cmp(const ASN1_OBJECT *, const ASN1_OBJECT *);
ASN1_OBJECT *OBJ_dup(const ASN1_OBJECT *);
int OBJ_create(const char *, const char *, const char *);
void OBJ_cleanup(void);
"""

MACROS = """
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
