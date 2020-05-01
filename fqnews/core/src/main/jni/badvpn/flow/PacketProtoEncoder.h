/**
 * @file PacketProtoEncoder.h
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
 * Object which encodes packets according to PacketProto.
 */

#ifndef BADVPN_FLOW_PACKETPROTOENCODER_H
#define BADVPN_FLOW_PACKETPROTOENCODER_H

#include <stdint.h>

#include <base/DebugObject.h>
#include <flow/PacketRecvInterface.h>

/**
 * Object which encodes packets according to PacketProto.
 *
 * Input is with {@link PacketRecvInterface}.
 * Output is with {@link PacketRecvInterface}.
 */
typedef struct {
    PacketRecvInterface *input;
    PacketRecvInterface output;
    uint8_t *output_packet;
    DebugObject d_obj;
} PacketProtoEncoder;

/**
 * Initializes the object.
 *
 * @param enc the object
 * @param input input interface. Its MTU must be <=PACKETPROTO_MAXPAYLOAD.
 * @param pg pending group
 */
void PacketProtoEncoder_Init (PacketProtoEncoder *enc, PacketRecvInterface *input, BPendingGroup *pg);

/**
 * Frees the object.
 *
 * @param enc the object
 */
void PacketProtoEncoder_Free (PacketProtoEncoder *enc);

/**
 * Returns the output interface.
 * The MTU of the output interface is PACKETPROTO_ENCLEN(MTU of input interface).
 *
 * @param enc the object
 * @return output interface
 */
PacketRecvInterface * PacketProtoEncoder_GetOutput (PacketProtoEncoder *enc);

#endif
