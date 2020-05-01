/**
 * @file BufferWriter.h
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
 * Object for writing packets to a {@link PacketRecvInterface} client
 * in a best-effort fashion.
 */

#ifndef BADVPN_FLOW_BUFFERWRITER_H
#define BADVPN_FLOW_BUFFERWRITER_H

#include <stdint.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <flow/PacketRecvInterface.h>

/**
 * Object for writing packets to a {@link PacketRecvInterface} client
 * in a best-effort fashion.
 */
typedef struct {
    PacketRecvInterface recv_interface;
    int out_have;
    uint8_t *out;
    DebugObject d_obj;
    #ifndef NDEBUG
    int d_mtu;
    int d_writing;
    #endif
} BufferWriter;

/**
 * Initializes the object.
 * The object is initialized in not writing state.
 *
 * @param o the object
 * @param mtu maximum input packet length
 * @param pg pending group
 */
void BufferWriter_Init (BufferWriter *o, int mtu, BPendingGroup *pg);

/**
 * Frees the object.
 *
 * @param o the object
 */
void BufferWriter_Free (BufferWriter *o);

/**
 * Returns the output interface.
 *
 * @param o the object
 * @return output interface
 */
PacketRecvInterface * BufferWriter_GetOutput (BufferWriter *o);

/**
 * Attempts to provide a memory location for writing a packet.
 * The object must be in not writing state.
 * On success, the object enters writing state.
 * 
 * @param o the object
 * @param buf if not NULL, on success, the memory location will be stored here.
 *            It will have space for MTU bytes.
 * @return 1 on success, 0 on failure
 */
int BufferWriter_StartPacket (BufferWriter *o, uint8_t **buf) WARN_UNUSED;

/**
 * Submits a packet written to the buffer.
 * The object must be in writing state.
 * Yhe object enters not writing state.
 * 
 * @param o the object
 * @param len length of the packet that was written. Must be >=0 and
 *            <=MTU.
 */
void BufferWriter_EndPacket (BufferWriter *o, int len);

#endif
