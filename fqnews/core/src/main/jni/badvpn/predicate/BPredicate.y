/**
 * @file BPredicate.y
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
 * {@link BPredicate} grammar file.
 */

%{

#include <stdlib.h>

#include <predicate/BPredicate_internal.h>
#include <predicate/BPredicate_parser.h>

#define YYLEX_PARAM scanner

static struct predicate_node * make_constant (int val)
{
    struct predicate_node *n = (struct predicate_node *)malloc(sizeof(*n));
    if (!n) {
        return NULL;
    }

    n->type = NODE_CONSTANT;
    n->constant.val = val;

    return n;
}

static struct predicate_node * make_negation (struct predicate_node *op)
{
    if (!op) {
        goto fail;
    }

    struct predicate_node *n = (struct predicate_node *)malloc(sizeof(*n));
    if (!n) {
        goto fail;
    }

    n->type = NODE_NEG;
    n->neg.op = op;

    return n;

fail:
    if (op) {
        free_predicate_node(op);
    }
    return NULL;
}

static struct predicate_node * make_conjunction (struct predicate_node *op1, struct predicate_node *op2)
{
    if (!op1 || !op2) {
        goto fail;
    }

    struct predicate_node *n = (struct predicate_node *)malloc(sizeof(*n));
    if (!n) {
        goto fail;
    }

    n->type = NODE_CONJUNCT;
    n->conjunct.op1 = op1;
    n->conjunct.op2 = op2;

    return n;

fail:
    if (op1) {
        free_predicate_node(op1);
    }
    if (op2) {
        free_predicate_node(op2);
    }
    return NULL;
}

static struct predicate_node * make_disjunction (struct predicate_node *op1, struct predicate_node *op2)
{
    if (!op1 || !op2) {
        goto fail;
    }

    struct predicate_node *n = (struct predicate_node *)malloc(sizeof(*n));
    if (!n) {
        goto fail;
    }

    n->type = NODE_DISJUNCT;
    n->disjunct.op1 = op1;
    n->disjunct.op2 = op2;

    return n;

fail:
    if (op1) {
        free_predicate_node(op1);
    }
    if (op2) {
        free_predicate_node(op2);
    }
    return NULL;
}

static struct predicate_node * make_function (char *name, struct arguments_node *args, int need_args)
{
    if (!name || (!args && need_args)) {
        goto fail;
    }

    struct predicate_node *n = (struct predicate_node *)malloc(sizeof(*n));
    if (!n) {
        goto fail;
    }

    n->type = NODE_FUNCTION;
    n->function.name = name;
    n->function.args = args;

    return n;

fail:
    if (name) {
        free(name);
    }
    if (args) {
        free_arguments_node(args);
    }
    return NULL;
}

static struct arguments_node * make_arguments (struct arguments_arg arg, struct arguments_node *next, int need_next)
{
    if (arg.type == ARGUMENT_INVALID || (!next && need_next)) {
        goto fail;
    }

    struct arguments_node *n = (struct arguments_node *)malloc(sizeof(*n));
    if (!n) {
        goto fail;
    }

    n->arg = arg;
    n->next = next;

    return n;

fail:
    free_argument(arg);
    if (next) {
        free_arguments_node(next);
    }
    return NULL;
}

static struct arguments_arg make_argument_predicate (struct predicate_node *pr)
{
    struct arguments_arg ret;

    if (!pr) {
        goto fail;
    }

    ret.type = ARGUMENT_PREDICATE;
    ret.predicate = pr;

    return ret;

fail:
    ret.type = ARGUMENT_INVALID;
    return ret;
}

static struct arguments_arg make_argument_string (char *string)
{
    struct arguments_arg ret;

    if (!string) {
        goto fail;
    }

    ret.type = ARGUMENT_STRING;
    ret.string = string;

    return ret;

fail:
    ret.type = ARGUMENT_INVALID;
    return ret;
}

%}

%pure-parser
%locations
%parse-param {void *scanner}
%parse-param {struct predicate_node **result}

%union {
    char *text;
    struct predicate_node *node;
    struct arguments_node *arg_node;
    struct predicate_node nfaw;
    struct arguments_arg arg_arg;
};

// token types
%token <text> STRING NAME
%token PEER1_NAME PEER2_NAME AND OR NOT SPAR EPAR CONSTANT_TRUE CONSTANT_FALSE COMMA

// string token destructor
%destructor {
    free($$);
} STRING NAME

// return values
%type <node> predicate constant parentheses neg conjunct disjunct function
%type <arg_node> arguments
%type <arg_arg> argument

// predicate node destructor
%destructor {
    if ($$) {
        free_predicate_node($$);
    }
} predicate constant parentheses neg conjunct disjunct function

// argument node destructor
%destructor {
    if ($$) {
        free_arguments_node($$);
    }
} arguments

// argument argument destructor
%destructor {
    free_argument($$);
} argument

%left OR
%left AND
%nonassoc NOT
%right COMMA

%%

input:
    predicate {
        *result = $1;
    }
    ;

predicate: constant | parentheses | neg | conjunct | disjunct | function;

constant:
    CONSTANT_TRUE {
        $$ = make_constant(1);
    }
    |
    CONSTANT_FALSE {
        $$ = make_constant(0);
    }
    ;

parentheses:
    SPAR predicate EPAR {
        $$ = $2;
    }
    ;

neg:
    NOT predicate {
        $$ = make_negation($2);
    }
    ;

conjunct:
    predicate AND predicate {
        $$ = make_conjunction($1, $3);
    }
    ;

disjunct:
    predicate OR predicate {
        $$ = make_disjunction($1, $3);
    }
    ;

function:
    NAME SPAR EPAR {
        $$ = make_function($1, NULL, 0);
    }
    |
    NAME SPAR arguments EPAR {
        $$ = make_function($1, $3, 1);
    }
    ;

arguments:
    argument {
        $$ = make_arguments($1, NULL, 0);
    }
    |
    argument COMMA arguments {
        $$ = make_arguments($1, $3, 1);
    }
    ;

argument:
    predicate {
        $$ = make_argument_predicate($1);
    }
    |
    STRING {
        $$ = make_argument_string($1);
    }
    ;
