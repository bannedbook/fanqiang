/**
 * @file OTPChecker.h
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
 * Object that checks OTPs agains known seeds.
 */

#ifndef BADVPN_SECURITY_OTPCHECKER_H
#define BADVPN_SECURITY_OTPCHECKER_H

#include <stdint.h>

#include <misc/balign.h>
#include <misc/debug.h>
#include <misc/modadd.h>
#include <security/OTPCalculator.h>
#include <base/DebugObject.h>
#include <threadwork/BThreadWork.h>

struct OTPChecker_entry {
    otp_t otp;
    int avail;
};

struct OTPChecker_table {
    uint16_t id;
    struct OTPChecker_entry *entries;
};

/**
 * Handler called when OTP generation for a seed is finished and new OTPs
 * can be recognized.
 * 
 * @param user as in {@link OTPChecker_Init}
 */
typedef void (*OTPChecker_handler) (void *user);

/**
 * Object that checks OTPs agains known seeds.
 */
typedef struct {
    BThreadWorkDispatcher *twd;
    OTPChecker_handler handler;
    void *user;
    int num_otps;
    int cipher;
    int num_entries;
    int num_tables;
    int tables_used;
    int next_table;
    OTPCalculator calc;
    struct OTPChecker_table *tables;
    struct OTPChecker_entry *entries;
    int tw_have;
    BThreadWork tw;
    uint8_t tw_key[BENCRYPTION_MAX_KEY_SIZE];
    uint8_t tw_iv[BENCRYPTION_MAX_BLOCK_SIZE];
    DebugObject d_obj;
} OTPChecker;

/**
 * Initializes the checker.
 * {@link BSecurity_GlobalInitThreadSafe} must have been done if
 * {@link BThreadWorkDispatcher_UsingThreads}(twd) = 1.
 *
 * @param mc the object
 * @param num_otps number of OTPs to generate from a seed. Must be >0.
 * @param cipher encryption cipher for calculating the OTPs. Must be valid
 *               according to {@link BEncryption_cipher_valid}.
 * @param num_tables number of tables to keep, each for one seed. Must be >0.
 * @param twd thread work dispatcher
 * @return 1 on success, 0 on failure
 */
int OTPChecker_Init (OTPChecker *mc, int num_otps, int cipher, int num_tables, BThreadWorkDispatcher *twd) WARN_UNUSED;

/**
 * Frees the checker.
 *
 * @param mc the object
 */
void OTPChecker_Free (OTPChecker *mc);

/**
 * Starts generating OTPs to recognize for a seed.
 * OTPs for this seed will not be recognized until the {@link OTPChecker_handler} handler is called.
 * If OTPs are still being generated for a previous seed, it will be forgotten.
 *
 * @param mc the object
 * @param seed_id seed identifier
 * @param key encryption key
 * @param iv initialization vector
 */
void OTPChecker_AddSeed (OTPChecker *mc, uint16_t seed_id, uint8_t *key, uint8_t *iv);

/**
 * Removes all active seeds.
 *
 * @param mc the object
 */
void OTPChecker_RemoveSeeds (OTPChecker *mc);

/**
 * Checks an OTP.
 *
 * @param mc the object
 * @param seed_id identifer of seed whom the OTP is claimed to belong to
 * @param otp OTP to check
 * @return 1 if the OTP is valid, 0 if not
 */
int OTPChecker_CheckOTP (OTPChecker *mc, uint16_t seed_id, otp_t otp);

/**
 * Sets handlers.
 *
 * @param mc the object
 * @param handler handler to call when generation of new OTPs is complete,
 *                after {@link OTPChecker_AddSeed} was called.
 * @param user argument to handler
 */
void OTPChecker_SetHandlers (OTPChecker *mc, OTPChecker_handler handler, void *user);

#endif
