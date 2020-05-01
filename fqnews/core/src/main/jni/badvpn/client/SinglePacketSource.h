/**
 * @file SinglePacketSource.h
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

#ifndef BADVPN_SINGLEPACKETSOURCE_H
#define BADVPN_SINGLEPACKETSOURCE_H

#include <base/DebugObject.h>
#include <flow/PacketRecvInterface.h>

/**
 * An object which provides a single packet through {@link PacketRecvInterface}.
 */
typedef struct {
    uint8_t *packet;
    int packet_len;
    int sent;
    PacketRecvInterface output;
    DebugObject d_obj;
} SinglePacketSource;

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param packet packet to provide to the output. Must stay available until the packet is provided.
 * @param packet_len length of packet. Must be >=0.
 * @param pg pending group we live in
 */
void SinglePacketSource_Init (SinglePacketSource *o, uint8_t *packet, int packet_len, BPendingGroup *pg);

/**
 * Frees the object.
 * 
 * @param o the object
 */
void SinglePacketSource_Free (SinglePacketSource *o);

/**
 * Returns the output interface.
 * The MTU of the interface will be packet_len.
 * 
 * @param o the object
 * @return output interface
 */
PacketRecvInterface * SinglePacketSource_GetOutput (SinglePacketSource *o);

#endif
