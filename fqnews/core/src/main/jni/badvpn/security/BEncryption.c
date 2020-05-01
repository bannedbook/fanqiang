/**
 * @file BEncryption.c
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

#include <base/BLog.h>

#include <security/BEncryption.h>

#include <generated/blog_channel_BEncryption.h>

int BEncryption_cipher_valid (int cipher)
{
    switch (cipher) {
        case BENCRYPTION_CIPHER_BLOWFISH:
        case BENCRYPTION_CIPHER_AES:
            return 1;
        default:
            return 0;
    }
}

int BEncryption_cipher_block_size (int cipher)
{
    switch (cipher) {
        case BENCRYPTION_CIPHER_BLOWFISH:
            return BENCRYPTION_CIPHER_BLOWFISH_BLOCK_SIZE;
        case BENCRYPTION_CIPHER_AES:
            return BENCRYPTION_CIPHER_AES_BLOCK_SIZE;
        default:
            ASSERT(0)
            return 0;
    }
}

int BEncryption_cipher_key_size (int cipher)
{
    switch (cipher) {
        case BENCRYPTION_CIPHER_BLOWFISH:
            return BENCRYPTION_CIPHER_BLOWFISH_KEY_SIZE;
        case BENCRYPTION_CIPHER_AES:
            return BENCRYPTION_CIPHER_AES_KEY_SIZE;
        default:
            ASSERT(0)
            return 0;
    }
}

void BEncryption_Init (BEncryption *enc, int mode, int cipher, uint8_t *key)
{
    ASSERT(!(mode&~(BENCRYPTION_MODE_ENCRYPT|BENCRYPTION_MODE_DECRYPT)))
    ASSERT((mode&BENCRYPTION_MODE_ENCRYPT) || (mode&BENCRYPTION_MODE_DECRYPT))
    
    enc->mode = mode;
    enc->cipher = cipher;
    
    #ifdef BADVPN_USE_CRYPTODEV
    
    switch (enc->cipher) {
        case BENCRYPTION_CIPHER_AES:
            enc->cryptodev.cipher = CRYPTO_AES_CBC;
            break;
        default:
            goto fail1;
    }
    
    if ((enc->cryptodev.fd = open("/dev/crypto", O_RDWR, 0)) < 0) {
        BLog(BLOG_ERROR, "failed to open /dev/crypto");
        goto fail1;
    }
    
    if (ioctl(enc->cryptodev.fd, CRIOGET, &enc->cryptodev.cfd)) {
        BLog(BLOG_ERROR, "failed ioctl(CRIOGET)");
        goto fail2;
    }
    
    struct session_op sess;
    memset(&sess, 0, sizeof(sess));
    sess.cipher = enc->cryptodev.cipher;
    sess.keylen = BEncryption_cipher_key_size(enc->cipher);
    sess.key = key;
    if (ioctl(enc->cryptodev.cfd, CIOCGSESSION, &sess)) {
        BLog(BLOG_ERROR, "failed ioctl(CIOCGSESSION)");
        goto fail3;
    }
    
    enc->cryptodev.ses = sess.ses;
    enc->use_cryptodev = 1;
    
    goto success;
    
fail3:
    ASSERT_FORCE(close(enc->cryptodev.cfd) == 0)
fail2:
    ASSERT_FORCE(close(enc->cryptodev.fd) == 0)
fail1:
    
    enc->use_cryptodev = 0;
    
    #endif
    
    int res;
    
    switch (enc->cipher) {
        case BENCRYPTION_CIPHER_BLOWFISH:
            BF_set_key(&enc->blowfish, BENCRYPTION_CIPHER_BLOWFISH_KEY_SIZE, key);
            break;
        case BENCRYPTION_CIPHER_AES:
            if (enc->mode&BENCRYPTION_MODE_ENCRYPT) {
                res = AES_set_encrypt_key(key, 128, &enc->aes.encrypt);
                ASSERT_EXECUTE(res >= 0)
            }
            if (enc->mode&BENCRYPTION_MODE_DECRYPT) {
                res = AES_set_decrypt_key(key, 128, &enc->aes.decrypt);
                ASSERT_EXECUTE(res >= 0)
            }
            break;
        default:
            ASSERT(0)
            ;
    }
    
    #ifdef BADVPN_USE_CRYPTODEV
success:
    #endif
    // init debug object
    DebugObject_Init(&enc->d_obj);
}

void BEncryption_Free (BEncryption *enc)
{
    // free debug object
    DebugObject_Free(&enc->d_obj);
    
    #ifdef BADVPN_USE_CRYPTODEV
    
    if (enc->use_cryptodev) {
        ASSERT_FORCE(ioctl(enc->cryptodev.cfd, CIOCFSESSION, &enc->cryptodev.ses) == 0)
        ASSERT_FORCE(close(enc->cryptodev.cfd) == 0)
        ASSERT_FORCE(close(enc->cryptodev.fd) == 0)
    }
    
    #endif
}

void BEncryption_Encrypt (BEncryption *enc, uint8_t *in, uint8_t *out, int len, uint8_t *iv)
{
    ASSERT(enc->mode&BENCRYPTION_MODE_ENCRYPT)
    ASSERT(len >= 0)
    ASSERT(len % BEncryption_cipher_block_size(enc->cipher) == 0)
    
    #ifdef BADVPN_USE_CRYPTODEV
    
    if (enc->use_cryptodev) {
        struct crypt_op cryp;
        memset(&cryp, 0, sizeof(cryp));
        cryp.ses = enc->cryptodev.ses;
        cryp.len = len;
        cryp.src = in;
        cryp.dst = out;
        cryp.iv = iv;
        cryp.op = COP_ENCRYPT;
        ASSERT_FORCE(ioctl(enc->cryptodev.cfd, CIOCCRYPT, &cryp) == 0)
        
        return;
    }
    
    #endif
    
    switch (enc->cipher) {
        case BENCRYPTION_CIPHER_BLOWFISH:
            BF_cbc_encrypt(in, out, len, &enc->blowfish, iv, BF_ENCRYPT);
            break;
        case BENCRYPTION_CIPHER_AES:
            AES_cbc_encrypt(in, out, len, &enc->aes.encrypt, iv, AES_ENCRYPT);
            break;
        default:
            ASSERT(0);
    }
}

void BEncryption_Decrypt (BEncryption *enc, uint8_t *in, uint8_t *out, int len, uint8_t *iv)
{
    ASSERT(enc->mode&BENCRYPTION_MODE_DECRYPT)
    ASSERT(len >= 0)
    ASSERT(len % BEncryption_cipher_block_size(enc->cipher) == 0)
    
    #ifdef BADVPN_USE_CRYPTODEV
    
    if (enc->use_cryptodev) {
        struct crypt_op cryp;
        memset(&cryp, 0, sizeof(cryp));
        cryp.ses = enc->cryptodev.ses;
        cryp.len = len;
        cryp.src = in;
        cryp.dst = out;
        cryp.iv = iv;
        cryp.op = COP_DECRYPT;
        ASSERT_FORCE(ioctl(enc->cryptodev.cfd, CIOCCRYPT, &cryp) == 0)
        
        return;
    }
    
    #endif
    
    switch (enc->cipher) {
        case BENCRYPTION_CIPHER_BLOWFISH:
            BF_cbc_encrypt(in, out, len, &enc->blowfish, iv, BF_DECRYPT);
            break;
        case BENCRYPTION_CIPHER_AES:
            AES_cbc_encrypt(in, out, len, &enc->aes.decrypt, iv, AES_DECRYPT);
            break;
        default:
            ASSERT(0);
    }
}
