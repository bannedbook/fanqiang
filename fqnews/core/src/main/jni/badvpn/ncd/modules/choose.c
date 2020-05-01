/**
 * @file choose.c
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
 * Multiple value selection based on boolean conditions.
 * 
 * Synopsis:
 *   choose({{string cond1, result1}, ..., {string condN, resultN}}, default_result)
 * 
 * Variables:
 *   (empty) - If cond1="true" then result1,
 *             else if cond2="true" then result2,
 *             ...,
 *             else default_result.
 */

#include <stdlib.h>
#include <string.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_choose.h>

struct instance {
    NCDModuleInst *i;
    NCDValRef result;
};

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef arg_choices;
    NCDValRef arg_default_result;
    if (!NCDVal_ListRead(params->args, 2, &arg_choices, &arg_default_result)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsList(arg_choices)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // iterate choices
    int have_result = 0;
    size_t count = NCDVal_ListCount(arg_choices);
    for (size_t j = 0; j < count; j++) {
        NCDValRef c = NCDVal_ListGet(arg_choices, j);
        
        // check choice type
        if (!NCDVal_IsList(c)) {
            ModuleLog(i, BLOG_ERROR, "wrong choice type");
            goto fail0;
        }
        
        // read choice
        NCDValRef c_cond;
        NCDValRef c_result;
        if (!NCDVal_ListRead(c, 2, &c_cond, &c_result)) {
            ModuleLog(i, BLOG_ERROR, "wrong choice contents arity");
            goto fail0;
        }
        int c_cond_val;
        if (!ncd_read_boolean(c_cond, &c_cond_val)) {
            ModuleLog(i, BLOG_ERROR, "wrong choice condition");
            goto fail0;
        }
        
        // update result
        if (!have_result && c_cond_val) {
            o->result = c_result;
            have_result = 1;
        }
    }
    
    // default?
    if (!have_result) {
        o->result = arg_default_result;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static int func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        *out = NCDVal_NewCopy(mem, o->result);
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "choose",
        .func_new2 = func_new,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_choose = {
    .modules = modules
};
