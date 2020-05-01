/**
 * @file depend_scope.c
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
 * Multiple-option dependencies module.
 * 
 * Synopsis:
 *   depend_scope()
 * 
 * Description:
 *   A scope for dependency names. Any dependency names used in provide() and depend()
 *   methods on this object are local to this object. Contrast to the multidepend module,
 *   which provides the same functionality as this module, but with a single global
 *   dependency name scope.
 * 
 * Synopsis:
 *   depend_scope::provide(name)
 * 
 * Arguments:
 *   name - provider identifier
 * 
 * Description:
 *   Satisfies a dependency.
 *   If any depend()'s get immediately bound to this provide(),
 *   the side effects of those first happen, and only then can the process of this
 *   provide() continue.
 *   When provide() is requested to deinitialize, if there are any depend()'s bound,
 *   provide() will not finish deinitializing until all the processes containing the
 *   bound depend()'s have backtracked to the point of the corresponding depend().
 *   More specifically, when backtracking has finished for the last bound depend(),
 *   first the immediate effects of the provide() finshing deinitialization will happen,
 *   and only then will the depend() attempt to rebind. (If the converse was true, the
 *   depend() could rebind, but when deinitialization of the provide()'s process
 *   continues, lose this binding. See ncd/tests/depend_scope.ncd .)
 * 
 * Synopsis:
 *   depend_scope::depend(list names)
 * 
 * Arguments:
 *   names - list of provider identifiers. Names more to the beginning are considered
 *     more desirable.
 * 
 * Description:
 *   Binds to the provide() providing one of the specified dependency names which is most
 *   desirable. If there is no provide() providing any of the given dependency names,
 *   waits and binds when one becomes available. 
 *   If the depend() is bound to a provide(), and the bound provide() is requested to
 *   deinitize, or a more desirable provide() becomes available, the depend() statement
 *   will go down (triggering backtracking), wait for backtracking to finish, and then
 *   try to bind to a provide() again, as if it was just initialized.
 *   When depend() is requested to deinitialize, it deinitializes immediately.
 * 
 * Attributes:
 *   Exposes objects as seen from the corresponding provide.
 */

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <misc/balloc.h>
#include <misc/BRefTarget.h>
#include <structure/LinkedList1.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_depend_scope.h>

struct scope {
    BRefTarget ref_target;
    LinkedList1 provides_list;
    LinkedList1 depends_list;
};

struct scope_instance {
    NCDModuleInst *i;
    struct scope *scope;
};

struct provide {
    NCDModuleInst *i;
    struct scope *scope;
    NCDValRef name;
    LinkedList1Node provides_list_node;
    LinkedList1 depends_list;
    int dying;
};

struct depend {
    NCDModuleInst *i;
    struct scope *scope;
    NCDValRef names;
    LinkedList1Node depends_list_node;
    struct provide *provide;
    LinkedList1Node provide_depends_list_node;
    int provide_collapsing;
};

static struct provide * find_provide (struct scope *o, NCDValRef name)
{
    for (LinkedList1Node *ln = LinkedList1_GetFirst(&o->provides_list); ln; ln = LinkedList1Node_Next(ln)) {
        struct provide *provide = UPPER_OBJECT(ln, struct provide, provides_list_node);
        ASSERT(provide->scope == o)
        if (NCDVal_Compare(provide->name, name) == 0) {
            return provide;
        }
    }
    
    return NULL;
}

static struct provide * depend_find_best_provide (struct depend *o)
{
    size_t count = NCDVal_ListCount(o->names);
    
