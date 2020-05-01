/**
 * @file BReactor_emscripten.c
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
 */

#include <stdio.h>
#include <inttypes.h>

#include <emscripten/emscripten.h>

#include "BReactor_emscripten.h"

#include <misc/debug.h>

#include <generated/blog_channel_BReactor.h>

static void assert_timer (BTimer *bt, BReactor *reactor)
{
    ASSERT(bt->active == 0 || bt->active == 1);
    ASSERT(bt->active == 0 || bt->reactor);
    ASSERT(bt->active == 0 || (!reactor || bt->reactor == reactor));
}

static void dispatch_pending (BReactor *o)
{
    while (BPendingGroup_HasJobs(&o->pending_jobs)) {
        BPendingGroup_ExecuteJob(&o->pending_jobs);
    }
}

__attribute__((used))
void breactor_timer_cb (BReactor *reactor, BTimer *bt)
{
    assert_timer(bt, reactor);
    ASSERT(bt->active);
    ASSERT(!BPendingGroup_HasJobs(&reactor->pending_jobs));
    
    bt->active = 0;
    
    bt->handler(bt->handler_pointer);
    dispatch_pending(reactor);
}

static void small_timer_handler (void *vbt)
{
    BSmallTimer *bt = vbt;
    
    bt->handler(bt);
}

void BTimer_Init (BTimer *bt, btime_t msTime, BTimer_handler handler, void *user)
{
    bt->msTime = msTime;
    bt->handler = handler;
    bt->handler_pointer = user;
    bt->active = 0;
}

int BTimer_IsRunning (BTimer *bt)
{
    assert_timer(bt, NULL);
    
    return bt->active;
}

void BSmallTimer_Init (BSmallTimer *bt, BSmallTimer_handler handler)
{
    BTimer_Init(&bt->timer, 0, small_timer_handler, bt);
    bt->handler = handler;
}

int BSmallTimer_IsRunning (BSmallTimer *bt)
{
    return BTimer_IsRunning(&bt->timer);
}

void BReactor_EmscriptenInit (BReactor *bsys)
{
    BPendingGroup_Init(&bsys->pending_jobs);
    
    DebugObject_Init(&bsys->d_obj);
}

void BReactor_EmscriptenFree (BReactor *bsys)
{
    DebugObject_Free(&bsys->d_obj);
    ASSERT(!BPendingGroup_HasJobs(&bsys->pending_jobs));
}

void BReactor_EmscriptenSync (BReactor *bsys)
{
    DebugObject_Access(&bsys->d_obj);
    
    dispatch_pending(bsys);
}

BPendingGroup * BReactor_PendingGroup (BReactor *bsys)
{
    DebugObject_Access(&bsys->d_obj);
    
    return &bsys->pending_jobs;
}

void BReactor_SetTimer (BReactor *bsys, BTimer *bt)
{
    BReactor_SetTimerAfter(bsys, bt, bt->msTime);
}

void BReactor_SetTimerAfter (BReactor *bsys, BTimer *bt, btime_t after)
{
    DebugObject_Access(&bsys->d_obj);
    assert_timer(bt, bsys);
    
    if (bt->active) {
        BReactor_RemoveTimer(bsys, bt);
    }
    
    char cmd[120];
    sprintf(cmd, "setTimeout(function(){Module.ccall('breactor_timer_cb','null',['number','number'],[%d,%d]);},%"PRIi64")", (int)bsys, (int)bt, after);
    
    bt->active = 1;
    bt->timerid = emscripten_run_script_int(cmd);
    bt->reactor = bsys;
}

void BReactor_RemoveTimer (BReactor *bsys, BTimer *bt)
{
    DebugObject_Access(&bsys->d_obj);
    assert_timer(bt, bsys);
    
    if (!bt->active) {
        return;
    }
    
    char cmd[30];
    sprintf(cmd, "clearTimeout(%d)", bt->timerid);
    
    emscripten_run_script(cmd);
    bt->active = 0;
}

void BReactor_SetSmallTimer (BReactor *bsys, BSmallTimer *bt, int mode, btime_t time)
{
    ASSERT(mode == BTIMER_SET_RELATIVE)
    
    BReactor_SetTimerAfter(bsys, &bt->timer, time);
}

void BReactor_RemoveSmallTimer (BReactor *bsys, BSmallTimer *bt)
{
    BReactor_RemoveTimer(bsys, &bt->timer);
}
