/**
 * @file BDHCPClientCore.c
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

#include <string.h>
#include <stdlib.h>

#include <misc/byteorder.h>
#include <misc/minmax.h>
#include <misc/balloc.h>
#include <misc/bsize.h>
#include <misc/dhcp_proto.h>
#include <base/BLog.h>

#include <dhcpclient/BDHCPClientCore.h>

#include <generated/blog_channel_BDHCPClientCore.h>

#define RESET_TIMEOUT 4000
#define REQUEST_TIMEOUT 3000
#define RENEW_REQUEST_TIMEOUT 20000
#define MAX_REQUESTS 4
#define RENEW_TIMEOUT(lease) ((btime_t)500 * (lease))
#define XID_REUSE_MAX 8

#define LEASE_TIMEOUT(lease) ((btime_t)1000 * (lease) - RENEW_TIMEOUT(lease))

#define STATE_RESETTING 1
#define STATE_SENT_DISCOVER 2
#define STATE_SENT_REQUEST 3
#define STATE_FINISHED 4
#define STATE_RENEWING 5

#define IP_UDP_HEADERS_SIZE 28

static void report_up (BDHCPClientCore *o)
{
    o->handler(o->user, BDHCPCLIENTCORE_EVENT_UP);
    return;
}

static void report_down (BDHCPClientCore *o)
{
    o->handler(o->user, BDHCPCLIENTCORE_EVENT_DOWN);
    return;
}

static void send_message (
    BDHCPClientCore *o,
    int type,
    uint32_t xid,
    int have_requested_ip_address, uint32_t requested_ip_address,
    int have_dhcp_server_identifier, uint32_t dhcp_server_identifier
)
{
    ASSERT(type == DHCP_MESSAGE_TYPE_DISCOVER || type == DHCP_MESSAGE_TYPE_REQUEST)
    
    if (o->sending) {
        BLog(BLOG_ERROR, "already sending");
        return;
    }
    
    // write header
    struct dhcp_header header;
    memset(&header, 0, sizeof(header));
    header.op = hton8(DHCP_OP_BOOTREQUEST);
    header.htype = hton8(DHCP_HARDWARE_ADDRESS_TYPE_ETHERNET);
    header.hlen = hton8(6);
    header.xid = xid;
    header.secs = hton16(0);
    memcpy(header.chaddr, o->client_mac_addr, sizeof(o->client_mac_addr));
    header.magic = hton32(DHCP_MAGIC);
    memcpy(o->send_buf, &header, sizeof(header));
    
    // write options
    
    char *out = o->send_buf + sizeof(header);
    struct dhcp_option_header oh;
    
    // DHCP message type
    {
        oh.type = hton8(DHCP_OPTION_DHCP_MESSAGE_TYPE);
        oh.len = hton8(sizeof(struct dhcp_option_dhcp_message_type));
        struct dhcp_option_dhcp_message_type opt;
        opt.type = hton8(type);
        memcpy(out, &oh, sizeof(oh));
        memcpy(out + sizeof(oh), &opt, sizeof(opt));
        out += sizeof(oh) + sizeof(opt);
    }
    
    if (have_requested_ip_address) {
        // requested IP address
        oh.type = hton8(DHCP_OPTION_REQUESTED_IP_ADDRESS);
        oh.len = hton8(sizeof(struct dhcp_option_addr));
        struct dhcp_option_addr opt;
        opt.addr = requested_ip_address;
        memcpy(out, &oh, sizeof(oh));
        memcpy(out + sizeof(oh), &opt, sizeof(opt));
        out += sizeof(oh) + sizeof(opt);
    }
    
    if (have_dhcp_server_identifier) {
        // DHCP server identifier
        oh.type = hton8(DHCP_OPTION_DHCP_SERVER_IDENTIFIER);
        oh.len = hton8(sizeof(struct dhcp_option_dhcp_server_identifier));
        struct dhcp_option_dhcp_server_identifier opt;
        opt.id = dhcp_server_identifier;
        memcpy(out, &oh, sizeof(oh));
        memcpy(out + sizeof(oh), &opt, sizeof(opt));
        out += sizeof(oh) + sizeof(opt);
    }
    
    // maximum message size
    {
        oh.type = hton8(DHCP_OPTION_MAXIMUM_MESSAGE_SIZE);
        oh.len = hton8(sizeof(struct dhcp_option_maximum_message_size));
        struct dhcp_option_maximum_message_size opt;
        opt.size = hton16(IP_UDP_HEADERS_SIZE + PacketRecvInterface_GetMTU(o->recv_if));
        memcpy(out, &oh, sizeof(oh));
        memcpy(out + sizeof(oh), &opt, sizeof(opt));
        out += sizeof(oh) + sizeof(opt);
    }
    
    // parameter request list
    {
        oh.type = hton8(DHCP_OPTION_PARAMETER_REQUEST_LIST);
        oh.len = hton8(4);
        uint8_t opt[4];
        opt[0] = DHCP_OPTION_SUBNET_MASK;
        opt[1] = DHCP_OPTION_ROUTER;
        opt[2] = DHCP_OPTION_DOMAIN_NAME_SERVER;
        opt[3] = DHCP_OPTION_IP_ADDRESS_LEASE_TIME;
        memcpy(out, &oh, sizeof(oh));
        memcpy(out + sizeof(oh), &opt, sizeof(opt));
        out += sizeof(oh) + sizeof(opt);
    }
    
    if (o->hostname) {
        // host name
        oh.type = hton8(DHCP_OPTION_HOST_NAME);
        oh.len = hton8(strlen(o->hostname));
        memcpy(out, &oh, sizeof(oh));
        memcpy(out + sizeof(oh), o->hostname, strlen(o->hostname));
        out += sizeof(oh) + strlen(o->hostname);
    }
    
    if (o->vendorclassid) {
        // vendor class identifier
        oh.type = hton8(DHCP_OPTION_VENDOR_CLASS_IDENTIFIER);
        oh.len = hton8(strlen(o->vendorclassid));
        memcpy(out, &oh, sizeof(oh));
        memcpy(out + sizeof(oh), o->vendorclassid, strlen(o->vendorclassid));
        out += sizeof(oh) + strlen(o->vendorclassid);
    }
    
    if (o->clientid) {
        // client identifier
        oh.type = hton8(DHCP_OPTION_CLIENT_IDENTIFIER);
        oh.len = hton8(o->clientid_len);
        memcpy(out, &oh, sizeof(oh));
        memcpy(out + sizeof(oh), o->clientid, o->clientid_len);
        out += sizeof(oh) + o->clientid_len;
    }
    
    // end option
    uint8_t end = 0xFF;
    memcpy(out, &end, sizeof(end));
    out += sizeof(end);
    
    // send it
    PacketPassInterface_Sender_Send(o->send_if, (uint8_t *)o->send_buf, out - o->send_buf);
    o->sending = 1;
}

static void send_handler_done (BDHCPClientCore *o)
{
    ASSERT(o->sending)
    DebugObject_Access(&o->d_obj);
    
    o->sending = 0;
}

static void recv_handler_done (BDHCPClientCore *o, int data_len)
{
    ASSERT(data_len >= 0)
    DebugObject_Access(&o->d_obj);
    
    // receive more packets
    PacketRecvInterface_Receiver_Recv(o->recv_if, (uint8_t *)o->recv_buf);
    
    if (o->state == STATE_RESETTING) {
        return;
    }
    
    // check header
    
    if (data_len < sizeof(struct dhcp_header)) {
        return;
    }
    
    struct dhcp_header header;
    memcpy(&header, o->recv_buf, sizeof(header));
    
    if (ntoh8(header.op) != DHCP_OP_BOOTREPLY) {
        return;
    }
    
    if (ntoh8(header.htype) != DHCP_HARDWARE_ADDRESS_TYPE_ETHERNET) {
        return;
    }
    
    if (ntoh8(header.hlen) != 6) {
        return;
    }
    
    if (header.xid != o->xid) {
        return;
    }
    
    if (memcmp(header.chaddr, o->client_mac_addr, sizeof(o->client_mac_addr))) {
        return;
    }
    
    if (ntoh32(header.magic) != DHCP_MAGIC) {
        return;
    }
    
    // parse and check options
    
    uint8_t *pos = (uint8_t *)o->recv_buf + sizeof(header);
    int len = data_len - sizeof(header);
    
    int have_end = 0;
    
    int dhcp_message_type = -1;
    
    int have_dhcp_server_identifier = 0;
    uint32_t dhcp_server_identifier = 0; // to remove warning
    
    int have_ip_address_lease_time = 0;
    uint32_t ip_address_lease_time = 0; // to remove warning
    
    int have_subnet_mask = 0;
    uint32_t subnet_mask = 0; // to remove warning
    
    int have_router = 0;
    uint32_t router = 0; // to remove warning
    
    int domain_name_servers_count = 0;
    uint32_t domain_name_servers[BDHCPCLIENTCORE_MAX_DOMAIN_NAME_SERVERS];
    
    while (len > 0) {
        // padding option ?
        if (*pos == 0) {
            pos++;
            len--;
            continue;
        }
        
        if (have_end) {
            return;
        }
        
        // end option ?
        if (*pos == 0xff) {
            pos++;
            len--;
            have_end = 1;
            continue;
        }
        
        // check option header
        if (len < sizeof(struct dhcp_option_header)) {
            return;
        }
        struct dhcp_option_header opt;
        memcpy(&opt, pos, sizeof(opt));
        pos += sizeof(opt);
        len -= sizeof(opt);
        int opt_type = ntoh8(opt.type);
        int opt_len = ntoh8(opt.len);
        
        // check option payload
        if (opt_len > len) {
            return;
        }
        uint8_t *optval = pos;
        pos += opt_len;
        len -= opt_len;
        
        switch (opt_type) {
            case DHCP_OPTION_DHCP_MESSAGE_TYPE: {
                if (opt_len != sizeof(struct dhcp_option_dhcp_message_type)) {
                    return;
                }
                struct dhcp_option_dhcp_message_type val;
                memcpy(&val, optval, sizeof(val));
                
                dhcp_message_type = ntoh8(val.type);
            } break;
            
            case DHCP_OPTION_DHCP_SERVER_IDENTIFIER: {
                if (opt_len != sizeof(struct dhcp_option_dhcp_server_identifier)) {
                    return;
                }
                struct dhcp_option_dhcp_server_identifier val;
                memcpy(&val, optval, sizeof(val));
                
                dhcp_server_identifier = val.id;
                have_dhcp_server_identifier = 1;
            } break;
            
            case DHCP_OPTION_IP_ADDRESS_LEASE_TIME: {
                if (opt_len != sizeof(struct dhcp_option_time)) {
                    return;
                }
                struct dhcp_option_time val;
                memcpy(&val, optval, sizeof(val));
                
                ip_address_lease_time = ntoh32(val.time);
                have_ip_address_lease_time = 1;
            } break;
            
            case DHCP_OPTION_SUBNET_MASK: {
                if (opt_len != sizeof(struct dhcp_option_addr)) {
                    return;
                }
                struct dhcp_option_addr val;
                memcpy(&val, optval, sizeof(val));
                
                subnet_mask = val.addr;
                have_subnet_mask = 1;
            } break;
            
            case DHCP_OPTION_ROUTER: {
                if (opt_len != sizeof(struct dhcp_option_addr)) {
                    return;
                }
                struct dhcp_option_addr val;
                memcpy(&val, optval, sizeof(val));
                
                router = val.addr;
                have_router = 1;
            } break;
            
            case DHCP_OPTION_DOMAIN_NAME_SERVER: {
                if (opt_len % sizeof(struct dhcp_option_addr)) {
                    return;
                }
                
                int num_servers = opt_len / sizeof(struct dhcp_option_addr);
                
                int i;
                for (i = 0; i < num_servers && i < BDHCPCLIENTCORE_MAX_DOMAIN_NAME_SERVERS; i++) {
                    struct dhcp_option_addr addr;
                    memcpy(&addr, optval + i * sizeof(addr), sizeof(addr));
                    domain_name_servers[i] = addr.addr;
                }
                
                domain_name_servers_count = i;
            } break;
        }
    }
    
    if (!have_end) {
        return;
    }
    
    if (dhcp_message_type == -1) {
        return;
    }
    
    if (dhcp_message_type != DHCP_MESSAGE_TYPE_OFFER && dhcp_message_type != DHCP_MESSAGE_TYPE_ACK && dhcp_message_type != DHCP_MESSAGE_TYPE_NAK) {
        return;
    }
    
    if (!have_dhcp_server_identifier) {
        return;
    }
    
    if (dhcp_message_type == DHCP_MESSAGE_TYPE_NAK) {
        if (o->state != STATE_SENT_REQUEST && o->state != STATE_FINISHED && o->state != STATE_RENEWING) {
            return;
        }
        
        if (dhcp_server_identifier != o->offered.dhcp_server_identifier) {
            return;
        }
        
        if (o->state == STATE_SENT_REQUEST) {
            BLog(BLOG_INFO, "received NAK (in sent request)");
            
            // stop request timer
            BReactor_RemoveTimer(o->reactor, &o->request_timer);
            
            // start reset timer
            BReactor_SetTimer(o->reactor, &o->reset_timer);
            
            // set state
            o->state = STATE_RESETTING;
        }
        else if (o->state == STATE_FINISHED) {
            BLog(BLOG_INFO, "received NAK (in finished)");
            
            // stop renew timer
            BReactor_RemoveTimer(o->reactor, &o->renew_timer);
            
            // start reset timer
            BReactor_SetTimer(o->reactor, &o->reset_timer);
            
            // set state
            o->state = STATE_RESETTING;
            
            // report to user
            report_down(o);
            return;
        }
        else { // STATE_RENEWING
            BLog(BLOG_INFO, "received NAK (in renewing)");
            
            // stop renew request timer
            BReactor_RemoveTimer(o->reactor, &o->renew_request_timer);
            
            // stop lease timer
            BReactor_RemoveTimer(o->reactor, &o->lease_timer);
            
            // start reset timer
            BReactor_SetTimer(o->reactor, &o->reset_timer);
            
            // set state
            o->state = STATE_RESETTING;
            
            // report to user
            report_down(o);
            return;
        }
        
        return;
    }
    
    if (ntoh32(header.yiaddr) == 0) {
        return;
    }
    
    if (!have_ip_address_lease_time) {
        return;
    }
    
    if (!have_subnet_mask) {
        return;
    }
    
    if (o->state == STATE_SENT_DISCOVER && dhcp_message_type == DHCP_MESSAGE_TYPE_OFFER) {
        BLog(BLOG_INFO, "received OFFER");
        
        // remember offer
        o->offered.yiaddr = header.yiaddr;
        o->offered.dhcp_server_identifier = dhcp_server_identifier;
        
        // send request
        send_message(o, DHCP_MESSAGE_TYPE_REQUEST, o->xid, 1, o->offered.yiaddr, 1, o->offered.dhcp_server_identifier);
        
        // stop reset timer
        BReactor_RemoveTimer(o->reactor, &o->reset_timer);
        
        // start request timer
        BReactor_SetTimer(o->reactor, &o->request_timer);
        
        // set state
        o->state = STATE_SENT_REQUEST;
        
        // set request count
        o->request_count = 1;
    }
    else if (o->state == STATE_SENT_REQUEST && dhcp_message_type == DHCP_MESSAGE_TYPE_ACK) {
        if (header.yiaddr != o->offered.yiaddr) {
            return;
        }
        
        if (dhcp_server_identifier != o->offered.dhcp_server_identifier) {
            return;
        }
        
        BLog(BLOG_INFO, "received ACK (in sent request)");
        
        // remember stuff
        o->acked.ip_address_lease_time = ip_address_lease_time;
        o->acked.subnet_mask = subnet_mask;
        o->acked.have_router = have_router;
        if (have_router) {
            o->acked.router = router;
        }
        o->acked.domain_name_servers_count = domain_name_servers_count;
        memcpy(o->acked.domain_name_servers, domain_name_servers, domain_name_servers_count * sizeof(uint32_t));
        o->func_getsendermac(o->user, o->acked.server_mac);
        
        // stop request timer
        BReactor_RemoveTimer(o->reactor, &o->request_timer);
        
        // start renew timer
        BReactor_SetTimerAfter(o->reactor, &o->renew_timer, RENEW_TIMEOUT(o->acked.ip_address_lease_time));
        
        // set state
        o->state = STATE_FINISHED;
        
        // report to user
        report_up(o);
        return;
    }
    else if (o->state == STATE_RENEWING && dhcp_message_type == DHCP_MESSAGE_TYPE_ACK) {
        if (header.yiaddr != o->offered.yiaddr) {
            return;
        }
        
        if (dhcp_server_identifier != o->offered.dhcp_server_identifier) {
            return;
        }
        
        // TODO: check parameters?
        
        BLog(BLOG_INFO, "received ACK (in renewing)");
        
        // remember stuff
        o->acked.ip_address_lease_time = ip_address_lease_time;
        
        // stop renew request timer
        BReactor_RemoveTimer(o->reactor, &o->renew_request_timer);
        
        // stop lease timer
        BReactor_RemoveTimer(o->reactor, &o->lease_timer);
        
        // start renew timer
        BReactor_SetTimerAfter(o->reactor, &o->renew_timer, RENEW_TIMEOUT(o->acked.ip_address_lease_time));
        
        // set state
        o->state = STATE_FINISHED;
    }
}

static void start_process (BDHCPClientCore *o, int force_new_xid)
{
    if (force_new_xid || o->xid_reuse_counter == XID_REUSE_MAX) {
        // generate xid
        if (!BRandom2_GenBytes(o->random2, &o->xid, sizeof(o->xid))) {
            BLog(BLOG_ERROR, "BRandom2_GenBytes failed");
            o->xid = UINT32_C(3416960072);
        }
        
        // reset counter
        o->xid_reuse_counter = 0;
    }
    
    // increment counter
    o->xid_reuse_counter++;
    
    // send discover
    send_message(o, DHCP_MESSAGE_TYPE_DISCOVER, o->xid, 0, 0, 0, 0);
    
    // set timer
    BReactor_SetTimer(o->reactor, &o->reset_timer);
    
    // set state
    o->state = STATE_SENT_DISCOVER;
}

static void reset_timer_handler (BDHCPClientCore *o)
{
    ASSERT(o->state == STATE_RESETTING || o->state == STATE_SENT_DISCOVER)
    DebugObject_Access(&o->d_obj);
    
    BLog(BLOG_INFO, "reset timer");
    
    start_process(o, (o->state == STATE_RESETTING));
}

static void request_timer_handler (BDHCPClientCore *o)
{
    ASSERT(o->state == STATE_SENT_REQUEST)
    ASSERT(o->request_count >= 1)
    ASSERT(o->request_count <= MAX_REQUESTS)
    DebugObject_Access(&o->d_obj);
    
    // if we have sent enough requests, start again
    if (o->request_count == MAX_REQUESTS) {
        BLog(BLOG_INFO, "request timer, aborting");
        
        start_process(o, 0);
        return;
    }
    
    BLog(BLOG_INFO, "request timer, retrying");
    
    // send request
    send_message(o, DHCP_MESSAGE_TYPE_REQUEST, o->xid, 1, o->offered.yiaddr, 1, o->offered.dhcp_server_identifier);
    
    // start request timer
    BReactor_SetTimer(o->reactor, &o->request_timer);
    
    // increment request count
    o->request_count++;
}

static void renew_timer_handler (BDHCPClientCore *o)
{
    ASSERT(o->state == STATE_FINISHED)
    DebugObject_Access(&o->d_obj);
    
    BLog(BLOG_INFO, "renew timer");
    
    // send request
    send_message(o, DHCP_MESSAGE_TYPE_REQUEST, o->xid, 1, o->offered.yiaddr, 0, 0);
    
    // start renew request timer
    BReactor_SetTimer(o->reactor, &o->renew_request_timer);
    
    // start lease timer
    BReactor_SetTimerAfter(o->reactor, &o->lease_timer, LEASE_TIMEOUT(o->acked.ip_address_lease_time));
    
    // set state
    o->state = STATE_RENEWING;
}

static void renew_request_timer_handler (BDHCPClientCore *o)
{
    ASSERT(o->state == STATE_RENEWING)
    DebugObject_Access(&o->d_obj);
    
    BLog(BLOG_INFO, "renew request timer");
    
    // send request
    send_message(o, DHCP_MESSAGE_TYPE_REQUEST, o->xid, 1, o->offered.yiaddr, 0, 0);
    
    // start renew request timer
    BReactor_SetTimer(o->reactor, &o->renew_request_timer);
}

static void lease_timer_handler (BDHCPClientCore *o)
{
    ASSERT(o->state == STATE_RENEWING)
    DebugObject_Access(&o->d_obj);
    
    BLog(BLOG_INFO, "lease timer");
    
    // stop renew request timer
    BReactor_RemoveTimer(o->reactor, &o->renew_request_timer);
    
    // start again now
    start_process(o, 1);
    
    // report to user
    report_down(o);
    return;
}

static bsize_t maybe_len (const char *str)
{
    return bsize_fromsize(str ? strlen(str) : 0);
}

int BDHCPClientCore_Init (BDHCPClientCore *o, PacketPassInterface *send_if, PacketRecvInterface *recv_if,
                          uint8_t *client_mac_addr, struct BDHCPClientCore_opts opts, BReactor *reactor,
                          BRandom2 *random2, void *user,
                          BDHCPClientCore_func_getsendermac func_getsendermac,
                          BDHCPClientCore_handler handler)
{
    ASSERT(PacketPassInterface_GetMTU(send_if) == PacketRecvInterface_GetMTU(recv_if))
    ASSERT(PacketPassInterface_GetMTU(send_if) >= 576 - IP_UDP_HEADERS_SIZE)
    ASSERT(func_getsendermac)
    ASSERT(handler)
    
    // init arguments
    o->send_if = send_if;
    o->recv_if = recv_if;
    memcpy(o->client_mac_addr, client_mac_addr, sizeof(o->client_mac_addr));
    o->reactor = reactor;
    o->random2 = random2;
    o->user = user;
    o->func_getsendermac = func_getsendermac;
    o->handler = handler;
    
    o->hostname = NULL;
    o->vendorclassid = NULL;
    o->clientid = NULL;
    o->clientid_len = 0;
    
    // copy options
    if (opts.hostname && !(o->hostname = strdup(opts.hostname))) {
        BLog(BLOG_ERROR, "strdup failed");
        goto fail0;
    }
    if (opts.vendorclassid && !(o->vendorclassid = strdup(opts.vendorclassid))) {
        BLog(BLOG_ERROR, "strdup failed");
        goto fail0;
    }
    if (opts.clientid) {
        if (!(o->clientid = BAlloc(opts.clientid_len))) {
            BLog(BLOG_ERROR, "BAlloc failed");
            goto fail0;
        }
        memcpy(o->clientid, opts.clientid, opts.clientid_len);
        o->clientid_len = opts.clientid_len;
    }
    
    // make sure options aren't too long
    bsize_t opts_size = bsize_add(maybe_len(o->hostname), bsize_add(maybe_len(o->vendorclassid), bsize_fromsize(o->clientid_len)));
    if (opts_size.is_overflow || opts_size.value > 100) {
        BLog(BLOG_ERROR, "options too long together");
        goto fail0;
    }
    if (o->hostname && strlen(o->hostname) > 255) {
        BLog(BLOG_ERROR, "hostname too long");
        goto fail0;
    }
    if (o->vendorclassid && strlen(o->vendorclassid) > 255) {
        BLog(BLOG_ERROR, "vendorclassid too long");
        goto fail0;
    }
    if (o->clientid && o->clientid_len > 255) {
        BLog(BLOG_ERROR, "clientid too long");
        goto fail0;
    }
    
    // allocate buffers
    if (!(o->send_buf = BAlloc(PacketPassInterface_GetMTU(send_if)))) {
        BLog(BLOG_ERROR, "BAlloc send buf failed");
        goto fail0;
    }
    if (!(o->recv_buf = BAlloc(PacketRecvInterface_GetMTU(recv_if)))) {
        BLog(BLOG_ERROR, "BAlloc recv buf failed");
        goto fail1;
    }
    
    // init send interface
    PacketPassInterface_Sender_Init(o->send_if, (PacketPassInterface_handler_done)send_handler_done, o);
    
    // init receive interface
    PacketRecvInterface_Receiver_Init(o->recv_if, (PacketRecvInterface_handler_done)recv_handler_done, o);
    
    // set not sending
    o->sending = 0;
    
    // init timers
    BTimer_Init(&o->reset_timer, RESET_TIMEOUT, (BTimer_handler)reset_timer_handler, o);
    BTimer_Init(&o->request_timer, REQUEST_TIMEOUT, (BTimer_handler)request_timer_handler, o);
    BTimer_Init(&o->renew_timer, 0, (BTimer_handler)renew_timer_handler, o);
    BTimer_Init(&o->renew_request_timer, RENEW_REQUEST_TIMEOUT, (BTimer_handler)renew_request_timer_handler, o);
    BTimer_Init(&o->lease_timer, 0, (BTimer_handler)lease_timer_handler, o);
    
    // start receving
    PacketRecvInterface_Receiver_Recv(o->recv_if, (uint8_t *)o->recv_buf);
    
    // start
    start_process(o, 1);
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail1:
    BFree(o->send_buf);
fail0:
    BFree(o->clientid);
    free(o->vendorclassid);
    free(o->hostname);
    return 0;
}

void BDHCPClientCore_Free (BDHCPClientCore *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free timers
    BReactor_RemoveTimer(o->reactor, &o->lease_timer);
    BReactor_RemoveTimer(o->reactor, &o->renew_request_timer);
    BReactor_RemoveTimer(o->reactor, &o->renew_timer);
    BReactor_RemoveTimer(o->reactor, &o->request_timer);
    BReactor_RemoveTimer(o->reactor, &o->reset_timer);
    
    // free buffers
    BFree(o->recv_buf);
    BFree(o->send_buf);
    
    // free options
    BFree(o->clientid);
    free(o->vendorclassid);
    free(o->hostname);
}

void BDHCPClientCore_GetClientIP (BDHCPClientCore *o, uint32_t *out_ip)
{
    ASSERT(o->state == STATE_FINISHED || o->state == STATE_RENEWING)
    DebugObject_Access(&o->d_obj);
    
    *out_ip = o->offered.yiaddr;
}

void BDHCPClientCore_GetClientMask (BDHCPClientCore *o, uint32_t *out_mask)
{
    ASSERT(o->state == STATE_FINISHED || o->state == STATE_RENEWING)
    DebugObject_Access(&o->d_obj);
    
    *out_mask = o->acked.subnet_mask;
}

int BDHCPClientCore_GetRouter (BDHCPClientCore *o, uint32_t *out_router)
{
    ASSERT(o->state == STATE_FINISHED || o->state == STATE_RENEWING)
    DebugObject_Access(&o->d_obj);
    
    if (!o->acked.have_router) {
        return 0;
    }
    
    *out_router = o->acked.router;
    return 1;
}

int BDHCPClientCore_GetDNS (BDHCPClientCore *o, uint32_t *out_dns_servers, size_t max_dns_servers)
{
    ASSERT(o->state == STATE_FINISHED || o->state == STATE_RENEWING)
    DebugObject_Access(&o->d_obj);
    
    int num_return = bmin_int(o->acked.domain_name_servers_count, max_dns_servers);
    
    memcpy(out_dns_servers, o->acked.domain_name_servers, num_return * sizeof(uint32_t));
    return num_return;
}

void BDHCPClientCore_GetServerMAC (BDHCPClientCore *o, uint8_t *out_mac)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state == STATE_FINISHED || o->state == STATE_RENEWING)
    
    memcpy(out_mac, o->acked.server_mac, 6);
}
