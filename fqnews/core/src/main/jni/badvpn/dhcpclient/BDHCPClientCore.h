/**
 * @file BDHCPClientCore.h
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
 * DHCP client, excluding system-dependent details.
 */

#ifndef BADVPN_DHCPCLIENT_BDHCPCLIENTCORE_H
#define BADVPN_DHCPCLIENT_BDHCPCLIENTCORE_H

#include <stdint.h>
#include <stddef.h>

#include <system/BReactor.h>
#include <base/DebugObject.h>
#include <random/BRandom2.h>
#include <flow/PacketPassInterface.h>
#include <flow/PacketRecvInterface.h>

#define BDHCPCLIENTCORE_EVENT_UP 1
#define BDHCPCLIENTCORE_EVENT_DOWN 2

#define BDHCPCLIENTCORE_MAX_DOMAIN_NAME_SERVERS 16

typedef void (*BDHCPClientCore_func_getsendermac) (void *user, uint8_t *out_mac);
typedef void (*BDHCPClientCore_handler) (void *user, int event);

struct BDHCPClientCore_opts {
    const char *hostname;
    const char *vendorclassid;
    const uint8_t *clientid;
    size_t clientid_len;
};

typedef struct {
    PacketPassInterface *send_if;
    PacketRecvInterface *recv_if;
    uint8_t client_mac_addr[6];
    BReactor *reactor;
    BRandom2 *random2;
    void *user;
    BDHCPClientCore_func_getsendermac func_getsendermac;
    BDHCPClientCore_handler handler;
    char *hostname;
    char *vendorclassid;
    uint8_t *clientid;
    size_t clientid_len;
    char *send_buf;
    char *recv_buf;
    int sending;
    BTimer reset_timer;
    BTimer request_timer;
    BTimer renew_timer;
    BTimer renew_request_timer;
    BTimer lease_timer;
    int state;
    int request_count;
    uint32_t xid;
    int xid_reuse_counter;
    struct {
        uint32_t yiaddr;
        uint32_t dhcp_server_identifier;
    } offered;
    struct {
        uint32_t ip_address_lease_time;
        uint32_t subnet_mask;
        int have_router;
        uint32_t router;
        int domain_name_servers_count;
        uint32_t domain_name_servers[BDHCPCLIENTCORE_MAX_DOMAIN_NAME_SERVERS];
        uint8_t server_mac[6];
    } acked;
    DebugObject d_obj;
} BDHCPClientCore;

int BDHCPClientCore_Init (BDHCPClientCore *o, PacketPassInterface *send_if, PacketRecvInterface *recv_if,
                          uint8_t *client_mac_addr, struct BDHCPClientCore_opts opts, BReactor *reactor,
                          BRandom2 *random2, void *user,
                          BDHCPClientCore_func_getsendermac func_getsendermac,
                          BDHCPClientCore_handler handler);
void BDHCPClientCore_Free (BDHCPClientCore *o);
void BDHCPClientCore_GetClientIP (BDHCPClientCore *o, uint32_t *out_ip);
void BDHCPClientCore_GetClientMask (BDHCPClientCore *o, uint32_t *out_mask);
int BDHCPClientCore_GetRouter (BDHCPClientCore *o, uint32_t *out_router);
int BDHCPClientCore_GetDNS (BDHCPClientCore *o, uint32_t *out_dns_servers, size_t max_dns_servers);
void BDHCPClientCore_GetServerMAC (BDHCPClientCore *o, uint8_t *out_mac);

#endif
