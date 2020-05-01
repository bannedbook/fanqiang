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

#include <misc/debug.h>
#include <base/BLog.h>

#include <tun2socks/SocksUdpGwClient.h>

#include <generated/blog_channel_SocksUdpGwClient.h>

#ifdef __ANDROID__

#include <misc/socks_proto.h>
#define CONNECTION_UDP_BUFFER_SIZE 8

#else

static void free_socks (SocksUdpGwClient *o);
static void try_connect (SocksUdpGwClient *o);
static void reconnect_timer_handler (SocksUdpGwClient *o);
static void socks_client_handler (SocksUdpGwClient *o, int event);
static void udpgw_handler_servererror (SocksUdpGwClient *o);
static void udpgw_handler_received (SocksUdpGwClient *o, BAddr local_addr, BAddr remote_addr, const uint8_t *data, int data_len);

#endif

#ifdef __ANDROID__
static void dgram_handler (SocksUdpGwClient_connection *o, int event);
static void dgram_handler_received (SocksUdpGwClient_connection *o, uint8_t *data, int data_len);
static int conaddr_comparator (void *unused, SocksUdpGwClient_conaddr *v1, SocksUdpGwClient_conaddr *v2);
static SocksUdpGwClient_connection * find_connection (SocksUdpGwClient *o, SocksUdpGwClient_conaddr conaddr, int is_dns);
static void remove_lru_connection (SocksUdpGwClient *o, SocksUdpGwClient_conaddr conaddr, int is_dns);
static void reset_connection (SocksUdpGwClient *o, SocksUdpGwClient_connection *con, SocksUdpGwClient_conaddr conaddr, int is_dns);
static int connection_send (SocksUdpGwClient_connection *o, const uint8_t *data, int data_len);
static void connection_first_job_handler (SocksUdpGwClient_connection *con);
static SocksUdpGwClient_connection *connection_init (SocksUdpGwClient *client, SocksUdpGwClient_conaddr conaddr, const uint8_t *data, int data_len, int is_dns);
static void connection_free (SocksUdpGwClient_connection *o);

static void dgram_handler (SocksUdpGwClient_connection *o, int event)
{
    SocksUdpGwClient *client = o->client;
    ASSERT(client);

    DebugObject_Access(&client->d_obj);

    BLog(BLOG_INFO, "UDP error");
}

