/**
 * @file NCDInterpreter.c
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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>

#include <misc/offset.h>
#include <misc/balloc.h>
#include <misc/expstring.h>
#include <base/BLog.h>
#include <ncd/NCDSugar.h>
#include <ncd/modules/modules.h>

#include "NCDInterpreter.h"

#include <generated/blog_channel_ncd.h>

#define SSTATE_CHILD 0
#define SSTATE_ADULT 1
#define SSTATE_DYING 2
#define SSTATE_FORGOTTEN 3

#define PSTATE_WORKING 0
#define PSTATE_UP 1
#define PSTATE_WAITING 2
#define PSTATE_TERMINATING 3

struct statement {
    NCDModuleInst inst;
    NCDValMem args_mem;
    int mem_size;
    int i;
};

struct process {
    NCDInterpreter *interp;
    BReactor *reactor;
    NCDInterpProcess *iprocess;
    NCDModuleProcess *module_process;
    BSmallTimer wait_timer;
    BSmallPending work_job;
    LinkedList1Node list_node; // node in processes
    int ap;
    int fp;
    int num_statements;
    unsigned int error:1;
    unsigned int have_alloc:1;
#ifndef NDEBUG
    int state;
#endif
    struct statement statements[];
};

struct func_call_context {
    struct statement *ps;
    NCDEvaluatorArgs *args;
};

static void start_terminate (NCDInterpreter *interp, int exit_code);
static char * implode_id_strings (NCDInterpreter *interp, const NCD_string_id_t *names, size_t num_names, char del);
static void clear_process_cache (NCDInterpreter *interp);
static struct process * process_allocate (NCDInterpreter *interp, NCDInterpProcess *iprocess);
static void process_release (struct process *p, int no_push);
static void process_assert_statements_cleared (struct process *p);
static int process_new (NCDInterpreter *interp, NCDInterpProcess *iprocess, NCDModuleProcess *module_process);
static void process_free (struct process *p, NCDModuleProcess **out_mp);
static void process_set_state (struct process *p, int state);
static void process_start_terminating (struct process *p);
static int process_have_child (struct process *p);
static void process_assert_pointers (struct process *p);
static void process_logfunc (struct process *p);
static void process_log (struct process *p, int level, const char *fmt, ...);
static void process_work_job_handler_working (struct process *p);
static void process_work_job_handler_up (struct process *p);
static void process_work_job_handler_waiting (struct process *p);
static void process_work_job_handler_terminating (struct process *p);
static int eval_func_eval_var (void *user, NCD_string_id_t const *varnames, size_t num_names, NCDValMem *mem, NCDValRef *out);
static int eval_func_eval_call (void *user, NCD_string_id_t func_name_id, NCDEvaluatorArgs args, NCDValMem *mem, NCDValRef *out);
static void process_advance (struct process *p);
static void process_wait_timer_handler (BSmallTimer *timer);
static int process_find_object (struct process *p, int pos, NCD_string_id_t name, NCDObject *out_object);
static int process_resolve_object_expr (struct process *p, int pos, const NCD_string_id_t *names, size_t num_names, NCDObject *out_object);
static int process_resolve_variable_expr (struct process *p, int pos, const NCD_string_id_t *names, size_t num_names, NCDValMem *mem, NCDValRef *out_value);
static void statement_logfunc (struct statement *ps);
static void statement_log (struct statement *ps, int level, const char *fmt, ...);
static struct process * statement_process (struct statement *ps);
static int statement_mem_is_allocated (struct statement *ps);
static int statement_mem_size (struct statement *ps);
static int statement_allocate_memory (struct statement *ps, int alloc_size);
static void statement_instance_func_event (NCDModuleInst *inst, int event);
static int statement_instance_func_getobj (NCDModuleInst *inst, NCD_string_id_t objname, NCDObject *out_object);
static int statement_instance_func_initprocess (void *vinterp, NCDModuleProcess *mp, NCD_string_id_t template_name);
static void statement_instance_logfunc (NCDModuleInst *inst);
static void statement_instance_func_interp_exit (void *vinterp, int exit_code);
static int statement_instance_func_interp_getargs (void *vinterp, NCDValMem *mem, NCDValRef *out_value);
static btime_t statement_instance_func_interp_getretrytime (void *vinterp);
static int statement_instance_func_interp_loadgroup (void *vinterp, const struct NCDModuleGroup *group);
static void process_moduleprocess_func_event (struct process *p, int event);
static int process_moduleprocess_func_getobj (struct process *p, NCD_string_id_t name, NCDObject *out_object);
static void function_logfunc (void *user);
static int function_eval_arg (void *user, size_t index, NCDValMem *mem, NCDValRef *out);

#define STATEMENT_LOG(ps, channel, ...) if (BLog_WouldLog(BLOG_CURRENT_CHANNEL, channel)) statement_log(ps, channel, __VA_ARGS__)

int NCDInterpreter_Init (NCDInterpreter *o, NCDProgram program, struct NCDInterpreter_params params)
{
    ASSERT(!NCDProgram_ContainsElemType(&program, NCDPROGRAMELEM_INCLUDE));
    ASSERT(!NCDProgram_ContainsElemType(&program, NCDPROGRAMELEM_INCLUDE_GUARD));
    ASSERT(params.handler_finished);
    ASSERT(params.num_extra_args >= 0);
    ASSERT(params.reactor);
#ifndef BADVPN_NO_PROCESS
    ASSERT(params.manager);
#endif
#ifndef BADVPN_NO_UDEV
    ASSERT(params.umanager);
#endif
#ifndef BADVPN_NO_RANDOM
    ASSERT(params.random2);
#endif
    
    // set params
    o->params = params;
    
    // set not terminating
    o->terminating = 0;
    
    // set program
    o->program = program;
    
    // init string index
    if (!NCDStringIndex_Init(&o->string_index)) {
        BLog(BLOG_ERROR, "NCDStringIndex_Init failed");
        goto fail0;
    }
    
    // init module index
    if (!NCDModuleIndex_Init(&o->mindex, &o->string_index)) {
        BLog(BLOG_ERROR, "NCDModuleIndex_Init failed");
        goto fail2;
    }
    
    // init pointers to global resources in out struct NCDModuleInst_iparams.
    // Don't initialize any callback at this point as these must not be called
    // from globalinit functions of modules.
    o->module_iparams.reactor = params.reactor;
#ifndef BADVPN_NO_PROCESS
    o->module_iparams.manager = params.manager;
#endif
#ifndef BADVPN_NO_UDEV
    o->module_iparams.umanager = params.umanager;
#endif
#ifndef BADVPN_NO_RANDOM
    o->module_iparams.random2 = params.random2;
#endif
    o->module_iparams.string_index = &o->string_index;
    
    // add module groups to index and allocate string id's for base_type's
    for (const struct NCDModuleGroup **g = ncd_modules; *g; g++) {
        if (!NCDModuleIndex_AddGroup(&o->mindex, *g, &o->module_iparams, &o->string_index)) {
            BLog(BLOG_ERROR, "NCDModuleIndex_AddGroup failed");
            goto fail3;
        }
    }
    
    // desugar
    if (!NCDSugar_Desugar(&o->program)) {
        BLog(BLOG_ERROR, "NCDSugar_Desugar failed");
        goto fail3;
    }
    
    // init expression evaluator
    if (!NCDEvaluator_Init(&o->evaluator, &o->string_index)) {
        BLog(BLOG_ERROR, "NCDEvaluator_Init failed");
        goto fail3;
    }
    
    // init interp program
    if (!NCDInterpProg_Init(&o->iprogram, &o->program, &o->string_index, &o->evaluator, &o->mindex)) {
        BLog(BLOG_ERROR, "NCDInterpProg_Init failed");
        goto fail5;
    }
    
    // init the rest of the module parameters structures
    o->module_params.func_event = statement_instance_func_event;
    o->module_params.func_getobj = statement_instance_func_getobj;
    o->module_params.logfunc = (BLog_logfunc)statement_instance_logfunc;
    o->module_params.iparams = &o->module_iparams;
    o->module_iparams.user = o;
    o->module_iparams.func_initprocess = statement_instance_func_initprocess;
    o->module_iparams.func_interp_exit = statement_instance_func_interp_exit;
    o->module_iparams.func_interp_getargs = statement_instance_func_interp_getargs;
    o->module_iparams.func_interp_getretrytime = statement_instance_func_interp_getretrytime;
    o->module_iparams.func_loadgroup = statement_instance_func_interp_loadgroup;
    o->module_call_shared.logfunc = function_logfunc;
    o->module_call_shared.func_eval_arg = function_eval_arg;
    o->module_call_shared.iparams = &o->module_iparams;
    
    // init processes list
    LinkedList1_Init(&o->processes);
    
    // init processes
    for (NCDProgramElem *elem = NCDProgram_FirstElem(&o->program); elem; elem = NCDProgram_NextElem(&o->program, elem)) {
        ASSERT(NCDProgramElem_Type(elem) == NCDPROGRAMELEM_PROCESS)
        NCDProcess *p = NCDProgramElem_Process(elem);
        
        if (NCDProcess_IsTemplate(p)) {
            continue;
        }
        
        // get string id for process name
        NCD_string_id_t name_id = NCDStringIndex_Lookup(&o->string_index, NCDProcess_Name(p));
        ASSERT(name_id >= 0)
        
        // find iprocess
        NCDInterpProcess *iprocess = NCDInterpProg_FindProcess(&o->iprogram, name_id);
        ASSERT(iprocess)
        
        if (!process_new(o, iprocess, NULL)) {
            BLog(BLOG_ERROR, "failed to initialize process, exiting");
            goto fail7;
        }
    }
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail7:;
    // free processes
    LinkedList1Node *ln;
    while (ln = LinkedList1_GetFirst(&o->processes)) {
        struct process *p = UPPER_OBJECT(ln, struct process, list_node);
        BSmallPending_Unset(&p->work_job, BReactor_PendingGroup(o->params.reactor));
        NCDModuleProcess *mp;
        process_free(p, &mp);
        ASSERT(!mp)
    }
    // clear process cache (process_free() above may push to cache)
    clear_process_cache(o);
    // free interp program
    NCDInterpProg_Free(&o->iprogram);
fail5:
    // free evaluator
    NCDEvaluator_Free(&o->evaluator);
fail3:
    // free module index
    NCDModuleIndex_Free(&o->mindex);
fail2:
    // free string index
    NCDStringIndex_Free(&o->string_index);
fail0:
    // free program AST
    NCDProgram_Free(&o->program);
    return 0;
}

void NCDInterpreter_Free (NCDInterpreter *o)
{
    DebugObject_Free(&o->d_obj);
    // any process that exists must be completely uninitialized
    
    // free processes
    LinkedList1Node *ln;
    while (ln = LinkedList1_GetFirst(&o->processes)) {
        struct process *p = UPPER_OBJECT(ln, struct process, list_node);
        BSmallPending_Unset(&p->work_job, BReactor_PendingGroup(o->params.reactor));
        NCDModuleProcess *mp;
        process_free(p, &mp);
        ASSERT(!mp)
    }
    
    // clear process cache
    clear_process_cache(o);
    
    // free interp program
    NCDInterpProg_Free(&o->iprogram);
    
    // free evaluator
    NCDEvaluator_Free(&o->evaluator);
    
    // free module index
    NCDModuleIndex_Free(&o->mindex);
    
    // free string index
    NCDStringIndex_Free(&o->string_index);
    
    // free program AST
    NCDProgram_Free(&o->program);
}

void NCDInterpreter_RequestShutdown (NCDInterpreter *o, int exit_code)
{
    DebugObject_Access(&o->d_obj);
    
    start_terminate(o, exit_code);
}

void start_terminate (NCDInterpreter *interp, int exit_code)
{
    // remember exit code
    interp->main_exit_code = exit_code;
    
    // if we're already terminating, there's nothing to do
    if (interp->terminating) {
        return;
    }
    
    // set the terminating flag
    interp->terminating = 1;
    
    // if there are no processes, we're done
    if (LinkedList1_IsEmpty(&interp->processes)) {
        interp->params.handler_finished(interp->params.user, interp->main_exit_code);
        return;
    }
    
    // start terminating non-template processes
    for (LinkedList1Node *ln = LinkedList1_GetFirst(&interp->processes); ln; ln = LinkedList1Node_Next(ln)) {
        struct process *p = UPPER_OBJECT(ln, struct process, list_node);
        if (p->module_process) {
            continue;
        }
        process_start_terminating(p);
    }
}

char * implode_id_strings (NCDInterpreter *interp, const NCD_string_id_t *names, size_t num_names, char del)
{
    ExpString str;
    if (!ExpString_Init(&str)) {
        goto fail0;
    }
    
    int is_first = 1;
    
    while (num_names > 0) {
        if (!is_first && !ExpString_AppendChar(&str, del)) {
            goto fail1;
        }
        const char *name_str = NCDStringIndex_Value(&interp->string_index, *names).ptr;
        if (!ExpString_Append(&str, name_str)) {
            goto fail1;
        }
        names++;
        num_names--;
        is_first = 0;
    }
    
    return ExpString_Get(&str);
    
fail1:
    ExpString_Free(&str);
fail0:
    return NULL;
}

void clear_process_cache (NCDInterpreter *interp)
{
    for (NCDProgramElem *elem = NCDProgram_FirstElem(&interp->program); elem; elem = NCDProgram_NextElem(&interp->program, elem)) {
        ASSERT(NCDProgramElem_Type(elem) == NCDPROGRAMELEM_PROCESS)
        NCDProcess *ast_p = NCDProgramElem_Process(elem);
        
        NCD_string_id_t name_id = NCDStringIndex_Lookup(&interp->string_index, NCDProcess_Name(ast_p));
        NCDInterpProcess *iprocess = NCDInterpProg_FindProcess(&interp->iprogram, name_id);
        
        struct process *p;
        while (p = NCDInterpProcess_CachePull(iprocess)) {
            process_release(p, 1);
        }
    }
}

struct process * process_allocate (NCDInterpreter *interp, NCDInterpProcess *iprocess)
{
    ASSERT(iprocess)
    
    // try to pull from cache
    struct process *p = NCDInterpProcess_CachePull(iprocess);
    if (p) {
        goto allocated;
    }
    
    // get number of statements
    int num_statements = NCDInterpProcess_NumStatements(iprocess);
    
    // get size of preallocated memory
    int mem_size = NCDInterpProcess_PreallocSize(iprocess);
    if (mem_size < 0) {
        goto fail0;
    }
    
    // start with size of process structure
    size_t alloc_size = sizeof(struct process);
    
    // add size of statements array
    if (num_statements > SIZE_MAX / sizeof(struct statement)) {
        goto fail0;
    }
    if (!BSizeAdd(&alloc_size, num_statements * sizeof(struct statement))) {
        goto fail0;
    }
    
    // align for preallocated memory
    if (!BSizeAlign(&alloc_size, BMAX_ALIGN)) {
        goto fail0;
    }
    size_t mem_off = alloc_size;
    
    // add size of preallocated memory
    if (mem_size > SIZE_MAX || !BSizeAdd(&alloc_size, mem_size)) {
        goto fail0;
    }
    
    // allocate memory
    p = BAlloc(alloc_size);
    if (!p) {
        goto fail0;
    }
    
    // set variables
    p->interp = interp;
    p->reactor = interp->params.reactor;
    p->iprocess = iprocess;
    p->ap = 0;
    p->fp = 0;
    p->num_statements = num_statements;
    p->error = 0;
    p->have_alloc = 0;
    
    // init statements
    char *mem = (char *)p + mem_off;
    for (int i = 0; i < num_statements; i++) {
        struct statement *ps = &p->statements[i];
        ps->i = i;
        ps->inst.istate = SSTATE_FORGOTTEN;
        ps->mem_size = NCDInterpProcess_StatementPreallocSize(iprocess, i);
        ps->inst.mem = mem + NCDInterpProcess_StatementPreallocOffset(iprocess, i);
    }
    
    // init timer
    BSmallTimer_Init(&p->wait_timer, process_wait_timer_handler);
    
    // init work job
    BSmallPending_Init(&p->work_job, BReactor_PendingGroup(p->reactor), NULL, NULL);
    
allocated:
    ASSERT(p->interp == interp)
    ASSERT(p->reactor == interp->params.reactor)
    ASSERT(p->iprocess == iprocess)
    ASSERT(p->ap == 0)
    ASSERT(p->fp == 0)
    ASSERT(p->num_statements == NCDInterpProcess_NumStatements(iprocess))
    ASSERT(p->error == 0)
    process_assert_statements_cleared(p);
    ASSERT(!BSmallPending_IsSet(&p->work_job))
    ASSERT(!BSmallTimer_IsRunning(&p->wait_timer))
    
    return p;
    
fail0:
    BLog(BLOG_ERROR, "failed to allocate memory for process %s", NCDInterpProcess_Name(iprocess));
    return NULL;
}

void process_release (struct process *p, int no_push)
{
    ASSERT(p->ap == 0)
    ASSERT(p->fp == 0)
    ASSERT(p->error == 0)
    process_assert_statements_cleared(p);
    ASSERT(!BSmallPending_IsSet(&p->work_job))
    ASSERT(!BSmallTimer_IsRunning(&p->wait_timer))
    
    // try to push to cache
    if (!no_push && !p->have_alloc) {
        if (NCDInterpProcess_CachePush(p->iprocess, p)) {
            return;
        }
    }
    
    // free work job
    BSmallPending_Free(&p->work_job, BReactor_PendingGroup(p->reactor));
    
    // free statement memory
    if (p->have_alloc) {
        for (int i = 0; i < p->num_statements; i++) {
            struct statement *ps = &p->statements[i];
            if (statement_mem_is_allocated(ps)) {
                free(ps->inst.mem);
            }
        }
    }
    
    // free strucure
    BFree(p);
}

void process_assert_statements_cleared (struct process *p)
{
#ifndef NDEBUG
    for (int i = 0; i < p->num_statements; i++) {
        ASSERT(p->statements[i].i == i)
        ASSERT(p->statements[i].inst.istate == SSTATE_FORGOTTEN)
    }
#endif
}

int process_new (NCDInterpreter *interp, NCDInterpProcess *iprocess, NCDModuleProcess *module_process)
{
    ASSERT(iprocess)
    
    // allocate prepared process struct
    struct process *p = process_allocate(interp, iprocess);
    if (!p) {
        return 0;
    }
    
    // set module process pointer
    p->module_process = module_process;
    
    // set module process handlers
    if (p->module_process) {
        NCDModuleProcess_Interp_SetHandlers(p->module_process, p,
                                            (NCDModuleProcess_interp_func_event)process_moduleprocess_func_event,
                                            (NCDModuleProcess_interp_func_getobj)process_moduleprocess_func_getobj);
    }
    
    // set state
    process_set_state(p, PSTATE_WORKING);
    BSmallPending_SetHandler(&p->work_job, (BSmallPending_handler)process_work_job_handler_working, p);
    
    // insert to processes list
    LinkedList1_Append(&interp->processes, &p->list_node);
    
    // schedule work
    BSmallPending_Set(&p->work_job, BReactor_PendingGroup(p->reactor));
    
    return 1;
}

void process_set_state (struct process *p, int state)
{
#ifndef NDEBUG
    p->state = state;
#endif
}

void process_free (struct process *p, NCDModuleProcess **out_mp)
{
    ASSERT(p->ap == 0)
    ASSERT(p->fp == 0)
    ASSERT(out_mp)
    ASSERT(!BSmallPending_IsSet(&p->work_job))
    
    // give module process to caller so it can inform the process creator that the process has terminated
    *out_mp = p->module_process;
    
    // remove from processes list
    LinkedList1_Remove(&p->interp->processes, &p->list_node);
    
    // free timer
    BReactor_RemoveSmallTimer(p->reactor, &p->wait_timer);
    
    // clear error
    p->error = 0;
    
    process_release(p, 0);
}

void process_start_terminating (struct process *p)
{
    // set state terminating
    process_set_state(p, PSTATE_TERMINATING);
    BSmallPending_SetHandler(&p->work_job, (BSmallPending_handler)process_work_job_handler_terminating, p);
    
    // schedule work
    BSmallPending_Set(&p->work_job, BReactor_PendingGroup(p->reactor));
}

int process_have_child (struct process *p)
{
    return (p->ap > 0 && p->statements[p->ap - 1].inst.istate == SSTATE_CHILD);
}

void process_assert_pointers (struct process *p)
{
    ASSERT(p->ap <= p->num_statements)
    ASSERT(p->fp >= p->ap)
    ASSERT(p->fp <= p->num_statements)
    
#ifndef NDEBUG
    // check AP
    for (int i = 0; i < p->ap; i++) {
        if (i == p->ap - 1) {
            ASSERT(p->statements[i].inst.istate == SSTATE_ADULT || p->statements[i].inst.istate == SSTATE_CHILD)
        } else {
            ASSERT(p->statements[i].inst.istate == SSTATE_ADULT)
        }
    }
    
    // check FP
    int fp = p->num_statements;
    while (fp > 0 && p->statements[fp - 1].inst.istate == SSTATE_FORGOTTEN) {
        fp--;
    }
    ASSERT(p->fp == fp)
#endif
}

void process_logfunc (struct process *p)
{
    BLog_Append("process %s: ", NCDInterpProcess_Name(p->iprocess));
}

void process_log (struct process *p, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    BLog_LogViaFuncVarArg((BLog_logfunc)process_logfunc, p, BLOG_CURRENT_CHANNEL, level, fmt, vl);
    va_end(vl);
}

void process_work_job_handler_working (struct process *p)
{
    process_assert_pointers(p);
    ASSERT(p->state == PSTATE_WORKING)
    
    // cleaning up?
    if (p->ap < p->fp) {
        // order the last living statement to die, if needed
        struct statement *ps = &p->statements[p->fp - 1];
        if (ps->inst.istate == SSTATE_DYING) {
            return;
        }
        
        STATEMENT_LOG(ps, BLOG_INFO, "killing");
        
        // set statement state DYING
        ps->inst.istate = SSTATE_DYING;
        
        // order it to die
        NCDModuleInst_Die(&ps->inst);
        return;
    }
    
    // clean?
    if (process_have_child(p)) {
        ASSERT(p->ap > 0)
        ASSERT(p->ap <= p->num_statements)
        
        struct statement *ps = &p->statements[p->ap - 1];
        ASSERT(ps->inst.istate == SSTATE_CHILD)
        
        STATEMENT_LOG(ps, BLOG_INFO, "clean");
        
        // report clean
        NCDModuleInst_Clean(&ps->inst);
        return;
    }
    
    // finished?
    if (p->ap == p->num_statements) {
        process_log(p, BLOG_INFO, "victory");
        
        // set state up
        process_set_state(p, PSTATE_UP);
        BSmallPending_SetHandler(&p->work_job, (BSmallPending_handler)process_work_job_handler_up, p);
        
        // set module process up
        if (p->module_process) {
            NCDModuleProcess_Interp_Up(p->module_process);
        }
        return;
    }
    
    // advancing?
    struct statement *ps = &p->statements[p->ap];
    ASSERT(ps->inst.istate == SSTATE_FORGOTTEN)
    
    if (p->error) {
        STATEMENT_LOG(ps, BLOG_INFO, "waiting after error");
        
        // clear error
        p->error = 0;
        
        // set wait timer
        BReactor_SetSmallTimer(p->reactor, &p->wait_timer, BTIMER_SET_RELATIVE, p->interp->params.retry_time);
    } else {
        // advance
        process_advance(p);
    }
}

void process_work_job_handler_up (struct process *p)
{
    process_assert_pointers(p);
    ASSERT(p->state == PSTATE_UP)
    ASSERT(p->ap < p->num_statements || process_have_child(p))
    
    // if we have module process, wait for its permission to continue
    if (p->module_process) {
        // set state waiting
        process_set_state(p, PSTATE_WAITING);
        BSmallPending_SetHandler(&p->work_job, (BSmallPending_handler)process_work_job_handler_waiting, p);
        
        // set module process down
        NCDModuleProcess_Interp_Down(p->module_process);
        return;
    }
    
    // set state working
    process_set_state(p, PSTATE_WORKING);
    BSmallPending_SetHandler(&p->work_job, (BSmallPending_handler)process_work_job_handler_working, p);
    
    // delegate the rest to the working handler
    process_work_job_handler_working(p);
}

void process_work_job_handler_waiting (struct process *p)
{
    process_assert_pointers(p);
    ASSERT(p->state == PSTATE_WAITING)
    
    // do absolutely nothing. Having this no-op handler avoids a branch
    // in statement_instance_func_event().
}

void process_work_job_handler_terminating (struct process *p)
{
    process_assert_pointers(p);
    ASSERT(p->state == PSTATE_TERMINATING)
    
again:
    if (p->fp == 0) {
        NCDInterpreter *interp = p->interp;
        
        // free process
        NCDModuleProcess *mp;
        process_free(p, &mp);
        
        // if program is terminating amd there are no more processes, exit program
        if (interp->terminating && LinkedList1_IsEmpty(&interp->processes)) {
            ASSERT(!mp)
            interp->params.handler_finished(interp->params.user, interp->main_exit_code);
            return;
        }
        
        // inform the process creator that the process has terminated
        if (mp) {
            NCDModuleProcess_Interp_Terminated(mp);
            return;
        }
        
        return;
    }
    
    // order the last living statement to die, if needed
    struct statement *ps = &p->statements[p->fp - 1];
    ASSERT(ps->inst.istate != SSTATE_FORGOTTEN)
    if (ps->inst.istate == SSTATE_DYING) {
        return;
    }
    
    STATEMENT_LOG(ps, BLOG_INFO, "killing");
    
    // update AP
    if (p->ap > ps->i) {
        p->ap = ps->i;
    }
    
    // optimize for statements which can be destroyed immediately
    if (NCDModuleInst_TryFree(&ps->inst)) {
        STATEMENT_LOG(ps, BLOG_INFO, "died");
        
        // free arguments memory
        NCDValMem_Free(&ps->args_mem);
        
        // set statement state FORGOTTEN
        ps->inst.istate = SSTATE_FORGOTTEN;
        
        // update FP
        while (p->fp > 0 && p->statements[p->fp - 1].inst.istate == SSTATE_FORGOTTEN) {
            p->fp--;
        }
        
        goto again;
    }
    
    // set statement state DYING
    ps->inst.istate = SSTATE_DYING;
    
    // order it to die
    NCDModuleInst_Die(&ps->inst);
    return;
}

int eval_func_eval_var (void *user, NCD_string_id_t const *varnames, size_t num_names, NCDValMem *mem, NCDValRef *out)
{
    struct process *p = user;
    ASSERT(varnames)
    ASSERT(num_names > 0)
    ASSERT(mem)
    ASSERT(out)
    
    return process_resolve_variable_expr(p, p->ap, varnames, num_names, mem, out);
}

static int eval_func_eval_call (void *user, NCD_string_id_t func_name_id, NCDEvaluatorArgs args, NCDValMem *mem, NCDValRef *out)
{
    struct process *p = user;
    struct statement *ps = &p->statements[p->ap];
    
    struct NCDInterpFunction const *ifunc = NCDModuleIndex_FindFunction(&p->interp->mindex, func_name_id);
    if (!ifunc) {
        STATEMENT_LOG(ps, BLOG_ERROR, "unknown function: %s", NCDStringIndex_Value(&p->interp->string_index, func_name_id).ptr);
        return 0;
    }
    
    struct func_call_context context;
    context.ps = ps;
    context.args = &args;
    
    return NCDCall_DoIt(&p->interp->module_call_shared, &context, ifunc, NCDEvaluatorArgs_Count(&args), mem, out);
}

void process_advance (struct process *p)
{
    process_assert_pointers(p);
    ASSERT(p->ap == p->fp)
    ASSERT(!process_have_child(p))
    ASSERT(p->ap < p->num_statements)
    ASSERT(!p->error)
    ASSERT(!BSmallPending_IsSet(&p->work_job))
    ASSERT(p->state == PSTATE_WORKING)
    
    struct statement *ps = &p->statements[p->ap];
    ASSERT(ps->inst.istate == SSTATE_FORGOTTEN)
    
    STATEMENT_LOG(ps, BLOG_INFO, "initializing");
    
    // need to determine the module and object to use it on (if it's a method)
    const struct NCDInterpModule *module;
    void *method_context = NULL;
    
    // get object names, e.g. "my.cat" in "my.cat->meow();"
    // (or NULL if this is not a method statement)
    const NCD_string_id_t *objnames;
    size_t num_objnames;
    NCDInterpProcess_StatementObjNames(p->iprocess, p->ap, &objnames, &num_objnames);
    
    if (!objnames) {
        // not a method; module is already known by NCDInterpProcess
        module = NCDInterpProcess_StatementGetSimpleModule(p->iprocess, p->ap, &p->interp->string_index, &p->interp->mindex);
        
        if (!module) {
            const char *cmdname_str = NCDInterpProcess_StatementCmdName(p->iprocess, p->ap, &p->interp->string_index);
            STATEMENT_LOG(ps, BLOG_ERROR, "unknown simple statement: %s", cmdname_str);
            goto fail0;
        }
    } else {
        // get object
        NCDObject object;
        if (!process_resolve_object_expr(p, p->ap, objnames, num_objnames, &object)) {
            goto fail0;
        }
        
        // get object type
        NCD_string_id_t object_type = NCDObject_Type(&object);
        if (object_type < 0) {
            STATEMENT_LOG(ps, BLOG_ERROR, "cannot call method on object with no type");
            goto fail0;
        }
        
        // get method context
        method_context = NCDObject_MethodUser(&object);
        
        // find module based on type of object
        module = NCDInterpProcess_StatementGetMethodModule(p->iprocess, p->ap, object_type, &p->interp->mindex);
        
        if (!module) {
            const char *type_str = NCDStringIndex_Value(&p->interp->string_index, object_type).ptr;
            const char *cmdname_str = NCDInterpProcess_StatementCmdName(p->iprocess, p->ap, &p->interp->string_index);
            STATEMENT_LOG(ps, BLOG_ERROR, "unknown method statement: %s::%s", type_str, cmdname_str);
            goto fail0;
        }
    }
    
    // get evaluator expression for the arguments
    NCDEvaluatorExpr *expr = NCDInterpProcess_GetStatementArgsExpr(p->iprocess, ps->i);
    
    // evaluate arguments
    NCDValRef args;
    NCDEvaluator_EvalFuncs funcs = {p, eval_func_eval_var, eval_func_eval_call};
    if (!NCDEvaluatorExpr_Eval(expr, &p->interp->evaluator, &funcs, &ps->args_mem, &args)) {
        STATEMENT_LOG(ps, BLOG_ERROR, "failed to evaluate arguments");
        goto fail0;
    }
    
    // allocate memory
    if (!statement_allocate_memory(ps, module->module.alloc_size)) {
        STATEMENT_LOG(ps, BLOG_ERROR, "failed to allocate memory");
        goto fail1;
    }
    
    // set statement state CHILD
    ps->inst.istate = SSTATE_CHILD;
    
    // increment AP
    p->ap++;
    
    // increment FP
    p->fp++;
    
    process_assert_pointers(p);
    
    // initialize module instance
    NCDModuleInst_Init(&ps->inst, module, method_context, args, &p->interp->module_params);
    return;
    
fail1:
    NCDValMem_Free(&ps->args_mem);
fail0:
    // set error
    p->error = 1;
    
    // schedule work to start the timer
    BSmallPending_Set(&p->work_job, BReactor_PendingGroup(p->reactor));
}

void process_wait_timer_handler (BSmallTimer *timer)
{
    struct process *p = UPPER_OBJECT(timer, struct process, wait_timer);
    process_assert_pointers(p);
    ASSERT(!BSmallPending_IsSet(&p->work_job))
    
    // check if something happened that means we no longer need to retry
    if (p->ap != p->fp || process_have_child(p) || p->ap == p->num_statements) {
        return;
    }
    
    process_log(p, BLOG_INFO, "retrying");
    
    // advance. Note: the asserts for this are indeed satisfied, though this
    // it not trivial to prove.
    process_advance(p);
}

int process_find_object (struct process *p, int pos, NCD_string_id_t name, NCDObject *out_object)
{
    ASSERT(pos >= 0)
    ASSERT(pos <= p->num_statements)
    ASSERT(out_object)
    
    int i = NCDInterpProcess_FindStatement(p->iprocess, pos, name);
    if (i >= 0) {
        struct statement *ps = &p->statements[i];
        ASSERT(i < p->num_statements)
        
        if (ps->inst.istate == SSTATE_FORGOTTEN) {
            process_log(p, BLOG_ERROR, "statement (%d) is uninitialized", i);
            return 0;
        }
        
        *out_object = NCDModuleInst_Object(&ps->inst);
        return 1;
    }
    
    if (p->module_process && NCDModuleProcess_Interp_GetSpecialObj(p->module_process, name, out_object)) {
        return 1;
    }
    
    return 0;
}

int process_resolve_object_expr (struct process *p, int pos, const NCD_string_id_t *names, size_t num_names, NCDObject *out_object)
{
    ASSERT(pos >= 0)
    ASSERT(pos <= p->num_statements)
    ASSERT(names)
    ASSERT(num_names > 0)
    ASSERT(out_object)
    
    NCDObject object;
    if (!process_find_object(p, pos, names[0], &object)) {
        goto fail;
    }
    
    if (!NCDObject_ResolveObjExprCompact(&object, names + 1, num_names - 1, out_object)) {
        goto fail;
    }
    
    return 1;
    
fail:;
    char *name = implode_id_strings(p->interp, names, num_names, '.');
    process_log(p, BLOG_ERROR, "failed to resolve object (%s) from position %zu", (name ? name : ""), pos);
    free(name);
    return 0;
}

int process_resolve_variable_expr (struct process *p, int pos, const NCD_string_id_t *names, size_t num_names, NCDValMem *mem, NCDValRef *out_value)
{
    ASSERT(pos >= 0)
    ASSERT(pos <= p->num_statements)
    ASSERT(names)
    ASSERT(num_names > 0)
    ASSERT(mem)
    ASSERT(out_value)
    
    NCDObject object;
    if (!process_find_object(p, pos, names[0], &object)) {
        goto fail;
    }
    
    if (!NCDObject_ResolveVarExprCompact(&object, names + 1, num_names - 1, mem, out_value)) {
        goto fail;
    }
    
    return 1;
    
fail:;
    char *name = implode_id_strings(p->interp, names, num_names, '.');
    process_log(p, BLOG_ERROR, "failed to resolve variable (%s) from position %zu", (name ? name : ""), pos);
    free(name);
    return 0;
}

void statement_logfunc (struct statement *ps)
{
    process_logfunc(statement_process(ps));
    BLog_Append("statement %zu: ", ps->i);
}

void statement_log (struct statement *ps, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    BLog_LogViaFuncVarArg((BLog_logfunc)statement_logfunc, ps, BLOG_CURRENT_CHANNEL, level, fmt, vl);
    va_end(vl);
}

struct process * statement_process (struct statement *ps)
{
    return UPPER_OBJECT(ps - ps->i, struct process, statements);
}

int statement_mem_is_allocated (struct statement *ps)
{
    return (ps->mem_size < 0);
}

int statement_mem_size (struct statement *ps)
{
    return (ps->mem_size >= 0 ? ps->mem_size : -ps->mem_size);
}

int statement_allocate_memory (struct statement *ps, int alloc_size)
{
    ASSERT(alloc_size >= 0)
    
    if (alloc_size > statement_mem_size(ps)) {
        // allocate new memory
        char *new_mem = malloc(alloc_size);
        if (!new_mem) {
            STATEMENT_LOG(ps, BLOG_ERROR, "malloc failed");
            return 0;
        }
        
        // release old memory unless it was preallocated
        if (statement_mem_is_allocated(ps)) {
            free(ps->inst.mem);
        }
        
        struct process *p = statement_process(ps);
        
        // register memory in statement
        ps->inst.mem = new_mem;
        ps->mem_size = -alloc_size;
        
        // set the alloc flag in the process to make sure process_free()
        // releases the allocated memory
        p->have_alloc = 1;
        
        // register alloc size for future preallocations
        NCDInterpProcess_StatementBumpAllocSize(p->iprocess, ps->i, alloc_size);
    }
    
    return 1;
}

void statement_instance_func_event (NCDModuleInst *inst, int event)
{
    struct statement *ps = UPPER_OBJECT(inst, struct statement, inst);
    ASSERT(ps->inst.istate == SSTATE_CHILD || ps->inst.istate == SSTATE_ADULT || ps->inst.istate == SSTATE_DYING)
    
    struct process *p = statement_process(ps);
    process_assert_pointers(p);
    
    // schedule work
    BSmallPending_Set(&p->work_job, BReactor_PendingGroup(p->reactor));
    
    switch (event) {
        case NCDMODULE_EVENT_UP: {
            ASSERT(ps->inst.istate == SSTATE_CHILD)
            
            STATEMENT_LOG(ps, BLOG_INFO, "up");
            
            // set state ADULT
            ps->inst.istate = SSTATE_ADULT;
        } break;
        
        case NCDMODULE_EVENT_DOWN: {
            ASSERT(ps->inst.istate == SSTATE_ADULT)
            
            STATEMENT_LOG(ps, BLOG_INFO, "down");
            
            // set state CHILD
            ps->inst.istate = SSTATE_CHILD;
            
            // clear error
            if (ps->i < p->ap) {
                p->error = 0;
            }
            
            // update AP
            if (p->ap > ps->i + 1) {
                p->ap = ps->i + 1;
            }
        } break;
        
        case NCDMODULE_EVENT_DOWNUP: {
            ASSERT(ps->inst.istate == SSTATE_ADULT)
            
            STATEMENT_LOG(ps, BLOG_INFO, "down");
            STATEMENT_LOG(ps, BLOG_INFO, "up");
            
            // clear error
            if (ps->i < p->ap) {
                p->error = 0;
            }
            
            // update AP
            if (p->ap > ps->i + 1) {
                p->ap = ps->i + 1;
            }
        } break;
        
        case NCDMODULE_EVENT_DEAD: {
            STATEMENT_LOG(ps, BLOG_INFO, "died");
            
            // free instance
            NCDModuleInst_Free(&ps->inst);
            
            // free arguments memory
            NCDValMem_Free(&ps->args_mem);
            
            // set state FORGOTTEN
            ps->inst.istate = SSTATE_FORGOTTEN;
            
            // update AP
            if (p->ap > ps->i) {
                p->ap = ps->i;
            }
            
            // update FP
            while (p->fp > 0 && p->statements[p->fp - 1].inst.istate == SSTATE_FORGOTTEN) {
                p->fp--;
            }
        } break;
        
        case NCDMODULE_EVENT_DEADERROR: {
            STATEMENT_LOG(ps, BLOG_ERROR, "died with error");
            
            // free instance
            NCDModuleInst_Free(&ps->inst);
            
            // free arguments memory
            NCDValMem_Free(&ps->args_mem);
            
            // set state FORGOTTEN
            ps->inst.istate = SSTATE_FORGOTTEN;
            
            // set error
            if (ps->i < p->ap) {
                p->error = 1;
            }
            
            // update AP
            if (p->ap > ps->i) {
                p->ap = ps->i;
            }
            
            // update FP
            while (p->fp > 0 && p->statements[p->fp - 1].inst.istate == SSTATE_FORGOTTEN) {
                p->fp--;
            }
        } break;
    }
}

int statement_instance_func_getobj (NCDModuleInst *inst, NCD_string_id_t objname, NCDObject *out_object)
{
    struct statement *ps = UPPER_OBJECT(inst, struct statement, inst);
    ASSERT(ps->inst.istate != SSTATE_FORGOTTEN)
    
    return process_find_object(statement_process(ps), ps->i, objname, out_object);
}

int statement_instance_func_initprocess (void *vinterp, NCDModuleProcess* mp, NCD_string_id_t template_name)
{
    NCDInterpreter *interp = vinterp;
    
    // find process
    NCDInterpProcess *iprocess = NCDInterpProg_FindProcess(&interp->iprogram, template_name);
    if (!iprocess) {
        const char *str = NCDStringIndex_Value(&interp->string_index, template_name).ptr;
        BLog(BLOG_ERROR, "no template named %s", str);
        return 0;
    }
    
    // make sure it's a template
    if (!NCDInterpProcess_IsTemplate(iprocess)) {
        const char *str = NCDStringIndex_Value(&interp->string_index, template_name).ptr;
        BLog(BLOG_ERROR, "need template to create a process, but %s is a process", str);
        return 0;
    }
    
    // create process
    if (!process_new(interp, iprocess, mp)) {
        const char *str = NCDStringIndex_Value(&interp->string_index, template_name).ptr;
        BLog(BLOG_ERROR, "failed to create process from template %s", str);
        return 0;
    }
    
    if (BLog_WouldLog(BLOG_INFO, BLOG_CURRENT_CHANNEL)) {
        const char *str = NCDStringIndex_Value(&interp->string_index, template_name).ptr;
        BLog(BLOG_INFO, "created process from template %s", str);
    }
    
    return 1;
}

void statement_instance_logfunc (NCDModuleInst *inst)
{
    struct statement *ps = UPPER_OBJECT(inst, struct statement, inst);
    ASSERT(ps->inst.istate != SSTATE_FORGOTTEN)
    
    statement_logfunc(ps);
    BLog_Append("module: ");
}

void statement_instance_func_interp_exit (void *vinterp, int exit_code)
{
    NCDInterpreter *interp = vinterp;
    
    start_terminate(interp, exit_code);
}

int statement_instance_func_interp_getargs (void *vinterp, NCDValMem *mem, NCDValRef *out_value)
{
    NCDInterpreter *interp = vinterp;
    
    *out_value = NCDVal_NewList(mem, interp->params.num_extra_args);
    if (NCDVal_IsInvalid(*out_value)) {
        BLog(BLOG_ERROR, "NCDVal_NewList failed");
        goto fail;
    }
    
    for (int i = 0; i < interp->params.num_extra_args; i++) {
        NCDValRef arg = NCDVal_NewString(mem, interp->params.extra_args[i]);
        if (NCDVal_IsInvalid(arg)) {
            BLog(BLOG_ERROR, "NCDVal_NewString failed");
            goto fail;
        }
        
        if (!NCDVal_ListAppend(*out_value, arg)) {
            BLog(BLOG_ERROR, "depth limit exceeded");
            goto fail;
        }
    }
    
    return 1;
    
fail:
    *out_value = NCDVal_NewInvalid();
    return 1;
}

btime_t statement_instance_func_interp_getretrytime (void *vinterp)
{
    NCDInterpreter *interp = vinterp;
    
    return interp->params.retry_time;
}

int statement_instance_func_interp_loadgroup (void *vinterp, const struct NCDModuleGroup *group)
{
    NCDInterpreter *interp = vinterp;
    
    if (!NCDModuleIndex_AddGroup(&interp->mindex, group, &interp->module_iparams, &interp->string_index)) {
        BLog(BLOG_ERROR, "NCDModuleIndex_AddGroup failed");
        return 0;
    }
    
    return 1;
}

void process_moduleprocess_func_event (struct process *p, int event)
{
    ASSERT(p->module_process)
    
    switch (event) {
        case NCDMODULEPROCESS_INTERP_EVENT_CONTINUE: {
            ASSERT(p->state == PSTATE_WAITING)
            
            // set state working
            process_set_state(p, PSTATE_WORKING);
            BSmallPending_SetHandler(&p->work_job, (BSmallPending_handler)process_work_job_handler_working, p);
            
            // schedule work
            BSmallPending_Set(&p->work_job, BReactor_PendingGroup(p->reactor));
        } break;
        
        case NCDMODULEPROCESS_INTERP_EVENT_TERMINATE: {
            ASSERT(p->state != PSTATE_TERMINATING)
            
            process_log(p, BLOG_INFO, "process termination requested");
        
            // start terminating
            process_start_terminating(p);
        } break;
        
        default: ASSERT(0);
    }
}

int process_moduleprocess_func_getobj (struct process *p, NCD_string_id_t name, NCDObject *out_object)
{
    ASSERT(p->module_process)
    
    return process_find_object(p, p->num_statements, name, out_object);
}

void function_logfunc (void *user)
{
    struct func_call_context const *context = user;
    ASSERT(context->ps->inst.istate == SSTATE_FORGOTTEN)
    
    statement_logfunc(context->ps);
    BLog_Append("func_eval: ");
}

int function_eval_arg (void *user, size_t index, NCDValMem *mem, NCDValRef *out)
{
    struct func_call_context const *context = user;
    ASSERT(context->ps->inst.istate == SSTATE_FORGOTTEN)
    
    return NCDEvaluatorArgs_EvalArg(context->args, index, mem, out);
}
