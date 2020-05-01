/*
 * crypto.c - Manage the global crypto
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

#if defined(__linux__) && defined(HAVE_LINUX_RANDOM_H)
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/random.h>
#endif

#include <stdint.h>
#include <sodium.h>
#include <mbedtls/version.h>
#include <mbedtls/md5.h>

#include "base64.h"
#include "crypto.h"
#include "stream.h"
#include "aead.h"
#include "utils.h"
#include "ppbloom.h"

int
balloc(buffer_t *ptr, size_t capacity)
{
    sodium_memzero(ptr, sizeof(buffer_t));
    ptr->data     = ss_malloc(capacity);
    ptr->capacity = capacity;
    return capacity;
}

int
brealloc(buffer_t *ptr, size_t len, size_t capacity)
{
    if (ptr == NULL)
        return -1;
    size_t real_capacity = max(len, capacity);
    if (ptr->capacity < real_capacity) {
        ptr->data     = ss_realloc(ptr->data, real_capacity);
        ptr->capacity = real_capacity;
    }
    return real_capacity;
}

void
bfree(buffer_t *ptr)
{
    if (ptr == NULL)
        return;
    ptr->idx      = 0;
    ptr->len      = 0;
    ptr->capacity = 0;
    if (ptr->data != NULL) {
        ss_free(ptr->data);
    }
}

int
bprepend(buffer_t *dst, buffer_t *src, size_t capacity)
{
    brealloc(dst, dst->len + src->len, capacity);
    memmove(dst->data + src->len, dst->data, dst->len);
    memcpy(dst->data, src->data, src->len);
    dst->len = dst->len + src->len;
    return dst->len;
}

int
rand_bytes(void *output, int len)
{
    randombytes_buf(output, len);
    // always return success
    return 0;
}

unsigned char *
crypto_md5(const unsigned char *d, size_t n, unsigned char *md)
{
    static unsigned char m[16];
    if (md == NULL) {
        md = m;
    }
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
    if (mbedtls_md5_ret(d, n, md) != 0)
        FATAL("Failed to calculate MD5");
#else
    mbedtls_md5(d, n, md);
#endif
    return md;
}

static void
entropy_check(void)
{
#if defined(__linux__) && defined(HAVE_LINUX_RANDOM_H) && defined(RNDGETENTCNT)
    int fd;
    int c;

    if ((fd = open("/dev/random", O_RDONLY)) != -1) {
        if (ioctl(fd, RNDGETENTCNT, &c) == 0 && c < 160) {
            LOGI("This system doesn't provide enough entropy to quickly generate high-quality random numbers.\n"
                 "Installing the rng-utils/rng-tools, jitterentropy or haveged packages may help.\n"
                 "On virtualized Linux environments, also consider using virtio-rng.\n"
                 "The service will not start until enough entropy has been collected.\n");
        }
        close(fd);
    }
#endif
}

crypto_t *
crypto_init(const char *password, const char *key, const char *method)
{
    int i, m = -1;

    entropy_check();
    // Initialize sodium for random generator
    if (sodium_init() == -1) {
        FATAL("Failed to initialize sodium");
    }

    // Initialize NONCE bloom filter
#ifdef MODULE_REMOTE
    ppbloom_init(BF_NUM_ENTRIES_FOR_SERVER, BF_ERROR_RATE_FOR_SERVER);
#else
    ppbloom_init(BF_NUM_ENTRIES_FOR_CLIENT, BF_ERROR_RATE_FOR_CLIENT);
#endif

    if (method != NULL) {
        for (i = 0; i < STREAM_CIPHER_NUM; i++)
            if (strcmp(method, supported_stream_ciphers[i]) == 0) {
                m = i;
                break;
            }
        if (m != -1) {
            LOGI("Stream ciphers are insecure, therefore deprecated, and should be almost always avoided.");
            cipher_t *cipher = stream_init(password, key, method);
            if (cipher == NULL)
                return NULL;
            crypto_t *crypto = (crypto_t *)ss_malloc(sizeof(crypto_t));
            crypto_t tmp     = {
                .cipher      = cipher,
                .encrypt_all = &stream_encrypt_all,
                .decrypt_all = &stream_decrypt_all,
                .encrypt     = &stream_encrypt,
                .decrypt     = &stream_decrypt,
                .ctx_init    = &stream_ctx_init,
                .ctx_release = &stream_ctx_release,
            };
            memcpy(crypto, &tmp, sizeof(crypto_t));
            return crypto;
        }

        for (i = 0; i < AEAD_CIPHER_NUM; i++)
            if (strcmp(method, supported_aead_ciphers[i]) == 0) {
                m = i;
                break;
            }
        if (m != -1) {
            cipher_t *cipher = aead_init(password, key, method);
            if (cipher == NULL)
                return NULL;
            crypto_t *crypto = (crypto_t *)ss_malloc(sizeof(crypto_t));
            crypto_t tmp     = {
                .cipher      = cipher,
                .encrypt_all = &aead_encrypt_all,
                .decrypt_all = &aead_decrypt_all,
                .encrypt     = &aead_encrypt,
                .decrypt     = &aead_decrypt,
                .ctx_init    = &aead_ctx_init,
                .ctx_release = &aead_ctx_release,
            };
            memcpy(crypto, &tmp, sizeof(crypto_t));
            return crypto;
        }
    }

    LOGE("invalid cipher name: %s", method);
    return NULL;
}

int
crypto_derive_key(const char *pass, uint8_t *key, size_t key_len)
{
    size_t datal;
    datal = strlen((const char *)pass);

    const digest_type_t *md = mbedtls_md_info_from_string("MD5");
    if (md == NULL) {
        FATAL("MD5 Digest not found in crypto library");
    }

    mbedtls_md_context_t c;
    unsigned char md_buf[MAX_MD_SIZE];
    int addmd;
    unsigned int i, j, mds;

    mds = mbedtls_md_get_size(md);
    memset(&c, 0, sizeof(mbedtls_md_context_t));

    if (pass == NULL)
        return key_len;
    if (mbedtls_md_setup(&c, md, 1))
        return 0;

    for (j = 0, addmd = 0; j < key_len; addmd++) {
        mbedtls_md_starts(&c);
        if (addmd) {
            mbedtls_md_update(&c, md_buf, mds);
        }
        mbedtls_md_update(&c, (uint8_t *)pass, datal);
        mbedtls_md_finish(&c, &(md_buf[0]));

        for (i = 0; i < mds; i++, j++) {
            if (j >= key_len)
                break;
            key[j] = md_buf[i];
        }
    }

    mbedtls_md_free(&c);
    return key_len;
}

/* HKDF-Extract + HKDF-Expand */
int
crypto_hkdf(const mbedtls_md_info_t *md, const unsigned char *salt,
            int salt_len, const unsigned char *ikm, int ikm_len,
            const unsigned char *info, int info_len, unsigned char *okm,
            int okm_len)
{
    unsigned char prk[MBEDTLS_MD_MAX_SIZE];

    return crypto_hkdf_extract(md, salt, salt_len, ikm, ikm_len, prk) ||
           crypto_hkdf_expand(md, prk, mbedtls_md_get_size(md), info, info_len,
                              okm, okm_len);
}

