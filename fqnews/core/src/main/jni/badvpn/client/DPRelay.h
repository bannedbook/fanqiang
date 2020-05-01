/**
 * @file DPRelay.h
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

#ifndef BADVPN_CLIENT_DPRELAY_H
#define BADVPN_CLIENT_DPRELAY_H

#include <stdint.h>
#include <limits.h>

#include <protocol/scproto.h>
#include <protocol/dataproto.h>
#include <misc/debug.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <flow/BufferWriter.h>
#include <client/DataProto.h>

struct DPRelay_flow;

typedef struct {
    int frame_mtu;
    BufferWriter writer;
    DataProtoSource dp_source;
    struct DPRelay_flow *current_flow;
    DebugObject d_obj;
    DebugCounter d_ctr;
} DPRelayRouter;

typedef struct {
    DPRelayRouter *router;
    peerid_t source_id;
    LinkedList1 flows_list;
    DebugObject d_obj;
} DPRelaySource;

typedef struct {
    peerid_t dest_id;
    LinkedList1 flows_list;
    DataProtoSink *dp_sink;
    DebugObject d_obj;
} DPRelaySink;

struct DPRelay_flow {
    DPRelaySource *src;
    DPRelaySink *sink;
    DataProtoFlow dp_flow;
    LinkedList1Node src_list_node;
    LinkedList1Node sink_list_node;
};

int DPRelayRouter_Init (DPRelayRouter *o, int frame_mtu, BReactor *reactor) WARN_UNUSED;
void DPRelayRouter_Free (DPRelayRouter *o);
void DPRelayRouter_SubmitFrame (DPRelayRouter *o, DPRelaySource *src, DPRelaySink *sink, uint8_t *data, int data_len, int num_packets, int inactivity_time);

void DPRelaySource_Init (DPRelaySource *o, DPRelayRouter *router, peerid_t source_id, BReactor *reactor);
void DPRelaySource_Free (DPRelaySource *o);

void DPRelaySink_Init (DPRelaySink *o, peerid_t dest_id);
void DPRelaySink_Free (DPRelaySink *o);
void DPRelaySink_Attach (DPRelaySink *o, DataProtoSink *dp_sink);
void DPRelaySink_Detach (DPRelaySink *o);

#endif
