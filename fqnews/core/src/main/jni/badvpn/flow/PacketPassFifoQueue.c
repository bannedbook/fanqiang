/**
 * @file PacketPassFifoQueue.c
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

#include "PacketPassFifoQueue.h"

static void schedule (PacketPassFifoQueue *o)
{
    ASSERT(!o->freeing)
    ASSERT(!o->sending_flow)
    ASSERT(!LinkedList1_IsEmpty(&o->waiting_flows_list))
    ASSERT(!BPending_IsSet(&o->schedule_job))
    
    // get first waiting flow
    PacketPassFifoQueueFlow *flow = UPPER_OBJECT(LinkedList1_GetFirst(&o->waiting_flows_list), PacketPassFifoQueueFlow, waiting_flows_list_node);
    ASSERT(flow->queue == o)
    ASSERT(flow->is_waiting)
    
    // remove it from queue
    LinkedList1_Remove(&o->waiting_flows_list, &flow->waiting_flows_list_node);
    flow->is_waiting = 0;
    
    // send
    PacketPassInterface_Sender_Send(o->output, flow->waiting_data, flow->waiting_len);
    o->sending_flow = flow;
}

static void schedule_job_handler (PacketPassFifoQueue *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->freeing)
    ASSERT(!o->sending_flow)
    
    if (!LinkedList1_IsEmpty(&o->waiting_flows_list)) {
        schedule(o);
    }
}

static void input_handler_send (PacketPassFifoQueueFlow *o, uint8_t *data, int data_len)
{
    PacketPassFifoQueue *queue = o->queue;
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->is_waiting)
    ASSERT(o != queue->sending_flow)
    ASSERT(!queue->freeing)
    
    // queue flow
    o->waiting_data = data;
    o->waiting_len = data_len;
    LinkedList1_Append(&queue->waiting_flows_list, &o->waiting_flows_list_node);
    o->is_waiting = 1;
    
    // schedule
    if (!queue->sending_flow && !BPending_IsSet(&queue->schedule_job)) {
        schedule(queue);
    }
}

static void output_handler_done (PacketPassFifoQueue *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->sending_flow)
    ASSERT(!BPending_IsSet(&o->schedule_job))
    ASSERT(!o->freeing)
    ASSERT(!o->sending_flow->is_waiting)
    
    PacketPassFifoQueueFlow *flow = o->sending_flow;
    
    // set no sending flow
    o->sending_flow = NULL;
    
    // schedule schedule job
    BPending_Set(&o->schedule_job);
    
    // done input
    PacketPassInterface_Done(&flow->input);
    
    // call busy handler if set
    if (flow->handler_busy) {
        // handler is one-shot, unset it before calling
        PacketPassFifoQueue_handler_busy handler = flow->handler_busy;
        flow->handler_busy = NULL;
        
        // call handler
        handler(flow->user);
        return;
    }
}

void PacketPassFifoQueue_Init (PacketPassFifoQueue *o, PacketPassInterface *output, BPendingGroup *pg)
{
    // init arguments
    o->output = output;
    o->pg = pg;
    
    // init output
    PacketPassInterface_Sender_Init(output, (PacketPassInterface_handler_done)output_handler_done, o);
    
    // init waiting flows list
    LinkedList1_Init(&o->waiting_flows_list);
    
    // set no sending flow
    o->sending_flow = NULL;
    
    // init schedule job
    BPending_Init(&o->schedule_job, pg, (BPending_handler)schedule_job_handler, o);
    
    // set not freeing
    o->freeing = 0;
    
    DebugCounter_Init(&o->d_flows_ctr);
    DebugObject_Init(&o->d_obj);
}

void PacketPassFifoQueue_Free (PacketPassFifoQueue *o)
{
    DebugObject_Free(&o->d_obj);
    DebugCounter_Free(&o->d_flows_ctr);
    ASSERT(LinkedList1_IsEmpty(&o->waiting_flows_list))
    ASSERT(!o->sending_flow)
    
    // free schedule job
    BPending_Free(&o->schedule_job);
}

void PacketPassFifoQueue_PrepareFree (PacketPassFifoQueue *o)
{
    DebugObject_Access(&o->d_obj);
    
    // set freeing
    o->freeing = 1;
}

void PacketPassFifoQueueFlow_Init (PacketPassFifoQueueFlow *o, PacketPassFifoQueue *queue)
{
    DebugObject_Access(&queue->d_obj);
    ASSERT(!queue->freeing)
    
    // init arguments
    o->queue = queue;
    
    // init input
    PacketPassInterface_Init(&o->input, PacketPassInterface_GetMTU(queue->output), (PacketPassInterface_handler_send)input_handler_send, o, queue->pg);
    
    // set not waiting
    o->is_waiting = 0;
    
    // set no busy handler
    o->handler_busy = NULL;
    
    DebugCounter_Increment(&queue->d_flows_ctr);
    DebugObject_Init(&o->d_obj);
}

void PacketPassFifoQueueFlow_Free (PacketPassFifoQueueFlow *o)
{
    PacketPassFifoQueue *queue = o->queue;
    DebugObject_Free(&o->d_obj);
    DebugCounter_Decrement(&queue->d_flows_ctr);
    ASSERT(queue->freeing || o != queue->sending_flow)
    
    // remove from sending flow
    if (o == queue->sending_flow) {
        queue->sending_flow = NULL;
    }
    
    // remove from waiting flows list
    if (o->is_waiting) {
        LinkedList1_Remove(&queue->waiting_flows_list, &o->waiting_flows_list_node);
    }
    
    // free input
    PacketPassInterface_Free(&o->input);
}

void PacketPassFifoQueueFlow_AssertFree (PacketPassFifoQueueFlow *o)
{
    PacketPassFifoQueue *queue = o->queue;
    B_USE(queue)
    DebugObject_Access(&o->d_obj);
    ASSERT(queue->freeing || o != queue->sending_flow)
}

int PacketPassFifoQueueFlow_IsBusy (PacketPassFifoQueueFlow *o)
{
    PacketPassFifoQueue *queue = o->queue;
    DebugObject_Access(&o->d_obj);
    ASSERT(!queue->freeing)
    
    return (o == queue->sending_flow);
}

void PacketPassFifoQueueFlow_SetBusyHandler (PacketPassFifoQueueFlow *o, PacketPassFifoQueue_handler_busy handler_busy, void *user)
{
    PacketPassFifoQueue *queue = o->queue;
    B_USE(queue)
    DebugObject_Access(&o->d_obj);
    ASSERT(!queue->freeing)
    ASSERT(o == queue->sending_flow)
    
    // set (or unset) busy handler
    o->handler_busy = handler_busy;
    o->user = user;
}

PacketPassInterface * PacketPassFifoQueueFlow_GetInput (PacketPassFifoQueueFlow *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->input;
}
