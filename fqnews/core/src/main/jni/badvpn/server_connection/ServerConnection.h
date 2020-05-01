/**
 * @file ServerConnection.h
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
 * Object used to communicate with a VPN chat server.
 */

#ifndef BADVPN_SERVERCONNECTION_SERVERCONNECTION_H
#define BADVPN_SERVERCONNECTION_SERVERCONNECTION_H

#include <stdint.h>

#include <prinit.h>
#include <prio.h>
#include <prerror.h>
#include <prtypes.h>
#include <nss/nss.h>
#include <nss/ssl.h>
#include <nss/pk11func.h>
#include <nss/cert.h>
#include <nss/keyhi.h>

#include <misc/debug.h>
#include <misc/debugerror.h>
#include <protocol/scproto.h>
#include <protocol/msgproto.h>
#include <base/DebugObject.h>
#include <system/BConnection.h>
#include <flow/PacketProtoEncoder.h>
#include <flow/PacketStreamSender.h>
#include <flow/PacketProtoDecoder.h>
#include <flow/PacketPassPriorityQueue.h>
#include <flow/PacketProtoFlow.h>
#include <flowextra/KeepaliveIO.h>
#include <nspr_support/BSSLConnection.h>
#include <server_connection/SCKeepaliveSource.h>

/**
 * Handler function invoked when an error occurs.
 * The object must be freed from withing this function.
 *
 * @param user value passed to {@link ServerConnection_Init}
 */
typedef void (*ServerConnection_handler_error) (void *user);

/**
 * Handler function invoked when the server becomes ready, i.e.
 * the hello packet has been received.
 * The object was in not ready state before.
 * The object enters ready state before the handler is invoked.
 *
 * @param user value passed to {@link ServerConnection_Init}
 * @param my_id our ID as reported by the server
 * @param ext_ip the clientAddr field in the server's hello packet
 */
typedef void (*ServerConnection_handler_ready) (void *user, peerid_t my_id, uint32_t ext_ip);

/**
 * Handler function invoked when a newclient packet is received.
 * The object was in ready state.
 *
 * @param user value passed to {@link ServerConnection_Init}
 * @param peer_id ID of the peer
 * @param flags flags field from the newclient message
 * @param cert peer's certificate (if any)
 * @param cert_len certificate length. Will be >=0.
 */
typedef void (*ServerConnection_handler_newclient) (void *user, peerid_t peer_id, int flags, const uint8_t *cert, int cert_len);

/**
 * Handler function invoked when an enclient packet is received.
 * The object was in ready state.
 *
 * @param user value passed to {@link ServerConnection_Init}
 * @param peer_id ID of the peer
 */
typedef void (*ServerConnection_handler_endclient) (void *user, peerid_t peer_id);

/**
 * Handler function invoked when an inmsg packet is received.
 * The object was in ready state.
 *
 * @param user value passed to {@link ServerConnection_Init}
 * @param peer_id ID of the peer from which the message came
 * @param data message payload
 * @param data_len message length. Will be >=0.
 */
typedef void (*ServerConnection_handler_message) (void *user, peerid_t peer_id, uint8_t *data, int data_len);

/**
 * Object used to communicate with a VPN chat server.
 */
