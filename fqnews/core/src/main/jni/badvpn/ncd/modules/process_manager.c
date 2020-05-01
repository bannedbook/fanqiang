/**
 * @file process_manager.c
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
 * Module which allows starting and controlling template processes using an imperative
 * interface.
 * 
 * Synopsis:
 *   process_manager()
 * 
 * Description:
 *   Represents a set of managed processes. Each process has a "name", which is a value
 *   that uniquely identifies it within its process manager.
 *   When deinitialization is requested, requests termination of all managed processes
 *   and waits for all of them to terminate before deinitializing.
 *   Managed processes can access objects as seen from the process_manager() statement
 *   via the special _caller object.
 * 
 * Synopsis:
 *   process_manager::start(name, string template_name, list args)
 *   process_manager::start(string template_name, list args)
 * 
 * Description:
 *   Creates a new process from the template named 'template_name', with arguments 'args',
 *   identified by 'name' within the process manager. If the two-argument form of start() is
 *   used, the process does not have a name, and cannot be imperatively stopped using
 *   stop().
 *   If a process with this name already exists and is not being terminated, does nothing.
 *   If it exists and is being terminated, it will be restarted using the given parameters
 *   after it terminates. If the process does not exist, it is created immediately, and the
 *   immediate effects of the process being created happnen before the immediate effects of
 *   the start() statement going up.
 * 
 * Synopsis:
 *   process_manager::stop(name)
 * 
 * Description:
 *   Initiates termination of the process identified by 'name' within the process manager.
 *   If there is no such process, or the process is already being terminated, does nothing.
 *   If the process does exist and is not already being terminated, termination of the
 *   process is requested, and the immediate effects of the termination request happen
 *   before the immediate effects of the stop() statement going up.
 */

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <misc/strdup.h>
#include <misc/balloc.h>
#include <structure/LinkedList1.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_process_manager.h>

#define RETRY_TIME 10000

#define PROCESS_STATE_RUNNING 1
#define PROCESS_STATE_STOPPING 2
#define PROCESS_STATE_RESTARTING 3
#define PROCESS_STATE_RETRYING 4

struct instance {
    NCDModuleInst *i;
    LinkedList1 processes_list;
    int dying;
};

struct process {
    struct instance *manager;
    LinkedList1Node processes_list_node;
    BSmallTimer retry_timer; // running if state=retrying
    int state;
    NCD_string_id_t template_name;
    NCDValMem current_mem;
    NCDValSafeRef current_name;
    NCDValSafeRef current_args;
    NCDValMem next_mem; // next_* if state=restarting
    NCDValSafeRef next_name;
    NCDValSafeRef next_args;
    NCDModuleProcess module_process; // if state!=retrying
};

static struct process * find_process (struct instance *o, NCDValRef name);
static int process_new (struct instance *o, NCDValMem *mem, NCDValSafeRef name, NCDValSafeRef template_name, NCDValSafeRef args);
static void process_free (struct process *p);
static void process_try (struct process *p);
static void process_retry_timer_handler (BSmallTimer *retry_timer);
static void process_module_process_handler_event (NCDModuleProcess *module_process, int event);
static int process_module_process_func_getspecialobj (NCDModuleProcess *module_process, NCD_string_id_t name, NCDObject *out_object);
static int process_module_process_caller_obj_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object);
static void process_stop (struct process *p);
static int process_restart (struct process *p, NCDValMem *mem, NCDValSafeRef name, NCDValSafeRef template_name, NCDValSafeRef args);
static void instance_free (struct instance *o);

static struct process * find_process (struct instance *o, NCDValRef name)
{
    ASSERT(!NCDVal_IsInvalid(name))
    
    LinkedList1Node *n = LinkedList1_GetFirst(&o->processes_list);
    while (n) {
        struct process *p = UPPER_OBJECT(n, struct process, processes_list_node);
        ASSERT(p->manager == o)
        if (!NCDVal_IsInvalid(NCDVal_FromSafe(&p->current_mem, p->current_name)) && NCDVal_Compare(NCDVal_FromSafe(&p->current_mem, p->current_name), name) == 0) {
            return p;
        }
        n = LinkedList1Node_Next(n);
    }
    
    return NULL;
}

