/**
 * @file DPRelay.c
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

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <base/BLog.h>

#include <client/DPRelay.h>

#include <generated/blog_channel_DPRelay.h>

static void flow_inactivity_handler (struct DPRelay_flow *flow);

static struct DPRelay_flow * create_flow (DPRelaySource *src, DPRelaySink *sink, int num_packets, int inactivity_time)
{
    ASSERT(num_packets > 0)
    
    // allocate structure
    struct DPRelay_flow *flow = (struct DPRelay_flow *)malloc(sizeof(*flow));
    if (!flow) {
        BLog(BLOG_ERROR, "relay flow %d->%d: malloc failed", (int)src->source_id, (int)sink->dest_id);
        goto fail0;
    }
    
    // set src and sink
    flow->src = src;
    flow->sink = sink;
    
    // init DataProtoFlow
    if (!DataProtoFlow_Init(&flow->dp_flow, &src->router->dp_source, src->source_id, sink->dest_id, num_packets, inactivity_time, flow, (DataProtoFlow_handler_inactivity)flow_inactivity_handler)) {
        BLog(BLOG_ERROR, "relay flow %d->%d: DataProtoFlow_Init failed", (int)src->source_id, (int)sink->dest_id);
        goto fail1;
    }
    
    // insert to source list
    LinkedList1_Append(&src->flows_list, &flow->src_list_node);
    
    // insert to sink list
    LinkedList1_Append(&sink->flows_list, &flow->sink_list_node);
    
    // attach flow if needed
    if (sink->dp_sink) {
        DataProtoFlow_Attach(&flow->dp_flow, sink->dp_sink);
    }
    
    BLog(BLOG_INFO, "relay flow %d->%d: created", (int)src->source_id, (int)sink->dest_id);
    
    return flow;
    
fail1:
    free(flow);
fail0:
    return NULL;
}

static void free_flow (struct DPRelay_flow *flow)
{
    // detach flow if needed
    if (flow->sink->dp_sink) {
        DataProtoFlow_Detach(&flow->dp_flow);
    }
    
    // remove posible router reference
    if (flow->src->router->current_flow == flow) {
        flow->src->router->current_flow = NULL;
    }
    
    // remove from sink list
    LinkedList1_Remove(&flow->sink->flows_list, &flow->sink_list_node);
    
    // remove from source list
    LinkedList1_Remove(&flow->src->flows_list, &flow->src_list_node);
    
    // free DataProtoFlow
    DataProtoFlow_Free(&flow->dp_flow);
    
    // free structore
    free(flow);
}

static void flow_inactivity_handler (struct DPRelay_flow *flow)
{
    BLog(BLOG_INFO, "relay flow %d->%d: timed out", (int)flow->src->source_id, (int)flow->sink->dest_id);
    
    free_flow(flow);
}

static struct DPRelay_flow * source_find_flow (DPRelaySource *o, DPRelaySink *sink)
{
    for (LinkedList1Node *node = LinkedList1_GetFirst(&o->flows_list); node; node = LinkedList1Node_Next(node)) {
        struct DPRelay_flow *flow = UPPER_OBJECT(node, struct DPRelay_flow, src_list_node);
        ASSERT(flow->src == o)
        if (flow->sink == sink) {
            return flow;
        }
    }
    
    return NULL;
}

static void router_dp_source_handler (DPRelayRouter *o, const uint8_t *frame, int frame_len)
{
    DebugObject_Access(&o->d_obj);
    
    if (!o->current_flow) {
        return;
    }
    
    // route frame to current flow
    DataProtoFlow_Route(&o->current_flow->dp_flow, 0);
    
    // set no current flow
    o->current_flow = NULL;
}

int DPRelayRouter_Init (DPRelayRouter *o, int frame_mtu, BReactor *reactor)
{
    ASSERT(frame_mtu >= 0)
    ASSERT(frame_mtu <= INT_MAX - DATAPROTO_MAX_OVERHEAD)
    
    // init arguments
    o->frame_mtu = frame_mtu;
    
    // init BufferWriter
    BufferWriter_Init(&o->writer, frame_mtu, BReactor_PendingGroup(reactor));
    
    // init DataProtoSource
    if (!DataProtoSource_Init(&o->dp_source, BufferWriter_GetOutput(&o->writer), (DataProtoSource_handler)router_dp_source_handler, o, reactor)) {
        BLog(BLOG_ERROR, "DataProtoSource_Init failed");
        goto fail1;
    }
    
    // have no current flow
    o->current_flow = NULL;
    
    DebugCounter_Init(&o->d_ctr);
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    BufferWriter_Free(&o->writer);
    return 0;
}

void DPRelayRouter_Free (DPRelayRouter *o)
{
    DebugObject_Free(&o->d_obj);
    DebugCounter_Free(&o->d_ctr);
    ASSERT(!o->current_flow) // have no sources
    
    // free DataProtoSource
    DataProtoSource_Free(&o->dp_source);
    
    // free BufferWriter
    BufferWriter_Free(&o->writer);
}

void DPRelayRouter_SubmitFrame (DPRelayRouter *o, DPRelaySource *src, DPRelaySink *sink, uint8_t *data, int data_len, int num_packets, int inactivity_time)
{
    DebugObject_Access(&o->d_obj);
    DebugObject_Access(&src->d_obj);
    DebugObject_Access(&sink->d_obj);
    ASSERT(!o->current_flow)
    ASSERT(src->router == o)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->frame_mtu)
    ASSERT(num_packets > 0)
    
    // get memory location
    uint8_t *out;
    if (!BufferWriter_StartPacket(&o->writer, &out)) {
        BLog(BLOG_ERROR, "BufferWriter_StartPacket failed for frame %d->%d !?", (int)src->source_id, (int)sink->dest_id);
        return;
    }
    
    // write frame
    memcpy(out, data, data_len);
    
    // submit frame
    BufferWriter_EndPacket(&o->writer, data_len);
    
    // get a flow
    // this comes _after_ writing the packet, in case flow initialization schedules jobs
    struct DPRelay_flow *flow = source_find_flow(src, sink);
    if (!flow) {
        if (!(flow = create_flow(src, sink, num_packets, inactivity_time))) {
            return;
        }
    }
    
    // remember flow so we know where to route the frame in router_dp_source_handler
    o->current_flow = flow;
}

void DPRelaySource_Init (DPRelaySource *o, DPRelayRouter *router, peerid_t source_id, BReactor *reactor)
{
    DebugObject_Access(&router->d_obj);
    
    // init arguments
    o->router = router;
    o->source_id = source_id;
    
    // init flows list
    LinkedList1_Init(&o->flows_list);
    
    DebugCounter_Increment(&o->router->d_ctr);
    DebugObject_Init(&o->d_obj);
}

void DPRelaySource_Free (DPRelaySource *o)
{
    DebugObject_Free(&o->d_obj);
    DebugCounter_Decrement(&o->router->d_ctr);
    
    // free flows, detaching them if needed
    LinkedList1Node *node;
    while (node = LinkedList1_GetFirst(&o->flows_list)) {
        struct DPRelay_flow *flow = UPPER_OBJECT(node, struct DPRelay_flow, src_list_node);
        free_flow(flow);
    }
}

void DPRelaySink_Init (DPRelaySink *o, peerid_t dest_id)
{
    // init arguments
    o->dest_id = dest_id;
    
    // init flows list
    LinkedList1_Init(&o->flows_list);
    
    // have no sink
    o->dp_sink = NULL;
    
    DebugObject_Init(&o->d_obj);
}

void DPRelaySink_Free (DPRelaySink *o)
{
    DebugObject_Free(&o->d_obj);
    ASSERT(!o->dp_sink)
    
    // free flows
    LinkedList1Node *node;
    while (node = LinkedList1_GetFirst(&o->flows_list)) {
        struct DPRelay_flow *flow = UPPER_OBJECT(node, struct DPRelay_flow, sink_list_node);
        free_flow(flow);
    }
}

void DPRelaySink_Attach (DPRelaySink *o, DataProtoSink *dp_sink)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->dp_sink)
    ASSERT(dp_sink)
    
    // attach flows
    for (LinkedList1Node *node = LinkedList1_GetFirst(&o->flows_list); node; node = LinkedList1Node_Next(node)) {
        struct DPRelay_flow *flow = UPPER_OBJECT(node, struct DPRelay_flow, sink_list_node);
        DataProtoFlow_Attach(&flow->dp_flow, dp_sink);
    }
    
    // set sink
    o->dp_sink = dp_sink;
}

void DPRelaySink_Detach (DPRelaySink *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->dp_sink)
    
    // detach flows
    for (LinkedList1Node *node = LinkedList1_GetFirst(&o->flows_list); node; node = LinkedList1Node_Next(node)) {
        struct DPRelay_flow *flow = UPPER_OBJECT(node, struct DPRelay_flow, sink_list_node);
        DataProtoFlow_Detach(&flow->dp_flow);
    }
    
    // set no sink
    o->dp_sink = NULL;
}
