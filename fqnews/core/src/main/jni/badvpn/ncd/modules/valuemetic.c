/**
 * @file valuemetic.c
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
 * Comparison functions for values.
 * 
 * Synopsis:
 *   val_lesser(v1, v2)
 *   val_greater(v1, v2)
 *   val_lesser_equal(v1, v2)
 *   val_greater_equal(v1, v2)
 *   val_equal(v1, v2)
 *   val_different(v1, v2)
 * 
 * Variables:
 *   (empty) - "true" or "false", reflecting the value of the relation in question
 * 
 * Description:
 *   These statements perform comparisons of values. Order of values is defined by the
 *   following rules:
 *   1. Values of different types have the following order: strings, lists, maps.
 *   2. String values are ordered lexicographically, with respect to the numeric values
 *      of their bytes.
 *   3. List values are ordered lexicographically, where the order of the elements is
 *      defined by recursive application of these rules.
 *   4. Map values are ordered lexicographically, as if a map was a list of (key, value)
 *      pairs ordered by key, where the order of both keys and values is defined by
 *      recursive application of these rules.
 */

#include <stdlib.h>
#include <string.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_valuemetic.h>

struct instance {
    NCDModuleInst *i;
    int result;
};

typedef int (*compute_func) (NCDValRef v1, NCDValRef v2);

static int compute_lesser (NCDValRef v1, NCDValRef v2)
{
    return NCDVal_Compare(v1, v2) < 0;
}

static int compute_greater (NCDValRef v1, NCDValRef v2)
{
    return NCDVal_Compare(v1, v2) > 0;
}

static int compute_lesser_equal (NCDValRef v1, NCDValRef v2)
{
    return NCDVal_Compare(v1, v2) <= 0;
}

static int compute_greater_equal (NCDValRef v1, NCDValRef v2)
{
    return NCDVal_Compare(v1, v2) >= 0;
}

static int compute_equal (NCDValRef v1, NCDValRef v2)
{
    return NCDVal_Compare(v1, v2) == 0;
}

static int compute_different (NCDValRef v1, NCDValRef v2)
{
    return NCDVal_Compare(v1, v2) != 0;
}

static void new_templ (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, compute_func cfunc)
{
    struct instance *o = vo;
    o->i = i;
    
    NCDValRef v1_arg;
    NCDValRef v2_arg;
    if (!NCDVal_ListRead(params->args, 2, &v1_arg, &v2_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    o->result = cfunc(v1_arg, v2_arg);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        *out = ncd_make_boolean(mem, o->result);
        return 1;
    }
    
    return 0;
}

static void func_new_lesser (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, compute_lesser);
}

static void func_new_greater (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, compute_greater);
}

static void func_new_lesser_equal (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, compute_lesser_equal);
}

static void func_new_greater_equal (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, compute_greater_equal);
}

static void func_new_equal (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, compute_equal);
}

static void func_new_different (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, compute_different);
}

static struct NCDModule modules[] = {
    {
        .type = "val_lesser",
        .func_new2 = func_new_lesser,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "val_greater",
        .func_new2 = func_new_greater,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "val_lesser_equal",
        .func_new2 = func_new_lesser_equal,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "val_greater_equal",
        .func_new2 = func_new_greater_equal,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "val_equal",
        .func_new2 = func_new_equal,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "val_different",
        .func_new2 = func_new_different,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_valuemetic = {
    .modules = modules
};
