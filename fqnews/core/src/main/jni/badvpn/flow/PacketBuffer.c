/**
 * @file PacketBuffer.c
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

#include <stdlib.h>

#include <misc/debug.h>
#include <misc/balloc.h>

#include <flow/PacketBuffer.h>

static void input_handler_done (PacketBuffer *buf, int in_len);
static void output_handler_done (PacketBuffer *buf);

void input_handler_done (PacketBuffer *buf, int in_len)
{
    ASSERT(in_len >= 0)
    ASSERT(in_len <= buf->input_mtu)
    DebugObject_Access(&buf->d_obj);
    
    // remember if buffer is empty
    int was_empty = (buf->buf.output_avail < 0);
    
    // submit packet to buffer
    ChunkBuffer2_SubmitPacket(&buf->buf, in_len);
    
    // if there is space, schedule receive
    if (buf->buf.input_avail >= buf->input_mtu) {
        PacketRecvInterface_Receiver_Recv(buf->input, buf->buf.input_dest);
    }
    
    // if buffer was empty, schedule send
    if (was_empty) {
        PacketPassInterface_Sender_Send(buf->output, buf->buf.output_dest, buf->buf.output_avail);
    }
}

void output_handler_done (PacketBuffer *buf)
{
    DebugObject_Access(&buf->d_obj);
    
    // remember if buffer is full
    int was_full = (buf->buf.input_avail < buf->input_mtu);
    
    // remove packet from buffer
    ChunkBuffer2_ConsumePacket(&buf->buf);
    
    // if buffer was full and there is space, schedule receive
    if (was_full && buf->buf.input_avail >= buf->input_mtu) {
        PacketRecvInterface_Receiver_Recv(buf->input, buf->buf.input_dest);
    }
    
    // if there is more data, schedule send
    if (buf->buf.output_avail >= 0) {
        PacketPassInterface_Sender_Send(buf->output, buf->buf.output_dest, buf->buf.output_avail);
    }
}

int PacketBuffer_Init (PacketBuffer *buf, PacketRecvInterface *input, PacketPassInterface *output, int num_packets, BPendingGroup *pg)
{
    ASSERT(PacketPassInterface_GetMTU(output) >= PacketRecvInterface_GetMTU(input))
    ASSERT(num_packets > 0)
    
    // init arguments
    buf->input = input;
    buf->output = output;
    
    // init input
    PacketRecvInterface_Receiver_Init(buf->input, (PacketRecvInterface_handler_done)input_handler_done, buf);
    
    // set input MTU
    buf->input_mtu = PacketRecvInterface_GetMTU(buf->input);
    
    // init output
    PacketPassInterface_Sender_Init(buf->output, (PacketPassInterface_handler_done)output_handler_done, buf);
    
    // allocate buffer
    int num_blocks = ChunkBuffer2_calc_blocks(buf->input_mtu, num_packets);
    if (num_blocks < 0) {
        goto fail0;
    }
    if (!(buf->buf_data = (struct ChunkBuffer2_block *)BAllocArray(num_blocks, sizeof(buf->buf_data[0])))) {
        goto fail0;
    }
    
    // init buffer
    ChunkBuffer2_Init(&buf->buf, buf->buf_data, num_blocks, buf->input_mtu);
    
    // schedule receive
    PacketRecvInterface_Receiver_Recv(buf->input, buf->buf.input_dest);
    
    DebugObject_Init(&buf->d_obj);
    
    return 1;
    
fail0:
    return 0;
}

void PacketBuffer_Free (PacketBuffer *buf)
{
    DebugObject_Free(&buf->d_obj);
    
    // free buffer
    BFree(buf->buf_data);
}
