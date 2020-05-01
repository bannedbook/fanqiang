/**
 * @file PacketPassFairQueue.h
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
 * Fair queue using {@link PacketPassInterface}.
 */

#ifndef BADVPN_FLOW_PACKETPASSFAIRQUEUE_H
#define BADVPN_FLOW_PACKETPASSFAIRQUEUE_H

#include <stdint.h>

#include <misc/debug.h>
#include <misc/debugcounter.h>
#include <structure/SAvl.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <base/BPending.h>
#include <flow/PacketPassInterface.h>

// reduce this to test time overflow handling
#define FAIRQUEUE_MAX_TIME UINT64_MAX

typedef void (*PacketPassFairQueue_handler_busy) (void *user);

struct PacketPassFairQueueFlow_s;

#include "PacketPassFairQueue_tree.h"
#include <structure/SAvl_decl.h>

typedef struct PacketPassFairQueueFlow_s {
    struct PacketPassFairQueue_s *m;
    PacketPassFairQueue_handler_busy handler_busy;
    void *user;
    PacketPassInterface input;
    uint64_t time;
    LinkedList1Node list_node;
    int is_queued;
    struct {
        PacketPassFairQueue__TreeNode tree_node;
        uint8_t *data;
        int data_len;
    } queued;
    DebugObject d_obj;
} PacketPassFairQueueFlow;

/**
 * Fair queue using {@link PacketPassInterface}.
 */
typedef struct PacketPassFairQueue_s {
    PacketPassInterface *output;
    BPendingGroup *pg;
    int use_cancel;
    int packet_weight;
    struct PacketPassFairQueueFlow_s *sending_flow;
    int sending_len;
    struct PacketPassFairQueueFlow_s *previous_flow;
    PacketPassFairQueue__Tree queued_tree;
    LinkedList1 flows_list;
    int freeing;
    BPending schedule_job;
    DebugObject d_obj;
    DebugCounter d_ctr;
} PacketPassFairQueue;

/**
 * Initializes the queue.
 *
 * @param m the object
 * @param output output interface
 * @param pg pending group
 * @param use_cancel whether cancel functionality is required. Must be 0 or 1.
 *                   If 1, output must support cancel functionality.
 * @param packet_weight additional weight a packet bears. Must be >0, to keep
 *                      the queue fair for zero size packets.
 * @return 1 on success, 0 on failure (because output MTU is too large)
 */
int PacketPassFairQueue_Init (PacketPassFairQueue *m, PacketPassInterface *output, BPendingGroup *pg, int use_cancel, int packet_weight) WARN_UNUSED;

/**
 * Frees the queue.
 * All flows must have been freed.
 *
 * @param m the object
 */
void PacketPassFairQueue_Free (PacketPassFairQueue *m);

/**
 * Prepares for freeing the entire queue. Must be called to allow freeing
 * the flows in the process of freeing the entire queue.
 * After this function is called, flows and the queue must be freed
 * before any further I/O.
 * May be called multiple times.
 * The queue enters freeing state.
 *
 * @param m the object
 */
void PacketPassFairQueue_PrepareFree (PacketPassFairQueue *m);

/**
 * Returns the MTU of the queue.
 *
 * @param m the object
 */
int PacketPassFairQueue_GetMTU (PacketPassFairQueue *m);

/**
 * Initializes a queue flow.
 * Queue must not be in freeing state.
 * Must not be called from queue calls to output.
 *
 * @param flow the object
 * @param m queue to attach to
 */
void PacketPassFairQueueFlow_Init (PacketPassFairQueueFlow *flow, PacketPassFairQueue *m);

/**
 * Frees a queue flow.
 * Unless the queue is in freeing state:
 * - The flow must not be busy as indicated by {@link PacketPassFairQueueFlow_IsBusy}.
 * - Must not be called from queue calls to output.
 *
 * @param flow the object
 */
void PacketPassFairQueueFlow_Free (PacketPassFairQueueFlow *flow);

/**
 * Does nothing.
 * It must be possible to free the flow (see {@link PacketPassFairQueueFlow_Free}).
 * 
 * @param flow the object
 */
void PacketPassFairQueueFlow_AssertFree (PacketPassFairQueueFlow *flow);

/**
 * Determines if the flow is busy. If the flow is considered busy, it must not
 * be freed. At any given time, at most one flow will be indicated as busy.
 * Queue must not be in freeing state.
 * Must not be called from queue calls to output.
 *
 * @param flow the object
 * @return 0 if not busy, 1 is busy
 */
int PacketPassFairQueueFlow_IsBusy (PacketPassFairQueueFlow *flow);

/**
 * Requests the output to stop processing the current packet as soon as possible.
 * Cancel functionality must be enabled for the queue.
 * The flow must be busy as indicated by {@link PacketPassFairQueueFlow_IsBusy}.
 * Queue must not be in freeing state.
 * 
 * @param flow the object
 */
void PacketPassFairQueueFlow_RequestCancel (PacketPassFairQueueFlow *flow);

/**
 * Sets up a callback to be called when the flow is no longer busy.
 * The handler will be called as soon as the flow is no longer busy, i.e. it is not
 * possible that this flow is no longer busy before the handler is called.
 * The flow must be busy as indicated by {@link PacketPassFairQueueFlow_IsBusy}.
 * Queue must not be in freeing state.
 * Must not be called from queue calls to output.
 *
 * @param flow the object
 * @param handler callback function. NULL to disable.
 * @param user value passed to callback function. Ignored if handler is NULL.
 */
void PacketPassFairQueueFlow_SetBusyHandler (PacketPassFairQueueFlow *flow, PacketPassFairQueue_handler_busy handler, void *user);

/**
 * Returns the input interface of the flow.
 *
 * @param flow the object
 * @return input interface
 */
PacketPassInterface * PacketPassFairQueueFlow_GetInput (PacketPassFairQueueFlow *flow);

#endif
