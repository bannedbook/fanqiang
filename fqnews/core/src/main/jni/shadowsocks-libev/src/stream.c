/*
 * stream.c - Manage stream ciphers
 *
 * Copyright (C) 2013 - 2019, Max Lv <max.c.lv@gmail.com>
 *
 * This file is part of the shadowsocks-libev.
 *
 * shadowsocks-libev is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * shadowsocks-libev is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shadowsocks-libev; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/version.h>
#define CIPHER_UNSUPPORTED "unsupported"

#include <sodium.h>

#include "ppbloom.h"
#include "stream.h"
#include "utils.h"

#define SODIUM_BLOCK_SIZE   64

/*
 * Spec: http://shadowsocks.org/en/spec/Stream-Ciphers.html
 *
 * Stream ciphers provide only confidentiality. Data integrity and authenticity is not guaranteed. Users should use AEAD
 * ciphers whenever possible.
 *
 * Stream Encryption/Decryption
 *
 * Stream_encrypt is a function that takes a secret key, an initialization vector, a message, and produces a ciphertext
 * with the same length as the message.
 *
 *      Stream_encrypt(key, IV, message) => ciphertext
 *
 * Stream_decrypt is a function that takes a secret key, an initializaiton vector, a ciphertext, and produces the
 * original message.
 *
 *      Stream_decrypt(key, IV, ciphertext) => message
 *
 * TCP
 *
 * A stream cipher encrypted TCP stream starts with a randomly generated initializaiton vector, followed by encrypted
 * payload data.
 *
 *      [IV][encrypted payload]
 *
 * UDP
 *
 * A stream cipher encrypted UDP packet has the following structure:
 *
 *      [IV][encrypted payload]
 *
 * Each UDP packet is encrypted/decrypted independently with a randomly generated initialization vector.
 *
 */

#define NONE                -1
#define TABLE               0
#define RC4                 1
#define RC4_MD5             2
#define AES_128_CFB         3
#define AES_192_CFB         4
#define AES_256_CFB         5
#define AES_128_CTR         6
#define AES_192_CTR         7
#define AES_256_CTR         8
#define BF_CFB              9
#define CAMELLIA_128_CFB    10
#define CAMELLIA_192_CFB    11
#define CAMELLIA_256_CFB    12
#define CAST5_CFB           13
#define DES_CFB             14
#define IDEA_CFB            15
#define RC2_CFB             16
#define SEED_CFB            17
#define SALSA20             18
#define CHACHA20            19
#define CHACHA20IETF        20

const char *supported_stream_ciphers[STREAM_CIPHER_NUM] = {
    "table",
    "rc4",
    "rc4-md5",
    "aes-128-cfb",
    "aes-192-cfb",
    "aes-256-cfb",
    "aes-128-ctr",
    "aes-192-ctr",
    "aes-256-ctr",
    "bf-cfb",
    "camellia-128-cfb",
    "camellia-192-cfb",
    "camellia-256-cfb",
    "cast5-cfb",
    "des-cfb",
    "idea-cfb",
    "rc2-cfb",
    "seed-cfb",
    "salsa20",
    "chacha20",
    "chacha20-ietf"
};

static const char *supported_stream_ciphers_mbedtls[STREAM_CIPHER_NUM] = {
    "table",
    "ARC4-128",
    "ARC4-128",
    "AES-128-CFB128",
    "AES-192-CFB128",
    "AES-256-CFB128",
    "AES-128-CTR",
    "AES-192-CTR",
    "AES-256-CTR",
    "BLOWFISH-CFB64",
    "CAMELLIA-128-CFB128",
    "CAMELLIA-192-CFB128",
    "CAMELLIA-256-CFB128",
    CIPHER_UNSUPPORTED,
    CIPHER_UNSUPPORTED,
    CIPHER_UNSUPPORTED,
    CIPHER_UNSUPPORTED,
    CIPHER_UNSUPPORTED,
    "salsa20",
    "chacha20",
    "chacha20-ietf"
};

