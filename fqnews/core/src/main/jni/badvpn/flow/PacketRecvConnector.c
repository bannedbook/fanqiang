/**
 * @file PacketRecvConnector.c
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

#include <flow/PacketRecvConnector.h>

static void output_handler_recv (PacketRecvConnector *o, uint8_t *data)
{
    ASSERT(!o->out_have)
    DebugObject_Access(&o->d_obj);
    
    // remember output packet
    o->out_have = 1;
    o->out = data;
    
    if (o->input) {
        // schedule receive
        PacketRecvInterface_Receiver_Recv(o->input, o->out);
    }
}

static void input_handler_done (PacketRecvConnector *o, int data_len)
{
    ASSERT(o->out_have)
    ASSERT(o->input)
    DebugObject_Access(&o->d_obj);
    
    // have no output packet
    o->out_have = 0;
    
    // allow output to receive more packets
    PacketRecvInterface_Done(&o->output, data_len);
}

void PacketRecvConnector_Init (PacketRecvConnector *o, int mtu, BPendingGroup *pg)
{
    ASSERT(mtu >= 0)
    
    // init arguments
    o->output_mtu = mtu;
    
    // init output
    PacketRecvInterface_Init(&o->output, o->output_mtu, (PacketRecvInterface_handler_recv)output_handler_recv, o, pg);
    
    // have no output packet
    o->out_have = 0;
    
    // have no input
    o->input = NULL;
    
    DebugObject_Init(&o->d_obj);
}

void PacketRecvConnector_Free (PacketRecvConnector *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free output
    PacketRecvInterface_Free(&o->output);
}

PacketRecvInterface * PacketRecvConnector_GetOutput (PacketRecvConnector *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}

void PacketRecvConnector_ConnectInput (PacketRecvConnector *o, PacketRecvInterface *input)
{
    ASSERT(!o->input)
    ASSERT(PacketRecvInterface_GetMTU(input) <= o->output_mtu)
    DebugObject_Access(&o->d_obj);
    
    // set input
    o->input = input;
    
    // init input
    PacketRecvInterface_Receiver_Init(o->input, (PacketRecvInterface_handler_done)input_handler_done, o);
    
    // if we have an output packet, schedule receive
    if (o->out_have) {
        PacketRecvInterface_Receiver_Recv(o->input, o->out);
    }
}

void PacketRecvConnector_DisconnectInput (PacketRecvConnector *o)
{
    ASSERT(o->input)
    DebugObject_Access(&o->d_obj);
    
    // set no input
    o->input = NULL;
}
