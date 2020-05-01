/**
 * @file NCDConfigParser_parse.y
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

// argument for passing state to parser hooks
%extra_argument { struct parser_state *parser_out }

// type of structure representing tokens
%token_type { struct token }

// token destructor frees extra memory allocated for tokens
%token_destructor { free_token($$); }

// types of nonterminals
%type list_contents { struct value }
%type list { struct value }
%type map_contents { struct value }
%type map  { struct value }
%type value  { struct value }

// mention parser_out in some destructor to and avoid unused variable warning
%destructor list_contents { (void)parser_out; }

// try to dynamically grow the parse stack
%stack_size 0

// on syntax error, set the corresponding error flag
%syntax_error {
    parser_out->error_flags |= ERROR_FLAG_SYNTAX;
}

// workaroud Lemon bug: if the stack overflows, the token that caused the overflow will be leaked
%stack_overflow {
    if (yypMinor) {
        free_token(yypMinor->yy0);
    }
}

input ::= value(A). {
    if (!A.have) {
        goto failZ0;
    }
    
    if (!NCDVal_IsInvalid(parser_out->value)) {
        // should never happen
        parser_out->error_flags |= ERROR_FLAG_SYNTAX;
        goto failZ0;
    }
    
    if (!NCDValCons_Complete(&parser_out->cons, A.v, &parser_out->value, &parser_out->cons_error)) {
        handle_cons_error(parser_out);
        goto failZ0;
    }
    
failZ0:;
}

list_contents(R) ::= value(A). {
    if (!A.have) {
        goto failL0;
    }

    NCDValCons_NewList(&parser_out->cons, &R.v);

    if (!NCDValCons_ListPrepend(&parser_out->cons, &R.v, A.v, &parser_out->cons_error)) {
        handle_cons_error(parser_out);
        goto failL0;
    }

    R.have = 1;
    goto doneL;

failL0:
    R.have = 0;
doneL:;
}

list_contents(R) ::= value(A) COMMA list_contents(N). {
    if (!A.have || !N.have) {
        goto failM0;
    }

    if (!NCDValCons_ListPrepend(&parser_out->cons, &N.v, A.v, &parser_out->cons_error)) {
        handle_cons_error(parser_out);
        goto failM0;
    }

    R.have = 1;
    R.v = N.v;
    goto doneM;

failM0:
    R.have = 0;
doneM:;
}

list(R) ::= CURLY_OPEN CURLY_CLOSE. {
    NCDValCons_NewList(&parser_out->cons, &R.v);
    R.have = 1;
}

list(R) ::= CURLY_OPEN list_contents(A) CURLY_CLOSE. {
    R = A;
}

map_contents(R) ::= value(A) COLON value(B). {
    if (!A.have || !B.have) {
        goto failS0;
    }

    NCDValCons_NewMap(&parser_out->cons, &R.v);

    if (!NCDValCons_MapInsert(&parser_out->cons, &R.v, A.v, B.v, &parser_out->cons_error)) {
        handle_cons_error(parser_out);
        goto failS0;
    }

    R.have = 1;
    goto doneS;

failS0:
    R.have = 0;
doneS:;
}

map_contents(R) ::= value(A) COLON value(B) COMMA map_contents(N). {
    if (!A.have || !B.have || !N.have) {
        goto failT0;
    }

    if (!NCDValCons_MapInsert(&parser_out->cons, &N.v, A.v, B.v, &parser_out->cons_error)) {
        handle_cons_error(parser_out);
        goto failT0;
    }

    R.have = 1;
    R.v = N.v;
    goto doneT;

failT0:
    R.have = 0;
doneT:;
}

map(R) ::= BRACKET_OPEN BRACKET_CLOSE. {
    NCDValCons_NewMap(&parser_out->cons, &R.v);
    R.have = 1;
}

map(R) ::= BRACKET_OPEN map_contents(A) BRACKET_CLOSE. {
    R = A;
}

value(R) ::= STRING(A). {
    ASSERT(A.str)

    if (!NCDValCons_NewString(&parser_out->cons, (const uint8_t *)A.str, A.len, &R.v, &parser_out->cons_error)) {
        handle_cons_error(parser_out);
        goto failU0;
    }

    R.have = 1;
    goto doneU;

failU0:
    R.have = 0;
doneU:;
    free_token(A);
}

value(R) ::= list(A). {
    R = A;
}

value(R) ::= map(A). {
    R = A;
}
