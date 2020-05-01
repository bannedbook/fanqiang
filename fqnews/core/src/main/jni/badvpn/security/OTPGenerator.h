/**
 * @file OTPGenerator.h
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
 * Object which generates OTPs for use in sending packets.
 */

#ifndef BADVPN_SECURITY_OTPGENERATOR_H
#define BADVPN_SECURITY_OTPGENERATOR_H

#include <misc/debug.h>
#include <security/OTPCalculator.h>
#include <base/DebugObject.h>
#include <threadwork/BThreadWork.h>

/**
 * Handler called when OTP generation for a seed is finished.
 * The OTP position is reset to zero before the handler is called.
 * 
 * @param user as in {@link OTPGenerator_Init}
 */
typedef void (*OTPGenerator_handler) (void *user);

/**
 * Object which generates OTPs for use in sending packets.
 */
typedef struct {
    int num_otps;
    int cipher;
    BThreadWorkDispatcher *twd;
    OTPGenerator_handler handler;
    void *user;
    int position;
    int cur_calc;
    OTPCalculator calc[2];
    otp_t *otps[2];
    int tw_have;
    BThreadWork tw;
    uint8_t tw_key[BENCRYPTION_MAX_KEY_SIZE];
    uint8_t tw_iv[BENCRYPTION_MAX_BLOCK_SIZE];
    DebugObject d_obj;
} OTPGenerator;

/**
 * Initializes the generator.
 * The object is initialized with number of used OTPs = num_otps.
 * {@link BSecurity_GlobalInitThreadSafe} must have been done if
 * {@link BThreadWorkDispatcher_UsingThreads}(twd) = 1.
 *
 * @param g the object
 * @param num_otps number of OTPs to generate from a seed. Must be >=0.
 * @param cipher encryption cipher for calculating the OTPs. Must be valid
 *               according to {@link BEncryption_cipher_valid}.
 * @param twd thread work dispatcher
 * @param handler handler to call when generation of new OTPs is complete,
 *                after {@link OTPGenerator_SetSeed} was called.
 * @param user argument to handler
 * @return 1 on success, 0 on failure
 */
int OTPGenerator_Init (OTPGenerator *g, int num_otps, int cipher, BThreadWorkDispatcher *twd, OTPGenerator_handler handler, void *user) WARN_UNUSED;

/**
 * Frees the generator.
 *
 * @param g the object
 */
void OTPGenerator_Free (OTPGenerator *g);

/**
 * Starts generating OTPs for a seed.
 * When generation is complete and the new OTPs may be used, the {@link OTPGenerator_handler}
 * handler will be called.
 * If OTPs are still being generated for a previous seed, it will be forgotten.
 * This call by itself does not affect the OTP position; rather the position is set to zero
 * before the handler is called.
 *
 * @param g the object
 * @param key encryption key
 * @param iv initialization vector
 */
void OTPGenerator_SetSeed (OTPGenerator *g, uint8_t *key, uint8_t *iv);

/**
 * Returns the number of OTPs used up from the current seed so far.
 * If there is no seed yet, returns num_otps.
 *
 * @param g the object
 * @return number of used OTPs
 */
int OTPGenerator_GetPosition (OTPGenerator *g);

/**
 * Sets the number of used OTPs to num_otps.
 *
 * @param g the object
 */
void OTPGenerator_Reset (OTPGenerator *g);

/**
 * Generates a single OTP.
 * The number of used OTPs must be < num_otps.
 * The number of used OTPs is incremented.
 *
 * @param g the object
 */
otp_t OTPGenerator_GetOTP (OTPGenerator *g);

#endif
