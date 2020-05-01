/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_TIMESTAMP_H
#define LIBCORK_CORE_TIMESTAMP_H


#include <libcork/core/api.h>
#include <libcork/core/error.h>
#include <libcork/core/types.h>
#include <libcork/ds/buffer.h>


typedef uint64_t  cork_timestamp;


#define cork_timestamp_init_sec(ts, sec) \
    do { \
        *(ts) = (((uint64_t) (sec)) << 32); \
    } while (0)

#define cork_timestamp_init_gsec(ts, sec, gsec) \
    do { \
        *(ts) = (((uint64_t) (sec)) << 32) | \
                (((uint64_t) (gsec)) & 0xffffffff); \
    } while (0)

#define cork_timestamp_init_msec(ts, sec, msec) \
    do { \
        *(ts) = (((uint64_t) (sec)) << 32) | \
                ((((uint64_t) (msec)) << 32) / 1000); \
    } while (0)

#define cork_timestamp_init_usec(ts, sec, usec) \
    do { \
        *(ts) = (((uint64_t) (sec)) << 32) | \
                ((((uint64_t) (usec)) << 32) / 1000000); \
    } while (0)

#define cork_timestamp_init_nsec(ts, sec, nsec) \
    do { \
        *(ts) = (((uint64_t) (sec)) << 32) | \
                ((((uint64_t) (nsec)) << 32) / 1000000000); \
    } while (0)


CORK_API void
cork_timestamp_init_now(cork_timestamp *ts);


#define cork_timestamp_sec(ts)  ((uint32_t) ((ts) >> 32))
#define cork_timestamp_gsec(ts)  ((uint32_t) ((ts) & 0xffffffff))

CORK_ATTR_UNUSED
static inline uint64_t
cork_timestamp_gsec_to_units(const cork_timestamp ts, uint64_t denom)
{
    uint64_t  half = ((uint64_t) 1 << 31) / denom;
    uint64_t  gsec = cork_timestamp_gsec(ts);
    gsec += half;
    gsec *= denom;
    gsec >>= 32;
    return gsec;
}

#define cork_timestamp_msec(ts)  cork_timestamp_gsec_to_units(ts, 1000)
#define cork_timestamp_usec(ts)  cork_timestamp_gsec_to_units(ts, 1000000)
#define cork_timestamp_nsec(ts)  cork_timestamp_gsec_to_units(ts, 1000000000)


CORK_API int
cork_timestamp_format_utc(const cork_timestamp ts, const char *format,
                          struct cork_buffer *dest);

CORK_API int
cork_timestamp_format_local(const cork_timestamp ts, const char *format,
                            struct cork_buffer *dest);


#endif /* LIBCORK_CORE_TIMESTAMP_H */
