/**
 * @file ipaddr6.h
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
 * IPv6 address parsing functions.
 */

#ifndef BADVPN_MISC_IPADDR6_H
#define BADVPN_MISC_IPADDR6_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <misc/debug.h>
#include <misc/byteorder.h>
#include <misc/parse_number.h>
#include <misc/find_char.h>
#include <misc/print_macros.h>
#include <misc/memref.h>

struct ipv6_addr {
    uint8_t bytes[16];
};

struct ipv6_ifaddr {
    struct ipv6_addr addr;
    int prefix;
};

static int ipaddr6_parse_ipv6_addr (MemRef name, struct ipv6_addr *out_addr);
static int ipaddr6_parse_ipv6_prefix (MemRef str, int *out_num);
static int ipaddr6_parse_ipv6_ifaddr (MemRef str, struct ipv6_ifaddr *out);
static int ipaddr6_ipv6_ifaddr_from_addr_mask (struct ipv6_addr addr, struct ipv6_addr mask, struct ipv6_ifaddr *out);
static void ipaddr6_ipv6_mask_from_prefix (int prefix, struct ipv6_addr *out_mask);
static int ipaddr6_ipv6_prefix_from_mask (struct ipv6_addr mask, int *out_prefix);
static int ipaddr6_ipv6_addrs_in_network (struct ipv6_addr addr1, struct ipv6_addr addr2, int netprefix);

#define IPADDR6_PRINT_MAX 44

static void ipaddr6_print_addr (struct ipv6_addr addr, char *out_buf);
static void ipaddr6_print_ifaddr (struct ipv6_ifaddr addr, char *out_buf);

int ipaddr6_parse_ipv6_addr (MemRef name, struct ipv6_addr *out_addr)
{
    int num_blocks = 0;
    int compress_pos = -1;
    uint16_t block = 0;
    int empty = 1;
    
    size_t i = 0;
    
    while (i < name.len) {
        if (name.ptr[i] == '.') {
            goto ipv4_ending;
        } else if (name.ptr[i] == ':') {
            int is_double = (i + 1 < name.len && name.ptr[i + 1] == ':');
            
            if (i > 0) {
                if (empty || num_blocks == 7) {
                    return 0;
                }
                out_addr->bytes[2 * num_blocks + 0] = block >> 8;
                out_addr->bytes[2 * num_blocks + 1] = block & 0xFF;
                num_blocks++;
                block = 0;
                empty = 1;
            }
            else if (!is_double) {
                return 0;
            }
            
            if (is_double) {
                if (compress_pos != -1) {
                    return 0;
                }
                compress_pos = num_blocks;
            }
            
            i += 1 + is_double;
        } else {
            int digit = decode_hex_digit(name.ptr[i]);
            if (digit < 0) {
                return 0;
            }
            if (block > UINT16_MAX / 16) {
                return 0;
            }
            block *= 16;
            if (digit > UINT16_MAX - block) {
                return 0;
            }
            block += digit;
            empty = 0;
            i += 1;
        }
    }
    
    if (!empty) {
        out_addr->bytes[2 * num_blocks + 0] = block >> 8;
        out_addr->bytes[2 * num_blocks + 1] = block & 0xFF;
        num_blocks++;
    }
    else if (num_blocks != compress_pos) {
        return 0;
    }
    
ipv4_done:
    if (compress_pos == -1) {
        if (num_blocks != 8) {
            return 0;
        }
        compress_pos = 0;
    }
    
    int num_rear = num_blocks - compress_pos;
    memmove(out_addr->bytes + 2 * (8 - num_rear), out_addr->bytes + 2 * compress_pos, 2 * num_rear);
    memset(out_addr->bytes + 2 * compress_pos, 0, 2 * (8 - num_rear - compress_pos));
    
    return 1;
    
ipv4_ending:
    if (empty || (num_blocks == 0 && compress_pos == -1)) {
        return 0;
    }
    
    while (name.ptr[i - 1] != ':') {
        i--;
    }
    
    uint8_t bytes[4];
    int cur_byte = 0;
    uint8_t byte = 0;
    empty = 1;
    
    while (i < name.len) {
        if (name.ptr[i] == '.') {
            if (empty || cur_byte == 3) {
                return 0;
            }
            bytes[cur_byte] = byte;
            cur_byte++;
            byte = 0;
            empty = 1;
        } else {
            if (!empty && byte == 0) {
                return 0;
            }
            int digit = decode_decimal_digit(name.ptr[i]);
            if (digit < 0) {
                return 0;
            }
            if (byte > UINT8_MAX / 10) {
                return 0;
            }
            byte *= 10;
            if (digit > UINT8_MAX - byte) {
                return 0;
            }
            byte += digit;
            empty = 0;
        }
        i++;
    }
    
    if (cur_byte != 3 || empty) {
        return 0;
    }
    bytes[cur_byte] = byte;
    
    if (8 - num_blocks < 2) {
        return 0;
    }
    memcpy(out_addr->bytes + 2 * num_blocks, bytes, 4);
    num_blocks += 2;
    
    goto ipv4_done;
}

