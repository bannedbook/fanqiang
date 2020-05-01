/**
 * @file SPProtoDecoder.h
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
 * Object which decodes packets according to SPProto.
 */

#ifndef BADVPN_CLIENT_SPPROTODECODER_H
#define BADVPN_CLIENT_SPPROTODECODER_H

#include <stdint.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <base/BLog.h>
#include <protocol/spproto.h>
#include <security/BEncryption.h>
#include <security/OTPChecker.h>
#include <flow/PacketPassInterface.h>

/**
 * Handler called when OTP generation for a new seed is finished.
 * 
 * @param user as in {@link SPProtoDecoder_Init}
 */
typedef void (*SPProtoDecoder_otp_handler) (void *user);

/**
 * Object which decodes packets according to SPProto.
 * Input is with {@link PacketPassInterface}.
 * Output is with {@link PacketPassInterface}.
 */
typedef struct {
    PacketPassInterface *output;
    struct spproto_security_params sp_params;
    BThreadWorkDispatcher *twd;
    void *user;
    BLog_logfunc logfunc;
    int output_mtu;
    int hash_size;
    int enc_block_size;
    int enc_key_size;
    int input_mtu;
    uint8_t *buf;
    PacketPassInterface input;
    OTPChecker otpchecker;
    int have_encryption_key;
    BEncryption encryptor;
    uint8_t *in;
    int in_len;
    int tw_have;
    BThreadWork tw;
    uint16_t tw_out_seed_id;
    otp_t tw_out_otp;
    uint8_t *tw_out;
    int tw_out_len;
    DebugObject d_obj;
} SPProtoDecoder;

/**
 * Initializes the object.
 * {@link BSecurity_GlobalInitThreadSafe} must have been done if
 * {@link BThreadWorkDispatcher_UsingThreads}(twd) = 1.
 *
 * @param o the object
 * @param output output interface. Its MTU must not be too large, i.e. this must hold:
 *               spproto_carrier_mtu_for_payload_mtu(sp_params, output MTU) >= 0
 * @param sp_params SPProto parameters
 * @param encryption_key if using encryption, the encryption key
 * @param num_otp_seeds if using OTPs, how many OTP seeds to keep for checking
 *                      receiving packets. Must be >=2 if using OTPs.
 * @param pg pending group
 * @param twd thread work dispatcher
 * @param user argument to handlers
 * @param logfunc function which prepends the log prefix using {@link BLog_Append}
 * @return 1 on success, 0 on failure
 */
int SPProtoDecoder_Init (SPProtoDecoder *o, PacketPassInterface *output, struct spproto_security_params sp_params, int num_otp_seeds, BPendingGroup *pg, BThreadWorkDispatcher *twd, void *user, BLog_logfunc logfunc) WARN_UNUSED;

/**
 * Frees the object.
 *
 * @param o the object
 */
void SPProtoDecoder_Free (SPProtoDecoder *o);

/**
 * Returns the input interface.
 * The MTU of the input interface will depend on the output MTU and security parameters,
 * that is spproto_carrier_mtu_for_payload_mtu(sp_params, output MTU).
 *
 * @param o the object
 * @return input interface
 */
PacketPassInterface * SPProtoDecoder_GetInput (SPProtoDecoder *o);

/**
 * Sets an encryption key for decrypting packets.
 * Encryption must be enabled.
 *
 * @param o the object
 * @param encryption_key key to use
 */
void SPProtoDecoder_SetEncryptionKey (SPProtoDecoder *o, uint8_t *encryption_key);

/**
 * Removes an encryption key if one is configured.
 * Encryption must be enabled.
 *
 * @param o the object
 */
void SPProtoDecoder_RemoveEncryptionKey (SPProtoDecoder *o);

/**
 * Starts generating OTPs for a seed to check received packets against.
 * OTPs for this seed will not be recognized until the {@link SPProtoDecoder_otp_handler} handler
 * is called.
 * If OTPs are still being generated for the previous seed, it will be forgotten.
 * OTPs must be enabled.
 *
 * @param o the object
 * @param seed_id seed identifier
 * @param key OTP encryption key
 * @param iv OTP initialization vector
 */
void SPProtoDecoder_AddOTPSeed (SPProtoDecoder *o, uint16_t seed_id, uint8_t *key, uint8_t *iv);

/**
 * Removes all OTP seeds for checking received packets against.
 * OTPs must be enabled.
 *
 * @param o the object
 */
void SPProtoDecoder_RemoveOTPSeeds (SPProtoDecoder *o);

/**
 * Sets handlers.
 *
 * @param o the object
 * @param otp_handler handler called when OTP generation is finished
 * @param user argument to handler
 */
void SPProtoDecoder_SetHandlers (SPProtoDecoder *o, SPProtoDecoder_otp_handler otp_handler, void *user);

#endif
