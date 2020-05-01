/**
 * @file imperative.c
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
 * Imperative statement.
 * 
 * Synopsis:
 *   imperative(string init_template, list init_args, string deinit_template, list deinit_args, string deinit_timeout)
 * 
 * Description:
 *   Does the following, in order:
 *     1. Starts a template process from (init_template, init_args) and waits for it to
 *        initialize completely.
 *     2. Initiates termination of the process and wait for it to terminate.
 *     3. Puts the statement UP, then waits for a statement termination request (which may
 *        already have been received).
 *     4. Starts a template process from (deinit_template, deinit_args) and waits for it
 *        to initialize completely, or for the timeout to elapse.
 *     5. Initiates termination of the process and wait for it to terminate.
 *     6. Terminates the statement.
 * 
 *   If init_template="<none>", steps (1-2) are skipped.
 *   If deinit_template="<none>", steps (4-5) are skipped.
 */

#include <stdlib.h>
#include <string.h>

#include <misc/string_begins_with.h>
#include <misc/offset.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_imperative.h>

#define STATE_INIT_WORKING 1
#define STATE_INIT_CLEANING 2
#define STATE_UP 3
#define STATE_DEINIT_WORKING 4
#define STATE_DEINIT_CLEANING 5

struct instance {
    NCDModuleInst *i;
    NCDValRef deinit_template;
    NCDValRef deinit_args;
    BTimer deinit_timer;
    NCDModuleProcess process;
    int state;
    int dying;
};

static int start_process (struct instance *o, NCDValRef template_name, NCDValRef args, NCDModuleProcess_handler_event handler);
static void go_deinit (struct instance *o);
static void init_process_handler_event (NCDModuleProcess *process, int event);
static void deinit_process_handler_event (NCDModuleProcess *process, int event);
static int process_func_getspecialobj (NCDModuleProcess *process, NCD_string_id_t name, NCDObject *out_object);
static int process_caller_object_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object);
static void deinit_timer_handler (struct instance *o);
static void instance_free (struct instance *o);

static int start_process (struct instance *o, NCDValRef template_name, NCDValRef args, NCDModuleProcess_handler_event handler)
{
    ASSERT(NCDVal_IsString(template_name))
    ASSERT(NCDVal_IsList(args))
    
    // create process
    if (!NCDModuleProcess_InitValue(&o->process, o->i, template_name, args, handler)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDModuleProcess_Init failed");
        return 0;
    }
    
    // set special functions
    NCDModuleProcess_SetSpecialFuncs(&o->process, process_func_getspecialobj);
    return 1;
}

static void go_deinit (struct instance *o)
{
    ASSERT(o->dying)
    
    // deinit is no-op?
    if (ncd_is_none(o->deinit_template)) {
        instance_free(o);
        return;
    }
    
    // start deinit process
    if (!start_process(o, o->deinit_template, o->deinit_args, deinit_process_handler_event)) {
        instance_free(o);
        return;
    }
    
    // start timer
    BReactor_SetTimer(o->i->params->iparams->reactor, &o->deinit_timer);
    
    // set state deinit working
    o->state = STATE_DEINIT_WORKING;
}

static void init_process_handler_event (NCDModuleProcess *process, int event)
{
    struct instance *o = UPPER_OBJECT(process, struct instance, process);
    
    switch (event) {
        case NCDMODULEPROCESS_EVENT_UP: {
            ASSERT(o->state == STATE_INIT_WORKING)
            
            // start terminating
            NCDModuleProcess_Terminate(&o->process);
            
            // set state init cleaning
            o->state = STATE_INIT_CLEANING;
        } break;
        
        case NCDMODULEPROCESS_EVENT_TERMINATED: {
            ASSERT(o->state == STATE_INIT_CLEANING)
            
            // free process
            NCDModuleProcess_Free(&o->process);
            
            // were we requested to die aleady?
            if (o->dying) {
                go_deinit(o);
                return;
            }
            
            // signal up
            NCDModuleInst_Backend_Up(o->i);
            
            // set state up
            o->state = STATE_UP;
        } break;
        
        default: ASSERT(0);
    }
}

