/**
 * @file ipv4_proto.h
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
 * Definitions for the IPv4 protocol.
 */

#ifndef BADVPN_MISC_IPV4_PROTO_H
#define BADVPN_MISC_IPV4_PROTO_H

#include <stdint.h>
#include <string.h>

#include <misc/debug.h>
#include <misc/byteorder.h>
#include <misc/packed.h>
#include <misc/read_write_int.h>

#define IPV4_PROTOCOL_IGMP 2
#define IPV4_PROTOCOL_UDP 17

B_START_PACKED
struct ipv4_header {
    uint8_t version4_ihl4;
    uint8_t ds;
    uint16_t total_length;
    //
    uint16_t identification;
    uint16_t flags3_fragmentoffset13;
    //
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    //
    uint32_t source_address;
    //
    uint32_t destination_address;
} B_PACKED;
B_END_PACKED

#define IPV4_GET_VERSION(_header) (((_header).version4_ihl4&0xF0)>>4)
#define IPV4_GET_IHL(_header) (((_header).version4_ihl4&0x0F)>>0)

#define IPV4_MAKE_VERSION_IHL(size) (((size)/4) + (4 << 4))

static uint16_t ipv4_checksum (const struct ipv4_header *header, const char *extra, uint16_t extra_len)
{
    ASSERT(extra_len % 2 == 0)
    ASSERT(extra_len == 0 || extra)
    
    uint32_t t = 0;
    
    for (uint16_t i = 0; i < sizeof(*header) / 2; i++) {
        t += badvpn_read_be16((const char *)header + 2 * i);
    }
    
    for (uint16_t i = 0; i < extra_len / 2; i++) {
        t += badvpn_read_be16((const char *)extra + 2 * i);
    }
    
    while (t >> 16) {
        t = (t & 0xFFFF) + (t >> 16);
    }
    
    return hton16(~t);
}

static int ipv4_check (uint8_t *data, int data_len, struct ipv4_header *out_header, uint8_t **out_payload, int *out_payload_len)
{
    ASSERT(data_len >= 0)
    ASSERT(out_header)
    ASSERT(out_payload)
    ASSERT(out_payload_len)
    
    // check base header
    if (data_len < sizeof(struct ipv4_header)) {
        return 0;
    }
    memcpy(out_header, data, sizeof(*out_header));
    
    // check version
    if (IPV4_GET_VERSION(*out_header) != 4) {
        return 0;
    }
    
    // check options
    uint16_t header_len = IPV4_GET_IHL(*out_header) * 4;
    if (header_len < sizeof(struct ipv4_header)) {
        return 0;
    }
    if (header_len > data_len) {
        return 0;
    }
    
    // check total length
    uint16_t total_length = ntoh16(out_header->total_length);
    if (total_length < header_len) {
        return 0;
    }
    if (total_length > data_len) {
        return 0;
    }
    
    // check checksum
    uint16_t checksum_in_packet = out_header->checksum;
    out_header->checksum = hton16(0);
    uint16_t checksum_computed = ipv4_checksum(out_header, (char *)data + sizeof(*out_header), header_len - sizeof(*out_header));
    out_header->checksum = checksum_in_packet;
    if (checksum_in_packet != checksum_computed) {
        return 0;
    }
    
    *out_payload = data + header_len;
    *out_payload_len = total_length - header_len;
    
    return 1;
}

#endif
