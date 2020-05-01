/**
 * @file NCDConfigParser.y
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

%include {

#include <string.h>
#include <stddef.h>

#include <misc/debug.h>
#include <misc/concat_strings.h>
#include <ncd/NCDAst.h>

struct parser_out {
    int out_of_memory;
    int syntax_error;
    int have_ast;
    NCDProgram ast;
};

struct token {
    char *str;
    size_t len;
};

struct program {
    int have;
    NCDProgram v;
};

struct block {
    int have;
    NCDBlock v;
};

struct statement {
    int have;
    NCDStatement v;
};

struct ifblock {
    int have;
    NCDIfBlock v;
};

struct value {
    int have;
    NCDValue v;
};

static void free_token (struct token o) { free(o.str); }
static void free_program (struct program o) { if (o.have) NCDProgram_Free(&o.v); }
static void free_block (struct block o) { if (o.have) NCDBlock_Free(&o.v); }
static void free_statement (struct statement o) { if (o.have) NCDStatement_Free(&o.v); }
static void free_ifblock (struct ifblock o) { if (o.have) NCDIfBlock_Free(&o.v); }
static void free_value (struct value o) { if (o.have) NCDValue_Free(&o.v); }

}

%extra_argument { struct parser_out *parser_out }

%token_type { struct token }

%token_destructor { free_token($$); }

%type processes { struct program }
%type statement { struct statement }
%type elif_maybe { struct ifblock }
%type elif { struct ifblock }
%type else_maybe { struct block }
%type statements { struct block }
%type dotted_name { char * }
%type list_contents_maybe { struct value }
%type list_contents { struct value }
%type list { struct value }
%type map_contents { struct value }
%type map  { struct value }
%type invoc { struct value }
%type value  { struct value }
%type name_maybe { char * }
%type process_or_template { int }
%type name_list { struct value }
%type interrupt_maybe { struct block }

// mention parser_out in some destructor to avoid an unused-variable warning
%destructor processes { (void)parser_out; free_program($$); }
%destructor statement { free_statement($$); }
%destructor elif_maybe { free_ifblock($$); }
%destructor elif { free_ifblock($$); }
%destructor else_maybe { free_block($$); }
%destructor statements { free_block($$); }
%destructor dotted_name { free($$); }
%destructor list_contents_maybe { free_value($$); }
%destructor list_contents { free_value($$); }
%destructor list { free_value($$); }
%destructor map_contents { free_value($$); }
%destructor map { free_value($$); }
%destructor invoc { free_value($$); }
%destructor value { free_value($$); }
%destructor name_maybe { free($$); }
%destructor name_list { free_value($$); }
%destructor interrupt_maybe { free_block($$); }

%stack_size 0

%syntax_error {
    parser_out->syntax_error = 1;
}

// workaroud Lemon bug: if the stack overflows, the token that caused the overflow will be leaked
%stack_overflow {
    if (yypMinor) {
        free_token(yypMinor->yy0);
    }
}

input ::= processes(A). {
    ASSERT(!parser_out->have_ast)

    if (A.have) {
        parser_out->have_ast = 1;
        parser_out->ast = A.v;
    }
}

processes(R) ::= . {
    NCDProgram prog;
    NCDProgram_Init(&prog);
    
    R.have = 1;
    R.v = prog;
}

processes(R) ::= INCLUDE STRING(A) processes(N). {
    ASSERT(A.str)
    if (!N.have) {
        goto failA0;
    }
    
    NCDProgramElem elem;
    if (!NCDProgramElem_InitInclude(&elem, A.str, A.len)) {
        goto failA0;
    }
    
    if (!NCDProgram_PrependElem(&N.v, elem)) {
        goto failA1;
    }
    
    R.have = 1;
    R.v = N.v;
    N.have = 0;
    goto doneA;

failA1:
    NCDProgramElem_Free(&elem);
failA0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneA:
    free_token(A);
    free_program(N);
}

processes(R) ::= INCLUDE_GUARD STRING(A) processes(N). {
    ASSERT(A.str)
    if (!N.have) {
        goto failZ0;
    }
    
    NCDProgramElem elem;
    if (!NCDProgramElem_InitIncludeGuard(&elem, A.str, A.len)) {
        goto failZ0;
    }
    
    if (!NCDProgram_PrependElem(&N.v, elem)) {
        goto failZ1;
    }
    
    R.have = 1;
    R.v = N.v;
    N.have = 0;
    goto doneZ;

failZ1:
    NCDProgramElem_Free(&elem);
failZ0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneZ:
    free_token(A);
    free_program(N);
}

processes(R) ::= process_or_template(T) NAME(A) CURLY_OPEN statements(B) CURLY_CLOSE processes(N). {
    ASSERT(A.str)
    if (!B.have || !N.have) {
        goto failB0;
    }

    NCDProcess proc;
    if (!NCDProcess_Init(&proc, T, A.str, B.v)) {
        goto failB0;
    }
    B.have = 0;
    
    NCDProgramElem elem;
    NCDProgramElem_InitProcess(&elem, proc);

    if (!NCDProgram_PrependElem(&N.v, elem)) {
        goto failB1;
    }

    R.have = 1;
    R.v = N.v;
    N.have = 0;
    goto doneB;

failB1:
    NCDProgramElem_Free(&elem);
failB0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneB:
    free_token(A);
    free_block(B);
    free_program(N);
}

statement(R) ::= dotted_name(A) ROUND_OPEN list_contents_maybe(B) ROUND_CLOSE name_maybe(C) SEMICOLON. {
    if (!A || !B.have) {
        goto failC0;
    }

    if (!NCDStatement_InitReg(&R.v, C, NULL, A, B.v)) {
        goto failC0;
    }
    B.have = 0;

    R.have = 1;
    goto doneC;

failC0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneC:
    free(A);
    free_value(B);
    free(C);
}

statement(R) ::= dotted_name(M) ARROW dotted_name(A) ROUND_OPEN list_contents_maybe(B) ROUND_CLOSE name_maybe(C) SEMICOLON. {
    if (!M || !A || !B.have) {
        goto failD0;
    }

    if (!NCDStatement_InitReg(&R.v, C, M, A, B.v)) {
        goto failD0;
    }
    B.have = 0;

    R.have = 1;
    goto doneD;

failD0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneD:
    free(M);
    free(A);
    free_value(B);
    free(C);
}

statement(R) ::= IF ROUND_OPEN value(A) ROUND_CLOSE CURLY_OPEN statements(B) CURLY_CLOSE elif_maybe(I) else_maybe(E) name_maybe(C) SEMICOLON. {
    if (!A.have || !B.have || !I.have) {
        goto failE0;
    }

    NCDIf ifc;
    NCDIf_Init(&ifc, A.v, B.v);
    A.have = 0;
    B.have = 0;

    if (!NCDIfBlock_PrependIf(&I.v, ifc)) {
        NCDIf_Free(&ifc);
        goto failE0;
    }

    if (!NCDStatement_InitIf(&R.v, C, I.v, NCDIFTYPE_IF)) {
        goto failE0;
    }
    I.have = 0;

    if (E.have) {
        NCDStatement_IfAddElse(&R.v, E.v);
        E.have = 0;
    }

    R.have = 1;
    goto doneE;

failE0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneE:
    free_value(A);
    free_block(B);
    free_ifblock(I);
    free_block(E);
    free(C);
}

statement(R) ::= FOREACH ROUND_OPEN value(A) AS NAME(B) ROUND_CLOSE CURLY_OPEN statements(S) CURLY_CLOSE name_maybe(N) SEMICOLON. {
    if (!A.have || !B.str || !S.have) {
        goto failEA0;
    }
    
    if (!NCDStatement_InitForeach(&R.v, N, A.v, B.str, NULL, S.v)) {
        goto failEA0;
    }
    A.have = 0;
    S.have = 0;
    
    R.have = 1;
    goto doneEA0;
    
failEA0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneEA0:
    free_value(A);
    free_token(B);
    free_block(S);
    free(N);
}

statement(R) ::= FOREACH ROUND_OPEN value(A) AS NAME(B) COLON NAME(C) ROUND_CLOSE CURLY_OPEN statements(S) CURLY_CLOSE name_maybe(N) SEMICOLON. {
    if (!A.have || !B.str || !C.str || !S.have) {
        goto failEB0;
    }
    
    if (!NCDStatement_InitForeach(&R.v, N, A.v, B.str, C.str, S.v)) {
        goto failEB0;
    }
    A.have = 0;
    S.have = 0;
    
    R.have = 1;
    goto doneEB0;
    
failEB0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneEB0:
    free_value(A);
    free_token(B);
    free_token(C);
    free_block(S);
    free(N);
}

elif_maybe(R) ::= . {
    NCDIfBlock_Init(&R.v);
    R.have = 1;
}

elif_maybe(R) ::= elif(A). {
    R = A;
}

elif(R) ::= ELIF ROUND_OPEN value(A) ROUND_CLOSE CURLY_OPEN statements(B) CURLY_CLOSE. {
    if (!A.have || !B.have) {
        goto failF0;
    }

    NCDIfBlock_Init(&R.v);

    NCDIf ifc;
    NCDIf_Init(&ifc, A.v, B.v);
    A.have = 0;
    B.have = 0;

    if (!NCDIfBlock_PrependIf(&R.v, ifc)) {
        goto failF1;
    }

    R.have = 1;
    goto doneF0;

failF1:
    NCDIf_Free(&ifc);
    NCDIfBlock_Free(&R.v);
failF0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneF0:
    free_value(A);
    free_block(B);
}

elif(R) ::= ELIF ROUND_OPEN value(A) ROUND_CLOSE CURLY_OPEN statements(B) CURLY_CLOSE elif(N). {
    if (!A.have || !B.have || !N.have) {
        goto failG0;
    }

    NCDIf ifc;
    NCDIf_Init(&ifc, A.v, B.v);
    A.have = 0;
    B.have = 0;

    if (!NCDIfBlock_PrependIf(&N.v, ifc)) {
        goto failG1;
    }

    R.have = 1;
    R.v = N.v;
    N.have = 0;
    goto doneG0;

failG1:
    NCDIf_Free(&ifc);
failG0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneG0:
    free_value(A);
    free_block(B);
    free_ifblock(N);
}

else_maybe(R) ::= . {
    R.have = 0;
}

else_maybe(R) ::= ELSE CURLY_OPEN statements(B) CURLY_CLOSE. {
    R = B;
}

statement(R) ::= BLOCK CURLY_OPEN statements(S) CURLY_CLOSE name_maybe(N) SEMICOLON. {
    if (!S.have) {
        goto failGA0;
    }
    
    if (!NCDStatement_InitBlock(&R.v, N, S.v)) {
        goto failGA0;
    }
    S.have = 0;
    
    R.have = 1;
    goto doneGA0;
    
failGA0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneGA0:
    free_block(S);
    free(N);
}

interrupt_maybe(R) ::= . {
    R.have = 0;
}

interrupt_maybe(R) ::= TOKEN_INTERRUPT CURLY_OPEN statements(S) CURLY_CLOSE. {
    R = S;
}

statement(R) ::= TOKEN_DO CURLY_OPEN statements(S) CURLY_CLOSE interrupt_maybe(I) name_maybe(N) SEMICOLON. {
    if (!S.have) {
        goto failGB0;
    }
    
    NCDIfBlock if_block;
    NCDIfBlock_Init(&if_block);
    
    if (I.have) {
        NCDIf int_if;
        NCDIf_InitBlock(&int_if, I.v);
        I.have = 0;
        
        if (!NCDIfBlock_PrependIf(&if_block, int_if)) {
            NCDIf_Free(&int_if);
            goto failGB1;
        }
    }
    
    NCDIf the_if;
    NCDIf_InitBlock(&the_if, S.v);
    S.have = 0;
    
    if (!NCDIfBlock_PrependIf(&if_block, the_if)) {
        NCDIf_Free(&the_if);
        goto failGB1;
    }
    
    if (!NCDStatement_InitIf(&R.v, N, if_block, NCDIFTYPE_DO)) {
        goto failGB1;
    }
    
    R.have = 1;
    goto doneGB0;
    
failGB1:
    NCDIfBlock_Free(&if_block);
failGB0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneGB0:
    free_block(S);
    free_block(I);
    free(N);
}

statements(R) ::= statement(A). {
    if (!A.have) {
        goto failH0;
    }

    NCDBlock_Init(&R.v);

    if (!NCDBlock_PrependStatement(&R.v, A.v)) {
        goto failH1;
    }
    A.have = 0;

    R.have = 1;
    goto doneH;

failH1:
    NCDBlock_Free(&R.v);
failH0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneH:
    free_statement(A);
}

statements(R) ::= statement(A) statements(N). {
    if (!A.have || !N.have) {
        goto failI0;
    }

    if (!NCDBlock_PrependStatement(&N.v, A.v)) {
        goto failI1;
    }
    A.have = 0;

    R.have = 1;
    R.v = N.v;
    N.have = 0;
    goto doneI;

failI1:
    NCDBlock_Free(&R.v);
failI0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneI:
    free_statement(A);
    free_block(N);
}

dotted_name(R) ::= NAME(A). {
    ASSERT(A.str)

    R = A.str;
}

dotted_name(R) ::= NAME(A) DOT dotted_name(N). {
    ASSERT(A.str)
    if (!N) {
        goto failJ0;
    }

    if (!(R = concat_strings(3, A.str, ".", N))) {
        goto failJ0;
    }

    goto doneJ;

failJ0:
    R = NULL;
    parser_out->out_of_memory = 1;
doneJ:
    free_token(A);
    free(N);
}

name_list(R) ::= NAME(A). {
    if (!A.str) {
        goto failK0;
    }

    NCDValue_InitList(&R.v);
    
    NCDValue this_string;
    if (!NCDValue_InitString(&this_string, A.str)) {
        goto failK1;
    }
    
    if (!NCDValue_ListPrepend(&R.v, this_string)) {
        goto failK2;
    }

    R.have = 1;
    goto doneK;

failK2:
    NCDValue_Free(&this_string);
failK1:
    NCDValue_Free(&R.v);
failK0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneK:
    free_token(A);
}

name_list(R) ::= NAME(A) DOT name_list(N). {
    if (!A.str || !N.have) {
        goto failKA0;
    }
    
    NCDValue this_string;
    if (!NCDValue_InitString(&this_string, A.str)) {
        goto failKA0;
    }

    if (!NCDValue_ListPrepend(&N.v, this_string)) {
        goto failKA1;
    }

    R.have = 1;
    R.v = N.v;
    N.have = 0;
    goto doneKA;

failKA1:
    NCDValue_Free(&this_string);
failKA0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneKA:
    free_token(A);
    free_value(N);
}

list_contents_maybe(R) ::= . {
    R.have = 1;
    NCDValue_InitList(&R.v);
}

list_contents_maybe(R) ::= list_contents(A). {
    R = A;
}

list_contents(R) ::= value(A). {
    if (!A.have) {
        goto failL0;
    }

    NCDValue_InitList(&R.v);

    if (!NCDValue_ListPrepend(&R.v, A.v)) {
        goto failL1;
    }
    A.have = 0;

    R.have = 1;
    goto doneL;

failL1:
    NCDValue_Free(&R.v);
failL0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneL:
    free_value(A);
}

list_contents(R) ::= value(A) COMMA list_contents(N). {
    if (!A.have || !N.have) {
        goto failM0;
    }

    if (!NCDValue_ListPrepend(&N.v, A.v)) {
        goto failM0;
    }
    A.have = 0;

    R.have = 1;
    R.v = N.v;
    N.have = 0;
    goto doneM;

failM0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneM:
    free_value(A);
    free_value(N);
}

list(R) ::= CURLY_OPEN CURLY_CLOSE. {
    R.have = 1;
    NCDValue_InitList(&R.v);
}

list(R) ::= CURLY_OPEN list_contents(A) CURLY_CLOSE. {
    R = A;
}

map_contents(R) ::= value(A) COLON value(B). {
    if (!A.have || !B.have) {
        goto failS0;
    }

    NCDValue_InitMap(&R.v);

    if (!NCDValue_MapPrepend(&R.v, A.v, B.v)) {
        goto failS1;
    }
    A.have = 0;
    B.have = 0;

    R.have = 1;
    goto doneS;

failS1:
    NCDValue_Free(&R.v);
failS0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneS:
    free_value(A);
    free_value(B);
}

map_contents(R) ::= value(A) COLON value(B) COMMA map_contents(N). {
    if (!A.have || !B.have || !N.have) {
        goto failT0;
    }

    if (!NCDValue_MapPrepend(&N.v, A.v, B.v)) {
        goto failT0;
    }
    A.have = 0;
    B.have = 0;

    R.have = 1;
    R.v = N.v;
    N.have = 0;
    goto doneT;

failT0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneT:
    free_value(A);
    free_value(B);
    free_value(N);
}

map(R) ::= BRACKET_OPEN BRACKET_CLOSE. {
    R.have = 1;
    NCDValue_InitMap(&R.v);
}

map(R) ::= BRACKET_OPEN map_contents(A) BRACKET_CLOSE. {
    R = A;
}

invoc(R) ::= value(F) ROUND_OPEN list_contents_maybe(A) ROUND_CLOSE. {
    if (!F.have || !A.have) {
        goto failQ0;
    }
    
    if (!NCDValue_InitInvoc(&R.v, F.v, A.v)) {
        goto failQ0;
    }
    F.have = 0;
    A.have = 0;
    R.have = 1;
    goto doneQ;
    
failQ0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneQ:
    free_value(F);
    free_value(A);
}

value(R) ::= STRING(A). {
    ASSERT(A.str)

    if (!NCDValue_InitStringBin(&R.v, (uint8_t *)A.str, A.len)) {
        goto failU0;
    }

    R.have = 1;
    goto doneU;

failU0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneU:
    free_token(A);
}

value(R) ::= AT_SIGN dotted_name(A). {
    if (!A) {
        goto failUA0;
    }
    
    if (!NCDValue_InitString(&R.v, A)) {
        goto failUA0;
    }
    
    R.have = 1;
    goto doneUA0;
    
failUA0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneUA0:
    free(A);
}

value(R) ::= CARET name_list(A). {
    R = A;
}

value(R) ::= dotted_name(A). {
    if (!A) {
        goto failV0;
    }

    if (!NCDValue_InitVar(&R.v, A)) {
        goto failV0;
    }

    R.have = 1;
    goto doneV;

failV0:
    R.have = 0;
    parser_out->out_of_memory = 1;
doneV:
    free(A);
}

value(R) ::= list(A). {
    R = A;
}

value(R) ::= map(A). {
    R = A;
}

value(R) ::= ROUND_OPEN value(A) ROUND_CLOSE. {
    R = A;
}

value(R) ::= invoc(A). {
    R = A;
}

name_maybe(R) ::= . {
    R = NULL;
}

name_maybe(R) ::= NAME(A). {
    ASSERT(A.str)

    R = A.str;
}

process_or_template(R) ::= PROCESS. {
    R = 0;
}

process_or_template(R) ::= TEMPLATE. {
    R = 1;
}
