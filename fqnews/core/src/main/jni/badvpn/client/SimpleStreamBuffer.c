/**
 * @file SimpleStreamBuffer.c
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
#include <stddef.h>

#include <misc/balloc.h>
#include <misc/minmax.h>

#include "SimpleStreamBuffer.h"

static void try_output (SimpleStreamBuffer *o)
{
    ASSERT(o->output_data_len > 0)
    
    // calculate number of bytes to output
    int bytes = bmin_int(o->output_data_len, o->buf_used);
    if (bytes == 0) {
        return;
    }
    
    // copy bytes to output
    memcpy(o->output_data, o->buf, bytes);
    
    // shift buffer
    memmove(o->buf, o->buf + bytes, o->buf_used - bytes);
    o->buf_used -= bytes;
    
    // forget data
    o->output_data_len = -1;
    
    // done
    StreamRecvInterface_Done(&o->output, bytes);
}

static void output_handler_recv (SimpleStreamBuffer *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->output_data_len == -1)
    ASSERT(data)
    ASSERT(data_len > 0)
    
    // remember data
    o->output_data = data;
    o->output_data_len = data_len;
    
    try_output(o);
}

int SimpleStreamBuffer_Init (SimpleStreamBuffer *o, int buf_size, BPendingGroup *pg)
{
    ASSERT(buf_size > 0)
    
    // init arguments
    o->buf_size = buf_size;
    
    // init output
    StreamRecvInterface_Init(&o->output, (StreamRecvInterface_handler_recv)output_handler_recv, o, pg);
    
    // allocate buffer
    if (!(o->buf = (uint8_t *)BAlloc(buf_size))) {
        goto fail1;
    }
    
    // init buffer state
    o->buf_used = 0;
    
    // set no output data
    o->output_data_len = -1;
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    StreamRecvInterface_Free(&o->output);
    return 0;
}

void SimpleStreamBuffer_Free (SimpleStreamBuffer *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free buffer
    BFree(o->buf);
    
    // free output
    StreamRecvInterface_Free(&o->output);
}

StreamRecvInterface * SimpleStreamBuffer_GetOutput (SimpleStreamBuffer *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}

int SimpleStreamBuffer_Write (SimpleStreamBuffer *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(data_len >= 0)
    
    if (data_len > o->buf_size - o->buf_used) {
        return 0;
    }
    
    // copy to buffer
    memcpy(o->buf + o->buf_used, data, data_len);
    
    // update buffer state
    o->buf_used += data_len;
    
    // continue outputting
    if (o->output_data_len > 0) {
        try_output(o);
    }
    
    return 1;
}
