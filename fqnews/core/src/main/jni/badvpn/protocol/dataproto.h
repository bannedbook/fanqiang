/**
 * @file dataproto.h
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
 * Definitions for DataProto, the protocol for data transport between VPN peers.
 * 
 * All multi-byte integers in structs are little-endian, unless stated otherwise.
 * 
 * A DataProto packet consists of:
 *   - the header (struct {@link dataproto_header})
 *   - between zero and DATAPROTO_MAX_PEER_IDS destination peer IDs (struct {@link dataproto_peer_id})
 *   - the payload, e.g. Ethernet frame
 */

#ifndef BADVPN_PROTOCOL_DATAPROTO_H
#define BADVPN_PROTOCOL_DATAPROTO_H

#include <stdint.h>

#include <protocol/scproto.h>
#include <misc/packed.h>

#define DATAPROTO_MAX_PEER_IDS 1

#define DATAPROTO_FLAGS_RECEIVING_KEEPALIVES 1

/**
 * DataProto header.
 */
B_START_PACKED
struct dataproto_header {
    /**
     * Bitwise OR of flags. Possible flags:
     *   - DATAPROTO_FLAGS_RECEIVING_KEEPALIVES
     *     Indicates that when the peer sent this packet, it has received at least
     *     one packet from the other peer in the last keep-alive tolerance time.
     */
    uint8_t flags;
    
    /**
     * ID of the peer this frame originates from.
     */
    peerid_t from_id;
    
    /**
     * Number of destination peer IDs that follow.
     * Must be <=DATAPROTO_MAX_PEER_IDS.
     */
    peerid_t num_peer_ids;
} B_PACKED;
B_END_PACKED

/**
 * Structure for a destination peer ID in DataProto.
 * Wraps a single peerid_t in a packed struct for easy access.
 */
B_START_PACKED
struct dataproto_peer_id {
    peerid_t id;
} B_PACKED;
B_END_PACKED

#define DATAPROTO_MAX_OVERHEAD (sizeof(struct dataproto_header) + DATAPROTO_MAX_PEER_IDS * sizeof(struct dataproto_peer_id))

#endif
