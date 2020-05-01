/**
 * @file NCDConfigParser.c
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

#include <misc/debug.h>
#include <base/BLog.h>
#include <ncd/NCDConfigTokenizer.h>

#include "ncd/NCDConfigParser.h"

#include <generated/blog_channel_NCDConfigParser.h>

#include "../generated/NCDConfigParser_parse.c"
#include "../generated/NCDConfigParser_parse.h"

struct parser_state {
    struct parser_out out;
    int error;
    void *parser;
};

static int tokenizer_output (void *user, int token, char *value, size_t value_len, size_t line, size_t line_char)
{
    struct parser_state *state = user;
    ASSERT(!state->out.out_of_memory)
    ASSERT(!state->out.syntax_error)
    ASSERT(!state->error)
    
    if (token == NCD_ERROR) {
        BLog(BLOG_ERROR, "line %zu, character %zu: tokenizer error", line, line_char);
        state->error = 1;
        return 0;
    }
    
    struct token minor;
    minor.str = value;
    minor.len = value_len;
    
    switch (token) {
        case NCD_EOF: {
            Parse(state->parser, 0, minor, &state->out);
        } break;
        
        case NCD_TOKEN_CURLY_OPEN: {
            Parse(state->parser, CURLY_OPEN, minor, &state->out);
        } break;
        
        case NCD_TOKEN_CURLY_CLOSE: {
            Parse(state->parser, CURLY_CLOSE, minor, &state->out);
        } break;
        
        case NCD_TOKEN_ROUND_OPEN: {
            Parse(state->parser, ROUND_OPEN, minor, &state->out);
        } break;
        
        case NCD_TOKEN_ROUND_CLOSE: {
            Parse(state->parser, ROUND_CLOSE, minor, &state->out);
        } break;
        
        case NCD_TOKEN_SEMICOLON: {
            Parse(state->parser, SEMICOLON, minor, &state->out);
        } break;
        
        case NCD_TOKEN_DOT: {
            Parse(state->parser, DOT, minor, &state->out);
        } break;
        
        case NCD_TOKEN_COMMA: {
            Parse(state->parser, COMMA, minor, &state->out);
        } break;
        
        case NCD_TOKEN_ARROW: {
            Parse(state->parser, ARROW, minor, &state->out);
        } break;
        
        case NCD_TOKEN_PROCESS: {
            Parse(state->parser, PROCESS, minor, &state->out);
        } break;
        
        case NCD_TOKEN_TEMPLATE: {
            Parse(state->parser, TEMPLATE, minor, &state->out);
        } break;
        
        case NCD_TOKEN_NAME: {
            Parse(state->parser, NAME, minor, &state->out);
        } break;
        
        case NCD_TOKEN_STRING: {
            Parse(state->parser, STRING, minor, &state->out);
        } break;
        
        case NCD_TOKEN_COLON: {
            Parse(state->parser, COLON, minor, &state->out);
        } break;
        
        case NCD_TOKEN_BRACKET_OPEN: {
            Parse(state->parser, BRACKET_OPEN, minor, &state->out);
        } break;
        
        case NCD_TOKEN_BRACKET_CLOSE: {
            Parse(state->parser, BRACKET_CLOSE, minor, &state->out);
        } break;
        
        case NCD_TOKEN_IF: {
            Parse(state->parser, IF, minor, &state->out);
        } break;
        
        case NCD_TOKEN_ELIF: {
            Parse(state->parser, ELIF, minor, &state->out);
        } break;
        
        case NCD_TOKEN_ELSE: {
            Parse(state->parser, ELSE, minor, &state->out);
        } break;
        
        case NCD_TOKEN_FOREACH: {
            Parse(state->parser, FOREACH, minor, &state->out);
        } break;
        
        case NCD_TOKEN_AS: {
            Parse(state->parser, AS, minor, &state->out);
        } break;
        
        case NCD_TOKEN_INCLUDE: {
            Parse(state->parser, INCLUDE, minor, &state->out);
        } break;
        
        case NCD_TOKEN_INCLUDE_GUARD: {
            Parse(state->parser, INCLUDE_GUARD, minor, &state->out);
        } break;
        
        case NCD_TOKEN_AT: {
            Parse(state->parser, AT_SIGN, minor, &state->out);
        } break;
        
        case NCD_TOKEN_BLOCK: {
            Parse(state->parser, BLOCK, minor, &state->out);
        } break;
        
        case NCD_TOKEN_CARET: {
            Parse(state->parser, CARET, minor, &state->out);
        } break;
        
        case NCD_TOKEN_DO: {
            Parse(state->parser, TOKEN_DO, minor, &state->out);
        } break;
        
        case NCD_TOKEN_INTERRUPT: {
            Parse(state->parser, TOKEN_INTERRUPT, minor, &state->out);
        } break;
        
        default:
            BLog(BLOG_ERROR, "line %zu, character %zu: invalid token", line, line_char);
            free(minor.str);
            state->error = 1;
            return 0;
    }
    
    // if we got syntax error, stop parsing
    if (state->out.syntax_error) {
        BLog(BLOG_ERROR, "line %zu, character %zu: syntax error", line, line_char);
        state->error = 1;
        return 0;
    }
    
    if (state->out.out_of_memory) {
        BLog(BLOG_ERROR, "line %zu, character %zu: out of memory", line, line_char);
        state->error = 1;
        return 0;
    }
    
    return 1;
}

int NCDConfigParser_Parse (char *config, size_t config_len, NCDProgram *out_ast)
{
    struct parser_state state;
    
    state.out.out_of_memory = 0;
    state.out.syntax_error = 0;
    state.out.have_ast = 0;
    state.error = 0;
    
    if (!(state.parser = ParseAlloc(malloc))) {
        BLog(BLOG_ERROR, "ParseAlloc failed");
        return 0;
    }
    
    // tokenize and parse
    NCDConfigTokenizer_Tokenize(MemRef_Make(config, config_len), tokenizer_output, &state);
    
    ParseFree(state.parser, free);
    
    if (state.error) {
        if (state.out.have_ast) {
            NCDProgram_Free(&state.out.ast);
        }
        return 0;
    }
    
    ASSERT(state.out.have_ast)
    
    *out_ast = state.out.ast;
    return 1;
}
