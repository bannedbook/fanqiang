/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_HASH_H
#define LIBCORK_CORE_HASH_H


#include <libcork/core/api.h>
#include <libcork/core/attributes.h>
#include <libcork/core/byte-order.h>
#include <libcork/core/types.h>
#include <libcork/core/u128.h>

/* Needed for memcpy */
#include <string.h>
#include <stdlib.h>

#ifndef CORK_HASH_ATTRIBUTES
#define CORK_HASH_ATTRIBUTES  CORK_ATTR_UNUSED static inline
#endif


typedef uint32_t  cork_hash;

typedef struct {
    cork_u128  u128;
} cork_big_hash;

#define cork_big_hash_equal(h1, h2)  (cork_u128_eq((h1).u128, (h2).u128))

#define CORK_BIG_HASH_INIT()  {{{{0}}}}

/* We currently use MurmurHash3 [1], which is public domain, as our hash
 * implementation.
 *
 * [1] http://code.google.com/p/smhasher/
 */

#define CORK_ROTL32(a,b) (((a) << ((b) & 0x1f)) | ((a) >> (32 - ((b) & 0x1f))))
#define CORK_ROTL64(a,b) (((a) << ((b) & 0x3f)) | ((a) >> (64 - ((b) & 0x3f))))

CORK_ATTR_UNUSED
static inline
uint32_t cork_getblock32(const uint32_t *p, int i)
{
    uint32_t u;
    memcpy(&u, p + i, sizeof(u));
    return u;
}

CORK_ATTR_UNUSED
static inline
uint64_t cork_getblock64(const uint64_t *p, int i)
{
    uint64_t u;
    memcpy(&u, p + i, sizeof(u));
    return u;
}

CORK_ATTR_UNUSED
static inline
uint32_t cork_fmix32(uint32_t h)
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

CORK_ATTR_UNUSED
static inline
uint64_t cork_fmix64(uint64_t k)
{
    k ^= k >> 33;
    k *= UINT64_C(0xff51afd7ed558ccd);
    k ^= k >> 33;
    k *= UINT64_C(0xc4ceb9fe1a85ec53);
    k ^= k >> 33;
    return k;
}

CORK_HASH_ATTRIBUTES
cork_hash
cork_stable_hash_buffer(cork_hash seed, const void *src, size_t len)
{
    typedef uint32_t __attribute__((__may_alias__))  cork_aliased_uint32_t;

    /* This is exactly the same as cork_murmur_hash_x86_32, but with a byte swap
     * to make sure that we always process the uint32s little-endian. */
    const unsigned int  nblocks = (unsigned int)(len / 4);
    const cork_aliased_uint32_t  *blocks = (const cork_aliased_uint32_t *) src;
    const cork_aliased_uint32_t  *end = blocks + nblocks;
    const cork_aliased_uint32_t  *curr;
    const uint8_t  *tail = (const uint8_t *) end;

    uint32_t  h1 = seed;
    uint32_t  c1 = 0xcc9e2d51;
    uint32_t  c2 = 0x1b873593;
    uint32_t  k1 = 0;

    /* body */
    for (curr = blocks; curr != end; curr++) {
        uint32_t  k1 = CORK_UINT32_HOST_TO_LITTLE(cork_getblock32((const uint32_t *) curr, 0));

        k1 *= c1;
        k1 = CORK_ROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = CORK_ROTL32(h1,13);
        h1 = h1*5+0xe6546b64;
    }

    /* tail */
    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
                k1 *= c1; k1 = CORK_ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };

    /* finalization */
    h1 ^= len;
    h1 = cork_fmix32(h1);
    return h1;
}

#define cork_murmur_hash_x86_32(seed, src, len, dest) \
do { \
    typedef uint32_t __attribute__((__may_alias__))  cork_aliased_uint32_t; \
    \
    const unsigned int  nblocks = len / 4; \
    const cork_aliased_uint32_t  *blocks = (const cork_aliased_uint32_t *) src; \
    const cork_aliased_uint32_t  *end = blocks + nblocks; \
    const cork_aliased_uint32_t  *curr; \
    const uint8_t  *tail = (const uint8_t *) end; \
    \
    uint32_t  h1 = seed; \
    uint32_t  c1 = 0xcc9e2d51; \
    uint32_t  c2 = 0x1b873593; \
    uint32_t  k1 = 0; \
    \
    /* body */ \
    for (curr = blocks; curr != end; curr++) { \
        uint32_t  k1 = cork_getblock32((const uint32_t *) curr, 0); \
        \
        k1 *= c1; \
        k1 = CORK_ROTL32(k1,15); \
        k1 *= c2; \
        \
        h1 ^= k1; \
        h1 = CORK_ROTL32(h1,13); \
        h1 = h1*5+0xe6546b64; \
    } \
    \
    /* tail */ \
    switch (len & 3) { \
        case 3: k1 ^= tail[2] << 16; \
        case 2: k1 ^= tail[1] << 8; \
        case 1: k1 ^= tail[0]; \
                k1 *= c1; k1 = CORK_ROTL32(k1,15); k1 *= c2; h1 ^= k1; \
    }; \
    \
    /* finalization */ \
    h1 ^= len; \
    h1 = cork_fmix32(h1); \
    *(dest) = h1; \
} while (0)

