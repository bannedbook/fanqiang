/**
 * @file Utf16Encoder.h
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

#ifndef BADVPN_UTF16ENCODER_H
#define BADVPN_UTF16ENCODER_H

#include <stdint.h>

/**
 * Encodes a Unicode character into a sequence of 16-bit values according to UTF-16.
 * 
 * @param ch Unicode character to encode
 * @param out will receive the encoded 16-bit values. Must have space for 2 values.
 * @return number of 16-bit values written, 0-2, with 0 meaning the character cannot
 *         be encoded
 */
static int Utf16Encoder_EncodeCharacter (uint32_t ch, uint16_t *out);

int Utf16Encoder_EncodeCharacter (uint32_t ch, uint16_t *out)
{
    if (ch <= UINT32_C(0xFFFF)) {
        // surrogates
        if (ch >= UINT32_C(0xD800) && ch <= UINT32_C(0xDFFF)) {
            return 0;
        }
        
        out[0] = ch;
        return 1;
    }
    
    if (ch <= UINT32_C(0x10FFFF)) {
        uint32_t x = ch - UINT32_C(0x10000);
        out[0] = UINT32_C(0xD800) + (x >> 10);
        out[1] = UINT32_C(0xDC00) + (x & UINT32_C(0x3FF));
        return 2;
    }
    
    return 0;
}

#endif
