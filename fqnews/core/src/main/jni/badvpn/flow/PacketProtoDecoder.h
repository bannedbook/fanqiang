/**
 * @file PacketProtoDecoder.h
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
 * Object which decodes a stream according to PacketProto.
 */

#ifndef BADVPN_FLOW_PACKETPROTODECODER_H
#define BADVPN_FLOW_PACKETPROTODECODER_H

#include <stdint.h>

#include <protocol/packetproto.h>
#include <misc/debug.h>
#include <base/DebugObject.h>
#include <flow/StreamRecvInterface.h>
#include <flow/PacketPassInterface.h>

/**
 * Handler called when a protocol error occurs.
 * When an error occurs, the decoder is reset to the initial state.
 * 
 * @param user as in {@link PacketProtoDecoder_Init}
 */
typedef void (*PacketProtoDecoder_handler_error) (void *user);

typedef struct {
    StreamRecvInterface *input;
    PacketPassInterface *output;
    void *user;
    PacketProtoDecoder_handler_error handler_error;
    int output_mtu;
    int buf_size;
    int buf_start;
    int buf_used;
    uint8_t *buf;
    DebugObject d_obj;
} PacketProtoDecoder;

/**
 * Initializes the object.
 *
 * @param enc the object
 * @param input input interface. The decoder will accept packets with payload size up to its MTU
 *              (but the payload can never be more than PACKETPROTO_MAXPAYLOAD).
 * @param output output interface
 * @param pg pending group
 * @param user argument to handlers
 * @param handler_error error handler
 * @return 1 on success, 0 on failure
 */
int PacketProtoDecoder_Init (PacketProtoDecoder *enc, StreamRecvInterface *input, PacketPassInterface *output, BPendingGroup *pg, void *user, PacketProtoDecoder_handler_error handler_error) WARN_UNUSED;

/**
 * Frees the object.
 *
 * @param enc the object
 */
void PacketProtoDecoder_Free (PacketProtoDecoder *enc);

/**
 * Clears the internal buffer.
 * The next data received from the input will be treated as a new
 * PacketProto stream.
 *
 * @param enc the object
 */
void PacketProtoDecoder_Reset (PacketProtoDecoder *enc);

#endif
