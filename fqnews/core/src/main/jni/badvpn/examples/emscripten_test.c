/**
 * @file emscripten_test.c
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
#include <inttypes.h>

#include <emscripten/emscripten.h>

#include <misc/debug.h>
#include <system/BTime.h>
#include <system/BReactor.h>

BReactor reactor;
BTimer timer;
BPending job;

static void timer_handler (void *unused)
{
    printf("timer_handler %"PRIu64"\n", btime_gettime());
    
    BPending_Set(&job);
    BReactor_SetTimer(&reactor, &timer);
}

static void job_handler (void *unused)
{
    printf("job_handler %"PRIu64"\n", btime_gettime());
}

int main ()
{
    BTime_Init();
    
    BReactor_EmscriptenInit(&reactor);
    
    BTimer_Init(&timer, 500, timer_handler, NULL);
    BReactor_SetTimer(&reactor, &timer);
    
    BPending_Init(&job, BReactor_PendingGroup(&reactor), job_handler, NULL);
    BPending_Set(&job);
    
    BReactor_EmscriptenSync(&reactor);
    return 0;
}
