/**
 * @file BTime.h
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
 * System time abstraction used by {@link BReactor}.
 */

#ifndef BADVPN_SYSTEM_BTIME_H
#define BADVPN_SYSTEM_BTIME_H

#if defined(BADVPN_USE_WINAPI)
#include <windows.h>
#elif defined(BADVPN_EMSCRIPTEN)
#include <emscripten/emscripten.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

#include <stdint.h>

#include <misc/debug.h>
#include <misc/overflow.h>
#include <base/BLog.h>

#include <generated/blog_channel_BTime.h>

typedef int64_t btime_t;

#define BTIME_MIN INT64_MIN

struct _BTime_global {
    #ifndef NDEBUG
    int initialized; // initialized statically
    #endif
    #if defined(BADVPN_USE_WINAPI)
    LARGE_INTEGER start_time;
    #elif defined(BADVPN_EMSCRIPTEN)
    btime_t start_time;
    #else
    btime_t start_time;
    int use_gettimeofday;
    #endif
};

extern struct _BTime_global btime_global;

static void BTime_Init (void)
{
    ASSERT(!btime_global.initialized)
    
    #if defined(BADVPN_USE_WINAPI)
    
    ASSERT_FORCE(QueryPerformanceCounter(&btime_global.start_time))
    
    #elif defined(BADVPN_EMSCRIPTEN)
    
    btime_global.start_time = emscripten_get_now();
    
    #else
    
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
        BLog(BLOG_WARNING, "CLOCK_MONOTONIC is not available. Timers will be confused by clock changes.");
        
        struct timeval tv;
        ASSERT_FORCE(gettimeofday(&tv, NULL) == 0)
        
        btime_global.start_time = (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec/1000;
        btime_global.use_gettimeofday = 1;
    } else {
        btime_global.start_time = (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec/1000000;
        btime_global.use_gettimeofday = 0;
    }
    
    #endif
    
    #ifndef NDEBUG
    btime_global.initialized = 1;
    #endif
}

static btime_t btime_gettime (void)
{
    ASSERT(btime_global.initialized)
    
    #if defined(BADVPN_USE_WINAPI)
    
    LARGE_INTEGER count;
    LARGE_INTEGER freq;
    ASSERT_FORCE(QueryPerformanceCounter(&count))
    ASSERT_FORCE(QueryPerformanceFrequency(&freq))
    return (((count.QuadPart - btime_global.start_time.QuadPart) * 1000) / freq.QuadPart);
    
    #elif defined(BADVPN_EMSCRIPTEN)
    
    return (btime_t)emscripten_get_now() - btime_global.start_time;
    
    #else
    
    if (btime_global.use_gettimeofday) {
        struct timeval tv;
        ASSERT_FORCE(gettimeofday(&tv, NULL) == 0)
        return ((int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec/1000);
    } else {
        struct timespec ts;
        ASSERT_FORCE(clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (((int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec/1000000) - btime_global.start_time);
    }
    
    #endif
}

static btime_t btime_add (btime_t t1, btime_t t2)
{
    // handle overflow
    int overflows = add_int64_overflows(t1, t2);
    btime_t sum;
    if (overflows != 0) {
        if (overflows > 0) {
            sum = INT64_MAX;
        } else {
            sum = INT64_MIN;
        }
    } else {
        sum = t1 + t2;
    }
    
    return sum;
}

static btime_t btime_getpast (void)
{
    return INT64_MIN;
}

#endif
