/**
 * @file btimer_example.c
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
#include <stdlib.h>
#include <time.h>

#include <system/BReactor.h>
#include <base/BLog.h>
#include <system/BTime.h>

// gives average firing rate 100kHz
#define TIMER_NUM 500
#define TIMER_MODULO 10

BReactor sys;

void handle_timer (BTimer *bt)
{
    #ifdef BADVPN_USE_WINAPI
    btime_t time = btime_gettime() + rand()%TIMER_MODULO;
    #else
    btime_t time = btime_gettime() + random()%TIMER_MODULO;
    #endif
    BReactor_SetTimerAbsolute(&sys, bt, time);
}

int main ()
{
    BLog_InitStdout();
    
    #ifdef BADVPN_USE_WINAPI
    srand(time(NULL));
    #else
    srandom(time(NULL));
    #endif
    
    // init time
    BTime_Init();

    if (!BReactor_Init(&sys)) {
        DEBUG("BReactor_Init failed");
        return 1;
    }
    
    BTimer timers[TIMER_NUM];

    int i;
    for (i=0; i<TIMER_NUM; i++) {
        BTimer *timer = &timers[i];
        BTimer_Init(timer, 0, (BTimer_handler)handle_timer, timer);
        BReactor_SetTimer(&sys, timer);
    }
    
    int ret = BReactor_Exec(&sys);
    BReactor_Free(&sys);
    return ret;
}
