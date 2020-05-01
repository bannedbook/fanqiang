/**
 * @file PacketRouter.h
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
 * Object which simplifies routing packets to {@link RouteBuffer}'s from a
 * {@link PacketRecvInterface} input.
 */

#ifndef BADVPN_FLOW_PACKETROUTER_H
#define BADVPN_FLOW_PACKETROUTER_H

#include <base/DebugObject.h>
#include <base/BPending.h>
#include <flow/PacketRecvInterface.h>
#include <flow/RouteBuffer.h>

/**
 * Handler called when a packet is received, allowing the user to route it
 * to one or more buffers using {@link PacketRouter_Route}.
 * 
 * @param user as in {@link PacketRouter_Init}
 * @param buf the buffer for the packet. May be modified by the user.
 *            Will have space for mtu bytes. Only valid in the job context of
 *            this handler, until {@link PacketRouter_Route} is called successfully.
 * @param recv_len length of the input packet (located at recv_offset bytes offset)
 */
typedef void (*PacketRouter_handler) (void *user, uint8_t *buf, int recv_len);

/**
 * Object which simplifies routing packets to {@link RouteBuffer}'s from a
 * {@link PacketRecvInterface} input.
 * 
 * Packets are routed by calling {@link PacketRouter_Route} (possibly multiple times)
 * from the job context of the {@link PacketRouter_handler} handler.
 */
typedef struct {
    int mtu;
    int recv_offset;
    PacketRecvInterface *input;
    PacketRouter_handler handler;
    void *user;
    RouteBufferSource rbs;
    BPending next_job;
    DebugObject d_obj;
} PacketRouter;

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param mtu maximum packet size. Must be >=0. It will only be possible to route packets to
 *            {@link RouteBuffer}'s with the same MTU.
 * @param recv_offset offset from the beginning for receiving input packets.
 *                    Must be >=0 and <=mtu. The leading space should be initialized
 *                    by the user before routing a packet.
 * @param input input interface. Its MTU must be <= mtu - recv_offset.
 * @param handler handler called when a packet is received to allow the user to route it
 * @param user value passed to handler
 * @param pg pending group
 * @return 1 on success, 0 on failure
 */
int PacketRouter_Init (PacketRouter *o, int mtu, int recv_offset, PacketRecvInterface *input, PacketRouter_handler handler, void *user, BPendingGroup *pg) WARN_UNUSED;

/**
 * Frees the object.
 * 
 * @param o the object
 */
void PacketRouter_Free (PacketRouter *o);

/**
 * Routes the current packet to the given buffer.
 * Must be called from the job context of the {@link PacketRouter_handler} handler.
 * On success, copies part of the current packet to next one (regardless if next_buf
 * is provided or not; if not, copies before receiving another packet).
 * 
 * @param o the object
 * @param len total packet length (e.g. recv_offset + (recv_len from handler)).
 *            Must be >=0 and <=mtu.
 * @param output buffer to route to. Its MTU must be the same as of this object.
 * @param next_buf if not NULL, on success, will be set to the address of a new current
 *                 packet that can be routed. The pointer will be valid in the job context of
 *                 the calling handler, until this function is called successfully again
 *                 (as for the original pointer provided by the handler).
 * @param copy_offset Offset from the beginning for copying to the next packet.
 *                    Must be >=0 and <=mtu.
 * @param copy_len Number of bytes to copy from the old current
 *                 packet to the next one. Must be >=0 and <= mtu - copy_offset.
 * @return 1 on success, 0 on failure (buffer full)
 */
int PacketRouter_Route (PacketRouter *o, int len, RouteBuffer *output, uint8_t **next_buf, int copy_offset, int copy_len);

/**
 * Asserts that {@link PacketRouter_Route} can be called.
 * 
 * @param o the object
 */
void PacketRouter_AssertRoute (PacketRouter *o);

#endif
