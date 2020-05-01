/**
 * @file BEncryption.h
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
 * Block cipher encryption abstraction.
 */

#ifndef BADVPN_SECURITY_BENCRYPTION_H
#define BADVPN_SECURITY_BENCRYPTION_H

#include <stdint.h>
#include <string.h>

#ifdef BADVPN_USE_CRYPTODEV
#include <crypto/cryptodev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#endif

#include <openssl/blowfish.h>
#include <openssl/aes.h>

#include <misc/debug.h>
#include <base/DebugObject.h>

#define BENCRYPTION_MODE_ENCRYPT 1
#define BENCRYPTION_MODE_DECRYPT 2

#define BENCRYPTION_MAX_BLOCK_SIZE 16
#define BENCRYPTION_MAX_KEY_SIZE 16

#define BENCRYPTION_CIPHER_BLOWFISH 1
#define BENCRYPTION_CIPHER_BLOWFISH_BLOCK_SIZE 8
#define BENCRYPTION_CIPHER_BLOWFISH_KEY_SIZE 16

#define BENCRYPTION_CIPHER_AES 2
#define BENCRYPTION_CIPHER_AES_BLOCK_SIZE 16
#define BENCRYPTION_CIPHER_AES_KEY_SIZE 16

// NOTE: update the maximums above when adding a cipher!

/**
 * Block cipher encryption abstraction.
 */
typedef struct {
    DebugObject d_obj;
    int mode;
    int cipher;
    #ifdef BADVPN_USE_CRYPTODEV
    int use_cryptodev;
    #endif
    union {
        BF_KEY blowfish;
        struct {
            AES_KEY encrypt;
            AES_KEY decrypt;
        } aes;
        #ifdef BADVPN_USE_CRYPTODEV
        struct {
            int fd;
            int cfd;
            int cipher;
            uint32_t ses;
        } cryptodev;
        #endif
    };
} BEncryption;

/**
 * Checks if the given cipher number is valid.
 * 
 * @param cipher cipher number
 * @return 1 if valid, 0 if not
 */
int BEncryption_cipher_valid (int cipher);

/**
 * Returns the block size of a cipher.
 * 
 * @param cipher cipher number. Must be valid.
 * @return block size in bytes
 */
int BEncryption_cipher_block_size (int cipher);

/**
 * Returns the key size of a cipher.
 * 
 * @param cipher cipher number. Must be valid.
 * @return key size in bytes
 */
int BEncryption_cipher_key_size (int cipher);

/**
 * Initializes the object.
 * {@link BSecurity_GlobalInitThreadSafe} must have been done if this object
 * will be used from a non-main thread.
 * 
 * @param enc the object
 * @param mode whether encryption or decryption is to be done, or both.
 *             Must be a bitwise-OR of at least one of BENCRYPTION_MODE_ENCRYPT
 *             and BENCRYPTION_MODE_DECRYPT.
 * @param cipher cipher number. Must be valid.
 * @param key encryption key
 */
void BEncryption_Init (BEncryption *enc, int mode, int cipher, uint8_t *key);

/**
 * Frees the object.
 * 
 * @param enc the object
 */
void BEncryption_Free (BEncryption *enc);

/**
 * Encrypts data.
 * The object must have been initialized with mode including
 * BENCRYPTION_MODE_ENCRYPT.
 * 
 * @param enc the object
 * @param in data to encrypt
 * @param out ciphertext output
 * @param len number of bytes to encrypt. Must be >=0 and a multiple of
 *            block size.
 * @param iv initialization vector. Updated such that continuing a previous encryption
 *           starting with the updated IV is equivalent to performing just one encryption.
 */
void BEncryption_Encrypt (BEncryption *enc, uint8_t *in, uint8_t *out, int len, uint8_t *iv);

/**
 * Decrypts data.
 * The object must have been initialized with mode including
 * BENCRYPTION_MODE_DECRYPT.
 * 
 * @param enc the object
 * @param in data to decrypt
 * @param out plaintext output
 * @param len number of bytes to decrypt. Must be >=0 and a multiple of
 *            block size.
 * @param iv initialization vector. Updated such that continuing a previous decryption
 *           starting with the updated IV is equivalent to performing just one decryption.
 */
void BEncryption_Decrypt (BEncryption *enc, uint8_t *in, uint8_t *out, int len, uint8_t *iv);

#endif
