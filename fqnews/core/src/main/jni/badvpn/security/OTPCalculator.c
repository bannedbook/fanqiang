/**
 * @file OTPCalculator.c
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

#include <limits.h>

#include <misc/balloc.h>

#include <security/OTPCalculator.h>

int OTPCalculator_Init (OTPCalculator *calc, int num_otps, int cipher)
{
    ASSERT(num_otps >= 0)
    ASSERT(BEncryption_cipher_valid(cipher))
    
    // init arguments
    calc->num_otps = num_otps;
    calc->cipher = cipher;
    
    // remember block size
    calc->block_size = BEncryption_cipher_block_size(calc->cipher);
    
    // calculate number of blocks
    if (calc->num_otps > SIZE_MAX / sizeof(otp_t)) {
        goto fail0;
    }
    calc->num_blocks = bdivide_up(calc->num_otps * sizeof(otp_t), calc->block_size);
    
    // allocate buffer
    if (!(calc->data = (otp_t *)BAllocArray(calc->num_blocks, calc->block_size))) {
        goto fail0;
    }
    
    // init debug object
    DebugObject_Init(&calc->d_obj);
    
    return 1;
    
fail0:
    return 0;
}

void OTPCalculator_Free (OTPCalculator *calc)
{
    // free debug object
    DebugObject_Free(&calc->d_obj);
    
    // free buffer
    BFree(calc->data);
}

otp_t * OTPCalculator_Generate (OTPCalculator *calc, uint8_t *key, uint8_t *iv, int shuffle)
{
    ASSERT(shuffle == 0 || shuffle == 1)
    
    // copy IV so it can be updated
    uint8_t iv_work[BENCRYPTION_MAX_BLOCK_SIZE];
    memcpy(iv_work, iv, calc->block_size);
    
    // create zero block
    uint8_t zero[BENCRYPTION_MAX_BLOCK_SIZE];
    memset(zero, 0, calc->block_size);
    
    // init encryptor
    BEncryption encryptor;
    BEncryption_Init(&encryptor, BENCRYPTION_MODE_ENCRYPT, calc->cipher, key);
    
    // encrypt zero blocks
    for (size_t i = 0; i < calc->num_blocks; i++) {
        BEncryption_Encrypt(&encryptor, zero, (uint8_t *)calc->data + i * calc->block_size, calc->block_size, iv_work);
    }
    
    // free encryptor
    BEncryption_Free(&encryptor);
    
    // shuffle if requested
    if (shuffle) {
        int i = 0;
        while (i < calc->num_otps) {
            uint16_t ints[256];
            BRandom_randomize((uint8_t *)ints, sizeof(ints));
            for (int j = 0; j < 256 && i < calc->num_otps; j++) {
                int newIndex = i + (ints[j] % (calc->num_otps - i));
                otp_t temp = calc->data[i];
                calc->data[i] = calc->data[newIndex];
                calc->data[newIndex] = temp;
                i++;
            }
        }
    }
    
    return calc->data;
}
