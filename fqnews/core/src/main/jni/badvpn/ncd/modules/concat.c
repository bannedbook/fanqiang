/**
 * @file concat.c
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
 *   concat([string elem ...])
 *   concatv(list strings)
 * 
 * Description:
 *   Concatenates zero or more strings. The result is available as the empty
 *   string variable. For concatv(), the strings are provided as a single
 *   list argument. For concat(), the strings are provided as arguments
 *   themselves.
 */

#include <misc/bsize.h>
#include <ncd/extra/NCDRefString.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_concat.h>

struct instance {
    NCDModuleInst *i;
    size_t length;
    NCDRefString *refstr;
};

static void new_concat_common (void *vo, NCDModuleInst *i, NCDValRef list)
{
    ASSERT(NCDVal_IsList(list))
    struct instance *o = vo;
    o->i = i;
    
    size_t count = NCDVal_ListCount(list);
    bsize_t result_size = bsize_fromsize(0);
    
    // check arguments and compute result size
    for (size_t j = 0; j < count; j++) {
        NCDValRef arg = NCDVal_ListGet(list, j);
        if (!NCDVal_IsString(arg)) {
            ModuleLog(i, BLOG_ERROR, "wrong type");
            goto fail0;
        }
        result_size = bsize_add(result_size, bsize_fromsize(NCDVal_StringLength(arg)));
    }
    if (result_size.is_overflow) {
        ModuleLog(i, BLOG_ERROR, "size overflow");
        goto fail0;
    }
    
    // allocate result
    char *result_data;
    o->refstr = NCDRefString_New(result_size.value, &result_data);
    if (!o->refstr) {
        ModuleLog(i, BLOG_ERROR, "NCDRefString_New failed");
        goto fail0;
    }
    
    // copy data to result
    o->length = 0;
    for (size_t j = 0; j < count; j++) {
        NCDValRef arg = NCDVal_ListGet(list, j);
        MemRef mr = NCDVal_StringMemRef(arg);
        MemRef_CopyOut(mr, result_data + o->length);
        o->length += mr.len;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_concat (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_concat_common(vo, i, params->args);
}

static void func_new_concatv (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef list_arg;
    if (!NCDVal_ListRead(params->args, 1, &list_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsList(list_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    new_concat_common(vo, i, list_arg);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    // release result reference
    BRefTarget_Deref(NCDRefString_RefTarget(o->refstr));
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        *out = NCDVal_NewExternalString(mem, NCDRefString_GetBuf(o->refstr), o->length, NCDRefString_RefTarget(o->refstr));
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "concat",
        .func_new2 = func_new_concat,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "concatv",
        .func_new2 = func_new_concatv,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_concat = {
    .modules = modules
};
