/**
 * @file PacketProtoFlow.h
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
 * Buffer which encodes packets with PacketProto, with {@link BufferWriter}
 * input and {@link PacketPassInterface} output.
 */

#ifndef BADVPN_FLOW_PACKETPROTOFLOW_H
#define BADVPN_FLOW_PACKETPROTOFLOW_H

#include <misc/debug.h>

#include <base/DebugObject.h>
#include <flow/BufferWriter.h>
#include <flow/PacketProtoEncoder.h>
#include <flow/PacketBuffer.h>

/**
 * Buffer which encodes packets with PacketProto, with {@link BufferWriter}
 * input and {@link PacketPassInterface} output.
 */
typedef struct {
    BufferWriter ainput;
    PacketProtoEncoder encoder;
    PacketBuffer buffer;
    DebugObject d_obj;
} PacketProtoFlow;

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param input_mtu maximum input packet size. Must be >=0 and <=PACKETPROTO_MAXPAYLOAD.
 * @param num_packets minimum number of packets the buffer should hold. Must be >0.
 * @param output output interface. Its MTU must be >=PACKETPROTO_ENCLEN(input_mtu).
 * @param pg pending group
 * @return 1 on success, 0 on failure
 */
int PacketProtoFlow_Init (PacketProtoFlow *o, int input_mtu, int num_packets, PacketPassInterface *output, BPendingGroup *pg) WARN_UNUSED;

/**
 * Frees the object.
 * 
 * @param o the object
 */
void PacketProtoFlow_Free (PacketProtoFlow *o);

/**
 * Returns the input interface.
 * 
 * @param o the object
 * @return input interface
 */
BufferWriter * PacketProtoFlow_GetInput (PacketProtoFlow *o);

#endif
