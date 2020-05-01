/**
 * @file PacketStreamSender.h
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
 * Object which forwards packets obtained with {@link PacketPassInterface}
 * as a stream with {@link StreamPassInterface} (i.e. it concatenates them).
 */

#ifndef BADVPN_FLOW_PACKETSTREAMSENDER_H
#define BADVPN_FLOW_PACKETSTREAMSENDER_H

#include <stdint.h>

#include <base/DebugObject.h>
#include <flow/PacketPassInterface.h>
#include <flow/StreamPassInterface.h>

/**
 * Object which forwards packets obtained with {@link PacketPassInterface}
 * as a stream with {@link StreamPassInterface} (i.e. it concatenates them).
 */
typedef struct {
    DebugObject d_obj;
    PacketPassInterface input;
    StreamPassInterface *output;
    int in_len;
    uint8_t *in;
    int in_used;
} PacketStreamSender;

/**
 * Initializes the object.
 *
 * @param s the object
 * @param output output interface
 * @param mtu input MTU. Must be >=0.
 * @param pg pending group
 */
void PacketStreamSender_Init (PacketStreamSender *s, StreamPassInterface *output, int mtu, BPendingGroup *pg);

/**
 * Frees the object.
 *
 * @param s the object
 */
void PacketStreamSender_Free (PacketStreamSender *s);

/**
 * Returns the input interface.
 * Its MTU will be as in {@link PacketStreamSender_Init}.
 *
 * @param s the object
 * @return input interface
 */
PacketPassInterface * PacketStreamSender_GetInput (PacketStreamSender *s);

#endif
