/**
 * @file SinglePacketSender.h
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
 * A {@link PacketPassInterface} source which sends a single packet.
 */

#ifndef BADVPN_FLOW_SINGLEPACKETSENDER_H
#define BADVPN_FLOW_SINGLEPACKETSENDER_H

#include <stdint.h>

#include <misc/debugerror.h>
#include <base/DebugObject.h>
#include <flow/PacketPassInterface.h>

/**
 * Handler function called after the packet is sent.
 * The object must be freed from within this handler.
 * 
 * @param user as in {@link SinglePacketSender_Init}.
 */
typedef void (*SinglePacketSender_handler) (void *user);

/**
 * A {@link PacketPassInterface} source which sends a single packet.
 */
typedef struct {
    PacketPassInterface *output;
    SinglePacketSender_handler handler;
    void *user;
    DebugObject d_obj;
    DebugError d_err;
} SinglePacketSender;

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param packet packet to be sent. Must be available as long as the object exists.
 * @param packet_len length of the packet. Must be >=0 and <=(MTU of output).
 * @param output output interface
 * @param handler handler to call when the packet is sent
 * @param user value to pass to handler
 * @param pg pending group
 */
void SinglePacketSender_Init (SinglePacketSender *o, uint8_t *packet, int packet_len, PacketPassInterface *output, SinglePacketSender_handler handler, void *user, BPendingGroup *pg);

/**
 * Frees the object.
 * 
 * @param o the object
 */
void SinglePacketSender_Free (SinglePacketSender *o);

#endif
