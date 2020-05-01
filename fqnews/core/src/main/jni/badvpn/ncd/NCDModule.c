/**
 * @file NCDModule.c
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

#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <inttypes.h>
#include <limits.h>

#include <misc/offset.h>
#include <ncd/NCDModule.h>
#include <ncd/static_strings.h>

#define STATE_DEAD 0
#define STATE_DOWN_CLEAN 1
#define STATE_UP 2
#define STATE_DOWN_UNCLEAN 3
#define STATE_DYING 4

#define PROCESS_STATE_INIT 0
#define PROCESS_STATE_DOWN 1
#define PROCESS_STATE_UP 2
#define PROCESS_STATE_DOWN_WAITING 3
#define PROCESS_STATE_TERMINATING 4
#define PROCESS_STATE_TERMINATED 5

static int object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value);
static int object_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object);
static int process_args_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value);
static int process_arg_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value);

static void frontend_event (NCDModuleInst *n, int event)
{
    n->params->func_event(n, event);
}

static void inst_assert_backend (NCDModuleInst *n)
{
    ASSERT(n->state == STATE_DOWN_UNCLEAN || n->state == STATE_DOWN_CLEAN ||
           n->state == STATE_UP ||
           n->state == STATE_DYING)
}

static void set_process_state (NCDModuleProcess *p, int state)
{
#ifndef NDEBUG
    p->state = state;
#endif
}

static NCDObject persistent_obj_retobj (NCDPersistentObj const *pobj)
{
    NCDModuleInst *n = UPPER_OBJECT(pobj, NCDModuleInst, pobj);
    
    return NCDModuleInst_Object(n);
}

void NCDModuleInst_Init (NCDModuleInst *n, const struct NCDInterpModule *m, void *method_context, NCDValRef args, const struct NCDModuleInst_params *params)
{
    ASSERT(m)
    ASSERT(m->module.func_new2)
    ASSERT(m->module.alloc_size >= 0)
    ASSERT(m->base_type_id >= 0)
    ASSERT(m->group)
    ASSERT(n->mem)
    ASSERT(NCDVal_IsList(args))
    ASSERT(params)
    ASSERT(params->func_event)
    ASSERT(params->func_getobj)
    ASSERT(params->logfunc)
    ASSERT(params->iparams)
    ASSERT(params->iparams->func_initprocess)
    ASSERT(params->iparams->func_interp_exit)
    ASSERT(params->iparams->func_interp_getargs)
    ASSERT(params->iparams->func_interp_getretrytime)
    
    // init arguments
    n->m = m;
    n->params = params;
    
    // set initial state
    n->state = STATE_DOWN_CLEAN;
    
    // give NCDModuleInst to methods, not mem
    n->pass_mem_to_methods = 0;
    
    // init persistent object interface
    NCDPersistentObj_Init(&n->pobj, persistent_obj_retobj);
    
    DebugObject_Init(&n->d_obj);
    
    struct NCDModuleInst_new_params new_params;
    new_params.method_user = method_context;
    new_params.args = args;
    
    n->m->module.func_new2(n->mem, n, &new_params);
}

void NCDModuleInst_Free (NCDModuleInst *n)
{
    DebugObject_Free(&n->d_obj);
    ASSERT(n->state == STATE_DEAD)
    
    // free persistent object interface
    NCDPersistentObj_Free(&n->pobj);
}

void NCDModuleInst_Die (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_UP || n->state == STATE_DOWN_CLEAN || n->state == STATE_DOWN_UNCLEAN)
    
    n->state = STATE_DYING;
    
    if (!n->m->module.func_die) {
        NCDModuleInst_Backend_Dead(n);
        return;
    }
    
    n->m->module.func_die(n->mem);
    return;
}

int NCDModuleInst_TryFree (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_UP || n->state == STATE_DOWN_CLEAN || n->state == STATE_DOWN_UNCLEAN)
    
    if (n->m->module.func_die) {
        return 0;
    }
    
    DebugObject_Free(&n->d_obj);
    
    // free persistent object interface
    NCDPersistentObj_Free(&n->pobj);
    
    return 1;
}

void NCDModuleInst_Clean (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_DOWN_CLEAN || n->state == STATE_DOWN_UNCLEAN)
    
    if (n->state == STATE_DOWN_UNCLEAN) {
        n->state = STATE_DOWN_CLEAN;
            
        if (n->m->module.func_clean) {
            n->m->module.func_clean(n->mem);
            return;
        }
    }
}

NCDObject NCDModuleInst_Object (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->m->base_type_id >= 0)
    
    void *method_user = (n->pass_mem_to_methods ? n->mem : n);
    
    return NCDObject_BuildFull(n->m->base_type_id, n, 0, method_user, object_func_getvar, object_func_getobj, &n->pobj);
}

void NCDModuleInst_Backend_PassMemToMethods (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_DOWN_UNCLEAN || n->state == STATE_DOWN_CLEAN ||
           n->state == STATE_UP ||
           n->state == STATE_DYING)
    
    n->pass_mem_to_methods = 1;
}

static int can_resolve (NCDModuleInst *n)
{
    switch (n->state) {
        case STATE_UP:
            return 1;
        case STATE_DOWN_CLEAN:
        case STATE_DOWN_UNCLEAN:
            return !!(n->m->module.flags & NCDMODULE_FLAG_CAN_RESOLVE_WHEN_DOWN);
        default:
            return 0;
    }
}

static int object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value)
{
    NCDModuleInst *n = NCDObject_DataPtr(obj);
    DebugObject_Access(&n->d_obj);
    
    if ((!n->m->module.func_getvar && !n->m->module.func_getvar2) || !can_resolve(n)) {
        return 0;
    }
    
    int res;
    if (n->m->module.func_getvar2) {
        res = n->m->module.func_getvar2(n->mem, name, mem, out_value);
    } else {
        if (NCDStringIndex_HasNulls(n->params->iparams->string_index, name)) {
            return 0;
        }
        const char *name_str = NCDStringIndex_Value(n->params->iparams->string_index, name).ptr;
        res = n->m->module.func_getvar(n->mem, name_str, mem, out_value);
    }
    ASSERT(res == 0 || res == 1)
    ASSERT(res == 0 || (NCDVal_Assert(*out_value), 1))
    
    return res;
}

static int object_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object)
{
    NCDModuleInst *n = NCDObject_DataPtr(obj);
    DebugObject_Access(&n->d_obj);
    
    if (!n->m->module.func_getobj || !can_resolve(n)) {
        return 0;
    }
    
    int res = n->m->module.func_getobj(n->mem, name, out_object);
    ASSERT(res == 0 || res == 1)
    
    return res;
}

void * NCDModuleInst_Backend_GetUser (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_DOWN_UNCLEAN || n->state == STATE_DOWN_CLEAN ||
           n->state == STATE_UP ||
           n->state == STATE_DYING)
    
    return n->mem;
}

void NCDModuleInst_Backend_Up (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_DOWN_CLEAN || n->state == STATE_DOWN_UNCLEAN)
    
    n->state = STATE_UP;
    frontend_event(n, NCDMODULE_EVENT_UP);
}

void NCDModuleInst_Backend_Down (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_UP)
    
    n->state = STATE_DOWN_UNCLEAN;
    frontend_event(n, NCDMODULE_EVENT_DOWN);
}

void NCDModuleInst_Backend_DownUp (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_UP)
    
    frontend_event(n, NCDMODULE_EVENT_DOWNUP);
}

void NCDModuleInst_Backend_Dead (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_DOWN_CLEAN || n->state == STATE_DOWN_UNCLEAN ||
           n->state == STATE_UP || n->state == STATE_DYING)
    
    n->state = STATE_DEAD;
    
    frontend_event(n, NCDMODULE_EVENT_DEAD);
    return;
}

void NCDModuleInst_Backend_DeadError (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_DOWN_CLEAN || n->state == STATE_DOWN_UNCLEAN ||
           n->state == STATE_UP || n->state == STATE_DYING)
    
    n->state = STATE_DEAD;
    
    frontend_event(n, NCDMODULE_EVENT_DEADERROR);
    return;
}

int NCDModuleInst_Backend_GetObj (NCDModuleInst *n, NCD_string_id_t name, NCDObject *out_object)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_DOWN_UNCLEAN || n->state == STATE_DOWN_CLEAN ||
           n->state == STATE_UP ||
           n->state == STATE_DYING)
    ASSERT(out_object)
    
    int res = n->params->func_getobj(n, name, out_object);
    ASSERT(res == 0 || res == 1)
    
    return res;
}

void NCDModuleInst_Backend_Log (NCDModuleInst *n, int channel, int level, const char *fmt, ...)
{
    DebugObject_Access(&n->d_obj);
    
    va_list vl;
    va_start(vl, fmt);
    BLog_LogViaFuncVarArg(n->params->logfunc, n, channel, level, fmt, vl);
    va_end(vl);
}

void NCDModuleInst_Backend_LogVarArg (NCDModuleInst *n, int channel, int level, const char *fmt, va_list vl)
{
    DebugObject_Access(&n->d_obj);
    
    BLog_LogViaFuncVarArg(n->params->logfunc, n, channel, level, fmt, vl);
}

BLogContext NCDModuleInst_Backend_LogContext (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    
    return BLog_MakeContext(n->params->logfunc, n);
}

void NCDModuleInst_Backend_InterpExit (NCDModuleInst *n, int exit_code)
{
    DebugObject_Access(&n->d_obj);
    inst_assert_backend(n);
    
    n->params->iparams->func_interp_exit(n->params->iparams->user, exit_code);
}

int NCDModuleInst_Backend_InterpGetArgs (NCDModuleInst *n, NCDValMem *mem, NCDValRef *out_value)
{
    DebugObject_Access(&n->d_obj);
    inst_assert_backend(n);
    ASSERT(mem)
    ASSERT(out_value)
    
    int res = n->params->iparams->func_interp_getargs(n->params->iparams->user, mem, out_value);
    ASSERT(res == 0 || res == 1)
    ASSERT(res == 0 || (NCDVal_Assert(*out_value), 1))
    
    return res;
}

btime_t NCDModuleInst_Backend_InterpGetRetryTime (NCDModuleInst *n)
{
    DebugObject_Access(&n->d_obj);
    inst_assert_backend(n);
    
    return n->params->iparams->func_interp_getretrytime(n->params->iparams->user);
}

int NCDModuleInst_Backend_InterpLoadGroup (NCDModuleInst *n, const struct NCDModuleGroup *group)
{
    DebugObject_Access(&n->d_obj);
    inst_assert_backend(n);
    ASSERT(group)
    
    return n->params->iparams->func_loadgroup(n->params->iparams->user, group);
}

void NCDModuleRef_Init (NCDModuleRef *o, NCDModuleInst *inst)
{
    ASSERT(!inst->pass_mem_to_methods)
    
    NCDObject obj = NCDModuleInst_Object(inst);
    NCDObjRef_Init(&o->objref, NCDObject_Pobj(&obj));
    
    DebugObject_Init(&o->d_obj);
}

void NCDModuleRef_Free (NCDModuleRef *o)
{
    DebugObject_Free(&o->d_obj);
    
    NCDObjRef_Free(&o->objref);
}

NCDModuleInst * NCDModuleRef_Deref (NCDModuleRef *o)
{
    DebugObject_Access(&o->d_obj);
    
    NCDModuleInst *res = NULL;
    NCDObject obj;
    if (NCDObjRef_Deref(&o->objref, &obj)) {
        res = NCDObject_MethodUser(&obj);
    }
    return res;
}

int NCDModuleProcess_InitId (NCDModuleProcess *o, NCDModuleInst *n, NCD_string_id_t template_name, NCDValRef args, NCDModuleProcess_handler_event handler_event)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_DOWN_UNCLEAN || n->state == STATE_DOWN_CLEAN ||
           n->state == STATE_UP ||
           n->state == STATE_DYING)
    ASSERT(template_name >= 0)
    ASSERT(NCDVal_IsInvalid(args) || NCDVal_IsList(args))
    ASSERT(handler_event)
    
    // init arguments
    o->args = args;
    o->handler_event = handler_event;
    
    // set no special functions
    o->func_getspecialobj = NULL;
    
    // set state
    set_process_state(o, PROCESS_STATE_INIT);
    
#ifndef NDEBUG
    // clear interp functions so we can assert they were set
    o->interp_func_event = NULL;
    o->interp_func_getobj = NULL;
#endif
    
    // init interpreter part
    if (!(n->params->iparams->func_initprocess(n->params->iparams->user, o, template_name))) {
        goto fail1;
    }
    
    ASSERT(o->interp_func_event)
    ASSERT(o->interp_func_getobj)
    
    // set state
    set_process_state(o, PROCESS_STATE_DOWN);
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    return 0;
}

int NCDModuleProcess_InitValue (NCDModuleProcess *o, NCDModuleInst *n, NCDValRef template_name, NCDValRef args, NCDModuleProcess_handler_event handler_event)
{
    DebugObject_Access(&n->d_obj);
    ASSERT(n->state == STATE_DOWN_UNCLEAN || n->state == STATE_DOWN_CLEAN ||
           n->state == STATE_UP ||
           n->state == STATE_DYING)
    ASSERT(NCDVal_IsString(template_name))
    ASSERT(NCDVal_IsInvalid(args) || NCDVal_IsList(args))
    ASSERT(handler_event)
    
    NCD_string_id_t template_name_id;
    
    if (NCDVal_IsIdString(template_name)) {
        template_name_id = NCDVal_IdStringId(template_name);
    } else {
        template_name_id = NCDStringIndex_GetBinMr(n->params->iparams->string_index, NCDVal_StringMemRef(template_name));
        if (template_name_id < 0) {
            BLog(BLOG_ERROR, "NCDStringIndex_GetBin failed");
            return 0;
        }
    }
    
    return NCDModuleProcess_InitId(o, n, template_name_id, args, handler_event);
}

void NCDModuleProcess_Free (NCDModuleProcess *o)
{
    DebugObject_Free(&o->d_obj);
    ASSERT(o->state == PROCESS_STATE_TERMINATED)
}

void NCDModuleProcess_AssertFree (NCDModuleProcess *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state == PROCESS_STATE_TERMINATED)
}

void NCDModuleProcess_SetSpecialFuncs (NCDModuleProcess *o, NCDModuleProcess_func_getspecialobj func_getspecialobj)
{
    DebugObject_Access(&o->d_obj);
    
    o->func_getspecialobj = func_getspecialobj;
}

void NCDModuleProcess_Continue (NCDModuleProcess *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state == PROCESS_STATE_DOWN_WAITING)
    
    set_process_state(o, PROCESS_STATE_DOWN);
    
    o->interp_func_event(o->interp_user, NCDMODULEPROCESS_INTERP_EVENT_CONTINUE);
}

void NCDModuleProcess_Terminate (NCDModuleProcess *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state == PROCESS_STATE_DOWN || o->state == PROCESS_STATE_UP ||
           o->state == PROCESS_STATE_DOWN_WAITING)
    
    set_process_state(o, PROCESS_STATE_TERMINATING);
    
    o->interp_func_event(o->interp_user, NCDMODULEPROCESS_INTERP_EVENT_TERMINATE);
}

int NCDModuleProcess_GetObj (NCDModuleProcess *o, NCD_string_id_t name, NCDObject *out_object)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state != PROCESS_STATE_INIT)
    ASSERT(o->state != PROCESS_STATE_TERMINATED)
    ASSERT(out_object)
    
    int res = o->interp_func_getobj(o->interp_user, name, out_object);
    ASSERT(res == 0 || res == 1)
    
    return res;
}

static void process_assert_interp (NCDModuleProcess *o)
{
    // assert that the interpreter knows about the object, and we're not in init
    ASSERT(o->state == PROCESS_STATE_DOWN || o->state == PROCESS_STATE_UP ||
           o->state == PROCESS_STATE_DOWN_WAITING || o->state == PROCESS_STATE_TERMINATING)
}

void NCDModuleProcess_Interp_SetHandlers (NCDModuleProcess *o, void *interp_user,
                                          NCDModuleProcess_interp_func_event interp_func_event,
                                          NCDModuleProcess_interp_func_getobj interp_func_getobj)
{
    ASSERT(o->state == PROCESS_STATE_INIT)
    ASSERT(interp_func_event)
    ASSERT(interp_func_getobj)
    
    o->interp_user = interp_user;
    o->interp_func_event = interp_func_event;
    o->interp_func_getobj = interp_func_getobj;
}

void NCDModuleProcess_Interp_Up (NCDModuleProcess *o)
{
    DebugObject_Access(&o->d_obj);
    process_assert_interp(o);
    ASSERT(o->state == PROCESS_STATE_DOWN)
    
    set_process_state(o, PROCESS_STATE_UP);
    
    o->handler_event(o, NCDMODULEPROCESS_EVENT_UP);
    return;
}

void NCDModuleProcess_Interp_Down (NCDModuleProcess *o)
{
    DebugObject_Access(&o->d_obj);
    process_assert_interp(o);
    ASSERT(o->state == PROCESS_STATE_UP)
    
    set_process_state(o, PROCESS_STATE_DOWN_WAITING);
    
    o->handler_event(o, NCDMODULEPROCESS_EVENT_DOWN);
    return;
}

void NCDModuleProcess_Interp_Terminated (NCDModuleProcess *o)
{
    DebugObject_Access(&o->d_obj);
    process_assert_interp(o);
    ASSERT(o->state == PROCESS_STATE_TERMINATING)
    
    set_process_state(o, PROCESS_STATE_TERMINATED);
    
    o->handler_event(o, NCDMODULEPROCESS_EVENT_TERMINATED);
    return;
}

int NCDModuleProcess_Interp_GetSpecialObj (NCDModuleProcess *o, NCD_string_id_t name, NCDObject *out_object)
{
    DebugObject_Access(&o->d_obj);
    process_assert_interp(o);
    ASSERT(out_object)
    
    if (!NCDVal_IsInvalid(o->args)) {
        if (name == NCD_STRING_ARGS) {
            *out_object = NCDObject_Build(-1, o, process_args_object_func_getvar, NCDObject_no_getobj);
            return 1;
        }
        
        if (name >= NCD_STRING_ARG0 && name <= NCD_STRING_ARG19) {
            int num = name - NCD_STRING_ARG0;
            if (num < NCDVal_ListCount(o->args)) {
                *out_object = NCDObject_BuildFull(-1, o, num, NULL, process_arg_object_func_getvar, NCDObject_no_getobj, NULL);
                return 1;
            }
        }
    }
    
    if (!o->func_getspecialobj) {
        return 0;
    }
    
    int res = o->func_getspecialobj(o, name, out_object);
    ASSERT(res == 0 || res == 1)
    
    return res;
}

int NCDCall_DoIt (
    struct NCDCall_interp_shared const *interp_shared, void *interp_user,
    struct NCDInterpFunction const *interp_function,
    size_t arg_count, NCDValMem *res_mem, NCDValRef *res_out
)
{
    NCDValRef res = NCDVal_NewInvalid();
    
    NCDCall call;
    call.interp_shared = interp_shared;
    call.interp_user = interp_user;
    call.interp_function = interp_function;
    call.arg_count = arg_count;
    call.res_mem = res_mem;
    call.out_ref = &res;
    
    interp_function->function.func_eval(call);
    
    if (NCDVal_IsInvalid(res)) {
        return 0;
    }
    
    *res_out = res;
    return 1;
}

struct NCDInterpFunction const * NCDCall_InterpFunction (NCDCall const *o)
{
    return o->interp_function;
}

struct NCDModuleInst_iparams const * NCDCall_Iparams (NCDCall const *o)
{
    return o->interp_shared->iparams;
}

size_t NCDCall_ArgCount (NCDCall const *o)
{
    return o->arg_count;
}

NCDValRef NCDCall_EvalArg (NCDCall const *o, size_t index, NCDValMem *mem)
{
    ASSERT(index < o->arg_count)
    ASSERT(mem)
    
    NCDValRef res;
    
    int eval_res = o->interp_shared->func_eval_arg(o->interp_user, index, mem, &res);
    
    ASSERT(eval_res == 0 || eval_res == 1)
    ASSERT(eval_res == 0 || (NCDVal_Assert(res), 1))
    
    if (!eval_res) {
        res = NCDVal_NewInvalid();
    }
    
    return res;
}

NCDValMem * NCDCall_ResMem (NCDCall const *o)
{
    return o->res_mem;
}

void NCDCall_SetResult (NCDCall const *o, NCDValRef ref)
{
    *o->out_ref = ref;
}

BLogContext NCDCall_LogContext (NCDCall const *o)
{
    return BLog_MakeContext(o->interp_shared->logfunc, o->interp_user);
}

static int process_args_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value)
{
    NCDModuleProcess *o = NCDObject_DataPtr(obj);
    DebugObject_Access(&o->d_obj);
    process_assert_interp(o);
    ASSERT(!NCDVal_IsInvalid(o->args))
    
    if (name != NCD_STRING_EMPTY) {
        return 0;
    }
    
    *out_value = NCDVal_NewCopy(mem, o->args);
    if (NCDVal_IsInvalid(*out_value)) {
        BLog_LogToChannel(BLOG_CHANNEL_NCDModuleProcess, BLOG_ERROR, "NCDVal_NewCopy failed");
    }
    return 1;
}

static int process_arg_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value)
{
    NCDModuleProcess *o = NCDObject_DataPtr(obj);
    DebugObject_Access(&o->d_obj);
    process_assert_interp(o);
    ASSERT(!NCDVal_IsInvalid(o->args))
    
    if (name != NCD_STRING_EMPTY) {
        return 0;
    }
    
    *out_value = NCDVal_NewCopy(mem, NCDVal_ListGet(o->args, NCDObject_DataInt(obj)));
    if (NCDVal_IsInvalid(*out_value)) {
        BLog_LogToChannel(BLOG_CHANNEL_NCDModuleProcess, BLOG_ERROR, "NCDVal_NewCopy failed");
    }
    return 1;
}