#define cork_murmur_hash_x86_128(seed, src, len, dest) \
do { \
    typedef uint32_t __attribute__((__may_alias__))  cork_aliased_uint32_t; \
    \
    const unsigned int  nblocks = len / 16; \
    const cork_aliased_uint32_t  *blocks = (const cork_aliased_uint32_t *) src; \
    const cork_aliased_uint32_t  *end = blocks + (nblocks * 4); \
    const cork_aliased_uint32_t  *curr; \
    const uint8_t  *tail = (const uint8_t *) end; \
    \
    uint32_t  h1 = cork_u128_be32(seed.u128, 0); \
    uint32_t  h2 = cork_u128_be32(seed.u128, 1); \
    uint32_t  h3 = cork_u128_be32(seed.u128, 2); \
    uint32_t  h4 = cork_u128_be32(seed.u128, 3); \
    \
    uint32_t  c1 = 0x239b961b; \
    uint32_t  c2 = 0xab0e9789; \
    uint32_t  c3 = 0x38b34ae5; \
    uint32_t  c4 = 0xa1e38b93; \
    \
    uint32_t  k1 = 0; \
    uint32_t  k2 = 0; \
    uint32_t  k3 = 0; \
    uint32_t  k4 = 0; \
    \
    /* body */ \
    for (curr = blocks; curr != end; curr += 4) { \
        uint32_t  k1 = cork_getblock32((const uint32_t *) curr, 0); \
        uint32_t  k2 = cork_getblock32((const uint32_t *) curr, 1); \
        uint32_t  k3 = cork_getblock32((const uint32_t *) curr, 2); \
        uint32_t  k4 = cork_getblock32((const uint32_t *) curr, 3); \
        \
        k1 *= c1; k1  = CORK_ROTL32(k1,15); k1 *= c2; h1 ^= k1; \
        h1 = CORK_ROTL32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b; \
        \
        k2 *= c2; k2  = CORK_ROTL32(k2,16); k2 *= c3; h2 ^= k2; \
        h2 = CORK_ROTL32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747; \
        \
        k3 *= c3; k3  = CORK_ROTL32(k3,17); k3 *= c4; h3 ^= k3; \
        h3 = CORK_ROTL32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35; \
        \
        k4 *= c4; k4  = CORK_ROTL32(k4,18); k4 *= c1; h4 ^= k4; \
        h4 = CORK_ROTL32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17; \
    } \
    \
    /* tail */ \
    switch (len & 15) { \
        case 15: k4 ^= tail[14] << 16; \
        case 14: k4 ^= tail[13] << 8; \
        case 13: k4 ^= tail[12] << 0; \
                 k4 *= c4; k4 = CORK_ROTL32(k4,18); k4 *= c1; h4 ^= k4; \
        \
        case 12: k3 ^= tail[11] << 24; \
        case 11: k3 ^= tail[10] << 16; \
        case 10: k3 ^= tail[ 9] << 8; \
        case  9: k3 ^= tail[ 8] << 0; \
                 k3 *= c3; k3 = CORK_ROTL32(k3,17); k3 *= c4; h3 ^= k3; \
        \
        case  8: k2 ^= tail[ 7] << 24; \
        case  7: k2 ^= tail[ 6] << 16; \
        case  6: k2 ^= tail[ 5] << 8; \
        case  5: k2 ^= tail[ 4] << 0; \
                 k2 *= c2; k2 = CORK_ROTL32(k2,16); k2 *= c3; h2 ^= k2; \
        \
        case  4: k1 ^= tail[ 3] << 24; \
        case  3: k1 ^= tail[ 2] << 16; \
        case  2: k1 ^= tail[ 1] << 8; \
        case  1: k1 ^= tail[ 0] << 0; \
                 k1 *= c1; k1 = CORK_ROTL32(k1,15); k1 *= c2; h1 ^= k1; \
    }; \
    \
    /* finalization */ \
    \
    h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len; \
    \
    h1 += h2; h1 += h3; h1 += h4; \
    h2 += h1; h3 += h1; h4 += h1; \
    \
    h1 = cork_fmix32(h1); \
    h2 = cork_fmix32(h2); \
    h3 = cork_fmix32(h3); \
    h4 = cork_fmix32(h4); \
    \
    h1 += h2; h1 += h3; h1 += h4; \
    h2 += h1; h3 += h1; h4 += h1; \
    \
    (dest)->u128 = cork_u128_from_32(h1, h2, h3, h4); \
} while (0)