static int process_new (struct instance *o, NCDValMem *mem, NCDValSafeRef name, NCDValSafeRef template_name, NCDValSafeRef args)
{
    ASSERT(!o->dying)
    ASSERT(NCDVal_IsInvalid(NCDVal_FromSafe(mem, name)) || !find_process(o, NCDVal_FromSafe(mem, name)))
    ASSERT(NCDVal_IsString(NCDVal_FromSafe(mem, template_name)))
    ASSERT(NCDVal_IsList(NCDVal_FromSafe(mem, args)))
    
    // allocate structure
    struct process *p = BAlloc(sizeof(*p));
    if (!p) {
        ModuleLog(o->i, BLOG_ERROR, "BAlloc failed");
        goto fail0;
    }
    
    // set manager
    p->manager = o;

    // insert to processes list
    LinkedList1_Append(&o->processes_list, &p->processes_list_node);

    // init retry timer
    BSmallTimer_Init(&p->retry_timer, process_retry_timer_handler);
    
    // init template name
    p->template_name = ncd_get_string_id(NCDVal_FromSafe(mem, template_name));
    if (p->template_name < 0) {
        ModuleLog(o->i, BLOG_ERROR, "ncd_get_string_id failed");
        goto fail1;
    }
    
    // init current mem as a copy of mem
    if (!NCDValMem_InitCopy(&p->current_mem, mem)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDValMem_InitCopy failed");
        goto fail1;
    }
    
    // remember name and args
    p->current_name = name;
    p->current_args = args;
    
    // try starting it
    process_try(p);
    return 1;
    
fail1:
    LinkedList1_Remove(&o->processes_list, &p->processes_list_node);
    BFree(p);
fail0:
    return 0;
}

static void process_free (struct process *p)
{
    struct instance *o = p->manager;
    
    // free current mem
    NCDValMem_Free(&p->current_mem);
    
    // free timer
    BReactor_RemoveSmallTimer(o->i->params->iparams->reactor, &p->retry_timer);
    
    // remove from processes list
    LinkedList1_Remove(&o->processes_list, &p->processes_list_node);
    
    // free structure
    BFree(p);
}

static void process_try (struct process *p)
{
    struct instance *o = p->manager;
    ASSERT(!o->dying)
    ASSERT(!BSmallTimer_IsRunning(&p->retry_timer))
    
    // init module process
    if (!NCDModuleProcess_InitId(&p->module_process, o->i, p->template_name, NCDVal_FromSafe(&p->current_mem, p->current_args), process_module_process_handler_event)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDModuleProcess_Init failed");
        goto fail;
    }
        
    // set special objects function
    NCDModuleProcess_SetSpecialFuncs(&p->module_process, process_module_process_func_getspecialobj);
    
    // set state
    p->state = PROCESS_STATE_RUNNING;
    return;
    
fail:
    // set timer
    BReactor_SetSmallTimer(o->i->params->iparams->reactor, &p->retry_timer, BTIMER_SET_RELATIVE, RETRY_TIME);
    
    // set state
    p->state = PROCESS_STATE_RETRYING;
}

static void process_retry_timer_handler (BSmallTimer *retry_timer)
{
    struct process *p = UPPER_OBJECT(retry_timer, struct process, retry_timer);
    struct instance *o = p->manager;
    B_USE(o)
    ASSERT(p->state == PROCESS_STATE_RETRYING)
    ASSERT(!o->dying)
    
    // retry
    process_try(p);
}

void process_module_process_handler_event (NCDModuleProcess *module_process, int event)
{
    struct process *p = UPPER_OBJECT(module_process, struct process, module_process);
    struct instance *o = p->manager;
    ASSERT(p->state != PROCESS_STATE_RETRYING)
    ASSERT(p->state != PROCESS_STATE_RESTARTING || !o->dying)
    ASSERT(!BSmallTimer_IsRunning(&p->retry_timer))
    
    switch (event) {
        case NCDMODULEPROCESS_EVENT_UP: {
            ASSERT(p->state == PROCESS_STATE_RUNNING)
        } break;
        
        case NCDMODULEPROCESS_EVENT_DOWN: {
            ASSERT(p->state == PROCESS_STATE_RUNNING)
            
            // allow process to continue
            NCDModuleProcess_Continue(&p->module_process);
        } break;
        
        case NCDMODULEPROCESS_EVENT_TERMINATED: {
            ASSERT(p->state == PROCESS_STATE_RESTARTING || p->state == PROCESS_STATE_STOPPING)
            
            // free module process
            NCDModuleProcess_Free(&p->module_process);
            
            if (p->state == PROCESS_STATE_RESTARTING) {
                // free current mem
                NCDValMem_Free(&p->current_mem);
                
                // move next mem/values over current mem/values
                p->current_mem = p->next_mem;
                p->current_name = p->next_name;
                p->current_args = p->next_args;
                
                // try starting it again
                process_try(p);
                return;
            }
            
            // free process
            process_free(p);
            
            // if manager is dying and there are no more processes, let it die
            if (o->dying && LinkedList1_IsEmpty(&o->processes_list)) {
                instance_free(o);
            }
        } break;
    }
}

static int process_module_process_func_getspecialobj (NCDModuleProcess *module_process, NCD_string_id_t name, NCDObject *out_object)
{
    struct process *p = UPPER_OBJECT(module_process, struct process, module_process);
    ASSERT(p->state != PROCESS_STATE_RETRYING)
    
    if (name == NCD_STRING_CALLER) {
        *out_object = NCDObject_Build(-1, p, NCDObject_no_getvar, process_module_process_caller_obj_func_getobj);
        return 1;
    }
    
    return 0;
}

static int process_module_process_caller_obj_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object)
{
    struct process *p = NCDObject_DataPtr(obj);
    struct instance *o = p->manager;
    ASSERT(p->state != PROCESS_STATE_RETRYING)
    
    return NCDModuleInst_Backend_GetObj(o->i, name, out_object);
}

