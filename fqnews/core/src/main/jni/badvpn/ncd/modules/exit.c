/**
 * @file exit.c
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
 *   exit(string exit_code)
 * 
 * Description:
 *   Initiates termination of the interpreter. Calling exit() multiple times will override
 *   the previously set exit code. When the interpreter receives a signal, it reacts
 *   equivalently to calling exit(1) (possibly overriding your error code).
 */

#include <limits.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_exit.h>

static void func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    NCDValRef exit_code_arg;
    if (!NCDVal_ListRead(params->args, 1, &exit_code_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // parse exit code
    uintmax_t exit_code;
    if (!ncd_read_uintmax(exit_code_arg, &exit_code) || exit_code >= INT_MAX) {
        ModuleLog(i, BLOG_ERROR, "wrong exit code value");
        goto fail0;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    
    // initiate exit (before up!)
    NCDModuleInst_Backend_InterpExit(i, exit_code);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "exit",
        .func_new2 = func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_exit = {
    .modules = modules
};
