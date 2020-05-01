/**
 * @file PacketPassPriorityQueue.c
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

#include <misc/debug.h>
#include <misc/offset.h>
#include <misc/compare.h>

#include <flow/PacketPassPriorityQueue.h>

static int compare_flows (PacketPassPriorityQueueFlow *f1, PacketPassPriorityQueueFlow *f2)
{
    int cmp = B_COMPARE(f1->priority, f2->priority);
    if (cmp) {
        return cmp;
    }
    
    return B_COMPARE((uintptr_t)f1, (uintptr_t)f2);
}

#include "PacketPassPriorityQueue_tree.h"
#include <structure/SAvl_impl.h>

static void schedule (PacketPassPriorityQueue *m)
{
    ASSERT(!m->sending_flow)
    ASSERT(!m->freeing)
    ASSERT(!PacketPassPriorityQueue__Tree_IsEmpty(&m->queued_tree))
    
    // get first queued flow
    PacketPassPriorityQueueFlow *qflow = PacketPassPriorityQueue__Tree_GetFirst(&m->queued_tree, 0);
    ASSERT(qflow->is_queued)
    
    // remove flow from queue
    PacketPassPriorityQueue__Tree_Remove(&m->queued_tree, 0, qflow);
    qflow->is_queued = 0;
    
    // schedule send
    PacketPassInterface_Sender_Send(m->output, qflow->queued.data, qflow->queued.data_len);
    m->sending_flow = qflow;
}

static void schedule_job_handler (PacketPassPriorityQueue *m)
{
    ASSERT(!m->sending_flow)
    ASSERT(!m->freeing)
    DebugObject_Access(&m->d_obj);
    
    if (!PacketPassPriorityQueue__Tree_IsEmpty(&m->queued_tree)) {
        schedule(m);
    }
}

static void input_handler_send (PacketPassPriorityQueueFlow *flow, uint8_t *data, int data_len)
{
    PacketPassPriorityQueue *m = flow->m;
    
    ASSERT(flow != m->sending_flow)
    ASSERT(!flow->is_queued)
    ASSERT(!m->freeing)
    DebugObject_Access(&flow->d_obj);
    
    // queue flow
    flow->queued.data = data;
    flow->queued.data_len = data_len;
    int res = PacketPassPriorityQueue__Tree_Insert(&m->queued_tree, 0, flow, NULL);
    ASSERT_EXECUTE(res)
    flow->is_queued = 1;
    
    if (!m->sending_flow && !BPending_IsSet(&m->schedule_job)) {
        schedule(m);
    }
}

static void output_handler_done (PacketPassPriorityQueue *m)
{
    ASSERT(m->sending_flow)
    ASSERT(!BPending_IsSet(&m->schedule_job))
    ASSERT(!m->freeing)
    ASSERT(!m->sending_flow->is_queued)
    
    PacketPassPriorityQueueFlow *flow = m->sending_flow;
    
    // sending finished
    m->sending_flow = NULL;
    
    // schedule schedule
    BPending_Set(&m->schedule_job);
    
    // finish flow packet
    PacketPassInterface_Done(&flow->input);
    
    // call busy handler if set
    if (flow->handler_busy) {
        // handler is one-shot, unset it before calling
        PacketPassPriorityQueue_handler_busy handler = flow->handler_busy;
        flow->handler_busy = NULL;
        
        // call handler
        handler(flow->user);
        return;
    }
}

void PacketPassPriorityQueue_Init (PacketPassPriorityQueue *m, PacketPassInterface *output, BPendingGroup *pg, int use_cancel)
{
    ASSERT(use_cancel == 0 || use_cancel == 1)
    ASSERT(!use_cancel || PacketPassInterface_HasCancel(output))
    
    // init arguments
    m->output = output;
    m->pg = pg;
    m->use_cancel = use_cancel;
    
    // init output
    PacketPassInterface_Sender_Init(m->output, (PacketPassInterface_handler_done)output_handler_done, m);
    
    // not sending
    m->sending_flow = NULL;
    
    // init queued tree
    PacketPassPriorityQueue__Tree_Init(&m->queued_tree);
    
    // not freeing
    m->freeing = 0;
    
    // init schedule job
    BPending_Init(&m->schedule_job, m->pg, (BPending_handler)schedule_job_handler, m);
    
    DebugObject_Init(&m->d_obj);
    DebugCounter_Init(&m->d_ctr);
}

void PacketPassPriorityQueue_Free (PacketPassPriorityQueue *m)
{
    ASSERT(PacketPassPriorityQueue__Tree_IsEmpty(&m->queued_tree))
    ASSERT(!m->sending_flow)
    DebugCounter_Free(&m->d_ctr);
    DebugObject_Free(&m->d_obj);
    
    // free schedule job
    BPending_Free(&m->schedule_job);
}

void PacketPassPriorityQueue_PrepareFree (PacketPassPriorityQueue *m)
{
    DebugObject_Access(&m->d_obj);
    
    // set freeing
    m->freeing = 1;
}

int PacketPassPriorityQueue_GetMTU (PacketPassPriorityQueue *m)
{
    DebugObject_Access(&m->d_obj);
    
    return PacketPassInterface_GetMTU(m->output);
}

void PacketPassPriorityQueueFlow_Init (PacketPassPriorityQueueFlow *flow, PacketPassPriorityQueue *m, int priority)
{
    ASSERT(!m->freeing)
    DebugObject_Access(&m->d_obj);
    
    // init arguments
    flow->m = m;
    flow->priority = priority;
    
    // have no canfree handler
    flow->handler_busy = NULL;
    
    // init input
    PacketPassInterface_Init(&flow->input, PacketPassInterface_GetMTU(flow->m->output), (PacketPassInterface_handler_send)input_handler_send, flow, m->pg);
    
    // is not queued
    flow->is_queued = 0;
    
    DebugObject_Init(&flow->d_obj);
    DebugCounter_Increment(&m->d_ctr);
}

void PacketPassPriorityQueueFlow_Free (PacketPassPriorityQueueFlow *flow)
{
    PacketPassPriorityQueue *m = flow->m;
    
    ASSERT(m->freeing || flow != m->sending_flow)
    DebugCounter_Decrement(&m->d_ctr);
    DebugObject_Free(&flow->d_obj);
    
    // remove from current flow
    if (flow == m->sending_flow) {
        m->sending_flow = NULL;
    }
    
    // remove from queue
    if (flow->is_queued) {
        PacketPassPriorityQueue__Tree_Remove(&m->queued_tree, 0, flow);
    }
    
    // free input
    PacketPassInterface_Free(&flow->input);
}

void PacketPassPriorityQueueFlow_AssertFree (PacketPassPriorityQueueFlow *flow)
{
    PacketPassPriorityQueue *m = flow->m;
    B_USE(m)
    
    ASSERT(m->freeing || flow != m->sending_flow)
    DebugObject_Access(&flow->d_obj);
}

int PacketPassPriorityQueueFlow_IsBusy (PacketPassPriorityQueueFlow *flow)
{
    PacketPassPriorityQueue *m = flow->m;
    
    ASSERT(!m->freeing)
    DebugObject_Access(&flow->d_obj);
    
    return (flow == m->sending_flow);
}

void PacketPassPriorityQueueFlow_RequestCancel (PacketPassPriorityQueueFlow *flow)
{
    PacketPassPriorityQueue *m = flow->m;
    
    ASSERT(flow == m->sending_flow)
    ASSERT(m->use_cancel)
    ASSERT(!m->freeing)
    ASSERT(!BPending_IsSet(&m->schedule_job))
    DebugObject_Access(&flow->d_obj);
    
    // request cancel
    PacketPassInterface_Sender_RequestCancel(m->output);
}

void PacketPassPriorityQueueFlow_SetBusyHandler (PacketPassPriorityQueueFlow *flow, PacketPassPriorityQueue_handler_busy handler, void *user)
{
    PacketPassPriorityQueue *m = flow->m;
    B_USE(m)
    
    ASSERT(flow == m->sending_flow)
    ASSERT(!m->freeing)
    DebugObject_Access(&flow->d_obj);
    
    // set handler
    flow->handler_busy = handler;
    flow->user = user;
}

PacketPassInterface * PacketPassPriorityQueueFlow_GetInput (PacketPassPriorityQueueFlow *flow)
{
    DebugObject_Access(&flow->d_obj);
    
    return &flow->input;
}
