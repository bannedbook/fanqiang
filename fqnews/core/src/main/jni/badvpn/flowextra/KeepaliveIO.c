/**
 * @file KeepaliveIO.c
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

#include "KeepaliveIO.h"

static void keepalive_handler (KeepaliveIO *o)
{
    DebugObject_Access(&o->d_obj);
    
    PacketRecvBlocker_AllowBlockedPacket(&o->ka_blocker);
}

int KeepaliveIO_Init (KeepaliveIO *o, BReactor *reactor, PacketPassInterface *output, PacketRecvInterface *keepalive_input, btime_t keepalive_interval_ms)
{
    ASSERT(PacketRecvInterface_GetMTU(keepalive_input) <= PacketPassInterface_GetMTU(output))
    ASSERT(keepalive_interval_ms > 0)
    
    // set arguments
    o->reactor = reactor;
    
    // init keep-alive sender
    PacketPassInactivityMonitor_Init(&o->kasender, output, o->reactor, keepalive_interval_ms, (PacketPassInactivityMonitor_handler)keepalive_handler, o);
    
    // init queue
    PacketPassPriorityQueue_Init(&o->queue, PacketPassInactivityMonitor_GetInput(&o->kasender), BReactor_PendingGroup(o->reactor), 0);
    
    // init keepalive flow
    PacketPassPriorityQueueFlow_Init(&o->ka_qflow, &o->queue, -1);
    
    // init keepalive blocker
    PacketRecvBlocker_Init(&o->ka_blocker, keepalive_input, BReactor_PendingGroup(reactor));
    
    // init keepalive buffer
    if (!SinglePacketBuffer_Init(&o->ka_buffer, PacketRecvBlocker_GetOutput(&o->ka_blocker), PacketPassPriorityQueueFlow_GetInput(&o->ka_qflow), BReactor_PendingGroup(o->reactor))) {
        goto fail1;
    }
    
    // init user flow
    PacketPassPriorityQueueFlow_Init(&o->user_qflow, &o->queue, 0);
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail1:
    PacketRecvBlocker_Free(&o->ka_blocker);
    PacketPassPriorityQueueFlow_Free(&o->ka_qflow);
    PacketPassPriorityQueue_Free(&o->queue);
    PacketPassInactivityMonitor_Free(&o->kasender);
    return 0;
}

void KeepaliveIO_Free (KeepaliveIO *o)
{
    DebugObject_Free(&o->d_obj);

    // allow freeing queue flows
    PacketPassPriorityQueue_PrepareFree(&o->queue);
    
    // free user flow
    PacketPassPriorityQueueFlow_Free(&o->user_qflow);
    
    // free keepalive buffer
    SinglePacketBuffer_Free(&o->ka_buffer);
    
    // free keepalive blocker
    PacketRecvBlocker_Free(&o->ka_blocker);
    
    // free keepalive flow
    PacketPassPriorityQueueFlow_Free(&o->ka_qflow);
    
    // free queue
    PacketPassPriorityQueue_Free(&o->queue);
    
    // free keep-alive sender
    PacketPassInactivityMonitor_Free(&o->kasender);
}

PacketPassInterface * KeepaliveIO_GetInput (KeepaliveIO *o)
{
    DebugObject_Access(&o->d_obj);
    
    return PacketPassPriorityQueueFlow_GetInput(&o->user_qflow);
}
