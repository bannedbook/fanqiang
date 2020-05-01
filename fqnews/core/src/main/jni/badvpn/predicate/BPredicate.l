/**
 * @file BPredicate.l
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
 * {@link BPredicate} lexer file.
 */

%{

#include <string.h>
#include <stdlib.h>

#include <predicate/LexMemoryBufferInput.h>
#include <predicate/BPredicate_internal.h>

#include <generated/bison_BPredicate.h>

#define YY_INPUT(buffer, res, max_size) \
    int bytes_read = LexMemoryBufferInput_Read((LexMemoryBufferInput *)yyget_extra(yyscanner), buffer, max_size); \
    res = (bytes_read == 0 ? YY_NULL : bytes_read);

%}

%option reentrant stack noyywrap bison-bridge bison-locations never-interactive nounistd

%%
\(              return SPAR;
\)              return EPAR;
,               return COMMA;
AND             return AND;
OR              return OR;
NOT             return NOT;
true            return CONSTANT_TRUE;
false           return CONSTANT_FALSE;
[a-zA-Z0-9_]+   {
                    int l = strlen(yytext);
                    char *p = (char *)malloc(l + 1);
                    if (p) {
                        memcpy(p, yytext, l);
                        p[l] = '\0';
                    }
                    yylval->text = p;
                    return NAME;
                }
\"[^\"]*\"      {
                    int l = strlen(yytext);
                    char *p = (char *)malloc(l - 1);
                    if (p) {
                        memcpy(p, yytext + 1, l - 2);
                        p[l - 2] = '\0';
                    }
                    yylval->text = p;
                    return STRING;
                }
[ \t\n]+        ;
.               LexMemoryBufferInput_SetError((LexMemoryBufferInput *)yyget_extra(yyscanner)); return 0; // remember failure and report EOF
%%
