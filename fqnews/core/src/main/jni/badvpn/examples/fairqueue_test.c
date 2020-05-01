/**
 * @file fairqueue_test.c
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

#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include <misc/debug.h>
#include <system/BReactor.h>
#include <base/BLog.h>
#include <system/BTime.h>
#include <flow/PacketPassFairQueue.h>
#include <examples/FastPacketSource.h>
#include <examples/TimerPacketSink.h>

#define OUTPUT_INTERVAL 0
#define REMOVE_INTERVAL 1
#define NUM_INPUTS 3

BReactor reactor;
TimerPacketSink sink;
PacketPassFairQueue fq;
PacketPassFairQueueFlow flows[NUM_INPUTS];
FastPacketSource sources[NUM_INPUTS];
char *data[] = {"0 data", "1 datadatadata", "2 datadatadatadatadata"};
BTimer timer;
int current_cancel;

static void init_input (int i)
{
    PacketPassFairQueueFlow_Init(&flows[i], &fq);
    FastPacketSource_Init(&sources[i], PacketPassFairQueueFlow_GetInput(&flows[i]), (uint8_t *)data[i], strlen(data[i]), BReactor_PendingGroup(&reactor));
}

static void free_input (int i)
{
    FastPacketSource_Free(&sources[i]);
    PacketPassFairQueueFlow_Free(&flows[i]);
}

static void reset_input (void)
{
    PacketPassFairQueueFlow_AssertFree(&flows[current_cancel]);
    
    printf("removing %d\n", current_cancel);
    
    // remove flow
    free_input(current_cancel);
    
    // init flow
    init_input(current_cancel);
    
    // increment cancel
    current_cancel = (current_cancel + 1) % NUM_INPUTS;
    
    // reset timer
    BReactor_SetTimer(&reactor, &timer);
}

static void flow_handler_busy (void *user)
{
    PacketPassFairQueueFlow_AssertFree(&flows[current_cancel]);
    
    reset_input();
}

static void timer_handler (void *user)
{
    // if flow is busy, request cancel and wait for it
    if (PacketPassFairQueueFlow_IsBusy(&flows[current_cancel])) {
        printf("cancelling %d\n", current_cancel);
        PacketPassFairQueueFlow_RequestCancel(&flows[current_cancel]);
        PacketPassFairQueueFlow_SetBusyHandler(&flows[current_cancel], flow_handler_busy, NULL);
        return;
    }
    
    reset_input();
}

int main ()
{
    // initialize logging
    BLog_InitStdout();
    
    // init time
    BTime_Init();
    
    // initialize reactor
    if (!BReactor_Init(&reactor)) {
        DEBUG("BReactor_Init failed");
        return 1;
    }
    
    // initialize sink
    TimerPacketSink_Init(&sink, &reactor, 500, OUTPUT_INTERVAL);
    
    // initialize queue
    if (!PacketPassFairQueue_Init(&fq, TimerPacketSink_GetInput(&sink), BReactor_PendingGroup(&reactor), 1, 1)) {
        DEBUG("PacketPassFairQueue_Init failed");
        return 1;
    }
    
    // initialize inputs
    for (int i = 0; i < NUM_INPUTS; i++) {
        init_input(i);
    }
    
    // init cancel timer
    BTimer_Init(&timer, REMOVE_INTERVAL, timer_handler, NULL);
    BReactor_SetTimer(&reactor, &timer);
    
    // init cancel counter
    current_cancel = 0;
    
    // run reactor
    int ret = BReactor_Exec(&reactor);
    BReactor_Free(&reactor);
    return ret;
}
