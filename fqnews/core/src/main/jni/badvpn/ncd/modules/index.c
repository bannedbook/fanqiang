/**
 * @file index.c
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
 *   index index(string value)
 *   index index::next()
 * 
 * Description:
 *   Non-negative integer with range of a size_t.
 *   The first form creates an index from the given decimal string.
 *   The second form cretes an index with value one more than an existing
 *   index.
 * 
 * Variables:
 *   string (empty) - the index value. Note this may be different from
 *     than the value given to index() if it was not in normal form.
 */

#include <stdlib.h>
#include <string.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_index.h>

struct instance {
    NCDModuleInst *i;
    size_t value;
};

static void func_new_templ (void *vo, NCDModuleInst *i, size_t value)
{
    struct instance *o = vo;
    o->i = i;
    
    // set value
    o->value = value;
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
}

static void func_new_from_value (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    NCDValRef arg_value;
    if (!NCDVal_ListRead(params->args, 1, &arg_value)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // parse value
    uintmax_t value;
    if (!ncd_read_uintmax(arg_value, &value)) {
        ModuleLog(i, BLOG_ERROR, "wrong value");
        goto fail0;
    }
    
    // check overflow
    if (value > SIZE_MAX) {
        ModuleLog(i, BLOG_ERROR, "value too large");
        goto fail0;
    }
    
    func_new_templ(vo, i, value);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_from_index (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *index = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // check overflow
    if (index->value == SIZE_MAX) {
        ModuleLog(i, BLOG_ERROR, "overflow");
        goto fail0;
    }
    
    func_new_templ(vo, i, index->value + 1);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (!strcmp(name, "")) {
        *out = ncd_make_uintmax(mem, o->value);
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "index",
        .func_new2 = func_new_from_value,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "index::next",
        .base_type = "index",
        .func_new2 = func_new_from_index,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_index = {
    .modules = modules
};
