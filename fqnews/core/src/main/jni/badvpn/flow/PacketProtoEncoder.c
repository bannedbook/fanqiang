/**
 * @file PacketProtoEncoder.c
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
#include <string.h>

#include <protocol/packetproto.h>
#include <misc/balign.h>
#include <misc/debug.h>
#include <misc/byteorder.h>

#include <flow/PacketProtoEncoder.h>

static void output_handler_recv (PacketProtoEncoder *enc, uint8_t *data)
{
    ASSERT(!enc->output_packet)
    ASSERT(data)
    DebugObject_Access(&enc->d_obj);
    
    // schedule receive
    enc->output_packet = data;
    PacketRecvInterface_Receiver_Recv(enc->input, enc->output_packet + sizeof(struct packetproto_header));
}

static void input_handler_done (PacketProtoEncoder *enc, int in_len)
{
    ASSERT(enc->output_packet)
    DebugObject_Access(&enc->d_obj);
    
    // write length
    struct packetproto_header pp;
    pp.len = htol16(in_len);
    memcpy(enc->output_packet, &pp, sizeof(pp));
    
    // finish output packet
    enc->output_packet = NULL;
    PacketRecvInterface_Done(&enc->output, PACKETPROTO_ENCLEN(in_len));
}

void PacketProtoEncoder_Init (PacketProtoEncoder *enc, PacketRecvInterface *input, BPendingGroup *pg)
{
    ASSERT(PacketRecvInterface_GetMTU(input) <= PACKETPROTO_MAXPAYLOAD)
    
    // init arguments
    enc->input = input;
    
    // init input
    PacketRecvInterface_Receiver_Init(enc->input, (PacketRecvInterface_handler_done)input_handler_done, enc);
    
    // init output
    PacketRecvInterface_Init(
        &enc->output, PACKETPROTO_ENCLEN(PacketRecvInterface_GetMTU(enc->input)),
        (PacketRecvInterface_handler_recv)output_handler_recv, enc, pg
    );
    
    // set no output packet
    enc->output_packet = NULL;
    
    DebugObject_Init(&enc->d_obj);
}

void PacketProtoEncoder_Free (PacketProtoEncoder *enc)
{
    DebugObject_Free(&enc->d_obj);

    // free input
    PacketRecvInterface_Free(&enc->output);
}

PacketRecvInterface * PacketProtoEncoder_GetOutput (PacketProtoEncoder *enc)
{
    DebugObject_Access(&enc->d_obj);
    
    return &enc->output;
}
