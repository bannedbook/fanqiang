/**
 * @file BThreadWork.c
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

#include <stdint.h>
#include <stddef.h>

#ifdef BADVPN_THREADWORK_USE_PTHREAD
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
#endif

#include <misc/offset.h>
#include <base/BLog.h>

#include <generated/blog_channel_BThreadWork.h>

#include <threadwork/BThreadWork.h>

#ifdef BADVPN_THREADWORK_USE_PTHREAD

static void * dispatcher_thread (struct BThreadWorkDispatcher_thread *t)
{
    BThreadWorkDispatcher *o = t->d;
    
    ASSERT_FORCE(pthread_mutex_lock(&o->mutex) == 0)
    
    while (1) {
        // exit if requested
        if (o->cancel) {
            break;
        }
        
        if (LinkedList1_IsEmpty(&o->pending_list)) {
            // wait for event
            ASSERT_FORCE(pthread_cond_wait(&t->new_cond, &o->mutex) == 0)
            continue;
        }
        
        // grab the work
        BThreadWork *w = UPPER_OBJECT(LinkedList1_GetFirst(&o->pending_list), BThreadWork, list_node);
        ASSERT(w->state == BTHREADWORK_STATE_PENDING)
        LinkedList1_Remove(&o->pending_list, &w->list_node);
        t->running_work = w;
        w->state = BTHREADWORK_STATE_RUNNING;
        
        // do the work
        ASSERT_FORCE(pthread_mutex_unlock(&o->mutex) == 0)
        w->work_func(w->work_func_user);
        ASSERT_FORCE(pthread_mutex_lock(&o->mutex) == 0)
        
        // release the work
        t->running_work = NULL;
        LinkedList1_Append(&o->finished_list, &w->list_node);
        w->state = BTHREADWORK_STATE_FINISHED;
        ASSERT_FORCE(sem_post(&w->finished_sem) == 0)
        
        // write to pipe
        uint8_t b = 0;
        int res = write(o->pipe[1], &b, sizeof(b));
        if (res < 0) {
            int error = errno;
            ASSERT_FORCE(error == EAGAIN || error == EWOULDBLOCK)
        }
    }
    
    ASSERT_FORCE(pthread_mutex_unlock(&o->mutex) == 0)
    
    return NULL;
}

static void dispatch_job (BThreadWorkDispatcher *o)
{
    ASSERT(o->num_threads > 0)
    
    // lock
    ASSERT_FORCE(pthread_mutex_lock(&o->mutex) == 0)
    
    // check for finished job
    if (LinkedList1_IsEmpty(&o->finished_list)) {
        ASSERT_FORCE(pthread_mutex_unlock(&o->mutex) == 0)
        return;
    }
    
    // grab finished job
    BThreadWork *w = UPPER_OBJECT(LinkedList1_GetFirst(&o->finished_list), BThreadWork, list_node);
    ASSERT(w->state == BTHREADWORK_STATE_FINISHED)
    LinkedList1_Remove(&o->finished_list, &w->list_node);
    
    // schedule more
    if (!LinkedList1_IsEmpty(&o->finished_list)) {
        BPending_Set(&o->more_job);
    }
    
    // set state forgotten
    w->state = BTHREADWORK_STATE_FORGOTTEN;
    
    // unlock
    ASSERT_FORCE(pthread_mutex_unlock(&o->mutex) == 0)
    
    // call handler
    w->handler_done(w->user);
    return;
}

static void pipe_fd_handler (BThreadWorkDispatcher *o, int events)
{
    ASSERT(o->num_threads > 0)
    DebugObject_Access(&o->d_obj);
    
    // read data from pipe
    uint8_t b[64];
    int res = read(o->pipe[0], b, sizeof(b));
    if (res < 0) {
        int error = errno;
        ASSERT_FORCE(error == EAGAIN || error == EWOULDBLOCK)
    } else {
        ASSERT(res > 0)
    }
    
    dispatch_job(o);
    return;
}

static void more_job_handler (BThreadWorkDispatcher *o)
{
    ASSERT(o->num_threads > 0)
    DebugObject_Access(&o->d_obj);
    
    dispatch_job(o);
    return;
}

static void stop_threads (BThreadWorkDispatcher *o)
{
    // set cancelling
    ASSERT_FORCE(pthread_mutex_lock(&o->mutex) == 0)
    o->cancel = 1;
    ASSERT_FORCE(pthread_mutex_unlock(&o->mutex) == 0)
    
    while (o->num_threads > 0) {
        struct BThreadWorkDispatcher_thread *t = &o->threads[o->num_threads - 1];
        
        // wake up thread
        ASSERT_FORCE(pthread_cond_signal(&t->new_cond) == 0)
        
        // wait for thread to exit
        ASSERT_FORCE(pthread_join(t->thread, NULL) == 0)
        
        // free condition variable
        ASSERT_FORCE(pthread_cond_destroy(&t->new_cond) == 0)
        
        o->num_threads--;
    }
}

#endif

static void work_job_handler (BThreadWork *o)
{
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    ASSERT(o->d->num_threads == 0)
    #endif
    DebugObject_Access(&o->d_obj);
    
    // do the work
    o->work_func(o->work_func_user);
    
    // call handler
    o->handler_done(o->user);
    return;
}

int BThreadWorkDispatcher_Init (BThreadWorkDispatcher *o, BReactor *reactor, int num_threads_hint)
{
    // init arguments
    o->reactor = reactor;
    
    if (num_threads_hint < 0) {
        num_threads_hint = 2;
    }
    if (num_threads_hint > BTHREADWORK_MAX_THREADS) {
        num_threads_hint = BTHREADWORK_MAX_THREADS;
    }
    
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    
    if (num_threads_hint > 0) {
        // init pending list
        LinkedList1_Init(&o->pending_list);
        
        // init finished list
        LinkedList1_Init(&o->finished_list);
        
        // init mutex
        if (pthread_mutex_init(&o->mutex, NULL) != 0) {
            BLog(BLOG_ERROR, "pthread_mutex_init failed");
            goto fail0;
        }
        
        // init pipe
        if (pipe(o->pipe) < 0) {
            BLog(BLOG_ERROR, "pipe failed");
            goto fail1;
        }
        
        // set read end non-blocking
        if (fcntl(o->pipe[0], F_SETFL, O_NONBLOCK) < 0) {
            BLog(BLOG_ERROR, "fcntl failed");
            goto fail2;
        }
        
        // set write end non-blocking
        if (fcntl(o->pipe[1], F_SETFL, O_NONBLOCK) < 0) {
            BLog(BLOG_ERROR, "fcntl failed");
            goto fail2;
        }
        
        // init BFileDescriptor
        BFileDescriptor_Init(&o->bfd, o->pipe[0], (BFileDescriptor_handler)pipe_fd_handler, o);
        if (!BReactor_AddFileDescriptor(o->reactor, &o->bfd)) {
            BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
            goto fail2;
        }
        BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, BREACTOR_READ);
        
        // init more job
        BPending_Init(&o->more_job, BReactor_PendingGroup(o->reactor), (BPending_handler)more_job_handler, o);
        
        // set not cancelling
        o->cancel = 0;
        
        // init threads
        o->num_threads = 0;
        for (int i = 0; i < num_threads_hint; i++) {
            struct BThreadWorkDispatcher_thread *t = &o->threads[i];
            
            // set parent pointer
            t->d = o;
            
            // set no running work
            t->running_work = NULL;
            
            // init condition variable
            if (pthread_cond_init(&t->new_cond, NULL) != 0) {
                BLog(BLOG_ERROR, "pthread_cond_init failed");
                goto fail3;
            }
            
            // init thread
            if (pthread_create(&t->thread, NULL, (void * (*) (void *))dispatcher_thread, t) != 0) {
                BLog(BLOG_ERROR, "pthread_create failed");
                ASSERT_FORCE(pthread_cond_destroy(&t->new_cond) == 0)
                goto fail3;
            }
        
            o->num_threads++;
        }
    }
    
    #endif
    
    DebugObject_Init(&o->d_obj);
    DebugCounter_Init(&o->d_ctr);
    return 1;
    
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
fail3:
    stop_threads(o);
    BPending_Free(&o->more_job);
    BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
fail2:
    ASSERT_FORCE(close(o->pipe[0]) == 0)
    ASSERT_FORCE(close(o->pipe[1]) == 0)
fail1:
    ASSERT_FORCE(pthread_mutex_destroy(&o->mutex) == 0)
fail0:
    return 0;
    #endif
}

void BThreadWorkDispatcher_Free (BThreadWorkDispatcher *o)
{
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    if (o->num_threads > 0) {
        ASSERT(LinkedList1_IsEmpty(&o->pending_list))
        for (int i = 0; i < o->num_threads; i++) { ASSERT(!o->threads[i].running_work) }
        ASSERT(LinkedList1_IsEmpty(&o->finished_list))
    }
    #endif
    DebugObject_Free(&o->d_obj);
    DebugCounter_Free(&o->d_ctr);
    
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    
    if (o->num_threads > 0) {
        // stop threads
        stop_threads(o);
        
        // free more job
        BPending_Free(&o->more_job);
        
        // free BFileDescriptor
        BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
        
        // free pipe
        ASSERT_FORCE(close(o->pipe[0]) == 0)
        ASSERT_FORCE(close(o->pipe[1]) == 0)
        
        // free mutex
        ASSERT_FORCE(pthread_mutex_destroy(&o->mutex) == 0)
    }
    
    #endif
}

int BThreadWorkDispatcher_UsingThreads (BThreadWorkDispatcher *o)
{
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    return (o->num_threads > 0);
    #else
    return 0;
    #endif
}

void BThreadWork_Init (BThreadWork *o, BThreadWorkDispatcher *d, BThreadWork_handler_done handler_done, void *user, BThreadWork_work_func work_func, void *work_func_user)
{
    DebugObject_Access(&d->d_obj);
    
    // init arguments
    o->d = d;
    o->handler_done = handler_done;
    o->user = user;
    o->work_func = work_func;
    o->work_func_user = work_func_user;
    
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    if (d->num_threads > 0) {
        // set state
        o->state = BTHREADWORK_STATE_PENDING;
        
        // init finished semaphore
        ASSERT_FORCE(sem_init(&o->finished_sem, 0, 0) == 0)
        
        // post work
        ASSERT_FORCE(pthread_mutex_lock(&d->mutex) == 0)
        LinkedList1_Append(&d->pending_list, &o->list_node);
        for (int i = 0; i < d->num_threads; i++) {
            if (!d->threads[i].running_work) {
                ASSERT_FORCE(pthread_cond_signal(&d->threads[i].new_cond) == 0)
                break;
            }
        }
        ASSERT_FORCE(pthread_mutex_unlock(&d->mutex) == 0)
    } else {
    #endif
        // schedule job
        BPending_Init(&o->job, BReactor_PendingGroup(d->reactor), (BPending_handler)work_job_handler, o);
        BPending_Set(&o->job);
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    }
    #endif
    
    DebugObject_Init(&o->d_obj);
    DebugCounter_Increment(&d->d_ctr);
}

void BThreadWork_Free (BThreadWork *o)
{
    BThreadWorkDispatcher *d = o->d;
    DebugObject_Free(&o->d_obj);
    DebugCounter_Decrement(&d->d_ctr);
    
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    if (d->num_threads > 0) {
        ASSERT_FORCE(pthread_mutex_lock(&d->mutex) == 0)
        
        switch (o->state) {
            case BTHREADWORK_STATE_PENDING: {
                BLog(BLOG_DEBUG, "remove pending work");
                
                // remove from pending list
                LinkedList1_Remove(&d->pending_list, &o->list_node);
            } break;
            
            case BTHREADWORK_STATE_RUNNING: {
                BLog(BLOG_DEBUG, "remove running work");
                
                // wait for the work to finish running
                ASSERT_FORCE(pthread_mutex_unlock(&d->mutex) == 0)
                ASSERT_FORCE(sem_wait(&o->finished_sem) == 0)
                ASSERT_FORCE(pthread_mutex_lock(&d->mutex) == 0)
                
                ASSERT(o->state == BTHREADWORK_STATE_FINISHED)
                
                // remove from finished list
                LinkedList1_Remove(&d->finished_list, &o->list_node);
            } break;
            
            case BTHREADWORK_STATE_FINISHED: {
                BLog(BLOG_DEBUG, "remove finished work");
                
                // remove from finished list
                LinkedList1_Remove(&d->finished_list, &o->list_node);
            } break;
            
            case BTHREADWORK_STATE_FORGOTTEN: {
                BLog(BLOG_DEBUG, "remove forgotten work");
            } break;
            
            default:
                ASSERT(0);
        }
        
        ASSERT_FORCE(pthread_mutex_unlock(&d->mutex) == 0)
        
        // free finished semaphore
        ASSERT_FORCE(sem_destroy(&o->finished_sem) == 0)
    } else {
    #endif
        BPending_Free(&o->job);
    #ifdef BADVPN_THREADWORK_USE_PTHREAD
    }
    #endif
}
