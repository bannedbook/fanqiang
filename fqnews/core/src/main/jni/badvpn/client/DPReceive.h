/**
 * @file DPReceive.h
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
 * Receive processing for the VPN client.
 */

#ifndef BADVPN_CLIENT_DPRECEIVE_H
#define BADVPN_CLIENT_DPRECEIVE_H

#include <protocol/scproto.h>
#include <misc/debugcounter.h>
#include <misc/debug.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <client/DataProto.h>
#include <client/DPRelay.h>
#include <client/FrameDecider.h>

typedef void (*DPReceiveDevice_output_func) (void *output_user, uint8_t *data, int data_len);

struct DPReceiveReceiver_s;

typedef struct {
    int device_mtu;
    DPReceiveDevice_output_func output_func;
    void *output_func_user;
    BReactor *reactor;
    int relay_flow_buffer_size;
    int relay_flow_inactivity_time;
    int packet_mtu;
    DPRelayRouter relay_router;
    int have_peer_id;
    peerid_t peer_id;
    LinkedList1 peers_list;
    DebugObject d_obj;
} DPReceiveDevice;

typedef struct {
    DPReceiveDevice *device;
    peerid_t peer_id;
    FrameDeciderPeer *decider_peer;
    int is_relay_client;
    DPRelaySource relay_source;
    DPRelaySink relay_sink;
    DataProtoSink *dp_sink;
    LinkedList1Node list_node;
    DebugObject d_obj;
    DebugCounter d_receivers_ctr;
} DPReceivePeer;

typedef struct DPReceiveReceiver_s {
    DPReceivePeer *peer;
    DPReceiveDevice *device;
    PacketPassInterface recv_if;
    DebugObject d_obj;
} DPReceiveReceiver;

int DPReceiveDevice_Init (DPReceiveDevice *o, int device_mtu, DPReceiveDevice_output_func output_func, void *output_func_user, BReactor *reactor, int relay_flow_buffer_size, int relay_flow_inactivity_time) WARN_UNUSED;
void DPReceiveDevice_Free (DPReceiveDevice *o);
void DPReceiveDevice_SetPeerID (DPReceiveDevice *o, peerid_t peer_id);

void DPReceivePeer_Init (DPReceivePeer *o, DPReceiveDevice *device, peerid_t peer_id, FrameDeciderPeer *decider_peer, int is_relay_client);
void DPReceivePeer_Free (DPReceivePeer *o);
void DPReceivePeer_AttachSink (DPReceivePeer *o, DataProtoSink *dp_sink);
void DPReceivePeer_DetachSink (DPReceivePeer *o);

void DPReceiveReceiver_Init (DPReceiveReceiver *o, DPReceivePeer *peer);
void DPReceiveReceiver_Free (DPReceiveReceiver *o);
PacketPassInterface * DPReceiveReceiver_GetInput (DPReceiveReceiver *o);

#endif
