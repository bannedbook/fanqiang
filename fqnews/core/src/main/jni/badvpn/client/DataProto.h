/**
 * @file DataProto.h
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
 * Mudule for frame sending used in the VPN client program.
 */

#ifndef BADVPN_CLIENT_DATAPROTO_H
#define BADVPN_CLIENT_DATAPROTO_H

#include <stdint.h>

#include <misc/debugcounter.h>
#include <misc/debug.h>
#include <base/DebugObject.h>
#include <system/BReactor.h>
#include <flow/PacketPassFairQueue.h>
#include <flow/PacketPassNotifier.h>
#include <flow/PacketRecvBlocker.h>
#include <flow/SinglePacketBuffer.h>
#include <flow/PacketPassConnector.h>
#include <flow/PacketRouter.h>
#include <flowextra/PacketPassInactivityMonitor.h>
#include <client/DataProtoKeepaliveSource.h>

typedef void (*DataProtoSink_handler) (void *user, int up);
typedef void (*DataProtoSource_handler) (void *user, const uint8_t *frame, int frame_len);
typedef void (*DataProtoFlow_handler_inactivity) (void *user);

struct DataProtoFlow_buffer;

/**
 * Frame destination.
 * Represents a peer as a destination for sending frames to.
 */
typedef struct {
    BReactor *reactor;
    int frame_mtu;
    PacketPassFairQueue queue;
    PacketPassInactivityMonitor monitor;
    PacketPassNotifier notifier;
    DataProtoKeepaliveSource ka_source;
    PacketRecvBlocker ka_blocker;
    SinglePacketBuffer ka_buffer;
    PacketPassFairQueueFlow ka_qflow;
    BTimer receive_timer;
    int up;
    int up_report;
    DataProtoSink_handler handler;
    void *user;
    BPending up_job;
    struct DataProtoFlow_buffer *detaching_buffer;
    DebugObject d_obj;
    DebugCounter d_ctr;
} DataProtoSink;

/**
 * Receives frames from a {@link PacketRecvInterface} input and
 * allows the user to route them to buffers in {@link DataProtoFlow}'s.
 */
typedef struct {
    DataProtoSource_handler handler;
    void *user;
    BReactor *reactor;
    int frame_mtu;
    PacketRouter router;
    uint8_t *current_buf;
    int current_recv_len;
    DebugObject d_obj;
    DebugCounter d_ctr;
} DataProtoSource;

/**
 * Contains a buffer for frames from a specific peer to a specific peer.
 * Receives frames from a {@link DataProtoSource} as routed by the user.
 * Can be attached to a {@link DataProtoSink} to send out frames.
 */
typedef struct {
    DataProtoSource *source;
    peerid_t source_id;
    peerid_t dest_id;
    DataProtoSink *sink_desired;
    struct DataProtoFlow_buffer *b;
    DebugObject d_obj;
} DataProtoFlow;

struct DataProtoFlow_buffer {
    DataProtoFlow *flow;
    int inactivity_time;
    RouteBuffer rbuf;
    PacketPassInactivityMonitor monitor;
    PacketPassConnector connector;
    DataProtoSink *sink;
    PacketPassFairQueueFlow sink_qflow;
};

/**
 * Initializes the sink.
 * 
 * @param o the object
 * @param reactor reactor we live in
 * @param output output interface. Must support cancel functionality. Its MTU must be
 *               >=DATAPROTO_MAX_OVERHEAD.
 * @param keepalive_time keepalive time
 * @param tolerance_time after how long of not having received anything from the peer
 *                       to consider the link down
 * @param handler up state handler
 * @param user value to pass to handler
 * @return 1 on success, 0 on failure
 */
int DataProtoSink_Init (DataProtoSink *o, BReactor *reactor, PacketPassInterface *output, btime_t keepalive_time, btime_t tolerance_time, DataProtoSink_handler handler, void *user) WARN_UNUSED;

/**
 * Frees the sink.
 * There must be no local sources attached.
 * 
 * @param o the object
 */
void DataProtoSink_Free (DataProtoSink *o);

/**
 * Notifies the sink that a packet was received from the peer.
 * Must not be in freeing state.
 * 
 * @param o the object
 * @param peer_receiving whether the DATAPROTO_FLAGS_RECEIVING_KEEPALIVES flag was set in the packet.
 *                       Must be 0 or 1.
 */
void DataProtoSink_Received (DataProtoSink *o, int peer_receiving);

/**
 * Initiazes the source.
 * 
 * @param o the object
 * @param input frame input. Its input MTU must be <= INT_MAX - DATAPROTO_MAX_OVERHEAD.
 * @param handler handler called when a frame arrives to allow the user to route it to
 *                appropriate {@link DataProtoFlow}'s.
 * @param user value passed to handler
 * @param reactor reactor we live in
 * @return 1 on success, 0 on failure
 */
int DataProtoSource_Init (DataProtoSource *o, PacketRecvInterface *input, DataProtoSource_handler handler, void *user, BReactor *reactor) WARN_UNUSED;

/**
 * Frees the source.
 * There must be no {@link DataProtoFlow}'s using this source.
 * 
 * @param o the object
 */
void DataProtoSource_Free (DataProtoSource *o);

/**
 * Initializes the flow.
 * The flow is initialized in not attached state.
 * 
 * @param o the object
 * @param source source to receive frames from
 * @param source_id source peer ID to encode in the headers (i.e. our ID)
 * @param dest_id destination peer ID to encode in the headers (i.e. ID if the peer this
 *                flow belongs to)
 * @param num_packets number of packets the buffer should hold. Must be >0.
 * @param inactivity_time milliseconds of output inactivity after which to call the
 *                        inactivity handler; <0 to disable. Note that the flow is considered
 *                        active as long as its buffer is non-empty, even if is not attached to
 *                        a {@link DataProtoSink}.
 * @param user value to pass to handler
 * @param handler_inactivity inactivity handler, if inactivity_time >=0
 * @return 1 on success, 0 on failure
 */
int DataProtoFlow_Init (DataProtoFlow *o, DataProtoSource *source, peerid_t source_id, peerid_t dest_id, int num_packets, int inactivity_time, void *user,
                        DataProtoFlow_handler_inactivity handler_inactivity) WARN_UNUSED;

/**
 * Frees the flow.
 * The flow must be in not attached state.
 * 
 * @param o the object
 */
void DataProtoFlow_Free (DataProtoFlow *o);

/**
 * Routes a frame from the flow's source to this flow.
 * Must be called from within the job context of the {@link DataProtoSource_handler} handler.
 * Must not be called after this has been called with more=0 for the current frame.
 * 
 * @param o the object
 * @param more whether the current frame may have to be routed to more
 *             flows. If 0, must not be called again until the handler is
 *             called for the next frame. Must be 0 or 1.
 */
void DataProtoFlow_Route (DataProtoFlow *o, int more);

/**
 * Attaches the flow to a sink.
 * The flow must be in not attached state.
 * 
 * @param o the object
 * @param sink sink to attach to. This flow's frame_mtu must be <=
 *             (output MTU of sink) - DATAPROTO_MAX_OVERHEAD.
 */
void DataProtoFlow_Attach (DataProtoFlow *o, DataProtoSink *sink);

/**
 * Detaches the flow from a destination.
 * The flow must be in attached state.
 * 
 * @param o the object
 */
void DataProtoFlow_Detach (DataProtoFlow *o);

#endif
