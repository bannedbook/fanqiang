/**
 * @file NCDConfigTokenizer.h
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

#ifndef BADVPN_NCDCONFIG_NCDCONFIGTOKENIZER_H
#define BADVPN_NCDCONFIG_NCDCONFIGTOKENIZER_H

#include <stdlib.h>

#include <misc/memref.h>

#define NCD_ERROR -1
#define NCD_EOF 0
#define NCD_TOKEN_CURLY_OPEN 1
#define NCD_TOKEN_CURLY_CLOSE 2
#define NCD_TOKEN_ROUND_OPEN 3
#define NCD_TOKEN_ROUND_CLOSE 4
#define NCD_TOKEN_SEMICOLON 5
#define NCD_TOKEN_DOT 6
#define NCD_TOKEN_COMMA 7
#define NCD_TOKEN_PROCESS 8
#define NCD_TOKEN_NAME 9
#define NCD_TOKEN_STRING 10
#define NCD_TOKEN_ARROW 11
#define NCD_TOKEN_TEMPLATE 12
#define NCD_TOKEN_COLON 13
#define NCD_TOKEN_BRACKET_OPEN 14
#define NCD_TOKEN_BRACKET_CLOSE 15
#define NCD_TOKEN_IF 16
#define NCD_TOKEN_ELIF 17
#define NCD_TOKEN_ELSE 18
#define NCD_TOKEN_FOREACH 19
#define NCD_TOKEN_AS 20
#define NCD_TOKEN_INCLUDE 21
#define NCD_TOKEN_INCLUDE_GUARD 22
#define NCD_TOKEN_AT 23
#define NCD_TOKEN_BLOCK 24
#define NCD_TOKEN_CARET 25
#define NCD_TOKEN_DO 26
#define NCD_TOKEN_INTERRUPT 27

typedef int (*NCDConfigTokenizer_output) (void *user, int token, char *value, size_t value_len, size_t line, size_t line_char);

void NCDConfigTokenizer_Tokenize (MemRef str, NCDConfigTokenizer_output output, void *user);

#endif
