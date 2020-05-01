/**
 * @file explode.c
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
 * 
 * Synopsis:
 *   compile_search(string str)
 * 
 * Description:
 *   Performs calculations to enable efficient string searches for the
 *   given string (builds the KMP table). The string must be non-empty.
 * 
 * 
 * Synopsis:
 *   explode(string delimiter, string input [, string limit])
 *   compile_search::explode(string input [, string limit])
 * 
 * Description:
 *   Splits the string 'input' into a list of components. The first component
 *   is the part of 'input' until the first occurence of 'delimiter', if any.
 *   If 'delimiter' was found, the remaining components are defined recursively
 *   via the same procedure, starting with the part of 'input' after the first
 *   substring.
 *   'delimiter' must be nonempty.
 *   The compile_search variant uses an precompiled delimiter string for better
 *   performance.
 * 
 * Variables:
 *   list (empty) - the components of 'input', determined based on 'delimiter'
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <misc/exparray.h>
#include <misc/string_begins_with.h>
#include <misc/substring.h>
#include <misc/balloc.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_explode.h>

struct compile_search_instance {
    NCDModuleInst *i;
    MemRef str;
    size_t *table;
};

struct instance {
    NCDModuleInst *i;
    NCDValRef input;
    struct ExpArray arr;
    size_t num;
};

static void compile_search_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct compile_search_instance *o = vo;
    o->i = i;
    
    NCDValRef str_arg;
    if (!NCDVal_ListRead(params->args, 1, &str_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(str_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    o->str = NCDVal_StringMemRef(str_arg);
    if (o->str.len == 0) {
        ModuleLog(i, BLOG_ERROR, "string must be nonempty");
        goto fail0;
    }
    
    o->table = BAllocArray(o->str.len, sizeof(o->table[0]));
    if (!o->table) {
        ModuleLog(i, BLOG_ERROR, "BAllocArray failed");
        goto fail0;
    }
    
    build_substring_backtrack_table(o->str, o->table);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void compile_search_die (void *vo)
{
    struct compile_search_instance *o = vo;
    
    BFree(o->table);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_new_common (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, struct compile_search_instance *compiled)
{
    struct instance *o = vo;
    o->i = i;
    
    int arg_start;
    MemRef del;
    size_t const *table;
    
    if (compiled) {
        arg_start = 0;
        del = compiled->str;
        table = compiled->table;
    } else {
        NCDValRef delimiter_arg;
        if (!NCDVal_ListReadHead(params->args, 1, &delimiter_arg)) {
            ModuleLog(i, BLOG_ERROR, "missing delimiter argument");
            goto fail0;
        }
        if (!NCDVal_IsString(delimiter_arg)) {
            ModuleLog(i, BLOG_ERROR, "wrong delimiter type");
            goto fail0;
        }
        arg_start = 1;
        
        del = NCDVal_StringMemRef(delimiter_arg);
        if (del.len == 0) {
            ModuleLog(i, BLOG_ERROR, "delimiter must be nonempty");
            goto fail0;
        }
        
        table = BAllocArray(del.len, sizeof(table[0]));
        if (!table) {
            ModuleLog(i, BLOG_ERROR, "ExpArray_init failed");
            goto fail0;
        }
        
        build_substring_backtrack_table(del, (size_t *)table);
    }
    
    // read arguments
    NCDValRef input_arg;
    NCDValRef limit_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListReadStart(params->args, arg_start, 1, &input_arg) && !NCDVal_ListReadStart(params->args, arg_start, 2, &input_arg, &limit_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail1;
    }
    if (!NCDVal_IsString(input_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail1;
    }
    o->input = input_arg;
    
    size_t limit = SIZE_MAX;
    if (!NCDVal_IsInvalid(limit_arg)) {
        uintmax_t n;
        if (!ncd_read_uintmax(limit_arg, &n) || n == 0) {
            ModuleLog(i, BLOG_ERROR, "bad limit argument");
            goto fail1;
        }
        n--;
        limit = (n <= SIZE_MAX ? n : SIZE_MAX);
    }
    
    if (!ExpArray_init(&o->arr, sizeof(MemRef), 8)) {
        ModuleLog(i, BLOG_ERROR, "ExpArray_init failed");
        goto fail1;
    }
    o->num = 0;
    
    MemRef data = NCDVal_StringMemRef(input_arg);
    
    while (1) {
        size_t start;
        int is_end = 0;
        if (limit == 0 || !find_substring(data, del, table, &start)) {
            start = data.len;
            is_end = 1;
        }
        
        if (!ExpArray_resize(&o->arr, o->num + 1)) {
            ModuleLog(i, BLOG_ERROR, "ExpArray_init failed");
            goto fail2;
        }
        
        ((MemRef *)o->arr.v)[o->num] = MemRef_SubTo(data, start);
        o->num++;
        
        if (is_end) {
            break;
        }
        
        data = MemRef_SubFrom(data, start + del.len);
        limit--;
    }
    
    if (!compiled) {
        BFree((size_t *)table);
    }
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;

fail2:
    free(o->arr.v);
fail1:
    if (!compiled) {
        BFree((size_t *)table);
    }
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    return func_new_common(vo, i, params, NULL);
}

static void func_new_compiled (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct compile_search_instance *compiled = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    return func_new_common(vo, i, params, compiled);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    free(o->arr.v);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        *out = NCDVal_NewList(mem, o->num);
        if (NCDVal_IsInvalid(*out)) {
            goto fail;
        }
        int is_external = NCDVal_IsExternalString(o->input);
        for (size_t j = 0; j < o->num; j++) {
            MemRef elem = ((MemRef *)o->arr.v)[j];
            NCDValRef str;
            if (is_external) {
                str = NCDVal_NewExternalString(mem, elem.ptr, elem.len, NCDVal_ExternalStringTarget(o->input));
            } else {
                str = NCDVal_NewStringBinMr(mem, elem);
            }
            if (NCDVal_IsInvalid(str)) {
                goto fail;
            }
            if (!NCDVal_ListAppend(*out, str)) {
                goto fail;
            }
        }
        return 1;
    }
    
    return 0;
    
fail:
    *out = NCDVal_NewInvalid();
    return 1;
}

static struct NCDModule modules[] = {
    {
        .type = "compile_search",
        .func_new2 = compile_search_new,
        .func_die = compile_search_die,
        .alloc_size = sizeof(struct compile_search_instance)
    }, {
        .type = "explode",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "compile_search::explode",
        .func_new2 = func_new_compiled,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_explode = {
    .modules = modules
};
