/**
 * @file fairqueue_test2.c
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

#include <misc/debug.h>
#include <system/BReactor.h>
#include <base/BLog.h>
#include <system/BTime.h>
#include <flow/PacketPassFairQueue.h>
#include <examples/FastPacketSource.h>
#include <examples/RandomPacketSink.h>

#define SINK_TIMER 0

int main ()
{
    // initialize logging
    BLog_InitStdout();
    
    // init time
    BTime_Init();
    
    // initialize reactor
    BReactor reactor;
    if (!BReactor_Init(&reactor)) {
        DEBUG("BReactor_Init failed");
        return 1;
    }
    
    // initialize sink
    RandomPacketSink sink;
    RandomPacketSink_Init(&sink, &reactor, 500, SINK_TIMER);
    
    // initialize queue
    PacketPassFairQueue fq;
    if (!PacketPassFairQueue_Init(&fq, RandomPacketSink_GetInput(&sink), BReactor_PendingGroup(&reactor), 0, 1)) {
        DEBUG("PacketPassFairQueue_Init failed");
        return 1;
    }
    
    // initialize source 1
    PacketPassFairQueueFlow flow1;
    PacketPassFairQueueFlow_Init(&flow1, &fq);
    FastPacketSource source1;
    char data1[] = "data1";
    FastPacketSource_Init(&source1, PacketPassFairQueueFlow_GetInput(&flow1), (uint8_t *)data1, strlen(data1), BReactor_PendingGroup(&reactor));
    
    // initialize source 2
    PacketPassFairQueueFlow flow2;
    PacketPassFairQueueFlow_Init(&flow2, &fq);
    FastPacketSource source2;
    char data2[] = "data2data2";
    FastPacketSource_Init(&source2, PacketPassFairQueueFlow_GetInput(&flow2), (uint8_t *)data2, strlen(data2), BReactor_PendingGroup(&reactor));
    
    // initialize source 3
    PacketPassFairQueueFlow flow3;
    PacketPassFairQueueFlow_Init(&flow3, &fq);
    FastPacketSource source3;
    char data3[] = "data3data3data3data3data3data3data3data3data3";
    FastPacketSource_Init(&source3, PacketPassFairQueueFlow_GetInput(&flow3), (uint8_t *)data3, strlen(data3), BReactor_PendingGroup(&reactor));
    
    // run reactor
    int ret = BReactor_Exec(&reactor);
    BReactor_Free(&reactor);
    return ret;
}
