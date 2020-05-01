/**
 * @file PacketRouter.c
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

#include <flow/PacketRouter.h>

static void input_handler_done (PacketRouter *o, int data_len)
{
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->mtu - o->recv_offset)
    ASSERT(!BPending_IsSet(&o->next_job))
    DebugObject_Access(&o->d_obj);
    
    // set next job
    BPending_Set(&o->next_job);
    
    // call handler
    o->handler(o->user, RouteBufferSource_Pointer(&o->rbs), data_len);
    return;
}

static void next_job_handler (PacketRouter *o)
{
    DebugObject_Access(&o->d_obj);
    
    // receive
    PacketRecvInterface_Receiver_Recv(o->input, RouteBufferSource_Pointer(&o->rbs) + o->recv_offset);
}

int PacketRouter_Init (PacketRouter *o, int mtu, int recv_offset, PacketRecvInterface *input, PacketRouter_handler handler, void *user, BPendingGroup *pg)
{
    ASSERT(mtu >= 0)
    ASSERT(recv_offset >= 0)
    ASSERT(recv_offset <= mtu)
    ASSERT(PacketRecvInterface_GetMTU(input) <= mtu - recv_offset)
    
    // init arguments
    o->mtu = mtu;
    o->recv_offset = recv_offset;
    o->input = input;
    o->handler = handler;
    o->user = user;
    
    // init input
    PacketRecvInterface_Receiver_Init(o->input, (PacketRecvInterface_handler_done)input_handler_done, o);
    
    // init RouteBufferSource
    if (!RouteBufferSource_Init(&o->rbs, mtu)) {
        goto fail0;
    }
    
    // init next job
    BPending_Init(&o->next_job, pg, (BPending_handler)next_job_handler, o);
    
    // receive
    PacketRecvInterface_Receiver_Recv(o->input, RouteBufferSource_Pointer(&o->rbs) + o->recv_offset);
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail0:
    return 0;
}

void PacketRouter_Free (PacketRouter *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free next job
    BPending_Free(&o->next_job);
    
    // free RouteBufferSource
    RouteBufferSource_Free(&o->rbs);
}

int PacketRouter_Route (PacketRouter *o, int len, RouteBuffer *output, uint8_t **next_buf, int copy_offset, int copy_len)
{
    ASSERT(len >= 0)
    ASSERT(len <= o->mtu)
    ASSERT(RouteBuffer_GetMTU(output) == o->mtu)
    ASSERT(copy_offset >= 0)
    ASSERT(copy_offset <= o->mtu)
    ASSERT(copy_len >= 0)
    ASSERT(copy_len <= o->mtu - copy_offset)
    ASSERT(BPending_IsSet(&o->next_job))
    DebugObject_Access(&o->d_obj);
    
    if (!RouteBufferSource_Route(&o->rbs, len, output, copy_offset, copy_len)) {
        return 0;
    }
    
    if (next_buf) {
        *next_buf = RouteBufferSource_Pointer(&o->rbs);
    }
    
    return 1;
}

void PacketRouter_AssertRoute (PacketRouter *o)
{
    ASSERT(BPending_IsSet(&o->next_job))
    DebugObject_Access(&o->d_obj);
}
