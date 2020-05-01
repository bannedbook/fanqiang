/*
 * Copyright (C) Ambroz Bizjak <ambrop7@gmail.com>
 * Contributions:
 * Transparent DNS: Copyright (C) Kerem Hadimli <kerem.hadimli@gmail.com>
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

#ifndef BADVPN_PROTOCOL_UDPGW_PROTO_H
#define BADVPN_PROTOCOL_UDPGW_PROTO_H

#include <stdint.h>

#include <misc/bsize.h>
#include <misc/packed.h>

#define UDPGW_CLIENT_FLAG_KEEPALIVE (1 << 0)
#define UDPGW_CLIENT_FLAG_REBIND (1 << 1)
#define UDPGW_CLIENT_FLAG_DNS (1 << 2)
#define UDPGW_CLIENT_FLAG_IPV6 (1 << 3)

#ifdef __ANDROID__
B_START_PACKED
struct socks_udp_header {
    uint16_t rsv;
    uint8_t frag;
    uint8_t atyp;
} B_PACKED;
B_END_PACKED
#endif

B_START_PACKED
struct udpgw_header {
    uint8_t flags;
    uint16_t conid;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct udpgw_addr_ipv4 {
    uint32_t addr_ip;
    uint16_t addr_port;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct udpgw_addr_ipv6 {
    uint8_t addr_ip[16];
    uint16_t addr_port;
} B_PACKED;
B_END_PACKED

static int udpgw_compute_mtu (int dgram_mtu)
{
    bsize_t bs = bsize_add(
#ifdef __ANDROID__
        bsize_fromsize(sizeof(struct socks_udp_header)),
#else
        bsize_fromsize(sizeof(struct udpgw_header)),
#endif
        bsize_add(
            bsize_max(
                bsize_fromsize(sizeof(struct udpgw_addr_ipv4)),
                bsize_fromsize(sizeof(struct udpgw_addr_ipv6))
            ), 
            bsize_fromint(dgram_mtu)
        )
    );
    
    int s;
    if (!bsize_toint(bs, &s)) {
        return -1;
    }
    
    return s;
}

#endif
