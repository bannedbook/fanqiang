/**
 * @file multidepend.c
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
 * This is a compatibility module. It behaves exactly like the depend_scope module,
 * except that there is a single global scope for dependency names.
 * 
 * Use depend_scope instead. If you are using multidepend between non-template
 * processes, make those processes templates instead and start them via
 * process_manager(). For example, instead of this:
 * 
 *   process foo {
 *       multiprovide("FOO");
 *   }
 *   
 *   process bar {
 *       multidepend({"FOO"});
 *   }
 * 
 * Use this:
 * 
 *   process main {
 *       depend_scope() scope;
 *       process_manager() mgr;
 *       mgr->start("foo", "foo", {});
 *       mgr->start("bar", "bar", {});
 *   }
 *   
 *   template foo {
 *       _caller.scope->provide("FOO");
 *   }
 *   
 *   template bar {
 *       _caller.scope->depend({"FOO"});
 *   }
 * 
 * Synopsis:
 *   multiprovide(name)
 * 
 * Synopsis:
 *   multidepend(list names)
 */

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <misc/balloc.h>
#include <structure/LinkedList1.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_multidepend.h>

struct provide {
    NCDModuleInst *i;
    NCDValRef name;
    LinkedList1Node provides_list_node;
    LinkedList1 depends_list;
    int dying;
};

struct depend {
    NCDModuleInst *i;
    NCDValRef names;
    LinkedList1Node depends_list_node;
    struct provide *provide;
    LinkedList1Node provide_depends_list_node;
    int provide_collapsing;
};

struct global {
    LinkedList1 provides_list;
    LinkedList1 depends_list;
};

static struct provide * find_provide (struct global *g, NCDValRef name)
{
    for (LinkedList1Node *ln = LinkedList1_GetFirst(&g->provides_list); ln; ln = LinkedList1Node_Next(ln)) {
        struct provide *provide = UPPER_OBJECT(ln, struct provide, provides_list_node);
        if (NCDVal_Compare(provide->name, name) == 0) {
            return provide;
        }
    }
    
    return NULL;
}

static struct provide * depend_find_best_provide (struct depend *o)
{
    struct global *g = ModuleGlobal(o->i);
    
    size_t count = NCDVal_ListCount(o->names);
    
    for (size_t j = 0; j < count; j++) {
        NCDValRef name = NCDVal_ListGet(o->names, j);
        struct provide *provide = find_provide(g, name);
        if (provide && !provide->dying) {
            return provide;
        }
    }
    
    return NULL;
}

static void depend_update (struct depend *o)
{
    // if we're collapsing, do nothing
    if (o->provide && o->provide_collapsing) {
        return;
    }
    
    // find best provide
    struct provide *best_provide = depend_find_best_provide(o);
    ASSERT(!best_provide || !best_provide->dying)
    
    // has anything changed?
    if (best_provide == o->provide) {
        return;
    }
    
    if (o->provide) {
        // set collapsing
        o->provide_collapsing = 1;
        
        // signal down
        NCDModuleInst_Backend_Down(o->i);
    } else {
        // insert to provide's list
        LinkedList1_Append(&best_provide->depends_list, &o->provide_depends_list_node);
        
        // set not collapsing
        o->provide_collapsing = 0;
        
        // set provide
        o->provide = best_provide;
        
        // signal up
        NCDModuleInst_Backend_Up(o->i);
    }
}

static int func_globalinit (struct NCDInterpModuleGroup *group, const struct NCDModuleInst_iparams *params)
{
    // allocate global state structure
    struct global *g = BAlloc(sizeof(*g));
    if (!g) {
        BLog(BLOG_ERROR, "BAlloc failed");
        return 0;
    }
    
    // set group state pointer
    group->group_state = g;
    
    // init provides list
    LinkedList1_Init(&g->provides_list);
    
    // init depends list
    LinkedList1_Init(&g->depends_list);
    
    return 1;
}

static void func_globalfree (struct NCDInterpModuleGroup *group)
{
    struct global *g = group->group_state;
    ASSERT(LinkedList1_IsEmpty(&g->depends_list))
    ASSERT(LinkedList1_IsEmpty(&g->provides_list))
    
    // free global state structure
    BFree(g);
}

