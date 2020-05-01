/**
 * @file spawn.c
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
 * Module which starts a process from a process template on initialization, and
 * stops it on deinitialization.
 * 
 * Synopsis:
 *   spawn(string template_name, list args)
 * 
 * Description:
 *   On initialization, creates a new process from the template named
 *   'template_name', with arguments 'args'. On deinitialization, initiates termination
 *   of the process and waits for it to terminate. The process can access objects
 *   as seen from 'spawn' via the _caller special object.
 * 
 * Synopsis:
 *   spawn::join()
 * 
 * Description:
 *   A join() on a spawn() is like a depend() on a provide() which is located at the
 *   end of the spawned process.
 * 
 * Variables:
 *   Exposes objects from the spawned process.
 */

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <structure/LinkedList0.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_spawn.h>

#define STATE_WORKING 1
#define STATE_UP 2
#define STATE_WAITING 3
#define STATE_WAITING_TERMINATING 4
#define STATE_TERMINATING 5

struct instance {
    NCDModuleInst *i;
    NCDModuleProcess process;
    LinkedList0 clean_list;
    LinkedList0 dirty_list;
    int state;
};

struct join_instance {
    NCDModuleInst *i;
    struct instance *spawn;
    LinkedList0Node list_node;
    int is_dirty;
};

static void assert_dirty_state (struct instance *o);
static void process_handler_event (NCDModuleProcess *process, int event);
static int process_func_getspecialobj (NCDModuleProcess *process, NCD_string_id_t name, NCDObject *out_object);
static int caller_obj_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object);
static void bring_joins_down (struct instance *o);
static void continue_working (struct instance *o);
static void continue_terminating (struct instance *o);
static void instance_free (struct instance *o);

static void assert_dirty_state (struct instance *o)
{
    ASSERT(!LinkedList0_IsEmpty(&o->dirty_list) == (o->state == STATE_WAITING || o->state == STATE_WAITING_TERMINATING))
}

static void process_handler_event (NCDModuleProcess *process, int event)
{
    struct instance *o = UPPER_OBJECT(process, struct instance, process);
    assert_dirty_state(o);
    
    switch (event) {
        case NCDMODULEPROCESS_EVENT_UP: {
            ASSERT(o->state == STATE_WORKING)
            ASSERT(LinkedList0_IsEmpty(&o->dirty_list))
            
            // set state up
            o->state = STATE_UP;
            
            // bring joins up
            for (LinkedList0Node *ln = LinkedList0_GetFirst(&o->clean_list); ln; ln = LinkedList0Node_Next(ln)) {
                struct join_instance *join = UPPER_OBJECT(ln, struct join_instance, list_node);
                ASSERT(join->spawn == o)
                ASSERT(!join->is_dirty)
                NCDModuleInst_Backend_Up(join->i);
            }
        } break;
        
        case NCDMODULEPROCESS_EVENT_DOWN: {
            ASSERT(o->state == STATE_UP)
            ASSERT(LinkedList0_IsEmpty(&o->dirty_list))
            
            // bring joins down, moving them to the dirty list
            bring_joins_down(o);
            
            // set state waiting
            o->state = STATE_WAITING;
            
            // if we have no joins, continue immediately
            if (LinkedList0_IsEmpty(&o->dirty_list)) {
                continue_working(o);
                return;
            }
        } break;
        
        case NCDMODULEPROCESS_EVENT_TERMINATED: {
            ASSERT(o->state == STATE_TERMINATING)
            ASSERT(LinkedList0_IsEmpty(&o->dirty_list))
            
            // die finally
            instance_free(o);
            return;
        } break;
        
        default: ASSERT(0);
    }
}

static int process_func_getspecialobj (NCDModuleProcess *process, NCD_string_id_t name, NCDObject *out_object)
{
    struct instance *o = UPPER_OBJECT(process, struct instance, process);
    
    if (name == NCD_STRING_CALLER) {
        *out_object = NCDObject_Build(-1, o, NCDObject_no_getvar, caller_obj_func_getobj);
        return 1;
    }
    
    return 0;
}

static int caller_obj_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object)
{
    struct instance *o = NCDObject_DataPtr(obj);
    
    return NCDModuleInst_Backend_GetObj(o->i, name, out_object);
}

static void bring_joins_down (struct instance *o)
{
    LinkedList0Node *ln;
    while (ln = LinkedList0_GetFirst(&o->clean_list)) {
        struct join_instance *join = UPPER_OBJECT(ln, struct join_instance, list_node);
        ASSERT(join->spawn == o)
        ASSERT(!join->is_dirty)
        NCDModuleInst_Backend_Down(join->i);
        LinkedList0_Remove(&o->clean_list, &join->list_node);
        LinkedList0_Prepend(&o->dirty_list, &join->list_node);
        join->is_dirty = 1;
    }
}

static void continue_working (struct instance *o)
{
    ASSERT(o->state == STATE_WAITING)
    ASSERT(LinkedList0_IsEmpty(&o->dirty_list))
    
    // continue process
    NCDModuleProcess_Continue(&o->process);
    
    // set state working
    o->state = STATE_WORKING;
}

