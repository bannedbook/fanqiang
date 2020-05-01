/**
 * @file NCDEvaluator.c
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

#include <stddef.h>
#include <limits.h>

#include <misc/debug.h>
#include <misc/balloc.h>
#include <base/BLog.h>
#include <ncd/make_name_indices.h>

#include "NCDEvaluator.h"

#include <generated/blog_channel_ncd.h>

#define NCDEVALUATOR_DEFAULT_VARARRAY_CAPACITY 64
#define NCDEVALUATOR_DEFAULT_CALLARRAY_CAPACITY 16

#define MAX_LOCAL_IDS (NCDVAL_TOPPLID / 2)

#include "NCDEvaluator_var_vec.h"
#include <structure/Vector_impl.h>

#include "NCDEvaluator_call_vec.h"
#include <structure/Vector_impl.h>

struct NCDEvaluator__eval_context {
    NCDEvaluator *eval;
    NCDEvaluator_EvalFuncs const *funcs;
};

static int expr_init (struct NCDEvaluator__Expr *o, NCDEvaluator *eval, NCDValue *value);
static void expr_free (struct NCDEvaluator__Expr *o);
static int expr_eval (struct NCDEvaluator__Expr *o, struct NCDEvaluator__eval_context const *context, NCDValMem *out_newmem, NCDValRef *out_val);
static int add_expr_recurser (NCDEvaluator *o, NCDValue *value, NCDValMem *mem, NCDValRef *out);
static int replace_placeholders_callback (void *arg, int plid, NCDValMem *mem, NCDValRef *out);

static int expr_init (struct NCDEvaluator__Expr *o, NCDEvaluator *eval, NCDValue *value)
{
    ASSERT((NCDValue_Type(value), 1))
    
    NCDValMem_Init(&o->mem, eval->string_index);
    
    NCDValRef ref;
    if (!add_expr_recurser(eval, value, &o->mem, &ref)) {
        goto fail1;
    }
    
    o->ref = NCDVal_ToSafe(ref);
    
    if (!NCDVal_IsSafeRefPlaceholder(o->ref)) {
        if (!NCDValReplaceProg_Init(&o->prog, ref)) {
            BLog(BLOG_ERROR, "NCDValReplaceProg_Init failed");
            goto fail1;
        }
    }
    
    return 1;
    
fail1:
    NCDValMem_Free(&o->mem);
    return 0;
}

static void expr_free (struct NCDEvaluator__Expr *o)
{
    if (!NCDVal_IsSafeRefPlaceholder(o->ref)) {
        NCDValReplaceProg_Free(&o->prog);
    }
    NCDValMem_Free(&o->mem);
}

static int expr_eval (struct NCDEvaluator__Expr *o, struct NCDEvaluator__eval_context const *context, NCDValMem *out_newmem, NCDValRef *out_val)
{
    if (!NCDVal_IsSafeRefPlaceholder(o->ref)) {
        if (!NCDValMem_InitCopy(out_newmem, &o->mem)) {
            BLog(BLOG_ERROR, "NCDValMem_InitCopy failed");
            goto fail0;
        }
        
        if (!NCDValReplaceProg_Execute(o->prog, out_newmem, replace_placeholders_callback, (void *)context)) {
            goto fail_free;
        }
        
        *out_val = NCDVal_FromSafe(out_newmem, o->ref);
    } else {
        NCDValMem_Init(out_newmem, context->eval->string_index);
        
        NCDValRef ref;
        if (!replace_placeholders_callback((void *)context, NCDVal_GetSafeRefPlaceholderId(o->ref), out_newmem, &ref) || NCDVal_IsInvalid(ref)) {
            goto fail_free;
        }
        
        *out_val = ref;
    }
    
    return 1;
    
fail_free:
    NCDValMem_Free(out_newmem);
fail0:
    return 0;
}

static int add_expr_recurser (NCDEvaluator *o, NCDValue *value, NCDValMem *mem, NCDValRef *out)
{
    switch (NCDValue_Type(value)) {
        case NCDVALUE_STRING: {
            const char *str = NCDValue_StringValue(value);
            size_t len = NCDValue_StringLength(value);
            
            NCD_string_id_t string_id = NCDStringIndex_GetBin(o->string_index, str, len);
            if (string_id < 0) {
                BLog(BLOG_ERROR, "NCDStringIndex_GetBin failed");
                goto fail;
            }
            
            *out = NCDVal_NewIdString(mem, string_id);
            if (NCDVal_IsInvalid(*out)) {
                goto fail;
            }
        } break;
        
        case NCDVALUE_LIST: {
            *out = NCDVal_NewList(mem, NCDValue_ListCount(value));
            if (NCDVal_IsInvalid(*out)) {
                goto fail;
            }
            
            for (NCDValue *e = NCDValue_ListFirst(value); e; e = NCDValue_ListNext(value, e)) {
                NCDValRef vval;
                if (!add_expr_recurser(o, e, mem, &vval)) {
                    goto fail;
                }
                
                if (!NCDVal_ListAppend(*out, vval)) {
                    BLog(BLOG_ERROR, "depth limit exceeded");
                    goto fail;
                }
            }
        } break;
        
        case NCDVALUE_MAP: {
            *out = NCDVal_NewMap(mem, NCDValue_MapCount(value));
            if (NCDVal_IsInvalid(*out)) {
                goto fail;
            }
            
            for (NCDValue *ekey = NCDValue_MapFirstKey(value); ekey; ekey = NCDValue_MapNextKey(value, ekey)) {
                NCDValue *eval = NCDValue_MapKeyValue(value, ekey);
                
                NCDValRef vkey;
                NCDValRef vval;
                if (!add_expr_recurser(o, ekey, mem, &vkey) ||
                    !add_expr_recurser(o, eval, mem, &vval)
                ) {
                    goto fail;
                }
                
                int inserted;
                if (!NCDVal_MapInsert(*out, vkey, vval, &inserted)) {
                    BLog(BLOG_ERROR, "depth limit exceeded");
                    goto fail;
                }
                if (!inserted) {
                    BLog(BLOG_ERROR, "duplicate key in map");
                    goto fail;
                }
            }
        } break;
        
        case NCDVALUE_VAR: {
            struct NCDEvaluator__Var var;
            
            if (!ncd_make_name_indices(o->string_index, NCDValue_VarName(value), &var.varnames, &var.num_names)) {
                BLog(BLOG_ERROR, "ncd_make_name_indices failed");
                goto fail_var0;
            }
            
            size_t index;
            struct NCDEvaluator__Var *varptr = NCDEvaluator__VarVec_Push(&o->vars, &index);
            if (!varptr) {
                BLog(BLOG_ERROR, "NCDEvaluator__VarVec_Push failed");
                goto fail_var1;
            }
            
            if (index >= MAX_LOCAL_IDS) {
                BLog(BLOG_ERROR, "too many variables");
                goto fail_var2;
            }
            
            *varptr = var;
            
            *out = NCDVal_NewPlaceholder(mem, ((int)index << 1) | 0);
            break;
            
        fail_var2:
            NCDEvaluator__VarVec_Pop(&o->vars, NULL);
        fail_var1:
            BFree(var.varnames);
        fail_var0:
            goto fail;
        } break;
        
        case NCDVALUE_INVOC: {
            struct NCDEvaluator__Call call;
            
            NCDValue *func = NCDValue_InvocFunc(value);
            if (NCDValue_Type(func) != NCDVALUE_STRING) {
                BLog(BLOG_ERROR, "call function is not a string");
                goto fail_invoc0;
            }
            
            call.func_name_id = NCDStringIndex_GetBin(o->string_index, NCDValue_StringValue(func), NCDValue_StringLength(func));
            if (call.func_name_id < 0) {
                BLog(BLOG_ERROR, "NCDStringIndex_GetBin failed");
                goto fail_invoc0;
            }
            
            NCDValue *arg = NCDValue_InvocArg(value);
            if (NCDValue_Type(arg) != NCDVALUE_LIST) {
                BLog(BLOG_ERROR, "call argument is not a list literal!?");
                goto fail_invoc0;
            }
            
            if (!(call.args = BAllocArray(NCDValue_ListCount(arg), sizeof(call.args[0])))) {
                BLog(BLOG_ERROR, "BAllocArray failed");
                goto fail_invoc0;
            }
            call.num_args = 0;
            
            for (NCDValue *e = NCDValue_ListFirst(arg); e; e = NCDValue_ListNext(arg, e)) {
                if (!expr_init(&call.args[call.num_args], o, e)) {
                    goto fail_invoc1;
                }
                call.num_args++;
            }
            
            size_t index;
            struct NCDEvaluator__Call *callptr = NCDEvaluator__CallVec_Push(&o->calls, &index);
            if (!callptr) {
                BLog(BLOG_ERROR, "NCDEvaluator__CallVec_Push failed");
                goto fail_invoc1;
            }
            
            if (index >= MAX_LOCAL_IDS) {
                BLog(BLOG_ERROR, "too many variables");
                goto fail_invoc2;
            }
            
            *callptr = call;
            
            *out = NCDVal_NewPlaceholder(mem, ((int)index << 1) | 1);
            break;
            
        fail_invoc2:
            NCDEvaluator__CallVec_Pop(&o->calls, NULL);
        fail_invoc1:
            while (call.num_args-- > 0) {
                expr_free(&call.args[call.num_args]);
            }
            BFree(call.args);
        fail_invoc0:
            goto fail;
        } break;
        
        default: {
            BLog(BLOG_ERROR, "expression type not supported");
            goto fail;
        } break;
    }
    
    return 1;
    
fail:
    return 0;
}

static int replace_placeholders_callback (void *arg, int plid, NCDValMem *mem, NCDValRef *out)
{
    struct NCDEvaluator__eval_context const *context = arg;
    NCDEvaluator *o = context->eval;
    
    int type = plid & 1;
    int index = plid >> 1;
    
    ASSERT(index >= 0)
    ASSERT(index < MAX_LOCAL_IDS)
    
    int res;
    
    switch (type) {
        case 0: {
            struct NCDEvaluator__Var *var = NCDEvaluator__VarVec_Get(&o->vars, index);
            
            res = context->funcs->func_eval_var(context->funcs->user, var->varnames, var->num_names, mem, out);
        } break;
        
        case 1: {
            struct NCDEvaluator__Call *call = NCDEvaluator__CallVec_Get(&o->calls, index);
            
            NCDEvaluatorArgs args;
            args.context = context;
            args.call_index = index;
            
            res = context->funcs->func_eval_call(context->funcs->user, call->func_name_id, args, mem, out);
        } break;
        
        default: {
            ASSERT(0)
            res = 0;
        } break;
    }
    
    ASSERT(res == 0 || res == 1)
    return res;
}

int NCDEvaluator_Init (NCDEvaluator *o, NCDStringIndex *string_index)
{
    o->string_index = string_index;
    
    if (!NCDEvaluator__VarVec_Init(&o->vars, NCDEVALUATOR_DEFAULT_VARARRAY_CAPACITY)) {
        BLog(BLOG_ERROR, "NCDEvaluator__VarVec_Init failed");
        goto fail0;
    }
    
    if (!NCDEvaluator__CallVec_Init(&o->calls, NCDEVALUATOR_DEFAULT_CALLARRAY_CAPACITY)) {
        BLog(BLOG_ERROR, "NCDEvaluator__CallVec_Init failed");
        goto fail1;
    }
    
    return 1;
    
fail1:
    NCDEvaluator__VarVec_Free(&o->vars);
fail0:
    return 0;
}

void NCDEvaluator_Free (NCDEvaluator *o)
{
    for (size_t i = 0; i < o->vars.count; i++) {
        BFree(o->vars.elems[i].varnames);
    }
    
    for (size_t i = 0; i < o->calls.count; i++) {
        struct NCDEvaluator__Call *call = NCDEvaluator__CallVec_Get(&o->calls, i);
        while (call->num_args-- > 0) {
            expr_free(&call->args[call->num_args]);
        }
        BFree(call->args);
    }
    
    NCDEvaluator__CallVec_Free(&o->calls);
    NCDEvaluator__VarVec_Free(&o->vars);
}

int NCDEvaluatorExpr_Init (NCDEvaluatorExpr *o, NCDEvaluator *eval, NCDValue *value)
{
    return expr_init(&o->expr, eval, value);
}

void NCDEvaluatorExpr_Free (NCDEvaluatorExpr *o)
{
    expr_free(&o->expr);
}

int NCDEvaluatorExpr_Eval (NCDEvaluatorExpr *o, NCDEvaluator *eval, NCDEvaluator_EvalFuncs const *funcs, NCDValMem *out_newmem, NCDValRef *out_val)
{
    ASSERT(funcs)
    ASSERT(out_newmem)
    ASSERT(out_val)
    
    struct NCDEvaluator__eval_context context;
    context.eval = eval;
    context.funcs = funcs;
    
    return expr_eval(&o->expr, &context, out_newmem, out_val);
}

size_t NCDEvaluatorArgs_Count (NCDEvaluatorArgs *o)
{
    struct NCDEvaluator__Call *call = NCDEvaluator__CallVec_Get(&o->context->eval->calls, o->call_index);
    
    return call->num_args;
}

int NCDEvaluatorArgs_EvalArgNewMem (NCDEvaluatorArgs *o, size_t index, NCDValMem *out_newmem, NCDValRef *out_ref)
{
    struct NCDEvaluator__Call *call = NCDEvaluator__CallVec_Get(&o->context->eval->calls, o->call_index);
    ASSERT(index < call->num_args)
    
    return expr_eval(&call->args[index], o->context, out_newmem, out_ref);
}

int NCDEvaluatorArgs_EvalArg (NCDEvaluatorArgs *o, size_t index, NCDValMem *mem, NCDValRef *out_ref)
{
    int res = 0;
    
    NCDValMem temp_mem;
    NCDValRef temp_ref;
    if (!NCDEvaluatorArgs_EvalArgNewMem(o, index, &temp_mem, &temp_ref)) {
        goto fail0;
    }
    
    NCDValRef ref = NCDVal_NewCopy(mem, temp_ref);
    if (NCDVal_IsInvalid(ref)) {
        goto fail1;
    }
    
    *out_ref = ref;
    res = 1;
    
fail1:
    NCDValMem_Free(&temp_mem);
fail0:
    return res;
}
