/**
 * @file NCDValParser.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <base/BLog.h>
#include <ncd/NCDConfigTokenizer.h>
#include <ncd/NCDValCons.h>

#include "NCDValParser.h"

#include <generated/blog_channel_NCDValParser.h>

struct token {
    char *str;
    size_t len;
};

struct value {
    int have;
    NCDValConsVal v;
};

#define ERROR_FLAG_MEMORY        (1 << 0)
#define ERROR_FLAG_TOKENIZATION  (1 << 1)
#define ERROR_FLAG_SYNTAX        (1 << 2)
#define ERROR_FLAG_DUPLICATE_KEY (1 << 3)
#define ERROR_FLAG_DEPTH         (1 << 4)

struct parser_state {
    NCDValCons cons;
    NCDValRef value;
    int cons_error;
    int error_flags;
    void *parser;
};

static void free_token (struct token o)
{
    if (o.str) {
        free(o.str);
    }
};

static void handle_cons_error (struct parser_state *state)
{
    switch (state->cons_error) {
        case NCDVALCONS_ERROR_MEMORY:
            state->error_flags |= ERROR_FLAG_MEMORY;
            break;
        case NCDVALCONS_ERROR_DUPLICATE_KEY:
            state->error_flags |= ERROR_FLAG_DUPLICATE_KEY;
            break;
        case NCDVALCONS_ERROR_DEPTH:
            state->error_flags |= ERROR_FLAG_DEPTH;
            break;
        default:
            ASSERT(0);
    }
}

// rename non-static functions defined by our Lemon parser
// to avoid clashes with other Lemon parsers
#define ParseTrace ParseTrace_NCDValParser
#define ParseAlloc ParseAlloc_NCDValParser
#define ParseFree ParseFree_NCDValParser
#define Parse Parse_NCDValParser

// include the generated Lemon parser
#include "../generated/NCDValParser_parse.c"
#include "../generated/NCDValParser_parse.h"

static int tokenizer_output (void *user, int token, char *value, size_t value_len, size_t line, size_t line_char)
{
    struct parser_state *state = user;
    ASSERT(!state->error_flags)
    
    if (token == NCD_ERROR) {
        state->error_flags |= ERROR_FLAG_TOKENIZATION;
        goto fail;
    }
    
    struct token minor;
    minor.str = value;
    minor.len = value_len;
    
    switch (token) {
        case NCD_EOF: {
            Parse(state->parser, 0, minor, state);
        } break;
        
        case NCD_TOKEN_CURLY_OPEN: {
            Parse(state->parser, CURLY_OPEN, minor, state);
        } break;
        
        case NCD_TOKEN_CURLY_CLOSE: {
            Parse(state->parser, CURLY_CLOSE, minor, state);
        } break;
        
        case NCD_TOKEN_COMMA: {
            Parse(state->parser, COMMA, minor, state);
        } break;
        
        case NCD_TOKEN_STRING: {
            Parse(state->parser, STRING, minor, state);
        } break;
        
        case NCD_TOKEN_COLON: {
            Parse(state->parser, COLON, minor, state);
        } break;
        
        case NCD_TOKEN_BRACKET_OPEN: {
            Parse(state->parser, BRACKET_OPEN, minor, state);
        } break;
        
        case NCD_TOKEN_BRACKET_CLOSE: {
            Parse(state->parser, BRACKET_CLOSE, minor, state);
        } break;
        
        default:
            state->error_flags |= ERROR_FLAG_TOKENIZATION;
            free_token(minor);
            goto fail;
    }
    
    if (state->error_flags) {
        goto fail;
    }
    
    return 1;
    
fail:
    ASSERT(state->error_flags)
    
    if ((state->error_flags & ERROR_FLAG_MEMORY)) {
        BLog(BLOG_ERROR, "line %zu, character %zu: memory allocation error", line, line_char);
    }
    
    if ((state->error_flags & ERROR_FLAG_TOKENIZATION)) {
        BLog(BLOG_ERROR, "line %zu, character %zu: tokenization error", line, line_char);
    }
    
    if ((state->error_flags & ERROR_FLAG_SYNTAX)) {
        BLog(BLOG_ERROR, "line %zu, character %zu: syntax error", line, line_char);
    }
    
    if ((state->error_flags & ERROR_FLAG_DUPLICATE_KEY)) {
        BLog(BLOG_ERROR, "line %zu, character %zu: duplicate key in map error", line, line_char);
    }
    
    if ((state->error_flags & ERROR_FLAG_DEPTH)) {
        BLog(BLOG_ERROR, "line %zu, character %zu: depth limit exceeded", line, line_char);
    }
    
    return 0;
}

int NCDValParser_Parse (MemRef str, NCDValMem *mem, NCDValRef *out_value)
{
    ASSERT(str.len == 0 || str.ptr)
    ASSERT(mem)
    ASSERT(out_value)
    
    int ret = 0;
    
    struct parser_state state;
    state.value = NCDVal_NewInvalid();
    state.error_flags = 0;
    
    if (!NCDValCons_Init(&state.cons, mem)) {
        BLog(BLOG_ERROR, "NCDValCons_Init failed");
        goto fail0;
    }
    
    if (!(state.parser = ParseAlloc(malloc))) {
        BLog(BLOG_ERROR, "ParseAlloc failed");
        goto fail1;
    }
    
    NCDConfigTokenizer_Tokenize(str, tokenizer_output, &state);
    
    ParseFree(state.parser, free);
    
    if (state.error_flags) {
        goto fail1;
    }
    
    ASSERT(!NCDVal_IsInvalid(state.value))
    
    *out_value = state.value;
    ret = 1;
    
fail1:
    NCDValCons_Free(&state.cons);
fail0:
    return ret;
}
