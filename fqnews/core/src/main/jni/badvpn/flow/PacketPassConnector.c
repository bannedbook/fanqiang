/**
 * @file PacketPassConnector.c
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

#include <flow/PacketPassConnector.h>

static void input_handler_send (PacketPassConnector *o, uint8_t *data, int data_len)
{
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->input_mtu)
    ASSERT(o->in_len == -1)
    DebugObject_Access(&o->d_obj);
    
    // remember input packet
    o->in_len = data_len;
    o->in = data;
    
    if (o->output) {
        // schedule send
        PacketPassInterface_Sender_Send(o->output, o->in, o->in_len);
    }
}

static void output_handler_done (PacketPassConnector *o)
{
    ASSERT(o->in_len >= 0)
    ASSERT(o->output)
    DebugObject_Access(&o->d_obj);
    
    // have no input packet
    o->in_len = -1;
    
    // allow input to send more packets
    PacketPassInterface_Done(&o->input);
}

void PacketPassConnector_Init (PacketPassConnector *o, int mtu, BPendingGroup *pg)
{
    ASSERT(mtu >= 0)
    
    // init arguments
    o->input_mtu = mtu;
    
    // init input
    PacketPassInterface_Init(&o->input, o->input_mtu, (PacketPassInterface_handler_send)input_handler_send, o, pg);
    
    // have no input packet
    o->in_len = -1;
    
    // have no output
    o->output = NULL;
    
    DebugObject_Init(&o->d_obj);
}

void PacketPassConnector_Free (PacketPassConnector *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free input
    PacketPassInterface_Free(&o->input);
}

PacketPassInterface * PacketPassConnector_GetInput (PacketPassConnector *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->input;
}

void PacketPassConnector_ConnectOutput (PacketPassConnector *o, PacketPassInterface *output)
{
    ASSERT(!o->output)
    ASSERT(PacketPassInterface_GetMTU(output) >= o->input_mtu)
    DebugObject_Access(&o->d_obj);
    
    // set output
    o->output = output;
    
    // init output
    PacketPassInterface_Sender_Init(o->output, (PacketPassInterface_handler_done)output_handler_done, o);
    
    // if we have an input packet, schedule send
    if (o->in_len >= 0) {
        PacketPassInterface_Sender_Send(o->output, o->in, o->in_len);
    }
}

void PacketPassConnector_DisconnectOutput (PacketPassConnector *o)
{
    ASSERT(o->output)
    DebugObject_Access(&o->d_obj);
    
    // set no output
    o->output = NULL;
}
