/**
 * @file net_backend_badvpn.c
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
 * BadVPN interface module.
 * 
 * Synopsis: net.backend.badvpn(string ifname, string user, string exec, list(string) args)
 */

#include <stdlib.h>
#include <string.h>

#include <misc/cmdline.h>
#include <ncd/extra/NCDIfConfig.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_backend_badvpn.h>

#define RETRY_TIME 5000

struct instance {
    NCDModuleInst *i;
    NCDValNullTermString ifname_nts;
    NCDValNullTermString user_nts;
    MemRef exec;
    NCDValRef args;
    int dying;
    int started;
    BTimer timer;
    BProcess process;
};

static void try_process (struct instance *o);
static void process_handler (struct instance *o, int normally, uint8_t normally_exit_status);
static void timer_handler (struct instance *o);
static void instance_free (struct instance *o);

void try_process (struct instance *o)
{
    CmdLine c;
    if (!CmdLine_Init(&c)) {
        goto fail0;
    }
    
    // append exec
    if (!CmdLine_AppendNoNullMr(&c, o->exec)) {
        goto fail1;
    }
    
    // append tapdev
    if (!CmdLine_Append(&c, "--tapdev") || !CmdLine_Append(&c, o->ifname_nts.data)) {
        goto fail1;
    }
    
    // append arguments
    size_t count = NCDVal_ListCount(o->args);
    for (size_t j = 0; j < count; j++) {
        NCDValRef arg = NCDVal_ListGet(o->args, j);
        if (!CmdLine_AppendNoNullMr(&c, NCDVal_StringMemRef(arg))) {
            goto fail1;
        }
    }
    
    // terminate cmdline
    if (!CmdLine_Finish(&c)) {
        goto fail1;
    }
    
    // start process
    if (!BProcess_Init(&o->process, o->i->params->iparams->manager, (BProcess_handler)process_handler, o, ((char **)c.arr.v)[0], (char **)c.arr.v, o->user_nts.data)) {
        ModuleLog(o->i, BLOG_ERROR, "BProcess_Init failed");
        goto fail1;
    }
    
    CmdLine_Free(&c);
    
    // set started
    o->started = 1;
    
    return;
    
fail1:
    CmdLine_Free(&c);
fail0:
    // retry
    o->started = 0;
    BReactor_SetTimer(o->i->params->iparams->reactor, &o->timer);
}

void process_handler (struct instance *o, int normally, uint8_t normally_exit_status)
{
    ASSERT(o->started)
    
    ModuleLog(o->i, BLOG_INFO, "process terminated");
    
    // free process
    BProcess_Free(&o->process);
    
    // set not started
    o->started = 0;
    
    if (o->dying) {
        instance_free(o);
        return;
    }
    
    // set timer
    BReactor_SetTimer(o->i->params->iparams->reactor, &o->timer);
}

void timer_handler (struct instance *o)
{
    ASSERT(!o->started)
    
    ModuleLog(o->i, BLOG_INFO, "retrying");
    
    // try starting process again
    try_process(o);
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef ifname_arg;
    NCDValRef user_arg;
    NCDValRef exec_arg;
    NCDValRef args_arg;
    if (!NCDVal_ListRead(params->args, 4, &ifname_arg, &user_arg, &exec_arg, &args_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(ifname_arg) || !NCDVal_IsStringNoNulls(user_arg) ||
        !NCDVal_IsStringNoNulls(exec_arg) || !NCDVal_IsList(args_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    o->exec = NCDVal_StringMemRef(exec_arg);
    o->args = args_arg;
    
    // check arguments
    size_t count = NCDVal_ListCount(o->args);
    for (size_t j = 0; j < count; j++) {
        NCDValRef arg = NCDVal_ListGet(o->args, j);
        if (!NCDVal_IsStringNoNulls(arg)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong type");
            goto fail0;
        }
    }
    
    // null terminate user
    if (!NCDVal_StringNullTerminate(user_arg, &o->user_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    // null terminate ifname
    if (!NCDVal_StringNullTerminate(ifname_arg, &o->ifname_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail1;
    }
    
    // create TAP device
    if (!NCDIfConfig_make_tuntap(o->ifname_nts.data, o->user_nts.data, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "failed to create TAP device");
        goto fail2;
    }
    
    // set device up
    if (!NCDIfConfig_set_up(o->ifname_nts.data)) {
        ModuleLog(o->i, BLOG_ERROR, "failed to set device up");
        goto fail3;
    }
    
    // set not dying
    o->dying = 0;
    
    // init timer
    BTimer_Init(&o->timer, RETRY_TIME, (BTimer_handler)timer_handler, o);
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    
    // try starting process
    try_process(o);
    return;
    
fail3:
    if (!NCDIfConfig_remove_tuntap(o->ifname_nts.data, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "failed to remove TAP device");
    }
fail2:
    NCDValNullTermString_Free(&o->ifname_nts);
fail1:
    NCDValNullTermString_Free(&o->user_nts);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

void instance_free (struct instance *o)
{
    ASSERT(!o->started)
    
    // free timer
    BReactor_RemoveTimer(o->i->params->iparams->reactor, &o->timer);
    
    // set device down
    if (!NCDIfConfig_set_down(o->ifname_nts.data)) {
        ModuleLog(o->i, BLOG_ERROR, "failed to set device down");
    }
    
    // free TAP device
    if (!NCDIfConfig_remove_tuntap(o->ifname_nts.data, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "failed to remove TAP device");
    }
    
    // free ifname nts
    NCDValNullTermString_Free(&o->ifname_nts);
    
    // free user nts
    NCDValNullTermString_Free(&o->user_nts);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    ASSERT(!o->dying)
    
    if (!o->started) {
        instance_free(o);
        return;
    }
    
    // request termination
    BProcess_Terminate(&o->process);
    
    // remember dying
    o->dying = 1;
}

static struct NCDModule modules[] = {
    {
        .type = "net.backend.badvpn",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_backend_badvpn = {
    .modules = modules
};
