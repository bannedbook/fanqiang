/**
 * @file ondemand.c
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
 * On-demand process manager.
 * 
 * Synopsis:
 *   ondemand(string template_name, list args)
 * 
 * Description:
 *   Manages an on-demand template process using a process template named
 *   template_name.
 *   On deinitialization, if the process is running, reqests its termination
 *   and waits for it to terminate.
 * 
 * Synopsis:
 *   ondemand::demand()
 * 
 * Description:
 *   Demands the availability of an on-demand template process.
 *   This statement is in UP state if and only if the template process of the
 *   corresponding ondemand object is completely up.
 * 
 * Variables:
 *   Exposes variables and objects from the template process corresponding to
 *   the ondemand object.
 */

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <structure/LinkedList1.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_ondemand.h>

struct ondemand {
    NCDModuleInst *i;
    NCDValRef template_name;
    NCDValRef args;
    LinkedList1 demands_list;
    int dying;
    int have_process;
    NCDModuleProcess process;
    int process_terminating;
    int process_up;
};

struct demand {
    NCDModuleInst *i;
    struct ondemand *od;
    LinkedList1Node demands_list_node;
};

static int ondemand_start_process (struct ondemand *o);
static void ondemand_terminate_process (struct ondemand *o);
static void ondemand_process_handler (NCDModuleProcess *process, int event);
static void ondemand_free (struct ondemand *o);
static void demand_free (struct demand *o, int is_error);

static int ondemand_start_process (struct ondemand *o)
{
    ASSERT(!o->dying)
    ASSERT(!o->have_process)
    
    // start process
    if (!NCDModuleProcess_InitValue(&o->process, o->i, o->template_name, o->args, ondemand_process_handler)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDModuleProcess_Init failed");
        goto fail0;
    }
    
    // set have process
    o->have_process = 1;
    
    // set process not terminating
    o->process_terminating = 0;
    
    // set process not up
    o->process_up = 0;
    
    return 1;
    
fail0:
    return 0;
}

static void ondemand_terminate_process (struct ondemand *o)
{
    ASSERT(o->have_process)
    ASSERT(!o->process_terminating)
    
    // request termination
    NCDModuleProcess_Terminate(&o->process);
    
    // set process terminating
    o->process_terminating = 1;
    
    if (o->process_up) {
        // set process down
        o->process_up = 0;
        
        // signal demands down
        for (LinkedList1Node *n = LinkedList1_GetFirst(&o->demands_list); n; n = LinkedList1Node_Next(n)) {
            struct demand *demand = UPPER_OBJECT(n, struct demand, demands_list_node);
            ASSERT(demand->od == o)
            NCDModuleInst_Backend_Down(demand->i);
        }
    }
}

static void ondemand_process_handler (NCDModuleProcess *process, int event)
{
    struct ondemand *o = UPPER_OBJECT(process, struct ondemand, process);
    ASSERT(o->have_process)
    
    switch (event) {
        case NCDMODULEPROCESS_EVENT_UP: {
            ASSERT(!o->process_terminating)
            ASSERT(!o->process_up)
            
            // set process up
            o->process_up = 1;
            
            // signal demands up
            for (LinkedList1Node *n = LinkedList1_GetFirst(&o->demands_list); n; n = LinkedList1Node_Next(n)) {
                struct demand *demand = UPPER_OBJECT(n, struct demand, demands_list_node);
                ASSERT(demand->od == o)
                NCDModuleInst_Backend_Up(demand->i);
            }
        } break;
        
        case NCDMODULEPROCESS_EVENT_DOWN: {
            ASSERT(!o->process_terminating)
            ASSERT(o->process_up)
            
            // continue process
            NCDModuleProcess_Continue(&o->process);
            
            // set process down
            o->process_up = 0;
            
            // signal demands down
            for (LinkedList1Node *n = LinkedList1_GetFirst(&o->demands_list); n; n = LinkedList1Node_Next(n)) {
                struct demand *demand = UPPER_OBJECT(n, struct demand, demands_list_node);
                ASSERT(demand->od == o)
                NCDModuleInst_Backend_Down(demand->i);
            }
        } break;
        
        case NCDMODULEPROCESS_EVENT_TERMINATED: {
            ASSERT(o->process_terminating)
            ASSERT(!o->process_up)
            
            // free process
            NCDModuleProcess_Free(&o->process);
            
            // set have no process
            o->have_process = 0;
            
            // if dying, die finally
            if (o->dying) {
                ondemand_free(o);
                return;
            }
            
            // if demands arrivied, restart process
            if (!LinkedList1_IsEmpty(&o->demands_list)) {
                if (!ondemand_start_process(o)) {
                    // error demands
                    while (!LinkedList1_IsEmpty(&o->demands_list)) {
                        struct demand *demand = UPPER_OBJECT(LinkedList1_GetFirst(&o->demands_list), struct demand, demands_list_node);
                        ASSERT(demand->od == o)
                        demand_free(demand, 1);
                    }
                }
            }
        } break;
    }
}

