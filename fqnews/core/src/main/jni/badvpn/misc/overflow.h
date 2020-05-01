/**
 * @file overflow.h
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
 * Functions for checking for overflow of integer addition.
 */

#ifndef BADVPN_MISC_OVERFLOW_H
#define BADVPN_MISC_OVERFLOW_H

#include <limits.h>
#include <stdint.h>

#define DEFINE_UNSIGNED_OVERFLOW(_name, _type, _max) \
static int add_ ## _name ## _overflows (_type a, _type b) \
{\
    return (b > _max - a); \
}

#define DEFINE_SIGNED_OVERFLOW(_name, _type, _min, _max) \
static int add_ ## _name ## _overflows (_type a, _type b) \
{\
    if ((a < 0) ^ (b < 0)) return 0; \
    if (a < 0) return -(a < _min - b); \
    return (a > _max - b); \
}

DEFINE_UNSIGNED_OVERFLOW(uint, unsigned int, UINT_MAX)
DEFINE_UNSIGNED_OVERFLOW(uint8, uint8_t, UINT8_MAX)
DEFINE_UNSIGNED_OVERFLOW(uint16, uint16_t, UINT16_MAX)
DEFINE_UNSIGNED_OVERFLOW(uint32, uint32_t, UINT32_MAX)
DEFINE_UNSIGNED_OVERFLOW(uint64, uint64_t, UINT64_MAX)

DEFINE_SIGNED_OVERFLOW(int, int, INT_MIN, INT_MAX)
DEFINE_SIGNED_OVERFLOW(int8, int8_t, INT8_MIN, INT8_MAX)
DEFINE_SIGNED_OVERFLOW(int16, int16_t, INT16_MIN, INT16_MAX)
DEFINE_SIGNED_OVERFLOW(int32, int32_t, INT32_MIN, INT32_MAX)
DEFINE_SIGNED_OVERFLOW(int64, int64_t, INT64_MIN, INT64_MAX)

#endif
