/**
 * @file PacketPassInterface.h
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
 * Interface allowing a packet sender to pass data packets to a packet receiver.
 */

#ifndef BADVPN_FLOW_PACKETPASSINTERFACE_H
#define BADVPN_FLOW_PACKETPASSINTERFACE_H

#include <stdint.h>
#include <stddef.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <base/BPending.h>

#define PPI_STATE_NONE 1
#define PPI_STATE_OPERATION_PENDING 2
#define PPI_STATE_BUSY 3
#define PPI_STATE_DONE_PENDING 4

typedef void (*PacketPassInterface_handler_send) (void *user, uint8_t *data, int data_len);

typedef void (*PacketPassInterface_handler_requestcancel) (void *user);

typedef void (*PacketPassInterface_handler_done) (void *user);

typedef struct {
    // provider data
    int mtu;
    PacketPassInterface_handler_send handler_operation;
    PacketPassInterface_handler_requestcancel handler_requestcancel;
    void *user_provider;
    
    // user data
    PacketPassInterface_handler_done handler_done;
    void *user_user;
    
    // operation job
    BPending job_operation;
    uint8_t *job_operation_data;
    int job_operation_len;
    
    // requestcancel job
    BPending job_requestcancel;
    
    // done job
    BPending job_done;
    
    // state
    int state;
    int cancel_requested;
    
    DebugObject d_obj;
} PacketPassInterface;

static void PacketPassInterface_Init (PacketPassInterface *i, int mtu, PacketPassInterface_handler_send handler_operation, void *user, BPendingGroup *pg);

static void PacketPassInterface_Free (PacketPassInterface *i);

static void PacketPassInterface_EnableCancel (PacketPassInterface *i, PacketPassInterface_handler_requestcancel handler_requestcancel);

static void PacketPassInterface_Done (PacketPassInterface *i);

static int PacketPassInterface_GetMTU (PacketPassInterface *i);

static void PacketPassInterface_Sender_Init (PacketPassInterface *i, PacketPassInterface_handler_done handler_done, void *user);

static void PacketPassInterface_Sender_Send (PacketPassInterface *i, uint8_t *data, int data_len);

static void PacketPassInterface_Sender_RequestCancel (PacketPassInterface *i);

static int PacketPassInterface_HasCancel (PacketPassInterface *i);

void _PacketPassInterface_job_operation (PacketPassInterface *i);
void _PacketPassInterface_job_requestcancel (PacketPassInterface *i);
void _PacketPassInterface_job_done (PacketPassInterface *i);

void PacketPassInterface_Init (PacketPassInterface *i, int mtu, PacketPassInterface_handler_send handler_operation, void *user, BPendingGroup *pg)
{
    ASSERT(mtu >= 0)
    
    // init arguments
    i->mtu = mtu;
    i->handler_operation = handler_operation;
    i->handler_requestcancel = NULL;
    i->user_provider = user;
    
    // set no user
    i->handler_done = NULL;
    
    // init jobs
    BPending_Init(&i->job_operation, pg, (BPending_handler)_PacketPassInterface_job_operation, i);
    BPending_Init(&i->job_requestcancel, pg, (BPending_handler)_PacketPassInterface_job_requestcancel, i);
    BPending_Init(&i->job_done, pg, (BPending_handler)_PacketPassInterface_job_done, i);
    
    // set state
    i->state = PPI_STATE_NONE;
    
    DebugObject_Init(&i->d_obj);
}

void PacketPassInterface_Free (PacketPassInterface *i)
{
    DebugObject_Free(&i->d_obj);
    
    // free jobs
    BPending_Free(&i->job_done);
    BPending_Free(&i->job_requestcancel);
    BPending_Free(&i->job_operation);
}

void PacketPassInterface_EnableCancel (PacketPassInterface *i, PacketPassInterface_handler_requestcancel handler_requestcancel)
{
    ASSERT(!i->handler_requestcancel)
    ASSERT(!i->handler_done)
    ASSERT(handler_requestcancel)
    
    i->handler_requestcancel = handler_requestcancel;
}

void PacketPassInterface_Done (PacketPassInterface *i)
{
    ASSERT(i->state == PPI_STATE_BUSY)
    DebugObject_Access(&i->d_obj);
    
    // unset requestcancel job
    BPending_Unset(&i->job_requestcancel);
    
    // schedule done
    BPending_Set(&i->job_done);
    
    // set state
    i->state = PPI_STATE_DONE_PENDING;
}

int PacketPassInterface_GetMTU (PacketPassInterface *i)
{
    DebugObject_Access(&i->d_obj);
    
    return i->mtu;
}

void PacketPassInterface_Sender_Init (PacketPassInterface *i, PacketPassInterface_handler_done handler_done, void *user)
{
    ASSERT(handler_done)
    ASSERT(!i->handler_done)
    DebugObject_Access(&i->d_obj);
    
    i->handler_done = handler_done;
    i->user_user = user;
}

void PacketPassInterface_Sender_Send (PacketPassInterface *i, uint8_t *data, int data_len)
{
    ASSERT(data_len >= 0)
    ASSERT(data_len <= i->mtu)
    ASSERT(!(data_len > 0) || data)
    ASSERT(i->state == PPI_STATE_NONE)
    ASSERT(i->handler_done)
    DebugObject_Access(&i->d_obj);
    
    // schedule operation
    i->job_operation_data = data;
    i->job_operation_len = data_len;
    BPending_Set(&i->job_operation);
    
    // set state
    i->state = PPI_STATE_OPERATION_PENDING;
    i->cancel_requested = 0;
}

void PacketPassInterface_Sender_RequestCancel (PacketPassInterface *i)
{
    ASSERT(i->state == PPI_STATE_OPERATION_PENDING || i->state == PPI_STATE_BUSY || i->state == PPI_STATE_DONE_PENDING)
    ASSERT(i->handler_requestcancel)
    DebugObject_Access(&i->d_obj);
    
    // ignore multiple cancel requests
    if (i->cancel_requested) {
        return;
    }
    
    // remember we requested cancel
    i->cancel_requested = 1;
    
    if (i->state == PPI_STATE_OPERATION_PENDING) {
        // unset operation job
        BPending_Unset(&i->job_operation);
        
        // set done job
        BPending_Set(&i->job_done);
        
        // set state
        i->state = PPI_STATE_DONE_PENDING;
    } else if (i->state == PPI_STATE_BUSY) {
        // set requestcancel job
        BPending_Set(&i->job_requestcancel);
    }
}

int PacketPassInterface_HasCancel (PacketPassInterface *i)
{
    DebugObject_Access(&i->d_obj);
    
    return !!i->handler_requestcancel;
}

#endif
