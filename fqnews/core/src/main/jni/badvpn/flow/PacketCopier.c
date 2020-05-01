/**
 * @file PacketCopier.c
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

#include <string.h>

#include <misc/debug.h>

#include <flow/PacketCopier.h>

static void input_handler_send (PacketCopier *o, uint8_t *data, int data_len)
{
    ASSERT(o->in_len == -1)
    ASSERT(data_len >= 0)
    DebugObject_Access(&o->d_obj);
    
    if (!o->out_have) {
        o->in_len = data_len;
        o->in = data;
        return;
    }
    
    memcpy(o->out, data, data_len);
    
    // finish input packet
    PacketPassInterface_Done(&o->input);
    
    // finish output packet
    PacketRecvInterface_Done(&o->output, data_len);
    
    o->out_have = 0;
}

static void input_handler_requestcancel (PacketCopier *o)
{
    ASSERT(o->in_len >= 0)
    ASSERT(!o->out_have)
    DebugObject_Access(&o->d_obj);
    
    // finish input packet
    PacketPassInterface_Done(&o->input);
    
    o->in_len = -1;
}

static void output_handler_recv (PacketCopier *o, uint8_t *data)
{
    ASSERT(!o->out_have)
    DebugObject_Access(&o->d_obj);
    
    if (o->in_len < 0) {
        o->out_have = 1;
        o->out = data;
        return;
    }
    
    memcpy(data, o->in, o->in_len);
    
    // finish input packet
    PacketPassInterface_Done(&o->input);
    
    // finish output packet
    PacketRecvInterface_Done(&o->output, o->in_len);
    
    o->in_len = -1;
}

void PacketCopier_Init (PacketCopier *o, int mtu, BPendingGroup *pg)
{
    ASSERT(mtu >= 0)
    
    // init input
    PacketPassInterface_Init(&o->input, mtu, (PacketPassInterface_handler_send)input_handler_send, o, pg);
    PacketPassInterface_EnableCancel(&o->input, (PacketPassInterface_handler_requestcancel)input_handler_requestcancel);
    
    // init output
    PacketRecvInterface_Init(&o->output, mtu, (PacketRecvInterface_handler_recv)output_handler_recv, o, pg);
    
    // set no input packet
    o->in_len = -1;
    
    // set no output packet
    o->out_have = 0;
    
    DebugObject_Init(&o->d_obj);
}

void PacketCopier_Free (PacketCopier *o)
{
    DebugObject_Free(&o->d_obj);

    // free output
    PacketRecvInterface_Free(&o->output);
    
    // free input
    PacketPassInterface_Free(&o->input);
}

PacketPassInterface * PacketCopier_GetInput (PacketCopier *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->input;
}

PacketRecvInterface * PacketCopier_GetOutput (PacketCopier *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}
