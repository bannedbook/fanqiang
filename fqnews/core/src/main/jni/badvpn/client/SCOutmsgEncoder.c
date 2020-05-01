/**
 * @file SCOutmsgEncoder.c
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
#include <limits.h>
#include <string.h>

#include <misc/balign.h>
#include <misc/debug.h>
#include <misc/byteorder.h>

#include "SCOutmsgEncoder.h"

static void output_handler_recv (SCOutmsgEncoder *o, uint8_t *data)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->output_packet)
    ASSERT(data)
    
    // schedule receive
    o->output_packet = data;
    PacketRecvInterface_Receiver_Recv(o->input, o->output_packet + SCOUTMSG_OVERHEAD);
}

static void input_handler_done (SCOutmsgEncoder *o, int in_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->output_packet)
    
    // write SC header
    struct sc_header header;
    header.type = htol8(SCID_OUTMSG);
    memcpy(o->output_packet, &header, sizeof(header));
    
    // write outmsg
    struct sc_client_outmsg outmsg;
    outmsg.clientid = htol16(o->peer_id);
    memcpy(o->output_packet + sizeof(header), &outmsg, sizeof(outmsg));
    
    // finish output packet
    o->output_packet = NULL;
    PacketRecvInterface_Done(&o->output, SCOUTMSG_OVERHEAD + in_len);
}

void SCOutmsgEncoder_Init (SCOutmsgEncoder *o, peerid_t peer_id, PacketRecvInterface *input, BPendingGroup *pg)
{
    ASSERT(PacketRecvInterface_GetMTU(input) <= INT_MAX - SCOUTMSG_OVERHEAD)
    
    // init arguments
    o->peer_id = peer_id;
    o->input = input;
    
    // init input
    PacketRecvInterface_Receiver_Init(o->input, (PacketRecvInterface_handler_done)input_handler_done, o);
    
    // init output
    PacketRecvInterface_Init(&o->output, SCOUTMSG_OVERHEAD + PacketRecvInterface_GetMTU(o->input), (PacketRecvInterface_handler_recv)output_handler_recv, o, pg);
    
    // set no output packet
    o->output_packet = NULL;
    
    DebugObject_Init(&o->d_obj);
}

void SCOutmsgEncoder_Free (SCOutmsgEncoder *o)
{
    DebugObject_Free(&o->d_obj);

    // free input
    PacketRecvInterface_Free(&o->output);
}

PacketRecvInterface * SCOutmsgEncoder_GetOutput (SCOutmsgEncoder *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}