static void deinit_process_handler_event (NCDModuleProcess *process, int event)
{
    struct instance *o = UPPER_OBJECT(process, struct instance, process);
    ASSERT(o->dying)
    
    switch (event) {
        case NCDMODULEPROCESS_EVENT_UP: {
            ASSERT(o->state == STATE_DEINIT_WORKING)
            
            // stop timer
            BReactor_RemoveTimer(o->i->params->iparams->reactor, &o->deinit_timer);
            
            // start terminating
            NCDModuleProcess_Terminate(&o->process);
            
            // set state deinit cleaning
            o->state = STATE_DEINIT_CLEANING;
        } break;
        
        case NCDMODULEPROCESS_EVENT_TERMINATED: {
            ASSERT(o->state == STATE_DEINIT_CLEANING)
            
            // free process
            NCDModuleProcess_Free(&o->process);
            
            // die
            instance_free(o);
            return;
        } break;
        
        default: ASSERT(0);
    }
}

static int process_func_getspecialobj (NCDModuleProcess *process, NCD_string_id_t name, NCDObject *out_object)
{
    struct instance *o = UPPER_OBJECT(process, struct instance, process);
    ASSERT(o->state != STATE_UP)
    
    if (name == NCD_STRING_CALLER) {
        *out_object = NCDObject_Build(-1, o, NCDObject_no_getvar, process_caller_object_func_getobj);
        return 1;
    }
    
    return 0;
}

static int process_caller_object_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object)
{
    struct instance *o = NCDObject_DataPtr(obj);
    ASSERT(o->state != STATE_UP)
    
    return NCDModuleInst_Backend_GetObj(o->i, name, out_object);
}

static void deinit_timer_handler (struct instance *o)
{
    ASSERT(o->state == STATE_DEINIT_WORKING)
    
    ModuleLog(o->i, BLOG_ERROR, "imperative deinit timeout elapsed");
    
    // start terminating
    NCDModuleProcess_Terminate(&o->process);
    
    // set state deinit cleaning
    o->state = STATE_DEINIT_CLEANING;
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef init_template_arg;
    NCDValRef init_args;
    NCDValRef deinit_template_arg;
    NCDValRef deinit_timeout_arg;
    if (!NCDVal_ListRead(params->args, 5, &init_template_arg, &init_args, &deinit_template_arg, &o->deinit_args, &deinit_timeout_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(init_template_arg) || !NCDVal_IsList(init_args)  ||
        !NCDVal_IsString(deinit_template_arg) || !NCDVal_IsList(o->deinit_args)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    o->deinit_template = deinit_template_arg;
    
    // read timeout
    uintmax_t timeout;
    if (!ncd_read_uintmax(deinit_timeout_arg, &timeout) || timeout > UINT64_MAX){
        ModuleLog(i, BLOG_ERROR, "wrong timeout");
        goto fail0;
    }
    
    // init timer
    BTimer_Init(&o->deinit_timer, timeout, (BTimer_handler)deinit_timer_handler, o);
    
    if (ncd_is_none(init_template_arg)) {
        // signal up
        NCDModuleInst_Backend_Up(i);
        
        // set state up
        o->state = STATE_UP;
    } else {
        // start init process
        if (!start_process(o, init_template_arg, init_args, init_process_handler_event)) {
            goto fail0;
        }
        
        // set state init working
        o->state = STATE_INIT_WORKING;
    }
    
    // set not dying
    o->dying = 0;
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void instance_free (struct instance *o)
{
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    ASSERT(!o->dying)
    
    // set dying
    o->dying = 1;
    
    if (o->state == STATE_UP) {
        go_deinit(o);
        return;
    }
}

static struct NCDModule modules[] = {
    {
        .type = "imperative",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_imperative = {
    .modules = modules
};
