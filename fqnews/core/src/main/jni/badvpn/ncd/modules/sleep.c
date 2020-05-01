/**
 * @file sleep.c
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
 *   sleep(string ms_start [, string ms_stop])
 * 
 * Description:
 *   On init, sleeps 'ms_start' milliseconds then goes up, or goes up immediately
 *   if 'ms_start' is an empty string.
 *   On deinit, sleeps 'ms_stop' milliseconds then dies, or dies immediately if
 *   'ms_stop' is an empty string or is not provided. If a deinit is requested while
 *   the init sleep is still in progress, the init sleep is aborted and the deinit
 *   sleep is started immediately (if any).
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_sleep.h>

struct instance {
    NCDModuleInst *i;
    btime_t ms_stop;
    BTimer timer;
    int dying;
};

static void instance_free (struct instance *o);

static void timer_handler (void *vo)
{
    struct instance *o = vo;
    
    if (!o->dying) {
        // signal up
        NCDModuleInst_Backend_Up(o->i);
    } else {
        // die
        instance_free(o);
    }
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef ms_start_arg;
    NCDValRef ms_stop_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 1, &ms_start_arg) &&
        !NCDVal_ListRead(params->args, 2, &ms_start_arg, &ms_stop_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    uintmax_t ms;
    btime_t ms_start;
    
    if (NCDVal_IsString(ms_start_arg) && NCDVal_StringEqualsId(ms_start_arg, NCD_STRING_EMPTY)) {
        ms_start = -1;
    } else {
        if (!ncd_read_uintmax(ms_start_arg, &ms) || ms > INT64_MAX) {
            ModuleLog(o->i, BLOG_ERROR, "wrong start time");
            goto fail0;
        }
        ms_start = ms;
    }
    
    if (NCDVal_IsInvalid(ms_stop_arg) || (NCDVal_IsString(ms_stop_arg) && NCDVal_StringEqualsId(ms_stop_arg, NCD_STRING_EMPTY))) {
        o->ms_stop = -1;
    } else {
        if (!ncd_read_uintmax(ms_stop_arg, &ms) || ms > INT64_MAX) {
            ModuleLog(o->i, BLOG_ERROR, "wrong stop time");
            goto fail0;
        }
        o->ms_stop = ms;
    }
    
    // init timer
    BTimer_Init(&o->timer, 0, timer_handler, o);
    
    // set not dying
    o->dying = 0;
    
    if (ms_start < 0) {
        // go up
        NCDModuleInst_Backend_Up(i);
    } else {
        // set timer
        BReactor_SetTimerAfter(o->i->params->iparams->reactor, &o->timer, ms_start);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

void instance_free (struct instance *o)
{
    // free timer
    BReactor_RemoveTimer(o->i->params->iparams->reactor, &o->timer);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    if (o->ms_stop < 0) {
        // die immediately
        instance_free(o);
        return;
    }
    
    // set dying
    o->dying = 1;
    
    // set timer
    BReactor_SetTimerAfter(o->i->params->iparams->reactor, &o->timer, o->ms_stop);
}

static struct NCDModule modules[] = {
    {
        .type = "sleep",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_sleep = {
    .modules = modules
};
