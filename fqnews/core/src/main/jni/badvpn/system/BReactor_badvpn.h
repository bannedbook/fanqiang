/**
 * @file BReactor_badvpn.h
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
 * Event loop that supports file desciptor (Linux) or HANDLE (Windows) events
 * and timers.
 */

#ifndef BADVPN_SYSTEM_BREACTOR_H
#define BADVPN_SYSTEM_BREACTOR_H

#if (defined(BADVPN_USE_WINAPI) + defined(BADVPN_USE_EPOLL) + defined(BADVPN_USE_KEVENT) + defined(BADVPN_USE_POLL)) != 1
#error Unknown event backend or too many event backends
#endif

#ifdef BADVPN_USE_WINAPI
#include <windows.h>
#endif

#ifdef BADVPN_USE_EPOLL
#include <sys/epoll.h>
#endif

#ifdef BADVPN_USE_KEVENT
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#endif

#ifdef BADVPN_USE_POLL
#include <poll.h>
#endif

#include <stdint.h>

#include <misc/debug.h>
#include <misc/debugcounter.h>
#include <base/DebugObject.h>
#include <structure/LinkedList1.h>
#include <structure/CAvl.h>
#include <system/BTime.h>
#include <base/BPending.h>

struct BSmallTimer_t;
typedef struct BSmallTimer_t *BReactor_timerstree_link;

#include "BReactor_badvpn_timerstree.h"
#include <structure/CAvl_decl.h>

#define BTIMER_SET_ABSOLUTE 1
#define BTIMER_SET_RELATIVE 2

/**
 * Handler function invoked when the timer expires.
 * The timer was in running state.
 * The timer enters not running state before this function is invoked.
 * This function is being called from within the timer's previosly
 * associated reactor.
 *
 * @param timer pointer to the timer. Use the {@link UPPER_OBJECT} macro
 *              to obtain the pointer to the containing structure.
 */
typedef void (*BSmallTimer_handler) (struct BSmallTimer_t *timer);

/**
 * Handler function invoked when the timer expires.
 * The timer was in running state.
 * The timer enters not running state before this function is invoked.
 * This function is being called from within the timer's previosly
 * associated reactor.
 *
 * @param user value passed to {@link BTimer_Init}
 */
typedef void (*BTimer_handler) (void *user);

/**
 * Timer object used with {@link BReactor}.
 */
typedef struct BSmallTimer_t {
    union {
        BSmallTimer_handler smalll; // MSVC doesn't like "small"
        BTimer_handler heavy;
    } handler;
    union {
        LinkedList1Node list_node;
        struct BSmallTimer_t *tree_child[2];
    } u;
    struct BSmallTimer_t *tree_parent;
    btime_t absTime;
    int8_t tree_balance;
    uint8_t state;
    uint8_t is_small;
} BSmallTimer;

/**
 * Initializes the timer object.
 * The timer object is initialized in not running state.
 *
 * @param bt the object
 * @param handler handler function invoked when the timer expires
 */
void BSmallTimer_Init (BSmallTimer *bt, BSmallTimer_handler handler);

/**
 * Checks if the timer is running.
 *
 * @param bt the object
 * @return 1 if running, 0 if not running
 */
int BSmallTimer_IsRunning (BSmallTimer *bt);

/**
 * Timer object used with {@link BReactor}. This is a legacy wrapper
 * around {@link BSmallTimer} with an extra field for the default time.
 */
typedef struct {
    BSmallTimer base;
    void *user;
    btime_t msTime;
} BTimer;

/**
 * Initializes the timer object.
 * The timer object is initialized in not running state.
 *
 * @param bt the object
 * @param msTime default timeout in milliseconds
 * @param handler handler function invoked when the timer expires
 * @param user value to pass to the handler function
 */
void BTimer_Init (BTimer *bt, btime_t msTime, BTimer_handler handler, void *user);

/**
 * Checks if the timer is running.
 *
 * @param bt the object
 * @return 1 if running, 0 if not running
 */
int BTimer_IsRunning (BTimer *bt);

#ifndef BADVPN_USE_WINAPI

struct BFileDescriptor_t;

#define BREACTOR_READ (1 << 0)
#define BREACTOR_WRITE (1 << 1)
#define BREACTOR_ERROR (1 << 2)
#define BREACTOR_HUP (1 << 3)

