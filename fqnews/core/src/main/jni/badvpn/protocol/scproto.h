/**
 * @file scproto.h
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
 * Definitions for SCProto, the protocol that the clients communicate in
 * with the server.
 * 
 * All multi-byte integers in structs are little-endian, unless stated otherwise.
 * 
 * A SCProto packet consists of:
 *   - a header (struct {@link sc_header}) which contains the type of the
 *     packet
 *   - the payload
 * 
 * It goes roughly like that:
 * 
 * When the client connects to the server, it sends a "clienthello" packet
 * to the server. The packet contains the protocol version the client is using.
 * When the server receives the "clienthello" packet, it checks the version.
 * If it doesn't match, it disconnects the client. Otherwise the server sends
 * the client a "serverhello" packet to the client. That packet contains
 * the ID of the client and possibly its IPv4 address as the server sees it
 * (zero if not applicable).
 * 
 * The server than proceeds to synchronize the peers' knowledge of each other.
 * It does that by sending a "newclient" messages to a client to inform it of
 * another peer, and "endclient" messages to inform it that a peer is gone.
 * Each client, upon receiving a "newclient" message, MUST sent a corresponding
 * "acceptpeer" message, before sending any messages to the new peer.
 * The server forwards messages between synchronized peers to allow them to
 * communicate. A peer sends a message to another peer by sending the "outmsg"
 * packet to the server, and the server delivers a message to a peer by sending
 * it the "inmsg" packet.
 * 
 * The message service is reliable; messages from one client to another are
 * expected to arrive unmodified and in the same order. There is, however,
 * no flow control. This means that messages can not be used for bulk transfers
 * between the clients (and they are not). If the server runs out of buffer for
 * messages from one client to another, it will stop forwarding messages, and
 * will reset knowledge of the two clients after some delay. Similarly, if one
 * of the clients runs out of buffer locally, it will send the "resetpeer"
 * packet to make the server reset knowledge.
 * 
 * The messages transport either:
 * 
 * - If the relevant "newclient" packets do not contain the
 *   SCID_NEWCLIENT_FLAG_SSL flag, then plaintext MsgProto messages.
 * 
 * - If the relevant "newclient" packets do contain the SCID_NEWCLIENT_FLAG_SSL
 *   flag, then SSL, broken down into packets, PacketProto inside SSL, and finally
 *   MsgProto inside PacketProto. The master peer (one with higher ID) acts as an
 *   SSL server, and the other acts as an SSL client. The peers must identify with
 *   the same certificate they used when connecting to the server, and each peer
 *   must byte-compare the other's certificate agains the one provided to it by
 *   by the server in the relevent "newclient" message.
 */

#ifndef BADVPN_PROTOCOL_SCPROTO_H
#define BADVPN_PROTOCOL_SCPROTO_H

#include <stdint.h>

#include <misc/packed.h>

#define SC_VERSION 29
#define SC_OLDVERSION_NOSSL 27
#define SC_OLDVERSION_BROKENCERT 26

#define SC_KEEPALIVE_INTERVAL 10000

/**
 * SCProto packet header.
 * Follows up to SC_MAX_PAYLOAD bytes of payload.
 */
B_START_PACKED
struct sc_header {
    /**
     * Message type.
     */
    uint8_t type;
} B_PACKED;
B_END_PACKED

#define SC_MAX_PAYLOAD 2000
#define SC_MAX_ENC (sizeof(struct sc_header) + SC_MAX_PAYLOAD)

typedef uint16_t peerid_t;

#define SCID_KEEPALIVE 0
#define SCID_CLIENTHELLO 1
#define SCID_SERVERHELLO 2
#define SCID_NEWCLIENT 3
#define SCID_ENDCLIENT 4
#define SCID_OUTMSG 5
#define SCID_INMSG 6
#define SCID_RESETPEER 7
#define SCID_ACCEPTPEER 8

/**
 * "clienthello" client packet payload.
 * Packet type is SCID_CLIENTHELLO.
 */
