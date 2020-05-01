/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <string.h>
#include <stdio.h>

#include "libcork/core/types.h"
#include "libcork/core/u128.h"


/* From http://stackoverflow.com/questions/8023414/how-to-convert-a-128-bit-integer-to-a-decimal-ascii-string-in-c */

const char *
cork_u128_to_decimal(char *dest, cork_u128 val)
{
    uint32_t  n[4];
    char  *s = dest;
    char  *p = dest;
    unsigned int  i;

    /* This algorithm assumes that n[3] is the MSW. */
    n[3] = cork_u128_be32(val, 0);
    n[2] = cork_u128_be32(val, 1);
    n[1] = cork_u128_be32(val, 2);
    n[0] = cork_u128_be32(val, 3);

    memset(s, '0', CORK_U128_DECIMAL_LENGTH - 1);
    s[CORK_U128_DECIMAL_LENGTH - 1] = '\0';

    for (i = 0; i < 128; i++) {
        unsigned int  j;
        unsigned int carry;

        carry = (n[3] >= 0x80000000);
        /* Shift n[] left, doubling it */
        n[3] = ((n[3] << 1) & 0xFFFFFFFF) + (n[2] >= 0x80000000);
        n[2] = ((n[2] << 1) & 0xFFFFFFFF) + (n[1] >= 0x80000000);
        n[1] = ((n[1] << 1) & 0xFFFFFFFF) + (n[0] >= 0x80000000);
        n[0] = ((n[0] << 1) & 0xFFFFFFFF);

        /* Add s[] to itself in decimal, doubling it */
        for (j = CORK_U128_DECIMAL_LENGTH - 1; j-- > 0; ) {
            s[j] += s[j] - '0' + carry;
            carry = (s[j] > '9');
            if (carry) {
                s[j] -= 10;
            }
        }
    }

    while ((p[0] == '0') && (p < &s[CORK_U128_DECIMAL_LENGTH - 2])) {
        p++;
    }

    return p;
}


const char *
cork_u128_to_hex(char *buf, cork_u128 val)
{
    uint64_t  hi = val._.be64.hi;
    uint64_t  lo = val._.be64.lo;
    if (hi == 0) {
        snprintf(buf, CORK_U128_HEX_LENGTH, "%" PRIx64, lo);
    } else {
        snprintf(buf, CORK_U128_HEX_LENGTH, "%" PRIx64 "%016" PRIx64, hi, lo);
    }
    return buf;
}

const char *
cork_u128_to_padded_hex(char *buf, cork_u128 val)
{
    uint64_t  hi = val._.be64.hi;
    uint64_t  lo = val._.be64.lo;
    snprintf(buf, CORK_U128_HEX_LENGTH, "%016" PRIx64 "%016" PRIx64, hi, lo);
    return buf;
}
