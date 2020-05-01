/*
 * aead.c - Manage AEAD ciphers
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

#include <mbedtls/version.h>
#define CIPHER_UNSUPPORTED "unsupported"
#include <time.h>
#include <stdio.h>
#include <assert.h>

#include <sodium.h>
#ifndef __MINGW32__
#include <arpa/inet.h>
#endif

#include "ppbloom.h"
#include "aead.h"
#include "utils.h"
#include "winsock.h"

#define NONE                    (-1)
#define AES128GCM               0
#define AES192GCM               1
#define AES256GCM               2
/*
 * methods above requires gcm context
 * methods below doesn't require it,
 * then we need to fake one
 */
#define CHACHA20POLY1305IETF    3

#ifdef FS_HAVE_XCHACHA20IETF
#define XCHACHA20POLY1305IETF   4
#endif

#define CHUNK_SIZE_LEN          2
#define CHUNK_SIZE_MASK         0x3FFF

/*
 * Spec: http://shadowsocks.org/en/spec/AEAD-Ciphers.html
 *
 * The way Shadowsocks using AEAD ciphers is specified in SIP004 and amended in SIP007. SIP004 was proposed by @Mygod
 * with design inspirations from @wongsyrone, @Noisyfox and @breakwa11. SIP007 was proposed by @riobard with input from
 * @madeye, @Mygod, @wongsyrone, and many others.
 *
 * Key Derivation
 *
 * HKDF_SHA1 is a function that takes a secret key, a non-secret salt, an info string, and produces a subkey that is
 * cryptographically strong even if the input secret key is weak.
 *
 *      HKDF_SHA1(key, salt, info) => subkey
 *
 * The info string binds the generated subkey to a specific application context. In our case, it must be the string
 * "ss-subkey" without quotes.
 *
 * We derive a per-session subkey from a pre-shared master key using HKDF_SHA1. Salt must be unique through the entire
 * life of the pre-shared master key.
 *
 * TCP
 *
 * An AEAD encrypted TCP stream starts with a randomly generated salt to derive the per-session subkey, followed by any
 * number of encrypted chunks. Each chunk has the following structure:
 *
 *      [encrypted payload length][length tag][encrypted payload][payload tag]
 *
 * Payload length is a 2-byte big-endian unsigned integer capped at 0x3FFF. The higher two bits are reserved and must be
 * set to zero. Payload is therefore limited to 16*1024 - 1 bytes.
 *
 * The first AEAD encrypt/decrypt operation uses a counting nonce starting from 0. After each encrypt/decrypt operation,
 * the nonce is incremented by one as if it were an unsigned little-endian integer. Note that each TCP chunk involves
 * two AEAD encrypt/decrypt operation: one for the payload length, and one for the payload. Therefore each chunk
 * increases the nonce twice.
 *
 * UDP
 *
 * An AEAD encrypted UDP packet has the following structure:
 *
 *      [salt][encrypted payload][tag]
 *
 * The salt is used to derive the per-session subkey and must be generated randomly to ensure uniqueness. Each UDP
 * packet is encrypted/decrypted independently, using the derived subkey and a nonce with all zero bytes.
 *
 */

const char *supported_aead_ciphers[AEAD_CIPHER_NUM] = {
    "aes-128-gcm",
    "aes-192-gcm",
    "aes-256-gcm",
    "chacha20-ietf-poly1305",
#ifdef FS_HAVE_XCHACHA20IETF
    "xchacha20-ietf-poly1305"
#endif
};

/*
 * use mbed TLS cipher wrapper to unify handling
 */
static const char *supported_aead_ciphers_mbedtls[AEAD_CIPHER_NUM] = {
    "AES-128-GCM",
    "AES-192-GCM",
    "AES-256-GCM",
    CIPHER_UNSUPPORTED,
#ifdef FS_HAVE_XCHACHA20IETF
    CIPHER_UNSUPPORTED
#endif
};

static const int supported_aead_ciphers_nonce_size[AEAD_CIPHER_NUM] = {
    12, 12, 12, 12,
#ifdef FS_HAVE_XCHACHA20IETF
    24
#endif
};

