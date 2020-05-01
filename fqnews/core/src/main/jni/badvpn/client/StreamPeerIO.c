/**
 * @file StreamPeerIO.c
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

#include <stdlib.h>

#include <nss/ssl.h>
#include <nss/sslerr.h>

#include <misc/offset.h>
#include <misc/byteorder.h>

#include <client/StreamPeerIO.h>

#include <generated/blog_channel_StreamPeerIO.h>

#define MODE_NONE 0
#define MODE_CONNECT 1
#define MODE_LISTEN 2

#define CONNECT_STATE_CONNECTING 0
#define CONNECT_STATE_HANDSHAKE 1
#define CONNECT_STATE_SENDING 2
#define CONNECT_STATE_SENT 3
#define CONNECT_STATE_FINISHED 4

#define LISTEN_STATE_LISTENER 0
#define LISTEN_STATE_GOTCLIENT 1
#define LISTEN_STATE_FINISHED 2

#define PeerLog(_o, ...) BLog_LogViaFunc((_o)->logfunc, (_o)->user, BLOG_CURRENT_CHANNEL, __VA_ARGS__)

static void decoder_handler_error (StreamPeerIO *pio);
static void connector_handler (StreamPeerIO *pio, int is_error);
static void connection_handler (StreamPeerIO *pio, int event);
static void connect_sslcon_handler (StreamPeerIO *pio, int event);
static void pwsender_handler (StreamPeerIO *pio);
static void listener_handler_client (StreamPeerIO *pio, sslsocket *sock);
static int init_io (StreamPeerIO *pio, sslsocket *sock);
static void free_io (StreamPeerIO *pio);
static void sslcon_handler (StreamPeerIO *pio, int event);
static SECStatus client_auth_certificate_callback (StreamPeerIO *pio, PRFileDesc *fd, PRBool checkSig, PRBool isServer);
static SECStatus client_client_auth_data_callback (StreamPeerIO *pio, PRFileDesc *fd, CERTDistNames *caNames, CERTCertificate **pRetCert, SECKEYPrivateKey **pRetKey);
static int compare_certificate (StreamPeerIO *pio, CERTCertificate *cert);
static void reset_state (StreamPeerIO *pio);
static void reset_and_report_error (StreamPeerIO *pio);

void decoder_handler_error (StreamPeerIO *pio)
{
    DebugObject_Access(&pio->d_obj);
    
    PeerLog(pio, BLOG_ERROR, "decoder error");
    
    reset_and_report_error(pio);
    return;
}

void connector_handler (StreamPeerIO *pio, int is_error)
{
    DebugObject_Access(&pio->d_obj);
    ASSERT(pio->mode == MODE_CONNECT)
    ASSERT(pio->connect.state == CONNECT_STATE_CONNECTING)
    
    // check connection result
    if (is_error) {
        PeerLog(pio, BLOG_NOTICE, "connection failed");
        goto fail0;
    }
    
    // init connection
    if (!BConnection_Init(&pio->connect.sock.con, BConnection_source_connector(&pio->connect.connector), pio->reactor, pio, (BConnection_handler)connection_handler)) {
        PeerLog(pio, BLOG_ERROR, "BConnection_Init failed");
        goto fail0;
    }
    
    if (pio->ssl) {
        // init connection interfaces
        BConnection_SendAsync_Init(&pio->connect.sock.con);
        BConnection_RecvAsync_Init(&pio->connect.sock.con);
        
        // create bottom NSPR file descriptor
        if (!BSSLConnection_MakeBackend(&pio->connect.sock.bottom_prfd, BConnection_SendAsync_GetIf(&pio->connect.sock.con), BConnection_RecvAsync_GetIf(&pio->connect.sock.con), pio->twd, pio->ssl_flags)) {
            PeerLog(pio, BLOG_ERROR, "BSSLConnection_MakeBackend failed");
            goto fail1;
        }
        
        // create SSL file descriptor from the bottom NSPR file descriptor
        if (!(pio->connect.sock.ssl_prfd = SSL_ImportFD(NULL, &pio->connect.sock.bottom_prfd))) {
            ASSERT_FORCE(PR_Close(&pio->connect.sock.bottom_prfd) == PR_SUCCESS)
            goto fail1;
        }
        
        // set client mode
        if (SSL_ResetHandshake(pio->connect.sock.ssl_prfd, PR_FALSE) != SECSuccess) {
            PeerLog(pio, BLOG_ERROR, "SSL_ResetHandshake failed");
            goto fail2;
        }
        
        // set verify peer certificate hook
        if (SSL_AuthCertificateHook(pio->connect.sock.ssl_prfd, (SSLAuthCertificate)client_auth_certificate_callback, pio) != SECSuccess) {
            PeerLog(pio, BLOG_ERROR, "SSL_AuthCertificateHook failed");
            goto fail2;
        }
        
        // set client certificate callback
        if (SSL_GetClientAuthDataHook(pio->connect.sock.ssl_prfd, (SSLGetClientAuthData)client_client_auth_data_callback, pio) != SECSuccess) {
            PeerLog(pio, BLOG_ERROR, "SSL_GetClientAuthDataHook failed");
            goto fail2;
        }
        
        // init BSSLConnection
        BSSLConnection_Init(&pio->connect.sslcon, pio->connect.sock.ssl_prfd, 1, BReactor_PendingGroup(pio->reactor), pio, (BSSLConnection_handler)connect_sslcon_handler);
        
        // change state
        pio->connect.state = CONNECT_STATE_HANDSHAKE;
    } else {
        // init connection send interface
        BConnection_SendAsync_Init(&pio->connect.sock.con);
        
        // init password sender
        SingleStreamSender_Init(&pio->connect.pwsender, (uint8_t *)&pio->connect.password, sizeof(pio->connect.password), BConnection_SendAsync_GetIf(&pio->connect.sock.con), BReactor_PendingGroup(pio->reactor), pio, (SingleStreamSender_handler)pwsender_handler);
        
        // change state
        pio->connect.state = CONNECT_STATE_SENDING;
    }
    
    return;

    if (pio->ssl) {
fail2:
        ASSERT_FORCE(PR_Close(pio->connect.sock.ssl_prfd) == PR_SUCCESS)
fail1:
        BConnection_RecvAsync_Free(&pio->connect.sock.con);
        BConnection_SendAsync_Free(&pio->connect.sock.con);
    }
    BConnection_Free(&pio->connect.sock.con);
fail0:
    reset_and_report_error(pio);
    return;
}

void connection_handler (StreamPeerIO *pio, int event)
{
    DebugObject_Access(&pio->d_obj);
    ASSERT(pio->mode == MODE_CONNECT || pio->mode == MODE_LISTEN)
    ASSERT(!(pio->mode == MODE_CONNECT) || pio->connect.state >= CONNECT_STATE_HANDSHAKE)
    ASSERT(!(pio->mode == MODE_LISTEN) || pio->listen.state >= LISTEN_STATE_FINISHED)
    
    if (event == BCONNECTION_EVENT_RECVCLOSED) {
        PeerLog(pio, BLOG_NOTICE, "connection closed");
    } else {
        PeerLog(pio, BLOG_NOTICE, "connection error");
    }
    
    reset_and_report_error(pio);
    return;
}

void connect_sslcon_handler (StreamPeerIO *pio, int event)
{
    DebugObject_Access(&pio->d_obj);
    ASSERT(pio->ssl)
    ASSERT(pio->mode == MODE_CONNECT)
    ASSERT(pio->connect.state == CONNECT_STATE_HANDSHAKE || pio->connect.state == CONNECT_STATE_SENDING)
    ASSERT(event == BSSLCONNECTION_EVENT_UP || event == BSSLCONNECTION_EVENT_ERROR)
    
    if (event == BSSLCONNECTION_EVENT_ERROR) {
        PeerLog(pio, BLOG_NOTICE, "SSL error");
        
        reset_and_report_error(pio);
        return;
    }
    
    // handshake complete
    ASSERT(pio->connect.state == CONNECT_STATE_HANDSHAKE)
    
    // remove client certificate callback
    if (SSL_GetClientAuthDataHook(pio->connect.sock.ssl_prfd, NULL, NULL) != SECSuccess) {
        PeerLog(pio, BLOG_ERROR, "SSL_GetClientAuthDataHook failed");
        goto fail0;
    }
    
    // remove verify peer certificate callback
    if (SSL_AuthCertificateHook(pio->connect.sock.ssl_prfd, NULL, NULL) != SECSuccess) {
        PeerLog(pio, BLOG_ERROR, "SSL_AuthCertificateHook failed");
        goto fail0;
    }
    
    // init password sender
    SingleStreamSender_Init(&pio->connect.pwsender, (uint8_t *)&pio->connect.password, sizeof(pio->connect.password), BSSLConnection_GetSendIf(&pio->connect.sslcon), BReactor_PendingGroup(pio->reactor), pio, (SingleStreamSender_handler)pwsender_handler);
    
    // change state
    pio->connect.state = CONNECT_STATE_SENDING;
    
    return;
    
fail0:
    reset_and_report_error(pio);
    return;
}

void pwsender_handler (StreamPeerIO *pio)
{
    DebugObject_Access(&pio->d_obj);
    ASSERT(pio->mode == MODE_CONNECT)
    ASSERT(pio->connect.state == CONNECT_STATE_SENDING)
    
    // stop using any buffers before they get freed
    if (pio->ssl) {
        BSSLConnection_ReleaseBuffers(&pio->connect.sslcon);
    }
    
    // free password sender
    SingleStreamSender_Free(&pio->connect.pwsender);
    
    if (pio->ssl) {
        // free BSSLConnection (we used the send interface)
        BSSLConnection_Free(&pio->connect.sslcon);
    } else {
        // init connection send interface
        BConnection_SendAsync_Free(&pio->connect.sock.con);
    }
    
    // change state
    pio->connect.state = CONNECT_STATE_SENT;
    
    // setup i/o
    if (!init_io(pio, &pio->connect.sock)) {
        goto fail0;
    }
    
    // change state
    pio->connect.state = CONNECT_STATE_FINISHED;
    
    return;
    
fail0:
    reset_and_report_error(pio);
    return;
}

void listener_handler_client (StreamPeerIO *pio, sslsocket *sock)
{
    DebugObject_Access(&pio->d_obj);
    ASSERT(pio->mode == MODE_LISTEN)
    ASSERT(pio->listen.state == LISTEN_STATE_LISTENER)
    
    // remember socket
    pio->listen.sock = sock;
    
    // set connection handler
    BConnection_SetHandlers(&pio->listen.sock->con, pio, (BConnection_handler)connection_handler);
    
    // change state
    pio->listen.state = LISTEN_STATE_GOTCLIENT;
    
    // check ceritficate
    if (pio->ssl) {
        CERTCertificate *peer_cert = SSL_PeerCertificate(pio->listen.sock->ssl_prfd);
        if (!peer_cert) {
            PeerLog(pio, BLOG_ERROR, "SSL_PeerCertificate failed");
            goto fail0;
        }
        
        // compare certificate to the one provided by the server
        if (!compare_certificate(pio, peer_cert)) {
            CERT_DestroyCertificate(peer_cert);
            goto fail0;
        }
        
        CERT_DestroyCertificate(peer_cert);
    }
    
    // setup i/o
    if (!init_io(pio, pio->listen.sock)) {
        goto fail0;
    }
    
    // change state
    pio->listen.state = LISTEN_STATE_FINISHED;
    
    return;
    
fail0:
    reset_and_report_error(pio);
    return;
}

int init_io (StreamPeerIO *pio, sslsocket *sock)
{
    ASSERT(!pio->sock)
    
    // limit socket send buffer, else our scheduling is pointless
    if (pio->sock_sndbuf > 0) {
        if (!BConnection_SetSendBuffer(&sock->con, pio->sock_sndbuf)) {
            PeerLog(pio, BLOG_WARNING, "BConnection_SetSendBuffer failed");
        }
    }
    
    if (pio->ssl) {
        // init BSSLConnection
        BSSLConnection_Init(&pio->sslcon, sock->ssl_prfd, 0, BReactor_PendingGroup(pio->reactor), pio, (BSSLConnection_handler)sslcon_handler);
    } else {
        // init connection interfaces
        BConnection_SendAsync_Init(&sock->con);
        BConnection_RecvAsync_Init(&sock->con);
    }
    
    StreamPassInterface *send_if = (pio->ssl ? BSSLConnection_GetSendIf(&pio->sslcon) : BConnection_SendAsync_GetIf(&sock->con));
    StreamRecvInterface *recv_if = (pio->ssl ? BSSLConnection_GetRecvIf(&pio->sslcon) : BConnection_RecvAsync_GetIf(&sock->con));
    
    // init receiving
    StreamRecvConnector_ConnectInput(&pio->input_connector, recv_if);
    
    // init sending
    PacketStreamSender_Init(&pio->output_pss, send_if, PACKETPROTO_ENCLEN(pio->payload_mtu), BReactor_PendingGroup(pio->reactor));
    PacketPassConnector_ConnectOutput(&pio->output_connector, PacketStreamSender_GetInput(&pio->output_pss));
    
    pio->sock = sock;
    
    return 1;
}

void free_io (StreamPeerIO *pio)
{
    ASSERT(pio->sock)
    
    // stop using any buffers before they get freed
    if (pio->ssl) {
        BSSLConnection_ReleaseBuffers(&pio->sslcon);
    }
    
    // reset decoder
    PacketProtoDecoder_Reset(&pio->input_decoder);
    
    // free sending
    PacketPassConnector_DisconnectOutput(&pio->output_connector);
    PacketStreamSender_Free(&pio->output_pss);
    
    // free receiving
    StreamRecvConnector_DisconnectInput(&pio->input_connector);
    
    if (pio->ssl) {
        // free BSSLConnection
        BSSLConnection_Free(&pio->sslcon);
    } else {
        // free connection interfaces
        BConnection_RecvAsync_Free(&pio->sock->con);
        BConnection_SendAsync_Free(&pio->sock->con);
    }
    
    pio->sock = NULL;
}

void sslcon_handler (StreamPeerIO *pio, int event)
{
    DebugObject_Access(&pio->d_obj);
    ASSERT(pio->ssl)
    ASSERT(pio->mode == MODE_CONNECT || pio->mode == MODE_LISTEN)
    ASSERT(!(pio->mode == MODE_CONNECT) || pio->connect.state == CONNECT_STATE_FINISHED)
    ASSERT(!(pio->mode == MODE_LISTEN) || pio->listen.state == LISTEN_STATE_FINISHED)
    ASSERT(event == BSSLCONNECTION_EVENT_ERROR)
    
    PeerLog(pio, BLOG_NOTICE, "SSL error");
    
    reset_and_report_error(pio);
    return;
}

SECStatus client_auth_certificate_callback (StreamPeerIO *pio, PRFileDesc *fd, PRBool checkSig, PRBool isServer)
{
    ASSERT(pio->ssl)
    ASSERT(pio->mode == MODE_CONNECT)
    ASSERT(pio->connect.state == CONNECT_STATE_HANDSHAKE)
    DebugObject_Access(&pio->d_obj);
    
    // This callback is used to bypass checking the server's domain name, as peers
    // don't have domain names. We byte-compare the certificate to the one reported
    // by the server anyway.
    
    SECStatus ret = SECFailure;
    
    CERTCertificate *server_cert = SSL_PeerCertificate(pio->connect.sock.ssl_prfd);
    if (!server_cert) {
        PeerLog(pio, BLOG_ERROR, "SSL_PeerCertificate failed");
        PORT_SetError(SSL_ERROR_BAD_CERTIFICATE);
        goto fail1;
    }
    
    if (CERT_VerifyCertNow(CERT_GetDefaultCertDB(), server_cert, PR_TRUE, certUsageSSLServer, SSL_RevealPinArg(pio->connect.sock.ssl_prfd)) != SECSuccess) {
        goto fail2;
    }
    
    // compare to certificate provided by the server
    if (!compare_certificate(pio, server_cert)) {
        PORT_SetError(SSL_ERROR_BAD_CERTIFICATE);
        goto fail2;
    }
    
    ret = SECSuccess;
    
fail2:
    CERT_DestroyCertificate(server_cert);
fail1:
    return ret;
}

SECStatus client_client_auth_data_callback (StreamPeerIO *pio, PRFileDesc *fd, CERTDistNames *caNames, CERTCertificate **pRetCert, SECKEYPrivateKey **pRetKey)
{
    ASSERT(pio->ssl)
    ASSERT(pio->mode == MODE_CONNECT)
    ASSERT(pio->connect.state == CONNECT_STATE_HANDSHAKE)
    DebugObject_Access(&pio->d_obj);
    
    CERTCertificate *cert = CERT_DupCertificate(pio->connect.ssl_cert);
    if (!cert) {
        PeerLog(pio, BLOG_ERROR, "CERT_DupCertificate failed");
        goto fail0;
    }
    
    SECKEYPrivateKey *key = SECKEY_CopyPrivateKey(pio->connect.ssl_key);
    if (!key) {
        PeerLog(pio, BLOG_ERROR, "SECKEY_CopyPrivateKey failed");
        goto fail1;
    }
    
    *pRetCert = cert;
    *pRetKey = key;
    return SECSuccess;
    
fail1:
    CERT_DestroyCertificate(cert);
fail0:
    return SECFailure;
}

int compare_certificate (StreamPeerIO *pio, CERTCertificate *cert)
{
    ASSERT(pio->ssl)
    
    SECItem der = cert->derCert;
    if (der.len != pio->ssl_peer_cert_len || memcmp(der.data, pio->ssl_peer_cert, der.len)) {
        PeerLog(pio, BLOG_NOTICE, "Client certificate doesn't match");
        return 0;
    }
    
    return 1;
}

void reset_state (StreamPeerIO *pio)
{
    // free resources
    switch (pio->mode) {
        case MODE_NONE:
            break;
        case MODE_LISTEN:
            switch (pio->listen.state) {
                case LISTEN_STATE_FINISHED:
                    free_io(pio);
                case LISTEN_STATE_GOTCLIENT:
                    if (pio->ssl) {
                        ASSERT_FORCE(PR_Close(pio->listen.sock->ssl_prfd) == PR_SUCCESS)
                        BConnection_RecvAsync_Free(&pio->listen.sock->con);
                        BConnection_SendAsync_Free(&pio->listen.sock->con);
                    }
                    BConnection_Free(&pio->listen.sock->con);
                    free(pio->listen.sock);
                case LISTEN_STATE_LISTENER:
                    if (pio->listen.state == LISTEN_STATE_LISTENER) {
                        PasswordListener_RemoveEntry(pio->listen.listener, &pio->listen.pwentry);
                    }
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case MODE_CONNECT:
            switch (pio->connect.state) {
                case CONNECT_STATE_FINISHED:
                    free_io(pio);
                case CONNECT_STATE_SENT:
                case CONNECT_STATE_SENDING:
                    if (pio->connect.state == CONNECT_STATE_SENDING) {
                        if (pio->ssl) {
                            BSSLConnection_ReleaseBuffers(&pio->connect.sslcon);
                        }
                        SingleStreamSender_Free(&pio->connect.pwsender);
                        if (!pio->ssl) {
                            BConnection_SendAsync_Free(&pio->connect.sock.con);
                        }
                    }
                case CONNECT_STATE_HANDSHAKE:
                    if (pio->ssl) {
                        if (pio->connect.state == CONNECT_STATE_HANDSHAKE || pio->connect.state == CONNECT_STATE_SENDING) {
                            BSSLConnection_Free(&pio->connect.sslcon);
                        }
                        ASSERT_FORCE(PR_Close(pio->connect.sock.ssl_prfd) == PR_SUCCESS)
                        BConnection_RecvAsync_Free(&pio->connect.sock.con);
                        BConnection_SendAsync_Free(&pio->connect.sock.con);
                    }
                    BConnection_Free(&pio->connect.sock.con);
                case CONNECT_STATE_CONNECTING:
                    BConnector_Free(&pio->connect.connector);
                    break;
                default:
                    ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }
    
    // set mode none
    pio->mode = MODE_NONE;
    
    ASSERT(!pio->sock)
}

void reset_and_report_error (StreamPeerIO *pio)
{
    reset_state(pio);
    
    pio->handler_error(pio->user);
    return;
}

int StreamPeerIO_Init (
    StreamPeerIO *pio,
    BReactor *reactor,
    BThreadWorkDispatcher *twd,
    int ssl,
    int ssl_flags,
    uint8_t *ssl_peer_cert,
    int ssl_peer_cert_len,
    int payload_mtu,
    int sock_sndbuf,
    PacketPassInterface *user_recv_if,
    BLog_logfunc logfunc,
    StreamPeerIO_handler_error handler_error,
    void *user
)
{
    ASSERT(ssl == 0 || ssl == 1)
    ASSERT(payload_mtu >= 0)
    ASSERT(PacketPassInterface_GetMTU(user_recv_if) >= payload_mtu)
    ASSERT(handler_error)
    
    // init arguments
    pio->reactor = reactor;
    pio->twd = twd;
    pio->ssl = ssl;
    if (pio->ssl) {
        pio->ssl_flags = ssl_flags;
        pio->ssl_peer_cert = ssl_peer_cert;
        pio->ssl_peer_cert_len = ssl_peer_cert_len;
    }
    pio->payload_mtu = payload_mtu;
    pio->sock_sndbuf = sock_sndbuf;
    pio->logfunc = logfunc;
    pio->handler_error = handler_error;
    pio->user = user;
    
    // check payload MTU
    if (pio->payload_mtu > PACKETPROTO_MAXPAYLOAD) {
        PeerLog(pio, BLOG_ERROR, "payload MTU is too large");
        goto fail0;
    }
    
    // init receiveing objects
    StreamRecvConnector_Init(&pio->input_connector, BReactor_PendingGroup(pio->reactor));
    if (!PacketProtoDecoder_Init(&pio->input_decoder, StreamRecvConnector_GetOutput(&pio->input_connector), user_recv_if, BReactor_PendingGroup(pio->reactor), pio,
        (PacketProtoDecoder_handler_error)decoder_handler_error
    )) {
        PeerLog(pio, BLOG_ERROR, "FlowErrorDomain_Init failed");
        goto fail1;
    }
    
    // init sending objects
    PacketCopier_Init(&pio->output_user_copier, pio->payload_mtu, BReactor_PendingGroup(pio->reactor));
    PacketProtoEncoder_Init(&pio->output_user_ppe, PacketCopier_GetOutput(&pio->output_user_copier), BReactor_PendingGroup(pio->reactor));
    PacketPassConnector_Init(&pio->output_connector, PACKETPROTO_ENCLEN(pio->payload_mtu), BReactor_PendingGroup(pio->reactor));
    if (!SinglePacketBuffer_Init(&pio->output_user_spb, PacketProtoEncoder_GetOutput(&pio->output_user_ppe), PacketPassConnector_GetInput(&pio->output_connector), BReactor_PendingGroup(pio->reactor))) {
        PeerLog(pio, BLOG_ERROR, "SinglePacketBuffer_Init failed");
        goto fail2;
    }
    
    // set mode none
    pio->mode = MODE_NONE;
    
    // set no socket
    pio->sock = NULL;
    
    DebugObject_Init(&pio->d_obj);
    return 1;
    
fail2:
    PacketPassConnector_Free(&pio->output_connector);
    PacketProtoEncoder_Free(&pio->output_user_ppe);
    PacketCopier_Free(&pio->output_user_copier);
    PacketProtoDecoder_Free(&pio->input_decoder);
fail1:
    StreamRecvConnector_Free(&pio->input_connector);
fail0:
    return 0;
}

void StreamPeerIO_Free (StreamPeerIO *pio)
{
    DebugObject_Free(&pio->d_obj);

    // reset state
    reset_state(pio);
    
    // free sending objects
    SinglePacketBuffer_Free(&pio->output_user_spb);
    PacketPassConnector_Free(&pio->output_connector);
    PacketProtoEncoder_Free(&pio->output_user_ppe);
    PacketCopier_Free(&pio->output_user_copier);
    
    // free receiveing objects
    PacketProtoDecoder_Free(&pio->input_decoder);
    StreamRecvConnector_Free(&pio->input_connector);
}

PacketPassInterface * StreamPeerIO_GetSendInput (StreamPeerIO *pio)
{
    DebugObject_Access(&pio->d_obj);
    
    return PacketCopier_GetInput(&pio->output_user_copier);
}

int StreamPeerIO_Connect (StreamPeerIO *pio, BAddr addr, uint64_t password, CERTCertificate *ssl_cert, SECKEYPrivateKey *ssl_key)
{
    DebugObject_Access(&pio->d_obj);
    
    // reset state
    reset_state(pio);
    
    // check address
    if (!BConnection_AddressSupported(addr)) {
        PeerLog(pio, BLOG_ERROR, "BConnection_AddressSupported failed");
        goto fail0;
    }
    
    // init connector
    if (!BConnector_Init(&pio->connect.connector, addr, pio->reactor, pio, (BConnector_handler)connector_handler)) {
        PeerLog(pio, BLOG_ERROR, "BConnector_Init failed");
        goto fail0;
    }
    
    // remember data
    if (pio->ssl) {
        pio->connect.ssl_cert = ssl_cert;
        pio->connect.ssl_key = ssl_key;
    }
    pio->connect.password = htol64(password);
    
    // set state
    pio->mode = MODE_CONNECT;
    pio->connect.state = CONNECT_STATE_CONNECTING;
    
    return 1;
    
fail0:
    return 0;
}

void StreamPeerIO_Listen (StreamPeerIO *pio, PasswordListener *listener, uint64_t *password)
{
    DebugObject_Access(&pio->d_obj);
    ASSERT(listener->ssl == pio->ssl)
    
    // reset state
    reset_state(pio);
    
    // add PasswordListener entry
    uint64_t newpass = PasswordListener_AddEntry(listener, &pio->listen.pwentry, (PasswordListener_handler_client)listener_handler_client, pio);
    
    // remember data
    pio->listen.listener = listener;
    
    // set state
    pio->mode = MODE_LISTEN;
    pio->listen.state = LISTEN_STATE_LISTENER;
    
    *password = newpass;
}
