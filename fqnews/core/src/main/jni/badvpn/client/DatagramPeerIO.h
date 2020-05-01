/**
 * @file DatagramPeerIO.h
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
 * Object for comminicating with a peer using a datagram socket.
 */

#ifndef BADVPN_CLIENT_DATAGRAMPEERIO_H
#define BADVPN_CLIENT_DATAGRAMPEERIO_H

#include <stdint.h>

#include <misc/debug.h>
#include <protocol/spproto.h>
#include <protocol/fragmentproto.h>
#include <base/DebugObject.h>
#include <base/BLog.h>
#include <system/BReactor.h>
#include <system/BAddr.h>
#include <system/BDatagram.h>
#include <system/BTime.h>
#include <flow/PacketPassInterface.h>
#include <flow/PacketPassConnector.h>
#include <flow/SinglePacketBuffer.h>
#include <flow/PacketRecvConnector.h>
#include <flow/PacketPassNotifier.h>
#include <client/FragmentProtoDisassembler.h>
#include <client/FragmentProtoAssembler.h>
#include <client/SPProtoEncoder.h>
#include <client/SPProtoDecoder.h>

/**
 * Callback function invoked when an error occurs with the peer connection.
 * The object has entered default state.
 * May be called from within a sending Send call.
 *
 * @param user as in {@link DatagramPeerIO_SetHandlers}
 */
typedef void (*DatagramPeerIO_handler_error) (void *user);

/**
 * Handler function invoked when the number of used OTPs has reached
 * the specified warning number in {@link DatagramPeerIO_SetOTPWarningHandler}.
 * May be called from within a sending Send call.
 *
 * @param user as in {@link DatagramPeerIO_SetHandlers}
 */
typedef void (*DatagramPeerIO_handler_otp_warning) (void *user);

/**
 * Handler called when OTP generation for a new receive seed is finished.
 * 
 * @param user as in {@link DatagramPeerIO_SetHandlers}
 */
typedef void (*DatagramPeerIO_handler_otp_ready) (void *user);

/**
 * Object for comminicating with a peer using a datagram socket.
 *
 * The user provides data for sending to the peer through {@link PacketPassInterface}.
 * Received data is provided to the user through {@link PacketPassInterface}.
 *
 * The object has a logical state called a mode, which is one of the following:
 *     - default - nothing is send or received
 *     - connecting - an address was provided by the user for sending datagrams to.
 *                    Datagrams are being sent to that address through a socket,
 *                    and datagrams are being received on the same socket.
 *     - binding - an address was provided by the user to bind a socket to.
 *                 Datagrams are being received on the socket. Datagrams are not being
 *                 sent initially. When a datagram is received, its source address is
 *                 used as a destination address for sending datagrams.
 */
typedef struct {
    DebugObject d_obj;
    BReactor *reactor;
    int payload_mtu;
    struct spproto_security_params sp_params;
    void *user;
    BLog_logfunc logfunc;
    DatagramPeerIO_handler_error handler_error;
    int spproto_payload_mtu;
    int effective_socket_mtu;
    
    // sending base
    FragmentProtoDisassembler send_disassembler;
    SPProtoEncoder send_encoder;
    SinglePacketBuffer send_buffer;
    PacketPassConnector send_connector;
    
    // receiving
    PacketRecvConnector recv_connector;
    SinglePacketBuffer recv_buffer;
    SPProtoDecoder recv_decoder;
    PacketPassNotifier recv_notifier;
    FragmentProtoAssembler recv_assembler;
    
    // mode
    int mode;
    
    // datagram object
    BDatagram dgram;
} DatagramPeerIO;

