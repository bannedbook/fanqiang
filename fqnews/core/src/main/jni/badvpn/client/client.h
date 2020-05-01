/**
 * @file client.h
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
 */

#include <stdio.h>
#include <stdint.h>

#include <protocol/scproto.h>
#include <structure/LinkedList1.h>
#include <flow/PacketPassFairQueue.h>
#include <flow/SinglePacketBuffer.h>
#include <flow/PacketRecvConnector.h>
#include <client/DatagramPeerIO.h>
#include <client/StreamPeerIO.h>
#include <client/DataProto.h>
#include <client/DPReceive.h>
#include <client/FrameDecider.h>
#include <client/PeerChat.h>
#include <client/SinglePacketSource.h>

// NOTE: all time values are in milliseconds

// name of the program
#define PROGRAM_NAME "client"

// server output buffer size
#define SERVER_BUFFER_MIN_PACKETS 200

// maximum UDP payload size
#define CLIENT_UDP_MTU 1472

// maximum number of pending TCP PasswordListener clients
#define TCP_MAX_PASSWORD_LISTENER_CLIENTS 50

// maximum number of peers
#define DEFAULT_MAX_PEERS 256
// maximum number of peer's MAC addresses to remember
#define PEER_DEFAULT_MAX_MACS 16
// maximum number of multicast addresses per peer
#define PEER_DEFAULT_MAX_GROUPS 16
// how long we wait for a packet to reach full size before sending it (see FragmentProtoDisassembler latency argument)
#define PEER_DEFAULT_UDP_FRAGMENTATION_LATENCY 0
// value related to how much out-of-order input we tolerate (see FragmentProtoAssembler num_frames argument)
#define PEER_UDP_ASSEMBLER_NUM_FRAMES 4
// socket send buffer (SO_SNDBUF) for peer TCP connections, <=0 to not set
#define PEER_DEFAULT_TCP_SOCKET_SNDBUF 1048576
// keep-alive packet interval for p2p communication
#define PEER_KEEPALIVE_INTERVAL 10000
// keep-alive receive timer for p2p communication (after how long to consider the link down)
#define PEER_KEEPALIVE_RECEIVE_TIMER 22000
// size of frame send buffer, in number of frames
#define PEER_DEFAULT_SEND_BUFFER_SIZE 32
// size of frame send buffer for relayed packets, in number of frames
#define PEER_DEFAULT_SEND_BUFFER_RELAY_SIZE 32
// time after an unused relay flow is freed (-1 for never)
#define PEER_RELAY_FLOW_INACTIVITY_TIME 10000
// retry time
#define PEER_RETRY_TIME 5000

// for how long a peer can send no Membership Reports for a group
// before the peer and group are disassociated
#define DEFAULT_IGMP_GROUP_MEMBERSHIP_INTERVAL 260000
// how long to wait for joins after a Group Specific query has been
// forwarded to a peer before assuming there are no listeners at the peer
#define DEFAULT_IGMP_LAST_MEMBER_QUERY_TIME 2000

// maximum bind addresses
#define MAX_BIND_ADDRS 8
// maximum external addresses per bind address
#define MAX_EXT_ADDRS 8
// maximum scopes
#define MAX_SCOPES 8

//#define SIMULATE_PEER_OUT_OF_BUFFER 70

struct server_flow {
    PacketPassFairQueueFlow qflow;
    SinglePacketBuffer encoder_buffer;
    PacketRecvConnector connector;
    int connected;
};

struct peer_data {
    // peer identifier
    peerid_t id;
    
    // flags provided by the server
    int flags;
    
    // certificate reported by the server, defined only if using SSL
    uint8_t cert[SCID_NEWCLIENT_MAX_CERT_LEN];
    int cert_len;
    char *common_name;
    
    // init job
    BPending job_init;
    
    // server flow
    struct server_flow *server_flow;
    
    // chat
    int have_chat;
    PeerChat chat;
    int chat_send_msg_len;
    
    // resetpeer source (when chat fails)
    int have_resetpeer;
    uint8_t resetpeer_packet[sizeof(struct packetproto_header) + sizeof(struct sc_header) + sizeof(struct sc_client_resetpeer)];
    SinglePacketSource resetpeer_source;
    
    // local flow
    DataProtoFlow local_dpflow;
    
    // frame decider peer
    FrameDeciderPeer decider_peer;
    
    // receive peer
    DPReceivePeer receive_peer;
    
    // flag if link objects are initialized
    int have_link;
    
    // receive receiver
    DPReceiveReceiver receive_receiver;
    
    // transport-specific link objects
    union {
        struct {
            DatagramPeerIO pio;
            uint16_t sendseed_nextid;
            int sendseed_sent;
            uint16_t sendseed_sent_id;
            uint8_t sendseed_sent_key[BENCRYPTION_MAX_KEY_SIZE];
            uint8_t sendseed_sent_iv[BENCRYPTION_MAX_BLOCK_SIZE];
            uint16_t pending_recvseed_id;
            BPending job_send_seed;
        } udp;
        struct {
            StreamPeerIO pio;
        } tcp;
    } pio;
    
    // link sending
    DataProtoSink send_dp;
    
    // relaying objects
    struct peer_data *relaying_peer; // peer through which we are relaying, or NULL
    LinkedList1Node relaying_list_node; // node in relay peer's relay_users
    
    // waiting for relay data
    int waiting_relay;
    LinkedList1Node waiting_relay_list_node;
    
    // retry timer
    BTimer reset_timer;
    
    // relay server specific
    int is_relay;
    LinkedList1Node relay_list_node;
    LinkedList1 relay_users;
    
    // binding state
    int binding;
    int binding_addrpos;
    
    // peers linked list node
    LinkedList1Node list_node;
};
