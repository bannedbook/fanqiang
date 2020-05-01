/**
 * @file RouteBuffer.h
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
 * Packet buffer for zero-copy packet routing.
 */

#ifndef BADVPN_FLOW_ROUTEBUFFER_H
#define BADVPN_FLOW_ROUTEBUFFER_H

#include <misc/debug.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <flow/PacketPassInterface.h>

struct RouteBuffer_packet {
    LinkedList1Node node;
    int len;
};

/**
 * Packet buffer for zero-copy packet routing.
 * 
 * Packets are buffered using {@link RouteBufferSource} objects.
 */
typedef struct {
    int mtu;
    PacketPassInterface *output;
    LinkedList1 packets_free;
    LinkedList1 packets_used;
    DebugObject d_obj;
} RouteBuffer;

/**
 * Object through which packets are buffered into {@link RouteBuffer} objects.
 * 
 * A packet is routed by calling {@link RouteBufferSource_Pointer}, writing it to
 * the returned address, then calling {@link RouteBufferSource_Route}.
 */
typedef struct {
    int mtu;
    struct RouteBuffer_packet *current_packet;
    DebugObject d_obj;
} RouteBufferSource;

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param mtu maximum packet size. Must be >=0. It will only be possible to route packets to this buffer
 *            from {@link RouteBufferSource}.s with the same MTU.
 * @param output output interface. Its MTU must be >=mtu.
 * @param buf_size size of the buffer in number of packet. Must be >0.
 * @return 1 on success, 0 on failure
 */
int RouteBuffer_Init (RouteBuffer *o, int mtu, PacketPassInterface *output, int buf_size) WARN_UNUSED;

/**
 * Frees the object.
 */
void RouteBuffer_Free (RouteBuffer *o);

/**
 * Retuns the buffer's MTU (mtu argument to {@link RouteBuffer_Init}).
 * 
 * @return MTU
 */
int RouteBuffer_GetMTU (RouteBuffer *o);

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param mtu maximum packet size. Must be >=0. The object will only be able to route packets
 *            to {@link RouteBuffer}'s with the same MTU.
 * @return 1 on success, 0 on failure
 */
int RouteBufferSource_Init (RouteBufferSource *o, int mtu) WARN_UNUSED;

/**
 * Frees the object.
 * 
 * @param o the object
 */
void RouteBufferSource_Free (RouteBufferSource *o);

/**
 * Returns a pointer to the current packet.
 * The pointed to memory area will have space for MTU bytes.
 * The pointer is only valid until {@link RouteBufferSource_Route} succeeds.
 * 
 * @param o the object
 * @return pointer to the current packet
 */
uint8_t * RouteBufferSource_Pointer (RouteBufferSource *o);

/**
 * Routes the current packet to a given buffer.
 * On success, this invalidates the pointer previously returned from
 * {@link RouteBufferSource_Pointer}.
 * 
 * @param o the object
 * @param len length of the packet. Must be >=0 and <=MTU.
 * @param b buffer to route to. Its MTU must equal this object's MTU.
 * @param copy_offset Offset from the beginning for copying. Must be >=0 and
 *                    <=mtu.
 * @param copy_len Number of bytes to copy from the old current packet to the new one.
 *                 Must be >=0 and <= mtu - copy_offset.
 * @return 1 on success, 0 on failure
 */
int RouteBufferSource_Route (RouteBufferSource *o, int len, RouteBuffer *b, int copy_offset, int copy_len);

#endif
