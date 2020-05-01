/**
 * @file NCDEvaluator.h
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

#ifndef BADVPN_NCDEVALUATOR_H
#define BADVPN_NCDEVALUATOR_H

#include <stddef.h>

#include <misc/debug.h>
#include <structure/Vector.h>
#include <ncd/NCDAst.h>
#include <ncd/NCDStringIndex.h>
#include <ncd/NCDVal.h>

struct NCDEvaluator__Expr {
    NCDValMem mem;
    NCDValSafeRef ref;
    NCDValReplaceProg prog;
};

struct NCDEvaluator__Var {
    NCD_string_id_t *varnames;
    size_t num_names;
};

#include "NCDEvaluator_var_vec.h"
#include <structure/Vector_decl.h>

struct NCDEvaluator__Call {
    NCD_string_id_t func_name_id;
    struct NCDEvaluator__Expr *args;
    size_t num_args;
};

#include "NCDEvaluator_call_vec.h"
#include <structure/Vector_decl.h>

struct NCDEvaluator__eval_context;

typedef struct {
    NCDStringIndex *string_index;
    NCDEvaluator__VarVec vars;
    NCDEvaluator__CallVec calls;
} NCDEvaluator;

typedef struct {
    struct NCDEvaluator__Expr expr;
} NCDEvaluatorExpr;

typedef struct {
    struct NCDEvaluator__eval_context const *context;
    int call_index;
} NCDEvaluatorArgs;

typedef struct {
    void *user;
    int (*func_eval_var) (void *user, NCD_string_id_t const *varnames, size_t num_names, NCDValMem *mem, NCDValRef *out);
    int (*func_eval_call) (void *user, NCD_string_id_t func_name_id, NCDEvaluatorArgs args, NCDValMem *mem, NCDValRef *out);
} NCDEvaluator_EvalFuncs;

int NCDEvaluator_Init (NCDEvaluator *o, NCDStringIndex *string_index) WARN_UNUSED;
void NCDEvaluator_Free (NCDEvaluator *o);
int NCDEvaluatorExpr_Init (NCDEvaluatorExpr *o, NCDEvaluator *eval, NCDValue *value) WARN_UNUSED;
void NCDEvaluatorExpr_Free (NCDEvaluatorExpr *o);
int NCDEvaluatorExpr_Eval (NCDEvaluatorExpr *o, NCDEvaluator *eval, NCDEvaluator_EvalFuncs const *funcs, NCDValMem *out_newmem, NCDValRef *out_val) WARN_UNUSED;
size_t NCDEvaluatorArgs_Count (NCDEvaluatorArgs *o);
int NCDEvaluatorArgs_EvalArg (NCDEvaluatorArgs *o, size_t index, NCDValMem *mem, NCDValRef *out_ref) WARN_UNUSED;
int NCDEvaluatorArgs_EvalArgNewMem (NCDEvaluatorArgs *o, size_t index, NCDValMem *out_newmem, NCDValRef *out_ref) WARN_UNUSED;

#endif
