/**
 * @file BPredicate_internal.h
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
 * {@link BPredicate} expression tree definitions and functions.
 */

#ifndef BADVPN_PREDICATE_BPREDICATE_INTERNAL_H
#define BADVPN_PREDICATE_BPREDICATE_INTERNAL_H

#include <misc/debug.h>

#define NODE_CONSTANT 0
#define NODE_NEG 2
#define NODE_CONJUNCT 3
#define NODE_DISJUNCT 4
#define NODE_FUNCTION 5

struct arguments_node;

struct predicate_node {
    int type;
    union {
        struct {
            int val;
        } constant;
        struct {
            struct predicate_node *op;
        } neg;
        struct {
            struct predicate_node *op1;
            struct predicate_node *op2;
        } conjunct;
        struct {
            struct predicate_node *op1;
            struct predicate_node *op2;
        } disjunct;
        struct {
            char *name;
            struct arguments_node *args;
        } function;
    };
    int eval_value;
};

#define ARGUMENT_INVALID 0
#define ARGUMENT_PREDICATE 1
#define ARGUMENT_STRING 2

struct arguments_arg {
    int type;
    union {
        struct predicate_node *predicate;
        char *string;
    };
};

struct arguments_node {
    struct arguments_arg arg;
    struct arguments_node *next;
};

static void free_predicate_node (struct predicate_node *root);
static void free_argument (struct arguments_arg arg);
static void free_arguments_node (struct arguments_node *n);

void free_predicate_node (struct predicate_node *root)
{
    ASSERT(root)
    
    switch (root->type) {
        case NODE_CONSTANT:
            break;
        case NODE_NEG:
            free_predicate_node(root->neg.op);
            break;
        case NODE_CONJUNCT:
            free_predicate_node(root->conjunct.op1);
            free_predicate_node(root->conjunct.op2);
            break;
        case NODE_DISJUNCT:
            free_predicate_node(root->disjunct.op1);
            free_predicate_node(root->disjunct.op2);
            break;
        case NODE_FUNCTION:
            free(root->function.name);
            if (root->function.args) {
                free_arguments_node(root->function.args);
            }
            break;
        default:
            ASSERT(0)
            break;
    }
    
    free(root);
}

void free_argument (struct arguments_arg arg)
{
    switch (arg.type) {
        case ARGUMENT_INVALID:
            break;
        case ARGUMENT_PREDICATE:
            free_predicate_node(arg.predicate);
            break;
        case ARGUMENT_STRING:
            free(arg.string);
            break;
        default:
            ASSERT(0);
    }
}

void free_arguments_node (struct arguments_node *n)
{
    ASSERT(n)
    
    free_argument(n->arg);
    
    if (n->next) {
        free_arguments_node(n->next);
    }
    
    free(n);
}

#endif
