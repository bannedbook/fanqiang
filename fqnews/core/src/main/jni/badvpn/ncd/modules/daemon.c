/**
 * @file daemon.c
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
 * Runs a program in the background, restarting it if it crashes.
 * On deinitialization, sends SIGTERM to the daemon and waits for it to terminate
 * (unless it's crashed at the time).
 * 
 * Synopsis:
 *   daemon(list(string) cmd [, map options])
 * 
 * Arguments:
 *   cmd - Command for the daemon. The first element is the full path
 *     to the executable, other elements are command line arguments (excluding
 *     the zeroth argument).
 *   options - Map of options:
 *     "keep_stdout":"true" - Start the program with the same stdout as the NCD process.
 *     "keep_stderr":"true" - Start the program with the same stderr as the NCD process.
 *     "do_setsid":"true" - Call setsid() in the child before exec. This is needed to
 *       start the 'agetty' program.
 *     "username":username_string - Start the process under the permissions of the
 *       specified user. 
 *     "retry_time":milliseconds - Wait for this long after the daemon exits
 *       unexpectedtly before starting it again.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <misc/cmdline.h>
#include <misc/strdup.h>
#include <system/BProcess.h>
#include <ncd/extra/NCDBProcessOpts.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_daemon.h>

#define DEFAULT_RETRY_TIME 10000

#define STATE_RETRYING 1
#define STATE_RUNNING 2
#define STATE_RUNNING_DIE 3

struct instance {
    NCDModuleInst *i;
    NCDValRef cmd_arg;
    btime_t retry_time;
    NCDBProcessOpts opts;
    BTimer timer;
    BProcess process;
    int state;
};

static int build_cmdline (NCDModuleInst *i, NCDValRef cmd_arg, char **exec, CmdLine *cl);
static void start_process (struct instance *o);
static void timer_handler (struct instance *o);
static void process_handler (struct instance *o, int normally, uint8_t normally_exit_status);
static void instance_free (struct instance *o);

static int build_cmdline (NCDModuleInst *i, NCDValRef cmd_arg, char **exec, CmdLine *cl)
{
    ASSERT(!NCDVal_IsInvalid(cmd_arg))
    
    if (!NCDVal_IsList(cmd_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    size_t count = NCDVal_ListCount(cmd_arg);
    
    // read exec
    if (count == 0) {
        ModuleLog(i, BLOG_ERROR, "missing executable name");
        goto fail0;
    }
    NCDValRef exec_arg = NCDVal_ListGet(cmd_arg, 0);
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
        NCDValRef arg = NCDVal_ListGet(cmd_arg, j);
        
        if (!NCDVal_IsStringNoNulls(arg)) {
            ModuleLog(i, BLOG_ERROR, "wrong type");
            goto fail2;
        }
        
        if (!CmdLine_AppendNoNullMr(cl, NCDVal_StringMemRef(arg))) {
            ModuleLog(i, BLOG_ERROR, "CmdLine_AppendNoNullMr failed");
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

static void start_process (struct instance *o)
{
    // build cmdline
    char *exec;
    CmdLine cl;
    if (!build_cmdline(o->i, o->cmd_arg, &exec, &cl)) {
        goto fail;
    }
    
    // start process
    struct BProcess_params p_params = NCDBProcessOpts_GetParams(&o->opts);
    int res = BProcess_Init2(&o->process, o->i->params->iparams->manager, (BProcess_handler)process_handler, o, exec, CmdLine_Get(&cl), p_params);
    CmdLine_Free(&cl);
    free(exec);
    
    if (!res) {
        ModuleLog(o->i, BLOG_ERROR, "BProcess_Init2 failed");
        goto fail;
    }
    
    // set state running
    o->state = STATE_RUNNING;
    return;
    
fail:
    // start timer
    BReactor_SetTimer(o->i->params->iparams->reactor, &o->timer);
    
    // set state retrying
    o->state = STATE_RETRYING;
}

static void timer_handler (struct instance *o)
{
    ASSERT(o->state == STATE_RETRYING)
    
    ModuleLog(o->i, BLOG_INFO, "restarting after crash");
    
    start_process(o);
}

static void process_handler (struct instance *o, int normally, uint8_t normally_exit_status)
{
    ASSERT(o->state == STATE_RUNNING || o->state == STATE_RUNNING_DIE)
    
    // free process
    BProcess_Free(&o->process);
    
    // if we were requested to die, die now
    if (o->state == STATE_RUNNING_DIE) {
        instance_free(o);
        return;
    }
    
    ModuleLog(o->i, BLOG_ERROR, "daemon crashed");
    
    // start timer
    BReactor_SetTimer(o->i->params->iparams->reactor, &o->timer);
    
    // set state retrying
    o->state = STATE_RETRYING;
}

static int opts_func_unknown (void *user, NCDValRef key, NCDValRef val)
{
    struct instance *o = user;
    
    if (NCDVal_IsString(key) && NCDVal_StringEquals(key, "retry_time")) {
        if (!ncd_read_time(val, &o->retry_time)) {
            ModuleLog(o->i, BLOG_ERROR, "retry_time: bad value");
            return 0;
        }
        return 1;
    }
    
    return 0;
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef opts_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 1, &o->cmd_arg) &&
        !NCDVal_ListRead(params->args, 2, &o->cmd_arg, &opts_arg)
    ) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // init options
    o->retry_time = DEFAULT_RETRY_TIME;
    if (!NCDBProcessOpts_Init(&o->opts, opts_arg, opts_func_unknown, o, i, BLOG_CURRENT_CHANNEL)) {
        goto fail0;
    }
    
    // init timer
    BTimer_Init(&o->timer, o->retry_time, (BTimer_handler)timer_handler, o);
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    
    // try starting process
    start_process(o);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void instance_free (struct instance *o)
{
    // free timer
    BReactor_RemoveTimer(o->i->params->iparams->reactor, &o->timer);
    
    // free options
    NCDBProcessOpts_Free(&o->opts);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    ASSERT(o->state != STATE_RUNNING_DIE)
    
    // if not running, die immediately
    if (o->state == STATE_RETRYING) {
        instance_free(o);
        return;
    }
    
    // request termination
    BProcess_Terminate(&o->process);
    
    // set state running die
    o->state = STATE_RUNNING_DIE;
}

static struct NCDModule modules[] = {
    {
        .type = "daemon",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_daemon = {
    .modules = modules
};
