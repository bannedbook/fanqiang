/**
 * @file var.c
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
 * Variable module.
 * 
 * Synopsis: var(value)
 * Variables:
 *   (empty) - value
 * 
 * Synopsis: var::set(value)
 */

#include <stdlib.h>
#include <string.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_var.h>

struct instance {
    NCDModuleInst *i;
    NCDValMem mem;
    NCDValRef value;
};

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // read argument
    NCDValRef value_arg;
    if (!NCDVal_ListRead(params->args, 1, &value_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // init mem
    NCDValMem_Init(&o->mem, i->params->iparams->string_index);
    
    // copy value
    o->value = NCDVal_NewCopy(&o->mem, value_arg);
    if (NCDVal_IsInvalid(o->value)) {
        goto fail1;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail1:
    NCDValMem_Free(&o->mem);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    // free mem
    NCDValMem_Free(&o->mem);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        *out = NCDVal_NewCopy(mem, o->value);
        return 1;
    }
    
    return 0;
}

static void set_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    NCDValRef value_arg;
    if (!NCDVal_ListRead(params->args, 1, &value_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // allocate new mem
    NCDValMem mem;
    NCDValMem_Init(&mem, i->params->iparams->string_index);
    
    // copy value
    NCDValRef copy = NCDVal_NewCopy(&mem, value_arg);
    if (NCDVal_IsInvalid(copy)) {
        goto fail1;
    }
    
    // replace value in var
    NCDValMem_Free(&mo->mem);
    mo->mem = mem;
    mo->value = NCDVal_Moved(&mo->mem, copy);
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail1:
    NCDValMem_Free(&mem);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "var",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "var::set",
        .func_new2 = set_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_var = {
    .modules = modules
};