static const int supported_stream_ciphers_nonce_size[STREAM_CIPHER_NUM] = {
    0, 0, 16, 16, 16, 16, 16, 16, 16, 8, 16, 16, 16, 8, 8, 8, 8, 16, 8, 8, 12
};

static const int supported_stream_ciphers_key_size[STREAM_CIPHER_NUM] = {
    0, 16, 16, 16, 24, 32, 16, 24, 32, 16, 16, 24, 32, 16, 8, 16, 16, 16, 32, 32, 32
};

static int
crypto_stream_xor_ic(uint8_t *c, const uint8_t *m, uint64_t mlen,
                     const uint8_t *n, uint64_t ic, const uint8_t *k,
                     int method)
{
    switch (method) {
    case SALSA20:
        return crypto_stream_salsa20_xor_ic(c, m, mlen, n, ic, k);
    case CHACHA20:
        return crypto_stream_chacha20_xor_ic(c, m, mlen, n, ic, k);
    case CHACHA20IETF:
        return crypto_stream_chacha20_ietf_xor_ic(c, m, mlen, n, (uint32_t)ic, k);
    }
    // always return 0
    return 0;
}

int
cipher_nonce_size(const cipher_t *cipher)
{
    if (cipher == NULL) {
        return 0;
    }
    return cipher->info->iv_size;
}

int
cipher_key_size(const cipher_t *cipher)
{
    /*
     * Semi-API changes (technically public, morally prnonceate)
     * Renamed a few headers to include _internal in the name. Those headers are
     * not supposed to be included by users.
     * Changed md_info_t into an opaque structure (use md_get_xxx() accessors).
     * Changed pk_info_t into an opaque structure.
     * Changed cipher_base_t into an opaque structure.
     */
    if (cipher == NULL) {
        return 0;
    }
    /* From Version 1.2.7 released 2013-04-13 Default Blowfish keysize is now 128-bits */
    return cipher->info->key_bitlen / 8;
}

const cipher_kt_t *
stream_get_cipher_type(int method)
{
    if (method <= TABLE || method >= STREAM_CIPHER_NUM) {
        LOGE("stream_get_cipher_type(): Illegal method");
        return NULL;
    }

    if (method == RC4_MD5) {
        method = RC4;
    }

    if (method >= SALSA20) {
        return NULL;
    }

    const char *ciphername  = supported_stream_ciphers[method];
    const char *mbedtlsname = supported_stream_ciphers_mbedtls[method];
    if (strcmp(mbedtlsname, CIPHER_UNSUPPORTED) == 0) {
        LOGE("Cipher %s currently is not supported by mbed TLS library",
             ciphername);
        return NULL;
    }
    return mbedtls_cipher_info_from_string(mbedtlsname);
}

void
stream_cipher_ctx_init(cipher_ctx_t *ctx, int method, int enc)
{
    if (method <= TABLE || method >= STREAM_CIPHER_NUM) {
        LOGE("stream_ctx_init(): Illegal method");
        return;
    }

    if (method >= SALSA20) {
        return;
    }

    const char *ciphername    = supported_stream_ciphers[method];
    const cipher_kt_t *cipher = stream_get_cipher_type(method);

    ctx->evp = ss_malloc(sizeof(cipher_evp_t));
    memset(ctx->evp, 0, sizeof(cipher_evp_t));
    cipher_evp_t *evp = ctx->evp;

    if (cipher == NULL) {
        LOGE("Cipher %s not found in mbed TLS library", ciphername);
        FATAL("Cannot initialize mbed TLS cipher");
    }
    mbedtls_cipher_init(evp);
    if (mbedtls_cipher_setup(evp, cipher) != 0) {
        FATAL("Cannot initialize mbed TLS cipher context");
    }
}

void
stream_ctx_release(cipher_ctx_t *cipher_ctx)
{
    if (cipher_ctx->chunk != NULL) {
        bfree(cipher_ctx->chunk);
        ss_free(cipher_ctx->chunk);
        cipher_ctx->chunk = NULL;
    }

    if (cipher_ctx->cipher->method >= SALSA20) {
        return;
    }

    mbedtls_cipher_free(cipher_ctx->evp);
    ss_free(cipher_ctx->evp);
}

