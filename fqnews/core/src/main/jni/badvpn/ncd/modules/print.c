/**
 * @file print.c
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
 * Modules for printing to standard output.
 * 
 * Synopsis:
 *   print([string str ...])
 * Description:
 *   On initialization, prints strings to standard output.
 * 
 * Synopsis:
 *   println([string str ...])
 * Description:
 *   On initialization, prints strings to standard output, and a newline.
 * 
 * Synopsis:
 *   rprint([string str ...])
 * Description:
 *   On deinitialization, prints strings to standard output.
 * 
 * Synopsis:
 *   rprintln([string str ...])
 * Description:
 *   On deinitialization, prints strings to standard output, and a newline.
 */

#include <stdlib.h>
#include <stdio.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_print.h>

struct rprint_instance {
    NCDModuleInst *i;
    NCDValRef args;
    int ln;
};

static int check_args (NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    size_t num_args = NCDVal_ListCount(params->args);
    
    for (size_t j = 0; j < num_args; j++) {
        NCDValRef arg = NCDVal_ListGet(params->args, j);
        if (!NCDVal_IsString(arg)) {
            ModuleLog(i, BLOG_ERROR, "wrong type");
            return 0;
        }
    }
    
    return 1;
}

static void do_print (NCDModuleInst *i, NCDValRef args, int ln)
{
    size_t num_args = NCDVal_ListCount(args);
    
    for (size_t j = 0; j < num_args; j++) {
        NCDValRef arg = NCDVal_ListGet(args, j);
        ASSERT(NCDVal_IsString(arg))
        
        MemRef arg_mr = NCDVal_StringMemRef(arg);
        
        size_t pos = 0;
        while (pos < arg_mr.len) {
            ssize_t res = fwrite(arg_mr.ptr + pos, 1, arg_mr.len - pos, stdout);
            if (res <= 0) {
                goto out;
            }
            pos += res;
        }
    }
    
out:
    if (ln) {
        printf("\n");
    }
}

static void rprint_func_new_common (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int ln)
{
    struct rprint_instance *o = vo;
    o->i = i;
    o->args = params->args;
    o->ln = ln;
    
    if (!check_args(i, params)) {
        goto fail0;
    }
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void rprint_func_die (void *vo)
{
    struct rprint_instance *o = vo;
    
    do_print(o->i, o->args, o->ln);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void print_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    if (!check_args(i, params)) {
        goto fail0;
    }
    
    do_print(i, params->args, 0);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void println_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    if (!check_args(i, params)) {
        goto fail0;
    }
    
    do_print(i, params->args, 1);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void rprint_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    return rprint_func_new_common(vo, i, params, 0);
}

static void rprintln_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    return rprint_func_new_common(vo, i, params, 1);
}

static struct NCDModule modules[] = {
    {
        .type = "print",
        .func_new2 = print_func_new
    }, {
        .type = "println",
        .func_new2 = println_func_new
    }, {
        .type = "rprint",
        .func_new2 = rprint_func_new,
        .func_die = rprint_func_die,
        .alloc_size = sizeof(struct rprint_instance)
     }, {
        .type = "rprintln",
        .func_new2 = rprintln_func_new,
        .func_die = rprint_func_die,
        .alloc_size = sizeof(struct rprint_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_print = {
    .modules = modules
};
