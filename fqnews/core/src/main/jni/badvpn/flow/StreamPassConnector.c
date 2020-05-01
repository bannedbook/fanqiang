/**
 * @file StreamPassConnector.c
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

#include <flow/StreamPassConnector.h>

static void input_handler_send (StreamPassConnector *o, uint8_t *data, int data_len)
{
    ASSERT(data_len > 0)
    ASSERT(o->in_len == -1)
    DebugObject_Access(&o->d_obj);
    
    // remember input packet
    o->in_len = data_len;
    o->in = data;
    
    if (o->output) {
        // schedule send
        StreamPassInterface_Sender_Send(o->output, o->in, o->in_len);
    }
}

static void output_handler_done (StreamPassConnector *o, int data_len)
{
    ASSERT(data_len > 0)
    ASSERT(data_len <= o->in_len)
    ASSERT(o->in_len > 0)
    ASSERT(o->output)
    DebugObject_Access(&o->d_obj);
    
    // have no input packet
    o->in_len = -1;
    
    // allow input to send more packets
    StreamPassInterface_Done(&o->input, data_len);
}

void StreamPassConnector_Init (StreamPassConnector *o, BPendingGroup *pg)
{
    // init output
    StreamPassInterface_Init(&o->input, (StreamPassInterface_handler_send)input_handler_send, o, pg);
    
    // have no input packet
    o->in_len = -1;
    
    // have no output
    o->output = NULL;
    
    DebugObject_Init(&o->d_obj);
}

void StreamPassConnector_Free (StreamPassConnector *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free output
    StreamPassInterface_Free(&o->input);
}

StreamPassInterface * StreamPassConnector_GetInput (StreamPassConnector *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->input;
}

void StreamPassConnector_ConnectOutput (StreamPassConnector *o, StreamPassInterface *output)
{
    ASSERT(!o->output)
    DebugObject_Access(&o->d_obj);
    
    // set output
    o->output = output;
    
    // init output
    StreamPassInterface_Sender_Init(o->output, (StreamPassInterface_handler_done)output_handler_done, o);
    
    // if we have an input packet, schedule send
    if (o->in_len > 0) {
        StreamPassInterface_Sender_Send(o->output, o->in, o->in_len);
    }
}

void StreamPassConnector_DisconnectOutput (StreamPassConnector *o)
{
    ASSERT(o->output)
    DebugObject_Access(&o->d_obj);
    
    // set no output
    o->output = NULL;
}
