/**
 * @file BEventLock.c
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

#include <misc/offset.h>

#include "BEventLock.h"

static void exec_job_handler (BEventLock *o)
{
    ASSERT(!LinkedList1_IsEmpty(&o->jobs))
    DebugObject_Access(&o->d_obj);
    
    // get job
    BEventLockJob *j = UPPER_OBJECT(LinkedList1_GetFirst(&o->jobs), BEventLockJob, pending_node);
    ASSERT(j->pending)
    
    // call handler
    j->handler(j->user);
    return;
}

void BEventLock_Init (BEventLock *o, BPendingGroup *pg)
{
    // init jobs list
    LinkedList1_Init(&o->jobs);
    
    // init exec job
    BPending_Init(&o->exec_job, pg, (BPending_handler)exec_job_handler, o);
    
    DebugObject_Init(&o->d_obj);
    DebugCounter_Init(&o->pending_ctr);
}

void BEventLock_Free (BEventLock *o)
{
    ASSERT(LinkedList1_IsEmpty(&o->jobs))
    DebugCounter_Free(&o->pending_ctr);
    DebugObject_Free(&o->d_obj);
    
    // free exec jobs
    BPending_Free(&o->exec_job);
}

void BEventLockJob_Init (BEventLockJob *o, BEventLock *l, BEventLock_handler handler, void *user)
{
    // init arguments
    o->l = l;
    o->handler = handler;
    o->user = user;
    
    // set not pending
    o->pending = 0;
    
    DebugObject_Init(&o->d_obj);
    DebugCounter_Increment(&l->pending_ctr);
}

void BEventLockJob_Free (BEventLockJob *o)
{
    BEventLock *l = o->l;
    
    DebugCounter_Decrement(&l->pending_ctr);
    DebugObject_Free(&o->d_obj);
    
    if (o->pending) {
        int was_first = (&o->pending_node == LinkedList1_GetFirst(&l->jobs));
        
        // remove from jobs list
        LinkedList1_Remove(&l->jobs, &o->pending_node);
        
        // schedule/unschedule job
        if (was_first) {
            if (LinkedList1_IsEmpty(&l->jobs)) {
                BPending_Unset(&l->exec_job);
            } else {
                BPending_Set(&l->exec_job);
            }
        }
    }
}

void BEventLockJob_Wait (BEventLockJob *o)
{
    BEventLock *l = o->l;
    ASSERT(!o->pending)
    
    // append to jobs
    LinkedList1_Append(&l->jobs, &o->pending_node);
    
    // set pending
    o->pending = 1;
    
    // schedule next job if needed
    if (&o->pending_node == LinkedList1_GetFirst(&l->jobs)) {
        BPending_Set(&l->exec_job);
    }
}

void BEventLockJob_Release (BEventLockJob *o)
{
    BEventLock *l = o->l;
    ASSERT(o->pending)
    
    int was_first = (&o->pending_node == LinkedList1_GetFirst(&l->jobs));
    
    // remove from jobs list
    LinkedList1_Remove(&l->jobs, &o->pending_node);
    
    // set not pending
    o->pending = 0;
    
    // schedule/unschedule job
    if (was_first) {
        if (LinkedList1_IsEmpty(&l->jobs)) {
            BPending_Unset(&l->exec_job);
        } else {
            BPending_Set(&l->exec_job);
        }
    }
}
