/**
 * @file ipaddr.h
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
 * IP address parsing functions.
 */

#ifndef BADVPN_MISC_IPADDR_H
#define BADVPN_MISC_IPADDR_H

#include <string.h>
#include <stdlib.h>

#include <misc/debug.h>
#include <misc/byteorder.h>
#include <misc/parse_number.h>
#include <misc/find_char.h>
#include <misc/print_macros.h>
#include <misc/memref.h>

struct ipv4_ifaddr {
    uint32_t addr;
    int prefix;
};

static int ipaddr_parse_ipv4_addr (MemRef name, uint32_t *out_addr);
static int ipaddr_parse_ipv4_prefix (MemRef str, int *num);
static int ipaddr_parse_ipv4_ifaddr (MemRef str, struct ipv4_ifaddr *out);
static int ipaddr_ipv4_ifaddr_from_addr_mask (uint32_t addr, uint32_t mask, struct ipv4_ifaddr *out);
static uint32_t ipaddr_ipv4_mask_from_prefix (int prefix);
static int ipaddr_ipv4_prefix_from_mask (uint32_t mask, int *out_prefix);
static int ipaddr_ipv4_addrs_in_network (uint32_t addr1, uint32_t addr2, int netprefix);

#define IPADDR_PRINT_MAX 19

static void ipaddr_print_addr (uint32_t addr, char *out);
static void ipaddr_print_ifaddr (struct ipv4_ifaddr ifaddr, char *out);

int ipaddr_parse_ipv4_addr (MemRef name, uint32_t *out_addr)
{
    for (size_t i = 0; ; i++) {
        size_t j;
        for (j = 0; j < name.len && name.ptr[j] != '.'; j++);
        
        if ((j == name.len && i < 3) || (j < name.len && i == 3)) {
            return 0;
        }
        
        if (j < 1 || j > 3) {
            return 0;
        }
        
        uintmax_t d;
        if (!parse_unsigned_integer(MemRef_SubTo(name, j), &d)) {
            return 0;
        }
        
        if (d > 255) {
            return 0;
        }
        
        ((uint8_t *)out_addr)[i] = d;
        
        if (i == 3) {
            return 1;
        }
        
        name.ptr += j + 1;
        name.len -= j + 1;
    }
}

int ipaddr_parse_ipv4_prefix (MemRef str, int *num)
{
    uintmax_t d;
    if (!parse_unsigned_integer(str, &d)) {
        return 0;
    }
    if (d > 32) {
        return 0;
    }
    
    *num = d;
    return 1;
}

int ipaddr_parse_ipv4_ifaddr (MemRef str, struct ipv4_ifaddr *out)
{
    size_t slash_pos;
    if (!MemRef_FindChar(str, '/', &slash_pos)) {
        return 0;
    }
    
    return (ipaddr_parse_ipv4_addr(MemRef_SubTo(str, slash_pos), &out->addr) &&
            ipaddr_parse_ipv4_prefix(MemRef_SubFrom(str, slash_pos + 1), &out->prefix));
}

int ipaddr_ipv4_ifaddr_from_addr_mask (uint32_t addr, uint32_t mask, struct ipv4_ifaddr *out)
{
    int prefix;
    if (!ipaddr_ipv4_prefix_from_mask(mask, &prefix)) {
        return 0;
    }
    
    out->addr = addr;
    out->prefix = prefix;
    return 1;
}

uint32_t ipaddr_ipv4_mask_from_prefix (int prefix)
{
    ASSERT(prefix >= 0)
    ASSERT(prefix <= 32)
    
    uint32_t t = 0;
    for (int i = 0; i < prefix; i++) {
        t |= 1 << (32 - i - 1);
    }
    
    return hton32(t);
}

int ipaddr_ipv4_prefix_from_mask (uint32_t mask, int *out_prefix)
{
    uint32_t t = 0;
    int i;
    for (i = 0; i <= 32; i++) {
        if (ntoh32(mask) == t) {
            break;
        }
        if (i < 32) {
            t |= (1 << (32 - i - 1));
        }
    }
    if (!(i <= 32)) {
        return 0;
    }
    
    *out_prefix = i;
    return 1;
}

int ipaddr_ipv4_addrs_in_network (uint32_t addr1, uint32_t addr2, int netprefix)
{
    ASSERT(netprefix >= 0)
    ASSERT(netprefix <= 32)
    
    uint32_t mask = ipaddr_ipv4_mask_from_prefix(netprefix);
    
    return !!((addr1 & mask) == (addr2 & mask));
}

void ipaddr_print_addr (uint32_t addr, char *out)
{
    ASSERT(out)
    
    uint8_t *b = (uint8_t *)&addr;
    
    sprintf(out, "%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8,
            b[0], b[1], b[2], b[3]);
}

void ipaddr_print_ifaddr (struct ipv4_ifaddr ifaddr, char *out)
{
    ASSERT(ifaddr.prefix >= 0)
    ASSERT(ifaddr.prefix <= 32)
    ASSERT(out)
    
    uint8_t *b = (uint8_t *)&ifaddr.addr;
    
    sprintf(out, "%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"/%d",
            b[0], b[1], b[2], b[3], ifaddr.prefix);
}

#endif