static const int supported_aead_ciphers_key_size[AEAD_CIPHER_NUM] = {
    16, 24, 32, 32,
#ifdef FS_HAVE_XCHACHA20IETF
    32
#endif
};

static const int supported_aead_ciphers_tag_size[AEAD_CIPHER_NUM] = {
    16, 16, 16, 16,
#ifdef FS_HAVE_XCHACHA20IETF
    16
#endif
};

static int
aead_cipher_encrypt(cipher_ctx_t *cipher_ctx,
                    uint8_t *c,
                    size_t *clen,
                    uint8_t *m,
                    size_t mlen,
                    uint8_t *ad,
                    size_t adlen,
                    uint8_t *n,
                    uint8_t *k)
{
    int err                      = CRYPTO_OK;
    unsigned long long long_clen = 0;

    size_t nlen = cipher_ctx->cipher->nonce_len;
    size_t tlen = cipher_ctx->cipher->tag_len;

    switch (cipher_ctx->cipher->method) {
    case AES256GCM: // Only AES-256-GCM is supported by libsodium.
        if (cipher_ctx->aes256gcm_ctx != NULL) { // Use it if availble
            err = crypto_aead_aes256gcm_encrypt_afternm(c, &long_clen, m, mlen,
                                                        ad, adlen, NULL, n,
                                                        (const aes256gcm_ctx *)cipher_ctx->aes256gcm_ctx);
            *clen = (size_t)long_clen; // it's safe to cast 64bit to 32bit length here
            break;
        }
    // Otherwise, just use the mbedTLS one with crappy AES-NI.
    case AES192GCM:
    case AES128GCM:

        err = mbedtls_cipher_auth_encrypt(cipher_ctx->evp, n, nlen, ad, adlen,
                                          m, mlen, c, clen, c + mlen, tlen);
        *clen += tlen;
        break;
    case CHACHA20POLY1305IETF:
        err = crypto_aead_chacha20poly1305_ietf_encrypt(c, &long_clen, m, mlen,
                                                        ad, adlen, NULL, n, k);
        *clen = (size_t)long_clen;
        break;
#ifdef FS_HAVE_XCHACHA20IETF
    case XCHACHA20POLY1305IETF:
        err = crypto_aead_xchacha20poly1305_ietf_encrypt(c, &long_clen, m, mlen,
                                                         ad, adlen, NULL, n, k);
        *clen = (size_t)long_clen;
        break;
#endif
    default:
        return CRYPTO_ERROR;
    }

    return err;
}

static int
aead_cipher_decrypt(cipher_ctx_t *cipher_ctx,
                    uint8_t *p, size_t *plen,
                    uint8_t *m, size_t mlen,
                    uint8_t *ad, size_t adlen,
                    uint8_t *n, uint8_t *k)
{
    int err                      = CRYPTO_ERROR;
    unsigned long long long_plen = 0;

    size_t nlen = cipher_ctx->cipher->nonce_len;
    size_t tlen = cipher_ctx->cipher->tag_len;

    switch (cipher_ctx->cipher->method) {
    case AES256GCM: // Only AES-256-GCM is supported by libsodium.
        if (cipher_ctx->aes256gcm_ctx != NULL) { // Use it if availble
            err = crypto_aead_aes256gcm_decrypt_afternm(p, &long_plen, NULL, m, mlen,
                                                        ad, adlen, n,
                                                        (const aes256gcm_ctx *)cipher_ctx->aes256gcm_ctx);
            *plen = (size_t)long_plen; // it's safe to cast 64bit to 32bit length here
            break;
        }
    // Otherwise, just use the mbedTLS one with crappy AES-NI.
    case AES192GCM:
    case AES128GCM:
        err = mbedtls_cipher_auth_decrypt(cipher_ctx->evp, n, nlen, ad, adlen,
                                          m, mlen - tlen, p, plen, m + mlen - tlen, tlen);
        break;
    case CHACHA20POLY1305IETF:
        err = crypto_aead_chacha20poly1305_ietf_decrypt(p, &long_plen, NULL, m, mlen,
                                                        ad, adlen, n, k);
        *plen = (size_t)long_plen; // it's safe to cast 64bit to 32bit length here
        break;
#ifdef FS_HAVE_XCHACHA20IETF
    case XCHACHA20POLY1305IETF:
        err = crypto_aead_xchacha20poly1305_ietf_decrypt(p, &long_plen, NULL, m, mlen,
                                                         ad, adlen, n, k);
        *plen = (size_t)long_plen; // it's safe to cast 64bit to 32bit length here
        break;
#endif
    default:
        return CRYPTO_ERROR;
    }

    // The success return value ln libsodium and mbedTLS are both 0
    if (err != 0)
        // Although we never return any library specific value in the caller,
        // here we still set the error code to CRYPTO_ERROR to avoid confusion.
        err = CRYPTO_ERROR;

    return err;
}

