/**
 * @file ncd_tokenizer_test.c
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <misc/debug.h>
#include <base/BLog.h>
#include <ncd/NCDConfigTokenizer.h>

int error;

static int tokenizer_output (void *user, int token, char *value, size_t value_len, size_t line, size_t line_char)
{
    if (token == NCD_ERROR) {
        printf("line %zu, character %zu: tokenizer error\n", line, line_char);
        error = 1;
        return 0;
    }
    
    switch (token) {
        case NCD_EOF:
            printf("eof\n");
            break;
        case NCD_TOKEN_CURLY_OPEN:
            printf("curly_open\n");
            break;
        case NCD_TOKEN_CURLY_CLOSE:
            printf("curly_close\n");
            break;
        case NCD_TOKEN_ROUND_OPEN:
            printf("round_open\n");
            break;
        case NCD_TOKEN_ROUND_CLOSE:
            printf("round_close\n");
            break;
        case NCD_TOKEN_SEMICOLON:
            printf("semicolon\n");
            break;
        case NCD_TOKEN_DOT:
            printf("dot\n");
            break;
        case NCD_TOKEN_COMMA:
            printf("comma\n");
            break;
        case NCD_TOKEN_PROCESS:
            printf("process\n");
            break;
        case NCD_TOKEN_NAME:
            printf("name %s\n", value);
            free(value);
            break;
        case NCD_TOKEN_STRING:
            printf("string %s\n", value);
            free(value);
            break;
        case NCD_TOKEN_ARROW:
            printf("arrow\n");
            break;
        case NCD_TOKEN_TEMPLATE:
            printf("template\n");
            break;
        case NCD_TOKEN_COLON:
            printf("colon\n");
            break;
        case NCD_TOKEN_BRACKET_OPEN:
            printf("bracket open\n");
            break;
        case NCD_TOKEN_BRACKET_CLOSE:
            printf("bracket close\n");
            break;
        case NCD_TOKEN_IF:
            printf("if\n");
            break;
        case NCD_TOKEN_ELIF:
            printf("elif\n");
            break;
        case NCD_TOKEN_ELSE:
            printf("else\n");
            break;
        case NCD_TOKEN_FOREACH:
            printf("foreach\n");
            break;
        case NCD_TOKEN_AS:
            printf("as\n");
            break;
        case NCD_TOKEN_INCLUDE:
            printf("include\n");
            break;
        case NCD_TOKEN_INCLUDE_GUARD:
            printf("include_guard\n");
            break;
        case NCD_TOKEN_AT:
            printf("at\n");
            break;
        case NCD_TOKEN_BLOCK:
            printf("block\n");
            break;
        case NCD_TOKEN_CARET:
            printf("caret\n");
            break;
        case NCD_TOKEN_DO:
            printf("do\n");
            break;
        case NCD_TOKEN_INTERRUPT:
            printf("interrupt\n");
            break;
        default:
            printf("UNKNOWN_TOKEN\n");
            break;
    }
    
    return 1;
}

int main (int argc, char **argv)
{
    if (argc < 1) {
        return 1;
    }
    
    if (argc != 2) {
        printf("Usage: %s <string>\n", argv[0]);
        return 1;
    }
    
    BLog_InitStdout();
    
    error = 0;
    
    NCDConfigTokenizer_Tokenize(MemRef_MakeCstr(argv[1]), tokenizer_output, NULL);
    
    if (error) {
        return 1;
    }
    
    return 0;
}
