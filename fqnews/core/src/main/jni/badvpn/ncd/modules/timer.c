/**
 * @file timer.c
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
 *   periodic_timer(string down_time, string up_time)
 * 
 * Description:
 *   Periodically goes up and down. Starting in the down state, waits 'down_time'
 *   milliseconds. Then it goes up, and after 'up_time' milliseconds, goes back down,
 *   and the process repeats. When requested to terminate, terminates immedietely.
 */

#include <stddef.h>

#include <misc/debug.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_timer.h>

#define STATE_DOWN 1
#define STATE_UP 2

struct instance {
    NCDModuleInst *i;
    btime_t down_time;
    btime_t up_time;
    BTimer timer;
    int state;
};

static void timer_handler (void *vo)
{
    struct instance *o = vo;
    
    switch (o->state) {
        case STATE_DOWN: {
            o->state = STATE_UP;
            BReactor_SetTimerAfter(o->i->params->iparams->reactor, &o->timer, o->up_time);
            NCDModuleInst_Backend_Up(o->i);
        } break;
        
        case STATE_UP: {
            o->state = STATE_DOWN;
            BReactor_SetTimerAfter(o->i->params->iparams->reactor, &o->timer, o->down_time);
            NCDModuleInst_Backend_Down(o->i);
        } break;
        
        default: ASSERT(0);
    }
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef down_time_arg;
    NCDValRef up_time_arg;
    if (!NCDVal_ListRead(params->args, 2, &down_time_arg, &up_time_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // read times
    if (!ncd_read_time(down_time_arg, &o->down_time)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong down time");
        goto fail0;
    }
    if (!ncd_read_time(up_time_arg, &o->up_time)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong up time");
        goto fail0;
    }
    
    // init timer
    BTimer_Init(&o->timer, 0, timer_handler, o);
    
    // set state down
    o->state = STATE_DOWN;
    
    // set timer
    BReactor_SetTimerAfter(o->i->params->iparams->reactor, &o->timer, o->down_time);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    // free timer
    BReactor_RemoveTimer(o->i->params->iparams->reactor, &o->timer);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static struct NCDModule modules[] = {
    {
        .type = "periodic_timer",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_timer = {
    .modules = modules
};
