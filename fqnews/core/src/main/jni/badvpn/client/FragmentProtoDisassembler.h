/**
 * @file FragmentProtoDisassembler.h
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
 * Object which encodes packets into packets composed of chunks
 * according to FragmentProto.
 */

#ifndef BADVPN_CLIENT_CCPROTODISASSEMBLER_H
#define BADVPN_CLIENT_CCPROTODISASSEMBLER_H

#include <stdint.h>

#include <protocol/fragmentproto.h>
#include <base/DebugObject.h>
#include <system/BReactor.h>
#include <system/BTime.h>
#include <flow/PacketPassInterface.h>
#include <flow/PacketRecvInterface.h>

/**
 * Object which encodes packets into packets composed of chunks
 * according to FragmentProto.
 *
 * Input is with {@link PacketPassInterface}.
 * Output is with {@link PacketRecvInterface}.
 */
typedef struct {
    BReactor *reactor;
    int output_mtu;
    int chunk_mtu;
    btime_t latency;
    PacketPassInterface input;
    PacketRecvInterface output;
    BTimer timer;
    int in_len;
    uint8_t *in;
    int in_used;
    uint8_t *out;
    int out_used;
    fragmentproto_frameid frame_id;
    DebugObject d_obj;
} FragmentProtoDisassembler;

/**
 * Initializes the object.
 *
 * @param o the object
 * @param reactor reactor we live in
 * @param input_mtu maximum input packet size. Must be >=0 and <=UINT16_MAX.
 * @param output_mtu maximum output packet size. Must be >sizeof(struct fragmentproto_chunk_header).
 * @param chunk_mtu maximum chunk size. Must be >0, or <0 for no explicit limit.
 * @param latency maximum time a pending output packet with some data can wait for more data
 *                before being sent out. If nonnegative, a timer will be used. If negative,
 *                packets will always be sent out immediately. If low latency is desired,
 *                prefer setting this to zero rather than negative.
 */
void FragmentProtoDisassembler_Init (FragmentProtoDisassembler *o, BReactor *reactor, int input_mtu, int output_mtu, int chunk_mtu, btime_t latency);

/**
 * Frees the object.
 *
 * @param o the object
 */
void FragmentProtoDisassembler_Free (FragmentProtoDisassembler *o);

/**
 * Returns the input interface.
 *
 * @param o the object
 * @return input interface
 */
PacketPassInterface * FragmentProtoDisassembler_GetInput (FragmentProtoDisassembler *o);

/**
 * Returns the output interface.
 *
 * @param o the object
 * @return output interface
 */
PacketRecvInterface * FragmentProtoDisassembler_GetOutput (FragmentProtoDisassembler *o);

#endif
