/**
 * @file PacketBuffer.h
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
 * 
 * @section DESCRIPTION
 * 
 * Packet buffer with {@link PacketRecvInterface} input and {@link PacketPassInterface} output.
 */

#ifndef BADVPN_FLOW_PACKETBUFFER_H
#define BADVPN_FLOW_PACKETBUFFER_H

#include <stdint.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <structure/ChunkBuffer2.h>
#include <flow/PacketRecvInterface.h>
#include <flow/PacketPassInterface.h>

/**
 * Packet buffer with {@link PacketRecvInterface} input and {@link PacketPassInterface} output.
 */
typedef struct {
    DebugObject d_obj;
    PacketRecvInterface *input;
    int input_mtu;
    PacketPassInterface *output;
    struct ChunkBuffer2_block *buf_data;
    ChunkBuffer2 buf;
} PacketBuffer;

/**
 * Initializes the buffer.
 * Output MTU must be >= input MTU.
 *
 * @param buf the object
 * @param input input interface
 * @param output output interface
 * @param num_packets minimum number of packets the buffer must hold. Must be >0.
 * @param pg pending group
 * @return 1 on success, 0 on failure
 */
int PacketBuffer_Init (PacketBuffer *buf, PacketRecvInterface *input, PacketPassInterface *output, int num_packets, BPendingGroup *pg) WARN_UNUSED;

/**
 * Frees the buffer.
 *
 * @param buf the object
 */
void PacketBuffer_Free (PacketBuffer *buf);

#endif
