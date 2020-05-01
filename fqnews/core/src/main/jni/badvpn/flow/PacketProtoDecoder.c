/**
 * @file PacketProtoDecoder.c
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
#include <string.h>

#include <misc/debug.h>
#include <misc/byteorder.h>
#include <misc/minmax.h>
#include <base/BLog.h>

#include <flow/PacketProtoDecoder.h>

#include <generated/blog_channel_PacketProtoDecoder.h>

static void process_data (PacketProtoDecoder *enc);
static void input_handler_done (PacketProtoDecoder *enc, int data_len);
static void output_handler_done (PacketProtoDecoder *enc);

void process_data (PacketProtoDecoder *enc)
{
    int was_error = 0;
    
    do {
        uint8_t *data = enc->buf + enc->buf_start;
        int left = enc->buf_used;
        
        // check if header was received
        if (left < sizeof(struct packetproto_header)) {
            break;
        }
        struct packetproto_header header;
        memcpy(&header, data, sizeof(header));
        data += sizeof(struct packetproto_header);
        left -= sizeof(struct packetproto_header);
        int data_len = ltoh16(header.len);
        
        // check data length
        if (data_len > enc->output_mtu) {
            BLog(BLOG_NOTICE, "error: packet too large");
            was_error = 1;
            break;
        }
        
        // check if whole packet was received
        if (left < data_len) {
            break;
        }
        
        // update buffer
        enc->buf_start += sizeof(struct packetproto_header) + data_len;
        enc->buf_used -= sizeof(struct packetproto_header) + data_len;
        
        // submit packet
        PacketPassInterface_Sender_Send(enc->output, data, data_len);
        return;
    } while (0);
    
    if (was_error) {
        // reset buffer
        enc->buf_start = 0;
        enc->buf_used = 0;
    } else {
        // if we reached the end of the buffer, wrap around to allow more data to be received
        if (enc->buf_start + enc->buf_used == enc->buf_size) {
            memmove(enc->buf, enc->buf + enc->buf_start, enc->buf_used);
            enc->buf_start = 0;
        }
    }
    
    // receive data
    StreamRecvInterface_Receiver_Recv(enc->input, enc->buf + (enc->buf_start + enc->buf_used), enc->buf_size - (enc->buf_start + enc->buf_used));
    
    // if we had error, report it
    if (was_error) {
        enc->handler_error(enc->user);
        return;
    }
}

static void input_handler_done (PacketProtoDecoder *enc, int data_len)
{
    ASSERT(data_len > 0)
    ASSERT(data_len <= enc->buf_size - (enc->buf_start + enc->buf_used))
    DebugObject_Access(&enc->d_obj);
    
    // update buffer
    enc->buf_used += data_len;
    
    // process data
    process_data(enc);
    return;
}

void output_handler_done (PacketProtoDecoder *enc)
{
    DebugObject_Access(&enc->d_obj);
    
    // process data
    process_data(enc);
    return;
}

int PacketProtoDecoder_Init (PacketProtoDecoder *enc, StreamRecvInterface *input, PacketPassInterface *output, BPendingGroup *pg, void *user, PacketProtoDecoder_handler_error handler_error)
{
    // init arguments
    enc->input = input;
    enc->output = output;
    enc->user = user;
    enc->handler_error = handler_error;
    
    // init input
    StreamRecvInterface_Receiver_Init(enc->input, (StreamRecvInterface_handler_done)input_handler_done, enc);
    
    // init output
    PacketPassInterface_Sender_Init(enc->output, (PacketPassInterface_handler_done)output_handler_done, enc);
    
    // set output MTU, limit by maximum payload size
    enc->output_mtu = bmin_int(PacketPassInterface_GetMTU(enc->output), PACKETPROTO_MAXPAYLOAD);
    
    // init buffer state
    enc->buf_size = PACKETPROTO_ENCLEN(enc->output_mtu);
    enc->buf_start = 0;
    enc->buf_used = 0;
    
    // allocate buffer
    if (!(enc->buf = (uint8_t *)malloc(enc->buf_size))) {
        goto fail0;
    }
    
    // start receiving
    StreamRecvInterface_Receiver_Recv(enc->input, enc->buf, enc->buf_size);
    
    DebugObject_Init(&enc->d_obj);
    
    return 1;
    
fail0:
    return 0;
}

void PacketProtoDecoder_Free (PacketProtoDecoder *enc)
{
    DebugObject_Free(&enc->d_obj);
    
    // free buffer
    free(enc->buf);
}

void PacketProtoDecoder_Reset (PacketProtoDecoder *enc)
{
    DebugObject_Access(&enc->d_obj);
    
    enc->buf_start += enc->buf_used;
    enc->buf_used = 0;
}
