/**
 * @file BMutex.h
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

#ifndef BADVPN_BMUTEX_H
#define BADVPN_BMUTEX_H

#if !defined(BADVPN_THREAD_SAFE) || (BADVPN_THREAD_SAFE != 0 && BADVPN_THREAD_SAFE != 1)
#error BADVPN_THREAD_SAFE is not defined or incorrect
#endif

#if BADVPN_THREAD_SAFE
#include <pthread.h>
#endif

#include <misc/debug.h>
#include <base/DebugObject.h>

typedef struct {
#if BADVPN_THREAD_SAFE
    pthread_mutex_t pthread_mutex;
#endif
    DebugObject d_obj;
} BMutex;

static int BMutex_Init (BMutex *o) WARN_UNUSED;
static void BMutex_Free (BMutex *o);
static void BMutex_Lock (BMutex *o);
static void BMutex_Unlock (BMutex *o);

static int BMutex_Init (BMutex *o)
{
#if BADVPN_THREAD_SAFE
    if (pthread_mutex_init(&o->pthread_mutex, NULL) != 0) {
        return 0;
    }
#endif
    
    DebugObject_Init(&o->d_obj);
    return 1;
}

static void BMutex_Free (BMutex *o)
{
    DebugObject_Free(&o->d_obj);
    
#if BADVPN_THREAD_SAFE
    int res = pthread_mutex_destroy(&o->pthread_mutex);
    B_USE(res)
    ASSERT(res == 0)
#endif
}

static void BMutex_Lock (BMutex *o)
{
    DebugObject_Access(&o->d_obj);
    
#if BADVPN_THREAD_SAFE
    int res = pthread_mutex_lock(&o->pthread_mutex);
    B_USE(res)
    ASSERT(res == 0)
#endif
}

static void BMutex_Unlock (BMutex *o)
{
    DebugObject_Access(&o->d_obj);
    
#if BADVPN_THREAD_SAFE
    int res = pthread_mutex_unlock(&o->pthread_mutex);
    B_USE(res)
    ASSERT(res == 0)
#endif
}

#endif