    for (size_t j = 0; j < count; j++) {
        NCDValRef name = NCDVal_ListGet(o->names, j);
        struct provide *provide = find_provide(o->scope, name);
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

static void scope_ref_target_func_release (BRefTarget *ref_target)
{
    struct scope *o = UPPER_OBJECT(ref_target, struct scope, ref_target);
    ASSERT(LinkedList1_IsEmpty(&o->provides_list))
    ASSERT(LinkedList1_IsEmpty(&o->depends_list))
    
    BFree(o);
}

static void scope_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct scope_instance *o = vo;
    o->i = i;
    
    // pass scope instance pointer to methods not NCDModuleInst pointer
    NCDModuleInst_Backend_PassMemToMethods(i);
    
    // read arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // allocate scope
    o->scope = BAlloc(sizeof(*o->scope));
    if (!o->scope) {
        ModuleLog(i, BLOG_ERROR, "BAlloc failed");
        goto fail0;
    }
    
    // init reference target
    BRefTarget_Init(&o->scope->ref_target, scope_ref_target_func_release);
    
    // init provide and depend lists
    LinkedList1_Init(&o->scope->provides_list);
    LinkedList1_Init(&o->scope->depends_list);
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void scope_func_die (void *vo)
{
    struct scope_instance *o = vo;
    
    // release scope reference
    BRefTarget_Deref(&o->scope->ref_target);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void provide_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct provide *o = vo;
    o->i = i;
    o->scope = ((struct scope_instance *)params->method_user)->scope;
    
    // read arguments
    NCDValRef name_arg;
    if (!NCDVal_ListRead(params->args, 1, &name_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // remember name
    o->name = name_arg;
    
    // check for existing provide with this name
    if (find_provide(o->scope, o->name)) {
        ModuleLog(o->i, BLOG_ERROR, "a provide with this name already exists");
        goto fail0;
    }
    
    // grab scope reference
    if (!BRefTarget_Ref(&o->scope->ref_target)) {
        ModuleLog(o->i, BLOG_ERROR, "BRefTarget_Ref failed");
        goto fail0;
    }
    
    // insert to provides list
    LinkedList1_Append(&o->scope->provides_list, &o->provides_list_node);
    
    // init depends list
    LinkedList1_Init(&o->depends_list);
    
    // set not dying
    o->dying = 0;
    
    // signal up.
    // This comes above the loop which follows, so that effects on related depend statements are
    // computed before this process advances, avoiding problems like failed variable resolutions.
    NCDModuleInst_Backend_Up(o->i);
    
    // update depends
    for (LinkedList1Node *ln = LinkedList1_GetFirst(&o->scope->depends_list); ln; ln = LinkedList1Node_Next(ln)) {
        struct depend *depend = UPPER_OBJECT(ln, struct depend, depends_list_node);
        depend_update(depend);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void provide_free (struct provide *o)
{
    ASSERT(LinkedList1_IsEmpty(&o->depends_list))
    
    // remove from provides list
    LinkedList1_Remove(&o->scope->provides_list, &o->provides_list_node);
    
    // release scope reference
    BRefTarget_Deref(&o->scope->ref_target);
    
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
    struct depend *o = vo;
    o->i = i;
    o->scope = ((struct scope_instance *)params->method_user)->scope;
    
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
    
    // grab scope reference
    if (!BRefTarget_Ref(&o->scope->ref_target)) {
        ModuleLog(o->i, BLOG_ERROR, "BRefTarget_Ref failed");
        goto fail0;
    }
    
    // insert to depends list
    LinkedList1_Append(&o->scope->depends_list, &o->depends_list_node);
    
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
    
    if (o->provide) {
        // remove from provide's list
        LinkedList1_Remove(&o->provide->depends_list, &o->provide_depends_list_node);
        
        // if provide is dying and is empty, let it die
        if (o->provide->dying && LinkedList1_IsEmpty(&o->provide->depends_list)) {
            provide_free(o->provide);
        }
    }
    
    // remove from depends list
    LinkedList1_Remove(&o->scope->depends_list, &o->depends_list_node);
    
    // release scope reference
    BRefTarget_Deref(&o->scope->ref_target);
    
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
    
    // if provide is dying and is empty, let it die.
    // This comes after depend_update so that the side effects of the
    // provide dying have priority over rebinding the depend.
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
        .type = "depend_scope",
        .func_new2 = scope_func_new,
        .func_die = scope_func_die,
        .alloc_size = sizeof(struct scope_instance)
    }, {
        .type = "depend_scope::provide",
        .func_new2 = provide_func_new,
        .func_die = provide_func_die,
        .alloc_size = sizeof(struct provide)
    }, {
        .type = "depend_scope::depend",
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

const struct NCDModuleGroup ncdmodule_depend_scope = {
    .modules = modules
};
