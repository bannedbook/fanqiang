/**
 * @file SPProtoEncoder.h
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
 * Object which encodes packets according to SPProto.
 */

#ifndef BADVPN_CLIENT_SPPROTOENCODER_H
#define BADVPN_CLIENT_SPPROTOENCODER_H

#include <stdint.h>

#include <misc/debug.h>
#include <protocol/spproto.h>
#include <base/DebugObject.h>
#include <security/BEncryption.h>
#include <security/OTPGenerator.h>
#include <flow/PacketRecvInterface.h>
#include <threadwork/BThreadWork.h>

/**
 * Event context handler called when the remaining number of
 * OTPs equals the warning number after having encoded a packet.
 * 
 * @param user as in {@link SPProtoEncoder_Init}
 */
typedef void (*SPProtoEncoder_handler) (void *user);

/**
 * Object which encodes packets according to SPProto.
 *
 * Input is with {@link PacketRecvInterface}.
 * Output is with {@link PacketRecvInterface}.
 */
typedef struct {
    PacketRecvInterface *input;
    struct spproto_security_params sp_params;
    int otp_warning_count;
    SPProtoEncoder_handler handler;
    BThreadWorkDispatcher *twd;
    void *user;
    int hash_size;
    int enc_block_size;
    int enc_key_size;
    OTPGenerator otpgen;
    uint16_t otpgen_seed_id;
    uint16_t otpgen_pending_seed_id;
    int have_encryption_key;
    BEncryption encryptor;
    int input_mtu;
    int output_mtu;
    int in_len;
    PacketRecvInterface output;
    int out_have;
    uint8_t *out;
    uint8_t *buf;
    BPending handler_job;
    int tw_have;
    BThreadWork tw;
    uint16_t tw_seed_id;
    otp_t tw_otp;
    int tw_out_len;
    DebugObject d_obj;
} SPProtoEncoder;

/**
 * Initializes the object.
 * The object is initialized in blocked state.
 * {@link BSecurity_GlobalInitThreadSafe} must have been done if
 * {@link BThreadWorkDispatcher_UsingThreads}(twd) = 1.
 *
 * @param o the object
 * @param input input interface. Its MTU must not be too large, i.e. this must hold:
 *              spproto_carrier_mtu_for_payload_mtu(sp_params, input MTU) >= 0
 * @param sp_params SPProto security parameters
 * @param otp_warning_count If using OTPs, after how many encoded packets to call the handler.
 *                          In this case, must be >0 and <=sp_params.otp_num.
 * @param pg pending group
 * @param twd thread work dispatcher
 * @return 1 on success, 0 on failure
 */
int SPProtoEncoder_Init (SPProtoEncoder *o, PacketRecvInterface *input, struct spproto_security_params sp_params, int otp_warning_count, BPendingGroup *pg, BThreadWorkDispatcher *twd) WARN_UNUSED;

/**
 * Frees the object.
 *
 * @param o the object
 */
void SPProtoEncoder_Free (SPProtoEncoder *o);

/**
 * Returns the output interface.
 * The MTU of the output interface will depend on the input MTU and security parameters,
 * that is spproto_carrier_mtu_for_payload_mtu(sp_params, input MTU).
 *
 * @param o the object
 * @return output interface
 */
PacketRecvInterface * SPProtoEncoder_GetOutput (SPProtoEncoder *o);

/**
 * Sets an encryption key to use.
 * Encryption must be enabled.
 *
 * @param o the object
 * @param encryption_key key to use
 */
void SPProtoEncoder_SetEncryptionKey (SPProtoEncoder *o, uint8_t *encryption_key);

/**
 * Removes an encryption key if one is configured.
 * Encryption must be enabled.
 *
 * @param o the object
 */
void SPProtoEncoder_RemoveEncryptionKey (SPProtoEncoder *o);

/**
 * Sets an OTP seed to use.
 * OTPs must be enabled.
 *
 * @param o the object
 * @param seed_id seed identifier
 * @param key OTP encryption key
 * @param iv OTP initialization vector
 */
void SPProtoEncoder_SetOTPSeed (SPProtoEncoder *o, uint16_t seed_id, uint8_t *key, uint8_t *iv);

/**
 * Removes the OTP seed if one is configured.
 * OTPs must be enabled.
 *
 * @param o the object
 */
void SPProtoEncoder_RemoveOTPSeed (SPProtoEncoder *o);

/**
 * Sets handlers.
 *
 * @param o the object
 * @param handler OTP warning handler
 * @param user value to pass to handler
 */
void SPProtoEncoder_SetHandlers (SPProtoEncoder *o, SPProtoEncoder_handler handler, void *user);

#endif
