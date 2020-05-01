/**
 * @file PacketPassNotifier.h
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
 * A {@link PacketPassInterface} layer which calles a handler function before
 * passing a packet from input to output.
 */

#ifndef BADVPN_FLOW_PACKETPASSNOTIFIER_H
#define BADVPN_FLOW_PACKETPASSNOTIFIER_H

#include <stdint.h>

#include <base/DebugObject.h>
#include <flow/PacketPassInterface.h>

/**
 * Handler function called when input calls Send, but before the call is passed on to output.
 * 
 * @param user value specified in {@link PacketPassNotifier_SetHandler}
 * @param data packet provided by input
 * @param data_len size of the packet
 */
typedef void (*PacketPassNotifier_handler_notify) (void *user, uint8_t *data, int data_len);

/**
 * A {@link PacketPassInterface} layer which calles a handler function before
 * passing a packet from input to output.
 */
typedef struct {
    PacketPassInterface input;
    PacketPassInterface *output;
    PacketPassNotifier_handler_notify handler;
    void *handler_user;
    DebugObject d_obj;
} PacketPassNotifier;

/**
 * Initializes the object.
 *
 * @param o the object
 * @param output output interface
 * @param pg pending group
 */
void PacketPassNotifier_Init (PacketPassNotifier *o, PacketPassInterface *output, BPendingGroup *pg);

/**
 * Frees the object.
 *
 * @param o the object
 */
void PacketPassNotifier_Free (PacketPassNotifier *o);

/**
 * Returns the input interface.
 * The MTU of the interface will be the same as of the output interface.
 * The interface supports cancel functionality if the output interface supports it.
 *
 * @param o the object
 * @return input interface
 */
PacketPassInterface * PacketPassNotifier_GetInput (PacketPassNotifier *o);

/**
 * Configures a handler function to call before passing input packets to output.
 *
 * @param o the object
 * @param handler handler function, or NULL to disable.
 * @param user value to pass to handler function. Ignored if handler is NULL.
 */
void PacketPassNotifier_SetHandler (PacketPassNotifier *o, PacketPassNotifier_handler_notify handler, void *user);

#endif
