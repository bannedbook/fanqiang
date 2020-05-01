/**
 * @file predicate_test.c
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
#include <string.h>

#include <predicate/BPredicate.h>
#include <base/BLog.h>

static int func_hello (void *user, void **args)
{
    return 1;
}

static int func_neg (void *user, void **args)
{
    int arg = *((int *)args[0]);
    
    return !arg;
}

static int func_conj (void *user, void **args)
{
    int arg1 = *((int *)args[0]);
    int arg2 = *((int *)args[1]);
    
    return (arg1 && arg2);
}

static int func_strcmp (void *user, void **args)
{
    char *arg1 = (char *)args[0];
    char *arg2 = (char *)args[1];
    
    return (!strcmp(arg1, arg2));
}

static int func_error (void *user, void **args)
{
    return -1;
}

int main (int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <predicate>\n", argv[0]);
        return 1;
    }
    
    // init logger
    BLog_InitStdout();
    
    // init predicate
    BPredicate pr;
    if (!BPredicate_Init(&pr, argv[1])) {
        fprintf(stderr, "BPredicate_Init failed\n");
        return 1;
    }
    
    // init functions
    BPredicateFunction f_hello;
    BPredicateFunction_Init(&f_hello, &pr, "hello", NULL, 0, func_hello, NULL);
    int arr1[] = {PREDICATE_TYPE_BOOL};
    BPredicateFunction f_neg;
    BPredicateFunction_Init(&f_neg, &pr, "neg", arr1, 1, func_neg, NULL);
    int arr2[] = {PREDICATE_TYPE_BOOL, PREDICATE_TYPE_BOOL};
    BPredicateFunction f_conj;
    BPredicateFunction_Init(&f_conj, &pr, "conj", arr2, 2, func_conj, NULL);
    int arr3[] = {PREDICATE_TYPE_STRING, PREDICATE_TYPE_STRING};
    BPredicateFunction f_strcmp;
    BPredicateFunction_Init(&f_strcmp, &pr, "strcmp", arr3, 2, func_strcmp, NULL);
    BPredicateFunction f_error;
    BPredicateFunction_Init(&f_error, &pr, "error", NULL, 0, func_error, NULL);
    
    // evaluate
    int result = BPredicate_Eval(&pr);
    printf("%d\n", result);
    
    // free functions
    BPredicateFunction_Free(&f_hello);
    BPredicateFunction_Free(&f_neg);
    BPredicateFunction_Free(&f_conj);
    BPredicateFunction_Free(&f_strcmp);
    BPredicateFunction_Free(&f_error);
    
    // free predicate
    BPredicate_Free(&pr);
    
    return 0;
}
