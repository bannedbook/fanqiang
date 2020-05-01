/**
 * @file StreamRecvConnector.c
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

#include <flow/StreamRecvConnector.h>

static void output_handler_recv (StreamRecvConnector *o, uint8_t *data, int data_avail)
{
    ASSERT(data_avail > 0)
    ASSERT(o->out_avail == -1)
    DebugObject_Access(&o->d_obj);
    
    // remember output packet
    o->out_avail = data_avail;
    o->out = data;
    
    if (o->input) {
        // schedule receive
        StreamRecvInterface_Receiver_Recv(o->input, o->out, o->out_avail);
    }
}

static void input_handler_done (StreamRecvConnector *o, int data_len)
{
    ASSERT(data_len > 0)
    ASSERT(data_len <= o->out_avail)
    ASSERT(o->out_avail > 0)
    ASSERT(o->input)
    DebugObject_Access(&o->d_obj);
    
    // have no output packet
    o->out_avail = -1;
    
    // allow output to receive more packets
    StreamRecvInterface_Done(&o->output, data_len);
}

void StreamRecvConnector_Init (StreamRecvConnector *o, BPendingGroup *pg)
{
    // init output
    StreamRecvInterface_Init(&o->output, (StreamRecvInterface_handler_recv)output_handler_recv, o, pg);
    
    // have no output packet
    o->out_avail = -1;
    
    // have no input
    o->input = NULL;
    
    DebugObject_Init(&o->d_obj);
}

void StreamRecvConnector_Free (StreamRecvConnector *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free output
    StreamRecvInterface_Free(&o->output);
}

StreamRecvInterface * StreamRecvConnector_GetOutput (StreamRecvConnector *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}

void StreamRecvConnector_ConnectInput (StreamRecvConnector *o, StreamRecvInterface *input)
{
    ASSERT(!o->input)
    DebugObject_Access(&o->d_obj);
    
    // set input
    o->input = input;
    
    // init input
    StreamRecvInterface_Receiver_Init(o->input, (StreamRecvInterface_handler_done)input_handler_done, o);
    
    // if we have an output packet, schedule receive
    if (o->out_avail > 0) {
        StreamRecvInterface_Receiver_Recv(o->input, o->out, o->out_avail);
    }
}

void StreamRecvConnector_DisconnectInput (StreamRecvConnector *o)
{
    ASSERT(o->input)
    DebugObject_Access(&o->d_obj);
    
    // set no input
    o->input = NULL;
}
