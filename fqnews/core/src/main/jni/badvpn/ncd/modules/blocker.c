/**
 * @file blocker.c
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
 * Blocker module. Provides a statement that blocks when initialized, and which can be blocked
 * and unblocked from outside.
 * 
 * Synopsis: blocker([string initial_state])
 * Description: provides blocking operations. On deinitialization, waits for all corresponding
 *              use() statements to die before dying itself.
 *              The optional boolean argument initial_state specifies the initial up-state
 *              of the blocker. If not given, the default is false (not up).
 * Variables: string (empty) - the up-state (false or true).
 * 
 * Synopsis: blocker::up()
 * Description: sets the blocking state to up.
 *              The immediate effects of corresponding use() statements going up are processed before
 *              this statement goes up; but this statement statement still goes up immediately,
 *              assuming the effects mentioned haven't resulted in the intepreter scheduling this
 *              very statement for destruction.
 * 
 * Synopsis: blocker::down()
 * Description: sets the blocking state to down.
 *              The immediate effects of corresponding use() statements going up are processed before
 *              this statement goes up; but this statement statement still goes up immediately,
 *              assuming the effects mentioned haven't resulted in the intepreter scheduling this
 *              very statement for destruction.
 * 
 * Synopsis: blocker::downup()
 * Description: atomically sets the blocker to down state (if it was up), then (back) to up state.
 *              Note that this is not equivalent to calling down() and immediately up(); in that case,
 *              the interpreter will first handle the immediate effects of any use() statements
 *              going down as a result of having called down() and will only later execute the up()
 *              statement. In fact, it is possible that the effects of down() will prevent up() from
 *              executing, which may leave the program in an undesirable state.
 * 
 * Synopsis: blocker::rdownup()
 * Description: on deinitialization, atomically sets the blocker to down state (if it was up), then
 *              (back) to up state.
 *              The immediate effects of corresponding use() statements changing state are processed
 *              *after* the immediate effects of this statement dying (in contrast to downup()).
 * 
 * Synopsis: blocker::use()
 * Description: blocks on the blocker. This module is in up state if and only if the blocking state of
 * the blocker is up. Multiple use statements may be used with the same blocker.
 */

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <structure/LinkedList1.h>
#include <structure/LinkedList0.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_blocker.h>

struct instance {
    NCDModuleInst *i;
    LinkedList1 users;
    LinkedList0 rdownups_list;
    int up;
    int dying;
};

struct rdownup_instance {
    NCDModuleInst *i;
    struct instance *blocker;
    LinkedList0Node rdownups_list_node;
};

struct use_instance {
    NCDModuleInst *i;
    struct instance *blocker;
    LinkedList1Node blocker_node;
};

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef initial_state = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 0) &&
        !NCDVal_ListRead(params->args, 1, &initial_state)
    ) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get the initial state
    o->up = 0;
    if (!NCDVal_IsInvalid(initial_state)) {
        if (!ncd_read_boolean(initial_state, &o->up)) {
            ModuleLog(o->i, BLOG_ERROR, "bad initial_state argument");
            goto fail0;
        }
    }
    
    // init users list
    LinkedList1_Init(&o->users);
    
    // init rdownups list
    LinkedList0_Init(&o->rdownups_list);
    
    // set not dying
    o->dying = 0;
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void instance_free (struct instance *o)
{
    ASSERT(LinkedList1_IsEmpty(&o->users))
    
    // break any rdownups
    LinkedList0Node *ln;
    while (ln = LinkedList0_GetFirst(&o->rdownups_list)) {
        struct rdownup_instance *rdu = UPPER_OBJECT(ln, struct rdownup_instance, rdownups_list_node);
        ASSERT(rdu->blocker == o)
        LinkedList0_Remove(&o->rdownups_list, &rdu->rdownups_list_node);
        rdu->blocker = NULL;
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    ASSERT(!o->dying)
    
    // if we have no users, die right away, else wait for users
    if (LinkedList1_IsEmpty(&o->users)) {
        instance_free(o);
        return;
    }
    
    // set dying
    o->dying = 1;
}

static int func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        *out = ncd_make_boolean(mem, o->up);
        return 1;
    }
    
    return 0;
}

static void updown_func_new_templ (NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int up, int first_down)
{
    ASSERT(!first_down || up)
    
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    if (first_down || mo->up != up) {
        // signal users
        for (LinkedList1Node *node = LinkedList1_GetFirst(&mo->users); node; node = LinkedList1Node_Next(node)) {
            struct use_instance *user = UPPER_OBJECT(node, struct use_instance, blocker_node);
            ASSERT(user->blocker == mo)
            if (first_down && mo->up) {
                NCDModuleInst_Backend_Down(user->i);
            }
            if (up) {
                NCDModuleInst_Backend_Up(user->i);
            } else {
                NCDModuleInst_Backend_Down(user->i);
            }
        }
        
        // change up state
        mo->up = up;
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void up_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    updown_func_new_templ(i, params, 1, 0);
}

static void down_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    updown_func_new_templ(i, params, 0, 0);
}

static void downup_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    updown_func_new_templ(i, params, 1, 1);
}

static void rdownup_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct rdownup_instance *o = vo;
    o->i = i;
    
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get blocker
    struct instance *blk = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // set blocker
    o->blocker = blk;
    
    // insert to rdownups list
    LinkedList0_Prepend(&blk->rdownups_list, &o->rdownups_list_node);
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void rdownup_func_die (void *vo)
{
    struct rdownup_instance *o = vo;
    
    struct instance *blk = o->blocker;
    
    if (blk) {
        // remove from rdownups list
        LinkedList0_Remove(&blk->rdownups_list, &o->rdownups_list_node);
        
        // downup users
        for (LinkedList1Node *ln = LinkedList1_GetFirst(&blk->users); ln; ln = LinkedList1Node_Next(ln)) {
            struct use_instance *user = UPPER_OBJECT(ln, struct use_instance, blocker_node);
            ASSERT(user->blocker == blk)
            if (blk->up) {
                NCDModuleInst_Backend_Down(user->i);
            }
            NCDModuleInst_Backend_Up(user->i);
        }
        
        // set up
        blk->up = 1;
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void use_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct use_instance *o = vo;
    o->i = i;
    
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // set blocker
    o->blocker = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // add to blocker's list
    LinkedList1_Append(&o->blocker->users, &o->blocker_node);
    
    // signal up if needed
    if (o->blocker->up) {
        NCDModuleInst_Backend_Up(o->i);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void use_func_die (void *vo)
{
    struct use_instance *o = vo;
    
    // remove from blocker's list
    LinkedList1_Remove(&o->blocker->users, &o->blocker_node);
    
    // make the blocker die if needed
    if (o->blocker->dying && LinkedList1_IsEmpty(&o->blocker->users)) {
        instance_free(o->blocker);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static struct NCDModule modules[] = {
    {
        .type = "blocker",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "blocker::up",
        .func_new2 = up_func_new
    }, {
        .type = "blocker::down",
        .func_new2 = down_func_new
    }, {
        .type = "blocker::downup",
        .func_new2 = downup_func_new
    }, {
        .type = "blocker::rdownup",
        .func_new2 = rdownup_func_new,
        .func_die = rdownup_func_die,
        .alloc_size = sizeof(struct rdownup_instance)
    }, {
        .type = "blocker::use",
        .func_new2 = use_func_new,
        .func_die = use_func_die,
        .alloc_size = sizeof(struct use_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_blocker = {
    .modules = modules
};
