/**
 * @file udp_proto.h
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
 * Definitions for the UDP protocol.
 */

#ifndef BADVPN_MISC_UDP_PROTO_H
#define BADVPN_MISC_UDP_PROTO_H

#include <stdint.h>

#include <misc/debug.h>
#include <misc/byteorder.h>
#include <misc/ipv4_proto.h>
#include <misc/ipv6_proto.h>
#include <misc/read_write_int.h>

B_START_PACKED
struct udp_header {
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} B_PACKED;
B_END_PACKED

static uint32_t udp_checksum_summer (const char *data, uint16_t len)
{
    ASSERT(len % 2 == 0)
    
    uint32_t t = 0;
    
    for (uint16_t i = 0; i < len / 2; i++) {
        t += badvpn_read_be16(data + 2 * i);
    }
    
    return t;
}

static uint16_t udp_checksum (const struct udp_header *header, const uint8_t *payload, uint16_t payload_len, uint32_t source_addr, uint32_t dest_addr)
{
    uint32_t t = 0;
    
    t += udp_checksum_summer((char *)&source_addr, sizeof(source_addr));
    t += udp_checksum_summer((char *)&dest_addr, sizeof(dest_addr));
    
    uint16_t x;
    x = hton16(IPV4_PROTOCOL_UDP);
    t += udp_checksum_summer((char *)&x, sizeof(x));
    x = hton16(sizeof(*header) + payload_len);
    t += udp_checksum_summer((char *)&x, sizeof(x));
    
    t += udp_checksum_summer((const char *)header, sizeof(*header));
    
    if (payload_len % 2 == 0) {
        t += udp_checksum_summer((const char *)payload, payload_len);
    } else {
        t += udp_checksum_summer((const char *)payload, payload_len - 1);
        
        x = hton16(((uint16_t)payload[payload_len - 1]) << 8);
        t += udp_checksum_summer((char *)&x, sizeof(x));
    }
    
    while (t >> 16) {
        t = (t & 0xFFFF) + (t >> 16);
    }
    
    t = ~t;
    
    if (t == 0) {
        t = UINT16_MAX;
    }
    
    return hton16(t);
}

static uint16_t udp_ip6_checksum (const struct udp_header *header, const uint8_t *payload, uint16_t payload_len, const uint8_t *source_addr, const uint8_t *dest_addr)
{
    uint32_t t = 0;
    
    t += udp_checksum_summer((const char *)source_addr, 16);
    t += udp_checksum_summer((const char *)dest_addr, 16);
    
    uint32_t x;
    x = hton32(sizeof(*header) + payload_len);
    t += udp_checksum_summer((char *)&x, sizeof(x));
    x = hton32(IPV6_NEXT_UDP);
    t += udp_checksum_summer((char *)&x, sizeof(x));
    
    t += udp_checksum_summer((const char *)header, sizeof(*header));
    
    if (payload_len % 2 == 0) {
        t += udp_checksum_summer((const char *)payload, payload_len);
    } else {
        t += udp_checksum_summer((const char *)payload, payload_len - 1);
        
        uint16_t y;
        y = hton16(((uint16_t)payload[payload_len - 1]) << 8);
        t += udp_checksum_summer((char *)&y, sizeof(y));
    }
    
    while (t >> 16) {
        t = (t & 0xFFFF) + (t >> 16);
    }
    
    t = ~t;
    
    if (t == 0) {
        t = UINT16_MAX;
    }
    
    return hton16(t);
}

static int udp_check (const uint8_t *data, int data_len, struct udp_header *out_header, uint8_t **out_payload, int *out_payload_len)
{
    ASSERT(data_len >= 0)
    ASSERT(out_header)
    ASSERT(out_payload)
    ASSERT(out_payload_len)
    
    // parse UDP header
    if (data_len < sizeof(struct udp_header)) {
        return 0;
    }
    memcpy(out_header, data, sizeof(*out_header));
    data += sizeof(*out_header);
    data_len -= sizeof(*out_header);
    
    // verify UDP payload
    int udp_length = ntoh16(out_header->length);
    if (udp_length < sizeof(*out_header)) {
        return 0;
    }
    if (udp_length > sizeof(*out_header) + data_len) {
        return 0;
    }
    
    // ignore stray data
    data_len = udp_length - sizeof(*out_header);
    
    *out_payload = (uint8_t *)data;
    *out_payload_len = data_len;
    return 1;
}

#endif
