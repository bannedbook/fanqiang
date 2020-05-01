/**
 * @file parse_number.h
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * @section DESCRIPTION
 * 
 * Numeric string parsing.
 */

#ifndef BADVPN_MISC_PARSE_NUMBER_H
#define BADVPN_MISC_PARSE_NUMBER_H

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>

#include <misc/memref.h>
#include <misc/debug.h>

// public parsing functions
static int decode_decimal_digit (char c);
static int decode_hex_digit (char c);
static int parse_unsigned_integer (MemRef str, uintmax_t *out) WARN_UNUSED;
static int parse_unsigned_hex_integer (MemRef str, uintmax_t *out) WARN_UNUSED;
static int parse_signmag_integer (MemRef str, int *out_sign, uintmax_t *out_mag) WARN_UNUSED;

// public generation functions
static int compute_decimal_repr_size (uintmax_t x);
static void generate_decimal_repr (uintmax_t x, char *out, int repr_size);
static int generate_decimal_repr_string (uintmax_t x, char *out);

// implementation follows

// decimal representation of UINTMAX_MAX
static const char parse_number__uintmax_max_str[] = "18446744073709551615";

// make sure UINTMAX_MAX is what we think it is
static const char parse_number__uintmax_max_str_assert[(UINTMAX_MAX == UINTMAX_C(18446744073709551615)) ? 1 : -1];

static int decode_decimal_digit (char c)
{
    switch (c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
    }
    
    return -1;
}

static int decode_hex_digit (char c)
{
    switch (c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'A': case 'a': return 10;
        case 'B': case 'b': return 11;
        case 'C': case 'c': return 12;
        case 'D': case 'd': return 13;
        case 'E': case 'e': return 14;
        case 'F': case 'f': return 15;
    }
    
    return -1;
}

static int parse__no_overflow (const char *str, size_t str_len, uintmax_t *out)
{
    uintmax_t n = 0;
    
    while (str_len > 0) {
        if (*str < '0' || *str > '9') {
            return 0;
        }
        
        n = 10 * n + (*str - '0');
        
        str++;
        str_len--;
    }
    
    *out = n;
    return 1;
}

int parse_unsigned_integer (MemRef str, uintmax_t *out)
{
    // we do not allow empty strings
    if (str.len == 0) {
        return 0;
    }
    
    // remove leading zeros
    while (str.len > 0 && *str.ptr == '0') {
        str.ptr++;
        str.len--;
    }
    
    // detect overflow
    if (str.len > sizeof(parse_number__uintmax_max_str) - 1 ||
        (str.len == sizeof(parse_number__uintmax_max_str) - 1 && memcmp(str.ptr, parse_number__uintmax_max_str, sizeof(parse_number__uintmax_max_str) - 1) > 0)) {
        return 0;
    }
    
    // will not overflow (but can still have invalid characters)
    return parse__no_overflow(str.ptr, str.len, out);
}

int parse_unsigned_hex_integer (MemRef str, uintmax_t *out)
{
    uintmax_t n = 0;
    
    if (str.len == 0) {
        return 0;
    }
    
    while (str.len > 0) {
        int digit = decode_hex_digit(*str.ptr);
        if (digit < 0) {
            return 0;
        }
        
        if (n > UINTMAX_MAX / 16) {
            return 0;
        }
        n *= 16;
        
        if (digit > UINTMAX_MAX - n) {
            return 0;
        }
        n += digit;
        
        str.ptr++;
        str.len--;
    }
    
    *out = n;
    return 1;
}

int parse_signmag_integer (MemRef str, int *out_sign, uintmax_t *out_mag)
{
    int sign = 1;
    if (str.len > 0 && (str.ptr[0] == '+' || str.ptr[0] == '-')) {
        sign = 1 - 2 * (str.ptr[0] == '-');
        str.ptr++;
        str.len--;
    }
    
    if (!parse_unsigned_integer(str, out_mag)) {
        return 0;
    }
    
    *out_sign = sign;
    return 1;
}

int compute_decimal_repr_size (uintmax_t x)
{
    int size = 0;
    
    do {
        size++;
        x /= 10;
    } while (x > 0);
    
    return size;
}

void generate_decimal_repr (uintmax_t x, char *out, int repr_size)
{
    ASSERT(out)
    ASSERT(repr_size == compute_decimal_repr_size(x))
    
    out += repr_size;
    
    do {
        *(--out) = '0' + (x % 10);
        x /= 10;
    } while (x > 0);
}

int generate_decimal_repr_string (uintmax_t x, char *out)
{
    ASSERT(out)
    
    int repr_size = compute_decimal_repr_size(x);
    generate_decimal_repr(x, out, repr_size);
    out[repr_size] = '\0';
    
    return repr_size;
}

#endif