static void dgram_handler_received (SocksUdpGwClient_connection *o, uint8_t *data, int data_len)
{
    SocksUdpGwClient *client = o->client;
    ASSERT(client);

    DebugObject_Access(&client->d_obj);
    ASSERT(data_len >= 0)
    ASSERT(data_len <= client->udpgw_mtu)

    // accept packet
    PacketPassInterface_Done(&o->udp_recv_if);

    if (!o->is_dns) {

        // check header
        if (data_len < sizeof(struct socks_udp_header)) {
            BLog(BLOG_ERROR, "missing header");
            return;
        }

        struct socks_udp_header header;
        memcpy(&header, data, sizeof(header));
        data += sizeof(header);
        data_len -= sizeof(header);
        uint8_t frag = header.frag;
        uint8_t atyp = header.atyp;

        // check fragment
        if (frag) {
            BLog(BLOG_ERROR, "unexpected frag");
            return;
        }

        // remote addr parsed from header
        BAddr remote_addr;

        // parse address
        if (atyp == SOCKS_ATYP_IPV6) {
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

        // reset remote addr
        o->conaddr.remote_addr = remote_addr;

        // print remote addr
        char addr_str[BADDR_MAX_PRINT_LEN];
        BAddr_Print(&remote_addr, addr_str);

        BLog(BLOG_INFO, "receive packet from %s", addr_str);
    }

    // check remaining data
    if (data_len > client->udp_mtu) {
        BLog(BLOG_ERROR, "too much data");
        return;
    }

    // submit to user
    client->handler_received(client->user, o->conaddr.local_addr, o->conaddr.remote_addr, data, data_len);
}

static int conaddr_comparator (void *unused, SocksUdpGwClient_conaddr *v1, SocksUdpGwClient_conaddr *v2)
{
    return BAddr_CompareOrder(&v1->local_addr, &v2->local_addr);
}

static SocksUdpGwClient_connection * find_connection (SocksUdpGwClient *o, SocksUdpGwClient_conaddr conaddr, int is_dns)
{
    BAVLNode *tree_node = BAVL_LookupExact(&o->connections_tree, &conaddr);
    if (!tree_node) {
        return NULL;
    }

    SocksUdpGwClient_connection *con = UPPER_OBJECT(tree_node, SocksUdpGwClient_connection, connections_tree_node);
    if (!con && con->is_dns != is_dns) {
        return NULL;
    }

    return con;
}

static void remove_lru_connection (SocksUdpGwClient *o, SocksUdpGwClient_conaddr conaddr, int is_dns)
{
    ASSERT(!find_connection(o, conaddr, is_dns))
    ASSERT(o->num_connections > 0)

    // get least recently used connection
    SocksUdpGwClient_connection *con = UPPER_OBJECT(LinkedList1_GetFirst(&o->connections_list), SocksUdpGwClient_connection, connections_list_node);

    // free connection
    connection_free(con);
}

static void reset_connection (SocksUdpGwClient *o, SocksUdpGwClient_connection *con, SocksUdpGwClient_conaddr conaddr, int is_dns)
{
    // set new conaddr
    con->conaddr = conaddr;
    con->is_dns = is_dns;
}

static int connection_send (SocksUdpGwClient_connection *o, const uint8_t *data, int data_len)
{
    // get buffer location
    uint8_t *out;
    if (!BufferWriter_StartPacket(&o->udp_send_writer, &out)) {
        BLog(BLOG_ERROR, "out of UDP buffer");
        return 1;
    }
    int out_pos = 0;

    if (!o->is_dns) {

        // write header
        BAddr remote_addr = o->conaddr.remote_addr;
        struct socks_udp_header header;
        header.rsv = 0;
        header.frag = 0;
        if (remote_addr.type == BADDR_TYPE_IPV4) {
            header.atyp = SOCKS_ATYP_IPV4;
        } else {
            header.atyp = SOCKS_ATYP_IPV6;
        }
        memcpy(out + out_pos, &header, sizeof(header));
        out_pos += sizeof(header);

        // write address
        switch (remote_addr.type) {
            case BADDR_TYPE_IPV4: {
                struct udpgw_addr_ipv4 addr_ipv4;
                addr_ipv4.addr_ip = remote_addr.ipv4.ip;
                addr_ipv4.addr_port = remote_addr.ipv4.port;
                memcpy(out + out_pos, &addr_ipv4, sizeof(addr_ipv4));
                out_pos += sizeof(addr_ipv4);
            } break;
            case BADDR_TYPE_IPV6: {
                struct udpgw_addr_ipv6 addr_ipv6;
                memcpy(addr_ipv6.addr_ip, remote_addr.ipv6.ip, sizeof(addr_ipv6.addr_ip));
                addr_ipv6.addr_port = remote_addr.ipv6.port;
                memcpy(out + out_pos, &addr_ipv6, sizeof(addr_ipv6));
                out_pos += sizeof(addr_ipv6);
            } break;
        }
    }

    // write packet to buffer
    memcpy(out + out_pos, data, data_len);
    out_pos += data_len;

    // submit written message
    BufferWriter_EndPacket(&o->udp_send_writer, out_pos);

    return 0;
}

static void connection_first_job_handler (SocksUdpGwClient_connection *con)
{
    connection_send(con, con->first_data, con->first_data_len);
}

static SocksUdpGwClient_connection *connection_init (SocksUdpGwClient *client, SocksUdpGwClient_conaddr conaddr, const uint8_t *data, int data_len, int is_dns)
{
    // allocate structure
    SocksUdpGwClient_connection *o = (SocksUdpGwClient_connection *) malloc(sizeof(*o));
    if (!o) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail;
    }

    // init arguments
    o->client = client;
    o->conaddr = conaddr;
    o->first_data = data;
    o->first_data_len = data_len;
    o->is_dns = is_dns;

    // init first job
    BPending_Init(&o->first_job, BReactor_PendingGroup(client->reactor), (BPending_handler)connection_first_job_handler, o);
    BPending_Set(&o->first_job);

    // init UDP dgram
    if (!BDatagram_Init(&o->udp_dgram, client->socks_server_addr.type, client->reactor, o, (BDatagram_handler)dgram_handler)) {
        goto fail0;
    }

    // set SO_REUSEADDR
    if (!BDatagram_SetReuseAddr(&o->udp_dgram, 1)) {
        BLog(BLOG_ERROR, "set SO_REUSEADDR failed");
        goto fail1;
    }

    // set UDP dgram send address
    BIPAddr ipaddr;
    memset(&ipaddr, 0, sizeof(ipaddr));

    if (o->is_dns) {
        ipaddr.type = client->dnsgw.type;
        BDatagram_SetSendAddrs(&o->udp_dgram, client->dnsgw, ipaddr);
    } else {
        ipaddr.type = client->socks_server_addr.type;
        BDatagram_SetSendAddrs(&o->udp_dgram, client->socks_server_addr, ipaddr);
    }

    // init UDP dgram interfaces
    BDatagram_SendAsync_Init(&o->udp_dgram, client->udp_mtu);
    BDatagram_RecvAsync_Init(&o->udp_dgram, client->udp_mtu);

    // init UDP writer
    BufferWriter_Init(&o->udp_send_writer, client->udp_mtu, BReactor_PendingGroup(client->reactor));

    // init UDP buffer
    if (!PacketBuffer_Init(&o->udp_send_buffer, BufferWriter_GetOutput(&o->udp_send_writer), BDatagram_SendAsync_GetIf(&o->udp_dgram), CONNECTION_UDP_BUFFER_SIZE, BReactor_PendingGroup(client->reactor))) {
        BLog(BLOG_ERROR, "PacketBuffer_Init failed");
        goto fail2;
    }

    // init UDP recv interface
    PacketPassInterface_Init(&o->udp_recv_if, client->udp_mtu, (PacketPassInterface_handler_send)dgram_handler_received, o, BReactor_PendingGroup(client->reactor));

    // init UDP recv buffer
    if (!SinglePacketBuffer_Init(&o->udp_recv_buffer, BDatagram_RecvAsync_GetIf(&o->udp_dgram), &o->udp_recv_if, BReactor_PendingGroup(client->reactor))) {
        BLog(BLOG_ERROR, "SinglePacketBuffer_Init failed");
        goto fail3;
    }

    // insert to connections tree by conaddr
    ASSERT_EXECUTE(BAVL_Insert(&client->connections_tree, &o->connections_tree_node, NULL));

    // insert to connections list
    LinkedList1_Append(&client->connections_list, &o->connections_list_node);

    // increment number of connections
    client->num_connections++;

    // succeed to init
    return o;

fail3:
    PacketPassInterface_Free(&o->udp_recv_if);
    PacketBuffer_Free(&o->udp_send_buffer);
fail2:
    BufferWriter_Free(&o->udp_send_writer);
    BDatagram_RecvAsync_Free(&o->udp_dgram);
    BDatagram_SendAsync_Free(&o->udp_dgram);
fail1:
    BDatagram_Free(&o->udp_dgram);

fail0:
    BPending_Free(&o->first_job);
    free(o);
fail:
    return NULL;
}

