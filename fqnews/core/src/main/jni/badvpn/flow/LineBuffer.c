/**
 * @file LineBuffer.c
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

#include <base/BLog.h>

#include <flow/LineBuffer.h>

#include <generated/blog_channel_LineBuffer.h>

static void input_handler_done (LineBuffer *o, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(data_len > 0)
    ASSERT(data_len <= o->buf_size - o->buf_used)
    
    // update buffer
    o->buf_used += data_len;
    
    // look for newline
    int i;
    for (i = o->buf_used - data_len; i < o->buf_used; i++) {
        if (o->buf[i] == o->nl_char) {
            break;
        }
    }
    
    if (i < o->buf_used || o->buf_used == o->buf_size) {
        if (i == o->buf_used) {
            BLog(BLOG_WARNING, "line too long");
        }
        
        // pass to output
        o->buf_consumed = (i < o->buf_used ? i + 1 : i);
        PacketPassInterface_Sender_Send(o->output, o->buf, o->buf_consumed);
    } else {
        // receive more data
        StreamRecvInterface_Receiver_Recv(o->input, o->buf + o->buf_used, o->buf_size - o->buf_used);
    }
}

static void output_handler_done (LineBuffer *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->buf_consumed > 0)
    ASSERT(o->buf_consumed <= o->buf_used)
    
    // update buffer
    memmove(o->buf, o->buf + o->buf_consumed, o->buf_used - o->buf_consumed);
    o->buf_used -= o->buf_consumed;
    
    // look for newline
    int i;
    for (i = 0; i < o->buf_used; i++) {
        if (o->buf[i] == o->nl_char) {
            break;
        }
    }
    
    if (i < o->buf_used || o->buf_used == o->buf_size) {
        // pass to output
        o->buf_consumed = (i < o->buf_used ? i + 1 : i);
        PacketPassInterface_Sender_Send(o->output, o->buf, o->buf_consumed);
    } else {
        // receive more data
        StreamRecvInterface_Receiver_Recv(o->input, o->buf + o->buf_used, o->buf_size - o->buf_used);
    }
}

int LineBuffer_Init (LineBuffer *o, StreamRecvInterface *input, PacketPassInterface *output, int buf_size, uint8_t nl_char)
{
    ASSERT(buf_size > 0)
    ASSERT(PacketPassInterface_GetMTU(output) >= buf_size)
    
    // init arguments
    o->input = input;
    o->output = output;
    o->buf_size = buf_size;
    o->nl_char = nl_char;
    
    // init input
    StreamRecvInterface_Receiver_Init(o->input, (StreamRecvInterface_handler_done)input_handler_done, o);
    
    // init output
    PacketPassInterface_Sender_Init(o->output, (PacketPassInterface_handler_done)output_handler_done, o);
    
    // set buffer empty
    o->buf_used = 0;
    
    // allocate buffer
    if (!(o->buf = (uint8_t *)malloc(o->buf_size))) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // start receiving
    StreamRecvInterface_Receiver_Recv(o->input, o->buf, o->buf_size);
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail0:
    return 0;
}

void LineBuffer_Free (LineBuffer *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free buffer
    free(o->buf);
}
