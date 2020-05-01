/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_BYTE_ORDER_H
#define LIBCORK_CORE_BYTE_ORDER_H


#include <libcork/config.h>
#include <libcork/core/types.h>


/* Constants to represent big endianness and little endianness */
#define CORK_BIG_ENDIAN  4321
#define CORK_LITTLE_ENDIAN  1234

/* Whether the current host is big- or little-endian.  HOST gives us the
 * current system's endianness; OTHER gives the opposite endianness.
 * The _NAME macros can be used in debugging messages and other
 * human-readable output.
 *
 * Note that we actually detect the endianness in the various header
 * files in the libcork/config directory, since we want to keep
 * everything detection-related separated out from what we define based
 * on that detection. */

#if CORK_CONFIG_IS_BIG_ENDIAN
#define CORK_HOST_ENDIANNESS  CORK_BIG_ENDIAN
#define CORK_OTHER_ENDIANNESS  CORK_LITTLE_ENDIAN
#define CORK_HOST_ENDIANNESS_NAME   "big"
#define CORK_OTHER_ENDIANNESS_NAME  "little"

#elif CORK_CONFIG_IS_LITTLE_ENDIAN
#define CORK_HOST_ENDIANNESS  CORK_LITTLE_ENDIAN
#define CORK_OTHER_ENDIANNESS  CORK_BIG_ENDIAN
#define CORK_HOST_ENDIANNESS_NAME   "little"
#define CORK_OTHER_ENDIANNESS_NAME  "big"

#else
#error "Unknown endianness"
#endif


/* Returns the byte-swapped version an integer, regardless of the
 * underlying endianness.
 *
 * These macros only require an rvalue as their parameter (which can
 * therefore be any arbitrary expression), and they don't modify the
 * original contents if it happens to be a variable.  */

#define CORK_SWAP_UINT16(__u16) \
    (((((uint16_t) __u16) & 0xff00u) >> 8) | \
     ((((uint16_t) __u16) & 0x00ffu) << 8))

#define CORK_SWAP_UINT32(__u32) \
    (((((uint32_t) __u32) & 0xff000000u) >> 24) | \
     ((((uint32_t) __u32) & 0x00ff0000u) >>  8) | \
     ((((uint32_t) __u32) & 0x0000ff00u) <<  8) | \
     ((((uint32_t) __u32) & 0x000000ffu) << 24))

#define CORK_SWAP_UINT64(__u64) \
    (((((uint64_t) __u64) & UINT64_C(0xff00000000000000)) >> 56) | \
     ((((uint64_t) __u64) & UINT64_C(0x00ff000000000000)) >> 40) | \
     ((((uint64_t) __u64) & UINT64_C(0x0000ff0000000000)) >> 24) | \
     ((((uint64_t) __u64) & UINT64_C(0x000000ff00000000)) >>  8) | \
     ((((uint64_t) __u64) & UINT64_C(0x00000000ff000000)) <<  8) | \
     ((((uint64_t) __u64) & UINT64_C(0x0000000000ff0000)) << 24) | \
     ((((uint64_t) __u64) & UINT64_C(0x000000000000ff00)) << 40) | \
     ((((uint64_t) __u64) & UINT64_C(0x00000000000000ff)) << 56))

/* Bytes-swaps an integer variable in place.
 *
 * These macros require an lvalue as their parameter; the contents of
 * this variable will be modified by the macro. */

#define CORK_SWAP_IN_PLACE_UINT16(__u16) \
    do { \
        (__u16) = CORK_SWAP_UINT16(__u16); \
    } while (0)

#define CORK_SWAP_IN_PLACE_UINT32(__u32) \
    do { \
        (__u32) = CORK_SWAP_UINT32(__u32); \
    } while (0)

#define CORK_SWAP_IN_PLACE_UINT64(__u64) \
    do { \
        (__u64) = CORK_SWAP_UINT64(__u64); \
    } while (0)


/*
 * A slew of swapping macros whose operation depends on the endianness
 * of the current system:
 *
 * uint16_t CORK_UINT16_BIG_TO_HOST(u16)
 * uint32_t CORK_UINT32_BIG_TO_HOST(u32)
 * uint64_t CORK_UINT64_BIG_TO_HOST(u64)
 * uint16_t CORK_UINT16_LITTLE_TO_HOST(u16)
 * uint32_t CORK_UINT32_LITTLE_TO_HOST(u32)
 * uint64_t CORK_UINT64_LITTLE_TO_HOST(u64)
 * void CORK_UINT16_BIG_TO_HOST_IN_PLACE(&u16)
 * void CORK_UINT32_BIG_TO_HOST_IN_PLACE(&u32)
 * void CORK_UINT64_BIG_TO_HOST_IN_PLACE(&u64)
 * void CORK_UINT16_LITTLE_TO_HOST_IN_PLACE(&u16)
 * void CORK_UINT32_LITTLE_TO_HOST_IN_PLACE(&u32)
 * void CORK_UINT64_LITTLE_TO_HOST_IN_PLACE(&u64)
 *
 * uint16_t CORK_UINT16_HOST_TO_BIG(u16)
 * uint32_t CORK_UINT32_HOST_TO_BIG(u32)
 * uint64_t CORK_UINT64_HOST_TO_BIG(u64)
 * uint16_t CORK_UINT16_HOST_TO_LITTLE(u16)
 * uint32_t CORK_UINT32_HOST_TO_LITTLE(u32)
 * uint64_t CORK_UINT64_HOST_TO_LITTLE(u64)
 * void CORK_UINT16_HOST_TO_BIG_IN_PLACE(&u16)
 * void CORK_UINT32_HOST_TO_BIG_IN_PLACE(&u32)
 * void CORK_UINT64_HOST_TO_BIG_IN_PLACE(&u64)
 * void CORK_UINT16_HOST_TO_LITTLE_IN_PLACE(&u16)
 * void CORK_UINT32_HOST_TO_LITTLE_IN_PLACE(&u32)
 * void CORK_UINT64_HOST_TO_LITTLE_IN_PLACE(&u64)
 */

