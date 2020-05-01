/**
 * @file PacketProtoFlow.c
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

#include <protocol/packetproto.h>
#include <misc/debug.h>

#include <flow/PacketProtoFlow.h>

int PacketProtoFlow_Init (PacketProtoFlow *o, int input_mtu, int num_packets, PacketPassInterface *output, BPendingGroup *pg)
{
    ASSERT(input_mtu >= 0)
    ASSERT(input_mtu <= PACKETPROTO_MAXPAYLOAD)
    ASSERT(num_packets > 0)
    ASSERT(PacketPassInterface_GetMTU(output) >= PACKETPROTO_ENCLEN(input_mtu))
    
    // init async input
    BufferWriter_Init(&o->ainput, input_mtu, pg);
    
    // init encoder
    PacketProtoEncoder_Init(&o->encoder, BufferWriter_GetOutput(&o->ainput), pg);
    
    // init buffer
    if (!PacketBuffer_Init(&o->buffer, PacketProtoEncoder_GetOutput(&o->encoder), output, num_packets, pg)) {
        goto fail0;
    }
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail0:
    PacketProtoEncoder_Free(&o->encoder);
    BufferWriter_Free(&o->ainput);
    return 0;
}

void PacketProtoFlow_Free (PacketProtoFlow *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free buffer
    PacketBuffer_Free(&o->buffer);
    
    // free encoder
    PacketProtoEncoder_Free(&o->encoder);
    
    // free async input
    BufferWriter_Free(&o->ainput);
}

BufferWriter * PacketProtoFlow_GetInput (PacketProtoFlow *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->ainput;
}