static void provide_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct global *g = ModuleGlobal(i);
    struct provide *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef name_arg;
    if (!NCDVal_ListRead(params->args, 1, &name_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // remember name
    o->name = name_arg;
    
    // check for existing provide with this name
    if (find_provide(g, o->name)) {
        ModuleLog(o->i, BLOG_ERROR, "a provide with this name already exists");
        goto fail0;
    }
    
    // insert to provides list
    LinkedList1_Append(&g->provides_list, &o->provides_list_node);
    
    // init depends list
    LinkedList1_Init(&o->depends_list);
    
    // set not dying
    o->dying = 0;
    
    // signal up.
    // This comes above the loop which follows, so that effects on related depend statements are
    // computed before this process advances, avoiding problems like failed variable resolutions.
    NCDModuleInst_Backend_Up(o->i);
    
    // update depends
    for (LinkedList1Node *ln = LinkedList1_GetFirst(&g->depends_list); ln; ln = LinkedList1Node_Next(ln)) {
        struct depend *depend = UPPER_OBJECT(ln, struct depend, depends_list_node);
        depend_update(depend);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void provide_free (struct provide *o)
{
    struct global *g = ModuleGlobal(o->i);
    ASSERT(LinkedList1_IsEmpty(&o->depends_list))
    
    // remove from provides list
    LinkedList1_Remove(&g->provides_list, &o->provides_list_node);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void provide_func_die (void *vo)
{
    struct provide *o = vo;
    ASSERT(!o->dying)
    
    // if we have no depends, die immediately
    if (LinkedList1_IsEmpty(&o->depends_list)) {
        provide_free(o);
        return;
    }
    
    // set dying
    o->dying = 1;
    
    // start collapsing our depends
    for (LinkedList1Node *ln = LinkedList1_GetFirst(&o->depends_list); ln; ln = LinkedList1Node_Next(ln)) {
        struct depend *depend = UPPER_OBJECT(ln, struct depend, provide_depends_list_node);
        ASSERT(depend->provide == o)
        
        // update depend to make sure it is collapsing
        depend_update(depend);
    }
}

static void depend_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct global *g = ModuleGlobal(i);
    struct depend *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef names_arg;
    if (!NCDVal_ListRead(params->args, 1, &names_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsList(names_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // remember names
    o->names = names_arg;
    
    // insert to depends list
    LinkedList1_Append(&g->depends_list, &o->depends_list_node);
    
    // set no provide
    o->provide = NULL;
    
    // update
    depend_update(o);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void depend_func_die (void *vo)
{
    struct depend *o = vo;
    struct global *g = ModuleGlobal(o->i);
    
    if (o->provide) {
        // remove from provide's list
        LinkedList1_Remove(&o->provide->depends_list, &o->provide_depends_list_node);
        
        // if provide is dying and is empty, let it die
        if (o->provide->dying && LinkedList1_IsEmpty(&o->provide->depends_list)) {
            provide_free(o->provide);
        }
    }
    
    // remove from depends list
    LinkedList1_Remove(&g->depends_list, &o->depends_list_node);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void depend_func_clean (void *vo)
{
    struct depend *o = vo;
    
    if (!(o->provide && o->provide_collapsing)) {
        return;
    }
    
    // save provide
    struct provide *provide = o->provide;
    
    // remove from provide's list
    LinkedList1_Remove(&provide->depends_list, &o->provide_depends_list_node);
    
    // set no provide
    o->provide = NULL;
    
    // update
    depend_update(o);
    
    // if provide is dying and is empty, let it die
    if (provide->dying && LinkedList1_IsEmpty(&provide->depends_list)) {
        provide_free(provide);
    }
}

static int depend_func_getobj (void *vo, NCD_string_id_t objname, NCDObject *out_object)
{
    struct depend *o = vo;
    
    if (!o->provide) {
        return 0;
    }
    
    return NCDModuleInst_Backend_GetObj(o->provide->i, objname, out_object);
}

static struct NCDModule modules[] = {
    {
        .type = "multiprovide",
        .func_new2 = provide_func_new,
        .func_die = provide_func_die,
        .alloc_size = sizeof(struct provide)
    }, {
        .type = "multidepend",
        .func_new2 = depend_func_new,
        .func_die = depend_func_die,
        .func_clean = depend_func_clean,
        .func_getobj = depend_func_getobj,
        .flags = NCDMODULE_FLAG_CAN_RESOLVE_WHEN_DOWN,
        .alloc_size = sizeof(struct depend)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_multidepend = {
    .func_globalinit = func_globalinit,
    .func_globalfree = func_globalfree,
    .modules = modules
};
