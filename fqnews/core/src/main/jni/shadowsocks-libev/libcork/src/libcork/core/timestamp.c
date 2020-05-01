/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "libcork/core/timestamp.h"
#include "libcork/core/types.h"
#include "libcork/helpers/errors.h"
#include "libcork/helpers/mingw.h"

void
cork_timestamp_init_now(cork_timestamp *ts)
{
    struct timeval  tp;
    gettimeofday(&tp, NULL);
    cork_timestamp_init_usec(ts, tp.tv_sec, tp.tv_usec);
}


#define is_digit(ch)  ((ch) >= '0' && (ch) <= '9')

static uint64_t
power_of_10(unsigned int width)
{
    uint64_t  accumulator = 10;
    uint64_t  result = 1;
    while (width != 0) {
        if ((width % 2) == 1) {
            result *= accumulator;
            width--;
        }
        accumulator *= accumulator;
        width /= 2;
    }
    return result;
}

static int
append_fractional(const cork_timestamp ts, unsigned int width,
                  struct cork_buffer *dest)
{
    if (CORK_UNLIKELY(width == 0 || width > 9)) {
        cork_error_set_printf
            (EINVAL,
             "Invalid width %u for fractional cork_timestamp", width);
        return -1;
    } else {
        uint64_t  denom = power_of_10(width);
        uint64_t  frac = cork_timestamp_gsec_to_units(ts, denom);
        cork_buffer_append_printf(dest, "%0*" PRIu64, width, frac);
        return 0;
    }
}

static int
cork_timestamp_format_parts(const cork_timestamp ts, struct tm *tm,
                            const char *format, struct cork_buffer *dest)
{
    const char  *next_percent;

    while ((next_percent = strchr(format, '%')) != NULL) {
        const char  *spec = next_percent + 1;
        unsigned int  width = 0;

        /* First append any text in between the previous format specifier and
         * this one. */
        cork_buffer_append(dest, format, next_percent - format);

        /* Then parse the format specifier */
        while (is_digit(*spec)) {
            width *= 10;
            width += (*spec++ - '0');
        }

        switch (*spec) {
            case '\0':
                cork_error_set_string
                    (EINVAL,
                     "Trailing %% at end of cork_timestamp format string");
                return -1;

            case '%':
                cork_buffer_append(dest, "%", 1);
                break;

            case 'Y':
                cork_buffer_append_printf(dest, "%04d", tm->tm_year + 1900);
                break;

            case 'm':
                cork_buffer_append_printf(dest, "%02d", tm->tm_mon + 1);
                break;

            case 'd':
                cork_buffer_append_printf(dest, "%02d", tm->tm_mday);
                break;

            case 'H':
                cork_buffer_append_printf(dest, "%02d", tm->tm_hour);
                break;

            case 'M':
                cork_buffer_append_printf(dest, "%02d", tm->tm_min);
                break;

            case 'S':
                cork_buffer_append_printf(dest, "%02d", tm->tm_sec);
                break;

            case 's':
                cork_buffer_append_printf
                    (dest, "%" PRIu32, cork_timestamp_sec(ts));
                break;

            case 'f':
                rii_check(append_fractional(ts, width, dest));
                break;

            default:
                cork_error_set_printf
                    (EINVAL,
                     "Unknown cork_timestamp format specifier %%%c", *spec);
                return -1;
        }

        format = spec + 1;
    }

    /* When we fall through, there is some additional content after the final
     * format specifier. */
    cork_buffer_append_string(dest, format);
    return 0;
}

int
cork_timestamp_format_utc(const cork_timestamp ts, const char *format,
                          struct cork_buffer *dest)
{
    time_t  clock;
    struct tm  tm;
    clock = cork_timestamp_sec(ts);
    gmtime_r(&clock, &tm);
    return cork_timestamp_format_parts(ts, &tm, format, dest);
}

int
cork_timestamp_format_local(const cork_timestamp ts, const char *format,
                            struct cork_buffer *dest)
{
    time_t  clock;
    struct tm  tm;
    clock = cork_timestamp_sec(ts);
    localtime_r(&clock, &tm);
    return cork_timestamp_format_parts(ts, &tm, format, dest);
}
