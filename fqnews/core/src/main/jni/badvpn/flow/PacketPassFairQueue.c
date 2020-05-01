/**
 * @file PacketPassFairQueue.c
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
#include <misc/minmax.h>
#include <misc/compare.h>

#include <flow/PacketPassFairQueue.h>

static int compare_flows (PacketPassFairQueueFlow *f1, PacketPassFairQueueFlow *f2)
{
    int cmp = B_COMPARE(f1->time, f2->time);
    if (cmp) {
        return cmp;
    }
    
    return B_COMPARE((uintptr_t)f1, (uintptr_t)f2);
}

#include "PacketPassFairQueue_tree.h"
#include <structure/SAvl_impl.h>

static uint64_t get_current_time (PacketPassFairQueue *m)
{
    if (m->sending_flow) {
        return m->sending_flow->time;
    }
    
    uint64_t time = 0; // to remove warning
    int have = 0;
    
    PacketPassFairQueueFlow *first_flow = PacketPassFairQueue__Tree_GetFirst(&m->queued_tree, 0);
    if (first_flow) {
        ASSERT(first_flow->is_queued)
        
        time = first_flow->time;
        have = 1;
    }
    
    if (m->previous_flow) {
        if (!have || m->previous_flow->time < time) {
            time = m->previous_flow->time;
            have = 1;
        }
    }
    
    return (have ? time : 0);
}

static void increment_sent_flow (PacketPassFairQueueFlow *flow, uint64_t amount)
{
    PacketPassFairQueue *m = flow->m;
    
    ASSERT(amount <= FAIRQUEUE_MAX_TIME)
    ASSERT(!flow->is_queued)
    ASSERT(!m->sending_flow)
    
    // does time overflow?
    if (amount > FAIRQUEUE_MAX_TIME - flow->time) {
        // get time to subtract
        uint64_t subtract;
        PacketPassFairQueueFlow *first_flow = PacketPassFairQueue__Tree_GetFirst(&m->queued_tree, 0);
        if (!first_flow) {
            subtract = flow->time;
        } else {
            ASSERT(first_flow->is_queued)
            subtract = first_flow->time;
        }
        
        // subtract time from all flows
        for (LinkedList1Node *list_node = LinkedList1_GetFirst(&m->flows_list); list_node; list_node = LinkedList1Node_Next(list_node)) {
            PacketPassFairQueueFlow *someflow = UPPER_OBJECT(list_node, PacketPassFairQueueFlow, list_node);
            
            // don't subtract more time than there is, except for the just finished flow,
            // where we allow time to underflow and then overflow to the correct value after adding to it
            if (subtract > someflow->time && someflow != flow) {
                ASSERT(!someflow->is_queued)
                someflow->time = 0;
            } else {
                someflow->time -= subtract;
            }
        }
    }
    
    // add time to flow
    flow->time += amount;
}

static void schedule (PacketPassFairQueue *m)
{
    ASSERT(!m->sending_flow)
    ASSERT(!m->previous_flow)
    ASSERT(!m->freeing)
    ASSERT(!PacketPassFairQueue__Tree_IsEmpty(&m->queued_tree))
    
    // get first queued flow
    PacketPassFairQueueFlow *qflow = PacketPassFairQueue__Tree_GetFirst(&m->queued_tree, 0);
    ASSERT(qflow->is_queued)
    
    // remove flow from queue
    PacketPassFairQueue__Tree_Remove(&m->queued_tree, 0, qflow);
    qflow->is_queued = 0;
    
    // schedule send
    PacketPassInterface_Sender_Send(m->output, qflow->queued.data, qflow->queued.data_len);
    m->sending_flow = qflow;
    m->sending_len = qflow->queued.data_len;
}

static void schedule_job_handler (PacketPassFairQueue *m)
{
    ASSERT(!m->sending_flow)
    ASSERT(!m->freeing)
    DebugObject_Access(&m->d_obj);
    
    // remove previous flow
    m->previous_flow = NULL;
    
    if (!PacketPassFairQueue__Tree_IsEmpty(&m->queued_tree)) {
        schedule(m);
    }
}

static void input_handler_send (PacketPassFairQueueFlow *flow, uint8_t *data, int data_len)
{
    PacketPassFairQueue *m = flow->m;
    
    ASSERT(flow != m->sending_flow)
    ASSERT(!flow->is_queued)
    ASSERT(!m->freeing)
    DebugObject_Access(&flow->d_obj);
    
    if (flow == m->previous_flow) {
        // remove from previous flow
        m->previous_flow = NULL;
    } else {
        // raise time
        flow->time = bmax_uint64(flow->time, get_current_time(m));
    }
    
    // queue flow
    flow->queued.data = data;
    flow->queued.data_len = data_len;
    int res = PacketPassFairQueue__Tree_Insert(&m->queued_tree, 0, flow, NULL);
    ASSERT_EXECUTE(res)
    flow->is_queued = 1;
    
    if (!m->sending_flow && !BPending_IsSet(&m->schedule_job)) {
        schedule(m);
    }
}

static void output_handler_done (PacketPassFairQueue *m)
{
    ASSERT(m->sending_flow)
    ASSERT(!m->previous_flow)
    ASSERT(!BPending_IsSet(&m->schedule_job))
    ASSERT(!m->freeing)
    ASSERT(!m->sending_flow->is_queued)
    
    PacketPassFairQueueFlow *flow = m->sending_flow;
    
    // sending finished
    m->sending_flow = NULL;
    
    // remember this flow so the schedule job can remove its time if it didn's send
    m->previous_flow = flow;
    
    // update flow time by packet size
    increment_sent_flow(flow, (uint64_t)m->packet_weight + m->sending_len);
    
    // schedule schedule
    BPending_Set(&m->schedule_job);
    
    // finish flow packet
    PacketPassInterface_Done(&flow->input);
    
    // call busy handler if set
    if (flow->handler_busy) {
        // handler is one-shot, unset it before calling
        PacketPassFairQueue_handler_busy handler = flow->handler_busy;
        flow->handler_busy = NULL;
        
        // call handler
        handler(flow->user);
        return;
    }
}

int PacketPassFairQueue_Init (PacketPassFairQueue *m, PacketPassInterface *output, BPendingGroup *pg, int use_cancel, int packet_weight)
{
    ASSERT(packet_weight > 0)
    ASSERT(use_cancel == 0 || use_cancel == 1)
    ASSERT(!use_cancel || PacketPassInterface_HasCancel(output))
    
    // init arguments
    m->output = output;
    m->pg = pg;
    m->use_cancel = use_cancel;
    m->packet_weight = packet_weight;
    
    // make sure that (output MTU + packet_weight <= FAIRQUEUE_MAX_TIME)
    if (!(
        (PacketPassInterface_GetMTU(output) <= FAIRQUEUE_MAX_TIME) &&
        (packet_weight <= FAIRQUEUE_MAX_TIME - PacketPassInterface_GetMTU(output))
    )) {
        goto fail0;
    }
    
    // init output
    PacketPassInterface_Sender_Init(m->output, (PacketPassInterface_handler_done)output_handler_done, m);
    
    // not sending
    m->sending_flow = NULL;
    
    // no previous flow
    m->previous_flow = NULL;
    
    // init queued tree
    PacketPassFairQueue__Tree_Init(&m->queued_tree);
    
    // init flows list
    LinkedList1_Init(&m->flows_list);
    
    // not freeing
    m->freeing = 0;
    
    // init schedule job
    BPending_Init(&m->schedule_job, m->pg, (BPending_handler)schedule_job_handler, m);
    
    DebugObject_Init(&m->d_obj);
    DebugCounter_Init(&m->d_ctr);
    return 1;
    
fail0:
    return 0;
}

void PacketPassFairQueue_Free (PacketPassFairQueue *m)
{
    ASSERT(LinkedList1_IsEmpty(&m->flows_list))
    ASSERT(PacketPassFairQueue__Tree_IsEmpty(&m->queued_tree))
    ASSERT(!m->previous_flow)
    ASSERT(!m->sending_flow)
    DebugCounter_Free(&m->d_ctr);
    DebugObject_Free(&m->d_obj);
    
    // free schedule job
    BPending_Free(&m->schedule_job);
}

void PacketPassFairQueue_PrepareFree (PacketPassFairQueue *m)
{
    DebugObject_Access(&m->d_obj);
    
    // set freeing
    m->freeing = 1;
}

int PacketPassFairQueue_GetMTU (PacketPassFairQueue *m)
{
    DebugObject_Access(&m->d_obj);
    
    return PacketPassInterface_GetMTU(m->output);
}

void PacketPassFairQueueFlow_Init (PacketPassFairQueueFlow *flow, PacketPassFairQueue *m)
{
    ASSERT(!m->freeing)
    DebugObject_Access(&m->d_obj);
    
    // init arguments
    flow->m = m;
    
    // have no canfree handler
    flow->handler_busy = NULL;
    
    // init input
    PacketPassInterface_Init(&flow->input, PacketPassInterface_GetMTU(flow->m->output), (PacketPassInterface_handler_send)input_handler_send, flow, m->pg);
    
    // set time
    flow->time = 0;
    
    // add to flows list
    LinkedList1_Append(&m->flows_list, &flow->list_node);
    
    // is not queued
    flow->is_queued = 0;
    
    DebugObject_Init(&flow->d_obj);
    DebugCounter_Increment(&m->d_ctr);
}

void PacketPassFairQueueFlow_Free (PacketPassFairQueueFlow *flow)
{
    PacketPassFairQueue *m = flow->m;
    
    ASSERT(m->freeing || flow != m->sending_flow)
    DebugCounter_Decrement(&m->d_ctr);
    DebugObject_Free(&flow->d_obj);
    
    // remove from current flow
    if (flow == m->sending_flow) {
        m->sending_flow = NULL;
    }
    
    // remove from previous flow
    if (flow == m->previous_flow) {
        m->previous_flow = NULL;
    }
    
    // remove from queue
    if (flow->is_queued) {
        PacketPassFairQueue__Tree_Remove(&m->queued_tree, 0, flow);
    }
    
    // remove from flows list
    LinkedList1_Remove(&m->flows_list, &flow->list_node);
    
    // free input
    PacketPassInterface_Free(&flow->input);
}

void PacketPassFairQueueFlow_AssertFree (PacketPassFairQueueFlow *flow)
{
    PacketPassFairQueue *m = flow->m;
    B_USE(m)
    
    ASSERT(m->freeing || flow != m->sending_flow)
    DebugObject_Access(&flow->d_obj);
}

int PacketPassFairQueueFlow_IsBusy (PacketPassFairQueueFlow *flow)
{
    PacketPassFairQueue *m = flow->m;
    
    ASSERT(!m->freeing)
    DebugObject_Access(&flow->d_obj);
    
    return (flow == m->sending_flow);
}

void PacketPassFairQueueFlow_RequestCancel (PacketPassFairQueueFlow *flow)
{
    PacketPassFairQueue *m = flow->m;
    
    ASSERT(flow == m->sending_flow)
    ASSERT(m->use_cancel)
    ASSERT(!m->freeing)
    ASSERT(!BPending_IsSet(&m->schedule_job))
    DebugObject_Access(&flow->d_obj);
    
    // request cancel
    PacketPassInterface_Sender_RequestCancel(m->output);
}

void PacketPassFairQueueFlow_SetBusyHandler (PacketPassFairQueueFlow *flow, PacketPassFairQueue_handler_busy handler, void *user)
{
    PacketPassFairQueue *m = flow->m;
    B_USE(m)
    
    ASSERT(flow == m->sending_flow)
    ASSERT(!m->freeing)
    DebugObject_Access(&flow->d_obj);
    
    // set handler
    flow->handler_busy = handler;
    flow->user = user;
}

PacketPassInterface * PacketPassFairQueueFlow_GetInput (PacketPassFairQueueFlow *flow)
{
    DebugObject_Access(&flow->d_obj);
    
    return &flow->input;
}
