/**
 * @file BReactor_glib.c
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

#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>
#include <base/BLog.h>

#include "BReactor_glib.h"

#include <generated/blog_channel_BReactor.h>

struct fd_source {
    GSource source;
    BFileDescriptor *bfd;
};

static void assert_timer (BSmallTimer *bt)
{
    ASSERT(bt->is_small == 0 || bt->is_small == 1)
    ASSERT(bt->active == 0 || bt->active == 1)
    ASSERT(!bt->active || bt->reactor)
    ASSERT(!bt->active || bt->source)
}

static void dispatch_pending (BReactor *o)
{
    while (!o->exiting && BPendingGroup_HasJobs(&o->pending_jobs)) {
        BPendingGroup_ExecuteJob(&o->pending_jobs);
    }
}

static void reset_limits (BReactor *o)
{
    LinkedList1Node *list_node;
    while (list_node = LinkedList1_GetFirst(&o->active_limits_list)) {
        BReactorLimit *limit = UPPER_OBJECT(list_node, BReactorLimit, active_limits_list_node);
        ASSERT(limit->count > 0)
        limit->count = 0;
        LinkedList1_Remove(&o->active_limits_list, &limit->active_limits_list_node);
    }
}

static gushort get_glib_wait_events (int ev)
{
    gushort gev = G_IO_ERR | G_IO_HUP;
    
    if (ev & BREACTOR_READ) {
        gev |= G_IO_IN;
    }
    
    if (ev & BREACTOR_WRITE) {
        gev |= G_IO_OUT;
    }
    
    return gev;
}

static int get_fd_dispatchable_events (BFileDescriptor *bfd)
{
    ASSERT(bfd->active)
    
    int ev = 0;
    
    if ((bfd->waitEvents & BREACTOR_READ) && (bfd->pollfd.revents & G_IO_IN)) {
        ev |= BREACTOR_READ;
    }
    
    if ((bfd->waitEvents & BREACTOR_WRITE) && (bfd->pollfd.revents & G_IO_OUT)) {
        ev |= BREACTOR_WRITE;
    }
    
    if ((bfd->pollfd.revents & G_IO_ERR)) {
        ev |= BREACTOR_ERROR;
    }
    
    if ((bfd->pollfd.revents & G_IO_HUP)) {
        ev |= BREACTOR_HUP;
    }
    
    return ev;
}

static gboolean timer_source_handler (gpointer data)
{
    BSmallTimer *bt = (void *)data;
    assert_timer(bt);
    ASSERT(bt->active)
    
    BReactor *reactor = bt->reactor;
    
    if (reactor->exiting) {
        return FALSE;
    }
    
    g_source_destroy(bt->source);
    g_source_unref(bt->source);
    bt->active = 0;
    DebugCounter_Decrement(&reactor->d_timers_ctr);
    
    if (bt->is_small) {
        bt->handler.smalll(bt);
    } else {
        BTimer *btimer = UPPER_OBJECT(bt, BTimer, base);
        bt->handler.heavy(btimer->user);
    }
    
    dispatch_pending(reactor);
    reset_limits(reactor);
    
    return FALSE;
}

static gboolean fd_source_func_prepare (GSource *source, gint *timeout)
{
    BFileDescriptor *bfd = ((struct fd_source *)source)->bfd;
    ASSERT(bfd->active)
    ASSERT(bfd->source == source)
    
    *timeout = -1;
    return FALSE;
}

static gboolean fd_source_func_check (GSource *source)
{
    BFileDescriptor *bfd = ((struct fd_source *)source)->bfd;
    ASSERT(bfd->active)
    ASSERT(bfd->source == source)
    
    return (get_fd_dispatchable_events(bfd) ? TRUE : FALSE);
}

static gboolean fd_source_func_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
    BFileDescriptor *bfd = ((struct fd_source *)source)->bfd;
    BReactor *reactor = bfd->reactor;
    ASSERT(bfd->active)
    ASSERT(bfd->source == source)
    
    if (reactor->exiting) {
        return TRUE;
    }
    
    int events = get_fd_dispatchable_events(bfd);
    if (!events) {
        return TRUE;
    }
    
    bfd->handler(bfd->user, events);
    dispatch_pending(reactor);
    reset_limits(reactor);
    
    return TRUE;
}

void BSmallTimer_Init (BSmallTimer *bt, BSmallTimer_handler handler)
{
    bt->handler.smalll = handler;
    bt->active = 0;
    bt->is_small = 1;
}

int BSmallTimer_IsRunning (BSmallTimer *bt)
{
    assert_timer(bt);
    
    return bt->active;
}

void BTimer_Init (BTimer *bt, btime_t msTime, BTimer_handler handler, void *user)
{
    bt->base.handler.heavy = handler;
    bt->base.active = 0;
    bt->base.is_small = 0;
    bt->user = user;
    bt->msTime = msTime;
}

int BTimer_IsRunning (BTimer *bt)
{
    return BSmallTimer_IsRunning(&bt->base);
}

void BFileDescriptor_Init (BFileDescriptor *bs, int fd, BFileDescriptor_handler handler, void *user)
{
    bs->fd = fd;
    bs->handler = handler;
    bs->user = user;
    bs->active = 0;
}

int BReactor_Init (BReactor *bsys)
{
    return BReactor_InitFromExistingGMainLoop(bsys, g_main_loop_new(NULL, FALSE), 1);
}

void BReactor_Free (BReactor *bsys)
{
    DebugObject_Free(&bsys->d_obj);
    DebugCounter_Free(&bsys->d_timers_ctr);
    DebugCounter_Free(&bsys->d_limits_ctr);
    DebugCounter_Free(&bsys->d_fds_counter);
    ASSERT(!BPendingGroup_HasJobs(&bsys->pending_jobs))
    ASSERT(LinkedList1_IsEmpty(&bsys->active_limits_list))
    
    // free job queue
    BPendingGroup_Free(&bsys->pending_jobs);
    
    // unref main loop if needed
    if (bsys->unref_gloop_on_free) {
        g_main_loop_unref(bsys->gloop);
    }
}

int BReactor_Exec (BReactor *bsys)
{
    DebugObject_Access(&bsys->d_obj);
    
    // dispatch pending jobs (until exiting) and reset limits
    dispatch_pending(bsys);
    reset_limits(bsys);
    
    // if exiting, do not enter glib loop
    if (bsys->exiting) {
        return bsys->exit_code;
    }
    
    // enter glib loop
    g_main_loop_run(bsys->gloop);
    
    ASSERT(bsys->exiting)
    
    return bsys->exit_code;
}

void BReactor_Quit (BReactor *bsys, int code)
{
    DebugObject_Access(&bsys->d_obj);
    
    // remember exiting
    bsys->exiting = 1;
    bsys->exit_code = code;
    
    // request termination of glib loop
    g_main_loop_quit(bsys->gloop);
}

void BReactor_SetSmallTimer (BReactor *bsys, BSmallTimer *bt, int mode, btime_t time)
{
    DebugObject_Access(&bsys->d_obj);
    assert_timer(bt);
    
    // remove timer if it's already set
    BReactor_RemoveSmallTimer(bsys, bt);
    
    // if mode is absolute, subtract current time
    if (mode == BTIMER_SET_ABSOLUTE) {
        btime_t now = btime_gettime();
        time = (time < now ? 0 : time - now);
    }
    
    // set active and reactor
    bt->active = 1;
    bt->reactor = bsys;
    
    // init source
    bt->source = g_timeout_source_new(time);
    g_source_set_callback(bt->source, timer_source_handler, bt, NULL);
    g_source_attach(bt->source, g_main_loop_get_context(bsys->gloop));
    
    DebugCounter_Increment(&bsys->d_timers_ctr);
}

void BReactor_RemoveSmallTimer (BReactor *bsys, BSmallTimer *bt)
{
    DebugObject_Access(&bsys->d_obj);
    assert_timer(bt);
    
    // do nothing if timer is not active
    if (!bt->active) {
        return;
    }
    
    // free source
    g_source_destroy(bt->source);
    g_source_unref(bt->source);
    
    // set not active
    bt->active = 0;
    
    DebugCounter_Decrement(&bsys->d_timers_ctr);
}

void BReactor_SetTimer (BReactor *bsys, BTimer *bt)
{
    BReactor_SetSmallTimer(bsys, &bt->base, BTIMER_SET_RELATIVE, bt->msTime);
}

void BReactor_SetTimerAfter (BReactor *bsys, BTimer *bt, btime_t after)
{
    BReactor_SetSmallTimer(bsys, &bt->base, BTIMER_SET_RELATIVE, after);
}

void BReactor_SetTimerAbsolute (BReactor *bsys, BTimer *bt, btime_t time)
{
    BReactor_SetSmallTimer(bsys, &bt->base, BTIMER_SET_ABSOLUTE, time);
}

void BReactor_RemoveTimer (BReactor *bsys, BTimer *bt)
{
    return BReactor_RemoveSmallTimer(bsys, &bt->base);
}

BPendingGroup * BReactor_PendingGroup (BReactor *bsys)
{
    DebugObject_Access(&bsys->d_obj);
    
    return &bsys->pending_jobs;
}

int BReactor_Synchronize (BReactor *bsys, BSmallPending *ref)
{
    DebugObject_Access(&bsys->d_obj);
    ASSERT(ref)
    
    while (!bsys->exiting) {
        ASSERT(BPendingGroup_HasJobs(&bsys->pending_jobs))
        
        if (BPendingGroup_PeekJob(&bsys->pending_jobs) == ref) {
            return 1;
        }
        
        BPendingGroup_ExecuteJob(&bsys->pending_jobs);
    }
    
    return 0;
}

int BReactor_AddFileDescriptor (BReactor *bsys, BFileDescriptor *bs)
{
    DebugObject_Access(&bsys->d_obj);
    ASSERT(!bs->active)
    
    // set active, no wait events, and set reactor
    bs->active = 1;
    bs->waitEvents = 0;
    bs->reactor = bsys;
    
    // create source
    bs->source = g_source_new(&bsys->fd_source_funcs, sizeof(struct fd_source));
    ((struct fd_source *)bs->source)->bfd = bs;
    
    // init pollfd
    bs->pollfd.fd = bs->fd;
    bs->pollfd.events = get_glib_wait_events(bs->waitEvents);
    bs->pollfd.revents = 0;
    
    // start source
    g_source_add_poll(bs->source, &bs->pollfd);
    g_source_attach(bs->source, g_main_loop_get_context(bsys->gloop));
    
    DebugCounter_Increment(&bsys->d_fds_counter);
    return 1;
}

void BReactor_RemoveFileDescriptor (BReactor *bsys, BFileDescriptor *bs)
{
    DebugObject_Access(&bsys->d_obj);
    DebugCounter_Decrement(&bsys->d_fds_counter);
    ASSERT(bs->active)
    
    // free source
    g_source_destroy(bs->source);
    g_source_unref(bs->source);
    
    // set not active
    bs->active = 0;
}

void BReactor_SetFileDescriptorEvents (BReactor *bsys, BFileDescriptor *bs, int events)
{
    DebugObject_Access(&bsys->d_obj);
    ASSERT(bs->active)
    ASSERT(!(events&~(BREACTOR_READ|BREACTOR_WRITE)))
    
    // set new wait events
    bs->waitEvents = events;
    
    // update pollfd wait events
    bs->pollfd.events = get_glib_wait_events(bs->waitEvents);
}

int BReactor_InitFromExistingGMainLoop (BReactor *bsys, GMainLoop *gloop, int unref_gloop_on_free)
{
    ASSERT(gloop)
    ASSERT(unref_gloop_on_free == !!unref_gloop_on_free)
    
    // set not exiting
    bsys->exiting = 0;
    
    // set gloop and unref on free flag
    bsys->gloop = gloop;
    bsys->unref_gloop_on_free = unref_gloop_on_free;
    
    // init fd source functions table
    memset(&bsys->fd_source_funcs, 0, sizeof(bsys->fd_source_funcs));
    bsys->fd_source_funcs.prepare = fd_source_func_prepare;
    bsys->fd_source_funcs.check = fd_source_func_check;
    bsys->fd_source_funcs.dispatch = fd_source_func_dispatch;
    bsys->fd_source_funcs.finalize = NULL;
    
    // init job queue
    BPendingGroup_Init(&bsys->pending_jobs);
    
    // init active limits list
    LinkedList1_Init(&bsys->active_limits_list);
    
    DebugCounter_Init(&bsys->d_fds_counter);
    DebugCounter_Init(&bsys->d_limits_ctr);
    DebugCounter_Init(&bsys->d_timers_ctr);
    DebugObject_Init(&bsys->d_obj);
    return 1;
}

GMainLoop * BReactor_GetGMainLoop (BReactor *bsys)
{
    DebugObject_Access(&bsys->d_obj);
    
    return bsys->gloop;
}

int BReactor_SynchronizeAll (BReactor *bsys)
{
    DebugObject_Access(&bsys->d_obj);
    
    dispatch_pending(bsys);
    
    return !bsys->exiting;
}

void BReactorLimit_Init (BReactorLimit *o, BReactor *reactor, int limit)
{
    DebugObject_Access(&reactor->d_obj);
    ASSERT(limit > 0)
    
    // init arguments
    o->reactor = reactor;
    o->limit = limit;
    
    // set count zero
    o->count = 0;
    
    DebugCounter_Increment(&reactor->d_limits_ctr);
    DebugObject_Init(&o->d_obj);
}

void BReactorLimit_Free (BReactorLimit *o)
{
    BReactor *reactor = o->reactor;
    DebugObject_Free(&o->d_obj);
    DebugCounter_Decrement(&reactor->d_limits_ctr);
    
    // remove from active limits list
    if (o->count > 0) {
        LinkedList1_Remove(&reactor->active_limits_list, &o->active_limits_list_node);
    }
}

int BReactorLimit_Increment (BReactorLimit *o)
{
    BReactor *reactor = o->reactor;
    DebugObject_Access(&o->d_obj);
    
    // check count against limit
    if (o->count >= o->limit) {
        return 0;
    }
    
    // increment count
    o->count++;
    
    // if limit was zero, add to active limits list
    if (o->count == 1) {
        LinkedList1_Append(&reactor->active_limits_list, &o->active_limits_list_node);
    }
    
    return 1;
}

void BReactorLimit_SetLimit (BReactorLimit *o, int limit)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(limit > 0)
    
    // set limit
    o->limit = limit;
}
