/**
 * @file bencryption_bench.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#include <misc/balloc.h>
#include <security/BRandom.h>
#include <security/BEncryption.h>
#include <base/DebugObject.h>

static void usage (char *name)
{
    printf(
        "Usage: %s <enc/dec> <ciper> <num_blocks> <num_ops>\n"
        "    <cipher> is one of (blowfish, aes).\n",
        name
    );
    
    exit(1);
}

int main (int argc, char **argv)
{
    if (argc <= 0) {
        return 1;
    }
    
    if (argc != 5) {
        usage(argv[0]);
    }
    
    char *mode_str = argv[1];
    char *cipher_str = argv[2];
    
    int mode;
    int cipher = 0; // silence warning
    int num_blocks = atoi(argv[3]);
    int num_ops = atoi(argv[4]);
    
    if (!strcmp(mode_str, "enc")) {
        mode = BENCRYPTION_MODE_ENCRYPT;
    }
    else if (!strcmp(mode_str, "dec")) {
        mode = BENCRYPTION_MODE_DECRYPT;
    }
    else {
        usage(argv[0]);
    }
    
    if (!strcmp(cipher_str, "blowfish")) {
        cipher = BENCRYPTION_CIPHER_BLOWFISH;
    }
    else if (!strcmp(cipher_str, "aes")) {
        cipher = BENCRYPTION_CIPHER_AES;
    }
    else {
        usage(argv[0]);
    }
    
    if (num_blocks < 0 || num_ops < 0) {
        usage(argv[0]);
    }
    
    int key_size = BEncryption_cipher_key_size(cipher);
    int block_size = BEncryption_cipher_block_size(cipher);
    
    uint8_t key[BENCRYPTION_MAX_KEY_SIZE];
    BRandom_randomize(key, key_size);
    
    uint8_t iv[BENCRYPTION_MAX_BLOCK_SIZE];
    BRandom_randomize(iv, block_size);
    
    if (num_blocks > INT_MAX / block_size) {
        printf("too much");
        goto fail0;
    }
    int unit_size = num_blocks * block_size;
    
    printf("unit size %d\n", unit_size);
    
    uint8_t *buf1 = (uint8_t *)BAlloc(unit_size);
    if (!buf1) {
        printf("BAlloc failed");
        goto fail0;
    }
    
    uint8_t *buf2 = (uint8_t *)BAlloc(unit_size);
    if (!buf2) {
        printf("BAlloc failed");
        goto fail1;
    }
    
    BEncryption enc;
    BEncryption_Init(&enc, mode, cipher, key);
    
    uint8_t *in = buf1;
    uint8_t *out = buf2;
    BRandom_randomize(in, unit_size);
    
    for (int i = 0; i < num_ops; i++) {
        BEncryption_Encrypt(&enc, in, out, unit_size, iv);
        
        uint8_t *t = in;
        in = out;
        out = t;
    }
    
    BEncryption_Free(&enc);
    BFree(buf2);
fail1:
    BFree(buf1);
fail0:
    DebugObjectGlobal_Finish();
    
    return 0;
}