/**
 * Handler function invoked by the reactor when one or more events are detected.
 * The events argument will contain a subset of the monitored events (BREACTOR_READ, BREACTOR_WRITE),
 * plus possibly the error event (BREACTOR_ERROR).
 * The file descriptor object is in active state, being called from within
 * the associated reactor.
 *
 * @param user value passed to {@link BFileDescriptor_Init}
 * @param events bitmask composed of a subset of monitored events (BREACTOR_READ, BREACTOR_WRITE),
 *               and possibly the error event BREACTOR_ERROR and the hang-up event BREACTOR_HUP.
 *               Will be nonzero.
 */
typedef void (*BFileDescriptor_handler) (void *user, int events);

/**
 * File descriptor object used with {@link BReactor}.
 */
typedef struct BFileDescriptor_t {
    int fd;
    BFileDescriptor_handler handler;
    void *user;
    int active;
    int waitEvents;
    
    #ifdef BADVPN_USE_EPOLL
    struct BFileDescriptor_t **epoll_returned_ptr;
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    int kevent_tag;
    int kevent_last_event;
    #endif
    
    #ifdef BADVPN_USE_POLL
    LinkedList1Node poll_enabled_fds_list_node;
    int poll_returned_index;
    #endif
} BFileDescriptor;

/**
 * Intializes the file descriptor object.
 * The object is initialized in not active state.
 *
 * @param bs file descriptor object to initialize
 * @param fb file descriptor to represent
 * @param handler handler function invoked by the reactor when a monitored event is detected
 * @param user value passed to the handler functuon
 */
void BFileDescriptor_Init (BFileDescriptor *bs, int fd, BFileDescriptor_handler handler, void *user);

#endif

// BReactor

#define BSYSTEM_MAX_RESULTS 64
#define BSYSTEM_MAX_HANDLES 64
#define BSYSTEM_MAX_POLL_FDS 4096

/**
 * Event loop that supports file desciptor (Linux) or HANDLE (Windows) events
 * and timers.
 */
typedef struct {
    int exiting;
    int exit_code;
    
    // jobs
    BPendingGroup pending_jobs;
    
    // timers
    BReactor__TimersTree timers_tree;
    LinkedList1 timers_expired_list;
    
    // limits
    LinkedList1 active_limits_list;
    
    #ifdef BADVPN_USE_WINAPI
    LinkedList1 iocp_list;
    HANDLE iocp_handle;
    LinkedList1 iocp_ready_list;
    #endif
    
    #ifdef BADVPN_USE_EPOLL
    int efd; // epoll fd
    struct epoll_event epoll_results[BSYSTEM_MAX_RESULTS]; // epoll returned events buffer
    int epoll_results_num; // number of events in the array
    int epoll_results_pos; // number of events processed so far
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    int kqueue_fd;
    struct kevent kevent_results[BSYSTEM_MAX_RESULTS];
    int kevent_prev_event[BSYSTEM_MAX_RESULTS];
    int kevent_results_num;
    int kevent_results_pos;
    #endif
    
    #ifdef BADVPN_USE_POLL
    LinkedList1 poll_enabled_fds_list;
    int poll_num_enabled_fds;
    int poll_results_num;
    int poll_results_pos;
    struct pollfd *poll_results_pollfds;
    BFileDescriptor **poll_results_bfds;
    #endif
    
    DebugObject d_obj;
    #ifndef BADVPN_USE_WINAPI
    DebugCounter d_fds_counter;
    #endif
    #ifdef BADVPN_USE_KEVENT
    DebugCounter d_kevent_ctr;
    #endif
    DebugCounter d_limits_ctr;
} BReactor;

/**
 * Initializes the reactor.
 * {@link BLog_Init} must have been done.
 * {@link BTime_Init} must have been done.
 *
 * @param bsys the object
 * @return 1 on success, 0 on failure
 */
int BReactor_Init (BReactor *bsys) WARN_UNUSED;

/**
 * Frees the reactor.
 * Must not be called from within the event loop ({@link BReactor_Exec}).
 * There must be no {@link BPending} or {@link BSmallPending} objects using the
 * pending group returned by {@link BReactor_PendingGroup}.
 * There must be no running timers in this reactor.
 * There must be no limit objects in this reactor.
 * There must be no file descriptors or handles registered
 * with this reactor.
 * There must be no {@link BReactorKEvent} objects in this reactor.
 *
 * @param bsys the object
 */
void BReactor_Free (BReactor *bsys);

/**
 * Runs the event loop.
 *
 * @param bsys the object
 * @return value passed to {@link BReactor_Quit}
 */