void
cipher_ctx_set_nonce(cipher_ctx_t *cipher_ctx, uint8_t *nonce, size_t nonce_len,
                     int enc)
{
    const unsigned char *true_key;

    cipher_t *cipher = cipher_ctx->cipher;

    if (nonce == NULL) {
        LOGE("cipher_ctx_set_nonce(): NONCE is null");
        return;
    }

    if (cipher->method >= SALSA20) {
        return;
    }

    if (cipher->method == RC4_MD5) {
        unsigned char key_nonce[32];
        memcpy(key_nonce, cipher->key, 16);
        memcpy(key_nonce + 16, nonce, 16);
        true_key  = crypto_md5(key_nonce, 32, NULL);
        nonce_len = 0;
    } else {
        true_key = cipher->key;
    }

    cipher_evp_t *evp = cipher_ctx->evp;
    if (evp == NULL) {
        LOGE("cipher_ctx_set_nonce(): Cipher context is null");
        return;
    }
    if (mbedtls_cipher_setkey(evp, true_key, cipher->key_len * 8, enc) != 0) {
        mbedtls_cipher_free(evp);
        FATAL("Cannot set mbed TLS cipher key");
    }
    if (mbedtls_cipher_set_iv(evp, nonce, nonce_len) != 0) {
        mbedtls_cipher_free(evp);
        FATAL("Cannot set mbed TLS cipher NONCE");
    }
    if (mbedtls_cipher_reset(evp) != 0) {
        mbedtls_cipher_free(evp);
        FATAL("Cannot finalize mbed TLS cipher context");
    }

#ifdef SS_DEBUG
    dump("NONCE", (char *)nonce, nonce_len);
    dump("KEY", (char *)true_key, 32);
#endif
}

static int
cipher_ctx_update(cipher_ctx_t *ctx, uint8_t *output, size_t *olen,
                  const uint8_t *input, size_t ilen)
{
    cipher_evp_t *evp = ctx->evp;
    return mbedtls_cipher_update(evp, (const uint8_t *)input, ilen,
                                 (uint8_t *)output, olen);
}

int
stream_encrypt_all(buffer_t *plaintext, cipher_t *cipher, size_t capacity)
{
    cipher_ctx_t cipher_ctx;
    stream_ctx_init(cipher, &cipher_ctx, 1);

    size_t nonce_len = cipher->nonce_len;
    int err          = CRYPTO_OK;

    static buffer_t tmp = { 0, 0, 0, NULL };
    brealloc(&tmp, nonce_len + plaintext->len, capacity);
    buffer_t *ciphertext = &tmp;
    ciphertext->len = plaintext->len;

    uint8_t *nonce = cipher_ctx.nonce;
    cipher_ctx_set_nonce(&cipher_ctx, nonce, nonce_len, 1);
    memcpy(ciphertext->data, nonce, nonce_len);

#ifdef MODULE_REMOTE
    ppbloom_add((void *)nonce, nonce_len);
#endif

    if (cipher->method >= SALSA20) {
        crypto_stream_xor_ic((uint8_t *)(ciphertext->data + nonce_len),
                             (const uint8_t *)plaintext->data, (uint64_t)(plaintext->len),
                             (const uint8_t *)nonce,
                             0, cipher->key, cipher->method);
    } else {
        err = cipher_ctx_update(&cipher_ctx, (uint8_t *)(ciphertext->data + nonce_len),
                                &ciphertext->len, (const uint8_t *)plaintext->data,
                                plaintext->len);
    }

    stream_ctx_release(&cipher_ctx);

    if (err)
        return CRYPTO_ERROR;

#ifdef SS_DEBUG
    dump("PLAIN", plaintext->data, plaintext->len);
    dump("CIPHER", ciphertext->data + nonce_len, ciphertext->len);
    dump("NONCE", ciphertext->data, nonce_len);
#endif

    brealloc(plaintext, nonce_len + ciphertext->len, capacity);
    memcpy(plaintext->data, ciphertext->data, nonce_len + ciphertext->len);
    plaintext->len = nonce_len + ciphertext->len;

    return CRYPTO_OK;
}

