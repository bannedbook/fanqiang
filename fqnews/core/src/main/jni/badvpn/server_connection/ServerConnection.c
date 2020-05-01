/**
 * @file ServerConnection.c
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
#include <string.h>
#include <stddef.h>

#include <misc/debug.h>
#include <misc/strdup.h>
#include <base/BLog.h>

#include <server_connection/ServerConnection.h>

#include <generated/blog_channel_ServerConnection.h>

#define STATE_CONNECTING 1
#define STATE_WAITINIT 2
#define STATE_COMPLETE 3

static void report_error (ServerConnection *o);
static void connector_handler (ServerConnection *o, int is_error);
static void pending_handler (ServerConnection *o);
static SECStatus client_auth_data_callback (ServerConnection *o, PRFileDesc *fd, CERTDistNames *caNames, CERTCertificate **pRetCert, SECKEYPrivateKey **pRetKey);
static void connection_handler (ServerConnection *o, int event);
static void sslcon_handler (ServerConnection *o, int event);
static void decoder_handler_error (ServerConnection *o);
static void input_handler_send (ServerConnection *o, uint8_t *data, int data_len);
static void packet_hello (ServerConnection *o, uint8_t *data, int data_len);
static void packet_newclient (ServerConnection *o, uint8_t *data, int data_len);
static void packet_endclient (ServerConnection *o, uint8_t *data, int data_len);
static void packet_inmsg (ServerConnection *o, uint8_t *data, int data_len);
static int start_packet (ServerConnection *o, void **data, int len);
static void end_packet (ServerConnection *o, uint8_t type);
static void newclient_job_handler (ServerConnection *o);

void report_error (ServerConnection *o)
{
    DEBUGERROR(&o->d_err, o->handler_error(o->user))
}

void connector_handler (ServerConnection *o, int is_error)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state == STATE_CONNECTING)
    ASSERT(!o->buffers_released)
    
    // check connection attempt result
    if (is_error) {
        BLog(BLOG_ERROR, "connection failed");
        goto fail0;
    }
    
    BLog(BLOG_NOTICE, "connected");
    
    // init connection
    if (!BConnection_Init(&o->con, BConnection_source_connector(&o->connector), o->reactor, o, (BConnection_handler)connection_handler)) {
        BLog(BLOG_ERROR, "BConnection_Init failed");
        goto fail0;
    }
    
    // init connection interfaces
    BConnection_SendAsync_Init(&o->con);
    BConnection_RecvAsync_Init(&o->con);
    
    StreamPassInterface *send_iface = BConnection_SendAsync_GetIf(&o->con);
    StreamRecvInterface *recv_iface = BConnection_RecvAsync_GetIf(&o->con);
    
    if (o->have_ssl) {
        // create bottom NSPR file descriptor
        if (!BSSLConnection_MakeBackend(&o->bottom_prfd, send_iface, recv_iface, o->twd, o->ssl_flags)) {
            BLog(BLOG_ERROR, "BSSLConnection_MakeBackend failed");
            goto fail0a;
        }
        
        // create SSL file descriptor from the bottom NSPR file descriptor
        if (!(o->ssl_prfd = SSL_ImportFD(NULL, &o->bottom_prfd))) {
            BLog(BLOG_ERROR, "SSL_ImportFD failed");
            ASSERT_FORCE(PR_Close(&o->bottom_prfd) == PR_SUCCESS)
            goto fail0a;
        }
        
        // set client mode
        if (SSL_ResetHandshake(o->ssl_prfd, PR_FALSE) != SECSuccess) {
            BLog(BLOG_ERROR, "SSL_ResetHandshake failed");
            goto fail1;
        }
        
        // set server name
        if (SSL_SetURL(o->ssl_prfd, o->server_name) != SECSuccess) {
            BLog(BLOG_ERROR, "SSL_SetURL failed");
            goto fail1;
        }
        
        // set client certificate callback
        if (SSL_GetClientAuthDataHook(o->ssl_prfd, (SSLGetClientAuthData)client_auth_data_callback, o) != SECSuccess) {
            BLog(BLOG_ERROR, "SSL_GetClientAuthDataHook failed");
            goto fail1;
        }
        
        // init BSSLConnection
        BSSLConnection_Init(&o->sslcon, o->ssl_prfd, 0, BReactor_PendingGroup(o->reactor), o, (BSSLConnection_handler)sslcon_handler);
        
        send_iface = BSSLConnection_GetSendIf(&o->sslcon);
        recv_iface = BSSLConnection_GetRecvIf(&o->sslcon);
    }
    
    // init input chain
    PacketPassInterface_Init(&o->input_interface, SC_MAX_ENC, (PacketPassInterface_handler_send)input_handler_send, o, BReactor_PendingGroup(o->reactor));
    if (!PacketProtoDecoder_Init(&o->input_decoder, recv_iface, &o->input_interface, BReactor_PendingGroup(o->reactor), o, (PacketProtoDecoder_handler_error)decoder_handler_error)) {
        BLog(BLOG_ERROR, "PacketProtoDecoder_Init failed");
        goto fail2;
    }
    
    // set job to send hello
    // this needs to be in here because hello sending must be done after sending started (so we can write into the send buffer),
    // but before receiving started (so we don't get into conflict with the user sending packets)
    BPending_Init(&o->start_job, BReactor_PendingGroup(o->reactor), (BPending_handler)pending_handler, o);
    BPending_Set(&o->start_job);
    
    // init keepalive output branch
    SCKeepaliveSource_Init(&o->output_ka_zero, BReactor_PendingGroup(o->reactor));
    PacketProtoEncoder_Init(&o->output_ka_encoder, SCKeepaliveSource_GetOutput(&o->output_ka_zero), BReactor_PendingGroup(o->reactor));
    
    // init output common
    
    // init sender
    PacketStreamSender_Init(&o->output_sender, send_iface, PACKETPROTO_ENCLEN(SC_MAX_ENC), BReactor_PendingGroup(o->reactor));
    
    // init keepalives
    if (!KeepaliveIO_Init(&o->output_keepaliveio, o->reactor, PacketStreamSender_GetInput(&o->output_sender), PacketProtoEncoder_GetOutput(&o->output_ka_encoder), o->keepalive_interval)) {
        BLog(BLOG_ERROR, "KeepaliveIO_Init failed");
        goto fail3;
    }
    
    // init queue
    PacketPassPriorityQueue_Init(&o->output_queue, KeepaliveIO_GetInput(&o->output_keepaliveio), BReactor_PendingGroup(o->reactor), 0);
    
    // init output local flow
    
    // init queue flow
    PacketPassPriorityQueueFlow_Init(&o->output_local_qflow, &o->output_queue, 0);
    
    // init PacketProtoFlow
    if (!PacketProtoFlow_Init(&o->output_local_oflow, SC_MAX_ENC, o->buffer_size, PacketPassPriorityQueueFlow_GetInput(&o->output_local_qflow), BReactor_PendingGroup(o->reactor))) {
        BLog(BLOG_ERROR, "PacketProtoFlow_Init failed");
        goto fail4;
    }
    o->output_local_if = PacketProtoFlow_GetInput(&o->output_local_oflow);
    
    // have no output packet
    o->output_local_packet_len = -1;
    
    // init output user flow
    PacketPassPriorityQueueFlow_Init(&o->output_user_qflow, &o->output_queue, 1);
    
    // update state
    o->state = STATE_WAITINIT;
    
    return;
    
fail4:
    PacketPassPriorityQueueFlow_Free(&o->output_local_qflow);
    PacketPassPriorityQueue_Free(&o->output_queue);
    KeepaliveIO_Free(&o->output_keepaliveio);
fail3:
    PacketStreamSender_Free(&o->output_sender);
    PacketProtoEncoder_Free(&o->output_ka_encoder);
    SCKeepaliveSource_Free(&o->output_ka_zero);
    BPending_Free(&o->start_job);
    PacketProtoDecoder_Free(&o->input_decoder);
fail2:
    PacketPassInterface_Free(&o->input_interface);
    if (o->have_ssl) {
        BSSLConnection_Free(&o->sslcon);
fail1:
        ASSERT_FORCE(PR_Close(o->ssl_prfd) == PR_SUCCESS)
    }
fail0a:
    BConnection_RecvAsync_Free(&o->con);
    BConnection_SendAsync_Free(&o->con);
    BConnection_Free(&o->con);
fail0:
    // report error
    report_error(o);
}

void pending_handler (ServerConnection *o)
{
    ASSERT(o->state == STATE_WAITINIT)
    ASSERT(!o->buffers_released)
    DebugObject_Access(&o->d_obj);
    
    // send hello
    struct sc_client_hello omsg;
    void *packet;
    if (!start_packet(o, &packet, sizeof(omsg))) {
        BLog(BLOG_ERROR, "no buffer for hello");
        report_error(o);
        return;
    }
    omsg.version = htol16(SC_VERSION);
    memcpy(packet, &omsg, sizeof(omsg));
    end_packet(o, SCID_CLIENTHELLO);
}

SECStatus client_auth_data_callback (ServerConnection *o, PRFileDesc *fd, CERTDistNames *caNames, CERTCertificate **pRetCert, SECKEYPrivateKey **pRetKey)
{
    ASSERT(o->have_ssl)
    DebugObject_Access(&o->d_obj);
    
    CERTCertificate *newcert;
    if (!(newcert = CERT_DupCertificate(o->client_cert))) {
        return SECFailure;
    }
    
    SECKEYPrivateKey *newkey;
    if (!(newkey = SECKEY_CopyPrivateKey(o->client_key))) {
        CERT_DestroyCertificate(newcert);
        return SECFailure;
    }
    
    *pRetCert = newcert;
    *pRetKey = newkey;
    return SECSuccess;
}

void connection_handler (ServerConnection *o, int event)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state >= STATE_WAITINIT)
    ASSERT(!o->buffers_released)
    
    if (event == BCONNECTION_EVENT_RECVCLOSED) {
        BLog(BLOG_INFO, "connection closed");
    } else {
        BLog(BLOG_INFO, "connection error");
    }
    
    report_error(o);
    return;
}

void sslcon_handler (ServerConnection *o, int event)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_ssl)
    ASSERT(o->state >= STATE_WAITINIT)
    ASSERT(!o->buffers_released)
    ASSERT(event == BSSLCONNECTION_EVENT_ERROR)
    
    BLog(BLOG_ERROR, "SSL error");
    
    report_error(o);
    return;
}

void decoder_handler_error (ServerConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state >= STATE_WAITINIT)
    ASSERT(!o->buffers_released)
    
    BLog(BLOG_ERROR, "decoder error");
    
    report_error(o);
    return;
}

void input_handler_send (ServerConnection *o, uint8_t *data, int data_len)
{
    ASSERT(o->state >= STATE_WAITINIT)
    ASSERT(!o->buffers_released)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= SC_MAX_ENC)
    DebugObject_Access(&o->d_obj);
    
    // accept packet
    PacketPassInterface_Done(&o->input_interface);
    
    // parse header
    if (data_len < sizeof(struct sc_header)) {
        BLog(BLOG_ERROR, "packet too short (no sc header)");
        report_error(o);
        return;
    }
    struct sc_header header;
    memcpy(&header, data, sizeof(header));
    data += sizeof(header);
    data_len -= sizeof(header);
    uint8_t type = ltoh8(header.type);
    
    // call appropriate handler based on packet type
    switch (type) {
        case SCID_SERVERHELLO:
            packet_hello(o, data, data_len);
            return;
        case SCID_NEWCLIENT:
            packet_newclient(o, data, data_len);
            return;
        case SCID_ENDCLIENT:
            packet_endclient(o, data, data_len);
            return;
        case SCID_INMSG:
            packet_inmsg(o, data, data_len);
            return;
        default:
            BLog(BLOG_ERROR, "unknown packet type %d", (int)type);
            report_error(o);
            return;
    }
}

void packet_hello (ServerConnection *o, uint8_t *data, int data_len)
{
    if (o->state != STATE_WAITINIT) {
        BLog(BLOG_ERROR, "hello: not expected");
        report_error(o);
        return;
    }
    
    if (data_len != sizeof(struct sc_server_hello)) {
        BLog(BLOG_ERROR, "hello: invalid length");
        report_error(o);
        return;
    }
    struct sc_server_hello msg;
    memcpy(&msg, data, sizeof(msg));
    peerid_t id = ltoh16(msg.id);
    
    // change state
    o->state = STATE_COMPLETE;
    
    // report
    o->handler_ready(o->user, id, msg.clientAddr);
    return;
}

void packet_newclient (ServerConnection *o, uint8_t *data, int data_len)
{
    if (o->state != STATE_COMPLETE) {
        BLog(BLOG_ERROR, "newclient: not expected");
        report_error(o);
        return;
    }
    
    if (data_len < sizeof(struct sc_server_newclient) || data_len > sizeof(struct sc_server_newclient) + SCID_NEWCLIENT_MAX_CERT_LEN) {
        BLog(BLOG_ERROR, "newclient: invalid length");
        report_error(o);
        return;
    }
    
    struct sc_server_newclient msg;
    memcpy(&msg, data, sizeof(msg));
    peerid_t id = ltoh16(msg.id);
    
    // schedule reporting new client
    o->newclient_data = data;
    o->newclient_data_len = data_len;
    BPending_Set(&o->newclient_job);
    
    // send acceptpeer
    struct sc_client_acceptpeer omsg;
    void *packet;
    if (!start_packet(o, &packet, sizeof(omsg))) {
        BLog(BLOG_ERROR, "newclient: out of buffer for acceptpeer");
        report_error(o);
        return;
    }
    omsg.clientid = htol16(id);
    memcpy(packet, &omsg, sizeof(omsg));
    end_packet(o, SCID_ACCEPTPEER);
}

void packet_endclient (ServerConnection *o, uint8_t *data, int data_len)
{
    if (o->state != STATE_COMPLETE) {
        BLog(BLOG_ERROR, "endclient: not expected");
        report_error(o);
        return;
    }
    
    if (data_len != sizeof(struct sc_server_endclient)) {
        BLog(BLOG_ERROR, "endclient: invalid length");
        report_error(o);
        return;
    }
    
    struct sc_server_endclient msg;
    memcpy(&msg, data, sizeof(msg));
    peerid_t id = ltoh16(msg.id);
    
    // report
    o->handler_endclient(o->user, id);
    return;
}

void packet_inmsg (ServerConnection *o, uint8_t *data, int data_len)
{
    if (o->state != STATE_COMPLETE) {
        BLog(BLOG_ERROR, "inmsg: not expected");
        report_error(o);
        return;
    }
    
    if (data_len < sizeof(struct sc_server_inmsg)) {
        BLog(BLOG_ERROR, "inmsg: missing header");
        report_error(o);
        return;
    }
    
    if (data_len > sizeof(struct sc_server_inmsg) + SC_MAX_MSGLEN) {
        BLog(BLOG_ERROR, "inmsg: too long");
        report_error(o);
        return;
    }
    
    struct sc_server_inmsg msg;
    memcpy(&msg, data, sizeof(msg));
    peerid_t peer_id = ltoh16(msg.clientid);
    uint8_t *payload = data + sizeof(struct sc_server_inmsg);
    int payload_len = data_len - sizeof(struct sc_server_inmsg);
    
    // report
    o->handler_message(o->user, peer_id, payload, payload_len);
    return;
}

int start_packet (ServerConnection *o, void **data, int len)
{
    ASSERT(o->state >= STATE_WAITINIT)
    ASSERT(o->output_local_packet_len == -1)
    ASSERT(len >= 0)
    ASSERT(len <= SC_MAX_PAYLOAD)
    ASSERT(data || len == 0)
    
    // obtain memory location
    if (!BufferWriter_StartPacket(o->output_local_if, &o->output_local_packet)) {
        BLog(BLOG_ERROR, "out of buffer");
        return 0;
    }
    
    o->output_local_packet_len = len;
    
    if (data) {
        *data = o->output_local_packet + sizeof(struct sc_header);
    }
    
    return 1;
}

void end_packet (ServerConnection *o, uint8_t type)
{
    ASSERT(o->state >= STATE_WAITINIT)
    ASSERT(o->output_local_packet_len >= 0)
    ASSERT(o->output_local_packet_len <= SC_MAX_PAYLOAD)
    
    // write header
    struct sc_header header;
    header.type = htol8(type);
    memcpy(o->output_local_packet, &header, sizeof(header));
    
    // finish writing packet
    BufferWriter_EndPacket(o->output_local_if, sizeof(struct sc_header) + o->output_local_packet_len);
    
    o->output_local_packet_len = -1;
}

int ServerConnection_Init (
    ServerConnection *o,
    BReactor *reactor,
    BThreadWorkDispatcher *twd,
    BAddr addr,
    int keepalive_interval,
    int buffer_size,
    int have_ssl,
    int ssl_flags,
    CERTCertificate *client_cert,
    SECKEYPrivateKey *client_key,
    const char *server_name,
    void *user,
    ServerConnection_handler_error handler_error,
    ServerConnection_handler_ready handler_ready,
    ServerConnection_handler_newclient handler_newclient,
    ServerConnection_handler_endclient handler_endclient,
    ServerConnection_handler_message handler_message
)
{
    ASSERT(keepalive_interval > 0)
    ASSERT(buffer_size > 0)
    ASSERT(have_ssl == 0 || have_ssl == 1)
    ASSERT(!have_ssl || server_name)
    
    // init arguments
    o->reactor = reactor;
    o->twd = twd;
    o->keepalive_interval = keepalive_interval;
    o->buffer_size = buffer_size;
    o->have_ssl = have_ssl;
    if (have_ssl) {
        o->ssl_flags = ssl_flags;
        o->client_cert = client_cert;
        o->client_key = client_key;
    }
    o->user = user;
    o->handler_error = handler_error;
    o->handler_ready = handler_ready;
    o->handler_newclient = handler_newclient;
    o->handler_endclient = handler_endclient;
    o->handler_message = handler_message;
    
    o->server_name = NULL;
    if (have_ssl && !(o->server_name = b_strdup(server_name))) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    if (!BConnection_AddressSupported(addr)) {
        BLog(BLOG_ERROR, "BConnection_AddressSupported failed");
        goto fail1;
    }
    
    // init connector
    if (!BConnector_Init(&o->connector, addr, o->reactor, o, (BConnector_handler)connector_handler)) {
        BLog(BLOG_ERROR, "BConnector_Init failed");
        goto fail1;
    }
    
    // init newclient job
    BPending_Init(&o->newclient_job, BReactor_PendingGroup(o->reactor), (BPending_handler)newclient_job_handler, o);
    
    // set state
    o->state = STATE_CONNECTING;
    o->buffers_released = 0;
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(o->reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    free(o->server_name);
fail0:
    return 0;
}

void ServerConnection_Free (ServerConnection *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    
    if (o->state > STATE_CONNECTING) {
        // allow freeing queue flows
        PacketPassPriorityQueue_PrepareFree(&o->output_queue);
        
        // stop using any buffers before they get freed
        if (o->have_ssl && !o->buffers_released) {
            BSSLConnection_ReleaseBuffers(&o->sslcon);
        }
        
        // free output user flow
        PacketPassPriorityQueueFlow_Free(&o->output_user_qflow);
        
        // free output local flow
        PacketProtoFlow_Free(&o->output_local_oflow);
        PacketPassPriorityQueueFlow_Free(&o->output_local_qflow);
        
        // free output common
        PacketPassPriorityQueue_Free(&o->output_queue);
        KeepaliveIO_Free(&o->output_keepaliveio);
        PacketStreamSender_Free(&o->output_sender);
        
        // free output keep-alive branch
        PacketProtoEncoder_Free(&o->output_ka_encoder);
        SCKeepaliveSource_Free(&o->output_ka_zero);
        
        // free job
        BPending_Free(&o->start_job);
        
        // free input chain
        PacketProtoDecoder_Free(&o->input_decoder);
        PacketPassInterface_Free(&o->input_interface);
        
        // free SSL
        if (o->have_ssl) {
            BSSLConnection_Free(&o->sslcon);
            ASSERT_FORCE(PR_Close(o->ssl_prfd) == PR_SUCCESS)
        }
        
        // free connection interfaces
        BConnection_RecvAsync_Free(&o->con);
        BConnection_SendAsync_Free(&o->con);
        
        // free connection
        BConnection_Free(&o->con);
    }
    
    // free newclient job
    BPending_Free(&o->newclient_job);
    
    // free connector
    BConnector_Free(&o->connector);
    
    // free server name
    free(o->server_name);
}

void ServerConnection_ReleaseBuffers (ServerConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->buffers_released)
    
    if (o->state > STATE_CONNECTING && o->have_ssl) {
        BSSLConnection_ReleaseBuffers(&o->sslcon);
    }
    
    o->buffers_released = 1;
}

PacketPassInterface * ServerConnection_GetSendInterface (ServerConnection *o)
{
    ASSERT(o->state == STATE_COMPLETE)
    DebugError_AssertNoError(&o->d_err);
    DebugObject_Access(&o->d_obj);
    
    return PacketPassPriorityQueueFlow_GetInput(&o->output_user_qflow);
}

void newclient_job_handler (ServerConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state == STATE_COMPLETE)
    
    struct sc_server_newclient msg;
    memcpy(&msg, o->newclient_data, sizeof(msg));
    peerid_t id = ltoh16(msg.id);
    int flags = ltoh16(msg.flags);
    
    uint8_t *cert_data = o->newclient_data + sizeof(msg);
    int cert_len = o->newclient_data_len - sizeof(msg);
    
    // report new client
    o->handler_newclient(o->user, id, flags, cert_data, cert_len);
    return;
}
