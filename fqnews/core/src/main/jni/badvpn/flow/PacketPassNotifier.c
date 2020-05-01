/**
 * @file PacketPassNotifier.c
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

#include <flow/PacketPassNotifier.h>

void input_handler_send (PacketPassNotifier *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    
    // schedule send
    PacketPassInterface_Sender_Send(o->output, data, data_len);
    
    // if we have a handler, call it
    if (o->handler) {
        o->handler(o->handler_user, data, data_len);
        return;
    }
}

void input_handler_requestcancel (PacketPassNotifier *o)
{
    DebugObject_Access(&o->d_obj);
    
    PacketPassInterface_Sender_RequestCancel(o->output);
}

void output_handler_done (PacketPassNotifier *o)
{
    DebugObject_Access(&o->d_obj);
    
    PacketPassInterface_Done(&o->input);
}

void PacketPassNotifier_Init (PacketPassNotifier *o, PacketPassInterface *output, BPendingGroup *pg)
{
    // init arguments
    o->output = output;
    
    // init input
    PacketPassInterface_Init(&o->input, PacketPassInterface_GetMTU(o->output), (PacketPassInterface_handler_send)input_handler_send, o, pg);
    if (PacketPassInterface_HasCancel(o->output)) {
        PacketPassInterface_EnableCancel(&o->input, (PacketPassInterface_handler_requestcancel)input_handler_requestcancel);
    }
    
    // init output
    PacketPassInterface_Sender_Init(o->output, (PacketPassInterface_handler_done)output_handler_done, o);
    
    // set no handler
    o->handler = NULL;
    
    DebugObject_Init(&o->d_obj);
}

void PacketPassNotifier_Free (PacketPassNotifier *o)
{
    DebugObject_Free(&o->d_obj);

    // free input
    PacketPassInterface_Free(&o->input);
}

PacketPassInterface * PacketPassNotifier_GetInput (PacketPassNotifier *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->input;
}

void PacketPassNotifier_SetHandler (PacketPassNotifier *o, PacketPassNotifier_handler_notify handler, void *user)
{
    DebugObject_Access(&o->d_obj);
    
    o->handler = handler;
    o->handler_user = user;
}