static void ondemand_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct ondemand *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef arg_template_name;
    NCDValRef arg_args;
    if (!NCDVal_ListRead(params->args, 2, &arg_template_name, &arg_args)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(arg_template_name) || !NCDVal_IsList(arg_args)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    o->template_name = arg_template_name;
    o->args = arg_args;
    
    // init demands list
    LinkedList1_Init(&o->demands_list);
    
    // set not dying
    o->dying = 0;
    
    // set have no process
    o->have_process = 0;
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void ondemand_free (struct ondemand *o)
{
    ASSERT(!o->have_process)
    
    // die demands
    while (!LinkedList1_IsEmpty(&o->demands_list)) {
        struct demand *demand = UPPER_OBJECT(LinkedList1_GetFirst(&o->demands_list), struct demand, demands_list_node);
        ASSERT(demand->od == o)
        demand_free(demand, 0);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void ondemand_func_die (void *vo)
{
    struct ondemand *o = vo;
    ASSERT(!o->dying)
    
    // if not have process, die right away
    if (!o->have_process) {
        ondemand_free(o);
        return;
    }
    
    // set dying
    o->dying = 1;
    
    // request process termination if not already
    if (!o->process_terminating) {
        ondemand_terminate_process(o);
    }
}

static void demand_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct demand *o = vo;
    o->i = i;
    
    // read arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // set ondemand
    o->od = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // add to ondemand's demands list
    LinkedList1_Append(&o->od->demands_list, &o->demands_list_node);
    
    // start process if needed
    if (!o->od->have_process) {
        ASSERT(!o->od->dying)
        
        if (!ondemand_start_process(o->od)) {
            goto fail1;
        }
    }
    
    // if process is up, signal up
    if (o->od->process_up) {
        NCDModuleInst_Backend_Up(i);
    }
    
    return;
    
fail1:
    LinkedList1_Remove(&o->od->demands_list, &o->demands_list_node);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void demand_free (struct demand *o, int is_error)
{
    // remove from ondemand's demands list
    LinkedList1_Remove(&o->od->demands_list, &o->demands_list_node);
    
    // request process termination if no longer needed
    if (o->od->have_process && !o->od->process_terminating && LinkedList1_IsEmpty(&o->od->demands_list)) {
        ondemand_terminate_process(o->od);
    }
    
    if (is_error) {
        NCDModuleInst_Backend_DeadError(o->i);
    } else {
        NCDModuleInst_Backend_Dead(o->i);
    }
}

static void demand_func_die (void *vo)
{
    struct demand *o = vo;
    
    demand_free(o, 0);
}

static int demand_func_getobj (void *vo, NCD_string_id_t objname, NCDObject *out_object)
{
    struct demand *o = vo;
    ASSERT(o->od->have_process)
    ASSERT(o->od->process_up)
    
    return NCDModuleProcess_GetObj(&o->od->process, objname, out_object);
}

static struct NCDModule modules[] = {
    {
        .type = "ondemand",
        .func_new2 = ondemand_func_new,
        .func_die = ondemand_func_die,
        .alloc_size = sizeof(struct ondemand)
    }, {
        .type = "ondemand::demand",
        .func_new2 = demand_func_new,
        .func_die = demand_func_die,
        .func_getobj = demand_func_getobj,
        .alloc_size = sizeof(struct demand)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_ondemand = {
    .modules = modules
};
