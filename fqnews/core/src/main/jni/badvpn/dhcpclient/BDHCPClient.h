/**
 * @file BDHCPClient.h
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
 * DHCP client.
 */

#ifndef BADVPN_DHCPCLIENT_BDHCPCLIENT_H
#define BADVPN_DHCPCLIENT_BDHCPCLIENT_H

#include <base/DebugObject.h>
#include <system/BDatagram.h>
#include <flow/PacketCopier.h>
#include <flow/SinglePacketBuffer.h>
#include <dhcpclient/BDHCPClientCore.h>
#include <dhcpclient/DHCPIpUdpDecoder.h>
#include <dhcpclient/DHCPIpUdpEncoder.h>

#define BDHCPCLIENT_EVENT_UP 1
#define BDHCPCLIENT_EVENT_DOWN 2
#define BDHCPCLIENT_EVENT_ERROR 3

#define BDHCPCLIENT_MAX_DOMAIN_NAME_SERVERS BDHCPCLIENTCORE_MAX_DOMAIN_NAME_SERVERS

typedef void (*BDHCPClient_handler) (void *user, int event);

typedef struct {
    BReactor *reactor;
    BDatagram dgram;
    BDHCPClient_handler handler;
    void *user;
    PacketCopier send_copier;
    DHCPIpUdpEncoder send_encoder;
    SinglePacketBuffer send_buffer;
    SinglePacketBuffer recv_buffer;
    DHCPIpUdpDecoder recv_decoder;
    PacketCopier recv_copier;
    BDHCPClientCore dhcp;
    int up;
    DebugError d_err;
    DebugObject d_obj;
} BDHCPClient;

struct BDHCPClient_opts {
    const char *hostname;
    const char *vendorclassid;
    const uint8_t *clientid;
    size_t clientid_len;
    int auto_clientid;
};

int BDHCPClient_Init (BDHCPClient *o, const char *ifname, struct BDHCPClient_opts opts, BReactor *reactor, BRandom2 *random2, BDHCPClient_handler handler, void *user);
void BDHCPClient_Free (BDHCPClient *o);
int BDHCPClient_IsUp (BDHCPClient *o);
void BDHCPClient_GetClientIP (BDHCPClient *o, uint32_t *out_ip);
void BDHCPClient_GetClientMask (BDHCPClient *o, uint32_t *out_mask);
int BDHCPClient_GetRouter (BDHCPClient *o, uint32_t *out_router);
int BDHCPClient_GetDNS (BDHCPClient *o, uint32_t *out_dns_servers, size_t max_dns_servers);
void BDHCPClient_GetServerMAC (BDHCPClient *o, uint8_t *out_mac);

#endif
