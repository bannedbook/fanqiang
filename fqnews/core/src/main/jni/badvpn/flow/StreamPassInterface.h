/**
 * @file StreamPassInterface.h
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
 * Interface allowing a stream sender to pass stream data to a stream receiver.
 * 
 * Note that this interface behaves exactly the same and has the same code as
 * {@link StreamRecvInterface} if names and its external semantics are disregarded.
 * If you modify this file, you should probably modify {@link StreamRecvInterface}
 * too.
 */

#ifndef BADVPN_FLOW_STREAMPASSINTERFACE_H
#define BADVPN_FLOW_STREAMPASSINTERFACE_H

#include <stdint.h>
#include <stddef.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <base/BPending.h>

#define SPI_STATE_NONE 1
#define SPI_STATE_OPERATION_PENDING 2
#define SPI_STATE_BUSY 3
#define SPI_STATE_DONE_PENDING 4

typedef void (*StreamPassInterface_handler_send) (void *user, uint8_t *data, int data_len);

typedef void (*StreamPassInterface_handler_done) (void *user, int data_len);

typedef struct {
    // provider data
    StreamPassInterface_handler_send handler_operation;
    void *user_provider;
    
    // user data
    StreamPassInterface_handler_done handler_done;
    void *user_user;
    
    // operation job
    BPending job_operation;
    uint8_t *job_operation_data;
    int job_operation_len;
    
    // done job
    BPending job_done;
    int job_done_len;
    
    // state
    int state;
    
    DebugObject d_obj;
} StreamPassInterface;

static void StreamPassInterface_Init (StreamPassInterface *i, StreamPassInterface_handler_send handler_operation, void *user, BPendingGroup *pg);

static void StreamPassInterface_Free (StreamPassInterface *i);

static void StreamPassInterface_Done (StreamPassInterface *i, int data_len);

static void StreamPassInterface_Sender_Init (StreamPassInterface *i, StreamPassInterface_handler_done handler_done, void *user);

static void StreamPassInterface_Sender_Send (StreamPassInterface *i, uint8_t *data, int data_len);

void _StreamPassInterface_job_operation (StreamPassInterface *i);
void _StreamPassInterface_job_done (StreamPassInterface *i);

void StreamPassInterface_Init (StreamPassInterface *i, StreamPassInterface_handler_send handler_operation, void *user, BPendingGroup *pg)
{
    // init arguments
    i->handler_operation = handler_operation;
    i->user_provider = user;
    
    // set no user
    i->handler_done = NULL;
    
    // init jobs
    BPending_Init(&i->job_operation, pg, (BPending_handler)_StreamPassInterface_job_operation, i);
    BPending_Init(&i->job_done, pg, (BPending_handler)_StreamPassInterface_job_done, i);
    
    // set state
    i->state = SPI_STATE_NONE;
    
    DebugObject_Init(&i->d_obj);
}

void StreamPassInterface_Free (StreamPassInterface *i)
{
    DebugObject_Free(&i->d_obj);
    
    // free jobs
    BPending_Free(&i->job_done);
    BPending_Free(&i->job_operation);
}

void StreamPassInterface_Done (StreamPassInterface *i, int data_len)
{
    ASSERT(i->state == SPI_STATE_BUSY)
    ASSERT(data_len > 0)
    ASSERT(data_len <= i->job_operation_len)
    DebugObject_Access(&i->d_obj);
    
    // schedule done
    i->job_done_len = data_len;
    BPending_Set(&i->job_done);
    
    // set state
    i->state = SPI_STATE_DONE_PENDING;
}

void StreamPassInterface_Sender_Init (StreamPassInterface *i, StreamPassInterface_handler_done handler_done, void *user)
{
    ASSERT(handler_done)
    ASSERT(!i->handler_done)
    DebugObject_Access(&i->d_obj);
    
    i->handler_done = handler_done;
    i->user_user = user;
}

void StreamPassInterface_Sender_Send (StreamPassInterface *i, uint8_t *data, int data_len)
{
    ASSERT(data_len > 0)
    ASSERT(data)
    ASSERT(i->state == SPI_STATE_NONE)
    ASSERT(i->handler_done)
    DebugObject_Access(&i->d_obj);
    
    // schedule operation
    i->job_operation_data = data;
    i->job_operation_len = data_len;
    BPending_Set(&i->job_operation);
    
    // set state
    i->state = SPI_STATE_OPERATION_PENDING;
}

#endif
