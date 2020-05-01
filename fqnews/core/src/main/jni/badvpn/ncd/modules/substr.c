/**
 * @file substr.c
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
 *   substr(string str, string start [, string max])
 * 
 * Description:
 *   Extracts a substring from a string. The result is the longest substring which
 *   starts at the offset 'start' bytes into 'str', and is no longer than 'max' bytes.
 *   If 'max' is not provided, the result is the substring from the offset 'start' to
 *   the end. In any case, 'start' must not be greater than the length of 'str'.
 */

#include <stddef.h>
#include <string.h>
#include <limits.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_substr.h>

struct substr_instance {
    NCDModuleInst *i;
    MemRef data;
    int is_external;
    BRefTarget *external_ref_target;
};

static void substr_func_new_common (void *vo, NCDModuleInst *i, MemRef data, int is_external, BRefTarget *external_ref_target)
{
    struct substr_instance *o = vo;
    o->i = i;
    
    o->data = data;
    o->is_external = is_external;
    o->external_ref_target = external_ref_target;
    
    NCDModuleInst_Backend_Up(i);
}

static int substr_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct substr_instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        if (o->is_external) {
            *out = NCDVal_NewExternalString(mem, o->data.ptr, o->data.len, o->external_ref_target);
        } else {
            *out = NCDVal_NewStringBinMr(mem, o->data);
        }
        return 1;
    }
    
    return 0;
}

static void func_new_substr (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef str_arg;
    NCDValRef start_arg;
    NCDValRef max_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 2, &str_arg, &start_arg) &&
        !NCDVal_ListRead(params->args, 3, &str_arg, &start_arg, &max_arg)
    ) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(str_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    uintmax_t start;
    if (!ncd_read_uintmax(start_arg, &start) || start > SIZE_MAX) {
        ModuleLog(i, BLOG_ERROR, "wrong size");
        goto fail0;
    }
    
    uintmax_t max = SIZE_MAX;
    if (!NCDVal_IsInvalid(max_arg)) {
        if (!ncd_read_uintmax(max_arg, &max) || max > SIZE_MAX) {
            ModuleLog(i, BLOG_ERROR, "wrong max");
            goto fail0;
        }
    }
    
    MemRef str = NCDVal_StringMemRef(str_arg);
    
    if (start > str.len) {
        ModuleLog(i, BLOG_ERROR, "start is beyond the end of the string");
        goto fail0;
    }
    
    const char *sub_data = str.ptr + start;
    size_t sub_length = str.len - start;
    if (sub_length > max) {
        sub_length = max;
    }
    
    int is_external = 0;
    BRefTarget *external_ref_target = NULL;
    
    if (NCDVal_IsExternalString(str_arg)) {
        is_external = 1;
        external_ref_target = NCDVal_ExternalStringTarget(str_arg);
    }
    else if (NCDVal_IsIdString(str_arg)) {
        is_external = 1;
    }
    
    substr_func_new_common(vo, i, MemRef_Make(sub_data, sub_length), is_external, external_ref_target);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "substr",
        .func_new2 = func_new_substr,
        .func_getvar2 = substr_func_getvar,
        .alloc_size = sizeof(struct substr_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_substr = {
    .modules = modules
};
