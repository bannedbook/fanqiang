/**
 * @file DatagramPeerIO.c
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

#include <client/DatagramPeerIO.h>

#include <generated/blog_channel_DatagramPeerIO.h>

#define DATAGRAMPEERIO_MODE_NONE 0
#define DATAGRAMPEERIO_MODE_CONNECT 1
#define DATAGRAMPEERIO_MODE_BIND 2

#define PeerLog(_o, ...) BLog_LogViaFunc((_o)->logfunc, (_o)->user, BLOG_CURRENT_CHANNEL, __VA_ARGS__)

static void init_io (DatagramPeerIO *o);
static void free_io (DatagramPeerIO *o);
static void dgram_handler (DatagramPeerIO *o, int event);
static void reset_mode (DatagramPeerIO *o);
static void recv_decoder_notifier_handler (DatagramPeerIO *o, uint8_t *data, int data_len);

void init_io (DatagramPeerIO *o)
{
    // init dgram recv interface
    BDatagram_RecvAsync_Init(&o->dgram, o->effective_socket_mtu);
    
    // connect source
    PacketRecvConnector_ConnectInput(&o->recv_connector, BDatagram_RecvAsync_GetIf(&o->dgram));
    
    // init dgram send interface
    BDatagram_SendAsync_Init(&o->dgram, o->effective_socket_mtu);
    
    // connect sink
    PacketPassConnector_ConnectOutput(&o->send_connector, BDatagram_SendAsync_GetIf(&o->dgram));
}

void free_io (DatagramPeerIO *o)
{
    // disconnect sink
    PacketPassConnector_DisconnectOutput(&o->send_connector);
    
    // free dgram send interface
    BDatagram_SendAsync_Free(&o->dgram);
    
    // disconnect source
    PacketRecvConnector_DisconnectInput(&o->recv_connector);
    
    // free dgram recv interface
    BDatagram_RecvAsync_Free(&o->dgram);
}

void dgram_handler (DatagramPeerIO *o, int event)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->mode == DATAGRAMPEERIO_MODE_CONNECT || o->mode == DATAGRAMPEERIO_MODE_BIND)
    
    PeerLog(o, BLOG_NOTICE, "error");
    
    // reset mode
    reset_mode(o);
    
    // report error
    if (o->handler_error) {
        o->handler_error(o->user);
        return;
    }
}

void reset_mode (DatagramPeerIO *o)
{
    ASSERT(o->mode == DATAGRAMPEERIO_MODE_NONE || o->mode == DATAGRAMPEERIO_MODE_CONNECT || o->mode == DATAGRAMPEERIO_MODE_BIND)
    
    if (o->mode == DATAGRAMPEERIO_MODE_NONE) {
        return;
    }
    
    // remove recv notifier handler
    PacketPassNotifier_SetHandler(&o->recv_notifier, NULL, NULL);
    
    // free I/O
    free_io(o);
    
    // free datagram object
    BDatagram_Free(&o->dgram);
    
    // set mode
    o->mode = DATAGRAMPEERIO_MODE_NONE;
}

void recv_decoder_notifier_handler (DatagramPeerIO *o, uint8_t *data, int data_len)
{
    ASSERT(o->mode == DATAGRAMPEERIO_MODE_BIND)
    DebugObject_Access(&o->d_obj);
    
    // obtain addresses from last received packet
    BAddr addr;
    BIPAddr local_addr;
    ASSERT_EXECUTE(BDatagram_GetLastReceiveAddrs(&o->dgram, &addr, &local_addr))
    
    // check address family just in case
    if (!BDatagram_AddressFamilySupported(addr.type)) {
        PeerLog(o, BLOG_ERROR, "unsupported receive address");
        return;
    }
    
    // update addresses
    BDatagram_SetSendAddrs(&o->dgram, addr, local_addr);
}

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
)
{
    ASSERT(payload_mtu >= 0)
    ASSERT(socket_mtu >= 0)
    spproto_assert_security_params(sp_params);
    ASSERT(num_frames > 0)
    ASSERT(PacketPassInterface_GetMTU(recv_userif) >= payload_mtu)
    if (SPPROTO_HAVE_OTP(sp_params)) {
        ASSERT(otp_warning_count > 0)
        ASSERT(otp_warning_count <= sp_params.otp_num)
    }
    
    // set parameters
    o->reactor = reactor;
    o->payload_mtu = payload_mtu;
    o->sp_params = sp_params;
    o->user = user;
    o->logfunc = logfunc;
    o->handler_error = handler_error;
    
    // check num frames (for FragmentProtoAssembler)
    if (num_frames >= FPA_MAX_TIME) {
        PeerLog(o, BLOG_ERROR, "num_frames is too big");
        goto fail0;
    }
    
    // check payload MTU (for FragmentProto)
    if (o->payload_mtu > UINT16_MAX) {
        PeerLog(o, BLOG_ERROR, "payload MTU is too big");
        goto fail0;
    }
    
    // calculate SPProto payload MTU
    if ((o->spproto_payload_mtu = spproto_payload_mtu_for_carrier_mtu(o->sp_params, socket_mtu)) <= (int)sizeof(struct fragmentproto_chunk_header)) {
        PeerLog(o, BLOG_ERROR, "socket MTU is too small");
        goto fail0;
    }
    
    // calculate effective socket MTU
    if ((o->effective_socket_mtu = spproto_carrier_mtu_for_payload_mtu(o->sp_params, o->spproto_payload_mtu)) < 0) {
        PeerLog(o, BLOG_ERROR, "spproto_carrier_mtu_for_payload_mtu failed !?");
        goto fail0;
    }
    
    // init receiving
    
    // init assembler
    if (!FragmentProtoAssembler_Init(&o->recv_assembler, o->spproto_payload_mtu, recv_userif, num_frames, fragmentproto_max_chunks_for_frame(o->spproto_payload_mtu, o->payload_mtu),
                                     BReactor_PendingGroup(o->reactor), o->user, o->logfunc
    )) {
        PeerLog(o, BLOG_ERROR, "FragmentProtoAssembler_Init failed");
        goto fail0;
    }
    
    // init notifier
    PacketPassNotifier_Init(&o->recv_notifier, FragmentProtoAssembler_GetInput(&o->recv_assembler), BReactor_PendingGroup(o->reactor));
    
    // init decoder
    if (!SPProtoDecoder_Init(&o->recv_decoder, PacketPassNotifier_GetInput(&o->recv_notifier), o->sp_params, 2, BReactor_PendingGroup(o->reactor), twd, o->user, o->logfunc)) {
        PeerLog(o, BLOG_ERROR, "SPProtoDecoder_Init failed");
        goto fail1;
    }
    SPProtoDecoder_SetHandlers(&o->recv_decoder, handler_otp_ready, user);
    
    // init connector
    PacketRecvConnector_Init(&o->recv_connector, o->effective_socket_mtu, BReactor_PendingGroup(o->reactor));
    
    // init buffer
    if (!SinglePacketBuffer_Init(&o->recv_buffer, PacketRecvConnector_GetOutput(&o->recv_connector), SPProtoDecoder_GetInput(&o->recv_decoder), BReactor_PendingGroup(o->reactor))) {
        PeerLog(o, BLOG_ERROR, "SinglePacketBuffer_Init failed");
        goto fail2;
    }
    
    // init sending base
    
    // init disassembler
    FragmentProtoDisassembler_Init(&o->send_disassembler, o->reactor, o->payload_mtu, o->spproto_payload_mtu, -1, latency);
    
    // init encoder
    if (!SPProtoEncoder_Init(&o->send_encoder, FragmentProtoDisassembler_GetOutput(&o->send_disassembler), o->sp_params, otp_warning_count, BReactor_PendingGroup(o->reactor), twd)) {
        PeerLog(o, BLOG_ERROR, "SPProtoEncoder_Init failed");
        goto fail3;
    }
    SPProtoEncoder_SetHandlers(&o->send_encoder, handler_otp_warning, user);
    
    // init connector
    PacketPassConnector_Init(&o->send_connector, o->effective_socket_mtu, BReactor_PendingGroup(o->reactor));
    
    // init buffer
    if (!SinglePacketBuffer_Init(&o->send_buffer, SPProtoEncoder_GetOutput(&o->send_encoder), PacketPassConnector_GetInput(&o->send_connector), BReactor_PendingGroup(o->reactor))) {
        PeerLog(o, BLOG_ERROR, "SinglePacketBuffer_Init failed");
        goto fail4;
    }
    
    // set mode
    o->mode = DATAGRAMPEERIO_MODE_NONE;
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail4:
    PacketPassConnector_Free(&o->send_connector);
    SPProtoEncoder_Free(&o->send_encoder);
fail3:
    FragmentProtoDisassembler_Free(&o->send_disassembler);
    SinglePacketBuffer_Free(&o->recv_buffer);
fail2:
    PacketRecvConnector_Free(&o->recv_connector);
    SPProtoDecoder_Free(&o->recv_decoder);
fail1:
    PacketPassNotifier_Free(&o->recv_notifier);
    FragmentProtoAssembler_Free(&o->recv_assembler);
fail0:
    return 0;
}

void DatagramPeerIO_Free (DatagramPeerIO *o)
{
    DebugObject_Free(&o->d_obj);

    // reset mode
    reset_mode(o);
    
    // free sending base
    SinglePacketBuffer_Free(&o->send_buffer);
    PacketPassConnector_Free(&o->send_connector);
    SPProtoEncoder_Free(&o->send_encoder);
    FragmentProtoDisassembler_Free(&o->send_disassembler);
    
    // free receiving
    SinglePacketBuffer_Free(&o->recv_buffer);
    PacketRecvConnector_Free(&o->recv_connector);
    SPProtoDecoder_Free(&o->recv_decoder);
    PacketPassNotifier_Free(&o->recv_notifier);
    FragmentProtoAssembler_Free(&o->recv_assembler);
}

PacketPassInterface * DatagramPeerIO_GetSendInput (DatagramPeerIO *o)
{
    DebugObject_Access(&o->d_obj);
    
    return FragmentProtoDisassembler_GetInput(&o->send_disassembler);
}

int DatagramPeerIO_Connect (DatagramPeerIO *o, BAddr addr)
{
    DebugObject_Access(&o->d_obj);
    
    // reset mode
    reset_mode(o);
    
    // check address
    if (!BDatagram_AddressFamilySupported(addr.type)) {
        PeerLog(o, BLOG_ERROR, "BDatagram_AddressFamilySupported failed");
        goto fail0;
    }
    
    // init dgram
    if (!BDatagram_Init(&o->dgram, addr.type, o->reactor, o, (BDatagram_handler)dgram_handler)) {
        PeerLog(o, BLOG_ERROR, "BDatagram_Init failed");
        goto fail0;
    }
    
    // set send address
    BIPAddr local_addr;
    BIPAddr_InitInvalid(&local_addr);
    BDatagram_SetSendAddrs(&o->dgram, addr, local_addr);
    
    // init I/O
    init_io(o);
    
    // set mode
    o->mode = DATAGRAMPEERIO_MODE_CONNECT;
    
    return 1;
    
fail0:
    return 0;
}

int DatagramPeerIO_Bind (DatagramPeerIO *o, BAddr addr)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(BDatagram_AddressFamilySupported(addr.type))
    
    // reset mode
    reset_mode(o);
    
    // init dgram
    if (!BDatagram_Init(&o->dgram, addr.type, o->reactor, o, (BDatagram_handler)dgram_handler)) {
        PeerLog(o, BLOG_ERROR, "BDatagram_Init failed");
        goto fail0;
    }
    
    // bind dgram
    if (!BDatagram_Bind(&o->dgram, addr)) {
        PeerLog(o, BLOG_INFO, "BDatagram_Bind failed");
        goto fail1;
    }
    
    // init I/O
    init_io(o);
    
    // set recv notifier handler
    PacketPassNotifier_SetHandler(&o->recv_notifier, (PacketPassNotifier_handler_notify)recv_decoder_notifier_handler, o);
    
    // set mode
    o->mode = DATAGRAMPEERIO_MODE_BIND;
    
    return 1;
    
fail1:
    BDatagram_Free(&o->dgram);
fail0:
    return 0;
}

void DatagramPeerIO_SetEncryptionKey (DatagramPeerIO *o, uint8_t *encryption_key)
{
    ASSERT(SPPROTO_HAVE_ENCRYPTION(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // set sending key
    SPProtoEncoder_SetEncryptionKey(&o->send_encoder, encryption_key);
    
    // set receiving key
    SPProtoDecoder_SetEncryptionKey(&o->recv_decoder, encryption_key);
}

void DatagramPeerIO_RemoveEncryptionKey (DatagramPeerIO *o)
{
    ASSERT(SPPROTO_HAVE_ENCRYPTION(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // remove sending key
    SPProtoEncoder_RemoveEncryptionKey(&o->send_encoder);
    
    // remove receiving key
    SPProtoDecoder_RemoveEncryptionKey(&o->recv_decoder);
}

void DatagramPeerIO_SetOTPSendSeed (DatagramPeerIO *o, uint16_t seed_id, uint8_t *key, uint8_t *iv)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // set sending seed
    SPProtoEncoder_SetOTPSeed(&o->send_encoder, seed_id, key, iv);
}

void DatagramPeerIO_RemoveOTPSendSeed (DatagramPeerIO *o)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // remove sending seed
    SPProtoEncoder_RemoveOTPSeed(&o->send_encoder);
}

void DatagramPeerIO_AddOTPRecvSeed (DatagramPeerIO *o, uint16_t seed_id, uint8_t *key, uint8_t *iv)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // add receiving seed
    SPProtoDecoder_AddOTPSeed(&o->recv_decoder, seed_id, key, iv);
}

void DatagramPeerIO_RemoveOTPRecvSeeds (DatagramPeerIO *o)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // remove receiving seeds
    SPProtoDecoder_RemoveOTPSeeds(&o->recv_decoder);
}
