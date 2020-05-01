/**
 * @file fragmentproto.h
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
 * Definitions for FragmentProto, a protocol that allows sending of arbitrarily sized packets over
 * a link with a fixed MTU.
 * 
 * All multi-byte integers in structs are little-endian, unless stated otherwise.
 * 
 * A FragmentProto packet consists of a number of chunks.
 * Each chunk consists of:
 *   - the chunk header (struct {@link fragmentproto_chunk_header})
 *   - the chunk payload, i.e. part of the frame specified in the header
 */

#ifndef BADVPN_PROTOCOL_FRAGMENTPROTO_H
#define BADVPN_PROTOCOL_FRAGMENTPROTO_H

#include <stdint.h>

#include <misc/balign.h>
#include <misc/packed.h>

typedef uint16_t fragmentproto_frameid;

/**
 * FragmentProto chunk header.
 */
B_START_PACKED
struct fragmentproto_chunk_header {
    /**
     * Identifier of the frame this chunk belongs to.
     * Frames should be given ascending identifiers as they are encoded
     * into chunks (except when the ID wraps to zero).
     */
    fragmentproto_frameid frame_id;
    
    /**
     * Position in the frame where this chunk starts.
     */
    uint16_t chunk_start;
    
    /**
     * Length of the chunk's payload.
     */
    uint16_t chunk_len;
    
    /**
     * Whether this is the last chunk of the frame, i.e.
     * the total length of the frame is chunk_start + chunk_len.
     */
    uint8_t is_last;
} B_PACKED;
B_END_PACKED

/**
 * Calculates how many chunks are needed at most for encoding one frame of the
 * given maximum size with FragmentProto onto a carrier with a given MTU.
 * This includes the case when the first chunk of a frame is not the first chunk
 * in a FragmentProto packet.
 * 
 * @param carrier_mtu MTU of the carrier, i.e. maximum length of FragmentProto packets. Must be >sizeof(struct fragmentproto_chunk_header).
 * @param frame_mtu maximum frame size. Must be >=0.
 * @return maximum number of chunks needed. Will be >0.
 */
static int fragmentproto_max_chunks_for_frame (int carrier_mtu, int frame_mtu)
{
    ASSERT(carrier_mtu > sizeof(struct fragmentproto_chunk_header))
    ASSERT(frame_mtu >= 0)
    
    return (bdivide_up(frame_mtu, (carrier_mtu - sizeof(struct fragmentproto_chunk_header))) + 1);
}

#endif
