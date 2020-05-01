/**
 * @file BReactor_badvpn.c
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
#include <stdio.h>
#include <stddef.h>

#ifdef BADVPN_USE_WINAPI
#include <windows.h>
#else
#include <limits.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#endif

#include <misc/debug.h>
#include <misc/offset.h>
#include <misc/balloc.h>
#include <misc/compare.h>
#include <base/BLog.h>

#include <system/BReactor.h>

#include <generated/blog_channel_BReactor.h>

#define KEVENT_TAG_FD 1
#define KEVENT_TAG_KEVENT 2

#define TIMER_STATE_INACTIVE 1
#define TIMER_STATE_RUNNING 2
#define TIMER_STATE_EXPIRED 3

static int compare_timers (BSmallTimer *t1, BSmallTimer *t2)
{
    int cmp = B_COMPARE(t1->absTime, t2->absTime);
    if (cmp) {
        return cmp;
    }
    
    return B_COMPARE((uintptr_t)t1, (uintptr_t)t2);
}

#include "BReactor_badvpn_timerstree.h"
#include <structure/CAvl_impl.h>

static void assert_timer (BSmallTimer *bt)
{
    ASSERT(bt->state == TIMER_STATE_INACTIVE || bt->state == TIMER_STATE_RUNNING ||
           bt->state == TIMER_STATE_EXPIRED)
}

static int move_expired_timers (BReactor *bsys, btime_t now)
{
    int moved = 0;
    
    // move timed out timers to the expired list
    BReactor__TimersTreeRef ref;
    BSmallTimer *timer;
    while (timer = (ref = BReactor__TimersTree_GetFirst(&bsys->timers_tree, 0)).link) {
        ASSERT(timer->state == TIMER_STATE_RUNNING)
        
        // if it's in the future, stop
        if (timer->absTime > now) {
            break;
        }
        moved = 1;
        
        // remove from running timers tree
        BReactor__TimersTree_Remove(&bsys->timers_tree, 0, ref);
        
        // add to expired timers list
        LinkedList1_Append(&bsys->timers_expired_list, &timer->u.list_node);

        // set expired
        timer->state = TIMER_STATE_EXPIRED;
    }

    return moved;
}

static void move_first_timers (BReactor *bsys)
{
    BReactor__TimersTreeRef ref;
    
    // get the time of the first timer
    BSmallTimer *first_timer = (ref = BReactor__TimersTree_GetFirst(&bsys->timers_tree, 0)).link;
    ASSERT(first_timer)
    ASSERT(first_timer->state == TIMER_STATE_RUNNING)
    btime_t first_time = first_timer->absTime;
    
    // remove from running timers tree
    BReactor__TimersTree_Remove(&bsys->timers_tree, 0, ref);
    
    // add to expired timers list
    LinkedList1_Append(&bsys->timers_expired_list, &first_timer->u.list_node);
    
    // set expired
    first_timer->state = TIMER_STATE_EXPIRED;
    
    // also move other timers with the same timeout
    BSmallTimer *timer;
    while (timer = (ref = BReactor__TimersTree_GetFirst(&bsys->timers_tree, 0)).link) {
        ASSERT(timer->state == TIMER_STATE_RUNNING)
        ASSERT(timer->absTime >= first_time)
        
        // if it's in the future, stop
        if (timer->absTime > first_time) {
            break;
        }
        
        // remove from running timers tree
        BReactor__TimersTree_Remove(&bsys->timers_tree, 0, ref);
        
        // add to expired timers list
        LinkedList1_Append(&bsys->timers_expired_list, &timer->u.list_node);
        
        // set expired
        timer->state = TIMER_STATE_EXPIRED;
    }
}

#ifdef BADVPN_USE_WINAPI

static void set_iocp_ready (BReactorIOCPOverlapped *olap, int succeeded, DWORD bytes)
{
    BReactor *reactor = olap->reactor;
    ASSERT(!olap->is_ready)
    
    // set parameters
    olap->ready_succeeded = succeeded;
    olap->ready_bytes = bytes;
    
    // insert to IOCP ready list
    LinkedList1_Append(&reactor->iocp_ready_list, &olap->ready_list_node);
    
    // set ready
    olap->is_ready = 1;
}

#endif

#ifdef BADVPN_USE_EPOLL

static void set_epoll_fd_pointers (BReactor *bsys)
{
    // Write pointers to our entry pointers into file descriptors.
    // If a handler function frees some other file descriptor, the
    // free routine will set our pointer to NULL so we don't dispatch it.
    for (int i = 0; i < bsys->epoll_results_num; i++) {
        struct epoll_event *event = &bsys->epoll_results[i];
        ASSERT(event->data.ptr)
        BFileDescriptor *bfd = (BFileDescriptor *)event->data.ptr;
        ASSERT(bfd->active)
        ASSERT(!bfd->epoll_returned_ptr)
        bfd->epoll_returned_ptr = (BFileDescriptor **)&event->data.ptr;
    }
}

#endif

#ifdef BADVPN_USE_KEVENT

static void set_kevent_fd_pointers (BReactor *bsys)
{
    for (int i = 0; i < bsys->kevent_results_num; i++) {
        struct kevent *event = &bsys->kevent_results[i];
        ASSERT(event->udata)
        
        int *tag = event->udata;
        switch (*tag) {
            case KEVENT_TAG_FD: {
                BFileDescriptor *bfd = UPPER_OBJECT(tag, BFileDescriptor, kevent_tag);
                ASSERT(bfd->active)
                bsys->kevent_prev_event[i] = bfd->kevent_last_event;
                bfd->kevent_last_event = i;
            } break;
            
            case KEVENT_TAG_KEVENT: {
                BReactorKEvent *kev = UPPER_OBJECT(tag, BReactorKEvent, kevent_tag);
                ASSERT(kev->reactor == bsys)
                bsys->kevent_prev_event[i] = kev->kevent_last_event;
                kev->kevent_last_event = i;
            } break;
            
            default:
                ASSERT(0);
        }
    }
}

static void update_kevent_fd_events (BReactor *bsys, BFileDescriptor *bs, int events)
{
    struct kevent event;
    
    if (!(bs->waitEvents & BREACTOR_READ) && (events & BREACTOR_READ)) {
        memset(&event, 0, sizeof(event));
        event.ident = bs->fd;
        event.filter = EVFILT_READ;
        event.flags = EV_ADD;
        event.udata = &bs->kevent_tag;
        ASSERT_FORCE(kevent(bsys->kqueue_fd, &event, 1, NULL, 0, NULL) == 0)
    }
    else if ((bs->waitEvents & BREACTOR_READ) && !(events & BREACTOR_READ)) {
        memset(&event, 0, sizeof(event));
        event.ident = bs->fd;
        event.filter = EVFILT_READ;
        event.flags = EV_DELETE;
        ASSERT_FORCE(kevent(bsys->kqueue_fd, &event, 1, NULL, 0, NULL) == 0)
    }
    
    if (!(bs->waitEvents & BREACTOR_WRITE) && (events & BREACTOR_WRITE)) {
        memset(&event, 0, sizeof(event));
        event.ident = bs->fd;
        event.filter = EVFILT_WRITE;
        event.flags = EV_ADD;
        event.udata = &bs->kevent_tag;
        ASSERT_FORCE(kevent(bsys->kqueue_fd, &event, 1, NULL, 0, NULL) == 0)
    }
    else if ((bs->waitEvents & BREACTOR_WRITE) && !(events & BREACTOR_WRITE)) {
        memset(&event, 0, sizeof(event));
        event.ident = bs->fd;
        event.filter = EVFILT_WRITE;
        event.flags = EV_DELETE;
        ASSERT_FORCE(kevent(bsys->kqueue_fd, &event, 1, NULL, 0, NULL) == 0)
    }
}

#endif

#ifdef BADVPN_USE_POLL

static void set_poll_fd_pointers (BReactor *bsys)
{
    for (int i = 0; i < bsys->poll_results_num; i++) {
        BFileDescriptor *bfd = bsys->poll_results_bfds[i];
        ASSERT(bfd)
        ASSERT(bfd->active)
        ASSERT(bfd->poll_returned_index == -1)
        bfd->poll_returned_index = i;
    }
}

#endif

static void wait_for_events (BReactor *bsys)
{
    // must have processed all pending events
    ASSERT(!BPendingGroup_HasJobs(&bsys->pending_jobs))
    ASSERT(LinkedList1_IsEmpty(&bsys->timers_expired_list))
    #ifdef BADVPN_USE_WINAPI
    ASSERT(LinkedList1_IsEmpty(&bsys->iocp_ready_list))
    #endif
    #ifdef BADVPN_USE_EPOLL
    ASSERT(bsys->epoll_results_pos == bsys->epoll_results_num)
    #endif
    #ifdef BADVPN_USE_KEVENT
    ASSERT(bsys->kevent_results_pos == bsys->kevent_results_num)
    #endif
    #ifdef BADVPN_USE_POLL
    ASSERT(bsys->poll_results_pos == bsys->poll_results_num)
    #endif

    // clean up epoll results
    #ifdef BADVPN_USE_EPOLL
    bsys->epoll_results_num = 0;
    bsys->epoll_results_pos = 0;
    #endif
    
    // clean up kevent results
    #ifdef BADVPN_USE_KEVENT
    bsys->kevent_results_num = 0;
    bsys->kevent_results_pos = 0;
    #endif
    
    // clean up poll results
    #ifdef BADVPN_USE_POLL
    bsys->poll_results_num = 0;
    bsys->poll_results_pos = 0;
    #endif
    
    // timeout vars
    int have_timeout = 0;
    btime_t timeout_abs;
    btime_t now = 0; // to remove warning
    
    // compute timeout
    BSmallTimer *first_timer = BReactor__TimersTree_GetFirst(&bsys->timers_tree, 0).link;
    if (first_timer) {
        ASSERT(first_timer->state == TIMER_STATE_RUNNING)
        
        // get current time
        now = btime_gettime();
        
        // if some timers have already timed out, return them immediately
        if (move_expired_timers(bsys, now)) {
            BLog(BLOG_DEBUG, "Got already expired timers");
            return;
        }
        
        // timeout is first timer, remember absolute time
        have_timeout = 1;
        timeout_abs = first_timer->absTime;
    }
    
    // wait until the timeout is reached or the file descriptor / handle in ready
    while (1) {
        // compute timeout
        btime_t timeout_rel = 0; // to remove warning
        btime_t timeout_rel_trunc = 0; // to remove warning
        if (have_timeout) {
            timeout_rel = timeout_abs - now;
            timeout_rel_trunc = timeout_rel;
        }
        
        // perform wait
        
        #ifdef BADVPN_USE_WINAPI
        
        if (have_timeout) {
            if (timeout_rel_trunc > INFINITE - 1) {
                timeout_rel_trunc = INFINITE - 1;
            }
        }
        
        DWORD bytes = 0;
        ULONG_PTR key;
        BReactorIOCPOverlapped *olap = NULL;
        BOOL res = GetQueuedCompletionStatus(bsys->iocp_handle, &bytes, &key, (OVERLAPPED **)&olap, (have_timeout ? timeout_rel_trunc : INFINITE));
        
        ASSERT_FORCE(olap || have_timeout)
        
        if (olap || timeout_rel_trunc == timeout_rel) {
            if (olap) {
                BLog(BLOG_DEBUG, "GetQueuedCompletionStatus returned event");
                
                DebugObject_Access(&olap->d_obj);
                ASSERT(olap->reactor == bsys)
                ASSERT(!olap->is_ready)
                
                set_iocp_ready(olap, (res == TRUE), bytes);
            } else {
                BLog(BLOG_DEBUG, "GetQueuedCompletionStatus timed out");
                move_first_timers(bsys);
            }
            break;
        }
        
        #endif
        
        #ifdef BADVPN_USE_EPOLL
        
        if (have_timeout) {
            if (timeout_rel_trunc > INT_MAX) {
                timeout_rel_trunc = INT_MAX;
            }
        }
        
        BLog(BLOG_DEBUG, "Calling epoll_wait");
        
        int waitres = epoll_wait(bsys->efd, bsys->epoll_results, BSYSTEM_MAX_RESULTS, (have_timeout ? timeout_rel_trunc : -1));
        if (waitres < 0) {
            int error = errno;
            if (error == EINTR) {
                BLog(BLOG_DEBUG, "epoll_wait interrupted");
                goto try_again;
            }
            perror("epoll_wait");
            ASSERT_FORCE(0)
        }
        
        ASSERT_FORCE(!(waitres == 0) || have_timeout)
        ASSERT_FORCE(waitres <= BSYSTEM_MAX_RESULTS)
        
        if (waitres != 0 || timeout_rel_trunc == timeout_rel) {
            if (waitres != 0) {
                BLog(BLOG_DEBUG, "epoll_wait returned %d file descriptors", waitres);
                bsys->epoll_results_num = waitres;
                set_epoll_fd_pointers(bsys);
            } else {
                BLog(BLOG_DEBUG, "epoll_wait timed out");
                move_first_timers(bsys);
            }
            break;
        }
        
        #endif
        
        #ifdef BADVPN_USE_KEVENT
        
        struct timespec ts;
        if (have_timeout) {
            if (timeout_rel_trunc > 86400000) {
                timeout_rel_trunc = 86400000;
            }
            ts.tv_sec = timeout_rel_trunc / 1000;
            ts.tv_nsec = (timeout_rel_trunc % 1000) * 1000000;
        }
        
        BLog(BLOG_DEBUG, "Calling kevent");
        
        int waitres = kevent(bsys->kqueue_fd, NULL, 0, bsys->kevent_results, BSYSTEM_MAX_RESULTS, (have_timeout ? &ts : NULL));
        if (waitres < 0) {
            int error = errno;
            if (error == EINTR) {
                BLog(BLOG_DEBUG, "kevent interrupted");
                goto try_again;
            }
            perror("kevent");
            ASSERT_FORCE(0)
        }
        
        ASSERT_FORCE(!(waitres == 0) || have_timeout)
        ASSERT_FORCE(waitres <= BSYSTEM_MAX_RESULTS)
        
        if (waitres != 0 || timeout_rel_trunc == timeout_rel) {
            if (waitres != 0) {
                BLog(BLOG_DEBUG, "kevent returned %d events", waitres);
                bsys->kevent_results_num = waitres;
                set_kevent_fd_pointers(bsys);
            } else {
                BLog(BLOG_DEBUG, "kevent timed out");
                move_first_timers(bsys);
            }
            break;
        }
        
        #endif
        
        #ifdef BADVPN_USE_POLL
        
        if (have_timeout) {
            if (timeout_rel_trunc > INT_MAX) {
                timeout_rel_trunc = INT_MAX;
            }
        }
        
        ASSERT(bsys->poll_num_enabled_fds >= 0)
        ASSERT(bsys->poll_num_enabled_fds <= BSYSTEM_MAX_POLL_FDS)
        int num_fds = 0;
        
        LinkedList1Node *list_node = LinkedList1_GetFirst(&bsys->poll_enabled_fds_list);
        while (list_node) {
            BFileDescriptor *bfd = UPPER_OBJECT(list_node, BFileDescriptor, poll_enabled_fds_list_node);
            ASSERT(bfd->active)
            ASSERT(bfd->poll_returned_index == -1)
            
            // calculate poll events
            int pevents = 0;
            if ((bfd->waitEvents & BREACTOR_READ)) {
                pevents |= POLLIN;
            }
            if ((bfd->waitEvents & BREACTOR_WRITE)) {
                pevents |= POLLOUT;
            }
            
            // write pollfd entry
            struct pollfd *pfd = &bsys->poll_results_pollfds[num_fds];
            pfd->fd = bfd->fd;
            pfd->events = pevents;
            pfd->revents = 0;
            
            // write BFileDescriptor reference entry
            bsys->poll_results_bfds[num_fds] = bfd;
            
            // increment number of fds in array
            num_fds++;
            
            list_node = LinkedList1Node_Next(list_node);
        }
        
        BLog(BLOG_DEBUG, "Calling poll");
        
        int waitres = poll(bsys->poll_results_pollfds, num_fds, (have_timeout ? timeout_rel_trunc : -1));
        if (waitres < 0) {
            int error = errno;
            if (error == EINTR) {
                BLog(BLOG_DEBUG, "poll interrupted");
                goto try_again;
            }
            perror("poll");
            ASSERT_FORCE(0)
        }
        
        ASSERT_FORCE(!(waitres == 0) || have_timeout)
        
        if (waitres != 0 || timeout_rel_trunc == timeout_rel) {
            if (waitres != 0) {
                BLog(BLOG_DEBUG, "poll returned %d file descriptors", waitres);
                bsys->poll_results_num = num_fds;
                bsys->poll_results_pos = 0;
                set_poll_fd_pointers(bsys);
            } else {
                BLog(BLOG_DEBUG, "poll timed out");
                move_first_timers(bsys);
            }
            break;
        }
        
        #endif
        
    try_again:
        if (have_timeout) {
            // get current time
            now = btime_gettime();
            // check if we already reached the time we're waiting for
            if (now >= timeout_abs) {
                BLog(BLOG_DEBUG, "already timed out while trying again");
                move_first_timers(bsys);
                break;
            }
        }
    }
    
    // reset limit objects
    LinkedList1Node *list_node;
    while (list_node = LinkedList1_GetFirst(&bsys->active_limits_list)) {
        BReactorLimit *limit = UPPER_OBJECT(list_node, BReactorLimit, active_limits_list_node);
        ASSERT(limit->count > 0)
        limit->count = 0;
        LinkedList1_Remove(&bsys->active_limits_list, &limit->active_limits_list_node);
    }
}

#ifndef BADVPN_USE_WINAPI

void BFileDescriptor_Init (BFileDescriptor *bs, int fd, BFileDescriptor_handler handler, void *user)
{
    bs->fd = fd;
    bs->handler = handler;
    bs->user = user;
    bs->active = 0;
}

#endif

void BSmallTimer_Init (BSmallTimer *bt, BSmallTimer_handler handler)
{
    bt->handler.smalll = handler;
    bt->state = TIMER_STATE_INACTIVE;
    bt->is_small = 1;
}

int BSmallTimer_IsRunning (BSmallTimer *bt)
{
    assert_timer(bt);
    
    return (bt->state != TIMER_STATE_INACTIVE);
}

void BTimer_Init (BTimer *bt, btime_t msTime, BTimer_handler handler, void *user)
{
    bt->base.handler.heavy = handler;
    bt->base.state = TIMER_STATE_INACTIVE;
    bt->base.is_small = 0;
    bt->user = user;
    bt->msTime = msTime;
}

int BTimer_IsRunning (BTimer *bt)
{
    return BSmallTimer_IsRunning(&bt->base);
}

int BReactor_Init (BReactor *bsys)
{
    BLog(BLOG_DEBUG, "Reactor initializing");
    
    // set not exiting
    bsys->exiting = 0;
    
    // init jobs
    BPendingGroup_Init(&bsys->pending_jobs);
    
    // init timers
    BReactor__TimersTree_Init(&bsys->timers_tree);
    LinkedList1_Init(&bsys->timers_expired_list);
    
    // init limits
    LinkedList1_Init(&bsys->active_limits_list);
    
    #ifdef BADVPN_USE_WINAPI
    
    // init IOCP list
    LinkedList1_Init(&bsys->iocp_list);
    
    // init IOCP handle
    if (!(bsys->iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1))) {
        BLog(BLOG_ERROR, "CreateIoCompletionPort failed");
        goto fail0;
    }
    
    // init IOCP ready list
    LinkedList1_Init(&bsys->iocp_ready_list);
    
    #endif
    
    #ifdef BADVPN_USE_EPOLL
    
    // create epoll fd
    if ((bsys->efd = epoll_create(10)) < 0) {
        BLog(BLOG_ERROR, "epoll_create failed");
        goto fail0;
    }
    
    // init results array
    bsys->epoll_results_num = 0;
    bsys->epoll_results_pos = 0;
    
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    
    // create kqueue fd
    if ((bsys->kqueue_fd = kqueue()) < 0) {
        BLog(BLOG_ERROR, "kqueue failed");
        goto fail0;
    }
    
    // init results array
    bsys->kevent_results_num = 0;
    bsys->kevent_results_pos = 0;
    
    #endif
    
    #ifdef BADVPN_USE_POLL
    
    // init enabled fds list
    LinkedList1_Init(&bsys->poll_enabled_fds_list);
    
    // set zero enabled fds
    bsys->poll_num_enabled_fds = 0;
    
    // allocate results arrays
    if (!(bsys->poll_results_pollfds = BAllocArray(BSYSTEM_MAX_POLL_FDS, sizeof(bsys->poll_results_pollfds[0])))) {
        BLog(BLOG_ERROR, "BAllocArray failed");
        goto fail0;
    }
    if (!(bsys->poll_results_bfds = BAllocArray(BSYSTEM_MAX_POLL_FDS, sizeof(bsys->poll_results_bfds[0])))) {
        BLog(BLOG_ERROR, "BAllocArray failed");
        goto fail1;
    }
    
    // init results array
    bsys->poll_results_num = 0;
    bsys->poll_results_pos = 0;
    
    #endif
    
    DebugObject_Init(&bsys->d_obj);
    #ifndef BADVPN_USE_WINAPI
    DebugCounter_Init(&bsys->d_fds_counter);
    #endif
    #ifdef BADVPN_USE_KEVENT
    DebugCounter_Init(&bsys->d_kevent_ctr);
    #endif
    DebugCounter_Init(&bsys->d_limits_ctr);
    
    return 1;
    
    #ifdef BADVPN_USE_POLL
fail1:
    BFree(bsys->poll_results_pollfds);
    #endif
fail0:
    BPendingGroup_Free(&bsys->pending_jobs);
    BLog(BLOG_ERROR, "Reactor failed to initialize");
    return 0;
}

void BReactor_Free (BReactor *bsys)
{
    DebugObject_Access(&bsys->d_obj);
    
    #ifdef BADVPN_USE_WINAPI
    while (!LinkedList1_IsEmpty(&bsys->iocp_list)) {
        BReactorIOCPOverlapped *olap = UPPER_OBJECT(LinkedList1_GetLast(&bsys->iocp_list), BReactorIOCPOverlapped, iocp_list_node);
        ASSERT(olap->reactor == bsys)
        olap->handler(olap->user, BREACTOR_IOCP_EVENT_EXITING, 0);
    }
    #endif
    
    // {pending group has no BPending objects}
    ASSERT(!BPendingGroup_HasJobs(&bsys->pending_jobs))
    ASSERT(BReactor__TimersTree_IsEmpty(&bsys->timers_tree))
    ASSERT(LinkedList1_IsEmpty(&bsys->timers_expired_list))
    ASSERT(LinkedList1_IsEmpty(&bsys->active_limits_list))
    DebugObject_Free(&bsys->d_obj);
    #ifdef BADVPN_USE_WINAPI
    ASSERT(LinkedList1_IsEmpty(&bsys->iocp_ready_list))
    ASSERT(LinkedList1_IsEmpty(&bsys->iocp_list))
    #endif
    #ifndef BADVPN_USE_WINAPI
    DebugCounter_Free(&bsys->d_fds_counter);
    #endif
    #ifdef BADVPN_USE_KEVENT
    DebugCounter_Free(&bsys->d_kevent_ctr);
    #endif
    DebugCounter_Free(&bsys->d_limits_ctr);
    #ifdef BADVPN_USE_POLL
    ASSERT(bsys->poll_num_enabled_fds == 0)
    ASSERT(LinkedList1_IsEmpty(&bsys->poll_enabled_fds_list))
    #endif
    
    BLog(BLOG_DEBUG, "Reactor freeing");
    
    #ifdef BADVPN_USE_WINAPI
    
    // close IOCP handle
    ASSERT_FORCE(CloseHandle(bsys->iocp_handle))
    
    #endif
    
    #ifdef BADVPN_USE_EPOLL
    
    // close epoll fd
    ASSERT_FORCE(close(bsys->efd) == 0)
    
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    
    // close kqueue fd
    ASSERT_FORCE(close(bsys->kqueue_fd) == 0)
    
    #endif
    
    #ifdef BADVPN_USE_POLL
    
    // free results arrays
    BFree(bsys->poll_results_bfds);
    BFree(bsys->poll_results_pollfds);
    
    #endif
    
    // free jobs
    BPendingGroup_Free(&bsys->pending_jobs);
}

int BReactor_Exec (BReactor *bsys)
{
    BLog(BLOG_DEBUG, "Entering event loop");
    
    while (!bsys->exiting) {
        // dispatch job
        if (BPendingGroup_HasJobs(&bsys->pending_jobs)) {
            BPendingGroup_ExecuteJob(&bsys->pending_jobs);
            continue;
        }
        
        // dispatch timer
        LinkedList1Node *list_node = LinkedList1_GetFirst(&bsys->timers_expired_list);
        if (list_node) {
            BSmallTimer *timer = UPPER_OBJECT(list_node, BSmallTimer, u.list_node);
            ASSERT(timer->state == TIMER_STATE_EXPIRED)
            
            // remove from expired list
            LinkedList1_Remove(&bsys->timers_expired_list, &timer->u.list_node);
            
            // set inactive
            timer->state = TIMER_STATE_INACTIVE;
            
            // call handler
            BLog(BLOG_DEBUG, "Dispatching timer");
            if (timer->is_small) {
                timer->handler.smalll(timer);
            } else {
                BTimer *btimer = UPPER_OBJECT(timer, BTimer, base);
                timer->handler.heavy(btimer->user);
            }
            continue;
        }
        
        #ifdef BADVPN_USE_WINAPI
        
        if (!LinkedList1_IsEmpty(&bsys->iocp_ready_list)) {
            BReactorIOCPOverlapped *olap = UPPER_OBJECT(LinkedList1_GetFirst(&bsys->iocp_ready_list), BReactorIOCPOverlapped, ready_list_node);
            ASSERT(olap->is_ready)
            ASSERT(olap->handler)
            
            // remove from ready list
            LinkedList1_Remove(&bsys->iocp_ready_list, &olap->ready_list_node);
            
            // set not ready
            olap->is_ready = 0;
            
            int event = (olap->ready_succeeded ? BREACTOR_IOCP_EVENT_SUCCEEDED : BREACTOR_IOCP_EVENT_FAILED);
            
            // call handler
            olap->handler(olap->user, event, olap->ready_bytes);
            continue;
        }
        
        #endif
        
        #ifdef BADVPN_USE_EPOLL
        
        // dispatch file descriptor
        if (bsys->epoll_results_pos < bsys->epoll_results_num) {
            // grab event
            struct epoll_event *event = &bsys->epoll_results[bsys->epoll_results_pos];
            bsys->epoll_results_pos++;
            
            // check if the BFileDescriptor was removed
            if (!event->data.ptr) {
                continue;
            }
            
            // get BFileDescriptor
            BFileDescriptor *bfd = (BFileDescriptor *)event->data.ptr;
            ASSERT(bfd->active)
            ASSERT(bfd->epoll_returned_ptr == (BFileDescriptor **)&event->data.ptr)
            
            // zero pointer to the epoll entry
            bfd->epoll_returned_ptr = NULL;
            
            // calculate events to report
            int events = 0;
            if ((bfd->waitEvents&BREACTOR_READ) && (event->events&EPOLLIN)) {
                events |= BREACTOR_READ;
            }
            if ((bfd->waitEvents&BREACTOR_WRITE) && (event->events&EPOLLOUT)) {
                events |= BREACTOR_WRITE;
            }
            if ((event->events&EPOLLERR)) {
                events |= BREACTOR_ERROR;
            }
            if ((event->events&EPOLLHUP)) {
                events |= BREACTOR_HUP;
            }
            
            if (!events) {
                BLog(BLOG_ERROR, "no events detected?");
                continue;
            }
            
            // call handler
            BLog(BLOG_DEBUG, "Dispatching file descriptor");
            bfd->handler(bfd->user, events);
            continue;
        }
        
        #endif
        
        #ifdef BADVPN_USE_KEVENT
        
        // dispatch kevent
        if (bsys->kevent_results_pos < bsys->kevent_results_num) {
            // grab event
            int event_index = bsys->kevent_results_pos;
            struct kevent *event = &bsys->kevent_results[event_index];
            bsys->kevent_results_pos++;
            
            // check if the event was removed
            if (!event->udata) {
                continue;
            }
            
            // check tag
            int *tag = event->udata;
            switch (*tag) {
                case KEVENT_TAG_FD: {
                    // get BFileDescriptor
                    BFileDescriptor *bfd = UPPER_OBJECT(tag, BFileDescriptor, kevent_tag);
                    ASSERT(bfd->active)
                    
                    // when we get to the last event for this fd, reset kevent_last_event
                    if (event_index == bfd->kevent_last_event) {
                        bfd->kevent_last_event = -1;
                    }
                    
                    // calculate event to report
                    int events = 0;
                    if ((bfd->waitEvents&BREACTOR_READ) && event->filter == EVFILT_READ) {
                        events |= BREACTOR_READ;
                    }
                    if ((bfd->waitEvents&BREACTOR_WRITE) && event->filter == EVFILT_WRITE) {
                        events |= BREACTOR_WRITE;
                    }
                    
                    if (!events) {
                        BLog(BLOG_ERROR, "no events detected?");
                        continue;
                    }
                    
                    // call handler
                    BLog(BLOG_DEBUG, "Dispatching file descriptor");
                    bfd->handler(bfd->user, events);
                    continue;
                } break;
                
                case KEVENT_TAG_KEVENT: {
                    // get BReactorKEvent
                    BReactorKEvent *kev = UPPER_OBJECT(tag, BReactorKEvent, kevent_tag);
                    ASSERT(kev->reactor == bsys)
                    
                    // when we get to the last event for this fd, reset kevent_last_event
                    if (event_index == kev->kevent_last_event) {
                        kev->kevent_last_event = -1;
                    }
                    
                    // call handler
                    BLog(BLOG_DEBUG, "Dispatching kevent");
                    kev->handler(kev->user, event->fflags, event->data);
                    continue;
                } break;
                
                default:
                    ASSERT(0);
            }
        }
        
        #endif
        
        #ifdef BADVPN_USE_POLL
        
        if (bsys->poll_results_pos < bsys->poll_results_num) {
            // grab event
            struct pollfd *pfd = &bsys->poll_results_pollfds[bsys->poll_results_pos];
            BFileDescriptor *bfd = bsys->poll_results_bfds[bsys->poll_results_pos];
            bsys->poll_results_pos++;
            
            // skip removed entry
            if (!bfd) {
                continue;
            }
            
            ASSERT(bfd->active)
            ASSERT(bfd->poll_returned_index == bsys->poll_results_pos - 1)
            
            // remove result reference
            bfd->poll_returned_index = -1;
            
            // calculate events to report
            int events = 0;
            if ((bfd->waitEvents & BREACTOR_READ) && (pfd->revents & POLLIN)) {
                events |= BREACTOR_READ;
            }
            if ((bfd->waitEvents & BREACTOR_WRITE) && (pfd->revents & POLLOUT)) {
                events |= BREACTOR_WRITE;
            }
            if ((pfd->revents & POLLERR) || (pfd->revents & POLLHUP)) {
                events |= BREACTOR_ERROR;
            }
            
            if (!events) {
                continue;
            }
            
            // call handler
            BLog(BLOG_DEBUG, "Dispatching file descriptor");
            bfd->handler(bfd->user, events);
            continue;
        }
        
        #endif
        
        wait_for_events(bsys);
    }

    BLog(BLOG_DEBUG, "Exiting event loop, exit code %d", bsys->exit_code);

    return bsys->exit_code;
}

void BReactor_Quit (BReactor *bsys, int code)
{
    bsys->exiting = 1;
    bsys->exit_code = code;
}

void BReactor_SetSmallTimer (BReactor *bsys, BSmallTimer *bt, int mode, btime_t time)
{
    assert_timer(bt);
    ASSERT(mode == BTIMER_SET_ABSOLUTE || mode == BTIMER_SET_RELATIVE)
    
    // unlink it if it's already in the list
    BReactor_RemoveSmallTimer(bsys, bt);
    
    // if mode is relative, add current time
    if (mode == BTIMER_SET_RELATIVE) {
        time = btime_add(btime_gettime(), time);
    }
    
    // set time
    bt->absTime = time;
    
    // set running
    bt->state = TIMER_STATE_RUNNING;
    
    // insert to running timers tree
    BReactor__TimersTreeRef ref = {bt, bt};
    int res = BReactor__TimersTree_Insert(&bsys->timers_tree, 0, ref, NULL);
    ASSERT_EXECUTE(res)
}

void BReactor_RemoveSmallTimer (BReactor *bsys, BSmallTimer *bt)
{
    assert_timer(bt);
    
    if (bt->state == TIMER_STATE_INACTIVE) {
        return;
    }

    if (bt->state == TIMER_STATE_EXPIRED) {
        // remove from expired list
        LinkedList1_Remove(&bsys->timers_expired_list, &bt->u.list_node);
    } else {
        // remove from running tree
        BReactor__TimersTreeRef ref = {bt, bt};
        BReactor__TimersTree_Remove(&bsys->timers_tree, 0, ref);
    }

    // set inactive
    bt->state = TIMER_STATE_INACTIVE;
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
    return &bsys->pending_jobs;
}

int BReactor_Synchronize (BReactor *bsys, BSmallPending *ref)
{
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

#ifndef BADVPN_USE_WINAPI

int BReactor_AddFileDescriptor (BReactor *bsys, BFileDescriptor *bs)
{
    ASSERT(!bs->active)
    
    #ifdef BADVPN_USE_EPOLL
    
    // add epoll entry
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = 0;
    event.data.ptr = bs;
    if (epoll_ctl(bsys->efd, EPOLL_CTL_ADD, bs->fd, &event) < 0) {
        int error = errno;
        BLog(BLOG_ERROR, "epoll_ctl failed: %d", error);
        return 0;
    }
    
    // set epoll returned pointer
    bs->epoll_returned_ptr = NULL;
    
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    
    // set kevent tag
    bs->kevent_tag = KEVENT_TAG_FD;
    
    // have no events
    bs->kevent_last_event = -1;
    
    #endif
    
    #ifdef BADVPN_USE_POLL
    
    if (bsys->poll_num_enabled_fds == BSYSTEM_MAX_POLL_FDS) {
        BLog(BLOG_ERROR, "too many fds");
        return 0;
    }
    
    // append to enabled fds list
    LinkedList1_Append(&bsys->poll_enabled_fds_list, &bs->poll_enabled_fds_list_node);
    bsys->poll_num_enabled_fds++;
    
    // set not returned
    bs->poll_returned_index = -1;
    
    #endif
    
    bs->active = 1;
    bs->waitEvents = 0;
    
    DebugCounter_Increment(&bsys->d_fds_counter);
    return 1;
}

void BReactor_RemoveFileDescriptor (BReactor *bsys, BFileDescriptor *bs)
{
    ASSERT(bs->active)
    DebugCounter_Decrement(&bsys->d_fds_counter);

    bs->active = 0;

    #ifdef BADVPN_USE_EPOLL
    
    // delete epoll entry
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    ASSERT_FORCE(epoll_ctl(bsys->efd, EPOLL_CTL_DEL, bs->fd, &event) == 0)
    
    // write through epoll returned pointer
    if (bs->epoll_returned_ptr) {
        *bs->epoll_returned_ptr = NULL;
    }
    
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    
    // delete kevents
    update_kevent_fd_events(bsys, bs, 0);
    
    // invalidate any events
    int event_index = bs->kevent_last_event;
    while (event_index != -1) {
        ASSERT(event_index >= 0 && event_index < bsys->kevent_results_num)
        struct kevent *event = &bsys->kevent_results[event_index];
        event->udata = NULL;
        event_index = bsys->kevent_prev_event[event_index];
    }
    
    #endif
    
    #ifdef BADVPN_USE_POLL
    
    // invalidate results entry
    if (bs->poll_returned_index != -1) {
        ASSERT(bs->poll_returned_index >= bsys->poll_results_pos)
        ASSERT(bs->poll_returned_index < bsys->poll_results_num)
        ASSERT(bsys->poll_results_bfds[bs->poll_returned_index] == bs)
        
        bsys->poll_results_bfds[bs->poll_returned_index] = NULL;
    }
    
    // remove from enabled fds list
    LinkedList1_Remove(&bsys->poll_enabled_fds_list, &bs->poll_enabled_fds_list_node);
    bsys->poll_num_enabled_fds--;
    
    #endif
}

void BReactor_SetFileDescriptorEvents (BReactor *bsys, BFileDescriptor *bs, int events)
{
    ASSERT(bs->active)
    ASSERT(!(events&~(BREACTOR_READ|BREACTOR_WRITE)))
    
    if (bs->waitEvents == events) {
        return;
    }
    
    #ifdef BADVPN_USE_EPOLL
    
    // calculate epoll events
    int eevents = 0;
    if ((events & BREACTOR_READ)) {
        eevents |= EPOLLIN;
    }
    if ((events & BREACTOR_WRITE)) {
        eevents |= EPOLLOUT;
    }
    
    // update epoll entry
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = eevents;
    event.data.ptr = bs;
    ASSERT_FORCE(epoll_ctl(bsys->efd, EPOLL_CTL_MOD, bs->fd, &event) == 0)
    
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    
    update_kevent_fd_events(bsys, bs, events);
    
    #endif
    
    // update events
    bs->waitEvents = events;
}

#endif

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

#ifdef BADVPN_USE_KEVENT

int BReactorKEvent_Init (BReactorKEvent *o, BReactor *reactor, BReactorKEvent_handler handler, void *user, uintptr_t ident, short filter, u_int fflags, intptr_t data)
{
    DebugObject_Access(&reactor->d_obj);
    
    // init arguments
    o->reactor = reactor;
    o->handler = handler;
    o->user = user;
    o->ident = ident;
    o->filter = filter;
    
    // add kevent
    struct kevent event;
    memset(&event, 0, sizeof(event));
    event.ident = o->ident;
    event.filter = o->filter;
    event.flags = EV_ADD;
    event.fflags = fflags;
    event.data = data;
    event.udata = &o->kevent_tag;
    if (kevent(o->reactor->kqueue_fd, &event, 1, NULL, 0, NULL) < 0) {
        return 0;
    }
    
    // set kevent tag
    o->kevent_tag = KEVENT_TAG_KEVENT;
    
    // have no events
    o->kevent_last_event = -1;
    
    DebugObject_Init(&o->d_obj);
    DebugCounter_Increment(&o->reactor->d_kevent_ctr);
    return 1;
}

void BReactorKEvent_Free (BReactorKEvent *o)
{
    BReactor *reactor = o->reactor;
    DebugObject_Free(&o->d_obj);
    DebugCounter_Decrement(&reactor->d_kevent_ctr);
    
    // invalidate any events
    int event_index = o->kevent_last_event;
    while (event_index != -1) {
        ASSERT(event_index >= 0 && event_index < reactor->kevent_results_num)
        struct kevent *event = &reactor->kevent_results[event_index];
        event->udata = NULL;
        event_index = reactor->kevent_prev_event[event_index];
    }
    
    // delete kevent
    struct kevent event;
    memset(&event, 0, sizeof(event));
    event.ident = o->ident;
    event.filter = o->filter;
    event.flags = EV_DELETE;
    ASSERT_FORCE(kevent(reactor->kqueue_fd, &event, 1, NULL, 0, NULL) == 0)
}

#endif

#ifdef BADVPN_USE_WINAPI

HANDLE BReactor_GetIOCPHandle (BReactor *reactor)
{
    DebugObject_Access(&reactor->d_obj);
    
    return reactor->iocp_handle;
}

void BReactorIOCPOverlapped_Init (BReactorIOCPOverlapped *o, BReactor *reactor, void *user, BReactorIOCPOverlapped_handler handler)
{
    DebugObject_Access(&reactor->d_obj);
    
    // init arguments
    o->reactor = reactor;
    o->user = user;
    o->handler = handler;
    
    // zero overlapped
    memset(&o->olap, 0, sizeof(o->olap));
    
    // append to IOCP list
    LinkedList1_Append(&reactor->iocp_list, &o->iocp_list_node);
    
    // set not ready
    o->is_ready = 0;
    
    DebugObject_Init(&o->d_obj);
}

void BReactorIOCPOverlapped_Free (BReactorIOCPOverlapped *o)
{
    BReactor *reactor = o->reactor;
    DebugObject_Free(&o->d_obj);
    
    // remove from IOCP ready list
    if (o->is_ready) {
        LinkedList1_Remove(&reactor->iocp_ready_list, &o->ready_list_node);
    }
    
    // remove from IOCP list
    LinkedList1_Remove(&reactor->iocp_list, &o->iocp_list_node);
}

void BReactorIOCPOverlapped_Wait (BReactorIOCPOverlapped *o, int *out_succeeded, DWORD *out_bytes)
{
    BReactor *reactor = o->reactor;
    DebugObject_Access(&o->d_obj);
    
    // wait for IOCP events until we get an event for this olap
    while (!o->is_ready) {
        DWORD bytes = 0;
        ULONG_PTR key;
        BReactorIOCPOverlapped *olap = NULL;
        BOOL res = GetQueuedCompletionStatus(reactor->iocp_handle, &bytes, &key, (OVERLAPPED **)&olap, INFINITE);
        
        ASSERT_FORCE(olap)
        DebugObject_Access(&olap->d_obj);
        ASSERT(olap->reactor == reactor)
        
        // regular I/O should be done synchronously, so we shoudln't ever get a second completion before an
        // existing one is dispatched. If however PostQueuedCompletionStatus is being used to signal events,
        // just discard any excess events.
        if (!olap->is_ready) {
            set_iocp_ready(olap, (res == TRUE), bytes);
        }
    }
    
    // remove from IOCP ready list
    LinkedList1_Remove(&reactor->iocp_ready_list, &o->ready_list_node);
    
    // set not ready
    o->is_ready = 0;
    
    if (out_succeeded) {
        *out_succeeded = o->ready_succeeded;
    }
    if (out_bytes) {
        *out_bytes = o->ready_bytes;
    }
}

#endif