static void continue_terminating (struct instance *o)
{
    ASSERT(o->state == STATE_WAITING_TERMINATING)
    ASSERT(LinkedList0_IsEmpty(&o->dirty_list))
    
    // request process to terminate
    NCDModuleProcess_Terminate(&o->process);
    
    // set state terminating
    o->state = STATE_TERMINATING;
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef template_name_arg;
    NCDValRef args_arg;
    if (!NCDVal_ListRead(params->args, 2, &template_name_arg, &args_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(template_name_arg) || !NCDVal_IsList(args_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // signal up.
    // Do it before creating the process so that the process starts initializing before our own process continues.
    NCDModuleInst_Backend_Up(o->i);
    
    // create process
    if (!NCDModuleProcess_InitValue(&o->process, o->i, template_name_arg, args_arg, process_handler_event)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDModuleProcess_Init failed");
        goto fail0;
    }
    
    // set object resolution function
    NCDModuleProcess_SetSpecialFuncs(&o->process, process_func_getspecialobj);
    
    // init lists
    LinkedList0_Init(&o->clean_list);
    LinkedList0_Init(&o->dirty_list);
    
    // set state working
    o->state = STATE_WORKING;
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

void instance_free (struct instance *o)
{
    ASSERT(LinkedList0_IsEmpty(&o->dirty_list))
    
    // unlink joins
    LinkedList0Node *ln;
    while (ln = LinkedList0_GetFirst(&o->clean_list)) {
        struct join_instance *join = UPPER_OBJECT(ln, struct join_instance, list_node);
        ASSERT(join->spawn == o)
        ASSERT(!join->is_dirty)
        LinkedList0_Remove(&o->clean_list, &join->list_node);
        join->spawn = NULL;
    }
    
    // free process
    NCDModuleProcess_Free(&o->process);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    ASSERT(o->state != STATE_WAITING_TERMINATING)
    ASSERT(o->state != STATE_TERMINATING)
    assert_dirty_state(o);
    
    // bring joins down
    if (o->state == STATE_UP) {
        bring_joins_down(o);
    }
    
    // set state waiting terminating
    o->state = STATE_WAITING_TERMINATING;
    
    // start terminating now if possible
    if (LinkedList0_IsEmpty(&o->dirty_list)) {
        continue_terminating(o);
        return;
    }
}

static void join_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct join_instance *o = vo;
    o->i = i;
    
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    o->spawn = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    assert_dirty_state(o->spawn);
    
    LinkedList0_Prepend(&o->spawn->clean_list, &o->list_node);
    o->is_dirty = 0;
    
    if (o->spawn->state == STATE_UP) {
        NCDModuleInst_Backend_Up(i);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void join_func_die (void *vo)
{
    struct join_instance *o = vo;
    
    if (o->spawn) {
        assert_dirty_state(o->spawn);
        
        // remove from list
        if (o->is_dirty) {
            LinkedList0_Remove(&o->spawn->dirty_list, &o->list_node);
        } else {
            LinkedList0_Remove(&o->spawn->clean_list, &o->list_node);
        }
        
        if (o->is_dirty && LinkedList0_IsEmpty(&o->spawn->dirty_list)) {
            ASSERT(o->spawn->state == STATE_WAITING || o->spawn->state == STATE_WAITING_TERMINATING)
            
            if (o->spawn->state == STATE_WAITING) {
                continue_working(o->spawn);
            } else {
                continue_terminating(o->spawn);
            }
        }
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int join_func_getobj (void *vo, NCD_string_id_t name, NCDObject *out)
{
    struct join_instance *o = vo;
    
    if (!o->spawn) {
        return 0;
    }
    
    return NCDModuleProcess_GetObj(&o->spawn->process, name, out);
}

static void join_func_clean (void *vo)
{
    struct join_instance *o = vo;
    
    if (!(o->spawn && o->is_dirty)) {
        return;
    }
    
    assert_dirty_state(o->spawn);
    ASSERT(o->spawn->state == STATE_WAITING || o->spawn->state == STATE_WAITING_TERMINATING)
    
    LinkedList0_Remove(&o->spawn->dirty_list, &o->list_node);
    LinkedList0_Prepend(&o->spawn->clean_list, &o->list_node);
    o->is_dirty = 0;
    
    if (LinkedList0_IsEmpty(&o->spawn->dirty_list)) {
        if (o->spawn->state == STATE_WAITING) {
            continue_working(o->spawn);
        } else {
            continue_terminating(o->spawn);
        }
    }
}

static struct NCDModule modules[] = {
    {
        .type = "spawn",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "synchronous_process", // deprecated name
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "spawn::join",
        .func_new2 = join_func_new,
        .func_die = join_func_die,
        .func_getobj = join_func_getobj,
        .func_clean = join_func_clean,
        .alloc_size = sizeof(struct join_instance),
        .flags = NCDMODULE_FLAG_CAN_RESOLVE_WHEN_DOWN
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_spawn = {
    .modules = modules
};
