/**
 * @file server.h
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

#include <stdint.h>

#include <protocol/scproto.h>
#include <structure/LinkedList1.h>
#include <structure/BAVL.h>
#include <flow/PacketProtoDecoder.h>
#include <flow/PacketStreamSender.h>
#include <flow/PacketPassPriorityQueue.h>
#include <flow/PacketPassFairQueue.h>
#include <flow/PacketProtoFlow.h>
#include <system/BReactor.h>
#include <system/BConnection.h>
#include <nspr_support/BSSLConnection.h>

// name of the program
#define PROGRAM_NAME "server"

// maxiumum number of connected clients. Must be <=2^16.
#define DEFAULT_MAX_CLIENTS 30
// client output control flow buffer size in packets
// it must hold: initdata, newclient's, endclient's (if other peers die when informing them)
// make it big enough to hold the initial packet burst (initdata, newclient's),
#define CLIENT_CONTROL_BUFFER_MIN_PACKETS (1 + 2*(MAX_CLIENTS - 1))
// size of client-to-client buffers in packets
#define CLIENT_PEER_FLOW_BUFFER_MIN_PACKETS 10
// after how long of not hearing anything from the client we disconnect it
#define CLIENT_NO_DATA_TIME_LIMIT 30000
// SO_SNDBFUF socket option for clients
#define CLIENT_DEFAULT_SOCKET_SNDBUF 16384
// reset time when a buffer runs out or when we get the resetpeer message
#define CLIENT_RESET_TIME 30000

// maxiumum listen addresses
#define MAX_LISTEN_ADDRS 16

//#define SIMULATE_OUT_OF_CONTROL_BUFFER 20
//#define SIMULATE_OUT_OF_FLOW_BUFFER 100


// performing SSL handshake
#define INITSTATUS_HANDSHAKE 1
// waiting for clienthello
#define INITSTATUS_WAITHELLO 2
// initialisation was complete
#define INITSTATUS_COMPLETE 3

#define INITSTATUS_HASLINK(status) ((status) == INITSTATUS_WAITHELLO || (status) == INITSTATUS_COMPLETE)

struct client_data;
struct peer_know;

struct peer_flow {
    // source client
    struct client_data *src_client;
    // destination client
    struct client_data *dest_client;
    peerid_t dest_client_id;
    // node in source client hash table (by destination), only when src_client != NULL
    BAVLNode src_tree_node;
    // node in source client list, only when src_client != NULL
    LinkedList1Node src_list_node;
    // node in destination client list
    LinkedList1Node dest_list_node;
    // output chain
    int have_io;
    PacketPassFairQueueFlow qflow;
    PacketProtoFlow oflow;
    BufferWriter *input;
    int packet_len;
    uint8_t *packet;
    // reset timer
    BTimer reset_timer;
    // opposite flow
    struct peer_flow *opposite;
    // pair data
    struct peer_know *know;
    int accepted;
    int resetting;
};

struct peer_know {
    struct client_data *from;
    struct client_data *to;
    int relay_server;
    int relay_client;
    LinkedList1Node from_node;
    LinkedList1Node to_node;
    BPending inform_job;
    BPending uninform_job;
};

struct client_data {
    // socket
    BConnection con;
    BAddr addr;
    
    // SSL connection, if using SSL
    PRFileDesc bottom_prfd;
    PRFileDesc *ssl_prfd;
    BSSLConnection sslcon;
    
    // initialization state
    int initstatus;
    
    // client data if using SSL
    uint8_t cert[SCID_NEWCLIENT_MAX_CERT_LEN];
    int cert_len;
    uint8_t cert_old[SCID_NEWCLIENT_MAX_CERT_LEN];
    int cert_old_len;
    char *common_name;
    
    // client version
    int version;
    
    // no data timer
    BTimer disconnect_timer;
    
    // client ID
    peerid_t id;
    
    // node in clients linked list
    LinkedList1Node list_node;
    // node in clients tree (by ID)
    BAVLNode tree_node;
    
    // knowledge lists
    LinkedList1 know_out_list;
    LinkedList1 know_in_list;
    
    // flows from us
    LinkedList1 peer_out_flows_list;
    BAVL peer_out_flows_tree;
    
    // whether it's being removed
    int dying;
    BPending dying_job;
    
    // input
    PacketProtoDecoder input_decoder;
    PacketPassInterface input_interface;
    
    // output common
    PacketStreamSender output_sender;
    PacketPassPriorityQueue output_priorityqueue;
    
    // output control flow
    PacketPassPriorityQueueFlow output_control_qflow;
    PacketProtoFlow output_control_oflow;
    BufferWriter *output_control_input;
    int output_control_packet_len;
    uint8_t *output_control_packet;
    
    // output peers flow
    PacketPassPriorityQueueFlow output_peers_qflow;
    PacketPassFairQueue output_peers_fairqueue;
    LinkedList1 output_peers_flows;
};
