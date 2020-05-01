/**
 * @file packetproto.h
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
 * Definitions for PacketProto, a protocol that allows sending of packets
 * over a reliable stream connection.
 * 
 * All multi-byte integers in structs are little-endian, unless stated otherwise.
 * 
 * Packets are encoded into a stream by representing each packet with:
 *   - a 16-bit little-endian unsigned integer representing the length
 *     of the payload
 *   - that many bytes of payload
 */

#ifndef BADVPN_PROTOCOL_PACKETPROTO_H
#define BADVPN_PROTOCOL_PACKETPROTO_H

#include <stdint.h>
#include <limits.h>

#include <misc/packed.h>

/**
 * PacketProto packet header.
 * Wraps a single uint16_t in a packed struct for easy access.
 */
B_START_PACKED
struct packetproto_header
{
    /**
     * Length of the packet payload that follows.
     */
    uint16_t len;
} B_PACKED;
B_END_PACKED

#define PACKETPROTO_ENCLEN(_len) (sizeof(struct packetproto_header) + (_len))

#define PACKETPROTO_MAXPAYLOAD UINT16_MAX

#endif
