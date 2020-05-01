/**
 * @file PacketRecvBlocker.h
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
 * {@link PacketRecvInterface} layer which blocks all output recv calls and only
 * passes a single blocked call on to input when the user wants so.
 */

#ifndef BADVPN_FLOW_PACKETRECVBLOCKER_H
#define BADVPN_FLOW_PACKETRECVBLOCKER_H

#include <stdint.h>

#include <base/DebugObject.h>
#include <flow/PacketRecvInterface.h>

/**
 * {@link PacketRecvInterface} layer which blocks all output recv calls and only
 * passes a single blocked call on to input when the user wants so.
 */
typedef struct {
    PacketRecvInterface output;
    int out_have;
    uint8_t *out;
    int out_input_blocking;
    PacketRecvInterface *input;
    DebugObject d_obj;
} PacketRecvBlocker;

/**
 * Initializes the object.
 *
 * @param o the object
 * @param input input interface
 */
void PacketRecvBlocker_Init (PacketRecvBlocker *o, PacketRecvInterface *input, BPendingGroup *pg);

/**
 * Frees the object.
 *
 * @param o the object
 */
void PacketRecvBlocker_Free (PacketRecvBlocker *o);

/**
 * Returns the output interface.
 * The MTU of the output interface will be the same as of the input interface.
 *
 * @param o the object
 * @return output interface
 */
PacketRecvInterface * PacketRecvBlocker_GetOutput (PacketRecvBlocker *o);

/**
 * Passes a blocked output recv call to input if there is one and it has not
 * been passed yet. Otherwise it does nothing.
 * Must not be called from input Recv calls.
 * This function may invoke I/O.
 *
 * @param o the object
 */
void PacketRecvBlocker_AllowBlockedPacket (PacketRecvBlocker *o);

#endif
