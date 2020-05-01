/**
 * @file assert.c
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
 *   assert(string cond)
 *   assert_false(string cond)
 * 
 * Description:
 *   If 'cond' is equal to the string "true" (assert) or "false" (assert_false),
 *   does nothing. Otherwise, logs an error and initiates interpreter termination
 *   with exit code 1, i.e. it is equivalent to calling exit("1").
 *   Note that "assert_false(cond);" is not completely equivalent to
 *   "not(cond) a; assert(a);", in case 'cond'  is something other than "true"
 *   or "false".
 */

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_assert.h>

static void func_new_common (NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int is_false)
{
    // check arguments
    NCDValRef cond_arg;
    if (!NCDVal_ListRead(params->args, 1, &cond_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(cond_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    
    // if failed, initiate exit (before up!)
    if ((!is_false && !NCDVal_StringEqualsId(cond_arg, NCD_STRING_TRUE)) ||
        (is_false && !NCDVal_StringEqualsId(cond_arg, NCD_STRING_FALSE))
    ) {
        ModuleLog(i, BLOG_ERROR, "assertion failed");
        NCDModuleInst_Backend_InterpExit(i, 1);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_common(i, params, 0);
}

static void func_new_false (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_common(i, params, 1);
}

static struct NCDModule modules[] = {
    {
        .type = "assert",
        .func_new2 = func_new
    }, {
        .type = "assert_false",
        .func_new2 = func_new_false
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_assert = {
    .modules = modules
};