static void connection_free (SocksUdpGwClient_connection *o)
{
    ASSERT(o)

    SocksUdpGwClient *client = o->client;

    // decrement number of connections
    client->num_connections--;

    // remove from connections list
    LinkedList1_Remove(&client->connections_list, &o->connections_list_node);

    // remove from connections tree by conaddr
    BAVL_Remove(&client->connections_tree, &o->connections_tree_node);

    // free UDP receive buffer
    SinglePacketBuffer_Free(&o->udp_recv_buffer);

    // free UDP receive interface
    PacketPassInterface_Free(&o->udp_recv_if);

    // free UDP buffer
    PacketBuffer_Free(&o->udp_send_buffer);

    // free UDP writer
    BufferWriter_Free(&o->udp_send_writer);

    // free UDP dgram interfaces
    BDatagram_RecvAsync_Free(&o->udp_dgram);
    BDatagram_SendAsync_Free(&o->udp_dgram);

    // free UDP dgram
    BDatagram_Free(&o->udp_dgram);

    // free structure
    free(o);
}

#else

static void free_socks (SocksUdpGwClient *o)
{
    ASSERT(o->have_socks)

    // disconnect udpgw client from SOCKS
    if (o->socks_up) {
        UdpGwClient_DisconnectServer(&o->udpgw_client);
    }

    // free SOCKS client
    BSocksClient_Free(&o->socks_client);

    // set have no SOCKS
    o->have_socks = 0;
}