typedef struct {
    // global resources
    BReactor *reactor;
    BThreadWorkDispatcher *twd;
    
    // keepalive interval
    int keepalive_interval;
    
    // send buffer size
    int buffer_size;
    
    // whether we use SSL
    int have_ssl;
    
    // ssl flags
    int ssl_flags;
    
    // client certificate if using SSL
    CERTCertificate *client_cert;

    // client private key if using SSL
    SECKEYPrivateKey *client_key;
    
    // server name if using SSL
    char *server_name;
    
    // handlers
    void *user;
    ServerConnection_handler_error handler_error;
    ServerConnection_handler_ready handler_ready;
    ServerConnection_handler_newclient handler_newclient;
    ServerConnection_handler_endclient handler_endclient;
    ServerConnection_handler_message handler_message;
    
    // socket
    BConnector connector;
    BConnection con;
    
    // job to report new client after sending acceptpeer
    BPending newclient_job;
    uint8_t *newclient_data;
    int newclient_data_len;
    
    // state
    int state;
    int buffers_released;
    
    // whether an error is being reported
    int error;
    
    // defined when state > SERVERCONNECTION_STATE_CONNECTING
    
    // SSL file descriptor, defined only if using SSL
    PRFileDesc bottom_prfd;
    PRFileDesc *ssl_prfd;
    BSSLConnection sslcon;
    
    // input
    PacketProtoDecoder input_decoder;
    PacketPassInterface input_interface;
    
    // keepalive output branch
    SCKeepaliveSource output_ka_zero;
    PacketProtoEncoder output_ka_encoder;
    
    // output common
    PacketPassPriorityQueue output_queue;
    KeepaliveIO output_keepaliveio;
    PacketStreamSender output_sender;
    
    // output local flow
    int output_local_packet_len;
    uint8_t *output_local_packet;
    BufferWriter *output_local_if;
    PacketProtoFlow output_local_oflow;
    PacketPassPriorityQueueFlow output_local_qflow;
    
    // output user flow
    PacketPassPriorityQueueFlow output_user_qflow;
    
    // job to start client I/O
    BPending start_job;
    
    DebugError d_err;
    DebugObject d_obj;
} ServerConnection;

/**
 * Initializes the object.
 * The object is initialized in not ready state.
 * {@link BLog_Init} must have been done.
 * {@link BNetwork_GlobalInit} must have been done.
 * {@link BSSLConnection_GlobalInit} must have been done if using SSL.
 *
 * @param o the object
 * @param reactor {@link BReactor} we live in
 * @param twd thread work dispatcher. May be NULL if ssl_flags does not request performing SSL
 *            operations in threads.
 * @param addr address to connect to
 * @param keepalive_interval keep-alive sending interval. Must be >0.
 * @param buffer_size minimum size of send buffer in number of packets. Must be >0.
 * @param have_ssl whether to use SSL for connecting to the server. Must be 1 or 0.
 * @param ssl_flags flags passed down to {@link BSSLConnection_MakeBackend}. May be used to
 *                  request performing SSL operations in threads.
 * @param client_cert if using SSL, client certificate to use. Must remain valid as
 *                    long as this object is alive.
 * @param client_key if using SSL, prvate ket to use. Must remain valid as
 *                   long as this object is alive.
 * @param server_name if using SSL, the name of the server. The string is copied.
 * @param user value passed to callback functions
 * @param handler_error error handler. The object must be freed from within the error
 *                      handler before doing anything else with this object.
 * @param handler_ready handler when the server becomes ready, i.e. the hello message has
 *                      been received.
 * @param handler_newclient handler when a newclient message has been received
 * @param handler_endclient handler when an endclient message has been received
 * @param handler_message handler when a peer message has been reveived
 * @return 1 on success, 0 on failure
 */
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
) WARN_UNUSED;

/**
 * Frees the object.
 * {@link ServerConnection_ReleaseBuffers} must have been called if the
 * send interface obtained from {@link ServerConnection_GetSendInterface}
 * was used.
 *
 * @param o the object
 */
void ServerConnection_Free (ServerConnection *o);

/**
 * Stops using any buffers passed to the send interface obtained from
 * {@link ServerConnection_GetSendInterface}. If the send interface
 * has been used, this must be called at appropriate time before this
 * object is freed.
 */
void ServerConnection_ReleaseBuffers (ServerConnection *o);

/**
 * Returns an interface for sending data to the server (just one).
 * This goes directly into the link (i.e. TCP, possibly via SSL), so packets
 * need to be manually encoded according to PacketProto.
 * The interface must not be used after an error was reported.
 * The object must be in ready state.
 * Must not be called from the error handler.
 *
 * @param o the object
 * @return the interface
 */
PacketPassInterface * ServerConnection_GetSendInterface (ServerConnection *o);

#endif
