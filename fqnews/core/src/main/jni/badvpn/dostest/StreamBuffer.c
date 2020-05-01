/**
 * @file StreamBuffer.c
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

#include <misc/balloc.h>
#include <misc/minmax.h>

#include "StreamBuffer.h"

// called when receive operation is complete
static void input_handler_done (void *vo, int data_len)
{
    StreamBuffer *o = (StreamBuffer *)vo;
    ASSERT(data_len > 0)
    ASSERT(data_len <= o->buf_size - (o->buf_start + o->buf_used))
    
    // remember if buffer was empty
    int was_empty = (o->buf_used == 0);
    
    // increment buf_used by the amount that was received
    o->buf_used += data_len;
    
    // start another receive operation unless buffer is full
    if (o->buf_used < o->buf_size - o->buf_start) {
        int end = o->buf_start + o->buf_used;
        StreamRecvInterface_Receiver_Recv(o->input, o->buf + end, o->buf_size - end);
    }
    else if (o->buf_used < o->buf_size) {
        // wrap around
        StreamRecvInterface_Receiver_Recv(o->input, o->buf, o->buf_start);
    }
    
    // if buffer was empty before, start send operation
    if (was_empty) {
        StreamPassInterface_Sender_Send(o->output, o->buf + o->buf_start, o->buf_used);
    }
}

// called when send operation is complete
static void output_handler_done (void *vo, int data_len)
{
    StreamBuffer *o = (StreamBuffer *)vo;
    ASSERT(data_len > 0)
    ASSERT(data_len <= o->buf_used)
    ASSERT(data_len <= o->buf_size - o->buf_start)
    
    // remember if buffer was full
    int was_full = (o->buf_used == o->buf_size);
    
    // increment buf_start and decrement buf_used by the
    // amount that was sent
    o->buf_start += data_len;
    o->buf_used -= data_len;
    
    // wrap around buf_start
    if (o->buf_start == o->buf_size) {
        o->buf_start = 0;
    }
    
    // start receive operation if buffer was full
    if (was_full) {
        int end;
        int avail;
        if (o->buf_used >= o->buf_size - o->buf_start) {
            end = o->buf_used - (o->buf_size - o->buf_start);
            avail = o->buf_start - end;
        } else {
            end = o->buf_start + o->buf_used;
            avail = o->buf_size - end;
        }
        StreamRecvInterface_Receiver_Recv(o->input, o->buf + end, avail);
    }
    
    // start another receive send unless buffer is empty
    if (o->buf_used > 0) {
        int to_send = bmin_int(o->buf_used, o->buf_size - o->buf_start);
        StreamPassInterface_Sender_Send(o->output, o->buf + o->buf_start, to_send);
    }
}

int StreamBuffer_Init (StreamBuffer *o, int buf_size, StreamRecvInterface *input, StreamPassInterface *output)
{
    ASSERT(buf_size > 0)
    ASSERT(input)
    ASSERT(output)
    
    // remember arguments
    o->buf_size = buf_size;
    o->input = input;
    o->output = output;
    
    // allocate buffer memory
    o->buf = (uint8_t *)BAllocSize(bsize_fromint(o->buf_size));
    if (!o->buf) {
        goto fail0;
    }
    
    // set initial buffer state
    o->buf_start = 0;
    o->buf_used = 0;
    
    // set receive and send done callbacks
    StreamRecvInterface_Receiver_Init(o->input, input_handler_done, o);
    StreamPassInterface_Sender_Init(o->output, output_handler_done, o);
    
    // start receive operation
    StreamRecvInterface_Receiver_Recv(o->input, o->buf, o->buf_size);
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail0:
    return 0;
}

void StreamBuffer_Free (StreamBuffer *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free buffer memory
    BFree(o->buf);
}
