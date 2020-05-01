/**
 * @file KeepaliveIO.h
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
 * A {@link PacketPassInterface} layer for sending keep-alive packets.
 */

#ifndef BADVPN_KEEPALIVEIO
#define BADVPN_KEEPALIVEIO

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <system/BReactor.h>
#include <flow/PacketPassInterface.h>
#include <flow/PacketRecvInterface.h>
#include <flow/PacketPassPriorityQueue.h>
#include <flow/SinglePacketBuffer.h>
#include <flow/PacketRecvBlocker.h>
#include <flowextra/PacketPassInactivityMonitor.h>

/**
 * A {@link PacketPassInterface} layer for sending keep-alive packets.
 */
typedef struct {
    BReactor *reactor;
    PacketPassInactivityMonitor kasender;
    PacketPassPriorityQueue queue;
    PacketPassPriorityQueueFlow user_qflow;
    PacketPassPriorityQueueFlow ka_qflow;
    SinglePacketBuffer ka_buffer;
    PacketRecvBlocker ka_blocker;
    DebugObject d_obj;
} KeepaliveIO;

/**
 * Initializes the object.
 *
 * @param o the object
 * @param reactor reactor we live in
 * @param output output interface
 * @param keepalive_input keepalive input interface. Its MTU must be <= MTU of output.
 * @param keepalive_interval_ms keepalive interval in milliseconds. Must be >0.
 * @return 1 on success, 0 on failure
 */
int KeepaliveIO_Init (KeepaliveIO *o, BReactor *reactor, PacketPassInterface *output, PacketRecvInterface *keepalive_input, btime_t keepalive_interval_ms) WARN_UNUSED;

/**
 * Frees the object.
 *
 * @param o the object
 */
void KeepaliveIO_Free (KeepaliveIO *o);

/**
 * Returns the input interface.
 *
 * @param o the object
 * @return input interface
 */
PacketPassInterface * KeepaliveIO_GetInput (KeepaliveIO *o);

#endif
