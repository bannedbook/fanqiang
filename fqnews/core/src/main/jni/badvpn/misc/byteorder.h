/**
 * @file byteorder.h
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
 * Byte order conversion functions.
 * 
 * hton* functions convert from host to big-endian (network) byte order.
 * htol* functions convert from host to little-endian byte order.
 * ntoh* functions convert from big-endian (network) to host byte order.
 * ltoh* functions convert from little-endian to host byte order.
 */

#ifndef BADVPN_MISC_BYTEORDER_H
#define BADVPN_MISC_BYTEORDER_H

#if (defined(BADVPN_LITTLE_ENDIAN) + defined(BADVPN_BIG_ENDIAN)) != 1
#error Unknown byte order or too many byte orders
#endif

#include <stdint.h>

static uint16_t badvpn_reverse16 (uint16_t x)
{
    uint16_t y;
    *((uint8_t *)&y+0) = *((uint8_t *)&x+1);
    *((uint8_t *)&y+1) = *((uint8_t *)&x+0);
    return y;
}

static uint32_t badvpn_reverse32 (uint32_t x)
{
    uint32_t y;
    *((uint8_t *)&y+0) = *((uint8_t *)&x+3);
    *((uint8_t *)&y+1) = *((uint8_t *)&x+2);
    *((uint8_t *)&y+2) = *((uint8_t *)&x+1);
    *((uint8_t *)&y+3) = *((uint8_t *)&x+0);
    return y;
}

static uint64_t badvpn_reverse64 (uint64_t x)
{
    uint64_t y;
    *((uint8_t *)&y+0) = *((uint8_t *)&x+7);
    *((uint8_t *)&y+1) = *((uint8_t *)&x+6);
    *((uint8_t *)&y+2) = *((uint8_t *)&x+5);
    *((uint8_t *)&y+3) = *((uint8_t *)&x+4);
    *((uint8_t *)&y+4) = *((uint8_t *)&x+3);
    *((uint8_t *)&y+5) = *((uint8_t *)&x+2);
    *((uint8_t *)&y+6) = *((uint8_t *)&x+1);
    *((uint8_t *)&y+7) = *((uint8_t *)&x+0);
    return y;
}

static uint8_t hton8 (uint8_t x)
{
    return x;
}

static uint8_t htol8 (uint8_t x)
{
    return x;
}

#if defined(BADVPN_LITTLE_ENDIAN)

static uint16_t hton16 (uint16_t x)
{
    return badvpn_reverse16(x);
}

static uint32_t hton32 (uint32_t x)
{
    return badvpn_reverse32(x);
}

static uint64_t hton64 (uint64_t x)
{
    return badvpn_reverse64(x);
}

static uint16_t htol16 (uint16_t x)
{
    return x;
}

static uint32_t htol32 (uint32_t x)
{
    return x;
}

static uint64_t htol64 (uint64_t x)
{
    return x;
}

#elif defined(BADVPN_BIG_ENDIAN)

static uint16_t hton16 (uint16_t x)
{
    return x;
}

static uint32_t hton32 (uint32_t x)
{
    return x;
}

static uint64_t hton64 (uint64_t x)
{
    return x;
}

static uint16_t htol16 (uint16_t x)
{
    return badvpn_reverse16(x);
}

static uint32_t htol32 (uint32_t x)
{
    return badvpn_reverse32(x);
}

static uint64_t htol64 (uint64_t x)
{
    return badvpn_reverse64(x);
}

#endif

static uint8_t ntoh8 (uint8_t x)
{
    return hton8(x);
}

static uint16_t ntoh16 (uint16_t x)
{
    return hton16(x);
}

static uint32_t ntoh32 (uint32_t x)
{
    return hton32(x);
}

static uint64_t ntoh64 (uint64_t x)
{
    return hton64(x);
}

static uint8_t ltoh8 (uint8_t x)
{
    return htol8(x);
}

static uint16_t ltoh16 (uint16_t x)
{
    return htol16(x);
}

static uint32_t ltoh32 (uint32_t x)
{
    return htol32(x);
}

static uint64_t ltoh64 (uint64_t x)
{
    return htol64(x);
}

#endif
