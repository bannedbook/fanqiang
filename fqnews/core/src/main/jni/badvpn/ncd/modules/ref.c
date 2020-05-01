/**
 * @file ref.c
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
 * References module.
 * 
 * Synopsis:
 *   refhere()
 * Variables:
 *   Exposes variables and objects as seen from this refhere() statement.
 * 
 * Synopsis:
 *   ref refhere::ref()
 *   ref ref::ref()
 * Variables:
 *   Exposes variables and objects as seen from the corresponding refhere()
 *   statement.
 */

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <structure/LinkedList0.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_ref.h>

struct refhere_instance {
    NCDModuleInst *i;
    LinkedList0 refs_list;
};

struct ref_instance {
    NCDModuleInst *i;
    struct refhere_instance *rh;
    LinkedList0Node refs_list_node;
};

static void ref_instance_free (struct ref_instance *o);

static void refhere_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct refhere_instance *o = vo;
    o->i = i;
    
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // init refs list
    LinkedList0_Init(&o->refs_list);
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void refhere_func_die (void *vo)
{
    struct refhere_instance *o = vo;
    
    // die refs
    while (!LinkedList0_IsEmpty(&o->refs_list)) {
        struct ref_instance *ref = UPPER_OBJECT(LinkedList0_GetFirst(&o->refs_list), struct ref_instance, refs_list_node);
        ASSERT(ref->rh == o)
        ref_instance_free(ref);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int refhere_func_getobj (void *vo, NCD_string_id_t objname, NCDObject *out_object)
{
    struct refhere_instance *o = vo;
    
    // We don't redirect methods, and there will never be an object
    // with empty name. Fail here so we don't report non-errors.
    if (objname == NCD_STRING_EMPTY) {
        return 0;
    }
    
    return NCDModuleInst_Backend_GetObj(o->i, objname, out_object);
}

static void ref_func_new_templ (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, struct refhere_instance *rh)
{
    struct ref_instance *o = vo;
    o->i = i;
    
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // set refhere
    o->rh = rh;
    
    // add to refhere's refs list
    LinkedList0_Prepend(&o->rh->refs_list, &o->refs_list_node);
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void ref_func_new_from_refhere (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct refhere_instance *rh = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    return ref_func_new_templ(vo, i, params, rh);
}

static void ref_func_new_from_ref (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct ref_instance *ref = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    return ref_func_new_templ(vo, i, params, ref->rh);
}

static void ref_instance_free (struct ref_instance *o)
{
    // remove from refhere's reft list
    LinkedList0_Remove(&o->rh->refs_list, &o->refs_list_node);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void ref_func_die (void *vo)
{
    struct ref_instance *o = vo;
    
    ref_instance_free(o);
}

static int ref_func_getobj (void *vo, NCD_string_id_t objname, NCDObject *out_object)
{
    struct ref_instance *o = vo;
    
    // We don't redirect methods, and there will never be an object
    // with empty name. Fail here so we don't report non-errors.
    if (objname == NCD_STRING_EMPTY) {
        return 0;
    }
    
    return NCDModuleInst_Backend_GetObj(o->rh->i, objname, out_object);
}

static struct NCDModule modules[] = {
    {
        .type = "refhere",
        .func_new2 = refhere_func_new,
        .func_die = refhere_func_die,
        .func_getobj = refhere_func_getobj,
        .alloc_size = sizeof(struct refhere_instance)
    }, {
        .type = "refhere::ref",
        .base_type = "ref",
        .func_new2 = ref_func_new_from_refhere,
        .func_die = ref_func_die,
        .func_getobj = ref_func_getobj,
        .alloc_size = sizeof(struct ref_instance)
    }, {
        .type = "ref::ref",
        .base_type = "ref",
        .func_new2 = ref_func_new_from_ref,
        .func_die = ref_func_die,
        .func_getobj = ref_func_getobj,
        .alloc_size = sizeof(struct ref_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_ref = {
    .modules = modules
};