int BReactor_Exec (BReactor *bsys);

/**
 * Causes the event loop ({@link BReactor_Exec}) to cease
 * dispatching events and return.
 * Any further calls of {@link BReactor_Exec} will return immediately.
 *
 * @param bsys the object
 * @param code value {@link BReactor_Exec} should return. If this is
 *             called more than once, it will return the last code.
 */
void BReactor_Quit (BReactor *bsys, int code);

/**
 * Starts a timer to expire at the specified time.
 * The timer must have been initialized with {@link BSmallTimer_Init}.
 * If the timer is in running state, it must be associated with this reactor.
 * The timer enters running state, associated with this reactor.
 *
 * @param bsys the object
 * @param bt timer to start
 * @param mode interpretation of time (BTIMER_SET_ABSOLUTE or BTIMER_SET_RELATIVE)
 * @param time absolute or relative expiration time
 */
void BReactor_SetSmallTimer (BReactor *bsys, BSmallTimer *bt, int mode, btime_t time);

/**
 * Stops a timer.
 * If the timer is in running state, it must be associated with this reactor.
 * The timer enters not running state.
 *
 * @param bsys the object
 * @param bt timer to stop
 */
void BReactor_RemoveSmallTimer (BReactor *bsys, BSmallTimer *bt);

/**
 * Starts a timer to expire after its default time.
 * The timer must have been initialized with {@link BTimer_Init}.
 * If the timer is in running state, it must be associated with this reactor.
 * The timer enters running state, associated with this reactor.
 *
 * @param bsys the object
 * @param bt timer to start
 */
void BReactor_SetTimer (BReactor *bsys, BTimer *bt);

/**
 * Starts a timer to expire after a given time.
 * The timer must have been initialized with {@link BTimer_Init}.
 * If the timer is in running state, it must be associated with this reactor.
 * The timer enters running state, associated with this reactor.
 *
 * @param bsys the object
 * @param bt timer to start
 * @param after relative expiration time
 */
void BReactor_SetTimerAfter (BReactor *bsys, BTimer *bt, btime_t after);

/**
 * Starts a timer to expire at the specified time.
 * The timer must have been initialized with {@link BTimer_Init}.
 * If the timer is in running state, it must be associated with this reactor.
 * The timer enters running state, associated with this reactor.
 * The timer's expiration time is set to the time argument.
 *
 * @param bsys the object
 * @param bt timer to start
 * @param time absolute expiration time (according to {@link btime_gettime})
 */
void BReactor_SetTimerAbsolute (BReactor *bsys, BTimer *bt, btime_t time);

/**
 * Stops a timer.
 * If the timer is in running state, it must be associated with this reactor.
 * The timer enters not running state.
 *
 * @param bsys the object
 * @param bt timer to stop
 */
void BReactor_RemoveTimer (BReactor *bsys, BTimer *bt);

/**
 * Returns a {@link BPendingGroup} object that can be used to schedule jobs for
 * the reactor to execute. These jobs have complete priority over other events
 * (timers, file descriptors and Windows handles).
 * The returned pending group may only be used as an argument to {@link BPending_Init},
 * and must not be accessed by other means.
 * All {@link BPending} and {@link BSmallPending} objects using this group must be
 * freed before freeing the reactor.
 * 
 * @param bsys the object
 * @return pending group for scheduling jobs for the reactor to execute
 */
BPendingGroup * BReactor_PendingGroup (BReactor *bsys);

/**
 * Executes pending jobs until either:
 *   - the reference job is reached, or
 *   - {@link BReactor_Quit} is called.
 * The reference job must be reached before the job list empties.
 * The reference job will not be executed.
 * 
 * WARNING: Use with care. This should only be used to to work around third-party software
 * that does not integrade into the jobs system. In particular, you should think about:
 *   - the effects the jobs to be executed may have, and
 *   - the environment those jobs expect to be executed in.
 * 
 * @param bsys the object
 * @param ref reference job. It is not accessed in any way, only its address is compared to
 *            pending jobs before they are executed.
 * @return 1 if the reference job was reached,
 *         0 if {@link BReactor_Quit} was called (either while executing a job, or before)
 */
int BReactor_Synchronize (BReactor *bsys, BSmallPending *ref);

#ifndef BADVPN_USE_WINAPI

