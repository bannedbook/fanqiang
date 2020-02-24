# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/hmac.h>
"""

TYPES = """
typedef struct { ...; } HMAC_CTX;
"""

FUNCTIONS = """
void HMAC_CTX_init(HMAC_CTX *);
void HMAC_CTX_cleanup(HMAC_CTX *);

int Cryptography_HMAC_Init_ex(HMAC_CTX *, const void *, int, const EVP_MD *,
                              ENGINE *);
int Cryptography_HMAC_Update(HMAC_CTX *, const unsigned char *, size_t);
int Cryptography_HMAC_Final(HMAC_CTX *, unsigned char *, unsigned int *);
int Cryptography_HMAC_CTX_copy(HMAC_CTX *, HMAC_CTX *);
"""

MACROS = """
"""

CUSTOMIZATIONS = """
int Cryptography_HMAC_Init_ex(HMAC_CTX *ctx, const void *key, int key_len,
                              const EVP_MD *md, ENGINE *impl) {
#if OPENSSL_VERSION_NUMBER >= 0x010000000
    return HMAC_Init_ex(ctx, key, key_len, md, impl);
#else
    HMAC_Init_ex(ctx, key, key_len, md, impl);
    return 1;
#endif
}

int Cryptography_HMAC_Update(HMAC_CTX *ctx, const unsigned char *data,
                             size_t data_len) {
#if OPENSSL_VERSION_NUMBER >= 0x010000000
    return HMAC_Update(ctx, data, data_len);
#else
    HMAC_Update(ctx, data, data_len);
    return 1;
#endif
}

int Cryptography_HMAC_Final(HMAC_CTX *ctx, unsigned char *digest,
    unsigned int *outlen) {
#if OPENSSL_VERSION_NUMBER >= 0x010000000
    return HMAC_Final(ctx, digest, outlen);
#else
    HMAC_Final(ctx, digest, outlen);
    return 1;
#endif
}

int Cryptography_HMAC_CTX_copy(HMAC_CTX *dst_ctx, HMAC_CTX *src_ctx) {
#if OPENSSL_VERSION_NUMBER >= 0x010000000
    return HMAC_CTX_copy(dst_ctx, src_ctx);
#else
    HMAC_CTX_init(dst_ctx);
    if (!EVP_MD_CTX_copy_ex(&dst_ctx->i_ctx, &src_ctx->i_ctx)) {
        goto err;
    }
    if (!EVP_MD_CTX_copy_ex(&dst_ctx->o_ctx, &src_ctx->o_ctx)) {
        goto err;
    }
    if (!EVP_MD_CTX_copy_ex(&dst_ctx->md_ctx, &src_ctx->md_ctx)) {
        goto err;
    }
    memcpy(dst_ctx->key, src_ctx->key, HMAC_MAX_MD_CBLOCK);
    dst_ctx->key_length = src_ctx->key_length;
    dst_ctx->md = src_ctx->md;
    return 1;

    err:
        return 0;
#endif
}
"""

CONDITIONAL_NAMES = {}