B_START_PACKED
struct sc_client_hello {
    /**
     * Protocol version the client is using.
     */
    uint16_t version;
} B_PACKED;
B_END_PACKED

/**
 * "serverhello" server packet payload.
 * Packet type is SCID_SERVERHELLO.
 */
B_START_PACKED
struct sc_server_hello {
    /**
     * Flags. Not used yet.
     */
    uint16_t flags;
    
    /**
     * Peer ID of the client.
     */
    peerid_t id;
    
    /**
     * IPv4 address of the client as seen by the server
     * (network byte order). Zero if not applicable.
     */
    uint32_t clientAddr;
} B_PACKED;
B_END_PACKED

/**
 * "newclient" server packet payload.
 * Packet type is SCID_NEWCLIENT.
 * If the server is using TLS, follows up to SCID_NEWCLIENT_MAX_CERT_LEN
 * bytes of the new client's certificate (encoded in DER).
 */
B_START_PACKED
struct sc_server_newclient {
    /**
     * ID of the new peer.
     */
    peerid_t id;
    
    /**
     * Flags. Possible flags:
     *   - SCID_NEWCLIENT_FLAG_RELAY_SERVER
     *     You can relay frames to other peers through this peer.
     *   - SCID_NEWCLIENT_FLAG_RELAY_CLIENT
     *     You must allow this peer to relay frames to other peers through you.
     *   - SCID_NEWCLIENT_FLAG_SSL
     *     SSL must be used to talk to this peer through messages.
     */
    uint16_t flags;
} B_PACKED;
B_END_PACKED

#define SCID_NEWCLIENT_FLAG_RELAY_SERVER 1
#define SCID_NEWCLIENT_FLAG_RELAY_CLIENT 2
#define SCID_NEWCLIENT_FLAG_SSL 4

#define SCID_NEWCLIENT_MAX_CERT_LEN (SC_MAX_PAYLOAD - sizeof(struct sc_server_newclient))

/**
 * "endclient" server packet payload.
 * Packet type is SCID_ENDCLIENT.
 */
B_START_PACKED
struct sc_server_endclient {
    /**
     * ID of the removed peer.
     */
    peerid_t id;
} B_PACKED;
B_END_PACKED

/**
 * "outmsg" client packet header.
 * Packet type is SCID_OUTMSG.
 * Follows up to SC_MAX_MSGLEN bytes of message payload.
 */
B_START_PACKED
struct sc_client_outmsg {
    /**
     * ID of the destionation peer.
     */
    peerid_t clientid;
} B_PACKED;
B_END_PACKED

/**
 * "inmsg" server packet payload.
 * Packet type is SCID_INMSG.
 * Follows up to SC_MAX_MSGLEN bytes of message payload.
 */
B_START_PACKED
struct sc_server_inmsg {
    /**
     * ID of the source peer.
     */
    peerid_t clientid;
} B_PACKED;
B_END_PACKED

#define _SC_MAX_OUTMSGLEN (SC_MAX_PAYLOAD - sizeof(struct sc_client_outmsg))
#define _SC_MAX_INMSGLEN (SC_MAX_PAYLOAD - sizeof(struct sc_server_inmsg))

#define SC_MAX_MSGLEN (_SC_MAX_OUTMSGLEN < _SC_MAX_INMSGLEN ? _SC_MAX_OUTMSGLEN : _SC_MAX_INMSGLEN)

/**
 * "resetpeer" client packet header.
 * Packet type is SCID_RESETPEER.
 */
B_START_PACKED
struct sc_client_resetpeer {
    /**
     * ID of the peer to reset.
     */
    peerid_t clientid;
} B_PACKED;
B_END_PACKED

/**
 * "acceptpeer" client packet payload.
 * Packet type is SCID_ACCEPTPEER.
 */
B_START_PACKED
struct sc_client_acceptpeer {
    /**
     * ID of the peer to accept.
     */
    peerid_t clientid;
} B_PACKED;
B_END_PACKED

#endif
