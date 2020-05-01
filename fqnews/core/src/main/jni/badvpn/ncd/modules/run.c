/**
 * @file run.c
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
 * Module for running arbitrary programs.
 * NOTE: There is no locking - the program may run in parallel with other
 * NCD processes and their programs.
 * 
 * Synopsis: run(list do_cmd, list undo_cmd)
 * Arguments:
 *   list do_cmd - Command run on startup. The first element is the full path
 *     to the executable, other elements are command line arguments (excluding
 *     the zeroth argument). An empty list is interpreted as no operation.
 *   list undo_cmd - Command run on shutdown, like do_cmd.
 */

#include <stdlib.h>
#include <string.h>

#include <misc/strdup.h>
#include <ncd/extra/value_utils.h>
#include <ncd/modules/command_template.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_run.h>

static void template_free_func (void *vo, int is_error);

struct instance {
    NCDModuleInst *i;
    BEventLock lock;
    command_template_instance cti;
};

static int build_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    // read arguments
    NCDValRef do_cmd_arg;
    NCDValRef undo_cmd_arg;
    if (!NCDVal_ListRead(args, 2, &do_cmd_arg, &undo_cmd_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsList(do_cmd_arg) || !NCDVal_IsList(undo_cmd_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    NCDValRef list = (remove ? undo_cmd_arg : do_cmd_arg);
    size_t count = NCDVal_ListCount(list);
    
    // check if there is no command
    if (count == 0) {
        *exec = NULL;
        return 1;
    }
    
    // read exec
    NCDValRef exec_arg = NCDVal_ListGet(list, 0);
    if (!NCDVal_IsStringNoNulls(exec_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    if (!(*exec = ncd_strdup(exec_arg))) {
        ModuleLog(i, BLOG_ERROR, "ncd_strdup failed");
        goto fail0;
    }
    
    // start cmdline
    if (!CmdLine_Init(cl)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Init failed");
        goto fail1;
    }
    
    // add header
    if (!CmdLine_Append(cl, *exec)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Append failed");
        goto fail2;
    }
    
    // add additional arguments
    for (size_t j = 1; j < count; j++) {
        NCDValRef arg = NCDVal_ListGet(list, j);
        
        if (!NCDVal_IsStringNoNulls(arg)) {
            ModuleLog(i, BLOG_ERROR, "wrong type");
            goto fail2;
        }
        
        if (!CmdLine_AppendNoNullMr(cl, NCDVal_StringMemRef(arg))) {
            ModuleLog(i, BLOG_ERROR, "CmdLine_AppendNoNull failed");
            goto fail2;
        }
    }
    
    // finish
    if (!CmdLine_Finish(cl)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Finish failed");
        goto fail2;
    }
    
    return 1;
    
fail2:
    CmdLine_Free(cl);
fail1:
    free(*exec);
fail0:
    return 0;
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // init dummy event lock
    BEventLock_Init(&o->lock, BReactor_PendingGroup(i->params->iparams->reactor));
    
    command_template_new(&o->cti, i, params, build_cmdline, template_free_func, o, BLOG_CURRENT_CHANNEL, &o->lock);
    return;
}

void template_free_func (void *vo, int is_error)
{
    struct instance *o = vo;
    
    // free dummy event lock
    BEventLock_Free(&o->lock);
    
    if (is_error) {
        NCDModuleInst_Backend_DeadError(o->i);
    } else {
        NCDModuleInst_Backend_Dead(o->i);
    }
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    command_template_die(&o->cti);
}

static struct NCDModule modules[] = {
    {
        .type = "run",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_run= {
    .modules = modules
};