#define cork_murmur_hash_x64_128(seed, src, len, dest) \
do { \
    typedef uint64_t __attribute__((__may_alias__))  cork_aliased_uint64_t; \
    \
    const unsigned int  nblocks = len / 16; \
    const cork_aliased_uint64_t  *blocks = (const cork_aliased_uint64_t *) src; \
    const cork_aliased_uint64_t  *end = blocks + (nblocks * 2); \
    const cork_aliased_uint64_t  *curr; \
    const uint8_t  *tail = (const uint8_t *) end; \
    \
    uint64_t  h1 = cork_u128_be64(seed.u128, 0); \
    uint64_t  h2 = cork_u128_be64(seed.u128, 1); \
    \
    uint64_t  c1 = UINT64_C(0x87c37b91114253d5); \
    uint64_t  c2 = UINT64_C(0x4cf5ad432745937f); \
    \
    uint64_t k1 = 0; \
    uint64_t k2 = 0; \
    \
    /* body */ \
    for (curr = blocks; curr != end; curr += 2) { \
        uint64_t  k1 = cork_getblock64((const uint64_t *) curr, 0); \
        uint64_t  k2 = cork_getblock64((const uint64_t *) curr, 1); \
    \
        k1 *= c1; k1  = CORK_ROTL64(k1,31); k1 *= c2; h1 ^= k1; \
        h1 = CORK_ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729; \
    \
        k2 *= c2; k2  = CORK_ROTL64(k2,33); k2 *= c1; h2 ^= k2; \
        h2 = CORK_ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5; \
    } \
    \
    /* tail */ \
    switch (len & 15) { \
        case 15: k2 ^= (uint64_t) (tail[14]) << 48; \
        case 14: k2 ^= (uint64_t) (tail[13]) << 40; \
        case 13: k2 ^= (uint64_t) (tail[12]) << 32; \
        case 12: k2 ^= (uint64_t) (tail[11]) << 24; \
        case 11: k2 ^= (uint64_t) (tail[10]) << 16; \
        case 10: k2 ^= (uint64_t) (tail[ 9]) << 8; \
        case  9: k2 ^= (uint64_t) (tail[ 8]) << 0; \
                 k2 *= c2; k2 = CORK_ROTL64(k2,33); k2 *= c1; h2 ^= k2; \
        \
        case  8: k1 ^= (uint64_t) (tail[ 7]) << 56; \
        case  7: k1 ^= (uint64_t) (tail[ 6]) << 48; \
        case  6: k1 ^= (uint64_t) (tail[ 5]) << 40; \
        case  5: k1 ^= (uint64_t) (tail[ 4]) << 32; \
        case  4: k1 ^= (uint64_t) (tail[ 3]) << 24; \
        case  3: k1 ^= (uint64_t) (tail[ 2]) << 16; \
        case  2: k1 ^= (uint64_t) (tail[ 1]) << 8; \
        case  1: k1 ^= (uint64_t) (tail[ 0]) << 0; \
                 k1 *= c1; k1 = CORK_ROTL64(k1,31); k1 *= c2; h1 ^= k1; \
    }; \
    \
    /* finalization */ \
    \
    h1 ^= len; h2 ^= len; \
    \
    h1 += h2; \
    h2 += h1; \
    \
    h1 = cork_fmix64(h1); \
    h2 = cork_fmix64(h2); \
    \
    h1 += h2; \
    h2 += h1; \
    \
    (dest)->u128 = cork_u128_from_64(h1, h2); \
} while (0)


#include <stdio.h>
CORK_HASH_ATTRIBUTES
cork_hash
cork_hash_buffer(cork_hash seed, const void *src, size_t len)
{
#if CORK_SIZEOF_POINTER == 8
    cork_big_hash  big_seed = {cork_u128_from_32(seed, seed, seed, seed)};
    cork_big_hash  hash;
    cork_murmur_hash_x64_128(big_seed, src, (unsigned int)len, &hash);
    return cork_u128_be32(hash.u128, 0);
#else
    cork_hash  hash = 0;
#ifdef __ANDROID__
    // Enforce 16-byte alignment for murmur hash function
    void *tmp;
    int err = posix_memalign(&tmp, 16, len);
    if (err == 0) {
        memcpy(tmp, src, len);
        cork_murmur_hash_x86_32(seed, tmp, (unsigned int)len, &hash);
        free(tmp);
    }
#else
    cork_murmur_hash_x86_32(seed, src, (unsigned int)len, &hash);
#endif
    return hash;
#endif
}


CORK_HASH_ATTRIBUTES
cork_big_hash
cork_big_hash_buffer(cork_big_hash seed, const void *src, size_t len)
{
    cork_big_hash  result;
#if CORK_SIZEOF_POINTER == 8
    cork_murmur_hash_x64_128(seed, src, (unsigned int)len, &result);
#else
    cork_murmur_hash_x86_128(seed, src, (unsigned int)len, &result);
#endif
    return result;
}


#define cork_hash_variable(seed, val) \
    (cork_hash_buffer((seed), &(val), sizeof((val))))
#define cork_stable_hash_variable(seed, val) \
    (cork_stable_hash_buffer((seed), &(val), sizeof((val))))
#define cork_big_hash_variable(seed, val) \
    (cork_big_hash_buffer((seed), &(val), sizeof((val))))


#endif /* LIBCORK_CORE_HASH_H */
