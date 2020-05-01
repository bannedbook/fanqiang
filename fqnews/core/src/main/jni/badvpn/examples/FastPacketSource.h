/**
 * @file FastPacketSource.h
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

#ifndef _FASTPACKETSOURCE_H
#define _FASTPACKETSOURCE_H

#include <stdint.h>
#include <string.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <flow/PacketPassInterface.h>

typedef struct {
    PacketPassInterface *output;
    int psize;
    uint8_t *data;
    int data_len;
    DebugObject d_obj;
} FastPacketSource;

static void _FastPacketSource_output_handler_done (FastPacketSource *s)
{
    DebugObject_Access(&s->d_obj);
    
    PacketPassInterface_Sender_Send(s->output, s->data, s->data_len);
}

static void FastPacketSource_Init (FastPacketSource *s, PacketPassInterface *output, uint8_t *data, int data_len, BPendingGroup *pg)
{
    ASSERT(data_len >= 0)
    ASSERT(data_len <= PacketPassInterface_GetMTU(output));
    
    // init arguments
    s->output = output;
    s->data = data;
    s->data_len = data_len;
    
    // init output
    PacketPassInterface_Sender_Init(s->output, (PacketPassInterface_handler_done)_FastPacketSource_output_handler_done, s);
    
    // schedule send
    PacketPassInterface_Sender_Send(s->output, s->data, s->data_len);
    
    DebugObject_Init(&s->d_obj);
}

static void FastPacketSource_Free (FastPacketSource *s)
{
    DebugObject_Free(&s->d_obj);
}

#endif