/*
 * get basic cipher info structure
 * it's a wrapper offered by crypto library
 */
const cipher_kt_t *
aead_get_cipher_type(int method)
{
    if (method < AES128GCM || method >= AEAD_CIPHER_NUM) {
        LOGE("aead_get_cipher_type(): Illegal method");
        return NULL;
    }

    /* cipher that don't use mbed TLS, just return */
    if (method >= CHACHA20POLY1305IETF) {
        return NULL;
    }

    const char *ciphername  = supported_aead_ciphers[method];
    const char *mbedtlsname = supported_aead_ciphers_mbedtls[method];
    if (strcmp(mbedtlsname, CIPHER_UNSUPPORTED) == 0) {
        LOGE("Cipher %s currently is not supported by mbed TLS library",
             ciphername);
        return NULL;
    }
    return mbedtls_cipher_info_from_string(mbedtlsname);
}

static void
aead_cipher_ctx_set_key(cipher_ctx_t *cipher_ctx, int enc)
{
    const digest_type_t *md = mbedtls_md_info_from_string("SHA1");
    if (md == NULL) {
        FATAL("SHA1 Digest not found in crypto library");
    }

    int err = crypto_hkdf(md,
                          cipher_ctx->salt, cipher_ctx->cipher->key_len,
                          cipher_ctx->cipher->key, cipher_ctx->cipher->key_len,
                          (uint8_t *)SUBKEY_INFO, strlen(SUBKEY_INFO),
                          cipher_ctx->skey, cipher_ctx->cipher->key_len);
    if (err) {
        FATAL("Unable to generate subkey");
    }

    memset(cipher_ctx->nonce, 0, cipher_ctx->cipher->nonce_len);

    /* cipher that don't use mbed TLS, just return */
    if (cipher_ctx->cipher->method >= CHACHA20POLY1305IETF) {
        return;
    }
    if (cipher_ctx->aes256gcm_ctx != NULL) {
        if (crypto_aead_aes256gcm_beforenm(cipher_ctx->aes256gcm_ctx,
                                           cipher_ctx->skey) != 0) {
            FATAL("Cannot set libsodium cipher key");
        }
        return;
    }
    if (mbedtls_cipher_setkey(cipher_ctx->evp, cipher_ctx->skey,
                              cipher_ctx->cipher->key_len * 8, enc) != 0) {
        FATAL("Cannot set mbed TLS cipher key");
    }
    if (mbedtls_cipher_reset(cipher_ctx->evp) != 0) {
        FATAL("Cannot finish preparation of mbed TLS cipher context");
    }
}

