/**
 * @file OTPGenerator.c
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

#include <security/OTPGenerator.h>

static void work_func (OTPGenerator *g)
{
    g->otps[!g->cur_calc] = OTPCalculator_Generate(&g->calc[!g->cur_calc], g->tw_key, g->tw_iv, 1);
}

static void work_done_handler (OTPGenerator *g)
{
    ASSERT(g->tw_have)
    DebugObject_Access(&g->d_obj);
    
    // free work
    BThreadWork_Free(&g->tw);
    g->tw_have = 0;
    
    // use new OTPs
    g->cur_calc = !g->cur_calc;
    g->position = 0;
    
    // call handler
    g->handler(g->user);
    return;
}

int OTPGenerator_Init (OTPGenerator *g, int num_otps, int cipher, BThreadWorkDispatcher *twd, OTPGenerator_handler handler, void *user)
{
    ASSERT(num_otps >= 0)
    ASSERT(BEncryption_cipher_valid(cipher))
    
    // init arguments
    g->num_otps = num_otps;
    g->cipher = cipher;
    g->twd = twd;
    g->handler = handler;
    g->user = user;
    
    // init position
    g->position = g->num_otps;
    
    // init calculator
    if (!OTPCalculator_Init(&g->calc[0], g->num_otps, g->cipher)) {
        goto fail0;
    }
    
    // init calculator
    if (!OTPCalculator_Init(&g->calc[1], g->num_otps, g->cipher)) {
        goto fail1;
    }
    
    // set current calculator
    g->cur_calc = 0;
    
    // have no work
    g->tw_have = 0;
    
    DebugObject_Init(&g->d_obj);
    return 1;
    
fail1:
    OTPCalculator_Free(&g->calc[0]);
fail0:
    return 0;
}

void OTPGenerator_Free (OTPGenerator *g)
{
    DebugObject_Free(&g->d_obj);
    
    // free work
    if (g->tw_have) {
        BThreadWork_Free(&g->tw);
    }
    
    // free calculator
    OTPCalculator_Free(&g->calc[1]);
    
    // free calculator
    OTPCalculator_Free(&g->calc[0]);
}

void OTPGenerator_SetSeed (OTPGenerator *g, uint8_t *key, uint8_t *iv)
{
    DebugObject_Access(&g->d_obj);
    
    // free existing work
    if (g->tw_have) {
        BThreadWork_Free(&g->tw);
    }
    
    // copy key and IV
    memcpy(g->tw_key, key, BEncryption_cipher_key_size(g->cipher));
    memcpy(g->tw_iv, iv, BEncryption_cipher_block_size(g->cipher));
    
    // start work
    BThreadWork_Init(&g->tw, g->twd, (BThreadWork_handler_done)work_done_handler, g, (BThreadWork_work_func)work_func, g);
    
    // set have work
    g->tw_have = 1;
}

int OTPGenerator_GetPosition (OTPGenerator *g)
{
    DebugObject_Access(&g->d_obj);
    
    return g->position;
}

void OTPGenerator_Reset (OTPGenerator *g)
{
    DebugObject_Access(&g->d_obj);
    
    // free existing work
    if (g->tw_have) {
        BThreadWork_Free(&g->tw);
        g->tw_have = 0;
    }
    
    g->position = g->num_otps;
}

otp_t OTPGenerator_GetOTP (OTPGenerator *g)
{
    ASSERT(g->position < g->num_otps)
    DebugObject_Access(&g->d_obj);
    
    return g->otps[g->cur_calc][g->position++];
}
