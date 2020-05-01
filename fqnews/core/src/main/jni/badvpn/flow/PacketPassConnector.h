/**
 * @file PacketPassConnector.h
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
 * A {@link PacketPassInterface} layer which allows the output to be
 * connected and disconnected on the fly.
 */

#ifndef BADVPN_FLOW_PACKETPASSCONNECTOR_H
#define BADVPN_FLOW_PACKETPASSCONNECTOR_H

#include <stdint.h>

#include <base/DebugObject.h>
#include <flow/PacketPassInterface.h>

/**
 * A {@link PacketPassInterface} layer which allows the output to be
 * connected and disconnected on the fly.
 */
typedef struct {
    PacketPassInterface input;
    int input_mtu;
    int in_len;
    uint8_t *in;
    PacketPassInterface *output;
    DebugObject d_obj;
} PacketPassConnector;

/**
 * Initializes the object.
 * The object is initialized in not connected state.
 *
 * @param o the object
 * @param mtu maximum input packet size. Must be >=0.
 * @param pg pending group
 */
void PacketPassConnector_Init (PacketPassConnector *o, int mtu, BPendingGroup *pg);

/**
 * Frees the object.
 *
 * @param o the object
 */
void PacketPassConnector_Free (PacketPassConnector *o);

/**
 * Returns the input interface.
 * The MTU of the interface will be as in {@link PacketPassConnector_Init}.
 *
 * @param o the object
 * @return input interface
 */
PacketPassInterface * PacketPassConnector_GetInput (PacketPassConnector *o);

/**
 * Connects output.
 * The object must be in not connected state.
 * The object enters connected state.
 *
 * @param o the object
 * @param output output to connect. Its MTU must be >= MTU specified in
 *               {@link PacketPassConnector_Init}.
 */
void PacketPassConnector_ConnectOutput (PacketPassConnector *o, PacketPassInterface *output);

/**
 * Disconnects output.
 * The object must be in connected state.
 * The object enters not connected state.
 *
 * @param o the object
 */
void PacketPassConnector_DisconnectOutput (PacketPassConnector *o);

#endif
