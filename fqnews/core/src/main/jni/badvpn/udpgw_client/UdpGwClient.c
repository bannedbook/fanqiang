/*
 * Copyright (C) Ambroz Bizjak <ambrop7@gmail.com>
 * Contributions:
 * Transparent DNS: Copyright (C) Kerem Hadimli <kerem.hadimli@gmail.com>
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

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <misc/byteorder.h>
#include <misc/compare.h>
#include <base/BLog.h>

#include <udpgw_client/UdpGwClient.h>

#include <generated/blog_channel_UdpGwClient.h>

static int uint16_comparator (void *unused, uint16_t *v1, uint16_t *v2);
static int conaddr_comparator (void *unused, struct UdpGwClient_conaddr *v1, struct UdpGwClient_conaddr *v2);
static void free_server (UdpGwClient *o);
static void decoder_handler_error (UdpGwClient *o);
static void recv_interface_handler_send (UdpGwClient *o, uint8_t *data, int data_len);
static void send_monitor_handler (UdpGwClient *o);
static void keepalive_if_handler_done (UdpGwClient *o);
static struct UdpGwClient_connection * find_connection_by_conaddr (UdpGwClient *o, struct UdpGwClient_conaddr conaddr);
static struct UdpGwClient_connection * find_connection_by_conid (UdpGwClient *o, uint16_t conid);
static uint16_t find_unused_conid (UdpGwClient *o);
static void connection_init (UdpGwClient *o, struct UdpGwClient_conaddr conaddr, uint8_t flags, const uint8_t *data, int data_len);
static void connection_free (struct UdpGwClient_connection *con);
static void connection_first_job_handler (struct UdpGwClient_connection *con);
static void connection_send (struct UdpGwClient_connection *con, uint8_t flags, const uint8_t *data, int data_len);
static struct UdpGwClient_connection * reuse_connection (UdpGwClient *o, struct UdpGwClient_conaddr conaddr);

static int uint16_comparator (void *unused, uint16_t *v1, uint16_t *v2)
{
    return B_COMPARE(*v1, *v2);
}

static int conaddr_comparator (void *unused, struct UdpGwClient_conaddr *v1, struct UdpGwClient_conaddr *v2)
{
    int r = BAddr_CompareOrder(&v1->remote_addr, &v2->remote_addr);
    if (r) {
        return r;
    }
    return BAddr_CompareOrder(&v1->local_addr, &v2->local_addr);
}

static void free_server (UdpGwClient *o)
{
    // disconnect send connector
    PacketPassConnector_DisconnectOutput(&o->send_connector);
    
    // free send sender
    PacketStreamSender_Free(&o->send_sender);
    
    // free receive decoder
    PacketProtoDecoder_Free(&o->recv_decoder);
    
    // free receive interface
    PacketPassInterface_Free(&o->recv_if);
}

static void decoder_handler_error (UdpGwClient *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_server)
    
    BLog(BLOG_ERROR, "decoder error");
    
    // report error
    o->handler_servererror(o->user);
    return;
}

static void recv_interface_handler_send (UdpGwClient *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_server)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->udpgw_mtu)
    
    // accept packet
    PacketPassInterface_Done(&o->recv_if);
    
    // check header
    if (data_len < sizeof(struct udpgw_header)) {
        BLog(BLOG_ERROR, "missing header");
        return;
    }
    struct udpgw_header header;
    memcpy(&header, data, sizeof(header));
    data += sizeof(header);
    data_len -= sizeof(header);
    uint8_t flags = ltoh8(header.flags);
    uint16_t conid = ltoh16(header.conid);
    
    // parse address
    BAddr remote_addr;
    if ((flags & UDPGW_CLIENT_FLAG_IPV6)) {
        if (data_len < sizeof(struct udpgw_addr_ipv6)) {
            BLog(BLOG_ERROR, "missing ipv6 address");
            return;
        }
        struct udpgw_addr_ipv6 addr_ipv6;
        memcpy(&addr_ipv6, data, sizeof(addr_ipv6));
        data += sizeof(addr_ipv6);
        data_len -= sizeof(addr_ipv6);
        BAddr_InitIPv6(&remote_addr, addr_ipv6.addr_ip, addr_ipv6.addr_port);
    } else {
        if (data_len < sizeof(struct udpgw_addr_ipv4)) {
            BLog(BLOG_ERROR, "missing ipv4 address");
            return;
        }
        struct udpgw_addr_ipv4 addr_ipv4;
        memcpy(&addr_ipv4, data, sizeof(addr_ipv4));
        data += sizeof(addr_ipv4);
        data_len -= sizeof(addr_ipv4);
        BAddr_InitIPv4(&remote_addr, addr_ipv4.addr_ip, addr_ipv4.addr_port);
    }
    
    // check remaining data
    if (data_len > o->udp_mtu) {
        BLog(BLOG_ERROR, "too much data");
        return;
    }
    
    // find connection
    struct UdpGwClient_connection *con = find_connection_by_conid(o, conid);
    if (!con) {
        BLog(BLOG_ERROR, "unknown conid");
        return;
    }
    
    // check remote address
    if (BAddr_CompareOrder(&con->conaddr.remote_addr, &remote_addr) != 0) {
        BLog(BLOG_ERROR, "wrong remote address");
        return;
    }
    
    // move connection to front of the list
    LinkedList1_Remove(&o->connections_list, &con->connections_list_node);
    LinkedList1_Append(&o->connections_list, &con->connections_list_node);
    
    // pass packet to user
    o->handler_received(o->user, con->conaddr.local_addr, con->conaddr.remote_addr, data, data_len);
    return;
}

static void send_monitor_handler (UdpGwClient *o)
{
    DebugObject_Access(&o->d_obj);
    
    if (o->keepalive_sending) {
        return;
    }
    
    BLog(BLOG_INFO, "keepalive");
    
    // send keepalive
    PacketPassInterface_Sender_Send(o->keepalive_if, (uint8_t *)&o->keepalive_packet, sizeof(o->keepalive_packet));
    
    // set sending keep-alive
    o->keepalive_sending = 1;
}

static void keepalive_if_handler_done (UdpGwClient *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->keepalive_sending)
    
    // set not sending keepalive
    o->keepalive_sending = 0;
}

static struct UdpGwClient_connection * find_connection_by_conaddr (UdpGwClient *o, struct UdpGwClient_conaddr conaddr)
{
    BAVLNode *tree_node = BAVL_LookupExact(&o->connections_tree_by_conaddr, &conaddr);
    if (!tree_node) {
        return NULL;
    }
    
    return UPPER_OBJECT(tree_node, struct UdpGwClient_connection, connections_tree_by_conaddr_node);
}

static struct UdpGwClient_connection * find_connection_by_conid (UdpGwClient *o, uint16_t conid)
{
    BAVLNode *tree_node = BAVL_LookupExact(&o->connections_tree_by_conid, &conid);
    if (!tree_node) {
        return NULL;
    }
    
    return UPPER_OBJECT(tree_node, struct UdpGwClient_connection, connections_tree_by_conid_node);
}

static uint16_t find_unused_conid (UdpGwClient *o)
{
    ASSERT(o->num_connections < o->max_connections)
    
    while (1) {
        if (!find_connection_by_conid(o, o->next_conid)) {
            return o->next_conid;
        }
        
        if (o->next_conid == o->max_connections - 1) {
            o->next_conid = 0;
        } else {
            o->next_conid++;
        }
    }
}

static void connection_init (UdpGwClient *o, struct UdpGwClient_conaddr conaddr, uint8_t flags, const uint8_t *data, int data_len)
{
    ASSERT(o->num_connections < o->max_connections)
    ASSERT(!find_connection_by_conaddr(o, conaddr))
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->udp_mtu)
    
    // allocate structure
    struct UdpGwClient_connection *con = (struct UdpGwClient_connection *)malloc(sizeof(*con));
    if (!con) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // init arguments
    con->client = o;
    con->conaddr = conaddr;
    con->first_flags = flags;
    con->first_data = data;
    con->first_data_len = data_len;
    
    // allocate conid
    con->conid = find_unused_conid(o);
    
    // init first job
    BPending_Init(&con->first_job, BReactor_PendingGroup(o->reactor), (BPending_handler)connection_first_job_handler, con);
    BPending_Set(&con->first_job);
    
    // init queue flow
    PacketPassFairQueueFlow_Init(&con->send_qflow, &o->send_queue);
    
    // init PacketProtoFlow
    if (!PacketProtoFlow_Init(&con->send_ppflow, o->udpgw_mtu, o->send_buffer_size, PacketPassFairQueueFlow_GetInput(&con->send_qflow), BReactor_PendingGroup(o->reactor))) {
        BLog(BLOG_ERROR, "PacketProtoFlow_Init failed");
        goto fail1;
    }
    con->send_if = PacketProtoFlow_GetInput(&con->send_ppflow);
    
    // insert to connections tree by conaddr
    ASSERT_EXECUTE(BAVL_Insert(&o->connections_tree_by_conaddr, &con->connections_tree_by_conaddr_node, NULL))
    
    // insert to connections tree by conid
    ASSERT_EXECUTE(BAVL_Insert(&o->connections_tree_by_conid, &con->connections_tree_by_conid_node, NULL))
    
    // insert to connections list
    LinkedList1_Append(&o->connections_list, &con->connections_list_node);
    
    // increment number of connections
    o->num_connections++;
    
    return;
    
fail1:
    PacketPassFairQueueFlow_Free(&con->send_qflow);
    BPending_Free(&con->first_job);
    free(con);
fail0:
    return;
}

static void connection_free (struct UdpGwClient_connection *con)
{
    UdpGwClient *o = con->client;
    PacketPassFairQueueFlow_AssertFree(&con->send_qflow);
    
    // decrement number of connections
    o->num_connections--;
    
    // remove from connections list
    LinkedList1_Remove(&o->connections_list, &con->connections_list_node);
    
    // remove from connections tree by conid
    BAVL_Remove(&o->connections_tree_by_conid, &con->connections_tree_by_conid_node);
    
    // remove from connections tree by conaddr
    BAVL_Remove(&o->connections_tree_by_conaddr, &con->connections_tree_by_conaddr_node);
    
    // free PacketProtoFlow
    PacketProtoFlow_Free(&con->send_ppflow);
    
    // free queue flow
    PacketPassFairQueueFlow_Free(&con->send_qflow);
    
    // free first job
    BPending_Free(&con->first_job);
    
    // free structure
    free(con);
}

static void connection_first_job_handler (struct UdpGwClient_connection *con)
{
    connection_send(con, UDPGW_CLIENT_FLAG_REBIND|con->first_flags, con->first_data, con->first_data_len);
}

static void connection_send (struct UdpGwClient_connection *con, uint8_t flags, const uint8_t *data, int data_len)
{
    UdpGwClient *o = con->client;
    B_USE(o)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->udp_mtu)
    
    // get buffer location
    uint8_t *out;
    if (!BufferWriter_StartPacket(con->send_if, &out)) {
        BLog(BLOG_ERROR, "out of buffer");
        return;
    }
    int out_pos = 0;
    
    if (con->conaddr.remote_addr.type == BADDR_TYPE_IPV6) {
        flags |= UDPGW_CLIENT_FLAG_IPV6;
    }
    
    // write header
    struct udpgw_header header;
    header.flags = ltoh8(flags);
    header.conid = ltoh16(con->conid);
    memcpy(out + out_pos, &header, sizeof(header));
    out_pos += sizeof(header);
    
    // write address
    switch (con->conaddr.remote_addr.type) {
        case BADDR_TYPE_IPV4: {
            struct udpgw_addr_ipv4 addr_ipv4;
            addr_ipv4.addr_ip = con->conaddr.remote_addr.ipv4.ip;
            addr_ipv4.addr_port = con->conaddr.remote_addr.ipv4.port;
            memcpy(out + out_pos, &addr_ipv4, sizeof(addr_ipv4));
            out_pos += sizeof(addr_ipv4);
        } break;
        case BADDR_TYPE_IPV6: {
            struct udpgw_addr_ipv6 addr_ipv6;
            memcpy(addr_ipv6.addr_ip, con->conaddr.remote_addr.ipv6.ip, sizeof(addr_ipv6.addr_ip));
            addr_ipv6.addr_port = con->conaddr.remote_addr.ipv6.port;
            memcpy(out + out_pos, &addr_ipv6, sizeof(addr_ipv6));
            out_pos += sizeof(addr_ipv6);
        } break;
    }
    
    // write packet to buffer
    memcpy(out + out_pos, data, data_len);
    out_pos += data_len;
    
    // submit packet to buffer
    BufferWriter_EndPacket(con->send_if, out_pos);
}

static struct UdpGwClient_connection * reuse_connection (UdpGwClient *o, struct UdpGwClient_conaddr conaddr)
{
    ASSERT(!find_connection_by_conaddr(o, conaddr))
    ASSERT(o->num_connections > 0)
    
    // get least recently used connection
    struct UdpGwClient_connection *con = UPPER_OBJECT(LinkedList1_GetFirst(&o->connections_list), struct UdpGwClient_connection, connections_list_node);
    
    // remove from connections tree by conaddr
    BAVL_Remove(&o->connections_tree_by_conaddr, &con->connections_tree_by_conaddr_node);
    
    // set new conaddr
    con->conaddr = conaddr;
    
    // insert to connections tree by conaddr
    ASSERT_EXECUTE(BAVL_Insert(&o->connections_tree_by_conaddr, &con->connections_tree_by_conaddr_node, NULL))
    
    return con;
}

int UdpGwClient_Init (UdpGwClient *o, int udp_mtu, int max_connections, int send_buffer_size, btime_t keepalive_time, BReactor *reactor, void *user,
                      UdpGwClient_handler_servererror handler_servererror,
                      UdpGwClient_handler_received handler_received)
{
    ASSERT(udp_mtu >= 0)
    ASSERT(udpgw_compute_mtu(udp_mtu) >= 0)
    ASSERT(udpgw_compute_mtu(udp_mtu) <= PACKETPROTO_MAXPAYLOAD)
    ASSERT(max_connections > 0)
    ASSERT(send_buffer_size > 0)
    
    // init arguments
    o->udp_mtu = udp_mtu;
    o->max_connections = max_connections;
    o->send_buffer_size = send_buffer_size;
    o->keepalive_time = keepalive_time;
    o->reactor = reactor;
    o->user = user;
    o->handler_servererror = handler_servererror;
    o->handler_received = handler_received;
    
    // limit max connections to number of conid's
    if (o->max_connections > UINT16_MAX + 1) {
        o->max_connections = UINT16_MAX + 1;
    }
    
    // compute MTUs
    o->udpgw_mtu = udpgw_compute_mtu(o->udp_mtu);
    o->pp_mtu = o->udpgw_mtu + sizeof(struct packetproto_header);
    
    // init connections tree by conaddr
    BAVL_Init(&o->connections_tree_by_conaddr, OFFSET_DIFF(struct UdpGwClient_connection, conaddr, connections_tree_by_conaddr_node), (BAVL_comparator)conaddr_comparator, NULL);
    
    // init connections tree by conid
    BAVL_Init(&o->connections_tree_by_conid, OFFSET_DIFF(struct UdpGwClient_connection, conid, connections_tree_by_conid_node), (BAVL_comparator)uint16_comparator, NULL);
    
    // init connections list
    LinkedList1_Init(&o->connections_list);
    
    // set zero connections
    o->num_connections = 0;
    
    // set next conid
    o->next_conid = 0;
    
    // init send connector
    PacketPassConnector_Init(&o->send_connector, o->pp_mtu, BReactor_PendingGroup(o->reactor));
    
    // init send monitor
    PacketPassInactivityMonitor_Init(&o->send_monitor, PacketPassConnector_GetInput(&o->send_connector), o->reactor, o->keepalive_time, (PacketPassInactivityMonitor_handler)send_monitor_handler, o);
    
    // init send queue
    if (!PacketPassFairQueue_Init(&o->send_queue, PacketPassInactivityMonitor_GetInput(&o->send_monitor), BReactor_PendingGroup(o->reactor), 0, 1)) {
        goto fail0;
    }
    
    // construct keepalive packet
    o->keepalive_packet.pp.len = sizeof(o->keepalive_packet.udpgw);
    memset(&o->keepalive_packet.udpgw, 0, sizeof(o->keepalive_packet.udpgw));
    o->keepalive_packet.udpgw.flags = UDPGW_CLIENT_FLAG_KEEPALIVE;
    
    // init keepalive queue flow
    PacketPassFairQueueFlow_Init(&o->keepalive_qflow, &o->send_queue);
    o->keepalive_if = PacketPassFairQueueFlow_GetInput(&o->keepalive_qflow);
    
    // init keepalive output
    PacketPassInterface_Sender_Init(o->keepalive_if, (PacketPassInterface_handler_done)keepalive_if_handler_done, o);
    
    // set not sending keepalive
    o->keepalive_sending = 0;
    
    // set have no server
    o->have_server = 0;
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail0:
    PacketPassInactivityMonitor_Free(&o->send_monitor);
    PacketPassConnector_Free(&o->send_connector);
    return 0;
}

void UdpGwClient_Free (UdpGwClient *o)
{
    DebugObject_Free(&o->d_obj);
    
    // allow freeing send queue flows
    PacketPassFairQueue_PrepareFree(&o->send_queue);
    
    // free connections
    while (!LinkedList1_IsEmpty(&o->connections_list)) {
        struct UdpGwClient_connection *con = UPPER_OBJECT(LinkedList1_GetFirst(&o->connections_list), struct UdpGwClient_connection, connections_list_node);
        connection_free(con);
    }
    
    // free server
    if (o->have_server) {
        free_server(o);
    }
    
    // free keepalive queue flow
    PacketPassFairQueueFlow_Free(&o->keepalive_qflow);
    
    // free send queue
    PacketPassFairQueue_Free(&o->send_queue);
    
    // free send
    PacketPassInactivityMonitor_Free(&o->send_monitor);
    
    // free send connector
    PacketPassConnector_Free(&o->send_connector);
}

void UdpGwClient_SubmitPacket (UdpGwClient *o, BAddr local_addr, BAddr remote_addr, int is_dns, const uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(local_addr.type == BADDR_TYPE_IPV4 || local_addr.type == BADDR_TYPE_IPV6)
    ASSERT(remote_addr.type == BADDR_TYPE_IPV4 || remote_addr.type == BADDR_TYPE_IPV6)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->udp_mtu)
    
    // build conaddr
    struct UdpGwClient_conaddr conaddr;
    conaddr.local_addr = local_addr;
    conaddr.remote_addr = remote_addr;
    
    // lookup connection
    struct UdpGwClient_connection *con = find_connection_by_conaddr(o, conaddr);
    
    uint8_t flags = 0;

    if (is_dns) {
        // route to remote DNS server instead of provided address
        flags |= UDPGW_CLIENT_FLAG_DNS;
    }
    
    // if no connection and can't create a new one, reuse the least recently used une
    if (!con && o->num_connections == o->max_connections) {
        con = reuse_connection(o, conaddr);
        flags |= UDPGW_CLIENT_FLAG_REBIND;
    }
    
    if (!con) {
        // create new connection
        connection_init(o, conaddr, flags, data, data_len);
    } else {
        // move connection to front of the list
        LinkedList1_Remove(&o->connections_list, &con->connections_list_node);
        LinkedList1_Append(&o->connections_list, &con->connections_list_node);
        
        // send packet to existing connection
        connection_send(con, flags, data, data_len);
    }
}

int UdpGwClient_ConnectServer (UdpGwClient *o, StreamPassInterface *send_if, StreamRecvInterface *recv_if)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->have_server)
    
    // init receive interface
    PacketPassInterface_Init(&o->recv_if, o->udpgw_mtu, (PacketPassInterface_handler_send)recv_interface_handler_send, o, BReactor_PendingGroup(o->reactor));
    
    // init receive decoder
    if (!PacketProtoDecoder_Init(&o->recv_decoder, recv_if, &o->recv_if, BReactor_PendingGroup(o->reactor), o, (PacketProtoDecoder_handler_error)decoder_handler_error)) {
        BLog(BLOG_ERROR, "PacketProtoDecoder_Init failed");
        goto fail1;
    }
    
    // init send sender
    PacketStreamSender_Init(&o->send_sender, send_if, o->pp_mtu, BReactor_PendingGroup(o->reactor));
    
    // connect send connector
    PacketPassConnector_ConnectOutput(&o->send_connector, PacketStreamSender_GetInput(&o->send_sender));
    
    // set have server
    o->have_server = 1;
    
    return 1;
    
fail1:
    PacketPassInterface_Free(&o->recv_if);
    return 0;
}

void UdpGwClient_DisconnectServer (UdpGwClient *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_server)
    
    // free server
    free_server(o);
    
    // set have no server
    o->have_server = 0;
}
