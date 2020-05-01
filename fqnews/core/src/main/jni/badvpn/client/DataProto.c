/**
 * @file DataProto.c
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
#include <limits.h>

#include <protocol/dataproto.h>
#include <misc/byteorder.h>
#include <base/BLog.h>

#include <client/DataProto.h>

#include <generated/blog_channel_DataProto.h>

static void monitor_handler (DataProtoSink *o);
static void refresh_up_job (DataProtoSink *o);
static void receive_timer_handler (DataProtoSink *o);
static void notifier_handler (DataProtoSink *o, uint8_t *data, int data_len);
static void up_job_handler (DataProtoSink *o);
static void flow_buffer_free (struct DataProtoFlow_buffer *b);
static void flow_buffer_attach (struct DataProtoFlow_buffer *b, DataProtoSink *sink);
static void flow_buffer_detach (struct DataProtoFlow_buffer *b);
static void flow_buffer_schedule_detach (struct DataProtoFlow_buffer *b);
static void flow_buffer_finish_detach (struct DataProtoFlow_buffer *b);
static void flow_buffer_qflow_handler_busy (struct DataProtoFlow_buffer *b);

void monitor_handler (DataProtoSink *o)
{
    DebugObject_Access(&o->d_obj);
    
    // send keep-alive
    PacketRecvBlocker_AllowBlockedPacket(&o->ka_blocker);
}

void refresh_up_job (DataProtoSink *o)
{
    if (o->up != o->up_report) {
        BPending_Set(&o->up_job);
    } else {
        BPending_Unset(&o->up_job);
    }
}

void receive_timer_handler (DataProtoSink *o)
{
    DebugObject_Access(&o->d_obj);
    
    // consider down
    o->up = 0;
    
    refresh_up_job(o);
}

void notifier_handler (DataProtoSink *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(data_len >= sizeof(struct dataproto_header))
    
    int flags = 0;
    
    // if we are receiving keepalives, set the flag
    if (BTimer_IsRunning(&o->receive_timer)) {
        flags |= DATAPROTO_FLAGS_RECEIVING_KEEPALIVES;
    }
    
    // modify existing packet here
    struct dataproto_header header;
    memcpy(&header, data, sizeof(header));
    header.flags = hton8(flags);
    memcpy(data, &header, sizeof(header));
}

void up_job_handler (DataProtoSink *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->up != o->up_report)
    
    o->up_report = o->up;
    
    o->handler(o->user, o->up);
    return;
}

void source_router_handler (DataProtoSource *o, uint8_t *buf, int recv_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(buf)
    ASSERT(recv_len >= 0)
    ASSERT(recv_len <= o->frame_mtu)
    
    // remember packet
    o->current_buf = buf;
    o->current_recv_len = recv_len;
    
    // call handler
    o->handler(o->user, buf + DATAPROTO_MAX_OVERHEAD, recv_len);
    return;
}

void flow_buffer_free (struct DataProtoFlow_buffer *b)
{
    ASSERT(!b->sink)
    
    // free route buffer
    RouteBuffer_Free(&b->rbuf);
    
    // free inactivity monitor
    if (b->inactivity_time >= 0) {
        PacketPassInactivityMonitor_Free(&b->monitor);
    }
    
    // free connector
    PacketPassConnector_Free(&b->connector);
    
    // free buffer structure
    free(b);
}

void flow_buffer_attach (struct DataProtoFlow_buffer *b, DataProtoSink *sink)
{
    ASSERT(!b->sink)
    
    // init queue flow
    PacketPassFairQueueFlow_Init(&b->sink_qflow, &sink->queue);
    
    // connect to queue flow
    PacketPassConnector_ConnectOutput(&b->connector, PacketPassFairQueueFlow_GetInput(&b->sink_qflow));
    
    // set DataProto
    b->sink = sink;
}

void flow_buffer_detach (struct DataProtoFlow_buffer *b)
{
    ASSERT(b->sink)
    PacketPassFairQueueFlow_AssertFree(&b->sink_qflow);
    
    // disconnect from queue flow
    PacketPassConnector_DisconnectOutput(&b->connector);
    
    // free queue flow
    PacketPassFairQueueFlow_Free(&b->sink_qflow);
    
    // clear reference to this buffer in the sink
    if (b->sink->detaching_buffer == b) {
        b->sink->detaching_buffer = NULL;
    }
    
    // set no DataProto
    b->sink = NULL;
}

void flow_buffer_schedule_detach (struct DataProtoFlow_buffer *b)
{
    ASSERT(b->sink)
    ASSERT(PacketPassFairQueueFlow_IsBusy(&b->sink_qflow))
    ASSERT(!b->sink->detaching_buffer || b->sink->detaching_buffer == b)
    
    if (b->sink->detaching_buffer == b) {
        return;
    }
    
    // request cancel
    PacketPassFairQueueFlow_RequestCancel(&b->sink_qflow);
    
    // set busy handler
    PacketPassFairQueueFlow_SetBusyHandler(&b->sink_qflow, (PacketPassFairQueue_handler_busy)flow_buffer_qflow_handler_busy, b);
    
    // remember this buffer in the sink so it can handle us if it goes away
    b->sink->detaching_buffer = b;
}

void flow_buffer_finish_detach (struct DataProtoFlow_buffer *b)
{
    ASSERT(b->sink)
    ASSERT(b->sink->detaching_buffer == b)
    PacketPassFairQueueFlow_AssertFree(&b->sink_qflow);
    
    // detach
    flow_buffer_detach(b);
    
    if (!b->flow) {
        // free
        flow_buffer_free(b);
    } else if (b->flow->sink_desired) {
        // attach
        flow_buffer_attach(b, b->flow->sink_desired);
    }
}

void flow_buffer_qflow_handler_busy (struct DataProtoFlow_buffer *b)
{
    ASSERT(b->sink)
    ASSERT(b->sink->detaching_buffer == b)
    PacketPassFairQueueFlow_AssertFree(&b->sink_qflow);
    
    flow_buffer_finish_detach(b);
}

int DataProtoSink_Init (DataProtoSink *o, BReactor *reactor, PacketPassInterface *output, btime_t keepalive_time, btime_t tolerance_time, DataProtoSink_handler handler, void *user)
{
    ASSERT(PacketPassInterface_HasCancel(output))
    ASSERT(PacketPassInterface_GetMTU(output) >= DATAPROTO_MAX_OVERHEAD)
    
    // init arguments
    o->reactor = reactor;
    o->handler = handler;
    o->user = user;
    
    // set frame MTU
    o->frame_mtu = PacketPassInterface_GetMTU(output) - DATAPROTO_MAX_OVERHEAD;
    
    // init notifier
    PacketPassNotifier_Init(&o->notifier, output, BReactor_PendingGroup(o->reactor));
    PacketPassNotifier_SetHandler(&o->notifier, (PacketPassNotifier_handler_notify)notifier_handler, o);
    
    // init monitor
    PacketPassInactivityMonitor_Init(&o->monitor, PacketPassNotifier_GetInput(&o->notifier), o->reactor, keepalive_time, (PacketPassInactivityMonitor_handler)monitor_handler, o);
    PacketPassInactivityMonitor_Force(&o->monitor);
    
    // init queue
    if (!PacketPassFairQueue_Init(&o->queue, PacketPassInactivityMonitor_GetInput(&o->monitor), BReactor_PendingGroup(o->reactor), 1, 1)) {
        BLog(BLOG_ERROR, "PacketPassFairQueue_Init failed");
        goto fail1;
    }
    
    // init keepalive queue flow
    PacketPassFairQueueFlow_Init(&o->ka_qflow, &o->queue);
    
    // init keepalive source
    DataProtoKeepaliveSource_Init(&o->ka_source, BReactor_PendingGroup(o->reactor));
    
    // init keepalive blocker
    PacketRecvBlocker_Init(&o->ka_blocker, DataProtoKeepaliveSource_GetOutput(&o->ka_source), BReactor_PendingGroup(o->reactor));
    
    // init keepalive buffer
    if (!SinglePacketBuffer_Init(&o->ka_buffer, PacketRecvBlocker_GetOutput(&o->ka_blocker), PacketPassFairQueueFlow_GetInput(&o->ka_qflow), BReactor_PendingGroup(o->reactor))) {
        BLog(BLOG_ERROR, "SinglePacketBuffer_Init failed");
        goto fail2;
    }
    
    // init receive timer
    BTimer_Init(&o->receive_timer, tolerance_time, (BTimer_handler)receive_timer_handler, o);
    
    // init handler job
    BPending_Init(&o->up_job, BReactor_PendingGroup(o->reactor), (BPending_handler)up_job_handler, o);
    
    // set not up
    o->up = 0;
    o->up_report = 0;
    
    // set no detaching buffer
    o->detaching_buffer = NULL;
    
    DebugCounter_Init(&o->d_ctr);
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail2:
    PacketRecvBlocker_Free(&o->ka_blocker);
    DataProtoKeepaliveSource_Free(&o->ka_source);
    PacketPassFairQueueFlow_Free(&o->ka_qflow);
    PacketPassFairQueue_Free(&o->queue);
fail1:
    PacketPassInactivityMonitor_Free(&o->monitor);
    PacketPassNotifier_Free(&o->notifier);
    return 0;
}

void DataProtoSink_Free (DataProtoSink *o)
{
    DebugObject_Free(&o->d_obj);
    DebugCounter_Free(&o->d_ctr);
    
    // allow freeing queue flows
    PacketPassFairQueue_PrepareFree(&o->queue);
    
    // release detaching buffer
    if (o->detaching_buffer) {
        ASSERT(!o->detaching_buffer->flow || o->detaching_buffer->flow->sink_desired != o)
        flow_buffer_finish_detach(o->detaching_buffer);
    }
    
    // free handler job
    BPending_Free(&o->up_job);
    
    // free receive timer
    BReactor_RemoveTimer(o->reactor, &o->receive_timer);
    
    // free keepalive buffer
    SinglePacketBuffer_Free(&o->ka_buffer);
    
    // free keepalive blocker
    PacketRecvBlocker_Free(&o->ka_blocker);
    
    // free keepalive source
    DataProtoKeepaliveSource_Free(&o->ka_source);
    
    // free keepalive queue flow
    PacketPassFairQueueFlow_Free(&o->ka_qflow);
    
    // free queue
    PacketPassFairQueue_Free(&o->queue);
    
    // free monitor
    PacketPassInactivityMonitor_Free(&o->monitor);
    
    // free notifier
    PacketPassNotifier_Free(&o->notifier);
}

void DataProtoSink_Received (DataProtoSink *o, int peer_receiving)
{
    ASSERT(peer_receiving == 0 || peer_receiving == 1)
    DebugObject_Access(&o->d_obj);
    
    // reset receive timer
    BReactor_SetTimer(o->reactor, &o->receive_timer);
    
    if (!peer_receiving) {
        // peer reports not receiving, consider down
        o->up = 0;
        // send keep-alive to converge faster
        PacketRecvBlocker_AllowBlockedPacket(&o->ka_blocker);
    } else {
        // consider up
        o->up = 1;
    }
    
    refresh_up_job(o);
}

int DataProtoSource_Init (DataProtoSource *o, PacketRecvInterface *input, DataProtoSource_handler handler, void *user, BReactor *reactor)
{
    ASSERT(PacketRecvInterface_GetMTU(input) <= INT_MAX - DATAPROTO_MAX_OVERHEAD)
    ASSERT(handler)
    
    // init arguments
    o->handler = handler;
    o->user = user;
    o->reactor = reactor;
    
    // remember frame MTU
    o->frame_mtu = PacketRecvInterface_GetMTU(input);
    
    // init router
    if (!PacketRouter_Init(&o->router, DATAPROTO_MAX_OVERHEAD + o->frame_mtu, DATAPROTO_MAX_OVERHEAD, input, (PacketRouter_handler)source_router_handler, o, BReactor_PendingGroup(reactor))) {
        BLog(BLOG_ERROR, "PacketRouter_Init failed");
        goto fail0;
    }
    
    DebugCounter_Init(&o->d_ctr);
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail0:
    return 0;
}

void DataProtoSource_Free (DataProtoSource *o)
{
    DebugObject_Free(&o->d_obj);
    DebugCounter_Free(&o->d_ctr);
    
    // free router
    PacketRouter_Free(&o->router);
}

int DataProtoFlow_Init (DataProtoFlow *o, DataProtoSource *source, peerid_t source_id, peerid_t dest_id, int num_packets, int inactivity_time, void *user,
                        DataProtoFlow_handler_inactivity handler_inactivity)
{
    DebugObject_Access(&source->d_obj);
    ASSERT(num_packets > 0)
    ASSERT(!(inactivity_time >= 0) || handler_inactivity)
    
    // init arguments
    o->source = source;
    o->source_id = source_id;
    o->dest_id = dest_id;
    
    // set no desired sink
    o->sink_desired = NULL;
    
    // allocate buffer structure
    struct DataProtoFlow_buffer *b = (struct DataProtoFlow_buffer *)malloc(sizeof(*b));
    if (!b) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    o->b = b;
    
    // set parent
    b->flow = o;
    
    // remember inactivity time
    b->inactivity_time = inactivity_time;
    
    // init connector
    PacketPassConnector_Init(&b->connector, DATAPROTO_MAX_OVERHEAD + source->frame_mtu, BReactor_PendingGroup(source->reactor));
    
    // init inactivity monitor
    PacketPassInterface *buf_out = PacketPassConnector_GetInput(&b->connector);
    if (b->inactivity_time >= 0) {
        PacketPassInactivityMonitor_Init(&b->monitor, buf_out, source->reactor, b->inactivity_time, handler_inactivity, user);
        buf_out = PacketPassInactivityMonitor_GetInput(&b->monitor);
    }
    
    // init route buffer
    if (!RouteBuffer_Init(&b->rbuf, DATAPROTO_MAX_OVERHEAD + source->frame_mtu, buf_out, num_packets)) {
        BLog(BLOG_ERROR, "RouteBuffer_Init failed");
        goto fail1;
    }
    
    // set no sink
    b->sink = NULL;
    
    DebugCounter_Increment(&source->d_ctr);
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    if (b->inactivity_time >= 0) {
        PacketPassInactivityMonitor_Free(&b->monitor);
    }
    PacketPassConnector_Free(&b->connector);
    free(b);
fail0:
    return 0;
}

void DataProtoFlow_Free (DataProtoFlow *o)
{
    DebugObject_Free(&o->d_obj);
    DebugCounter_Decrement(&o->source->d_ctr);
    ASSERT(!o->sink_desired)
    struct DataProtoFlow_buffer *b = o->b;
    
    if (b->sink) {
        if (PacketPassFairQueueFlow_IsBusy(&b->sink_qflow)) {
            // schedule detach, free buffer after detach
            flow_buffer_schedule_detach(b);
            b->flow = NULL;
            
            // remove inactivity handler
            if (b->inactivity_time >= 0) {
                PacketPassInactivityMonitor_SetHandler(&b->monitor, NULL, NULL);
            }
        } else {
            // detach and free buffer now
            flow_buffer_detach(b);
            flow_buffer_free(b);
        }
    } else {
        // free buffer
        flow_buffer_free(b);
    }
}

void DataProtoFlow_Route (DataProtoFlow *o, int more)
{
    DebugObject_Access(&o->d_obj);
    PacketRouter_AssertRoute(&o->source->router);
    ASSERT(o->source->current_buf)
    ASSERT(more == 0 || more == 1)
    struct DataProtoFlow_buffer *b = o->b;
    
    // write header. Don't set flags, it will be set in notifier_handler.
    struct dataproto_header header;
    struct dataproto_peer_id id;
    header.from_id = htol16(o->source_id);
    header.num_peer_ids = htol16(1);
    id.id = htol16(o->dest_id);
    memcpy(o->source->current_buf, &header, sizeof(header));
    memcpy(o->source->current_buf + sizeof(header), &id, sizeof(id));
    
    // route
    uint8_t *next_buf;
    if (!PacketRouter_Route(&o->source->router, DATAPROTO_MAX_OVERHEAD + o->source->current_recv_len, &b->rbuf,
                            &next_buf, DATAPROTO_MAX_OVERHEAD, (more ? o->source->current_recv_len : 0)
    )) {
        BLog(BLOG_NOTICE, "buffer full: %d->%d", (int)o->source_id, (int)o->dest_id);
        return;
    }
    
    // remember next buffer, or don't allow further routing if more==0
    o->source->current_buf = (more ? next_buf : NULL);
}

void DataProtoFlow_Attach (DataProtoFlow *o, DataProtoSink *sink)
{
    DebugObject_Access(&o->d_obj);
    DebugObject_Access(&sink->d_obj);
    ASSERT(!o->sink_desired)
    ASSERT(sink)
    ASSERT(o->source->frame_mtu <= sink->frame_mtu)
    struct DataProtoFlow_buffer *b = o->b;
    
    if (b->sink) {
        if (PacketPassFairQueueFlow_IsBusy(&b->sink_qflow)) {
            // schedule detach and reattach
            flow_buffer_schedule_detach(b);
        } else {
            // detach and reattach now
            flow_buffer_detach(b);
            flow_buffer_attach(b, sink);
        }
    } else {
        // attach
        flow_buffer_attach(b, sink);
    }
    
    // set desired sink
    o->sink_desired = sink;
    
    DebugCounter_Increment(&sink->d_ctr);
}

void DataProtoFlow_Detach (DataProtoFlow *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->sink_desired)
    struct DataProtoFlow_buffer *b = o->b;
    ASSERT(b->sink)
    
    DataProtoSink *sink = o->sink_desired;
    
    if (PacketPassFairQueueFlow_IsBusy(&b->sink_qflow)) {
        // schedule detach
        flow_buffer_schedule_detach(b);
    } else {
        // detach now
        flow_buffer_detach(b);
    }
    
    // set no desired sink
    o->sink_desired = NULL;
    
    DebugCounter_Decrement(&sink->d_ctr);
}
