/**
 * @file BufferWriter.c
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

#include <misc/debug.h>

#include <flow/BufferWriter.h>

static void output_handler_recv (BufferWriter *o, uint8_t *data)
{
    ASSERT(!o->out_have)
    
    // set output packet
    o->out_have = 1;
    o->out = data;
}

void BufferWriter_Init (BufferWriter *o, int mtu, BPendingGroup *pg)
{
    ASSERT(mtu >= 0)
    
    // init output
    PacketRecvInterface_Init(&o->recv_interface, mtu, (PacketRecvInterface_handler_recv)output_handler_recv, o, pg);
    
    // set no output packet
    o->out_have = 0;
    
    DebugObject_Init(&o->d_obj);
    #ifndef NDEBUG
    o->d_mtu = mtu;
    o->d_writing = 0;
    #endif
}

void BufferWriter_Free (BufferWriter *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free output
    PacketRecvInterface_Free(&o->recv_interface);
}

PacketRecvInterface * BufferWriter_GetOutput (BufferWriter *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->recv_interface;
}

int BufferWriter_StartPacket (BufferWriter *o, uint8_t **buf)
{
    ASSERT(!o->d_writing)
    DebugObject_Access(&o->d_obj);
    
    if (!o->out_have) {
        return 0;
    }
    
    if (buf) {
        *buf = o->out;
    }
    
    #ifndef NDEBUG
    o->d_writing = 1;
    #endif
    
    return 1;
}

void BufferWriter_EndPacket (BufferWriter *o, int len)
{
    ASSERT(len >= 0)
    ASSERT(len <= o->d_mtu)
    ASSERT(o->out_have)
    ASSERT(o->d_writing)
    DebugObject_Access(&o->d_obj);
    
    // set no output packet
    o->out_have = 0;
    
    // finish packet
    PacketRecvInterface_Done(&o->recv_interface, len);
    
    #ifndef NDEBUG
    o->d_writing = 0;
    #endif
}