/**
 * Starts monitoring a file descriptor.
 *
 * @param bsys the object
 * @param bs file descriptor object. Must have been initialized with
 *           {@link BFileDescriptor_Init} Must be in not active state.
 *           On success, the file descriptor object enters active state,
 *           associated with this reactor.
 * @return 1 on success, 0 on failure
 */
int BReactor_AddFileDescriptor (BReactor *bsys, BFileDescriptor *bs) WARN_UNUSED;

/**
 * Stops monitoring a file descriptor.
 *
 * @param bsys the object
 * @param bs {@link BFileDescriptor} object. Must be in active state,
 *           associated with this reactor. The file descriptor object
 *           enters not active state.
 */
void BReactor_RemoveFileDescriptor (BReactor *bsys, BFileDescriptor *bs);

/**
 * Sets monitored file descriptor events.
 *
 * @param bsys the object
 * @param bs {@link BFileDescriptor} object. Must be in active state,
 *           associated with this reactor.
 * @param events events to watch for. Must not have any bits other than
 *               BREACTOR_READ and BREACTOR_WRITE.
 *               This overrides previosly monitored events.
 */
void BReactor_SetFileDescriptorEvents (BReactor *bsys, BFileDescriptor *bs, int events);

#endif

typedef struct {
    BReactor *reactor;
    int limit;
    int count;
    LinkedList1Node active_limits_list_node;
    DebugObject d_obj;
} BReactorLimit;

/**
 * Initializes a limit object.
 * A limit object consists of a counter integer, which is initialized to
 * zero, is incremented by {@link BReactorLimit_Increment} up to \a limit,
 * and is reset to zero every time the event loop performs a wait.
 * If the event loop has processed all detected events, and before performing
 * a wait, it determines that timers have expired, the counter will not be reset.
 * 
 * @param o the object
 * @param reactor reactor the object is tied to
 * @param limit maximum counter value. Must be >0.
 */
void BReactorLimit_Init (BReactorLimit *o, BReactor *reactor, int limit);

/**
 * Frees a limit object.
 * 
 * @param o the object
 */
void BReactorLimit_Free (BReactorLimit *o);

/**
 * Attempts to increment the counter of a limit object.
 * 
 * @param o the object
 * @return 1 if the counter was lesser than the limit and was incremented,
 *         0 if the counter was greater or equal to the limit and could not be
 *           incremented
 */
int BReactorLimit_Increment (BReactorLimit *o);

/**
 * Sets the limit of a limit object.
 * 
 * @param o the object
 * @param limit new limit. Must be >0.
 */
void BReactorLimit_SetLimit (BReactorLimit *o, int limit);

#ifdef BADVPN_USE_KEVENT

typedef void (*BReactorKEvent_handler) (void *user, u_int fflags, intptr_t data);

typedef struct {
    BReactor *reactor;
    BReactorKEvent_handler handler;
    void *user;
    uintptr_t ident;
    short filter;
    int kevent_tag;
    int kevent_last_event;
    DebugObject d_obj;
} BReactorKEvent;

int BReactorKEvent_Init (BReactorKEvent *o, BReactor *reactor, BReactorKEvent_handler handler, void *user, uintptr_t ident, short filter, u_int fflags, intptr_t data);
void BReactorKEvent_Free (BReactorKEvent *o);

#endif

#ifdef BADVPN_USE_WINAPI

#define BREACTOR_IOCP_EVENT_SUCCEEDED 1
#define BREACTOR_IOCP_EVENT_FAILED 2
#define BREACTOR_IOCP_EVENT_EXITING 3

typedef void (*BReactorIOCPOverlapped_handler) (void *user, int event, DWORD bytes);

typedef struct {
    OVERLAPPED olap;
    BReactor *reactor;
    void *user;
    BReactorIOCPOverlapped_handler handler;
    LinkedList1Node iocp_list_node;
    int is_ready;
    LinkedList1Node ready_list_node;
    int ready_succeeded;
    DWORD ready_bytes;
    DebugObject d_obj;
} BReactorIOCPOverlapped;

HANDLE BReactor_GetIOCPHandle (BReactor *reactor);

void BReactorIOCPOverlapped_Init (BReactorIOCPOverlapped *o, BReactor *reactor, void *user, BReactorIOCPOverlapped_handler handler);
void BReactorIOCPOverlapped_Free (BReactorIOCPOverlapped *o);
void BReactorIOCPOverlapped_Wait (BReactorIOCPOverlapped *o, int *out_succeeded, DWORD *out_bytes);

#endif

#endif
