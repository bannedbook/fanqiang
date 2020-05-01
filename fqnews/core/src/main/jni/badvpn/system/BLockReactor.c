/**
 * @file BLockReactor.c
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

#include <misc/debug.h>
#include <misc/offset.h>
#include <base/BLog.h>

#include "BLockReactor.h"

#include <generated/blog_channel_BLockReactor.h>

static void thread_signal_handler (BThreadSignal *thread_signal)
{
    BLockReactor *o = UPPER_OBJECT(thread_signal, BLockReactor, thread_signal);
    DebugObject_Access(&o->d_obj);
    
    ASSERT_FORCE(sem_post(&o->sem1) == 0)
    ASSERT_FORCE(sem_wait(&o->sem2) == 0)
}

int BLockReactor_Init (BLockReactor *o, BReactor *reactor)
{
    o->reactor = reactor;
    
    if (!BThreadSignal_Init(&o->thread_signal, reactor, thread_signal_handler)) {
        BLog(BLOG_ERROR, "BThreadSignal_Init failed");
        goto fail0;
    }
    
    if (sem_init(&o->sem1, 0, 0) < 0) {
        BLog(BLOG_ERROR, "sem_init failed");
        goto fail1;
    }
    
    if (sem_init(&o->sem2, 0, 0) < 0) {
        BLog(BLOG_ERROR, "sem_init failed");
        goto fail2;
    }
    
#ifndef NDEBUG
    o->locked = 0;
#endif
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail2:
    if (sem_close(&o->sem1) < 0) {
        BLog(BLOG_ERROR, "sem_close failed");
    }
fail1:
    BThreadSignal_Free(&o->thread_signal);
fail0:
    return 0;
}

void BLockReactor_Free (BLockReactor *o)
{
    DebugObject_Free(&o->d_obj);
#ifndef NDEBUG
    ASSERT(!o->locked)
#endif
    
    if (sem_destroy(&o->sem2) < 0) {
        BLog(BLOG_ERROR, "sem_close failed");
    }
    if (sem_destroy(&o->sem1) < 0) {
        BLog(BLOG_ERROR, "sem_close failed");
    }
    BThreadSignal_Free(&o->thread_signal);
}

int BLockReactor_Thread_Lock (BLockReactor *o)
{
    DebugObject_Access(&o->d_obj);
#ifndef NDEBUG
    ASSERT(!o->locked)
#endif
    
    if (!BThreadSignal_Thread_Signal(&o->thread_signal)) {
        return 0;
    }
    
    ASSERT_FORCE(sem_wait(&o->sem1) == 0)
    
#ifndef NDEBUG
    o->locked = 1;
#endif
    
    return 1;
}

void BLockReactor_Thread_Unlock (BLockReactor *o)
{
    DebugObject_Access(&o->d_obj);
#ifndef NDEBUG
    ASSERT(o->locked)
#endif
    
#ifndef NDEBUG
    o->locked = 0;
#endif
    
    ASSERT_FORCE(sem_post(&o->sem2) == 0)
}