#if CORK_HOST_ENDIANNESS == CORK_BIG_ENDIAN

#define CORK_UINT16_BIG_TO_HOST(__u16) (__u16) /* nothing to do */
#define CORK_UINT16_LITTLE_TO_HOST(__u16)  CORK_SWAP_UINT16(__u16)

#define CORK_UINT32_BIG_TO_HOST(__u32) (__u32) /* nothing to do */
#define CORK_UINT32_LITTLE_TO_HOST(__u32)  CORK_SWAP_UINT32(__u32)

#define CORK_UINT64_BIG_TO_HOST(__u64) (__u64) /* nothing to do */
#define CORK_UINT64_LITTLE_TO_HOST(__u64)  CORK_SWAP_UINT64(__u64)

#define CORK_UINT16_BIG_TO_HOST_IN_PLACE(__u16) /* nothing to do */
#define CORK_UINT16_LITTLE_TO_HOST_IN_PLACE(__u16)  CORK_SWAP_IN_PLACE_UINT16(__u16)

#define CORK_UINT32_BIG_TO_HOST_IN_PLACE(__u32) /* nothing to do */
#define CORK_UINT32_LITTLE_TO_HOST_IN_PLACE(__u32)  CORK_SWAP_IN_PLACE_UINT32(__u32)

#define CORK_UINT64_BIG_TO_HOST_IN_PLACE(__u64) /* nothing to do */
#define CORK_UINT64_LITTLE_TO_HOST_IN_PLACE(__u64)  CORK_SWAP_IN_PLACE_UINT64(__u64)

#elif CORK_HOST_ENDIANNESS == CORK_LITTLE_ENDIAN

#define CORK_UINT16_BIG_TO_HOST(__u16)  CORK_SWAP_UINT16(__u16)
#define CORK_UINT16_LITTLE_TO_HOST(__u16) (__u16) /* nothing to do */

#define CORK_UINT32_BIG_TO_HOST(__u32)  CORK_SWAP_UINT32(__u32)
#define CORK_UINT32_LITTLE_TO_HOST(__u32) (__u32) /* nothing to do */

#define CORK_UINT64_BIG_TO_HOST(__u64)  CORK_SWAP_UINT64(__u64)
#define CORK_UINT64_LITTLE_TO_HOST(__u64) (__u64) /* nothing to do */

#define CORK_UINT16_BIG_TO_HOST_IN_PLACE(__u16)  CORK_SWAP_IN_PLACE_UINT16(__u16)
#define CORK_UINT16_LITTLE_TO_HOST_IN_PLACE(__u16) /* nothing to do */

#define CORK_UINT32_BIG_TO_HOST_IN_PLACE(__u32)  CORK_SWAP_IN_PLACE_UINT32(__u32)
#define CORK_UINT32_LITTLE_TO_HOST_IN_PLACE(__u32) /* nothing to do */

#define CORK_UINT64_BIG_TO_HOST_IN_PLACE(__u64)  CORK_SWAP_IN_PLACE_UINT64(__u64)
#define CORK_UINT64_LITTLE_TO_HOST_IN_PLACE(__u64) /* nothing to do */

#endif


#define CORK_UINT16_HOST_TO_BIG(__u16)  CORK_UINT16_BIG_TO_HOST(__u16)
#define CORK_UINT32_HOST_TO_BIG(__u32)  CORK_UINT32_BIG_TO_HOST(__u32)
#define CORK_UINT64_HOST_TO_BIG(__u64)  CORK_UINT64_BIG_TO_HOST(__u64)
#define CORK_UINT16_HOST_TO_LITTLE(__u16)  CORK_UINT16_LITTLE_TO_HOST(__u16)
#define CORK_UINT32_HOST_TO_LITTLE(__u32)  CORK_UINT32_LITTLE_TO_HOST(__u32)
#define CORK_UINT64_HOST_TO_LITTLE(__u64)  CORK_UINT64_LITTLE_TO_HOST(__u64)
#define CORK_UINT16_HOST_TO_BIG_IN_PLACE(__u16)  CORK_UINT16_BIG_TO_HOST_IN_PLACE(__u16)
#define CORK_UINT32_HOST_TO_BIG_IN_PLACE(__u32)  CORK_UINT32_BIG_TO_HOST_IN_PLACE(__u32)
#define CORK_UINT64_HOST_TO_BIG_IN_PLACE(__u64)  CORK_UINT64_BIG_TO_HOST_IN_PLACE(__u64)
#define CORK_UINT16_HOST_TO_LITTLE_IN_PLACE(__u16)  CORK_UINT16_LITTLE_TO_HOST_IN_PLACE(__u16)
#define CORK_UINT32_HOST_TO_LITTLE_IN_PLACE(__u32)  CORK_UINT32_LITTLE_TO_HOST_IN_PLACE(__u32)
#define CORK_UINT64_HOST_TO_LITTLE_IN_PLACE(__u64)  CORK_UINT64_LITTLE_TO_HOST_IN_PLACE(__u64)


#endif /* LIBCORK_CORE_BYTE_ORDER_H */
