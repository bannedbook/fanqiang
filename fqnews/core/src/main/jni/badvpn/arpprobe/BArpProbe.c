/**
 * @file BArpProbe.c
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <linux/filter.h>

#include <misc/debug.h>
#include <misc/byteorder.h>
#include <misc/ethernet_proto.h>
#include <misc/ipv4_proto.h>
#include <misc/udp_proto.h>
#include <misc/get_iface_info.h>
#include <base/BLog.h>

#include "BArpProbe.h"

#include <generated/blog_channel_BArpProbe.h>

#define STATE_INITIAL 1
#define STATE_NOEXIST 2
#define STATE_EXIST 3
#define STATE_EXIST_PANIC 4

static void dgram_handler (BArpProbe *o, int event)
{
    DebugObject_Access(&o->d_obj);
    
    BLog(BLOG_ERROR, "packet socket error");
    
    // report error
    DEBUGERROR(&o->d_err, o->handler(o->user, BARPPROBE_EVENT_ERROR));
    return;
}

static void send_request (BArpProbe *o)
{
    if (o->send_sending) {
        BLog(BLOG_ERROR, "cannot send packet while another packet is being sent!");
        return;
    }
    
    // build packet
    struct arp_packet *arp = &o->send_packet;
    arp->hardware_type = hton16(ARP_HARDWARE_TYPE_ETHERNET);
    arp->protocol_type = hton16(ETHERTYPE_IPV4);
    arp->hardware_size = hton8(6);
    arp->protocol_size = hton8(4);
    arp->opcode = hton16(ARP_OPCODE_REQUEST);
    memcpy(arp->sender_mac, o->if_mac, 6);
    arp->sender_ip = hton32(0);
    memset(arp->target_mac, 0, sizeof(arp->target_mac));
    arp->target_ip = o->addr;
    
    // send packet
    PacketPassInterface_Sender_Send(o->send_if, (uint8_t *)&o->send_packet, sizeof(o->send_packet));
    
    // set sending
    o->send_sending = 1;
}

static void send_if_handler_done (BArpProbe *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->send_sending)
    
    // set not sending
    o->send_sending = 0;
}

static void recv_if_handler_done (BArpProbe *o, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(data_len >= 0)
    ASSERT(data_len <= sizeof(struct arp_packet))
    
    // receive next packet
    PacketRecvInterface_Receiver_Recv(o->recv_if, (uint8_t *)&o->recv_packet);
    
    if (data_len != sizeof(struct arp_packet)) {
        BLog(BLOG_WARNING, "receive: wrong size");
        return;
    }
    
    struct arp_packet *arp = &o->recv_packet;
    
    if (ntoh16(arp->hardware_type) != ARP_HARDWARE_TYPE_ETHERNET) {
        BLog(BLOG_WARNING, "receive: wrong hardware type");
        return;
    }
    
    if (ntoh16(arp->protocol_type) != ETHERTYPE_IPV4) {
        BLog(BLOG_WARNING, "receive: wrong protocol type");
        return;
    }
    
    if (ntoh8(arp->hardware_size) != 6) {
        BLog(BLOG_WARNING, "receive: wrong hardware size");
        return;
    }
    
    if (ntoh8(arp->protocol_size) != 4) {
        BLog(BLOG_WARNING, "receive: wrong protocol size");
        return;
    }
    
    if (ntoh16(arp->opcode) != ARP_OPCODE_REPLY) {
        return;
    }
    
    if (arp->sender_ip != o->addr) {
        return;
    }
    
    int old_state = o->state;
    
    // set minus one missed
    o->num_missed = -1;
    
    // set timer
    BReactor_SetTimerAfter(o->reactor, &o->timer, BARPPROBE_EXIST_WAITSEND);
    
    // set state exist
    o->state = STATE_EXIST;
    
    // report exist if needed
    if (old_state == STATE_INITIAL || old_state == STATE_NOEXIST) {
        o->handler(o->user, BARPPROBE_EVENT_EXIST);
        return;
    }
}

static void timer_handler (BArpProbe *o)
{
    DebugObject_Access(&o->d_obj);
    
    // send request
    send_request(o);
    
    switch (o->state) {
        case STATE_INITIAL: {
            ASSERT(o->num_missed >= 0)
            ASSERT(o->num_missed < BARPPROBE_INITIAL_NUM_ATTEMPTS)
            
            // increment missed
            o->num_missed++;
            
            // all attempts failed?
            if (o->num_missed == BARPPROBE_INITIAL_NUM_ATTEMPTS) {
                // set timer
                BReactor_SetTimerAfter(o->reactor, &o->timer, BARPPROBE_NOEXIST_WAITRECV);
                
                // set state noexist
                o->state = STATE_NOEXIST;
                
                // report noexist
                o->handler(o->user, BARPPROBE_EVENT_NOEXIST);
                return;
            }
            
            // set timer
            BReactor_SetTimerAfter(o->reactor, &o->timer, BARPPROBE_INITIAL_WAITRECV);
        } break;
        
        case STATE_NOEXIST: {
            // set timer
            BReactor_SetTimerAfter(o->reactor, &o->timer, BARPPROBE_NOEXIST_WAITRECV);
        } break;
        
        case STATE_EXIST: {
            ASSERT(o->num_missed >= -1)
            ASSERT(o->num_missed < BARPPROBE_EXIST_NUM_NOREPLY)
            
            // increment missed
            o->num_missed++;
            
            // all missed?
            if (o->num_missed == BARPPROBE_EXIST_NUM_NOREPLY) {
                // set timer
                BReactor_SetTimerAfter(o->reactor, &o->timer, BARPPROBE_EXIST_PANIC_WAITRECV);
                
                // set zero missed
                o->num_missed = 0;
                
                // set state panic
                o->state = STATE_EXIST_PANIC;
                return;
            }
            
            // set timer
            BReactor_SetTimerAfter(o->reactor, &o->timer, BARPPROBE_EXIST_WAITRECV);
        } break;
        
        case STATE_EXIST_PANIC: {
            ASSERT(o->num_missed >= 0)
            ASSERT(o->num_missed < BARPPROBE_EXIST_PANIC_NUM_NOREPLY)
            
            // increment missed
            o->num_missed++;
            
            // all missed?
            if (o->num_missed == BARPPROBE_EXIST_PANIC_NUM_NOREPLY) {
                // set timer
                BReactor_SetTimerAfter(o->reactor, &o->timer, BARPPROBE_NOEXIST_WAITRECV);
                
                // set state panic
                o->state = STATE_NOEXIST;
                
                // report noexist
                o->handler(o->user, BARPPROBE_EVENT_NOEXIST);
                return;
            }
            
            // set timer
            BReactor_SetTimerAfter(o->reactor, &o->timer, BARPPROBE_EXIST_PANIC_WAITRECV);
        } break;
    }
}

int BArpProbe_Init (BArpProbe *o, const char *ifname, uint32_t addr, BReactor *reactor, void *user, BArpProbe_handler handler)
{
    ASSERT(ifname)
    ASSERT(handler)
    
    // init arguments
    o->addr = addr;
    o->reactor = reactor;
    o->user = user;
    o->handler = handler;
    
    // get interface information
    int if_mtu;
    int if_index;
    if (!badvpn_get_iface_info(ifname, o->if_mac, &if_mtu, &if_index)) {
        BLog(BLOG_ERROR, "failed to get interface information");
        goto fail0;
    }
    
    uint8_t *if_mac = o->if_mac;
    BLog(BLOG_INFO, "if_mac=%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8" if_mtu=%d if_index=%d",
         if_mac[0], if_mac[1], if_mac[2], if_mac[3], if_mac[4], if_mac[5], if_mtu, if_index);
    
    // check MTU
    if (if_mtu < sizeof(struct arp_packet)) {
        BLog(BLOG_ERROR, "MTU is too small for ARP !?!");
        goto fail0;
    }
    
    // init dgram
    if (!BDatagram_Init(&o->dgram, BADDR_TYPE_PACKET, o->reactor, o, (BDatagram_handler)dgram_handler)) {
        BLog(BLOG_ERROR, "BDatagram_Init failed");
        goto fail0;
    }
    
    // bind dgram
    BAddr bind_addr;
    BAddr_InitPacket(&bind_addr, hton16(ETHERTYPE_ARP), if_index, BADDR_PACKET_HEADER_TYPE_ETHERNET, BADDR_PACKET_PACKET_TYPE_HOST, if_mac);
    if (!BDatagram_Bind(&o->dgram, bind_addr)) {
        BLog(BLOG_ERROR, "BDatagram_Bind failed");
        goto fail1;
    }
    
    // set dgram send addresses
    BAddr dest_addr;
    uint8_t broadcast_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    BAddr_InitPacket(&dest_addr, hton16(ETHERTYPE_ARP), if_index, BADDR_PACKET_HEADER_TYPE_ETHERNET, BADDR_PACKET_PACKET_TYPE_BROADCAST, broadcast_mac);
    BIPAddr local_addr;
    BIPAddr_InitInvalid(&local_addr);
    BDatagram_SetSendAddrs(&o->dgram, dest_addr, local_addr);
    
    // init send interface
    BDatagram_SendAsync_Init(&o->dgram, sizeof(struct arp_packet));
    o->send_if = BDatagram_SendAsync_GetIf(&o->dgram);
    PacketPassInterface_Sender_Init(o->send_if, (PacketPassInterface_handler_done)send_if_handler_done, o);
    
    // set not sending
    o->send_sending = 0;
    
    // init recv interface
    BDatagram_RecvAsync_Init(&o->dgram, sizeof(struct arp_packet));
    o->recv_if = BDatagram_RecvAsync_GetIf(&o->dgram);
    PacketRecvInterface_Receiver_Init(o->recv_if, (PacketRecvInterface_handler_done)recv_if_handler_done, o);
    
    // init timer
    BTimer_Init(&o->timer, 0, (BTimer_handler)timer_handler, o);
    
    // receive first packet
    PacketRecvInterface_Receiver_Recv(o->recv_if, (uint8_t *)&o->recv_packet);
    
    // send request
    send_request(o);
    
    // set timer
    BReactor_SetTimerAfter(o->reactor, &o->timer, BARPPROBE_INITIAL_WAITRECV);
    
    // set zero missed
    o->num_missed = 0;
    
    // set state initial
    o->state = STATE_INITIAL;
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(o->reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    BDatagram_Free(&o->dgram);
fail0:
    return 0;
}

void BArpProbe_Free (BArpProbe *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    
    // free timer
    BReactor_RemoveTimer(o->reactor, &o->timer);
    
    // free recv interface
    BDatagram_RecvAsync_Free(&o->dgram);
    
    // free send interface
    BDatagram_SendAsync_Free(&o->dgram);
    
    // free dgram
    BDatagram_Free(&o->dgram);
}