/* HKDF-Extract(salt, IKM) -> PRK */
int
crypto_hkdf_extract(const mbedtls_md_info_t *md, const unsigned char *salt,
                    int salt_len, const unsigned char *ikm, int ikm_len,
                    unsigned char *prk)
{
    int hash_len;
    unsigned char null_salt[MBEDTLS_MD_MAX_SIZE] = { '\0' };

    if (salt_len < 0) {
        return CRYPTO_ERROR;
    }

    hash_len = mbedtls_md_get_size(md);

    if (salt == NULL) {
        salt     = null_salt;
        salt_len = hash_len;
    }

    return mbedtls_md_hmac(md, salt, salt_len, ikm, ikm_len, prk);
}

/* HKDF-Expand(PRK, info, L) -> OKM */
int
crypto_hkdf_expand(const mbedtls_md_info_t *md, const unsigned char *prk,
                   int prk_len, const unsigned char *info, int info_len,
                   unsigned char *okm, int okm_len)
{
    int hash_len;
    int N;
    int T_len = 0, where = 0, i, ret;
    mbedtls_md_context_t ctx;
    unsigned char T[MBEDTLS_MD_MAX_SIZE];

    if (info_len < 0 || okm_len < 0 || okm == NULL) {
        return CRYPTO_ERROR;
    }

    hash_len = mbedtls_md_get_size(md);

    if (prk_len < hash_len) {
        return CRYPTO_ERROR;
    }

    if (info == NULL) {
        info = (const unsigned char *)"";
    }

    N = okm_len / hash_len;

    if ((okm_len % hash_len) != 0) {
        N++;
    }

    if (N > 255) {
        return CRYPTO_ERROR;
    }

    mbedtls_md_init(&ctx);

    if ((ret = mbedtls_md_setup(&ctx, md, 1)) != 0) {
        mbedtls_md_free(&ctx);
        return ret;
    }

    /* Section 2.3. */
    for (i = 1; i <= N; i++) {
        unsigned char c = i;

        ret = mbedtls_md_hmac_starts(&ctx, prk, prk_len) ||
              mbedtls_md_hmac_update(&ctx, T, T_len) ||
              mbedtls_md_hmac_update(&ctx, info, info_len) ||
              /* The constant concatenated to the end of each T(n) is a single
               * octet. */
              mbedtls_md_hmac_update(&ctx, &c, 1) ||
              mbedtls_md_hmac_finish(&ctx, T);

        if (ret != 0) {
            mbedtls_md_free(&ctx);
            return ret;
        }

        memcpy(okm + where, T, (i != N) ? hash_len : (okm_len - where));
        where += hash_len;
        T_len  = hash_len;
    }

    mbedtls_md_free(&ctx);

    return 0;
}

int
crypto_parse_key(const char *base64, uint8_t *key, size_t key_len)
{
    size_t base64_len = strlen(base64);
    int out_len       = BASE64_SIZE(base64_len);
    uint8_t out[out_len];

    out_len = base64_decode(out, base64, out_len);
    if (out_len > 0 && out_len >= key_len) {
        memcpy(key, out, key_len);
#ifdef SS_DEBUG
        dump("KEY", (char *)key, key_len);
#endif
        return key_len;
    }

    out_len = BASE64_SIZE(key_len);
    char out_key[out_len];
    rand_bytes(key, key_len);
    base64_encode(out_key, out_len, key, key_len);
    LOGE("Invalid key for your chosen cipher!");
    LOGE("It requires a " SIZE_FMT "-byte key encoded with URL-safe Base64", key_len);
    LOGE("Generating a new random key: %s", out_key);
    FATAL("Please use the key above or input a valid key");
    return key_len;
}

#ifdef SS_DEBUG
void
dump(char *tag, char *text, int len)
{
    int i;
    printf("%s: ", tag);
    for (i = 0; i < len; i++)
        printf("0x%02x ", (uint8_t)text[i]);
    printf("\n");
}

#endif
