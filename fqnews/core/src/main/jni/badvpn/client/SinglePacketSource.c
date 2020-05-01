/**
 * @file SinglePacketSource.c
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

#include "SinglePacketSource.h"

static void output_handler_recv (SinglePacketSource *o, uint8_t *data)
{
    DebugObject_Access(&o->d_obj);
    
    // if we already sent one packet, stop
    if (o->sent) {
        return;
    }
    
    // set sent
    o->sent = 1;
    
    // write packet
    memcpy(data, o->packet, o->packet_len);
    
    // done
    PacketRecvInterface_Done(&o->output, o->packet_len);
}

void SinglePacketSource_Init (SinglePacketSource *o, uint8_t *packet, int packet_len, BPendingGroup *pg)
{
    ASSERT(packet_len >= 0)
    
    // init arguments
    o->packet = packet;
    o->packet_len = packet_len;
    
    // set not sent
    o->sent = 0;
    
    // init output
    PacketRecvInterface_Init(&o->output, o->packet_len, (PacketRecvInterface_handler_recv)output_handler_recv, o, pg);
    
    DebugObject_Init(&o->d_obj);
}

void SinglePacketSource_Free (SinglePacketSource *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free output
    PacketRecvInterface_Free(&o->output);
}

PacketRecvInterface * SinglePacketSource_GetOutput (SinglePacketSource *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}
