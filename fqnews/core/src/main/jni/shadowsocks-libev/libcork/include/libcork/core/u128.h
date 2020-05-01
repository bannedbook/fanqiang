/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_U128_H
#define LIBCORK_CORE_U128_H


#include <libcork/config.h>
#include <libcork/core/attributes.h>
#include <libcork/core/byte-order.h>
#include <libcork/core/types.h>

typedef struct {
    union {
        uint8_t  u8[16];
        uint16_t  u16[8];
        uint32_t  u32[4];
        uint64_t  u64[2];
#if CORK_HOST_ENDIANNESS == CORK_BIG_ENDIAN
        struct { uint64_t hi; uint64_t lo; } be64;
#else
        struct { uint64_t lo; uint64_t hi; } be64;
#endif
#if CORK_CONFIG_HAVE_GCC_INT128
#define CORK_U128_HAVE_U128  1
        unsigned __int128  u128;
#elif CORK_CONFIG_HAVE_GCC_MODE_ATTRIBUTE
#define CORK_U128_HAVE_U128  1
        unsigned int  u128 __attribute__((mode(TI)));
#else
#define CORK_U128_HAVE_U128  0
#endif
    } _;
} cork_u128;


/* i0-3 are given in big-endian order, regardless of host endianness */
CORK_ATTR_UNUSED
static cork_u128
cork_u128_from_32(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3)
{
    cork_u128  value;
#if CORK_HOST_ENDIANNESS == CORK_BIG_ENDIAN
    value._.u32[0] = i0;
    value._.u32[1] = i1;
    value._.u32[2] = i2;
    value._.u32[3] = i3;
#else
    value._.u32[3] = i0;
    value._.u32[2] = i1;
    value._.u32[1] = i2;
    value._.u32[0] = i3;
#endif
    return value;
}

/* i0-1 are given in big-endian order, regardless of host endianness */
CORK_ATTR_UNUSED
static cork_u128
cork_u128_from_64(uint64_t i0, uint64_t i1)
{
    cork_u128  value;
#if CORK_HOST_ENDIANNESS == CORK_BIG_ENDIAN
    value._.u64[0] = i0;
    value._.u64[1] = i1;
#else
    value._.u64[1] = i0;
    value._.u64[0] = i1;
#endif
    return value;
}


#if CORK_HOST_ENDIANNESS == CORK_BIG_ENDIAN
#define cork_u128_be8(val, idx)   ((val)._.u8[(idx)])
#define cork_u128_be16(val, idx)  ((val)._.u16[(idx)])
#define cork_u128_be32(val, idx)  ((val)._.u32[(idx)])
#define cork_u128_be64(val, idx)  ((val)._.u64[(idx)])
#else
#define cork_u128_be8(val, idx)   ((val)._.u8[15 - (idx)])
#define cork_u128_be16(val, idx)  ((val)._.u16[7 - (idx)])
#define cork_u128_be32(val, idx)  ((val)._.u32[3 - (idx)])
#define cork_u128_be64(val, idx)  ((val)._.u64[1 - (idx)])
#endif


CORK_ATTR_UNUSED
static cork_u128
cork_u128_add(cork_u128 a, cork_u128 b)
{
    cork_u128  result;
#if CORK_U128_HAVE_U128
    result._.u128 = a._.u128 + b._.u128;
#else
    result._.be64.lo = a._.be64.lo + b._.be64.lo;
    result._.be64.hi =
        a._.be64.hi + b._.be64.hi + (result._.be64.lo < a._.be64.lo);
#endif
    return result;
}

CORK_ATTR_UNUSED
static cork_u128
cork_u128_sub(cork_u128 a, cork_u128 b)
{
    cork_u128  result;
#if CORK_U128_HAVE_U128
    result._.u128 = a._.u128 - b._.u128;
#else
    result._.be64.lo = a._.be64.lo - b._.be64.lo;
    result._.be64.hi =
        a._.be64.hi - b._.be64.hi - (result._.be64.lo > a._.be64.lo);
#endif
    return result;
}


CORK_ATTR_UNUSED
static bool
cork_u128_eq(cork_u128 a, cork_u128 b)
{
#if CORK_U128_HAVE_U128
    return (a._.u128 == b._.u128);
#else
    return (a._.be64.hi == b._.be64.hi) && (a._.be64.lo == b._.be64.lo);
#endif
}

CORK_ATTR_UNUSED
static bool
cork_u128_ne(cork_u128 a, cork_u128 b)
{
#if CORK_U128_HAVE_U128
    return (a._.u128 != b._.u128);
#else
    return (a._.be64.hi != b._.be64.hi) || (a._.be64.lo != b._.be64.lo);
#endif
}

CORK_ATTR_UNUSED
static bool
cork_u128_lt(cork_u128 a, cork_u128 b)
{
#if CORK_U128_HAVE_U128
    return (a._.u128 < b._.u128);
#else
    if (a._.be64.hi == b._.be64.hi) {
        return a._.be64.lo < b._.be64.lo;
    } else {
        return a._.be64.hi < b._.be64.hi;
    }
#endif
}

CORK_ATTR_UNUSED
static bool
cork_u128_le(cork_u128 a, cork_u128 b)
{
#if CORK_U128_HAVE_U128
    return (a._.u128 <= b._.u128);
#else
    if (a._.be64.hi == b._.be64.hi) {
        return a._.be64.lo <= b._.be64.lo;
    } else {
        return a._.be64.hi <= b._.be64.hi;
    }
#endif
}

CORK_ATTR_UNUSED
static bool
cork_u128_gt(cork_u128 a, cork_u128 b)
{
#if CORK_U128_HAVE_U128
    return (a._.u128 > b._.u128);
#else
    if (a._.be64.hi == b._.be64.hi) {
        return a._.be64.lo > b._.be64.lo;
    } else {
        return a._.be64.hi > b._.be64.hi;
    }
#endif
}

CORK_ATTR_UNUSED
static bool
cork_u128_ge(cork_u128 a, cork_u128 b)
{
#if CORK_U128_HAVE_U128
    return (a._.u128 >= b._.u128);
#else
    if (a._.be64.hi == b._.be64.hi) {
        return a._.be64.lo >= b._.be64.lo;
    } else {
        return a._.be64.hi >= b._.be64.hi;
    }
#endif
}


/* log10(x) = log2(x) / log2(10) ~= log2(x) / 3.322 */
#define CORK_U128_DECIMAL_LENGTH  44  /* ~= 128 / 3 + 1 + 1 */

CORK_API const char *
cork_u128_to_decimal(char *buf, cork_u128 val);


#define CORK_U128_HEX_LENGTH  33

CORK_API const char *
cork_u128_to_hex(char *buf, cork_u128 val);

CORK_API const char *
cork_u128_to_padded_hex(char *buf, cork_u128 val);


#endif /* LIBCORK_CORE_U128_H */