/**
 * Initializes the object.
 * The interface is initialized in default mode.
 * {@link BLog_Init} must have been done.
 * {@link BNetwork_GlobalInit} must have been done.
 * {@link BSecurity_GlobalInitThreadSafe} must have been done if
 * {@link BThreadWorkDispatcher_UsingThreads}(twd) = 1.
 *
 * @param o the object
 * @param reactor {@link BReactor} we live in
 * @param payload_mtu maximum payload size. Must be >=0.
 * @param socket_mtu maximum datagram size for the socket. Must be >=0. Must be large enough so it is possible to
 *                   send a FragmentProto chunk with one byte of data over SPProto, i.e. the following has to hold:
 *                   spproto_payload_mtu_for_carrier_mtu(sp_params, socket_mtu) > sizeof(struct fragmentproto_chunk_header)
 * @param sp_params SPProto security parameters
 * @param latency latency parameter to {@link FragmentProtoDisassembler_Init}.
 * @param num_frames num_frames parameter to {@link FragmentProtoAssembler_Init}. Must be >0.
 * @param recv_userif interface to pass received packets to the user. Its MTU must be >=payload_mtu.
 * @param otp_warning_count If using OTPs, after how many encoded packets to call the handler.
 *                          In this case, must be >0 and <=sp_params.otp_num.
 * @param twd thread work dispatcher
 * @param user value to pass to handlers
 * @param logfunc function which prepends the log prefix using {@link BLog_Append}
 * @param handler_error error handler
 * @param handler_otp_warning OTP warning handler
 * @param handler_otp_ready handler called when OTP generation for a new receive seed is finished
 * @return 1 on success, 0 on failure
 */
int DatagramPeerIO_Init (
    DatagramPeerIO *o,
    BReactor *reactor,
    int payload_mtu,
    int socket_mtu,
    struct spproto_security_params sp_params,
    btime_t latency,
    int num_frames,
    PacketPassInterface *recv_userif,
    int otp_warning_count,
    BThreadWorkDispatcher *twd,
    void *user,
    BLog_logfunc logfunc,
    DatagramPeerIO_handler_error handler_error,
    DatagramPeerIO_handler_otp_warning handler_otp_warning,
    DatagramPeerIO_handler_otp_ready handler_otp_ready
) WARN_UNUSED;

/**
 * Frees the object.
 *
 * @param o the object
 */
void DatagramPeerIO_Free (DatagramPeerIO *o);

/**
 * Returns an interface the user should use to send packets.
 * The OTP warning handler may be called from within Send calls
 * to the interface.
 *
 * @param o the object
 * @return sending interface
 */
PacketPassInterface * DatagramPeerIO_GetSendInput (DatagramPeerIO *o);

/**
 * Attempts to establish connection to the peer which has bound to an address.
 * On success, the interface enters connecting mode.
 * On failure, the interface enters default mode.
 *
 * @param o the object
 * @param addr address to send packets to
 * @return 1 on success, 0 on failure
 */
int DatagramPeerIO_Connect (DatagramPeerIO *o, BAddr addr) WARN_UNUSED;

/**
 * Attempts to establish connection to the peer by binding to an address.
 * On success, the interface enters connecting mode.
 * On failure, the interface enters default mode.
 *
 * @param o the object
 * @param addr address to bind to. Must be supported according to
 *             {@link BDatagram_AddressFamilySupported}.
 * @return 1 on success, 0 on failure
 */
int DatagramPeerIO_Bind (DatagramPeerIO *o, BAddr addr) WARN_UNUSED;

/**
 * Sets the encryption key to use for sending and receiving.
 * Encryption must be enabled.
 *
 * @param o the object
 * @param encryption_key key to use
 */
void DatagramPeerIO_SetEncryptionKey (DatagramPeerIO *o, uint8_t *encryption_key);

/**
 * Removed the encryption key to use for sending and receiving.
 * Encryption must be enabled.
 *
 * @param o the object
 */
void DatagramPeerIO_RemoveEncryptionKey (DatagramPeerIO *o);

/**
 * Sets the OTP seed for sending.
 * OTPs must be enabled.
 *
 * @param o the object
 * @param seed_id seed identifier
 * @param key OTP encryption key
 * @param iv OTP initialization vector
 */
void DatagramPeerIO_SetOTPSendSeed (DatagramPeerIO *o, uint16_t seed_id, uint8_t *key, uint8_t *iv);

/**
 * Removes the OTP seed for sending of one is configured.
 * OTPs must be enabled.
 *
 * @param o the object
 */
void DatagramPeerIO_RemoveOTPSendSeed (DatagramPeerIO *o);

/**
 * Adds an OTP seed for reciving.
 * OTPs must be enabled.
 *
 * @param o the object
 * @param seed_id seed identifier
 * @param key OTP encryption key
 * @param iv OTP initialization vector
 */
void DatagramPeerIO_AddOTPRecvSeed (DatagramPeerIO *o, uint16_t seed_id, uint8_t *key, uint8_t *iv);

/**
 * Removes all OTP seeds for reciving.
 * OTPs must be enabled.
 *
 * @param o the object
 */
void DatagramPeerIO_RemoveOTPRecvSeeds (DatagramPeerIO *o);

#endif
