/**
 * @file LexMemoryBufferInput.h
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
 * Object that can be used by a lexer to read input from a memory buffer.
 */

#ifndef BADVPN_PREDICATE_LEXMEMORYBUFFERINPUT_H
#define BADVPN_PREDICATE_LEXMEMORYBUFFERINPUT_H

#include <string.h>

#include <misc/debug.h>

typedef struct {
    char *buf;
    int len;
    int pos;
    int error;
} LexMemoryBufferInput;

static void LexMemoryBufferInput_Init (LexMemoryBufferInput *input, char *buf, int len)
{
    input->buf = buf;
    input->len = len;
    input->pos = 0;
    input->error = 0;
}

static int LexMemoryBufferInput_Read (LexMemoryBufferInput *input, char *dest, int len)
{
    ASSERT(dest)
    ASSERT(len > 0)
    
    if (input->pos >= input->len) {
        return 0;
    }
    
    int to_read = input->len - input->pos;
    if (to_read > len) {
        to_read = len;
    }
    
    memcpy(dest, input->buf + input->pos, to_read);
    input->pos += to_read;
    
    return to_read;
}

static void LexMemoryBufferInput_SetError (LexMemoryBufferInput *input)
{
    input->error = 1;
}

static int LexMemoryBufferInput_HasError (LexMemoryBufferInput *input)
{
    return input->error;
}

#endif
