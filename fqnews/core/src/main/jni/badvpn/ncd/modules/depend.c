/**
 * @file depend.c
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
 * Dependencies module.
 * 
 * Synopsis: provide(string name)
 * Description: Provides a resource. On initialization, transitions any depend()-s
 *   waiting for this resource to UP state. On deinitialization, transitions
 *   depend()-s using this resource to DOWN state, and waits for all of them to
 *   receive the clean signal (i.e. wait for all of the statements following them in
 *   their processes to terminate). Initialization fails if a provide() already
 *   exists for this resource (including if it is being deinitialized).
 * 
 * Synopsis: provide_event(string name)
 * Description: Like provide(), but if another provide() already exists for this
 *   resource, initialization does not fail, and the request is queued to the active
 *   provide() for this resource. When an active provide() disappears that has
 *   queued provide()-s, one of them is promoted to be the active provide() for this
 *   resource, and the remaining queue is transferred to it.
 *   (mentions of provide() in this text also apply to provide_event())
 * 
 * Synopsis: depend(string name)
 * Description: Depends on a resource. Is in UP state when a provide()
 *   for this resource is available, and in DOWN state when it is not (either
 *   it does not exist or is being terminated).
 * Variables: Provides variables available from the corresponding provide,
 *     ("modname.varname" or "modname").
 */

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <misc/balloc.h>
#include <structure/LinkedList1.h>
#include <structure/LinkedList3.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_depend.h>

struct provide {
    NCDModuleInst *i;
    MemRef name;
    int is_queued;
    union {
        struct {
            LinkedList3Node queued_node; // node in list which begins with provide.queued_provides_firstnode
        };
        struct {
            LinkedList1Node provides_node; // node in provides
            LinkedList1 depends;
            LinkedList3Node queued_provides_firstnode;
            int dying;
        };
    };
};

struct depend {
    NCDModuleInst *i;
    MemRef name;
    struct provide *p;
    LinkedList1Node node;
};

struct global {
    LinkedList1 provides;
    LinkedList1 free_depends;
};

static struct provide * find_provide (struct global *g, MemRef name)
{
    for (LinkedList1Node *n = LinkedList1_GetFirst(&g->provides); n; n = LinkedList1Node_Next(n)) {
        struct provide *p = UPPER_OBJECT(n, struct provide, provides_node);
        ASSERT(!p->is_queued)
        
        if (MemRef_Equal(p->name, name)) {
            return p;
        }
    }
    
    return NULL;
}