int ipaddr6_parse_ipv6_prefix (MemRef str, int *out_num)
{
    uintmax_t d;
    if (!parse_unsigned_integer(str, &d)) {
        return 0;
    }
    if (d > 128) {
        return 0;
    }
    
    *out_num = d;
    return 1;
}

int ipaddr6_parse_ipv6_ifaddr (MemRef str, struct ipv6_ifaddr *out)
{
    size_t slash_pos;
    if (!MemRef_FindChar(str, '/', &slash_pos)) {
        return 0;
    }
    
    return (ipaddr6_parse_ipv6_addr(MemRef_SubTo(str, slash_pos), &out->addr) &&
            ipaddr6_parse_ipv6_prefix(MemRef_SubFrom(str, slash_pos + 1), &out->prefix));
}

int ipaddr6_ipv6_ifaddr_from_addr_mask (struct ipv6_addr addr, struct ipv6_addr mask, struct ipv6_ifaddr *out)
{
    int prefix;
    if (!ipaddr6_ipv6_prefix_from_mask(mask, &prefix)) {
        return 0;
    }
    
    out->addr = addr;
    out->prefix = prefix;
    return 1;
}

void ipaddr6_ipv6_mask_from_prefix (int prefix, struct ipv6_addr *out_mask)
{
    ASSERT(prefix >= 0)
    ASSERT(prefix <= 128)
    
    int quot = prefix / 8;
    int rem = prefix % 8;
    
    if (quot > 0) {
        memset(out_mask->bytes, UINT8_MAX, quot);
    }
    if (16 - quot > 0) {
        memset(out_mask->bytes + quot, 0, 16 - quot);
    }
    
    for (int i = 0; i < rem; i++) {
        out_mask->bytes[quot] |= (uint8_t)1 << (8 - i - 1);
    }
}

int ipaddr6_ipv6_prefix_from_mask (struct ipv6_addr mask, int *out_prefix)
{
    int prefix = 0;
    int i = 0;
    
    while (i < 16 && mask.bytes[i] == UINT8_MAX) {
        prefix += 8;
        i++;
    }
    
    if (i < 16) {
        uint8_t t = 0;
        int j;
        for (j = 0; j <= 8; j++) {
            if (mask.bytes[i] == t) {
                break;
            }
            if (j < 8) {
                t |= ((uint8_t)1 << (8 - j - 1));
            }
        }
        if (!(j <= 8)) {
            return 0;
        }
        
        prefix += j;
        i++;
        
        while (i < 16) {
            if (mask.bytes[i] != 0) {
                return 0;
            }
            i++;
        }
    }
    
    *out_prefix = prefix;
    return 1;
}

int ipaddr6_ipv6_addrs_in_network (struct ipv6_addr addr1, struct ipv6_addr addr2, int netprefix)
{
    ASSERT(netprefix >= 0)
    ASSERT(netprefix <= 128)
    
    int quot = netprefix / 8;
    int rem = netprefix % 8;
    
    if (memcmp(addr1.bytes, addr2.bytes, quot)) {
        return 0;
    }
    
    if (rem == 0) {
        return 1;
    }
    
    uint8_t t = 0;
    for (int i = 0; i < rem; i++) {
        t |= (uint8_t)1 << (8 - i - 1);
    }
    
    return ((addr1.bytes[quot] & t) == (addr2.bytes[quot] & t));
}

void ipaddr6_print_addr (struct ipv6_addr addr, char *out_buf)
{
    int largest_start = 0;
    int largest_len = 0;
    int current_start = 0;
    int current_len = 0;
    
    for (int i = 0; i < 8; i++) {
        if (addr.bytes[2 * i] == 0 && addr.bytes[2 * i + 1] == 0) {
            current_len++;
            if (current_len > largest_len) {
                largest_start = current_start;
                largest_len = current_len;
            }
        } else {
            current_start = i + 1;
            current_len = 0;
        }
    }
    
    if (largest_len > 1) {
        for (int i = 0; i < largest_start; i++) {
            uint16_t block = ((uint16_t)addr.bytes[2 * i] << 8) | addr.bytes[2 * i + 1];
            out_buf += sprintf(out_buf, "%"PRIx16":", block);
        }
        if (largest_start == 0) {
            out_buf += sprintf(out_buf, ":");
        }
        
        for (int i = largest_start + largest_len; i < 8; i++) {
            uint16_t block = ((uint16_t)addr.bytes[2 * i] << 8) | addr.bytes[2 * i + 1];
            out_buf += sprintf(out_buf, ":%"PRIx16, block);
        }
        if (largest_start + largest_len == 8) {
            out_buf += sprintf(out_buf, ":");
        }
    } else {
        const char *prefix = "";
        for (int i = 0; i < 8; i++) {
            uint16_t block = ((uint16_t)addr.bytes[2 * i] << 8) | addr.bytes[2 * i + 1];
            out_buf += sprintf(out_buf, "%s%"PRIx16, prefix, block);
            prefix = ":";
        }
    }
}

void ipaddr6_print_ifaddr (struct ipv6_ifaddr addr, char *out_buf)
{
    ASSERT(addr.prefix >= 0)
    ASSERT(addr.prefix <= 128)
    
    ipaddr6_print_addr(addr.addr, out_buf);
    sprintf(out_buf + strlen(out_buf), "/%d", addr.prefix);
}

#endif
