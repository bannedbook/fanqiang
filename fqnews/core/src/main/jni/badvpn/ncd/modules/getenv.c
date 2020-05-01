/**
 * @file getenv.c
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
 *   getenv(string name)
 * 
 * Variables:
 *   string (empty) - if the environment value exists, its value
 *   string exists - "true" if the variable exists, "false" if not
 */

#include <stdlib.h>

#include <misc/strdup.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_getenv.h>

struct instance {
    NCDModuleInst *i;
    char *value;
};

enum {STRING_EXISTS};

static const char *strings[] = {"exists", NULL};

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef name_arg;
    if (!NCDVal_ListRead(params->args, 1, &name_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(name_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    o->value = NULL;
    
    if (NCDVal_StringHasNulls(name_arg)) {
        goto out;
    }
    
    NCDValNullTermString nts;
    if (!NCDVal_StringNullTerminate(name_arg, &nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    char *result = getenv(nts.data);
    NCDValNullTermString_Free(&nts);
    if (!result) {
        goto out;
    }
    
    o->value = b_strdup(result);
    if (!o->value) {
        ModuleLog(i, BLOG_ERROR, "b_strdup failed");
        goto fail0;
    }
    
out:    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    // free value
    if (o->value) {
        BFree(o->value);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (name == NCD_STRING_EMPTY && o->value) {
        *out = NCDVal_NewString(mem, o->value);
        return 1;
    }
    
    if (name == ModuleString(o->i, STRING_EXISTS)) {
        *out = ncd_make_boolean(mem, !!o->value);
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "getenv",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar2 = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_getenv = {
    .modules = modules,
    .strings = strings
};
