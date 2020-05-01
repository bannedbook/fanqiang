/**
 * @file StreamPacketSender.h
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

#ifndef BADVPN_STREAMPACKETSENDER_H
#define BADVPN_STREAMPACKETSENDER_H

#include <base/DebugObject.h>
#include <flow/StreamPassInterface.h>
#include <flow/PacketPassInterface.h>

/**
 * Object which breaks an input stream into output packets. The resulting
 * packets will have positive length, and, when concatenated, will form the
 * original stream.
 * 
 * Input is with {@link StreamPassInterface}.
 * Output is with {@link PacketPassInterface}.
 */
typedef struct {
    PacketPassInterface *output;
    int output_mtu;
    StreamPassInterface input;
    int sending_len;
    DebugObject d_obj;
} StreamPacketSender;

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param output output interface. Its MTU must be >0.
 * @param pg pending group we live in
 */
void StreamPacketSender_Init (StreamPacketSender *o, PacketPassInterface *output, BPendingGroup *pg);

/**
 * Frees the object.
 * 
 * @param o the object
 */
void StreamPacketSender_Free (StreamPacketSender *o);

/**
 * Returns the input interface.
 * 
 * @param o the object
 * @return input interface
 */
StreamPassInterface * StreamPacketSender_GetInput (StreamPacketSender *o);

#endif
