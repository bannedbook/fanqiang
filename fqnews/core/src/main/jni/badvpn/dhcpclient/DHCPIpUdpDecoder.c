/**
 * @file DHCPIpUdpDecoder.c
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

#include <limits.h>
#include <string.h>

#include <misc/ipv4_proto.h>
#include <misc/udp_proto.h>
#include <misc/byteorder.h>

#include <dhcpclient/DHCPIpUdpDecoder.h>

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

#define IPUDP_HEADER_SIZE (sizeof(struct ipv4_header) + sizeof(struct udp_header))

static void input_handler_send (DHCPIpUdpDecoder *o, uint8_t *data, int data_len)
{
    ASSERT(data_len >= 0)
    DebugObject_Access(&o->d_obj);
    
    struct ipv4_header iph;
    uint8_t *pl;
    int pl_len;
    
    if (!ipv4_check(data, data_len, &iph, &pl, &pl_len)) {
        goto fail;
    }
    
    if (ntoh8(iph.protocol) != IPV4_PROTOCOL_UDP) {
        goto fail;
    }
    
    if (pl_len < sizeof(struct udp_header)) {
        goto fail;
    }
    struct udp_header udph;
    memcpy(&udph, pl, sizeof(udph));
    
    if (ntoh16(udph.source_port) != DHCP_SERVER_PORT) {
        goto fail;
    }
    
    if (ntoh16(udph.dest_port) != DHCP_CLIENT_PORT) {
        goto fail;
    }
    
    int udph_length = ntoh16(udph.length);
    if (udph_length < sizeof(udph)) {
        goto fail;
    }
    if (udph_length > data_len - (pl - data)) {
        goto fail;
    }
    
    if (ntoh16(udph.checksum) != 0) {
        uint16_t checksum_in_packet = udph.checksum;
        udph.checksum = 0;
        uint16_t checksum_computed = udp_checksum(&udph, pl + sizeof(udph), udph_length - sizeof(udph), iph.source_address, iph.destination_address);
        if (checksum_in_packet != checksum_computed) {
            goto fail;
        }
    }
    
    // pass payload to output
    PacketPassInterface_Sender_Send(o->output, pl + sizeof(udph), udph_length - sizeof(udph));
    
    return;
    
fail:
    PacketPassInterface_Done(&o->input);
}

static void output_handler_done (DHCPIpUdpDecoder *o)
{
    DebugObject_Access(&o->d_obj);
    
    PacketPassInterface_Done(&o->input);
}

void DHCPIpUdpDecoder_Init (DHCPIpUdpDecoder *o, PacketPassInterface *output, BPendingGroup *pg)
{
    ASSERT(PacketPassInterface_GetMTU(output) <= INT_MAX - IPUDP_HEADER_SIZE)
    
    // init arguments
    o->output = output;
    
    // init output
    PacketPassInterface_Sender_Init(o->output, (PacketPassInterface_handler_done)output_handler_done, o);
    
    // init input
    PacketPassInterface_Init(&o->input, IPUDP_HEADER_SIZE + PacketPassInterface_GetMTU(o->output), (PacketPassInterface_handler_send)input_handler_send, o, pg);
    
    DebugObject_Init(&o->d_obj);
}

void DHCPIpUdpDecoder_Free (DHCPIpUdpDecoder *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free input
    PacketPassInterface_Free(&o->input);
}

PacketPassInterface * DHCPIpUdpDecoder_GetInput (DHCPIpUdpDecoder *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->input;
}
