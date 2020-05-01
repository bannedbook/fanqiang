/**
 * @file read_write_int.h
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
 */

#ifndef BADVPN_READ_WRITE_INT_H
#define BADVPN_READ_WRITE_INT_H

#include <stdint.h>

static uint8_t badvpn_read_le8 (const char *c_ptr);
static uint16_t badvpn_read_le16 (const char *c_ptr);
static uint32_t badvpn_read_le32 (const char *c_ptr);
static uint64_t badvpn_read_le64 (const char *c_ptr);

static uint8_t badvpn_read_be8 (const char *c_ptr);
static uint16_t badvpn_read_be16 (const char *c_ptr);
static uint32_t badvpn_read_be32 (const char *c_ptr);
static uint64_t badvpn_read_be64 (const char *c_ptr);

static void badvpn_write_le8 (uint8_t x, char *c_ptr);
static void badvpn_write_le16 (uint16_t x, char *c_ptr);
static void badvpn_write_le32 (uint32_t x, char *c_ptr);
static void badvpn_write_le64 (uint64_t x, char *c_ptr);

static void badvpn_write_be8 (uint8_t x, char *c_ptr);
static void badvpn_write_be16 (uint16_t x, char *c_ptr);
static void badvpn_write_be32 (uint32_t x, char *c_ptr);
static void badvpn_write_be64 (uint64_t x, char *c_ptr);

static uint8_t badvpn_read_le8 (const char *c_ptr)
{
    const uint8_t *ptr = (const uint8_t *)c_ptr;
    return ((uint8_t)ptr[0] << 0);
}

static uint16_t badvpn_read_le16 (const char *c_ptr)
{
    const uint8_t *ptr = (const uint8_t *)c_ptr;
    return ((uint16_t)ptr[1] << 8) | ((uint16_t)ptr[0] << 0);
}

static uint32_t badvpn_read_le32 (const char *c_ptr)
{
    const uint8_t *ptr = (const uint8_t *)c_ptr;
    return ((uint32_t)ptr[3] << 24) | ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[1] <<  8) | ((uint32_t)ptr[0] <<  0);
}

static uint64_t badvpn_read_le64 (const char *c_ptr)
{
    const uint8_t *ptr = (const uint8_t *)c_ptr;
    return ((uint64_t)ptr[7] << 56) | ((uint64_t)ptr[6] << 48) |
           ((uint64_t)ptr[5] << 40) | ((uint64_t)ptr[4] << 32) |
           ((uint64_t)ptr[3] << 24) | ((uint64_t)ptr[2] << 16) |
           ((uint64_t)ptr[1] <<  8) | ((uint64_t)ptr[0] <<  0);
}

static uint8_t badvpn_read_be8 (const char *c_ptr)
{
    const uint8_t *ptr = (const uint8_t *)c_ptr;
    return ((uint8_t)ptr[0] << 0);
}

static uint16_t badvpn_read_be16 (const char *c_ptr)
{
    const uint8_t *ptr = (const uint8_t *)c_ptr;
    return ((uint16_t)ptr[0] << 8) | ((uint16_t)ptr[1] << 0);
}

static uint32_t badvpn_read_be32 (const char *c_ptr)
{
    const uint8_t *ptr = (const uint8_t *)c_ptr;
    return ((uint32_t)ptr[0] << 24) | ((uint32_t)ptr[1] << 16) |
           ((uint32_t)ptr[2] <<  8) | ((uint32_t)ptr[3] <<  0);
}

static uint64_t badvpn_read_be64 (const char *c_ptr)
{
    const uint8_t *ptr = (const uint8_t *)c_ptr;
    return ((uint64_t)ptr[0] << 56) | ((uint64_t)ptr[1] << 48) |
           ((uint64_t)ptr[2] << 40) | ((uint64_t)ptr[3] << 32) |
           ((uint64_t)ptr[4] << 24) | ((uint64_t)ptr[5] << 16) |
           ((uint64_t)ptr[6] <<  8) | ((uint64_t)ptr[7] <<  0);
}

static void badvpn_write_le8 (uint8_t x, char *c_ptr)
{
    uint8_t *ptr = (uint8_t *)c_ptr;
    ptr[0] = x >> 0;
}

static void badvpn_write_le16 (uint16_t x, char *c_ptr)
{
    uint8_t *ptr = (uint8_t *)c_ptr;
    ptr[1] = x >> 8;
    ptr[0] = x >> 0;
}

static void badvpn_write_le32 (uint32_t x, char *c_ptr)
{
    uint8_t *ptr = (uint8_t *)c_ptr;
    ptr[3] = x >> 24;
    ptr[2] = x >> 16;
    ptr[1] = x >> 8;
    ptr[0] = x >> 0;
}

static void badvpn_write_le64 (uint64_t x, char *c_ptr)
{
    uint8_t *ptr = (uint8_t *)c_ptr;
    ptr[7] = x >> 56;
    ptr[6] = x >> 48;
    ptr[5] = x >> 40;
    ptr[4] = x >> 32;
    ptr[3] = x >> 24;
    ptr[2] = x >> 16;
    ptr[1] = x >> 8;
    ptr[0] = x >> 0;
}

static void badvpn_write_be8 (uint8_t x, char *c_ptr)
{
    uint8_t *ptr = (uint8_t *)c_ptr;
    ptr[0] = x >> 0;
}

static void badvpn_write_be16 (uint16_t x, char *c_ptr)
{
    uint8_t *ptr = (uint8_t *)c_ptr;
    ptr[0] = x >> 8;
    ptr[1] = x >> 0;
}

static void badvpn_write_be32 (uint32_t x, char *c_ptr)
{
    uint8_t *ptr = (uint8_t *)c_ptr;
    ptr[0] = x >> 24;
    ptr[1] = x >> 16;
    ptr[2] = x >> 8;
    ptr[3] = x >> 0;
}

static void badvpn_write_be64 (uint64_t x, char *c_ptr)
{
    uint8_t *ptr = (uint8_t *)c_ptr;
    ptr[0] = x >> 56;
    ptr[1] = x >> 48;
    ptr[2] = x >> 40;
    ptr[3] = x >> 32;
    ptr[4] = x >> 24;
    ptr[5] = x >> 16;
    ptr[6] = x >> 8;
    ptr[7] = x >> 0;
}

#endif
