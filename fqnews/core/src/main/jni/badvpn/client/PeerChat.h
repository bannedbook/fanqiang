/**
 * @file PeerChat.h
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

#ifndef BADVPN_PEERCHAT_H
#define BADVPN_PEERCHAT_H

#include <nss/cert.h>
#include <nss/keyhi.h>

#include <protocol/packetproto.h>
#include <protocol/scproto.h>
#include <misc/debug.h>
#include <misc/debugerror.h>
#include <base/DebugObject.h>
#include <base/BPending.h>
#include <base/BLog.h>
#include <flow/SinglePacketSender.h>
#include <flow/PacketProtoEncoder.h>
#include <flow/PacketCopier.h>
#include <flow/StreamPacketSender.h>
#include <flow/PacketStreamSender.h>
#include <flow/SinglePacketBuffer.h>
#include <flow/PacketProtoDecoder.h>
#include <flow/PacketBuffer.h>
#include <flow/BufferWriter.h>
#include <nspr_support/BSSLConnection.h>
#include <client/SCOutmsgEncoder.h>
#include <client/SimpleStreamBuffer.h>

#define PEERCHAT_SSL_NONE 0
#define PEERCHAT_SSL_CLIENT 1
#define PEERCHAT_SSL_SERVER 2

#define PEERCHAT_SSL_RECV_BUF_SIZE 4096
#define PEERCHAT_SEND_BUF_SIZE 200

//#define PEERCHAT_SIMULATE_ERROR 40

typedef void (*PeerChat_handler_error) (void *user);
typedef void (*PeerChat_handler_message) (void *user, uint8_t *data, int data_len);

typedef struct {
    int ssl_mode;
    CERTCertificate *ssl_cert;
    SECKEYPrivateKey *ssl_key;
    uint8_t *ssl_peer_cert;
    int ssl_peer_cert_len;
    void *user;
    BLog_logfunc logfunc;
    PeerChat_handler_error handler_error;
    PeerChat_handler_message handler_message;
    
    // transport
    PacketProtoEncoder pp_encoder;
    SCOutmsgEncoder sc_encoder;
    PacketCopier copier;
    BPending recv_job;
    uint8_t *recv_data;
    int recv_data_len;
    
    // SSL transport
    StreamPacketSender ssl_sp_sender;
    SimpleStreamBuffer ssl_recv_buf;
    
    // SSL connection
    PRFileDesc ssl_bottom_prfd;
    PRFileDesc *ssl_prfd;
    BSSLConnection ssl_con;
    
    // SSL higher layer
    PacketStreamSender ssl_ps_sender;
    SinglePacketBuffer ssl_buffer;
    PacketProtoEncoder ssl_encoder;
    PacketCopier ssl_copier;
    PacketProtoDecoder ssl_recv_decoder;
    PacketPassInterface ssl_recv_if;
    
    // higher layer send buffer
    PacketBuffer send_buf;
    BufferWriter send_writer;
    
    DebugError d_err;
    DebugObject d_obj;
} PeerChat;

int PeerChat_Init (PeerChat *o, peerid_t peer_id, int ssl_mode, int ssl_flags, CERTCertificate *ssl_cert, SECKEYPrivateKey *ssl_key,
                   uint8_t *ssl_peer_cert, int ssl_peer_cert_len, BPendingGroup *pg, BThreadWorkDispatcher *twd, void *user,
                   BLog_logfunc logfunc,
                   PeerChat_handler_error handler_error,
                   PeerChat_handler_message handler_message) WARN_UNUSED;
void PeerChat_Free (PeerChat *o);
PacketRecvInterface * PeerChat_GetSendOutput (PeerChat *o);
void PeerChat_InputReceived (PeerChat *o, uint8_t *data, int data_len);
int PeerChat_StartMessage (PeerChat *o, uint8_t **data) WARN_UNUSED;
void PeerChat_EndMessage (PeerChat *o, int data_len);

#endif