int
stream_encrypt(buffer_t *plaintext, cipher_ctx_t *cipher_ctx, size_t capacity)
{
    if (cipher_ctx == NULL)
        return CRYPTO_ERROR;

    cipher_t *cipher = cipher_ctx->cipher;

    static buffer_t tmp = { 0, 0, 0, NULL };

    int err          = CRYPTO_OK;
    size_t nonce_len = 0;
    if (!cipher_ctx->init) {
        nonce_len = cipher_ctx->cipher->nonce_len;
    }

    brealloc(&tmp, nonce_len + plaintext->len, capacity);
    buffer_t *ciphertext = &tmp;
    ciphertext->len = plaintext->len;

    if (!cipher_ctx->init) {
        cipher_ctx_set_nonce(cipher_ctx, cipher_ctx->nonce, nonce_len, 1);
        memcpy(ciphertext->data, cipher_ctx->nonce, nonce_len);
        cipher_ctx->counter = 0;
        cipher_ctx->init    = 1;

#ifdef MODULE_REMOTE
        ppbloom_add((void *)cipher_ctx->nonce, nonce_len);
#endif
    }

    if (cipher->method >= SALSA20) {
        int padding = cipher_ctx->counter % SODIUM_BLOCK_SIZE;
        brealloc(ciphertext, nonce_len + (padding + ciphertext->len) * 2, capacity);
        if (padding) {
            brealloc(plaintext, plaintext->len + padding, capacity);
            memmove(plaintext->data + padding, plaintext->data, plaintext->len);
            sodium_memzero(plaintext->data, padding);
        }
        crypto_stream_xor_ic((uint8_t *)(ciphertext->data + nonce_len),
                             (const uint8_t *)plaintext->data,
                             (uint64_t)(plaintext->len + padding),
                             (const uint8_t *)cipher_ctx->nonce,
                             cipher_ctx->counter / SODIUM_BLOCK_SIZE, cipher->key,
                             cipher->method);
        cipher_ctx->counter += plaintext->len;
        if (padding) {
            memmove(ciphertext->data + nonce_len,
                    ciphertext->data + nonce_len + padding, ciphertext->len);
        }
    } else {
        err = cipher_ctx_update(cipher_ctx,
                                (uint8_t *)(ciphertext->data + nonce_len),
                                &ciphertext->len, (const uint8_t *)plaintext->data,
                                plaintext->len);
        if (err) {
            return CRYPTO_ERROR;
        }
    }

#ifdef SS_DEBUG
    dump("PLAIN", plaintext->data, plaintext->len);
    dump("CIPHER", ciphertext->data + nonce_len, ciphertext->len);
#endif

    brealloc(plaintext, nonce_len + ciphertext->len, capacity);
    memcpy(plaintext->data, ciphertext->data, nonce_len + ciphertext->len);
    plaintext->len = nonce_len + ciphertext->len;

    return CRYPTO_OK;
}