static void
aead_cipher_ctx_init(cipher_ctx_t *cipher_ctx, int method, int enc)
{
    if (method < AES128GCM || method >= AEAD_CIPHER_NUM) {
        LOGE("cipher_context_init(): Illegal method");
        return;
    }

    if (method >= CHACHA20POLY1305IETF) {
        return;
    }

    const char *ciphername = supported_aead_ciphers[method];

    const cipher_kt_t *cipher = aead_get_cipher_type(method);

    if (method == AES256GCM && crypto_aead_aes256gcm_is_available()) {
        cipher_ctx->aes256gcm_ctx = ss_aligned_malloc(sizeof(aes256gcm_ctx));
        memset(cipher_ctx->aes256gcm_ctx, 0, sizeof(aes256gcm_ctx));
    } else {
        cipher_ctx->aes256gcm_ctx = NULL;
        cipher_ctx->evp           = ss_malloc(sizeof(cipher_evp_t));
        memset(cipher_ctx->evp, 0, sizeof(cipher_evp_t));
        cipher_evp_t *evp = cipher_ctx->evp;
        mbedtls_cipher_init(evp);
        if (mbedtls_cipher_setup(evp, cipher) != 0) {
            FATAL("Cannot initialize mbed TLS cipher context");
        }
    }

    if (cipher == NULL) {
        LOGE("Cipher %s not found in mbed TLS library", ciphername);
        FATAL("Cannot initialize mbed TLS cipher");
    }

#ifdef SS_DEBUG
    dump("KEY", (char *)cipher_ctx->cipher->key, cipher_ctx->cipher->key_len);
#endif
}

void
aead_ctx_init(cipher_t *cipher, cipher_ctx_t *cipher_ctx, int enc)
{
    sodium_memzero(cipher_ctx, sizeof(cipher_ctx_t));
    cipher_ctx->cipher = cipher;

    aead_cipher_ctx_init(cipher_ctx, cipher->method, enc);

    if (enc) {
        rand_bytes(cipher_ctx->salt, cipher->key_len);
    }
}

void
aead_ctx_release(cipher_ctx_t *cipher_ctx)
{
    if (cipher_ctx->chunk != NULL) {
        bfree(cipher_ctx->chunk);
        ss_free(cipher_ctx->chunk);
        cipher_ctx->chunk = NULL;
    }

    if (cipher_ctx->cipher->method >= CHACHA20POLY1305IETF) {
        return;
    }

    if (cipher_ctx->aes256gcm_ctx != NULL) {
        ss_aligned_free(cipher_ctx->aes256gcm_ctx);
        return;
    }

    mbedtls_cipher_free(cipher_ctx->evp);
    ss_free(cipher_ctx->evp);
}

int
aead_encrypt_all(buffer_t *plaintext, cipher_t *cipher, size_t capacity)
{
    cipher_ctx_t cipher_ctx;
    aead_ctx_init(cipher, &cipher_ctx, 1);

    size_t salt_len = cipher->key_len;
    size_t tag_len  = cipher->tag_len;
    int err         = CRYPTO_OK;

    static buffer_t tmp = { 0, 0, 0, NULL };
    brealloc(&tmp, salt_len + tag_len + plaintext->len, capacity);
    buffer_t *ciphertext = &tmp;
    ciphertext->len = tag_len + plaintext->len;

    /* copy salt to first pos */
    memcpy(ciphertext->data, cipher_ctx.salt, salt_len);

    ppbloom_add((void *)cipher_ctx.salt, salt_len);

    aead_cipher_ctx_set_key(&cipher_ctx, 1);

    size_t clen = ciphertext->len;
    err = aead_cipher_encrypt(&cipher_ctx,
                              (uint8_t *)ciphertext->data + salt_len, &clen,
                              (uint8_t *)plaintext->data, plaintext->len,
                              NULL, 0, cipher_ctx.nonce, cipher_ctx.skey);

    aead_ctx_release(&cipher_ctx);

    if (err)
        return CRYPTO_ERROR;

    assert(ciphertext->len == clen);

    brealloc(plaintext, salt_len + ciphertext->len, capacity);
    memcpy(plaintext->data, ciphertext->data, salt_len + ciphertext->len);
    plaintext->len = salt_len + ciphertext->len;

    return CRYPTO_OK;
}

