/**
 * @file BPending.c
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

#include <stddef.h>

#include <misc/debug.h>
#include <misc/offset.h>

#include "BPending.h"

#include "BPending_list.h"
#include <structure/SLinkedList_impl.h>

void BPendingGroup_Init (BPendingGroup *g)
{
    // init jobs list
    BPending__List_Init(&g->jobs);
    
    // init pending counter
    DebugCounter_Init(&g->pending_ctr);
    
    // init debug object
    DebugObject_Init(&g->d_obj);
}

void BPendingGroup_Free (BPendingGroup *g)
{
    DebugCounter_Free(&g->pending_ctr);
    ASSERT(BPending__List_IsEmpty(&g->jobs))
    DebugObject_Free(&g->d_obj);
}

int BPendingGroup_HasJobs (BPendingGroup *g)
{
    DebugObject_Access(&g->d_obj);
    
    return !BPending__List_IsEmpty(&g->jobs);
}

void BPendingGroup_ExecuteJob (BPendingGroup *g)
{
    ASSERT(!BPending__List_IsEmpty(&g->jobs))
    DebugObject_Access(&g->d_obj);
    
    // get a job
    BSmallPending *p = BPending__List_First(&g->jobs);
    ASSERT(!BPending__ListIsRemoved(p))
    ASSERT(p->pending)
    
    // remove from jobs list
    BPending__List_RemoveFirst(&g->jobs);
    
    // set not pending
    BPending__ListMarkRemoved(p);
#ifndef NDEBUG
    p->pending = 0;
#endif
    
    // execute job
    p->handler(p->user);
    return;
}

BSmallPending * BPendingGroup_PeekJob (BPendingGroup *g)
{
    DebugObject_Access(&g->d_obj);
    
    return BPending__List_First(&g->jobs);
}

void BSmallPending_Init (BSmallPending *o, BPendingGroup *g, BSmallPending_handler handler, void *user)
{
    // init arguments
    o->handler = handler;
    o->user = user;
    
    // set not pending
    BPending__ListMarkRemoved(o);
#ifndef NDEBUG
    o->pending = 0;
#endif
    
    // increment pending counter
    DebugCounter_Increment(&g->pending_ctr);
    
    // init debug object
    DebugObject_Init(&o->d_obj);
}

void BSmallPending_Free (BSmallPending *o, BPendingGroup *g)
{
    DebugCounter_Decrement(&g->pending_ctr);
    DebugObject_Free(&o->d_obj);
    ASSERT(o->pending == !BPending__ListIsRemoved(o))
    
    // remove from jobs list
    if (!BPending__ListIsRemoved(o)) {
        BPending__List_Remove(&g->jobs, o);
    }
}

void BSmallPending_SetHandler (BSmallPending *o, BSmallPending_handler handler, void *user)
{
    DebugObject_Access(&o->d_obj);
    
    // set handler
    o->handler = handler;
    o->user = user;
}

void BSmallPending_Set (BSmallPending *o, BPendingGroup *g)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->pending == !BPending__ListIsRemoved(o))
    
    // remove from jobs list
    if (!BPending__ListIsRemoved(o)) {
        BPending__List_Remove(&g->jobs, o);
    }
    
    // insert to jobs list
    BPending__List_Prepend(&g->jobs, o);
    
    // set pending
#ifndef NDEBUG
    o->pending = 1;
#endif
}

void BSmallPending_Unset (BSmallPending *o, BPendingGroup *g)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->pending == !BPending__ListIsRemoved(o))
    
    if (!BPending__ListIsRemoved(o)) {
        // remove from jobs list
        BPending__List_Remove(&g->jobs, o);
        
        // set not pending
        BPending__ListMarkRemoved(o);
#ifndef NDEBUG
        o->pending = 0;
#endif
    }
}

int BSmallPending_IsSet (BSmallPending *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->pending == !BPending__ListIsRemoved(o))
    
    return !BPending__ListIsRemoved(o);
}

void BPending_Init (BPending *o, BPendingGroup *g, BPending_handler handler, void *user)
{
    BSmallPending_Init(&o->base, g, handler, user);
    o->g = g;
}

void BPending_Free (BPending *o)
{
    BSmallPending_Free(&o->base, o->g);
}

void BPending_Set (BPending *o)
{
    BSmallPending_Set(&o->base, o->g);
}

void BPending_Unset (BPending *o)
{
    BSmallPending_Unset(&o->base, o->g);
}

int BPending_IsSet (BPending *o)
{
    return BSmallPending_IsSet(&o->base);
}
