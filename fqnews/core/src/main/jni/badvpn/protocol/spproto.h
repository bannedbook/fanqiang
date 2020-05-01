/**
 * @file spproto.h
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
 * Protocol for securing datagram communication.
 * 
 * Security features implemented:
 *   - Encryption. Encrypts packets with a block cipher.
 *     Protects against a third party from seeing the data
 *     being transmitted.
 *   - Hashes. Adds a hash of the packet into the packet.
 *     Combined with encryption, protects against tampering
 *     with packets and crafting new packets.
 *   - One-time passwords. Adds a password to each packet
 *     for the receiver to recognize. Protects agains replaying
 *     packets and crafting new packets.
 * 
 * A SPProto plaintext packet contains the following, in order:
 *   - if OTPs are used, a struct {@link spproto_otpdata} which contains
 *     the seed ID and the OTP,
 *   - if hashes are used, the hash,
 *   - payload data.
 * 
 * If encryption is used:
 *   - the plaintext is padded by appending a 0x01 byte and as many 0x00
 *     bytes as needed to align to block size,
 *   - the padded plaintext is encrypted, and
 *   - the initialization vector (IV) is prepended.
 */

#ifndef BADVPN_PROTOCOL_SPPROTO_H
#define BADVPN_PROTOCOL_SPPROTO_H

#include <stdint.h>
#include <limits.h>

#include <misc/debug.h>
#include <misc/balign.h>
#include <misc/packed.h>
#include <security/BHash.h>
#include <security/BEncryption.h>
#include <security/OTPCalculator.h>

#define SPPROTO_HASH_MODE_NONE 0
#define SPPROTO_ENCRYPTION_MODE_NONE 0
#define SPPROTO_OTP_MODE_NONE 0

/**
 * Stores security parameters for SPProto.
 */
struct spproto_security_params {
    /**
     * Hash mode.
     * Either SPPROTO_HASH_MODE_NONE for no hashes, or a valid bhash
     * hash mode.
     */
    int hash_mode;
    
    /**
     * Encryption mode.
     * Either SPPROTO_ENCRYPTION_MODE_NONE for no encryption, or a valid
     * {@link BEncryption} cipher.
     */
    int encryption_mode;
    
    /**
     * One-time password (OTP) mode.
     * Either SPPROTO_OTP_MODE_NONE for no OTPs, or a valid
     * {@link BEncryption} cipher.
     */
    int otp_mode;
    
    /**
     * If OTPs are used (otp_mode != SPPROTO_OTP_MODE_NONE), number of
     * OTPs generated from a single seed.
     */
    int otp_num;
};

#define SPPROTO_HAVE_HASH(_params) ((_params).hash_mode != SPPROTO_HASH_MODE_NONE)
#define SPPROTO_HASH_SIZE(_params) ( \
    SPPROTO_HAVE_HASH(_params) ? \
    BHash_size((_params).hash_mode) : \
    0 \
)

#define SPPROTO_HAVE_ENCRYPTION(_params) ((_params).encryption_mode != SPPROTO_ENCRYPTION_MODE_NONE)

#define SPPROTO_HAVE_OTP(_params) ((_params).otp_mode != SPPROTO_OTP_MODE_NONE)

B_START_PACKED
struct spproto_otpdata {
    uint16_t seed_id;
    otp_t otp;
} B_PACKED;
B_END_PACKED

#define SPPROTO_HEADER_OTPDATA_OFF(_params) 0
#define SPPROTO_HEADER_OTPDATA_LEN(_params) (SPPROTO_HAVE_OTP(_params) ? sizeof(struct spproto_otpdata) : 0)
#define SPPROTO_HEADER_HASH_OFF(_params) (SPPROTO_HEADER_OTPDATA_OFF(_params) + SPPROTO_HEADER_OTPDATA_LEN(_params))
#define SPPROTO_HEADER_HASH_LEN(_params) SPPROTO_HASH_SIZE(_params)
#define SPPROTO_HEADER_LEN(_params) (SPPROTO_HEADER_HASH_OFF(_params) + SPPROTO_HEADER_HASH_LEN(_params))

/**
 * Asserts that the given SPProto security parameters are valid.
 * 
 * @param params security parameters
 */
static void spproto_assert_security_params (struct spproto_security_params params)
{
    ASSERT(params.hash_mode == SPPROTO_HASH_MODE_NONE || BHash_type_valid(params.hash_mode))
    ASSERT(params.encryption_mode == SPPROTO_ENCRYPTION_MODE_NONE || BEncryption_cipher_valid(params.encryption_mode))
    ASSERT(params.otp_mode == SPPROTO_OTP_MODE_NONE || BEncryption_cipher_valid(params.otp_mode))
    ASSERT(params.otp_mode == SPPROTO_OTP_MODE_NONE || params.otp_num > 0)
}

/**
 * Calculates the maximum payload size for SPProto given the
 * security parameters and the maximum encoded packet size.
 * 
 * @param params security parameters
 * @param carrier_mtu maximum encoded packet size. Must be >=0.
 * @return maximum payload size. Negative means is is impossible
 *         to encode anything.
 */
static int spproto_payload_mtu_for_carrier_mtu (struct spproto_security_params params, int carrier_mtu)
{
    spproto_assert_security_params(params);
    ASSERT(carrier_mtu >= 0)
    
    if (params.encryption_mode == SPPROTO_ENCRYPTION_MODE_NONE) {
        return (carrier_mtu - SPPROTO_HEADER_LEN(params));
    } else {
        int block_size = BEncryption_cipher_block_size(params.encryption_mode);
        return (balign_down(carrier_mtu, block_size) - block_size - SPPROTO_HEADER_LEN(params) - 1);
    }
}

/**
 * Calculates the maximum encoded packet size for SPProto given the
 * security parameters and the maximum payload size.
 * 
 * @param params security parameters
 * @param payload_mtu maximum payload size. Must be >=0.
 * @return maximum encoded packet size, -1 if payload_mtu is too large
 */
static int spproto_carrier_mtu_for_payload_mtu (struct spproto_security_params params, int payload_mtu)
{
    spproto_assert_security_params(params);
    ASSERT(payload_mtu >= 0)
    
    if (params.encryption_mode == SPPROTO_ENCRYPTION_MODE_NONE) {
        if (payload_mtu > INT_MAX - SPPROTO_HEADER_LEN(params)) {
            return -1;
        }
        
        return (SPPROTO_HEADER_LEN(params) + payload_mtu);
    } else {
        int block_size = BEncryption_cipher_block_size(params.encryption_mode);
        
        if (payload_mtu > INT_MAX - (block_size + SPPROTO_HEADER_LEN(params) + block_size)) {
            return -1;
        }
        
        return (block_size + balign_up((SPPROTO_HEADER_LEN(params) + payload_mtu + 1), block_size));
    }
}

#endif
