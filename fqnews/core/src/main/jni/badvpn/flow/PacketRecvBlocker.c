/**
 * @file PacketRecvBlocker.c
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

#include <misc/debug.h>

#include <flow/PacketRecvBlocker.h>

static void output_handler_recv (PacketRecvBlocker *o, uint8_t *data)
{
    ASSERT(!o->out_have)
    DebugObject_Access(&o->d_obj);
    
    // remember packet
    o->out_have = 1;
    o->out = data;
    o->out_input_blocking = 0;
}

static void input_handler_done (PacketRecvBlocker *o, int data_len)
{
    ASSERT(o->out_have)
    ASSERT(o->out_input_blocking)
    DebugObject_Access(&o->d_obj);
    
    // schedule done
    o->out_have = 0;
    PacketRecvInterface_Done(&o->output, data_len);
}

void PacketRecvBlocker_Init (PacketRecvBlocker *o, PacketRecvInterface *input, BPendingGroup *pg)
{
    // init arguments
    o->input = input;
    
    // init output
    PacketRecvInterface_Init(&o->output, PacketRecvInterface_GetMTU(o->input), (PacketRecvInterface_handler_recv)output_handler_recv, o, pg);
    
    // have no output packet
    o->out_have = 0;
    
    // init input
    PacketRecvInterface_Receiver_Init(o->input, (PacketRecvInterface_handler_done)input_handler_done, o);
    
    DebugObject_Init(&o->d_obj);
}

void PacketRecvBlocker_Free (PacketRecvBlocker *o)
{
    DebugObject_Free(&o->d_obj);

    // free output
    PacketRecvInterface_Free(&o->output);
}

PacketRecvInterface * PacketRecvBlocker_GetOutput (PacketRecvBlocker *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}

void PacketRecvBlocker_AllowBlockedPacket (PacketRecvBlocker *o)
{
    DebugObject_Access(&o->d_obj);
    
    if (!o->out_have || o->out_input_blocking) {
        return;
    }
    
    // schedule receive
    o->out_input_blocking = 1;
    PacketRecvInterface_Receiver_Recv(o->input, o->out);
}