int
stream_decrypt_all(buffer_t *ciphertext, cipher_t *cipher, size_t capacity)
{
    size_t nonce_len = cipher->nonce_len;
    int err          = CRYPTO_OK;

    if (ciphertext->len <= nonce_len) {
        return CRYPTO_ERROR;
    }

    cipher_ctx_t cipher_ctx;
    stream_ctx_init(cipher, &cipher_ctx, 0);

    static buffer_t tmp = { 0, 0, 0, NULL };
    brealloc(&tmp, ciphertext->len, capacity);
    buffer_t *plaintext = &tmp;
    plaintext->len = ciphertext->len - nonce_len;

    uint8_t *nonce = cipher_ctx.nonce;
    memcpy(nonce, ciphertext->data, nonce_len);

    if (ppbloom_check((void *)nonce, nonce_len) == 1) {
        LOGE("crypto: stream: repeat IV detected");
        return CRYPTO_ERROR;
    }

    cipher_ctx_set_nonce(&cipher_ctx, nonce, nonce_len, 0);

    if (cipher->method >= SALSA20) {
        crypto_stream_xor_ic((uint8_t *)plaintext->data,
                             (const uint8_t *)(ciphertext->data + nonce_len),
                             (uint64_t)(ciphertext->len - nonce_len),
                             (const uint8_t *)nonce, 0, cipher->key, cipher->method);
    } else {
        err = cipher_ctx_update(&cipher_ctx, (uint8_t *)plaintext->data, &plaintext->len,
                                (const uint8_t *)(ciphertext->data + nonce_len),
                                ciphertext->len - nonce_len);
    }

    stream_ctx_release(&cipher_ctx);

    if (err)
        return CRYPTO_ERROR;

#ifdef SS_DEBUG
    dump("PLAIN", plaintext->data, plaintext->len);
    dump("CIPHER", ciphertext->data + nonce_len, ciphertext->len - nonce_len);
    dump("NONCE", ciphertext->data, nonce_len);
#endif

    ppbloom_add((void *)nonce, nonce_len);

    brealloc(ciphertext, plaintext->len, capacity);
    memcpy(ciphertext->data, plaintext->data, plaintext->len);
    ciphertext->len = plaintext->len;

    return CRYPTO_OK;
}

int
stream_decrypt(buffer_t *ciphertext, cipher_ctx_t *cipher_ctx, size_t capacity)
{
    if (cipher_ctx == NULL)
        return CRYPTO_ERROR;

    cipher_t *cipher = cipher_ctx->cipher;

    static buffer_t tmp = { 0, 0, 0, NULL };

    int err = CRYPTO_OK;

    brealloc(&tmp, ciphertext->len, capacity);
    buffer_t *plaintext = &tmp;
    plaintext->len = ciphertext->len;

    if (!cipher_ctx->init) {
        if (cipher_ctx->chunk == NULL) {
            cipher_ctx->chunk = (buffer_t *)ss_malloc(sizeof(buffer_t));
            memset(cipher_ctx->chunk, 0, sizeof(buffer_t));
            balloc(cipher_ctx->chunk, cipher->nonce_len);
        }

        size_t left_len = min(cipher->nonce_len - cipher_ctx->chunk->len,
                              ciphertext->len);

        if (left_len > 0) {
            memcpy(cipher_ctx->chunk->data + cipher_ctx->chunk->len, ciphertext->data, left_len);
            memmove(ciphertext->data, ciphertext->data + left_len,
                    ciphertext->len - left_len);
            cipher_ctx->chunk->len += left_len;
            ciphertext->len        -= left_len;
        }

        if (cipher_ctx->chunk->len < cipher->nonce_len)
            return CRYPTO_NEED_MORE;

        uint8_t *nonce   = cipher_ctx->nonce;
        size_t nonce_len = cipher->nonce_len;
        plaintext->len -= left_len;

        memcpy(nonce, cipher_ctx->chunk->data, nonce_len);
        cipher_ctx_set_nonce(cipher_ctx, nonce, nonce_len, 0);
        cipher_ctx->counter = 0;
        cipher_ctx->init    = 1;

        if (cipher->method >= RC4_MD5) {
            if (ppbloom_check((void *)nonce, nonce_len) == 1) {
                LOGE("crypto: stream: repeat IV detected");
                return CRYPTO_ERROR;
            }
        }
    }

    if (ciphertext->len <= 0)
        return CRYPTO_NEED_MORE;

    if (cipher->method >= SALSA20) {
        int padding = cipher_ctx->counter % SODIUM_BLOCK_SIZE;
        brealloc(plaintext, (plaintext->len + padding) * 2, capacity);

        if (padding) {
            brealloc(ciphertext, ciphertext->len + padding, capacity);
            memmove(ciphertext->data + padding, ciphertext->data,
                    ciphertext->len);
            sodium_memzero(ciphertext->data, padding);
        }
        crypto_stream_xor_ic((uint8_t *)plaintext->data,
                             (const uint8_t *)(ciphertext->data),
                             (uint64_t)(ciphertext->len + padding),
                             (const uint8_t *)cipher_ctx->nonce,
                             cipher_ctx->counter / SODIUM_BLOCK_SIZE, cipher->key,
                             cipher->method);
        cipher_ctx->counter += ciphertext->len;
        if (padding) {
            memmove(plaintext->data, plaintext->data + padding, plaintext->len);
        }
    } else {
        err = cipher_ctx_update(cipher_ctx, (uint8_t *)plaintext->data, &plaintext->len,
                                (const uint8_t *)(ciphertext->data),
                                ciphertext->len);
    }

    if (err)
        return CRYPTO_ERROR;

#ifdef SS_DEBUG
    dump("PLAIN", plaintext->data, plaintext->len);
    dump("CIPHER", ciphertext->data, ciphertext->len);
#endif

    // Add to bloom filter
    if (cipher_ctx->init == 1) {
        if (cipher->method >= RC4_MD5) {
            if (ppbloom_check((void *)cipher_ctx->nonce, cipher->nonce_len) == 1) {
                LOGE("crypto: stream: repeat IV detected");
                return CRYPTO_ERROR;
            }
            ppbloom_add((void *)cipher_ctx->nonce, cipher->nonce_len);
            cipher_ctx->init = 2;
        }
    }

    brealloc(ciphertext, plaintext->len, capacity);
    memcpy(ciphertext->data, plaintext->data, plaintext->len);
    ciphertext->len = plaintext->len;

    return CRYPTO_OK;
}

