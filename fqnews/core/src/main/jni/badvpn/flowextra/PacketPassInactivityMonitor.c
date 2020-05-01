/**
 * @file PacketPassInactivityMonitor.c
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

#include "PacketPassInactivityMonitor.h"

static void input_handler_send (PacketPassInactivityMonitor *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    
    // schedule send
    PacketPassInterface_Sender_Send(o->output, data, data_len);
    
    // stop timer
    BReactor_RemoveTimer(o->reactor, &o->timer);
}

static void input_handler_requestcancel (PacketPassInactivityMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    
    // request cancel
    PacketPassInterface_Sender_RequestCancel(o->output);
}

static void output_handler_done (PacketPassInactivityMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    
    // output no longer busy, restart timer
    BReactor_SetTimer(o->reactor, &o->timer);
    
    // call done
    PacketPassInterface_Done(&o->input);
}

static void timer_handler (PacketPassInactivityMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    
    // restart timer
    BReactor_SetTimer(o->reactor, &o->timer);
    
    // call handler
    if (o->handler) {
        o->handler(o->user);
        return;
    }
}

void PacketPassInactivityMonitor_Init (PacketPassInactivityMonitor *o, PacketPassInterface *output, BReactor *reactor, btime_t interval, PacketPassInactivityMonitor_handler handler, void *user)
{
    // init arguments
    o->output = output;
    o->reactor = reactor;
    o->handler = handler;
    o->user = user;
    
    // init input
    PacketPassInterface_Init(&o->input, PacketPassInterface_GetMTU(o->output), (PacketPassInterface_handler_send)input_handler_send, o, BReactor_PendingGroup(o->reactor));
    if (PacketPassInterface_HasCancel(o->output)) {
        PacketPassInterface_EnableCancel(&o->input, (PacketPassInterface_handler_requestcancel)input_handler_requestcancel);
    }
    
    // init output
    PacketPassInterface_Sender_Init(o->output, (PacketPassInterface_handler_done)output_handler_done, o);
    
    // init timer
    BTimer_Init(&o->timer, interval, (BTimer_handler)timer_handler, o);
    BReactor_SetTimer(o->reactor, &o->timer);
    
    DebugObject_Init(&o->d_obj);
}

void PacketPassInactivityMonitor_Free (PacketPassInactivityMonitor *o)
{
    DebugObject_Free(&o->d_obj);

    // free timer
    BReactor_RemoveTimer(o->reactor, &o->timer);
    
    // free input
    PacketPassInterface_Free(&o->input);
}

PacketPassInterface * PacketPassInactivityMonitor_GetInput (PacketPassInactivityMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->input;
}

void PacketPassInactivityMonitor_SetHandler (PacketPassInactivityMonitor *o, PacketPassInactivityMonitor_handler handler, void *user)
{
    DebugObject_Access(&o->d_obj);
    
    o->handler = handler;
    o->user = user;
}

void PacketPassInactivityMonitor_Force (PacketPassInactivityMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    
    BReactor_SetTimerAfter(o->reactor, &o->timer, 0);
}
