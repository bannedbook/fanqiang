/**
 * @file BThreadWork.h
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
 * System for performing computations (possibly) in parallel with the event loop
 * in a different thread.
 */

#ifndef BADVPN_BTHREADWORK_BTHREADWORK_H
#define BADVPN_BTHREADWORK_BTHREADWORK_H

#ifdef BADVPN_THREADWORK_USE_PTHREAD
    #include <pthread.h>
    #include <semaphore.h>
#endif

#include <misc/debug.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <system/BReactor.h>

#define BTHREADWORK_STATE_PENDING 1
#define BTHREADWORK_STATE_RUNNING 2
#define BTHREADWORK_STATE_FINISHED 3
#define BTHREADWORK_STATE_FORGOTTEN 4

#define BTHREADWORK_MAX_THREADS 8

struct BThreadWork_s;
struct BThreadWorkDispatcher_s;

/**
 * Function called to do the work for a {@link BThreadWork}.
 * The function may be called in another thread, in parallel with the event loop.
 * 
 * @param user as work_func_user in {@link BThreadWork_Init}
 */
typedef void (*BThreadWork_work_func) (void *user);

/**
 * Handler called when a {@link BThreadWork} work is done.
 * 
 * @param user as in {@link BThreadWork_Init}
 */
typedef void (*BThreadWork_handler_done) (void *user);

#ifdef BADVPN_THREADWORK_USE_PTHREAD
struct BThreadWorkDispatcher_thread {
    struct BThreadWorkDispatcher_s *d;
    struct BThreadWork_s *running_work;
    pthread_cond_t new_cond;
    pthread_t thread;
};
#endif

typedef struct BThreadWorkDispatcher_s {
    BReactor *reactor;
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    LinkedList1 pending_list;
    LinkedList1 finished_list;
    pthread_mutex_t mutex;
    int pipe[2];
    BFileDescriptor bfd;
    BPending more_job;
    int cancel;
    int num_threads;
    struct BThreadWorkDispatcher_thread threads[BTHREADWORK_MAX_THREADS];
    #endif
    DebugObject d_obj;
    DebugCounter d_ctr;
} BThreadWorkDispatcher;

typedef struct BThreadWork_s {
    BThreadWorkDispatcher *d;
    BThreadWork_handler_done handler_done;
    void *user;
    BThreadWork_work_func work_func;
    void *work_func_user;
    union {
        #ifdef BADVPN_THREADWORK_USE_PTHREAD
        struct {
            LinkedList1Node list_node;
            int state;
            sem_t finished_sem;
        };
        #endif
        struct {
            BPending job;
        };
    };
    DebugObject d_obj;
} BThreadWork;

/**
 * Initializes the work dispatcher.
 * Works may be started using {@link BThreadWork_Init}.
 * 
 * @param o the object
 * @param reactor reactor we live in
 * @param num_threads_hint hint for the number of threads to use:
 *                         <0 - A choice will be made automatically, probably based on the number of CPUs.
 *                         0 - No additional threads will be used, and computations will be performed directly
 *                             in the event loop in job handlers.
 * @return 1 on success, 0 on failure
 */
int BThreadWorkDispatcher_Init (BThreadWorkDispatcher *o, BReactor *reactor, int num_threads_hint) WARN_UNUSED;

/**
 * Frees the work dispatcher.
 * There must be no {@link BThreadWork}'s with this dispatcher.
 * 
 * @param o the object
 */
void BThreadWorkDispatcher_Free (BThreadWorkDispatcher *o);

/**
 * Determines whether threads are being used for computations, or computations
 * are done in the event loop.
 * 
 * @return 1 if threads are being used, 0 if not
 */
int BThreadWorkDispatcher_UsingThreads (BThreadWorkDispatcher *o);

/**
 * Initializes the work.
 * 
 * @param o the object
 * @param d work dispatcher
 * @param handler_done handler to call when the work is done
 * @param user argument to handler
 * @param work_func function that will do the work, possibly from another thread
 * @param work_func_user argument to work_func
 */
void BThreadWork_Init (BThreadWork *o, BThreadWorkDispatcher *d, BThreadWork_handler_done handler_done, void *user, BThreadWork_work_func work_func, void *work_func_user);

/**
 * Frees the work.
 * After this function returns, the work function will either have fully executed,
 * or not called at all, and never will be.
 * 
 * @param o the object
 */
void BThreadWork_Free (BThreadWork *o);

#endif
