/**
 * @file BPredicate.h
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
 * Object that parses and evaluates a logical expression.
 * Allows the user to define custom functions than can be
 * used in the expression.
 * 
 * Syntax and semantics for logical expressions:
 * 
 *   - true
 *     Logical true constant. Evaluates to 1.
 * 
 *   - false
 *     Logical false constant. Evaluates to 0.
 * 
 *   - NOT expression
 *     Logical negation. If the expression evaluates to error, the
 *     negation evaluates to error.
 * 
 *   - expression OR expression
 *     Logical disjunction. The second expression is only evaluated
 *     if the first expression evaluates to false. If a sub-expression
 *     evaluates to error, the disjunction evaluates to error.
 * 
 *   - expression AND expression
 *     Logical conjunction. The second expression is only evaluated
 *     if the first expression evaluates to true. If a sub-expression
 *     evaluates to error, the conjunction evaluates to error.
 * 
 *   - function(arg, ..., arg)
 *     Evaluation of a user-provided function (function is the name of the
 *     function, [a-zA-Z0-9_]+).
 *     If the function with the given name does not exist, it evaluates to
 *     error.
 *     Arguments are evaluated from left to right. Each argument can either
 *     be a logical expression or a string (characters enclosed in double
 *     quotes, without any double quote).
 *     If an argument is encountered, but all needed arguments have already
 *     been evaluated, the function evaluates to error.
 *     If an argument is of wrong type, it is not evaluated and the function
 *     evaluates to error.
 *     If an argument evaluates to error, the function evaluates to error.
 *     If after all arguments have been evaluated, the function needs more
 *     arguments, it evaluates to error.
 *     Then the handler function is called. If it returns anything other
 *     than 1 and 0, the function evaluates to error. Otherwise it evaluates
 *     to what the handler function returned.
 */

#ifndef BADVPN_PREDICATE_BPREDICATE_H
#define BADVPN_PREDICATE_BPREDICATE_H

#include <misc/debug.h>
#include <structure/BAVL.h>
#include <base/DebugObject.h>

#define PREDICATE_TYPE_BOOL 1
#define PREDICATE_TYPE_STRING 2

#define PREDICATE_MAX_NAME 16
#define PREDICATE_MAX_ARGS 16

/**
 * Handler function called when evaluating a custom function in the predicate.
 * 
 * @param user value passed to {@link BPredicateFunction_Init}
 * @param args arguments to the function. Points to an array of pointers (as many as the
 *             function has arguments), where each pointer points to either to an int or
 *             a zero-terminated string (depending on the type of the argument).
 * @return 1 for true, 0 for false, -1 for error
 */
typedef int (*BPredicate_callback) (void *user, void **args);

/**
 * Object that parses and evaluates a logical expression.
 * Allows the user to define custom functions than can be
 * used in the expression.
 */
typedef struct {
    DebugObject d_obj;
    void *root;
    BAVL functions_tree;
    #ifndef NDEBUG
    int in_function;
    #endif
} BPredicate;

/**
 * Object that represents a custom function in {@link BPredicate}.
 */
typedef struct {
    DebugObject d_obj;
    BPredicate *p;
    char name[PREDICATE_MAX_NAME + 1];
    int args[PREDICATE_MAX_ARGS];
    int num_args;
    BPredicate_callback callback;
    void *user;
    BAVLNode tree_node;
} BPredicateFunction;

/**
 * Initializes the object.
 * 
 * @param p the object
 * @param str logical expression
 * @return 1 on success, 0 on failure
 */
int BPredicate_Init (BPredicate *p, char *str) WARN_UNUSED;

/**
 * Frees the object.
 * Must have no custom functions.
 * Must not be called from function handlers.
 * 
 * @param p the object
 */
void BPredicate_Free (BPredicate *p);

/**
 * Evaluates the logical expression.
 * Must not be called from function handlers.
 * 
 * @param p the object
 * @return 1 for true, 0 for false, -1 for error
 */
int BPredicate_Eval (BPredicate *p);

/**
 * Registers a custom function for {@link BPredicate}.
 * Must not be called from function handlers.
 * 
 * @param o the object
 * @param p predicate to register the function for
 * @param args array of argument types. Each type is either PREDICATE_TYPE_BOOL or PREDICATE_TYPE_STRING.
 * @param num_args number of arguments for the function. Must be >=0 and <=PREDICATE_MAX_ARGS.
 * @param callback handler to call to evaluate the function
 * @param user value to pass to handler
 */
void BPredicateFunction_Init (BPredicateFunction *o, BPredicate *p, char *name, int *args, int num_args, BPredicate_callback callback, void *user);

/**
 * Removes a custom function for {@link BPredicate}.
 * Must not be called from function handlers.
 * 
 * @param o the object
 */
void BPredicateFunction_Free (BPredicateFunction *o);

#endif
