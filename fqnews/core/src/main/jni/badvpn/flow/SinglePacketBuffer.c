/**
 * @file SinglePacketBuffer.c
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
#include <misc/balloc.h>

#include <flow/SinglePacketBuffer.h>

static void input_handler_done (SinglePacketBuffer *o, int in_len)
{
    DebugObject_Access(&o->d_obj);
    
    PacketPassInterface_Sender_Send(o->output, o->buf, in_len);
}

static void output_handler_done (SinglePacketBuffer *o)
{
    DebugObject_Access(&o->d_obj);
    
    PacketRecvInterface_Receiver_Recv(o->input, o->buf);
}

int SinglePacketBuffer_Init (SinglePacketBuffer *o, PacketRecvInterface *input, PacketPassInterface *output, BPendingGroup *pg) 
{
    ASSERT(PacketPassInterface_GetMTU(output) >= PacketRecvInterface_GetMTU(input))
    
    // init arguments
    o->input = input;
    o->output = output;
    
    // init input
    PacketRecvInterface_Receiver_Init(o->input, (PacketRecvInterface_handler_done)input_handler_done, o);
    
    // init output
    PacketPassInterface_Sender_Init(o->output, (PacketPassInterface_handler_done)output_handler_done, o);
    
    // init buffer
    if (!(o->buf = (uint8_t *)BAlloc(PacketRecvInterface_GetMTU(o->input)))) {
        goto fail1;
    }
    
    // schedule receive
    PacketRecvInterface_Receiver_Recv(o->input, o->buf);
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail1:
    return 0;
}

void SinglePacketBuffer_Free (SinglePacketBuffer *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free buffer
    BFree(o->buf);
}
