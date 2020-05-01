/**
 * @file PacketPassInactivityMonitor.h
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
 * A {@link PacketPassInterface} layer for detecting inactivity.
 */

#ifndef BADVPN_PACKETPASSINACTIVITYMONITOR_H
#define BADVPN_PACKETPASSINACTIVITYMONITOR_H

#include <base/DebugObject.h>
#include <system/BReactor.h>
#include <flow/PacketPassInterface.h>

/**
 * Handler function invoked when inactivity is detected.
 * It is guaranteed that the interfaces are in not sending state.
 *
 * @param user value given to {@link PacketPassInactivityMonitor_Init}
 */
typedef void (*PacketPassInactivityMonitor_handler) (void *user);

/**
 * A {@link PacketPassInterface} layer for detecting inactivity.
 * It reports inactivity to a user provided handler function.
 *
 * The object behaves like that:
 * ("timer set" means started with the given timeout whether if was running or not,
 * "timer unset" means stopped if it was running)
 *     - There is a timer.
 *     - The timer is set when the object is initialized.
 *     - When the input calls Send, the call is passed on to the output.
 *       If the output accepted the packet, the timer is set. If the output
 *       blocked the packet, the timer is unset.
 *     - When the output calls Done, the timer is set, and the call is
 *       passed on to the input.
 *     - When the input calls Cancel, the timer is set, and the call is
 *       passed on to the output.
 *     - When the timer expires, the timer is set, ant the user's handler
 *       function is invoked.
 */
typedef struct {
    DebugObject d_obj;
    PacketPassInterface *output;
    BReactor *reactor;
    PacketPassInactivityMonitor_handler handler;
    void *user;
    PacketPassInterface input;
    BTimer timer;
} PacketPassInactivityMonitor;

/**
 * Initializes the object.
 * See {@link PacketPassInactivityMonitor} for details.
 *
 * @param o the object
 * @param output output interface
 * @param reactor reactor we live in
 * @param interval timer value in milliseconds
 * @param handler handler function for reporting inactivity, or NULL to disable
 * @param user value passed to handler functions
 */
void PacketPassInactivityMonitor_Init (PacketPassInactivityMonitor *o, PacketPassInterface *output, BReactor *reactor, btime_t interval, PacketPassInactivityMonitor_handler handler, void *user);

/**
 * Frees the object.
 *
 * @param o the object
 */
void PacketPassInactivityMonitor_Free (PacketPassInactivityMonitor *o);

/**
 * Returns the input interface.
 * The MTU of the interface will be the same as of the output interface.
 * The interface supports cancel functionality if the output interface supports it.
 *
 * @param o the object
 * @return input interface
 */
PacketPassInterface * PacketPassInactivityMonitor_GetInput (PacketPassInactivityMonitor *o);

/**
 * Sets or removes the inactivity handler.
 *
 * @param o the object
 * @param handler handler function for reporting inactivity, or NULL to disable
 * @param user value passed to handler functions
 */
void PacketPassInactivityMonitor_SetHandler (PacketPassInactivityMonitor *o, PacketPassInactivityMonitor_handler handler, void *user);

/**
 * Sets the timer to expire immediately in order to force an inactivity report.
 * 
 * @param o the object
 */
void PacketPassInactivityMonitor_Force (PacketPassInactivityMonitor *o);

#endif
