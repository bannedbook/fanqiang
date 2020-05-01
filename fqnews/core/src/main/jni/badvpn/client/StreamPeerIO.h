/**
 * @file StreamPeerIO.h
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
 * Object used for communicating with a peer over TCP.
 */

#ifndef BADVPN_CLIENT_STREAMPEERIO_H
#define BADVPN_CLIENT_STREAMPEERIO_H

#include <stdint.h>

#include <nss/cert.h>
#include <nss/keyhi.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <base/BLog.h>
#include <system/BReactor.h>
#include <system/BConnection.h>
#include <structure/LinkedList1.h>
#include <flow/PacketProtoDecoder.h>
#include <flow/PacketStreamSender.h>
#include <flow/SinglePacketBuffer.h>
#include <flow/PacketProtoEncoder.h>
#include <flow/PacketCopier.h>
#include <flow/PacketPassConnector.h>
#include <flow/StreamRecvConnector.h>
#include <flow/SingleStreamSender.h>
#include <client/PasswordListener.h>

/**
 * Callback function invoked when an error occurs with the peer connection.
 * The object has entered default state.
 * May be called from within a sending Send call.
 *
 * @param user value given to {@link StreamPeerIO_Init}.
 */
typedef void (*StreamPeerIO_handler_error) (void *user);

/**
 * Object used for communicating with a peer over TCP.
 * The object has a logical state which can be one of the following:
 *     - default state
 *     - listening state
 *     - connecting state
 */
typedef struct {
    // common arguments
    BReactor *reactor;
    BThreadWorkDispatcher *twd;
    int ssl;
    int ssl_flags;
    uint8_t *ssl_peer_cert;
    int ssl_peer_cert_len;
    int payload_mtu;
    int sock_sndbuf;
    BLog_logfunc logfunc;
    StreamPeerIO_handler_error handler_error;
    void *user;
    
    // persistent I/O modules
    
    // base sending objects
    PacketCopier output_user_copier;
    PacketProtoEncoder output_user_ppe;
    SinglePacketBuffer output_user_spb;
    PacketPassConnector output_connector;
    
    // receiving objects
    StreamRecvConnector input_connector;
    PacketProtoDecoder input_decoder;
    
    // connection side
    int mode;
    
    union {
        // listening data
        struct {
            int state;
            PasswordListener *listener;
            PasswordListener_pwentry pwentry;
            sslsocket *sock;
        } listen;
        // connecting data
        struct {
            int state;
            CERTCertificate *ssl_cert;
            SECKEYPrivateKey *ssl_key;
            BConnector connector;
            sslsocket sock;
            BSSLConnection sslcon;
            uint64_t password;
            SingleStreamSender pwsender;
        } connect;
    };
    
    // socket data
    sslsocket *sock;
    BSSLConnection sslcon;
    
    // sending objects
    PacketStreamSender output_pss;
    
    DebugObject d_obj;
} StreamPeerIO;

/**
 * Initializes the object.
 * The object is initialized in default state.
 * {@link BLog_Init} must have been done.
 * {@link BNetwork_GlobalInit} must have been done.
 * {@link BSSLConnection_GlobalInit} must have been done if using SSL.
 *
 * @param pio the object
 * @param reactor reactor we live in
 * @param twd thread work dispatcher. May be NULL if ssl_flags does not request performing SSL
 *            operations in threads.
 * @param ssl if nonzero, SSL will be used for peer connection
 * @param ssl_flags flags passed down to {@link BSSLConnection_MakeBackend}. May be used to
 *                  request performing SSL operations in threads.
 * @param ssl_peer_cert if using SSL, the certificate we expect the peer to have
 * @param ssl_peer_cert_len if using SSL, the length of the certificate
 * @param payload_mtu maximum packet size as seen from the user. Must be >=0.
 * @param sock_sndbuf socket SO_SNDBUF option. Specify <=0 to not set it.
 * @param user_recv_if interface to use for submitting received packets. Its MTU
 *                     must be >=payload_mtu.
 * @param logfunc function which prepends the log prefix using {@link BLog_Append}
 * @param handler_error handler function invoked when a connection error occurs
 * @param user value to pass to handler functions
 * @return 1 on success, 0 on failure
 */
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
) WARN_UNUSED;

/**
 * Frees the object.
 *
 * @param pio the object
 */
void StreamPeerIO_Free (StreamPeerIO *pio);

/**
 * Returns the interface for sending packets to the peer.
 * The OTP warning handler may be called from within Send calls
 * to the interface.
 *
 * @param pio the object
 * @return interface for sending packets to the peer
 */
PacketPassInterface * StreamPeerIO_GetSendInput (StreamPeerIO *pio);

/**
 * Starts an attempt to connect to the peer.
 * On success, the object enters connecting state.
 * On failure, the object enters default state.
 *
 * @param pio the object
 * @param addr address to connect to
 * @param password identification code to send to the peer
 * @param ssl_cert if using SSL, the client certificate to use. This object does not
 *                 take ownership of the certificate; it must remain valid until
 *                 the object is reset.
 * @param ssl_key if using SSL, the private key to use. This object does not take
 *                ownership of the key; it must remain valid until the object is reset.
 * @return 1 on success, 0 on failure
 */
int StreamPeerIO_Connect (StreamPeerIO *pio, BAddr addr, uint64_t password, CERTCertificate *ssl_cert, SECKEYPrivateKey *ssl_key) WARN_UNUSED;

/**
 * Starts an attempt to accept a connection from the peer.
 * The object enters listening state.
 *
 * @param pio the object
 * @param listener {@link PasswordListener} object to use for accepting a connection.
 *                 The listener must have SSL enabled if and only if this object has
 *                 SSL enabled. The listener must be available until the object is
 *                 reset or {@link StreamPeerIO_handler_up} is called.
 * @param password will return the identification code the peer should send when connecting
 */
void StreamPeerIO_Listen (StreamPeerIO *pio, PasswordListener *listener, uint64_t *password);

#endif