static void try_connect (SocksUdpGwClient *o)
{
    ASSERT(!o->have_socks)
    ASSERT(!BTimer_IsRunning(&o->reconnect_timer))

    // init SOCKS client
    if (!BSocksClient_Init(&o->socks_client, o->socks_server_addr, o->auth_info, o->num_auth_info, o->remote_udpgw_addr, (BSocksClient_handler)socks_client_handler, o, o->reactor)) {
        BLog(BLOG_ERROR, "BSocksClient_Init failed");
        goto fail0;
    }

    // set have SOCKS
    o->have_socks = 1;

    // set SOCKS not up
    o->socks_up = 0;

    return;

fail0:
    // set reconnect timer
    BReactor_SetTimer(o->reactor, &o->reconnect_timer);
}

static void reconnect_timer_handler (SocksUdpGwClient *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->have_socks)

    // try connecting
    try_connect(o);
}

static void socks_client_handler (SocksUdpGwClient *o, int event)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_socks)

    switch (event) {
        case BSOCKSCLIENT_EVENT_UP: {
            ASSERT(!o->socks_up)

            BLog(BLOG_INFO, "SOCKS up");

            // connect udpgw client to SOCKS
            if (!UdpGwClient_ConnectServer(&o->udpgw_client, BSocksClient_GetSendInterface(&o->socks_client), BSocksClient_GetRecvInterface(&o->socks_client))) {
                BLog(BLOG_ERROR, "UdpGwClient_ConnectServer failed");
                goto fail0;
            }

            // set SOCKS up
            o->socks_up = 1;

            return;

        fail0:
            // free SOCKS
            free_socks(o);

            // set reconnect timer
            BReactor_SetTimer(o->reactor, &o->reconnect_timer);
        } break;

        case BSOCKSCLIENT_EVENT_ERROR:
        case BSOCKSCLIENT_EVENT_ERROR_CLOSED: {
            BLog(BLOG_INFO, "SOCKS error");

            // free SOCKS
            free_socks(o);

            // set reconnect timer
            BReactor_SetTimer(o->reactor, &o->reconnect_timer);
        } break;

        default: ASSERT(0);
    }
}

static void udpgw_handler_servererror (SocksUdpGwClient *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_socks)
    ASSERT(o->socks_up)

    BLog(BLOG_ERROR, "client reports server error");

    // free SOCKS
    free_socks(o);

    // set reconnect timer
    BReactor_SetTimer(o->reactor, &o->reconnect_timer);
}

static void udpgw_handler_received (SocksUdpGwClient *o, BAddr local_addr, BAddr remote_addr, const uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);

    // submit to user
    o->handler_received(o->user, local_addr, remote_addr, data, data_len);
    return;
}

#endif

