/**
 * @file arithmetic.c
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
 * Arithmetic functions for unsigned integers.
 * 
 * Synopsis:
 *   num_lesser(string n1, string n2)
 *   num_greater(string n1, string n2)
 *   num_lesser_equal(string n1, string n2)
 *   num_greater_equal(string n1, string n2)
 *   num_equal(string n1, string n2)
 *   num_different(string n1, string n2)
 * 
 * Variables:
 *   (empty) - "true" or "false", reflecting the value of the relation in question
 * 
 * Description:
 *   These statements perform arithmetic comparisons. The operands passed must be
 *   non-negative decimal integers representable in a uintmax_t. Otherwise, an error
 *   is triggered.
 * 
 * Synopsis:
 *   num_add(string n1, string n2)
 *   num_subtract(string n1, string n2)
 *   num_multiply(string n1, string n2)
 *   num_divide(string n1, string n2)
 *   num_modulo(string n1, string n2)
 * 
 * Description:
 *   These statements perform arithmetic operations. The operands passed must be
 *   non-negative decimal integers representable in a uintmax_t, and the result must
 *   also be representable and non-negative. For divide and modulo, n2 must be non-zero.
 *   If any of these restrictions is violated, an error is triggered.
 * 
 * Variables:
 *   is_error - whether there was an arithmetic error with the operation (true/false).
 *   (empty) - the result of the operation as a string representing a decimal number.
 *             If an attempt is made to access this variable after an arithmetic error,
 *             the variable resolution will fail, and an error will be logged including
 *             information about the particular arithemtic error.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>

#include <misc/parse_number.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_arithmetic.h>

struct boolean_instance {
    NCDModuleInst *i;
    int value;
};

typedef int (*boolean_compute_func) (uintmax_t n1, uintmax_t n2);

struct number_instance {
    NCDModuleInst *i;
    char const *error;
    uintmax_t value;
};

typedef char const * (*number_compute_func) (NCDModuleInst *i, uintmax_t n1, uintmax_t n2, uintmax_t *out);

static int compute_lesser (uintmax_t n1, uintmax_t n2)
{
    return n1 < n2;
}

static int compute_greater (uintmax_t n1, uintmax_t n2)
{
    return n1 > n2;
}

static int compute_lesser_equal (uintmax_t n1, uintmax_t n2)
{
    return n1 <= n2;
}

static int compute_greater_equal (uintmax_t n1, uintmax_t n2)
{
    return n1 >= n2;
}

static int compute_equal (uintmax_t n1, uintmax_t n2)
{
    return n1 == n2;
}

static int compute_different (uintmax_t n1, uintmax_t n2)
{
    return n1 != n2;
}

static char const * compute_add (NCDModuleInst *i, uintmax_t n1, uintmax_t n2, uintmax_t *out)
{
    if (n1 > UINTMAX_MAX - n2) {
        return "addition overflow";
    }
    *out = n1 + n2;
    return NULL;
}

static char const * compute_subtract (NCDModuleInst *i, uintmax_t n1, uintmax_t n2, uintmax_t *out)
{
    if (n1 < n2) {
        return "subtraction underflow";
    }
    *out = n1 - n2;
    return NULL;
}

static char const * compute_multiply (NCDModuleInst *i, uintmax_t n1, uintmax_t n2, uintmax_t *out)
{
    if (n2 != 0 && n1 > UINTMAX_MAX / n2) {
        return "multiplication overflow";
    }
    *out = n1 * n2;
    return NULL;
}

static char const * compute_divide (NCDModuleInst *i, uintmax_t n1, uintmax_t n2, uintmax_t *out)
{
    if (n2 == 0) {
        return "division quotient is zero";
    }
    *out = n1 / n2;
    return NULL;
}

static char const * compute_modulo (NCDModuleInst *i, uintmax_t n1, uintmax_t n2, uintmax_t *out)
{
    if (n2 == 0) {
        return "modulo modulus is zero";
    }
    *out = n1 % n2;
    return NULL;
}

static void new_boolean_templ (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, boolean_compute_func cfunc)
{
    struct boolean_instance *o = vo;
    o->i = i;
    
    NCDValRef n1_arg;
    NCDValRef n2_arg;
    if (!NCDVal_ListRead(params->args, 2, &n1_arg, &n2_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    uintmax_t n1;
    if (!ncd_read_uintmax(n1_arg, &n1)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong first argument");
        goto fail0;
    }
    
    uintmax_t n2;
    if (!ncd_read_uintmax(n2_arg, &n2)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong second argument");
        goto fail0;
    }
    
    o->value = cfunc(n1, n2);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static int boolean_func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct boolean_instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        *out = ncd_make_boolean(mem, o->value);
        return 1;
    }
    
    return 0;
}

static void new_number_templ (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, number_compute_func cfunc)
{
    struct number_instance *o = vo;
    o->i = i;
    
    NCDValRef n1_arg;
    NCDValRef n2_arg;
    if (!NCDVal_ListRead(params->args, 2, &n1_arg, &n2_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    uintmax_t n1;
    if (!ncd_read_uintmax(n1_arg, &n1)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong first argument");
        goto fail0;
    }
    
    uintmax_t n2;
    if (!ncd_read_uintmax(n2_arg, &n2)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong second argument");
        goto fail0;
    }
    
    o->error = cfunc(i, n1, n2, &o->value);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static int number_func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct number_instance *o = vo;
    
    if (name == NCD_STRING_IS_ERROR) {
        *out = ncd_make_boolean(mem, !!o->error);
        return 1;
    }
    
    if (name == NCD_STRING_EMPTY) {
        if (o->error) {
            ModuleLog(o->i, BLOG_ERROR, "%s", o->error);
            return 0;
        }
        *out = ncd_make_uintmax(mem, o->value);
        return 1;
    }
    
    return 0;
}

static void func_new_lesser (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_boolean_templ(vo, i, params, compute_lesser);
}

static void func_new_greater (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_boolean_templ(vo, i, params, compute_greater);
}

static void func_new_lesser_equal (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_boolean_templ(vo, i, params, compute_lesser_equal);
}

static void func_new_greater_equal (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_boolean_templ(vo, i, params, compute_greater_equal);
}

static void func_new_equal (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_boolean_templ(vo, i, params, compute_equal);
}

static void func_new_different (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_boolean_templ(vo, i, params, compute_different);
}

static void func_new_add (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_number_templ(vo, i, params, compute_add);
}

static void func_new_subtract (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_number_templ(vo, i, params, compute_subtract);
}

static void func_new_multiply (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_number_templ(vo, i, params, compute_multiply);
}

static void func_new_divide (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_number_templ(vo, i, params, compute_divide);
}

static void func_new_modulo (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_number_templ(vo, i, params, compute_modulo);
}

static struct NCDModule modules[] = {
    {
        .type = "num_lesser",
        .func_new2 = func_new_lesser,
        .func_getvar2 = boolean_func_getvar2,
        .alloc_size = sizeof(struct boolean_instance)
    }, {
        .type = "num_greater",
        .func_new2 = func_new_greater,
        .func_getvar2 = boolean_func_getvar2,
        .alloc_size = sizeof(struct boolean_instance)
    }, {
        .type = "num_lesser_equal",
        .func_new2 = func_new_lesser_equal,
        .func_getvar2 = boolean_func_getvar2,
        .alloc_size = sizeof(struct boolean_instance)
    }, {
        .type = "num_greater_equal",
        .func_new2 = func_new_greater_equal,
        .func_getvar2 = boolean_func_getvar2,
        .alloc_size = sizeof(struct boolean_instance)
    }, {
        .type = "num_equal",
        .func_new2 = func_new_equal,
        .func_getvar2 = boolean_func_getvar2,
        .alloc_size = sizeof(struct boolean_instance)
    }, {
        .type = "num_different",
        .func_new2 = func_new_different,
        .func_getvar2 = boolean_func_getvar2,
        .alloc_size = sizeof(struct boolean_instance)
    }, {
        .type = "num_add",
        .func_new2 = func_new_add,
        .func_getvar2 = number_func_getvar2,
        .alloc_size = sizeof(struct number_instance)
    }, {
        .type = "num_subtract",
        .func_new2 = func_new_subtract,
        .func_getvar2 = number_func_getvar2,
        .alloc_size = sizeof(struct number_instance)
    }, {
        .type = "num_multiply",
        .func_new2 = func_new_multiply,
        .func_getvar2 = number_func_getvar2,
        .alloc_size = sizeof(struct number_instance)
    }, {
        .type = "num_divide",
        .func_new2 = func_new_divide,
        .func_getvar2 = number_func_getvar2,
        .alloc_size = sizeof(struct number_instance)
    }, {
        .type = "num_modulo",
        .func_new2 = func_new_modulo,
        .func_getvar2 = number_func_getvar2,
        .alloc_size = sizeof(struct number_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_arithmetic = {
    .modules = modules
};
