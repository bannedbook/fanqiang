# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/x509.h>

/*
 * See the comment above Cryptography_STACK_OF_X509 in x509.py
 */
typedef STACK_OF(X509_NAME) Cryptography_STACK_OF_X509_NAME;
"""

TYPES = """
typedef ... X509_NAME;
typedef ... X509_NAME_ENTRY;
typedef ... Cryptography_STACK_OF_X509_NAME;
"""

FUNCTIONS = """
X509_NAME *X509_NAME_new(void);
void X509_NAME_free(X509_NAME *);

int X509_NAME_entry_count(X509_NAME *);
X509_NAME_ENTRY *X509_NAME_get_entry(X509_NAME *, int);
ASN1_OBJECT *X509_NAME_ENTRY_get_object(X509_NAME_ENTRY *);
ASN1_STRING *X509_NAME_ENTRY_get_data(X509_NAME_ENTRY *);
unsigned long X509_NAME_hash(X509_NAME *);

int i2d_X509_NAME(X509_NAME *, unsigned char **);
int X509_NAME_add_entry_by_txt(X509_NAME *, const char *, int,
                               const unsigned char *, int, int, int);
int X509_NAME_add_entry_by_NID(X509_NAME *, int, int, unsigned char *,
                               int, int, int);
X509_NAME_ENTRY *X509_NAME_delete_entry(X509_NAME *, int);
void X509_NAME_ENTRY_free(X509_NAME_ENTRY *);
int X509_NAME_get_index_by_NID(X509_NAME *, int, int);
int X509_NAME_cmp(const X509_NAME *, const X509_NAME *);
char *X509_NAME_oneline(X509_NAME *, char *, int);
X509_NAME *X509_NAME_dup(X509_NAME *);
"""

MACROS = """
Cryptography_STACK_OF_X509_NAME *sk_X509_NAME_new_null(void);
int sk_X509_NAME_num(Cryptography_STACK_OF_X509_NAME *);
int sk_X509_NAME_push(Cryptography_STACK_OF_X509_NAME *, X509_NAME *);
X509_NAME *sk_X509_NAME_value(Cryptography_STACK_OF_X509_NAME *, int);
void sk_X509_NAME_free(Cryptography_STACK_OF_X509_NAME *);
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
