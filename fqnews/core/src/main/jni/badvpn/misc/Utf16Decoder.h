/**
 * @file Utf16Decoder.h
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

#ifndef BADVPN_UTF16DECODER_H
#define BADVPN_UTF16DECODER_H

#include <stdint.h>

#include <misc/debug.h>

/**
 * Decodes UTF-16 data into Unicode characters.
 */
typedef struct {
    int cont;
    uint32_t ch;
} Utf16Decoder;

/**
 * Initializes the UTF-16 decoder.
 * 
 * @param o the object
 */
static void Utf16Decoder_Init (Utf16Decoder *o);

/**
 * Inputs a 16-bit value to the decoder.
 * 
 * @param o the object
 * @param b 16-bit value to input
 * @param out_ch will receive a Unicode character if this function returns 1.
 *               If written, the character will be in the range 0 - 0x10FFFF,
 *               excluding the surrogate range 0xD800 - 0xDFFF.
 * @return 1 if a Unicode character has been written to *out_ch, 0 if not
 */
static int Utf16Decoder_Input (Utf16Decoder *o, uint16_t b, uint32_t *out_ch);

void Utf16Decoder_Init (Utf16Decoder *o)
{
    o->cont = 0;
}

int Utf16Decoder_Input (Utf16Decoder *o, uint16_t b, uint32_t *out_ch)
{
    // high surrogate
    if (b >= UINT16_C(0xD800) && b <= UINT16_C(0xDBFF)) {
        // set continuation state
        o->cont = 1;
        
        // add high bits
        o->ch = (uint32_t)(b - UINT16_C(0xD800)) << 10;
        
        return 0;
    }
    
    // low surrogate
    if (b >= UINT16_C(0xDC00) && b <= UINT16_C(0xDFFF)) {
        // check continuation
        if (!o->cont) {
            return 0;
        }
        
        // add low bits
        o->ch |= (b - UINT16_C(0xDC00));
        
        // reset state
        o->cont = 0;
        
        // don't report surrogates
        if (o->ch >= UINT32_C(0xD800) && o->ch <= UINT32_C(0xDFFF)) {
            return 0;
        }
        
        // return character
        *out_ch = o->ch;
        return 1;
    }
    
    // reset state
    o->cont = 0;
    
    // return character
    *out_ch = b;
    return 1;
}

#endif
