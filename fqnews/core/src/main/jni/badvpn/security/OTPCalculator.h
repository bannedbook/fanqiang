/**
 * @file OTPCalculator.h
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
 * Object that calculates OTPs.
 */

#ifndef BADVPN_SECURITY_OTPCALCULATOR_H
#define BADVPN_SECURITY_OTPCALCULATOR_H

#include <stdlib.h>
#include <string.h>

#include <misc/balign.h>
#include <misc/debug.h>
#include <security/BRandom.h>
#include <security/BEncryption.h>
#include <base/DebugObject.h>

/**
 * Type for an OTP.
 */
typedef uint32_t otp_t;

/**
 * Object that calculates OTPs.
 */
typedef struct {
    DebugObject d_obj;
    int num_otps;
    int cipher;
    int block_size;
    size_t num_blocks;
    otp_t *data;
} OTPCalculator;

/**
 * Initializes the calculator.
 * {@link BSecurity_GlobalInitThreadSafe} must have been done if this object
 * will be used from a non-main thread.
 *
 * @param calc the object
 * @param num_otps number of OTPs to generate from a seed. Must be >=0.
 * @param cipher encryption cipher for calculating the OTPs. Must be valid
 *               according to {@link BEncryption_cipher_valid}.
 * @return 1 on success, 0 on failure
 */
int OTPCalculator_Init (OTPCalculator *calc, int num_otps, int cipher) WARN_UNUSED;

/**
 * Frees the calculator.
 *
 * @param calc the object
 */
void OTPCalculator_Free (OTPCalculator *calc);

/**
 * Generates OTPs from the given key and IV.
 *
 * @param calc the object
 * @param key encryption key
 * @param iv initialization vector
 * @param shuffle whether to shuffle the OTPs. Must be 1 or 0.
 * @return pointer to an array of 32-bit OPTs. Constains as many OTPs as was specified
 *         in {@link OTPCalculator_Init}. Valid until the next generation or
 *         until the object is freed.
 */
otp_t * OTPCalculator_Generate (OTPCalculator *calc, uint8_t *key, uint8_t *iv, int shuffle);

#endif
