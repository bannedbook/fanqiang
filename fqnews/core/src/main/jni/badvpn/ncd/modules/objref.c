/**
 * @file objref.c
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
 * Synopsis:
 *   objref(list(string) target)
 *   objref_arg(list(string) target)
 */

#include <misc/debug.h>
#include <misc/balloc.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_objref.h>

struct instance {
    NCDModuleInst *i;
    NCDObjRef ref;
};

static void func_new_common (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int is_arg)
{
    ASSERT(is_arg == 0 || is_arg == 1)
    
    struct instance *o = vo;
    o->i = i;
    
    NCDValRef target_arg;
    if (!NCDVal_ListRead(params->args, 1, &target_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    if (!NCDVal_IsList(target_arg)) {
        ModuleLog(i, BLOG_ERROR, "argument is not a list");
        goto fail0;
    }
    
    size_t num_names = NCDVal_ListCount(target_arg);
    if (num_names == 0) {
        ModuleLog(i, BLOG_ERROR, "target list is empty");
        goto fail0;
    }
    
    size_t total_names = is_arg + num_names;
    
    NCD_string_id_t *names = BAllocArray(total_names, sizeof(*names));
    if (!names) {
        ModuleLog(i, BLOG_ERROR, "BAllocArray failed");
        goto fail0;
    }
    
    if (is_arg) {
        names[0] = NCD_STRING_CALLER;
    }
    
    for (size_t j = 0; j < num_names; j++) {
        NCDValRef name_val = NCDVal_ListGet(target_arg, j);
        
        if (!NCDVal_IsString(name_val)) {
            ModuleLog(i, BLOG_ERROR, "target component is not a string");
            goto fail1;
        }
        
        NCD_string_id_t name_id = ncd_get_string_id(name_val);
        if (name_id < 0) {
            ModuleLog(i, BLOG_ERROR, "ncd_get_string_id failed");
            goto fail1;
        }
        
        names[is_arg + j] = name_id;
    }
    
    NCDObject first_obj;
    if (!NCDModuleInst_Backend_GetObj(i, names[0], &first_obj)) {
        ModuleLog(i, BLOG_ERROR, "first target object not found");
        goto fail1;
    }
    
    NCDObject obj;
    if (!NCDObject_ResolveObjExprCompact(&first_obj, names + 1, total_names - 1, &obj)) {
        ModuleLog(i, BLOG_ERROR, "non-first target object not found");
        goto fail1;
    }
    
    NCDPersistentObj *pobj = NCDObject_Pobj(&obj);
    if (!pobj) {
        ModuleLog(i, BLOG_ERROR, "object does not support references");
        goto fail1;
    }
    
    NCDObjRef_Init(&o->ref, pobj);
    
    BFree(names);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail1:
    BFree(names);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_objref (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_common(vo, i, params, 0);
}

static void func_new_objref_arg (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_common(vo, i, params, 1);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    NCDObjRef_Free(&o->ref);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getobj (void *vo, NCD_string_id_t name, NCDObject *out_object)
{
    struct instance *o = vo;
    
    NCDObject obj;
    if (!NCDObjRef_Deref(&o->ref, &obj)) {
        ModuleLog(o->i, BLOG_ERROR, "object reference has been broken");
        return 0;
    }
    
    if (name == NCD_STRING_EMPTY) {
        *out_object = obj;
        return 1;
    }
    
    return NCDObject_GetObj(&obj, name, out_object);
}

static struct NCDModule modules[] = {
    {
        .type = "objref",
        .func_new2 = func_new_objref,
        .func_die = func_die,
        .func_getobj = func_getobj,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "objref_arg",
        .func_new2 = func_new_objref_arg,
        .func_die = func_die,
        .func_getobj = func_getobj,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_objref = {
    .modules = modules
};