int SocksUdpGwClient_Init (SocksUdpGwClient *o, int udp_mtu, int max_connections, int send_buffer_size, btime_t keepalive_time,
                           BAddr socks_server_addr, BAddr dnsgw, const struct BSocksClient_auth_info *auth_info, size_t num_auth_info,
                           BAddr remote_udpgw_addr, btime_t reconnect_time, BReactor *reactor, void *user,
                           SocksUdpGwClient_handler_received handler_received)
{
    // see asserts in UdpGwClient_Init
    ASSERT(!BAddr_IsInvalid(&socks_server_addr))
#ifndef __ANDROID__
    ASSERT(remote_udpgw_addr.type == BADDR_TYPE_IPV4 || remote_udpgw_addr.type == BADDR_TYPE_IPV6)
#endif

    // init arguments
    o->udp_mtu = udp_mtu;
    o->socks_server_addr = socks_server_addr;
    o->auth_info = auth_info;
    o->num_auth_info = num_auth_info;
    o->remote_udpgw_addr = remote_udpgw_addr;
    o->reactor = reactor;
    o->user = user;
    o->handler_received = handler_received;
    o->dnsgw = dnsgw;

#ifdef __ANDROID__
    // compute MTUs
    o->udpgw_mtu = udpgw_compute_mtu(o->udp_mtu);
    o->max_connections = max_connections;

    // limit max connections to number of conid's
    if (o->max_connections > UINT16_MAX + 1) {
        o->max_connections = UINT16_MAX + 1;
    }

    // init connections tree by conaddr
    BAVL_Init(&o->connections_tree, OFFSET_DIFF(SocksUdpGwClient_connection, conaddr, connections_tree_node), (BAVL_comparator)conaddr_comparator, NULL);

    // init connections list
    LinkedList1_Init(&o->connections_list);
#else
    // init udpgw client
    if (!UdpGwClient_Init(&o->udpgw_client, udp_mtu, max_connections, send_buffer_size, keepalive_time, o->reactor, o,
                          (UdpGwClient_handler_servererror)udpgw_handler_servererror,
                          (UdpGwClient_handler_received)udpgw_handler_received
    )) {
        goto fail0;
    }

    // init reconnect timer
    BTimer_Init(&o->reconnect_timer, reconnect_time, (BTimer_handler)reconnect_timer_handler, o);

    // set have no SOCKS
    o->have_socks = 0;

    // try connecting
    try_connect(o);
#endif

    DebugObject_Init(&o->d_obj);
    return 1;

fail0:
    return 0;
}

void SocksUdpGwClient_Free (SocksUdpGwClient *o)
{
    DebugObject_Free(&o->d_obj);

#ifdef __ANDROID__
    // free connections
    while (!LinkedList1_IsEmpty(&o->connections_list)) {
        SocksUdpGwClient_connection *con = UPPER_OBJECT(LinkedList1_GetFirst(&o->connections_list), SocksUdpGwClient_connection, connections_list_node);
        connection_free(con);
    }
#else
    // free SOCKS
    if (o->have_socks) {
        free_socks(o);
    }

    // free reconnect timer
    BReactor_RemoveTimer(o->reactor, &o->reconnect_timer);

    // free udpgw client
    UdpGwClient_Free(&o->udpgw_client);
#endif
}

void SocksUdpGwClient_SubmitPacket (SocksUdpGwClient *o, BAddr local_addr, BAddr remote_addr, int is_dns, const uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    // see asserts in UdpGwClient_SubmitPacket

#ifdef __ANDROID__
    ASSERT(local_addr.type == BADDR_TYPE_IPV4 || local_addr.type == BADDR_TYPE_IPV6)
    ASSERT(remote_addr.type == BADDR_TYPE_IPV4 || remote_addr.type == BADDR_TYPE_IPV6)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->udp_mtu)

    // build conaddr
    SocksUdpGwClient_conaddr conaddr;
    conaddr.local_addr = local_addr;
    conaddr.remote_addr = remote_addr;

    // lookup connection
    SocksUdpGwClient_connection *con = find_connection(o, conaddr, is_dns);

    // if no connection and can't create a new one, reuse the least recently used une
    if (!con && o->num_connections == o->max_connections) {
        remove_lru_connection(o, conaddr, is_dns);
    }

    if (!con) {
        // create new connection
        con = connection_init(o, conaddr, data, data_len, is_dns);

    } else {
        // reset the connection
        reset_connection(o, con, conaddr, is_dns);

        // send packet to existing connection
        int res = connection_send(con, data, data_len);
        if (res == 1) {
            BLog(BLOG_ERROR, "The current UDP connection is broken, recreating a new one");

            // free broken connection
            connection_free(con);

            // create new connection
            con = connection_init(o, conaddr, data, data_len, is_dns);
        } else {
            // remove connection from list
            LinkedList1_Remove(&o->connections_list, &con->connections_list_node);

            // move connection to front of the list
            LinkedList1_Append(&o->connections_list, &con->connections_list_node);
        }
    }
#else
    // submit to udpgw client
    UdpGwClient_SubmitPacket(&o->udpgw_client, local_addr, remote_addr, is_dns, data, data_len);
#endif
}