int
aead_decrypt_all(buffer_t *ciphertext, cipher_t *cipher, size_t capacity)
{
    size_t salt_len = cipher->key_len;
    size_t tag_len  = cipher->tag_len;
    int err         = CRYPTO_OK;

    if (ciphertext->len <= salt_len + tag_len) {
        return CRYPTO_ERROR;
    }

    cipher_ctx_t cipher_ctx;
    aead_ctx_init(cipher, &cipher_ctx, 0);

    static buffer_t tmp = { 0, 0, 0, NULL };
    brealloc(&tmp, ciphertext->len, capacity);
    buffer_t *plaintext = &tmp;
    plaintext->len = ciphertext->len - salt_len - tag_len;

    /* get salt */
    uint8_t *salt = cipher_ctx.salt;
    memcpy(salt, ciphertext->data, salt_len);

    if (ppbloom_check((void *)salt, salt_len) == 1) {
        LOGE("crypto: AEAD: repeat salt detected");
        return CRYPTO_ERROR;
    }

    aead_cipher_ctx_set_key(&cipher_ctx, 0);

    size_t plen = plaintext->len;
    err = aead_cipher_decrypt(&cipher_ctx,
                              (uint8_t *)plaintext->data, &plen,
                              (uint8_t *)ciphertext->data + salt_len,
                              ciphertext->len - salt_len, NULL, 0,
                              cipher_ctx.nonce, cipher_ctx.skey);

    aead_ctx_release(&cipher_ctx);

    if (err)
        return CRYPTO_ERROR;

    ppbloom_add((void *)salt, salt_len);

    brealloc(ciphertext, plaintext->len, capacity);
    memcpy(ciphertext->data, plaintext->data, plaintext->len);
    ciphertext->len = plaintext->len;

    return CRYPTO_OK;
}

static int
aead_chunk_encrypt(cipher_ctx_t *ctx, uint8_t *p, uint8_t *c,
                   uint8_t *n, uint16_t plen)
{
    size_t nlen = ctx->cipher->nonce_len;
    size_t tlen = ctx->cipher->tag_len;

    assert(plen <= CHUNK_SIZE_MASK);

    int err;
    size_t clen;
    uint8_t len_buf[CHUNK_SIZE_LEN];
    uint16_t t = htons(plen & CHUNK_SIZE_MASK);
    memcpy(len_buf, &t, CHUNK_SIZE_LEN);

    clen = CHUNK_SIZE_LEN + tlen;
    err  = aead_cipher_encrypt(ctx, c, &clen, len_buf, CHUNK_SIZE_LEN,
                               NULL, 0, n, ctx->skey);
    if (err)
        return CRYPTO_ERROR;

    assert(clen == CHUNK_SIZE_LEN + tlen);

    sodium_increment(n, nlen);

    clen = plen + tlen;
    err  = aead_cipher_encrypt(ctx, c + CHUNK_SIZE_LEN + tlen, &clen, p, plen,
                               NULL, 0, n, ctx->skey);
    if (err)
        return CRYPTO_ERROR;

    assert(clen == plen + tlen);

    sodium_increment(n, nlen);

    return CRYPTO_OK;
}

/* TCP */
int
aead_encrypt(buffer_t *plaintext, cipher_ctx_t *cipher_ctx, size_t capacity)
{
    if (cipher_ctx == NULL)
        return CRYPTO_ERROR;

    if (plaintext->len == 0) {
        return CRYPTO_OK;
    }

    static buffer_t tmp = { 0, 0, 0, NULL };
    buffer_t *ciphertext;

    cipher_t *cipher = cipher_ctx->cipher;
    int err          = CRYPTO_ERROR;
    size_t salt_ofst = 0;
    size_t salt_len  = cipher->key_len;
    size_t tag_len   = cipher->tag_len;

    if (!cipher_ctx->init) {
        salt_ofst = salt_len;
    }

    size_t out_len = salt_ofst + 2 * tag_len + plaintext->len + CHUNK_SIZE_LEN;
    brealloc(&tmp, out_len, capacity);
    ciphertext      = &tmp;
    ciphertext->len = out_len;

    if (!cipher_ctx->init) {
        memcpy(ciphertext->data, cipher_ctx->salt, salt_len);
        aead_cipher_ctx_set_key(cipher_ctx, 1);
        cipher_ctx->init = 1;

        ppbloom_add((void *)cipher_ctx->salt, salt_len);
    }

    err = aead_chunk_encrypt(cipher_ctx,
                             (uint8_t *)plaintext->data,
                             (uint8_t *)ciphertext->data + salt_ofst,
                             cipher_ctx->nonce, plaintext->len);
    if (err)
        return err;

    brealloc(plaintext, ciphertext->len, capacity);
    memcpy(plaintext->data, ciphertext->data, ciphertext->len);
    plaintext->len = ciphertext->len;

    return 0;
}