static void process_stop (struct process *p)
{
    switch (p->state) {
        case PROCESS_STATE_RETRYING: {
            // free process
            process_free(p);
        } break;
        
        case PROCESS_STATE_RUNNING: {
            // request process to terminate
            NCDModuleProcess_Terminate(&p->module_process);
            
            // set state
            p->state = PROCESS_STATE_STOPPING;
        } break;
        
        case PROCESS_STATE_RESTARTING: {
            // free next mem
            NCDValMem_Free(&p->next_mem);
            
            // set state
            p->state = PROCESS_STATE_STOPPING;
        } break;
        
        case PROCESS_STATE_STOPPING: {
            // nothing to do
        } break;
        
        default: ASSERT(0);
    }
}

static int process_restart (struct process *p, NCDValMem *mem, NCDValSafeRef name, NCDValSafeRef template_name, NCDValSafeRef args)
{
    struct instance *o = p->manager;
    ASSERT(!o->dying)
    ASSERT(p->state == PROCESS_STATE_STOPPING)
    ASSERT(!NCDVal_IsInvalid(NCDVal_FromSafe(&p->current_mem, p->current_name)) || NCDVal_IsInvalid(NCDVal_FromSafe(mem, name)))
    ASSERT(NCDVal_IsInvalid(NCDVal_FromSafe(&p->current_mem, p->current_name)) || NCDVal_Compare(NCDVal_FromSafe(mem, name), NCDVal_FromSafe(&p->current_mem, p->current_name)) == 0)
    ASSERT(NCDVal_IsString(NCDVal_FromSafe(mem, template_name)))
    ASSERT(NCDVal_IsList(NCDVal_FromSafe(mem, args)))
    
    // copy mem to next mem
    if (!NCDValMem_InitCopy(&p->next_mem, mem)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDValMem_InitCopy failed");
        goto fail0;
    }
    
    // remember name and args to next
    p->next_name = name;
    p->next_args = args;
    
    // set state
    p->state = PROCESS_STATE_RESTARTING;
    return 1;
    
fail0:
    return 0;
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // init processes list
    LinkedList1_Init(&o->processes_list);
    
    // set not dying
    o->dying = 0;
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

void instance_free (struct instance *o)
{
    ASSERT(LinkedList1_IsEmpty(&o->processes_list))
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    ASSERT(!o->dying)
    
    // request all processes to die
    LinkedList1Node *n = LinkedList1_GetFirst(&o->processes_list);
    while (n) {
        LinkedList1Node *next = LinkedList1Node_Next(n);
        struct process *p = UPPER_OBJECT(n, struct process, processes_list_node);
        process_stop(p);
        n = next;
    }
    
    // if there are no processes, die immediately
    if (LinkedList1_IsEmpty(&o->processes_list)) {
        instance_free(o);
        return;
    }
    
    // set dying
    o->dying = 1;
}

static void start_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    NCDValRef name_arg = NCDVal_NewInvalid();
    NCDValRef template_name_arg;
    NCDValRef args_arg;
    if (!NCDVal_ListRead(params->args, 2, &template_name_arg, &args_arg) &&
        !NCDVal_ListRead(params->args, 3, &name_arg, &template_name_arg, &args_arg)
    ) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(template_name_arg) || !NCDVal_IsList(args_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // signal up.
    // Do it before creating the process so that the process starts initializing before our own process continues.
    NCDModuleInst_Backend_Up(i);
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    if (!mo->dying) {
        struct process *p = (NCDVal_IsInvalid(name_arg) ? NULL : find_process(mo, name_arg));
        if (!p || p->state == PROCESS_STATE_STOPPING) {
            if (p) {
                if (!process_restart(p, args_arg.mem, NCDVal_ToSafe(name_arg), NCDVal_ToSafe(template_name_arg), NCDVal_ToSafe(args_arg))) {
                    ModuleLog(i, BLOG_ERROR, "failed to restart process");
                    goto fail0;
                }
            } else {
                if (!process_new(mo, args_arg.mem, NCDVal_ToSafe(name_arg), NCDVal_ToSafe(template_name_arg), NCDVal_ToSafe(args_arg))) {
                    ModuleLog(i, BLOG_ERROR, "failed to create process");
                    goto fail0;
                }
            }
        }
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void stop_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    NCDValRef name_arg;
    if (!NCDVal_ListRead(params->args, 1, &name_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // signal up.
    // Do it before stopping the process so that the process starts terminating before our own process continues.
    NCDModuleInst_Backend_Up(i);
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    if (!mo->dying) {
        struct process *p = find_process(mo, name_arg);
        if (p && p->state != PROCESS_STATE_STOPPING) {
            process_stop(p);
        }
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "process_manager",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "process_manager::start",
        .func_new2 = start_func_new
    }, {
        .type = "process_manager::stop",
        .func_new2 = stop_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_process_manager = {
    .modules = modules
};
