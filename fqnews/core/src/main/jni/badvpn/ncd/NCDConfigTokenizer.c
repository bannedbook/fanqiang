/**
 * @file NCDConfigTokenizer.c
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

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include <misc/debug.h>
#include <misc/string_begins_with.h>
#include <misc/balloc.h>
#include <misc/expstring.h>
#include <misc/parse_number.h>
#include <base/BLog.h>

#include <ncd/NCDConfigTokenizer.h>

#include <generated/blog_channel_NCDConfigTokenizer.h>

static int is_name_char (char c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
}

static int is_name_first_char (char c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_');
}

static int is_space_char (char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static int string_equals (char *str, int str_len, char *needle)
{
    return (str_len == strlen(needle) && !memcmp(str, needle, str_len));
}

void NCDConfigTokenizer_Tokenize (MemRef the_str, NCDConfigTokenizer_output output, void *user)
{
    char const *str = the_str.ptr;
    size_t left = the_str.len;
    
    size_t line = 1;
    size_t line_char = 1;
    
    while (left > 0) {
        size_t l;
        int error = 0;
        int token;
        void *token_val = NULL;
        size_t token_len = 0;
        
        if (*str == '#') {
            l = 1;
            while (l < left && str[l] != '\n') {
                l++;
            }
            token = 0;
        }
        else if (l = data_begins_with(str, left, "{")) {
            token = NCD_TOKEN_CURLY_OPEN;
        }
        else if (l = data_begins_with(str, left, "}")) {
            token = NCD_TOKEN_CURLY_CLOSE;
        }
        else if (l = data_begins_with(str, left, "(")) {
            token = NCD_TOKEN_ROUND_OPEN;
        }
        else if (l = data_begins_with(str, left, ")")) {
            token = NCD_TOKEN_ROUND_CLOSE;
        }
        else if (l = data_begins_with(str, left, ";")) {
            token = NCD_TOKEN_SEMICOLON;
        }
        else if (l = data_begins_with(str, left, ".")) {
            token = NCD_TOKEN_DOT;
        }
        else if (l = data_begins_with(str, left, ",")) {
            token = NCD_TOKEN_COMMA;
        }
        else if (l = data_begins_with(str, left, ":")) {
            token = NCD_TOKEN_COLON;
        }
        else if (l = data_begins_with(str, left, "[")) {
            token = NCD_TOKEN_BRACKET_OPEN;
        }
        else if (l = data_begins_with(str, left, "]")) {
            token = NCD_TOKEN_BRACKET_CLOSE;
        }
        else if (l = data_begins_with(str, left, "@")) {
            token = NCD_TOKEN_AT;
        }
        else if (l = data_begins_with(str, left, "^")) {
            token = NCD_TOKEN_CARET;
        }
        else if (l = data_begins_with(str, left, "->")) {
            token = NCD_TOKEN_ARROW;
        }
        else if (l = data_begins_with(str, left, "If")) {
            token = NCD_TOKEN_IF;
        }
        else if (l = data_begins_with(str, left, "Elif")) {
            token = NCD_TOKEN_ELIF;
        }
        else if (l = data_begins_with(str, left, "elif")) {
            token = NCD_TOKEN_ELIF;
        }
        else if (l = data_begins_with(str, left, "Else")) {
            token = NCD_TOKEN_ELSE;
        }
        else if (l = data_begins_with(str, left, "else")) {
            token = NCD_TOKEN_ELSE;
        }
        else if (l = data_begins_with(str, left, "Foreach")) {
            token = NCD_TOKEN_FOREACH;
        }
        else if (l = data_begins_with(str, left, "As")) {
            token = NCD_TOKEN_AS;
        }
        else if (l = data_begins_with(str, left, "Block")) {
            token = NCD_TOKEN_BLOCK;
        }
        else if (l = data_begins_with(str, left, "Do")) {
            token = NCD_TOKEN_DO;
        }
        else if (l = data_begins_with(str, left, "Interrupt")) {
            token = NCD_TOKEN_INTERRUPT;
        }
        else if (l = data_begins_with(str, left, "include_guard")) {
            token = NCD_TOKEN_INCLUDE_GUARD;
        }
        else if (l = data_begins_with(str, left, "include")) {
            token = NCD_TOKEN_INCLUDE;
        }
        else if (is_name_first_char(*str)) {
            l = 1;
            while (l < left && is_name_char(str[l])) {
                l++;
            }
            
            // allocate buffer
            bsize_t bufsize = bsize_add(bsize_fromsize(l), bsize_fromint(1));
            char *buf;
            if (bufsize.is_overflow || !(buf = malloc(bufsize.value))) {
                BLog(BLOG_ERROR, "malloc failed");
                error = 1;
                goto out;
            }
            
            // copy and terminate
            memcpy(buf, str, l);
            buf[l] = '\0';
            
            if (!strcmp(buf, "process")) {
                token = NCD_TOKEN_PROCESS;
                free(buf);
            }
            else if (!strcmp(buf, "template")) {
                token = NCD_TOKEN_TEMPLATE;
                free(buf);
            }
            else {
                token = NCD_TOKEN_NAME;
                token_val = buf;
                token_len = l;
            }
        }
        else if (*str == '"') do {
            // init string
            ExpString estr;
            if (!ExpString_Init(&estr)) {
                BLog(BLOG_ERROR, "ExpString_Init failed");
                goto string_fail0;
            }
            
            // skip start quote
            l = 1;
            
            // decode string
            while (l < left) {
                uint8_t dec_ch;
                
                // get character
                if (str[l] == '\\') {
                    if (left - l < 2) {
                        BLog(BLOG_ERROR, "escape character found in string but nothing follows");
                        goto string_fail1;
                    }
                    
                    size_t extra = 0;
                    
                    switch (str[l + 1]) {
                        case '\'':
                        case '\"':
                        case '\\':
                        case '\?':
                            dec_ch = str[l + 1]; break;
                        
                        case 'a':
                            dec_ch = '\a'; break;
                        case 'b':
                            dec_ch = '\b'; break;
                        case 'f':
                            dec_ch = '\f'; break;
                        case 'n':
                            dec_ch = '\n'; break;
                        case 'r':
                            dec_ch = '\r'; break;
                        case 't':
                            dec_ch = '\t'; break;
                        case 'v':
                            dec_ch = '\v'; break;
                        
                        case '0':
                            dec_ch = 0; break;
                        
                        case 'x': {
                            if (left - l < 4) {
                                BLog(BLOG_ERROR, "hexadecimal escape found in string but too little characters follow");
                                goto string_fail1;
                            }
                            
                            uintmax_t hex_val;
                            if (!parse_unsigned_hex_integer(MemRef_Make(&str[l + 2], 2), &hex_val)) {
                                BLog(BLOG_ERROR, "hexadecimal escape found in string but two hex characters don't follow");
                                goto string_fail1;
                            }
                            
                            dec_ch = hex_val;
                            extra = 2;
                        } break;
                        
                        default:
                            BLog(BLOG_ERROR, "bad escape sequence in string");
                            goto string_fail1;
                    }
                    
                    l += 2 + extra;
                }
                else if (str[l] == '"') {
                    break;
                }
                else {
                    dec_ch = str[l];
                    l++;
                }
                
                // append character to string
                if (!ExpString_AppendByte(&estr, dec_ch)) {
                    BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                    goto string_fail1;
                }
            }
            
            // make sure ending quote was found
            if (l == left) {
                BLog(BLOG_ERROR, "missing ending quote for string");
                goto string_fail1;
            }
            
            // skip ending quote
            l++;
            
            token = NCD_TOKEN_STRING;
            token_val = ExpString_Get(&estr);
            token_len = ExpString_Length(&estr);
            break;
            
        string_fail1:
            ExpString_Free(&estr);
        string_fail0:
            error = 1;
        } while (0);
        else if (is_space_char(*str)) {
            token = 0;
            l = 1;
        }
        else {
            BLog(BLOG_ERROR, "unrecognized character");
            error = 1;
        }
        
    out:
        // report error
        if (error) {
            output(user, NCD_ERROR, NULL, 0, line, line_char);
            return;
        }
        
        // output token
        if (token) {
            if (!output(user, token, token_val, token_len, line, line_char)) {
                return;
            }
        }
        
        // update line/char counters
        for (size_t i = 0; i < l; i++) {
            if (str[i] == '\n') {
                line++;
                line_char = 1;
            } else {
                line_char++;
            }
        }
        
        str += l;
        left -= l;
    }
    
    output(user, NCD_EOF, NULL, 0, line, line_char);
}