static int
aead_chunk_decrypt(cipher_ctx_t *ctx, uint8_t *p, uint8_t *c, uint8_t *n,
                   size_t *plen, size_t *clen)
{
    int err;
    size_t mlen;
    size_t nlen = ctx->cipher->nonce_len;
    size_t tlen = ctx->cipher->tag_len;

    if (*clen <= 2 * tlen + CHUNK_SIZE_LEN)
        return CRYPTO_NEED_MORE;

    uint8_t len_buf[2];
    err = aead_cipher_decrypt(ctx, len_buf, plen, c, CHUNK_SIZE_LEN + tlen,
                              NULL, 0, n, ctx->skey);
    if (err)
        return CRYPTO_ERROR;
    assert(*plen == CHUNK_SIZE_LEN);

    mlen = load16_be(len_buf);
    mlen = mlen & CHUNK_SIZE_MASK;

    if (mlen == 0)
        return CRYPTO_ERROR;

    size_t chunk_len = 2 * tlen + CHUNK_SIZE_LEN + mlen;

    if (*clen < chunk_len)
        return CRYPTO_NEED_MORE;

    sodium_increment(n, nlen);

    err = aead_cipher_decrypt(ctx, p, plen, c + CHUNK_SIZE_LEN + tlen, mlen + tlen,
                              NULL, 0, n, ctx->skey);
    if (err)
        return CRYPTO_ERROR;
    assert(*plen == mlen);

    sodium_increment(n, nlen);

    *clen = *clen - chunk_len;

    return CRYPTO_OK;
}

int
aead_decrypt(buffer_t *ciphertext, cipher_ctx_t *cipher_ctx, size_t capacity)
{
    int err             = CRYPTO_OK;
    static buffer_t tmp = { 0, 0, 0, NULL };

    cipher_t *cipher = cipher_ctx->cipher;

    size_t salt_len = cipher->key_len;

    if (cipher_ctx->chunk == NULL) {
        cipher_ctx->chunk = (buffer_t *)ss_malloc(sizeof(buffer_t));
        memset(cipher_ctx->chunk, 0, sizeof(buffer_t));
        balloc(cipher_ctx->chunk, capacity);
    }

    brealloc(cipher_ctx->chunk,
             cipher_ctx->chunk->len + ciphertext->len, capacity);
    memcpy(cipher_ctx->chunk->data + cipher_ctx->chunk->len,
           ciphertext->data, ciphertext->len);
    cipher_ctx->chunk->len += ciphertext->len;

    brealloc(&tmp, cipher_ctx->chunk->len, capacity);
    buffer_t *plaintext = &tmp;

    if (!cipher_ctx->init) {
        if (cipher_ctx->chunk->len <= salt_len)
            return CRYPTO_NEED_MORE;

        memcpy(cipher_ctx->salt, cipher_ctx->chunk->data, salt_len);

        aead_cipher_ctx_set_key(cipher_ctx, 0);

        if (ppbloom_check((void *)cipher_ctx->salt, salt_len) == 1) {
            LOGE("crypto: AEAD: repeat salt detected");
            return CRYPTO_ERROR;
        }

        memmove(cipher_ctx->chunk->data, cipher_ctx->chunk->data + salt_len,
                cipher_ctx->chunk->len - salt_len);
        cipher_ctx->chunk->len -= salt_len;

        cipher_ctx->init = 1;
    }

    size_t plen = 0;
    size_t cidx = 0;
    while (cipher_ctx->chunk->len > 0) {
        size_t chunk_clen = cipher_ctx->chunk->len;
        size_t chunk_plen = 0;
        err = aead_chunk_decrypt(cipher_ctx,
                                 (uint8_t *)plaintext->data + plen,
                                 (uint8_t *)cipher_ctx->chunk->data + cidx,
                                 cipher_ctx->nonce, &chunk_plen, &chunk_clen);
        if (err == CRYPTO_ERROR) {
            return err;
        } else if (err == CRYPTO_NEED_MORE) {
            if (plen == 0)
                return err;
            else{
                memmove((uint8_t *)cipher_ctx->chunk->data, 
			(uint8_t *)cipher_ctx->chunk->data + cidx, chunk_clen);
                break;
            }
        }
        cipher_ctx->chunk->len = chunk_clen;
        cidx += cipher_ctx->cipher->tag_len * 2 + CHUNK_SIZE_LEN + chunk_plen;
        plen                  += chunk_plen;
    }
    plaintext->len = plen;

    // Add the salt to bloom filter
    if (cipher_ctx->init == 1) {
        if (ppbloom_check((void *)cipher_ctx->salt, salt_len) == 1) {
            LOGE("crypto: AEAD: repeat salt detected");
            return CRYPTO_ERROR;
        }
        ppbloom_add((void *)cipher_ctx->salt, salt_len);
        cipher_ctx->init = 2;
    }

    brealloc(ciphertext, plaintext->len, capacity);
    memcpy(ciphertext->data, plaintext->data, plaintext->len);
    ciphertext->len = plaintext->len;

    return CRYPTO_OK;
}