void
stream_ctx_init(cipher_t *cipher, cipher_ctx_t *cipher_ctx, int enc)
{
    sodium_memzero(cipher_ctx, sizeof(cipher_ctx_t));
    stream_cipher_ctx_init(cipher_ctx, cipher->method, enc);
    cipher_ctx->cipher = cipher;

    if (enc) {
        rand_bytes(cipher_ctx->nonce, cipher->nonce_len);
    }
}

cipher_t *
stream_key_init(int method, const char *pass, const char *key)
{
    if (method <= TABLE || method >= STREAM_CIPHER_NUM) {
        LOGE("cipher->key_init(): Illegal method");
        return NULL;
    }

    cipher_t *cipher = (cipher_t *)ss_malloc(sizeof(cipher_t));
    memset(cipher, 0, sizeof(cipher_t));

    if (method == SALSA20 || method == CHACHA20 || method == CHACHA20IETF) {
        cipher_kt_t *cipher_info = (cipher_kt_t *)ss_malloc(sizeof(cipher_kt_t));
        cipher->info             = cipher_info;
        cipher->info->base       = NULL;
        cipher->info->key_bitlen = supported_stream_ciphers_key_size[method] * 8;
        cipher->info->iv_size    = supported_stream_ciphers_nonce_size[method];
    } else {
        cipher->info = (cipher_kt_t *)stream_get_cipher_type(method);
    }

    if (cipher->info == NULL && cipher->key_len == 0) {
        LOGE("Cipher %s not found in crypto library", supported_stream_ciphers[method]);
        FATAL("Cannot initialize cipher");
    }

    if (key != NULL)
        cipher->key_len = crypto_parse_key(key, cipher->key, cipher_key_size(cipher));
    else
        cipher->key_len = crypto_derive_key(pass, cipher->key, cipher_key_size(cipher));

    if (cipher->key_len == 0) {
        FATAL("Cannot generate key and NONCE");
    }
    if (method == RC4_MD5) {
        cipher->nonce_len = 16;
    } else {
        cipher->nonce_len = cipher_nonce_size(cipher);
    }
    cipher->method = method;

    return cipher;
}

cipher_t *
stream_init(const char *pass, const char *key, const char *method)
{
    int m = TABLE;
    if (method != NULL) {
        for (m = TABLE; m < STREAM_CIPHER_NUM; m++)
            if (strcmp(method, supported_stream_ciphers[m]) == 0) {
                break;
            }
        if (m >= STREAM_CIPHER_NUM) {
            LOGE("Invalid cipher name: %s, use chacha20-ietf instead", method);
            m = CHACHA20IETF;
        }
    }
    if (m == TABLE) {
        LOGE("Table is deprecated");
        return NULL;
    }
    return stream_key_init(m, pass, key);
}
