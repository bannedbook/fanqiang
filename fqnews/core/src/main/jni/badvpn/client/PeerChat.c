/**
 * @file PeerChat.c
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

#include <string.h>

#include <nss/ssl.h>
#include <nss/sslerr.h>

#include <misc/byteorder.h>
#include <security/BRandom.h>

#include "PeerChat.h"

#include <generated/blog_channel_PeerChat.h>

#define PeerLog(_o, ...) BLog_LogViaFunc((_o)->logfunc, (_o)->user, BLOG_CURRENT_CHANNEL, __VA_ARGS__)

static void report_error (PeerChat *o)
{
    DebugError_AssertNoError(&o->d_err);
    
    DEBUGERROR(&o->d_err, o->handler_error(o->user))
    return;
}

static void recv_job_handler (PeerChat *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->recv_data_len >= 0)
    ASSERT(o->recv_data_len <= SC_MAX_MSGLEN)
    
    int data_len = o->recv_data_len;
    
    // set no received data
    o->recv_data_len = -1;
    
#ifdef PEERCHAT_SIMULATE_ERROR
    uint8_t x;
    BRandom_randomize(&x, sizeof(x));
    if (x < PEERCHAT_SIMULATE_ERROR) {
        PeerLog(o, BLOG_ERROR, "simulate error");
        report_error(o);
        return;
    }
#endif
    
    if (o->ssl_mode != PEERCHAT_SSL_NONE) {
        // buffer data
        if (!SimpleStreamBuffer_Write(&o->ssl_recv_buf, o->recv_data, data_len)) {
            PeerLog(o, BLOG_ERROR, "out of recv buffer");
            report_error(o);
            return;
        }
    } else {
        // call message handler
        o->handler_message(o->user, o->recv_data, data_len);
        return;
    }
}

static void ssl_con_handler (PeerChat *o, int event)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->ssl_mode == PEERCHAT_SSL_CLIENT || o->ssl_mode == PEERCHAT_SSL_SERVER)
    ASSERT(event == BSSLCONNECTION_EVENT_ERROR)
    
    PeerLog(o, BLOG_ERROR, "SSL error");
    
    report_error(o);
    return;
}

static SECStatus client_auth_data_callback (PeerChat *o, PRFileDesc *fd, CERTDistNames *caNames, CERTCertificate **pRetCert, SECKEYPrivateKey **pRetKey)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->ssl_mode == PEERCHAT_SSL_CLIENT)
    
    CERTCertificate *cert = CERT_DupCertificate(o->ssl_cert);
    if (!cert) {
        PeerLog(o, BLOG_ERROR, "CERT_DupCertificate failed");
        goto fail0;
    }
    
    SECKEYPrivateKey *key = SECKEY_CopyPrivateKey(o->ssl_key);
    if (!key) {
        PeerLog(o, BLOG_ERROR, "SECKEY_CopyPrivateKey failed");
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

static SECStatus auth_certificate_callback (PeerChat *o, PRFileDesc *fd, PRBool checkSig, PRBool isServer)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->ssl_mode == PEERCHAT_SSL_CLIENT || o->ssl_mode == PEERCHAT_SSL_SERVER)
    
    // This callback is used to bypass checking the server's domain name, as peers
    // don't have domain names. We byte-compare the certificate to the one reported
    // by the server anyway.
    
    SECStatus ret = SECFailure;
    
    CERTCertificate *cert = SSL_PeerCertificate(o->ssl_prfd);
    if (!cert) {
        PeerLog(o, BLOG_ERROR, "SSL_PeerCertificate failed");
        PORT_SetError(SSL_ERROR_BAD_CERTIFICATE);
        goto fail1;
    }
    
    SECCertUsage cert_usage = (o->ssl_mode == PEERCHAT_SSL_CLIENT ? certUsageSSLServer : certUsageSSLClient);
    
    if (CERT_VerifyCertNow(CERT_GetDefaultCertDB(), cert, PR_TRUE, cert_usage, SSL_RevealPinArg(o->ssl_prfd)) != SECSuccess) {
        goto fail2;
    }
    
    // compare to certificate provided by the server
    SECItem der = cert->derCert;
    if (der.len != o->ssl_peer_cert_len || memcmp(der.data, o->ssl_peer_cert, der.len)) {
        PeerLog(o, BLOG_ERROR, "peer certificate doesn't match");
        PORT_SetError(SSL_ERROR_BAD_CERTIFICATE);
        goto fail2;
    }
    
    ret = SECSuccess;
    
fail2:
    CERT_DestroyCertificate(cert);
fail1:
    return ret;
}

static void ssl_recv_if_handler_send (PeerChat *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->ssl_mode == PEERCHAT_SSL_CLIENT || o->ssl_mode == PEERCHAT_SSL_SERVER)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= SC_MAX_MSGLEN)
    
    // accept packet
    PacketPassInterface_Done(&o->ssl_recv_if);
    
    // call message handler
    o->handler_message(o->user, data, data_len);
    return;
}

static void ssl_recv_decoder_handler_error (PeerChat *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->ssl_mode == PEERCHAT_SSL_CLIENT || o->ssl_mode == PEERCHAT_SSL_SERVER)
    
    PeerLog(o, BLOG_ERROR, "decoder error");
    
    report_error(o);
    return;
}

int PeerChat_Init (PeerChat *o, peerid_t peer_id, int ssl_mode, int ssl_flags, CERTCertificate *ssl_cert, SECKEYPrivateKey *ssl_key,
                   uint8_t *ssl_peer_cert, int ssl_peer_cert_len, BPendingGroup *pg, BThreadWorkDispatcher *twd, void *user,
                   BLog_logfunc logfunc,
                   PeerChat_handler_error handler_error,
                   PeerChat_handler_message handler_message)
{
    ASSERT(ssl_mode == PEERCHAT_SSL_NONE || ssl_mode == PEERCHAT_SSL_CLIENT || ssl_mode == PEERCHAT_SSL_SERVER)
    ASSERT(ssl_mode == PEERCHAT_SSL_NONE || ssl_peer_cert_len >= 0)
    ASSERT(logfunc)
    ASSERT(handler_error)
    ASSERT(handler_message)
    
    // init arguments
    o->ssl_mode = ssl_mode;
    o->ssl_cert = ssl_cert;
    o->ssl_key = ssl_key;
    o->ssl_peer_cert = ssl_peer_cert;
    o->ssl_peer_cert_len = ssl_peer_cert_len;
    o->user = user;
    o->logfunc = logfunc;
    o->handler_error = handler_error;
    o->handler_message = handler_message;
    
    // init copier
    PacketCopier_Init(&o->copier, SC_MAX_MSGLEN, pg);
    
    // init SC encoder
    SCOutmsgEncoder_Init(&o->sc_encoder, peer_id, PacketCopier_GetOutput(&o->copier), pg);
    
    // init PacketProto encoder
    PacketProtoEncoder_Init(&o->pp_encoder, SCOutmsgEncoder_GetOutput(&o->sc_encoder), pg);
    
    // init recv job
    BPending_Init(&o->recv_job, pg, (BPending_handler)recv_job_handler, o);
    
    // set no received data
    o->recv_data_len = -1;
    
    PacketPassInterface *send_buf_output = PacketCopier_GetInput(&o->copier);
    
    if (o->ssl_mode != PEERCHAT_SSL_NONE) {
        // init receive buffer
        if (!SimpleStreamBuffer_Init(&o->ssl_recv_buf, PEERCHAT_SSL_RECV_BUF_SIZE, pg)) {
            PeerLog(o, BLOG_ERROR, "SimpleStreamBuffer_Init failed");
            goto fail1;
        }
        
        // init SSL StreamPacketSender
        StreamPacketSender_Init(&o->ssl_sp_sender, send_buf_output, pg);
        
        // init SSL bottom prfd
        if (!BSSLConnection_MakeBackend(&o->ssl_bottom_prfd, StreamPacketSender_GetInput(&o->ssl_sp_sender), SimpleStreamBuffer_GetOutput(&o->ssl_recv_buf), twd, ssl_flags)) {
            PeerLog(o, BLOG_ERROR, "BSSLConnection_MakeBackend failed");
            goto fail2;
        }
        
        // init SSL prfd
        if (!(o->ssl_prfd = SSL_ImportFD(NULL, &o->ssl_bottom_prfd))) {
            ASSERT_FORCE(PR_Close(&o->ssl_bottom_prfd) == PR_SUCCESS)
            PeerLog(o, BLOG_ERROR, "SSL_ImportFD failed");
            goto fail2;
        }
        
        // set client or server mode
        if (SSL_ResetHandshake(o->ssl_prfd, (o->ssl_mode == PEERCHAT_SSL_SERVER ? PR_TRUE : PR_FALSE)) != SECSuccess) {
            PeerLog(o, BLOG_ERROR, "SSL_ResetHandshake failed");
            goto fail3;
        }
        
        if (o->ssl_mode == PEERCHAT_SSL_SERVER) {
            // set server certificate
            if (SSL_ConfigSecureServer(o->ssl_prfd, o->ssl_cert, o->ssl_key, NSS_FindCertKEAType(o->ssl_cert)) != SECSuccess) {
                PeerLog(o, BLOG_ERROR, "SSL_ConfigSecureServer failed");
                goto fail3;
            }
            
            // set require client certificate
            if (SSL_OptionSet(o->ssl_prfd, SSL_REQUEST_CERTIFICATE, PR_TRUE) != SECSuccess) {
                PeerLog(o, BLOG_ERROR, "SSL_OptionSet(SSL_REQUEST_CERTIFICATE) failed");
                goto fail3;
            }
            if (SSL_OptionSet(o->ssl_prfd, SSL_REQUIRE_CERTIFICATE, PR_TRUE) != SECSuccess) {
                PeerLog(o, BLOG_ERROR, "SSL_OptionSet(SSL_REQUIRE_CERTIFICATE) failed");
                goto fail3;
            }
        } else {
            // set client certificate callback
            if (SSL_GetClientAuthDataHook(o->ssl_prfd, (SSLGetClientAuthData)client_auth_data_callback, o) != SECSuccess) {
                PeerLog(o, BLOG_ERROR, "SSL_GetClientAuthDataHook failed");
                goto fail3;
            }
        }
        
        // set verify peer certificate hook
        if (SSL_AuthCertificateHook(o->ssl_prfd, (SSLAuthCertificate)auth_certificate_callback, o) != SECSuccess) {
            PeerLog(o, BLOG_ERROR, "SSL_AuthCertificateHook failed");
            goto fail3;
        }
        
        // init SSL connection
        BSSLConnection_Init(&o->ssl_con, o->ssl_prfd, 0, pg, o, (BSSLConnection_handler)ssl_con_handler);
        
        // init SSL PacketStreamSender
        PacketStreamSender_Init(&o->ssl_ps_sender, BSSLConnection_GetSendIf(&o->ssl_con), sizeof(struct packetproto_header) + SC_MAX_MSGLEN, pg);
        
        // init SSL copier
        PacketCopier_Init(&o->ssl_copier, SC_MAX_MSGLEN, pg);
        
        // init SSL encoder
        PacketProtoEncoder_Init(&o->ssl_encoder, PacketCopier_GetOutput(&o->ssl_copier), pg);
        
        // init SSL buffer
        if (!SinglePacketBuffer_Init(&o->ssl_buffer, PacketProtoEncoder_GetOutput(&o->ssl_encoder), PacketStreamSender_GetInput(&o->ssl_ps_sender), pg)) {
            PeerLog(o, BLOG_ERROR, "SinglePacketBuffer_Init failed");
            goto fail4;
        }
        
        // init receive interface
        PacketPassInterface_Init(&o->ssl_recv_if, SC_MAX_MSGLEN, (PacketPassInterface_handler_send)ssl_recv_if_handler_send, o, pg);
        
        // init receive decoder
        if (!PacketProtoDecoder_Init(&o->ssl_recv_decoder, BSSLConnection_GetRecvIf(&o->ssl_con), &o->ssl_recv_if, pg, o, (PacketProtoDecoder_handler_error)ssl_recv_decoder_handler_error)) {
            PeerLog(o, BLOG_ERROR, "PacketProtoDecoder_Init failed");
            goto fail5;
        }
        
        send_buf_output = PacketCopier_GetInput(&o->ssl_copier);
    }
    
    // init send writer
    BufferWriter_Init(&o->send_writer, SC_MAX_MSGLEN, pg);
    
    // init send buffer
    if (!PacketBuffer_Init(&o->send_buf, BufferWriter_GetOutput(&o->send_writer), send_buf_output, PEERCHAT_SEND_BUF_SIZE, pg)) {
        PeerLog(o, BLOG_ERROR, "PacketBuffer_Init failed");
        goto fail6;
    }
    
    DebugError_Init(&o->d_err, pg);
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail6:
    BufferWriter_Free(&o->send_writer);
    if (o->ssl_mode != PEERCHAT_SSL_NONE) {
        PacketProtoDecoder_Free(&o->ssl_recv_decoder);
fail5:
        PacketPassInterface_Free(&o->ssl_recv_if);
        SinglePacketBuffer_Free(&o->ssl_buffer);
fail4:
        PacketProtoEncoder_Free(&o->ssl_encoder);
        PacketCopier_Free(&o->ssl_copier);
        PacketStreamSender_Free(&o->ssl_ps_sender);
        BSSLConnection_Free(&o->ssl_con);
fail3:
        ASSERT_FORCE(PR_Close(o->ssl_prfd) == PR_SUCCESS)
fail2:
        StreamPacketSender_Free(&o->ssl_sp_sender);
        SimpleStreamBuffer_Free(&o->ssl_recv_buf);
    }
fail1:
    BPending_Free(&o->recv_job);
    PacketProtoEncoder_Free(&o->pp_encoder);
    SCOutmsgEncoder_Free(&o->sc_encoder);
    PacketCopier_Free(&o->copier);
    return 0;
}

void PeerChat_Free (PeerChat *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    
    // stop using any buffers before they get freed
    if (o->ssl_mode != PEERCHAT_SSL_NONE) {
        BSSLConnection_ReleaseBuffers(&o->ssl_con);
    }
    
    PacketBuffer_Free(&o->send_buf);
    BufferWriter_Free(&o->send_writer);
    if (o->ssl_mode != PEERCHAT_SSL_NONE) {
        PacketProtoDecoder_Free(&o->ssl_recv_decoder);
        PacketPassInterface_Free(&o->ssl_recv_if);
        SinglePacketBuffer_Free(&o->ssl_buffer);
        PacketProtoEncoder_Free(&o->ssl_encoder);
        PacketCopier_Free(&o->ssl_copier);
        PacketStreamSender_Free(&o->ssl_ps_sender);
        BSSLConnection_Free(&o->ssl_con);
        ASSERT_FORCE(PR_Close(o->ssl_prfd) == PR_SUCCESS)
        StreamPacketSender_Free(&o->ssl_sp_sender);
        SimpleStreamBuffer_Free(&o->ssl_recv_buf);
    }
    BPending_Free(&o->recv_job);
    PacketProtoEncoder_Free(&o->pp_encoder);
    SCOutmsgEncoder_Free(&o->sc_encoder);
    PacketCopier_Free(&o->copier);
}

PacketRecvInterface * PeerChat_GetSendOutput (PeerChat *o)
{
    DebugObject_Access(&o->d_obj);
    
    return PacketProtoEncoder_GetOutput(&o->pp_encoder);
}

void PeerChat_InputReceived (PeerChat *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->recv_data_len == -1)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= SC_MAX_MSGLEN)
    
    // remember data
    o->recv_data = data;
    o->recv_data_len = data_len;
    
    // set received job
    BPending_Set(&o->recv_job);
}

int PeerChat_StartMessage (PeerChat *o, uint8_t **data)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    
    return BufferWriter_StartPacket(&o->send_writer, data);
}

void PeerChat_EndMessage (PeerChat *o, int data_len)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(data_len >= 0)
    ASSERT(data_len <= SC_MAX_MSGLEN)
    
    BufferWriter_EndPacket(&o->send_writer, data_len);
}
