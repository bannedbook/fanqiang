/**
 * @file ipv6_proto.h
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

#ifndef BADVPN_IPV6_PROTO_H
#define BADVPN_IPV6_PROTO_H

#include <stdint.h>
#include <string.h>

#include <misc/debug.h>
#include <misc/byteorder.h>
#include <misc/packed.h>

#define IPV6_NEXT_IGMP 2
#define IPV6_NEXT_UDP 17

B_START_PACKED
struct ipv6_header {
    uint8_t version4_tc4;
    uint8_t tc4_fl4;
    uint16_t fl;
    uint16_t payload_length;
    uint8_t next_header;
    uint8_t hop_limit;
    uint8_t source_address[16];
    uint8_t destination_address[16];
} B_PACKED;
B_END_PACKED

static int ipv6_check (uint8_t *data, int data_len, struct ipv6_header *out_header, uint8_t **out_payload, int *out_payload_len)
{
    ASSERT(data_len >= 0)
    ASSERT(out_header)
    ASSERT(out_payload)
    ASSERT(out_payload_len)
    
    // check base header
    if (data_len < sizeof(struct ipv6_header)) {
        return 0;
    }
    memcpy(out_header, data, sizeof(*out_header));
    
    // check version
    if ((ntoh8(out_header->version4_tc4) >> 4) != 6) {
        return 0;
    }
    
    // check payload length
    uint16_t payload_length = ntoh16(out_header->payload_length);
    if (payload_length > data_len - sizeof(struct ipv6_header)) {
        return 0;
    }
    
    *out_payload = data + sizeof(struct ipv6_header);
    *out_payload_len = payload_length;
    
    return 1;
}

#endif
