/**
 * @file Utf8Encoder.h
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

#ifndef BADVPN_UTF8ENCODER_H
#define BADVPN_UTF8ENCODER_H

#include <stdint.h>

/**
 * Encodes a Unicode character into a sequence of bytes according to UTF-8.
 * 
 * @param ch Unicode character to encode
 * @param out will receive the encoded bytes. Must have space for 4 bytes.
 * @return number of bytes written, 0-4, with 0 meaning the character cannot
 *         be encoded
 */
static int Utf8Encoder_EncodeCharacter (uint32_t ch, uint8_t *out);

int Utf8Encoder_EncodeCharacter (uint32_t ch, uint8_t *out)
{
    if (ch <= UINT32_C(0x007F)) {
        out[0] = ch;
        return 1;
    }
    
    if (ch <= UINT32_C(0x07FF)) {
        out[0] = (0xC0 | (ch >> 6));
        out[1] = (0x80 | ((ch >> 0) & 0x3F));
        return 2;
    }
    
    if (ch <= UINT32_C(0xFFFF)) {
        // surrogates
        if (ch >= UINT32_C(0xD800) && ch <= UINT32_C(0xDFFF)) {
            return 0;
        }
        
        out[0] = (0xE0 | (ch >> 12));
        out[1] = (0x80 | ((ch >> 6) & 0x3F));
        out[2] = (0x80 | ((ch >> 0) & 0x3F));
        return 3;
    }
    
    if (ch < UINT32_C(0x10FFFF)) {
        out[0] = (0xF0 | (ch >> 18));
        out[1] = (0x80 | ((ch >> 12) & 0x3F));
        out[2] = (0x80 | ((ch >> 6) & 0x3F));
        out[3] = (0x80 | ((ch >> 0) & 0x3F));
        return 4;
    }
    
    return 0;
}

#endif