cipher_t *
aead_key_init(int method, const char *pass, const char *key)
{
    if (method < AES128GCM || method >= AEAD_CIPHER_NUM) {
        LOGE("aead_key_init(): Illegal method");
        return NULL;
    }

    cipher_t *cipher = (cipher_t *)ss_malloc(sizeof(cipher_t));
    memset(cipher, 0, sizeof(cipher_t));

    if (method >= CHACHA20POLY1305IETF) {
        cipher_kt_t *cipher_info = (cipher_kt_t *)ss_malloc(sizeof(cipher_kt_t));
        cipher->info             = cipher_info;
        cipher->info->base       = NULL;
        cipher->info->key_bitlen = supported_aead_ciphers_key_size[method] * 8;
        cipher->info->iv_size    = supported_aead_ciphers_nonce_size[method];
    } else {
        cipher->info = (cipher_kt_t *)aead_get_cipher_type(method);
    }

    if (cipher->info == NULL && cipher->key_len == 0) {
        LOGE("Cipher %s not found in crypto library", supported_aead_ciphers[method]);
        FATAL("Cannot initialize cipher");
    }

    if (key != NULL)
        cipher->key_len = crypto_parse_key(key, cipher->key,
                                           supported_aead_ciphers_key_size[method]);
    else
        cipher->key_len = crypto_derive_key(pass, cipher->key,
                                            supported_aead_ciphers_key_size[method]);

    if (cipher->key_len == 0) {
        FATAL("Cannot generate key and nonce");
    }

    cipher->nonce_len = supported_aead_ciphers_nonce_size[method];
    cipher->tag_len   = supported_aead_ciphers_tag_size[method];
    cipher->method    = method;

    return cipher;
}

cipher_t *
aead_init(const char *pass, const char *key, const char *method)
{
    int m = AES128GCM;
    if (method != NULL) {
        /* check method validity */
        for (m = AES128GCM; m < AEAD_CIPHER_NUM; m++)
            if (strcmp(method, supported_aead_ciphers[m]) == 0) {
                break;
            }
        if (m >= AEAD_CIPHER_NUM) {
            LOGE("Invalid cipher name: %s, use chacha20-ietf-poly1305 instead", method);
            m = CHACHA20POLY1305IETF;
        }
    }
    return aead_key_init(m, pass, key);
}
