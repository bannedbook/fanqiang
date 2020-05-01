/**
 * @file unicode_funcs.h
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

#ifndef BADVPN_UNICODE_FUNCS_H
#define BADVPN_UNICODE_FUNCS_H

#include <misc/expstring.h>
#include <misc/bsize.h>
#include <misc/Utf8Encoder.h>
#include <misc/Utf8Decoder.h>
#include <misc/Utf16Encoder.h>
#include <misc/Utf16Decoder.h>

/**
 * Decodes UTF-16 data as bytes into an allocated null-terminated UTF-8 string.
 * 
 * @param data UTF-16 data, in big endian
 * @param data_len size of data in bytes
 * @param out_is_error if not NULL and the function returns a string,
 *                     *out_is_error will be set to 0 or 1, indicating
 *                     whether there have been errors decoding the input.
 *                     A null decoded character is treated as an error.
 * @return An UTF-8 null-terminated string which can be freed with free(),
 *         or NULL if out of memory.
 */
static char * unicode_decode_utf16_to_utf8 (const uint8_t *data, size_t data_len, int *out_is_error);

/**
 * Decodes UTF-8 data into UTF-16 data as bytes.
 * 
 * @param data UTF-8 data
 * @param data_len size of data in bytes
 * @param out output buffer
 * @param out_avail number of bytes available in output buffer
 * @param out_len if not NULL, *out_len will contain the number of bytes
 *                required to store the resulting data (or overflow)
 * @param out_is_error if not NULL, *out_is_error will contain 0 or 1,
 *                     indicating whether there have been errors decoding
 *                     the input
 */
static void unicode_decode_utf8_to_utf16 (const uint8_t *data, size_t data_len, uint8_t *out, size_t out_avail, bsize_t *out_len, int *out_is_error);

static char * unicode_decode_utf16_to_utf8 (const uint8_t *data, size_t data_len, int *out_is_error)
{
    // will build the resulting UTF-8 string by appending to ExpString
    ExpString str;
    if (!ExpString_Init(&str)) {
        goto fail0;
    }
    
    // init UTF-16 decoder
    Utf16Decoder decoder;
    Utf16Decoder_Init(&decoder);
    
    // set initial input and input matching positions
    size_t i_in = 0;
    size_t i_ch = 0;
    
    int error = 0;
    
    while (i_in < data_len) {
        // read two input bytes from the input position
        uint8_t x = data[i_in++];
        if (i_in == data_len) {
            break;
        }
        uint8_t y = data[i_in++];
        
        // combine them into a 16-bit value
        uint16_t xy = (((uint16_t)x << 8) | (uint16_t)y);
        
        // give the 16-bit value to the UTF-16 decoder and maybe
        // receive a Unicode character back
        uint32_t ch;
        if (!Utf16Decoder_Input(&decoder, xy, &ch)) {
            continue;
        }
        
        if (!error) {
            // encode the Unicode character back into UTF-16
            uint16_t chenc[2];
            int chenc_n = Utf16Encoder_EncodeCharacter(ch, chenc);
            ASSERT(chenc_n > 0)
            
            // match the result with input
            for (int chenc_i = 0; chenc_i < chenc_n; chenc_i++) {
                uint8_t cx = (chenc[chenc_i] >> 8);
                uint8_t cy = (chenc[chenc_i] & 0xFF);
                
                if (i_ch >= data_len || data[i_ch] != cx) {
                    error = 1;
                    break;
                }
                i_ch++;
                
                if (i_ch >= data_len || data[i_ch] != cy) {
                    error = 1;
                    break;
                }
                i_ch++;
            }
        }
        
        // we don't like null Unicode characters because we're building a
        // null-terminated UTF-8 string
        if (ch == 0) {
            error = 1;
            continue;
        }
        
        // encode the Unicode character into UTF-8
        uint8_t enc[5];
        int enc_n = Utf8Encoder_EncodeCharacter(ch, enc);
        ASSERT(enc_n > 0)
        
        // append the resulting UTF-8 bytes to the result string
        enc[enc_n] = 0;
        if (!ExpString_Append(&str, enc)) {
            goto fail1;
        }
    }
    
    // check if we matched the whole input string when encoding back
    if (i_ch < data_len) {
        error = 1;
    }
    
    if (out_is_error) {
        *out_is_error = error;
    }
    return ExpString_Get(&str);
    
fail1:
    ExpString_Free(&str);
fail0:
    return NULL;
}

static void unicode_decode_utf8_to_utf16 (const uint8_t *data, size_t data_len, uint8_t *out, size_t out_avail, bsize_t *out_len, int *out_is_error)
{
    Utf8Decoder decoder;
    Utf8Decoder_Init(&decoder);
    
    size_t i_in = 0;
    size_t i_ch = 0;
    
    bsize_t len = bsize_fromsize(0);
    
    int error = 0;
    
    while (i_in < data_len) {
        uint8_t x = data[i_in++];
        
        uint32_t ch;
        if (!Utf8Decoder_Input(&decoder, x, &ch)) {
            continue;
        }
        
        if (!error) {
            uint8_t chenc[4];
            int chenc_n = Utf8Encoder_EncodeCharacter(ch, chenc);
            ASSERT(chenc_n > 0)
            
            for (int chenc_i = 0; chenc_i < chenc_n; chenc_i++) {
                if (i_ch >= data_len || data[i_ch] != chenc[chenc_i]) {
                    error = 1;
                    break;
                }
                i_ch++;
            }
        }
        
        uint16_t enc[2];
        int enc_n = Utf16Encoder_EncodeCharacter(ch, enc);
        ASSERT(enc_n > 0)
        
        len = bsize_add(len, bsize_fromsize(2 * enc_n));
        
        for (int enc_i = 0; enc_i < enc_n; enc_i++) {
            if (out_avail == 0) {
                break;
            }
            *(out++) = (enc[enc_i] >> 8);
            out_avail--;
            
            if (out_avail == 0) {
                break;
            }
            *(out++) = (enc[enc_i] & 0xFF);
            out_avail--;
        }
    }
    
    if (i_ch < data_len) {
        error = 1;
    }
    
    if (out_len) {
        *out_len = len;
    }
    if (out_is_error) {
        *out_is_error = error;
    }
}

#endif