static void provide_promote (struct provide *o)
{
    struct global *g = ModuleGlobal(o->i);
    ASSERT(!find_provide(g, o->name))
    
    // set not queued
    o->is_queued = 0;
    
    // insert to provides list
    LinkedList1_Append(&g->provides, &o->provides_node);
    
    // init depends list
    LinkedList1_Init(&o->depends);
    
    // set not dying
    o->dying = 0;
    
    // attach free depends with this name
    LinkedList1Node *n = LinkedList1_GetFirst(&g->free_depends);
    while (n) {
        LinkedList1Node *next = LinkedList1Node_Next(n);
        struct depend *d = UPPER_OBJECT(n, struct depend, node);
        ASSERT(!d->p)
        
        if (!MemRef_Equal(d->name, o->name)) {
            n = next;
            continue;
        }
        
        // remove from free depends list
        LinkedList1_Remove(&g->free_depends, &d->node);
        
        // insert to provide's list
        LinkedList1_Append(&o->depends, &d->node);
        
        // set provide
        d->p = o;
        
        // signal up
        NCDModuleInst_Backend_Up(d->i);
        
        n = next;
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
    LinkedList1_Init(&g->provides);
    
    // init free depends list
    LinkedList1_Init(&g->free_depends);
    
    return 1;
}

static void func_globalfree (struct NCDInterpModuleGroup *group)
{
    struct global *g = group->group_state;
    ASSERT(LinkedList1_IsEmpty(&g->free_depends))
    ASSERT(LinkedList1_IsEmpty(&g->provides))
    
    // free global state structure
    BFree(g);
}

static void provide_func_new_templ (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int event)
{
    struct global *g = ModuleGlobal(i);
    struct provide *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef name_arg;
    if (!NCDVal_ListRead(params->args, 1, &name_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(name_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    o->name = NCDVal_StringMemRef(name_arg);
    
    // signal up.
    // This comes above provide_promote(), so that effects on related depend statements are
    // computed before this process advances, avoiding problems like failed variable resolutions.
    NCDModuleInst_Backend_Up(o->i);
    
    // check for existing provide with this name
    struct provide *ep = find_provide(g, o->name);
    if (ep) {
        ASSERT(!ep->is_queued)
        
        if (!event) {
            ModuleLog(o->i, BLOG_ERROR, "a provide with this name already exists");
            goto fail0;
        }
        
        // set queued
        o->is_queued = 1;
        
        // insert to existing provide's queued provides list
        LinkedList3Node_InitAfter(&o->queued_node, &ep->queued_provides_firstnode);
    } else {
        // init first node for queued provides list
        LinkedList3Node_InitLonely(&o->queued_provides_firstnode);
        
        // promote provide
        provide_promote(o);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void provide_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    provide_func_new_templ(vo, i, params, 0);
}

static void provide_event_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    provide_func_new_templ(vo, i, params, 1);
}

static void provide_free (struct provide *o)
{
    struct global *g = ModuleGlobal(o->i);
    ASSERT(o->is_queued || LinkedList1_IsEmpty(&o->depends))
    
    if (o->is_queued) {
        // remove from existing provide's queued provides list
        LinkedList3Node_Free(&o->queued_node);
    } else {
        // remove from provides list
        LinkedList1_Remove(&g->provides, &o->provides_node);
        
        // if we have provides queued, promote the first one
        if (LinkedList3Node_Next(&o->queued_provides_firstnode)) {
            // get first queued provide
            struct provide *qp = UPPER_OBJECT(LinkedList3Node_Next(&o->queued_provides_firstnode), struct provide, queued_node);
            ASSERT(qp->is_queued)
            
            // make it the head of the queued provides list
            LinkedList3Node_Free(&qp->queued_node);
            LinkedList3Node_InitAfter(&qp->queued_provides_firstnode, &o->queued_provides_firstnode);
            LinkedList3Node_Free(&o->queued_provides_firstnode);
            
            // promote provide
            provide_promote(qp);
        }
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void provide_func_die (void *vo)
{
    struct provide *o = vo;
    ASSERT(o->is_queued || !o->dying)
    
    // if we are queued or have no depends, die immediately
    if (o->is_queued || LinkedList1_IsEmpty(&o->depends)) {
        provide_free(o);
        return;
    }
    
    // set dying
    o->dying = 1;
    
    // signal our depends down
    for (LinkedList1Node *n = LinkedList1_GetFirst(&o->depends); n; n = LinkedList1Node_Next(n)) {
        struct depend *d = UPPER_OBJECT(n, struct depend, node);
        ASSERT(d->p == o)
        
        // signal down
        NCDModuleInst_Backend_Down(d->i);
    }
}

static void depend_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct global *g = ModuleGlobal(i);
    struct depend *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef name_arg;
    if (!NCDVal_ListRead(params->args, 1, &name_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(name_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    o->name = NCDVal_StringMemRef(name_arg);
    
    // find a provide with our name
    struct provide *p = find_provide(g, o->name);
    ASSERT(!p || !p->is_queued)
    
    if (p && !p->dying) {
        // insert to provide's list
        LinkedList1_Append(&p->depends, &o->node);
        
        // set provide
        o->p = p;
        
        // signal up
        NCDModuleInst_Backend_Up(o->i);
    } else {
        // insert to free depends list
        LinkedList1_Append(&g->free_depends, &o->node);
        
        // set no provide
        o->p = NULL;
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void depend_free (struct depend *o)
{
    struct global *g = ModuleGlobal(o->i);
    ASSERT(!o->p || !o->p->is_queued)
    
    if (o->p) {
        // remove from provide's list
        LinkedList1_Remove(&o->p->depends, &o->node);
        
        // if provide is dying and is empty, let it die
        if (o->p->dying && LinkedList1_IsEmpty(&o->p->depends)) {
            provide_free(o->p);
        }
    } else {
        // remove free depends list
        LinkedList1_Remove(&g->free_depends, &o->node);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void depend_func_die (void *vo)
{
    struct depend *o = vo;
    
    depend_free(o);
}

static void depend_func_clean (void *vo)
{
    struct depend *o = vo;
    struct global *g = ModuleGlobal(o->i);
    ASSERT(!o->p || !o->p->is_queued)
    
    if (!(o->p && o->p->dying)) {
        return;
    }
    
    struct provide *p = o->p;
    
    // remove from provide's list
    LinkedList1_Remove(&p->depends, &o->node);
    
    // insert to free depends list
    LinkedList1_Append(&g->free_depends, &o->node);
    
    // set no provide
    o->p = NULL;
    
    // if provide is empty, let it die
    if (LinkedList1_IsEmpty(&p->depends)) {
        provide_free(p);
    }
}

static int depend_func_getobj (void *vo, NCD_string_id_t objname, NCDObject *out_object)
{
    struct depend *o = vo;
    ASSERT(!o->p || !o->p->is_queued)
    
    if (!o->p) {
        return 0;
    }
    
    return NCDModuleInst_Backend_GetObj(o->p->i, objname, out_object);
}

static struct NCDModule modules[] = {
    {
        .type = "provide",
        .func_new2 = provide_func_new,
        .func_die = provide_func_die,
        .alloc_size = sizeof(struct provide)
    }, {
        .type = "provide_event",
        .func_new2 = provide_event_func_new,
        .func_die = provide_func_die,
        .alloc_size = sizeof(struct provide)
    }, {
        .type = "depend",
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

const struct NCDModuleGroup ncdmodule_depend = {
    .func_globalinit = func_globalinit,
    .func_globalfree = func_globalfree,
    .modules = modules
};
