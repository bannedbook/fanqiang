/**
 * @file RandomPacketSink.h
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

#ifndef _RANDOMPACKETSINK_H
#define _RANDOMPACKETSINK_H

#include <stdio.h>

#include <misc/debug.h>
#include <security/BRandom.h>
#include <system/BReactor.h>
#include <base/DebugObject.h>
#include <flow/PacketPassInterface.h>

typedef struct {
    BReactor *reactor;
    PacketPassInterface input;
    BTimer timer;
    DebugObject d_obj;
} RandomPacketSink;

static void _RandomPacketSink_input_handler_send (RandomPacketSink *s, uint8_t *data, int data_len)
{
    DebugObject_Access(&s->d_obj);
    
    printf("sink: send '");
    size_t res = fwrite(data, data_len, 1, stdout);
    B_USE(res)
    
    uint8_t r;
    BRandom_randomize(&r, sizeof(r));
    if (r&(uint8_t)1) {
        printf("' accepting\n");
        PacketPassInterface_Done(&s->input);
    } else {
        printf("' delaying\n");
        BReactor_SetTimer(s->reactor, &s->timer);
    }
}

static void _RandomPacketSink_input_handler_requestcancel (RandomPacketSink *s)
{
    DebugObject_Access(&s->d_obj);
    
    printf("sink: cancelled\n");
    BReactor_RemoveTimer(s->reactor, &s->timer);
    PacketPassInterface_Done(&s->input);
}

static void _RandomPacketSink_timer_handler (RandomPacketSink *s)
{
    DebugObject_Access(&s->d_obj);
    
    PacketPassInterface_Done(&s->input);
}

static void RandomPacketSink_Init (RandomPacketSink *s, BReactor *reactor, int mtu, int ms)
{
    // init arguments
    s->reactor = reactor;
    
    // init input
    PacketPassInterface_Init(&s->input, mtu, (PacketPassInterface_handler_send)_RandomPacketSink_input_handler_send, s, BReactor_PendingGroup(reactor));
    PacketPassInterface_EnableCancel(&s->input, (PacketPassInterface_handler_requestcancel)_RandomPacketSink_input_handler_requestcancel);
    
    // init timer
    BTimer_Init(&s->timer, ms, (BTimer_handler)_RandomPacketSink_timer_handler, s);
    
    DebugObject_Init(&s->d_obj);
}

static void RandomPacketSink_Free (RandomPacketSink *s)
{
    DebugObject_Free(&s->d_obj);
    
    // free timer
    BReactor_RemoveTimer(s->reactor, &s->timer);
    
    // free input
    PacketPassInterface_Free(&s->input);
}

static PacketPassInterface * RandomPacketSink_GetInput (RandomPacketSink *s)
{
    DebugObject_Access(&s->d_obj);
    
    return &s->input;
}

#endif
