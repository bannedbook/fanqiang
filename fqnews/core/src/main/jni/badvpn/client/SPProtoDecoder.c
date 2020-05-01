/**
 * @file SPProtoDecoder.c
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

#include <misc/balign.h>
#include <misc/byteorder.h>
#include <security/BHash.h>

#include "SPProtoDecoder.h"

#include <generated/blog_channel_SPProtoDecoder.h>

#define PeerLog(_o, ...) BLog_LogViaFunc((_o)->logfunc, (_o)->user, BLOG_CURRENT_CHANNEL, __VA_ARGS__)

static void decode_work_func (SPProtoDecoder *o)
{
    ASSERT(o->in_len >= 0)
    ASSERT(o->in_len <= o->input_mtu)
    
    uint8_t *in = o->in;
    int in_len = o->in_len;
    
    o->tw_out_len = -1;
    
    uint8_t *plaintext;
    int plaintext_len;
    
    // decrypt if needed
    if (!SPPROTO_HAVE_ENCRYPTION(o->sp_params)) {
        plaintext = in;
        plaintext_len = in_len;
    } else {
        // input must be a multiple of blocks size
        if (in_len % o->enc_block_size != 0) {
            PeerLog(o, BLOG_WARNING, "packet size not a multiple of block size");
            return;
        }
        
        // input must have an IV block
        if (in_len < o->enc_block_size) {
            PeerLog(o, BLOG_WARNING, "packet does not have an IV");
            return;
        }
        
        // check if we have encryption key
        if (!o->have_encryption_key) {
            PeerLog(o, BLOG_WARNING, "have no encryption key");
            return;
        }
        
        // copy IV as BEncryption_Decrypt changes the IV
        uint8_t iv[BENCRYPTION_MAX_BLOCK_SIZE];
        memcpy(iv, in, o->enc_block_size);
        
        // decrypt
        uint8_t *ciphertext = in + o->enc_block_size;
        int ciphertext_len = in_len - o->enc_block_size;
        plaintext = o->buf;
        BEncryption_Decrypt(&o->encryptor, ciphertext, plaintext, ciphertext_len, iv);
        
        // read padding
        if (ciphertext_len < o->enc_block_size) {
            PeerLog(o, BLOG_WARNING, "packet does not have a padding block");
            return;
        }
        int i;
        for (i = ciphertext_len - 1; i >= ciphertext_len - o->enc_block_size; i--) {
            if (plaintext[i] == 1) {
                break;
            }
            if (plaintext[i] != 0) {
                PeerLog(o, BLOG_WARNING, "packet padding wrong (nonzero byte)");
                return;
            }
        }
        if (i < ciphertext_len - o->enc_block_size) {
            PeerLog(o, BLOG_WARNING, "packet padding wrong (all zeroes)");
            return;
        }
        plaintext_len = i;
    }
    
    // check for header
    if (plaintext_len < SPPROTO_HEADER_LEN(o->sp_params)) {
        PeerLog(o, BLOG_WARNING, "packet has no header");
        return;
    }
    uint8_t *header = plaintext;
    
    // check data length
    if (plaintext_len - SPPROTO_HEADER_LEN(o->sp_params) > o->output_mtu) {
        PeerLog(o, BLOG_WARNING, "packet too long");
        return;
    }
    
    // check OTP
    if (SPPROTO_HAVE_OTP(o->sp_params)) {
        // remember seed and OTP (can't check from here)
        struct spproto_otpdata header_otpd;
        memcpy(&header_otpd, header + SPPROTO_HEADER_OTPDATA_OFF(o->sp_params), sizeof(header_otpd));
        o->tw_out_seed_id = ltoh16(header_otpd.seed_id);
        o->tw_out_otp = header_otpd.otp;
    }
    
    // check hash
    if (SPPROTO_HAVE_HASH(o->sp_params)) {
        uint8_t *header_hash = header + SPPROTO_HEADER_HASH_OFF(o->sp_params);
        // read hash
        uint8_t hash[BHASH_MAX_SIZE];
        memcpy(hash, header_hash, o->hash_size);
        // zero hash in packet
        memset(header_hash, 0, o->hash_size);
        // calculate hash
        uint8_t hash_calc[BHASH_MAX_SIZE];
        BHash_calculate(o->sp_params.hash_mode, plaintext, plaintext_len, hash_calc);
        // set hash field to its original value
        memcpy(header_hash, hash, o->hash_size);
        // compare hashes
        if (memcmp(hash, hash_calc, o->hash_size)) {
            PeerLog(o, BLOG_WARNING, "packet has wrong hash");
            return;
        }
    }
    
    // return packet
    o->tw_out = plaintext + SPPROTO_HEADER_LEN(o->sp_params);
    o->tw_out_len = plaintext_len - SPPROTO_HEADER_LEN(o->sp_params);
}

static void decode_work_handler (SPProtoDecoder *o)
{
    ASSERT(o->in_len >= 0)
    ASSERT(o->tw_have)
    DebugObject_Access(&o->d_obj);
    
    // free work
    BThreadWork_Free(&o->tw);
    o->tw_have = 0;
    
    // check OTP
    if (SPPROTO_HAVE_OTP(o->sp_params) && o->tw_out_len >= 0) {
        if (!OTPChecker_CheckOTP(&o->otpchecker, o->tw_out_seed_id, o->tw_out_otp)) {
            PeerLog(o, BLOG_WARNING, "packet has wrong OTP");
            o->tw_out_len = -1;
        }
    }
    
    if (o->tw_out_len < 0) {
        // cannot decode, finish input packet
        PacketPassInterface_Done(&o->input);
        o->in_len = -1;
    } else {
        // submit decoded packet to output
        PacketPassInterface_Sender_Send(o->output, o->tw_out, o->tw_out_len);
    }
}

static void input_handler_send (SPProtoDecoder *o, uint8_t *data, int data_len)
{
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->input_mtu)
    ASSERT(o->in_len == -1)
    ASSERT(!o->tw_have)
    DebugObject_Access(&o->d_obj);
    
    // remember input
    o->in = data;
    o->in_len = data_len;
    
    // start decoding
    BThreadWork_Init(&o->tw, o->twd, (BThreadWork_handler_done)decode_work_handler, o, (BThreadWork_work_func)decode_work_func, o);
    o->tw_have = 1;
}

static void output_handler_done (SPProtoDecoder *o)
{
    ASSERT(o->in_len >= 0)
    ASSERT(!o->tw_have)
    DebugObject_Access(&o->d_obj);
    
    // finish input packet
    PacketPassInterface_Done(&o->input);
    o->in_len = -1;
}

static void maybe_stop_work_and_ignore (SPProtoDecoder *o)
{
    ASSERT(!(o->tw_have) || o->in_len >= 0)
    
    if (o->tw_have) {
        // free work
        BThreadWork_Free(&o->tw);
        o->tw_have = 0;
        
        // ignore packet, receive next one
        PacketPassInterface_Done(&o->input);
        o->in_len = -1;
    }
}

int SPProtoDecoder_Init (SPProtoDecoder *o, PacketPassInterface *output, struct spproto_security_params sp_params, int num_otp_seeds, BPendingGroup *pg, BThreadWorkDispatcher *twd, void *user, BLog_logfunc logfunc)
{
    spproto_assert_security_params(sp_params);
    ASSERT(spproto_carrier_mtu_for_payload_mtu(sp_params, PacketPassInterface_GetMTU(output)) >= 0)
    ASSERT(!SPPROTO_HAVE_OTP(sp_params) || num_otp_seeds >= 2)
    
    // init arguments
    o->output = output;
    o->sp_params = sp_params;
    o->twd = twd;
    o->user = user;
    o->logfunc = logfunc;
    
    // init output
    PacketPassInterface_Sender_Init(o->output, (PacketPassInterface_handler_done)output_handler_done, o);
    
    // remember output MTU
    o->output_mtu = PacketPassInterface_GetMTU(o->output);
    
    // calculate hash size
    if (SPPROTO_HAVE_HASH(o->sp_params)) {
        o->hash_size = BHash_size(o->sp_params.hash_mode);
    }
    
    // calculate encryption block and key sizes
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) {
        o->enc_block_size = BEncryption_cipher_block_size(o->sp_params.encryption_mode);
        o->enc_key_size = BEncryption_cipher_key_size(o->sp_params.encryption_mode);
    }
    
    // calculate input MTU
    o->input_mtu = spproto_carrier_mtu_for_payload_mtu(o->sp_params, o->output_mtu);
    
    // allocate plaintext buffer
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) {
        int buf_size = balign_up((SPPROTO_HEADER_LEN(o->sp_params) + o->output_mtu + 1), o->enc_block_size);
        if (!(o->buf = (uint8_t *)malloc(buf_size))) {
            goto fail0;
        }
    }
    
    // init input
    PacketPassInterface_Init(&o->input, o->input_mtu, (PacketPassInterface_handler_send)input_handler_send, o, pg);
    
    // init OTP checker
    if (SPPROTO_HAVE_OTP(o->sp_params)) {
        if (!OTPChecker_Init(&o->otpchecker, o->sp_params.otp_num, o->sp_params.otp_mode, num_otp_seeds, o->twd)) {
            goto fail1;
        }
    }
    
    // have no encryption key
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) { 
        o->have_encryption_key = 0;
    }
    
    // have no input packet
    o->in_len = -1;
    
    // have no work
    o->tw_have = 0;
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail1:
    PacketPassInterface_Free(&o->input);
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) {
        free(o->buf);
    }
fail0:
    return 0;
}

void SPProtoDecoder_Free (SPProtoDecoder *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free work
    if (o->tw_have) {
        BThreadWork_Free(&o->tw);
    }
    
    // free encryptor
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params) && o->have_encryption_key) {
        BEncryption_Free(&o->encryptor);
    }
    
    // free OTP checker
    if (SPPROTO_HAVE_OTP(o->sp_params)) {
        OTPChecker_Free(&o->otpchecker);
    }
    
    // free input
    PacketPassInterface_Free(&o->input);
    
    // free plaintext buffer
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) {
        free(o->buf);
    }
}

PacketPassInterface * SPProtoDecoder_GetInput (SPProtoDecoder *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->input;
}

void SPProtoDecoder_SetEncryptionKey (SPProtoDecoder *o, uint8_t *encryption_key)
{
    ASSERT(SPPROTO_HAVE_ENCRYPTION(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // stop existing work
    maybe_stop_work_and_ignore(o);
    
    // free encryptor
    if (o->have_encryption_key) {
        BEncryption_Free(&o->encryptor);
    }
    
    // init encryptor
    BEncryption_Init(&o->encryptor, BENCRYPTION_MODE_DECRYPT, o->sp_params.encryption_mode, encryption_key);
    
    // have encryption key
    o->have_encryption_key = 1;
}

void SPProtoDecoder_RemoveEncryptionKey (SPProtoDecoder *o)
{
    ASSERT(SPPROTO_HAVE_ENCRYPTION(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // stop existing work
    maybe_stop_work_and_ignore(o);
    
    if (o->have_encryption_key) {
        // free encryptor
        BEncryption_Free(&o->encryptor);
        
        // have no encryption key
        o->have_encryption_key = 0;
    }
}

void SPProtoDecoder_AddOTPSeed (SPProtoDecoder *o, uint16_t seed_id, uint8_t *key, uint8_t *iv)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    OTPChecker_AddSeed(&o->otpchecker, seed_id, key, iv);
}

void SPProtoDecoder_RemoveOTPSeeds (SPProtoDecoder *o)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    OTPChecker_RemoveSeeds(&o->otpchecker);
}

void SPProtoDecoder_SetHandlers (SPProtoDecoder *o, SPProtoDecoder_otp_handler otp_handler, void *user)
{
    DebugObject_Access(&o->d_obj);
    
    if (SPPROTO_HAVE_OTP(o->sp_params)) {
        OTPChecker_SetHandlers(&o->otpchecker, otp_handler, user);
    }
}
