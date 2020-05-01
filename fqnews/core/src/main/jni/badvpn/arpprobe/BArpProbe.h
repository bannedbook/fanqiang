/**
 * @file BArpProbe.h
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

#ifndef BADVPN_BARPPROBE_H
#define BADVPN_BARPPROBE_H

#include <stdint.h>

#include <misc/debug.h>
#include <misc/debugerror.h>
#include <misc/arp_proto.h>
#include <misc/ethernet_proto.h>
#include <base/DebugObject.h>
#include <system/BDatagram.h>
#include <system/BReactor.h>

#define BARPPROBE_INITIAL_WAITRECV 1000
#define BARPPROBE_INITIAL_NUM_ATTEMPTS 6
#define BARPPROBE_NOEXIST_WAITRECV 15000
#define BARPPROBE_EXIST_WAITSEND 15000
#define BARPPROBE_EXIST_WAITRECV 10000
#define BARPPROBE_EXIST_NUM_NOREPLY 2
#define BARPPROBE_EXIST_PANIC_WAITRECV 1000
#define BARPPROBE_EXIST_PANIC_NUM_NOREPLY 6

#define BARPPROBE_EVENT_EXIST 1
#define BARPPROBE_EVENT_NOEXIST 2
#define BARPPROBE_EVENT_ERROR 3

typedef void (*BArpProbe_handler) (void *user, int event);

typedef struct {
    uint32_t addr;
    BReactor *reactor;
    void *user;
    BArpProbe_handler handler;
    BDatagram dgram;
    uint8_t if_mac[6];
    PacketPassInterface *send_if;
    int send_sending;
    struct arp_packet send_packet;
    PacketRecvInterface *recv_if;
    struct arp_packet recv_packet;
    BTimer timer;
    int state;
    int num_missed;
    DebugError d_err;
    DebugObject d_obj;
} BArpProbe;

int BArpProbe_Init (BArpProbe *o, const char *ifname, uint32_t addr, BReactor *reactor, void *user, BArpProbe_handler handler) WARN_UNUSED;
void BArpProbe_Free (BArpProbe *o);

#endif
