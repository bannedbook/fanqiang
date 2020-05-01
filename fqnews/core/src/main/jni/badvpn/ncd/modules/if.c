/**
 * @file if.c
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
 * Conditional module.
 * 
 * Synopsis: if(string cond)
 * Description: on initialization, transitions to UP state if cond equals "true", else
 *      remains in the DOWN state indefinitely.
 * 
 * Synopsis: ifnot(string cond)
 * Description: on initialization, transitions to UP state if cond does not equal "true", else
 *      remains in the DOWN state indefinitely.
 */

#include <stdlib.h>
#include <string.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_if.h>

static void new_templ (NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int is_not)
{
    // check arguments
    NCDValRef arg;
    if (!NCDVal_ListRead(params->args, 1, &arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    int arg_val;
    if (!ncd_read_boolean(arg, &arg_val)) {
        ModuleLog(i, BLOG_ERROR, "bad argument");
        goto fail0;
    }
    
    // compute logical value of argument
    int c = arg_val;
    
    // signal up if needed
    if ((is_not && !c) || (!is_not && c)) {
        NCDModuleInst_Backend_Up(i);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(i, params, 0);
}

static void func_new_not (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(i, params, 1);
}

static struct NCDModule modules[] = {
    {
        .type = "if",
        .func_new2 = func_new
    }, {
        .type = "ifnot",
        .func_new2 = func_new_not
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_if = {
    .modules = modules
};
