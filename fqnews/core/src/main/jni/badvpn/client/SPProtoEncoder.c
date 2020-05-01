/**
 * @file SPProtoEncoder.c
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
#include <stdlib.h>

#include <misc/balign.h>
#include <misc/offset.h>
#include <misc/byteorder.h>
#include <security/BRandom.h>
#include <security/BHash.h>

#include "SPProtoEncoder.h"

static int can_encode (SPProtoEncoder *o);
static void encode_packet (SPProtoEncoder *o);
static void encode_work_func (SPProtoEncoder *o);
static void encode_work_handler (SPProtoEncoder *o);
static void maybe_encode (SPProtoEncoder *o);
static void output_handler_recv (SPProtoEncoder *o, uint8_t *data);
static void input_handler_done (SPProtoEncoder *o, int data_len);
static void handler_job_hander (SPProtoEncoder *o);
static void otpgenerator_handler (SPProtoEncoder *o);
static void maybe_stop_work (SPProtoEncoder *o);

static int can_encode (SPProtoEncoder *o)
{
    ASSERT(o->in_len >= 0)
    ASSERT(o->out_have)
    ASSERT(!o->tw_have)
    
    return (
        (!SPPROTO_HAVE_OTP(o->sp_params) || OTPGenerator_GetPosition(&o->otpgen) < o->sp_params.otp_num) &&
        (!SPPROTO_HAVE_ENCRYPTION(o->sp_params) || o->have_encryption_key)
    );
}

static void encode_packet (SPProtoEncoder *o)
{
    ASSERT(o->in_len >= 0)
    ASSERT(o->out_have)
    ASSERT(!o->tw_have)
    ASSERT(can_encode(o))
    
    // generate OTP, remember seed ID
    if (SPPROTO_HAVE_OTP(o->sp_params)) {
        o->tw_seed_id = o->otpgen_seed_id;
        o->tw_otp = OTPGenerator_GetOTP(&o->otpgen);
    }
    
    // start work
    BThreadWork_Init(&o->tw, o->twd, (BThreadWork_handler_done)encode_work_handler, o, (BThreadWork_work_func)encode_work_func, o);
    o->tw_have = 1;
    
    // schedule OTP warning handler
    if (SPPROTO_HAVE_OTP(o->sp_params) && OTPGenerator_GetPosition(&o->otpgen) == o->otp_warning_count) {
        BPending_Set(&o->handler_job);
    }
}

static void encode_work_func (SPProtoEncoder *o)
{
    ASSERT(o->in_len >= 0)
    ASSERT(o->out_have)
    ASSERT(!SPPROTO_HAVE_ENCRYPTION(o->sp_params) || o->have_encryption_key)
    
    ASSERT(o->in_len <= o->input_mtu)
    
    // determine plaintext location
    uint8_t *plaintext = (SPPROTO_HAVE_ENCRYPTION(o->sp_params) ? o->buf : o->out);
    
    // plaintext begins with header
    uint8_t *header = plaintext;
    
    // plaintext is header + payload
    int plaintext_len = SPPROTO_HEADER_LEN(o->sp_params) + o->in_len;
    
    // write OTP
    if (SPPROTO_HAVE_OTP(o->sp_params)) {
        struct spproto_otpdata header_otpd;
        header_otpd.seed_id = htol16(o->tw_seed_id);
        header_otpd.otp = o->tw_otp;
        memcpy(header + SPPROTO_HEADER_OTPDATA_OFF(o->sp_params), &header_otpd, sizeof(header_otpd));
    }
    
    // write hash
    if (SPPROTO_HAVE_HASH(o->sp_params)) {
        uint8_t *header_hash = header + SPPROTO_HEADER_HASH_OFF(o->sp_params);
        // zero hash field
        memset(header_hash, 0, o->hash_size);
        // calculate hash
        uint8_t hash[BHASH_MAX_SIZE];
        BHash_calculate(o->sp_params.hash_mode, plaintext, plaintext_len, hash);
        // set hash field
        memcpy(header_hash, hash, o->hash_size);
    }
    
    int out_len;
    
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) {
        // encrypting pad(header + payload)
        int cyphertext_len = balign_up((plaintext_len + 1), o->enc_block_size);
        
        // write padding
        plaintext[plaintext_len] = 1;
        for (int i = plaintext_len + 1; i < cyphertext_len; i++) {
            plaintext[i] = 0;
        }
        
        // generate IV
        BRandom_randomize(o->out, o->enc_block_size);
        
        // copy IV because BEncryption_Encrypt changes the IV
        uint8_t iv[BENCRYPTION_MAX_BLOCK_SIZE];
        memcpy(iv, o->out, o->enc_block_size);
        
        // encrypt
        BEncryption_Encrypt(&o->encryptor, plaintext, o->out + o->enc_block_size, cyphertext_len, iv);
        out_len = o->enc_block_size + cyphertext_len;
    } else {
        out_len = plaintext_len;
    }
    
    // remember length
    o->tw_out_len = out_len;
}

static void encode_work_handler (SPProtoEncoder *o)
{
    ASSERT(o->in_len >= 0)
    ASSERT(o->out_have)
    ASSERT(o->tw_have)
    
    // free work
    BThreadWork_Free(&o->tw);
    o->tw_have = 0;
    
    // finish packet
    o->in_len = -1;
    o->out_have = 0;
    PacketRecvInterface_Done(&o->output, o->tw_out_len);
}

static void maybe_encode (SPProtoEncoder *o)
{
    if (o->in_len >= 0 && o->out_have && !o->tw_have && can_encode(o)) {
        encode_packet(o);
    }
}

static void output_handler_recv (SPProtoEncoder *o, uint8_t *data)
{
    ASSERT(o->in_len == -1)
    ASSERT(!o->out_have)
    ASSERT(!o->tw_have)
    DebugObject_Access(&o->d_obj);
    
    // remember output packet
    o->out_have = 1;
    o->out = data;
    
    // determine plaintext location
    uint8_t *plaintext = (SPPROTO_HAVE_ENCRYPTION(o->sp_params) ? o->buf : o->out);
    
    // schedule receive
    PacketRecvInterface_Receiver_Recv(o->input, plaintext + SPPROTO_HEADER_LEN(o->sp_params));
}

static void input_handler_done (SPProtoEncoder *o, int data_len)
{
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->input_mtu)
    ASSERT(o->in_len == -1)
    ASSERT(o->out_have)
    ASSERT(!o->tw_have)
    DebugObject_Access(&o->d_obj);
    
    // remember input packet
    o->in_len = data_len;
    
    // encode if possible
    if (can_encode(o)) {
        encode_packet(o);
    }
}

static void handler_job_hander (SPProtoEncoder *o)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    if (o->handler) {
        o->handler(o->user);
        return;
    }
}

static void otpgenerator_handler (SPProtoEncoder *o)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // remember seed ID
    o->otpgen_seed_id = o->otpgen_pending_seed_id;
    
    // possibly continue I/O
    maybe_encode(o);
}

static void maybe_stop_work (SPProtoEncoder *o)
{
    // stop existing work
    if (o->tw_have) {
        BThreadWork_Free(&o->tw);
        o->tw_have = 0;
    }
}

int SPProtoEncoder_Init (SPProtoEncoder *o, PacketRecvInterface *input, struct spproto_security_params sp_params, int otp_warning_count, BPendingGroup *pg, BThreadWorkDispatcher *twd)
{
    spproto_assert_security_params(sp_params);
    ASSERT(spproto_carrier_mtu_for_payload_mtu(sp_params, PacketRecvInterface_GetMTU(input)) >= 0)
    if (SPPROTO_HAVE_OTP(sp_params)) {
        ASSERT(otp_warning_count > 0)
        ASSERT(otp_warning_count <= sp_params.otp_num)
    }
    
    // init arguments
    o->input = input;
    o->sp_params = sp_params;
    o->otp_warning_count = otp_warning_count;
    o->twd = twd;
    
    // set no handlers
    o->handler = NULL;
    
    // calculate hash size
    if (SPPROTO_HAVE_HASH(o->sp_params)) {
        o->hash_size = BHash_size(o->sp_params.hash_mode);
    }
    
    // calculate encryption block and key sizes
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) {
        o->enc_block_size = BEncryption_cipher_block_size(o->sp_params.encryption_mode);
        o->enc_key_size = BEncryption_cipher_key_size(o->sp_params.encryption_mode);
    }
    
    // init otp generator
    if (SPPROTO_HAVE_OTP(o->sp_params)) {
        if (!OTPGenerator_Init(&o->otpgen, o->sp_params.otp_num, o->sp_params.otp_mode, o->twd, (OTPGenerator_handler)otpgenerator_handler, o)) {
            goto fail0;
        }
    }
    
    // have no encryption key
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) { 
        o->have_encryption_key = 0;
    }
    
    // remember input MTU
    o->input_mtu = PacketRecvInterface_GetMTU(o->input);
    
    // calculate output MTU
    o->output_mtu = spproto_carrier_mtu_for_payload_mtu(o->sp_params, o->input_mtu);
    
    // init input
    PacketRecvInterface_Receiver_Init(o->input, (PacketRecvInterface_handler_done)input_handler_done, o);
    
    // have no input in buffer
    o->in_len = -1;
    
    // init output
    PacketRecvInterface_Init(&o->output, o->output_mtu, (PacketRecvInterface_handler_recv)output_handler_recv, o, pg);
    
    // have no output available
    o->out_have = 0;
    
    // allocate plaintext buffer
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) {
        int buf_size = balign_up((SPPROTO_HEADER_LEN(o->sp_params) + o->input_mtu + 1), o->enc_block_size);
        if (!(o->buf = (uint8_t *)malloc(buf_size))) {
            goto fail1;
        }
    }
    
    // init handler job
    BPending_Init(&o->handler_job, pg, (BPending_handler)handler_job_hander, o);
    
    // have no work
    o->tw_have = 0;
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail1:
    PacketRecvInterface_Free(&o->output);
    if (SPPROTO_HAVE_OTP(o->sp_params)) {
        OTPGenerator_Free(&o->otpgen);
    }
fail0:
    return 0;
}

void SPProtoEncoder_Free (SPProtoEncoder *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free work
    if (o->tw_have) {
        BThreadWork_Free(&o->tw);
    }
    
    // free handler job
    BPending_Free(&o->handler_job);
    
    // free plaintext buffer
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params)) {
        free(o->buf);
    }
    
    // free output
    PacketRecvInterface_Free(&o->output);
    
    // free encryptor
    if (SPPROTO_HAVE_ENCRYPTION(o->sp_params) && o->have_encryption_key) {
        BEncryption_Free(&o->encryptor);
    }
    
    // free otp generator
    if (SPPROTO_HAVE_OTP(o->sp_params)) {
        OTPGenerator_Free(&o->otpgen);
    }
}

PacketRecvInterface * SPProtoEncoder_GetOutput (SPProtoEncoder *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}

void SPProtoEncoder_SetEncryptionKey (SPProtoEncoder *o, uint8_t *encryption_key)
{
    ASSERT(SPPROTO_HAVE_ENCRYPTION(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // stop existing work
    maybe_stop_work(o);
    
    // free encryptor
    if (o->have_encryption_key) {
        BEncryption_Free(&o->encryptor);
    }
    
    // init encryptor
    BEncryption_Init(&o->encryptor, BENCRYPTION_MODE_ENCRYPT, o->sp_params.encryption_mode, encryption_key);
    
    // have encryption key
    o->have_encryption_key = 1;
    
    // possibly continue I/O
    maybe_encode(o);
}

void SPProtoEncoder_RemoveEncryptionKey (SPProtoEncoder *o)
{
    ASSERT(SPPROTO_HAVE_ENCRYPTION(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // stop existing work
    maybe_stop_work(o);
    
    if (o->have_encryption_key) {
        // free encryptor
        BEncryption_Free(&o->encryptor);
        
        // have no encryption key
        o->have_encryption_key = 0;
    }
}

void SPProtoEncoder_SetOTPSeed (SPProtoEncoder *o, uint16_t seed_id, uint8_t *key, uint8_t *iv)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // give seed to OTP generator
    OTPGenerator_SetSeed(&o->otpgen, key, iv);
    
    // remember seed ID
    o->otpgen_pending_seed_id = seed_id;
}

void SPProtoEncoder_RemoveOTPSeed (SPProtoEncoder *o)
{
    ASSERT(SPPROTO_HAVE_OTP(o->sp_params))
    DebugObject_Access(&o->d_obj);
    
    // reset OTP generator
    OTPGenerator_Reset(&o->otpgen);
}

void SPProtoEncoder_SetHandlers (SPProtoEncoder *o, SPProtoEncoder_handler handler, void *user)
{
    DebugObject_Access(&o->d_obj);
    
    o->handler = handler;
    o->user = user;
}
