/**
 * @file RouteBuffer.c
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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <misc/offset.h>

#include <flow/RouteBuffer.h>

static struct RouteBuffer_packet * alloc_packet (int mtu)
{
    if (mtu > SIZE_MAX - sizeof(struct RouteBuffer_packet)) {
        return NULL;
    }
    
    // allocate memory
    struct RouteBuffer_packet *p = (struct RouteBuffer_packet *)malloc(sizeof(*p) + mtu);
    if (!p) {
        return NULL;
    }
    
    return p;
}

static int alloc_free_packet (RouteBuffer *o)
{
    struct RouteBuffer_packet *p = alloc_packet(o->mtu);
    if (!p) {
        return 0;
    }
    
    // add to free packets list
    LinkedList1_Append(&o->packets_free, &p->node);
    
    return 1;
}

static void free_free_packets (RouteBuffer *o)
{
    while (!LinkedList1_IsEmpty(&o->packets_free)) {
        // get packet
        struct RouteBuffer_packet *p = UPPER_OBJECT(LinkedList1_GetLast(&o->packets_free), struct RouteBuffer_packet, node);
        
        // remove from free packets list
        LinkedList1_Remove(&o->packets_free, &p->node);
        
        // free memory
        free(p);
    }
}

static void release_used_packet (RouteBuffer *o)
{
    ASSERT(!LinkedList1_IsEmpty(&o->packets_used))
    
    // get packet
    struct RouteBuffer_packet *p = UPPER_OBJECT(LinkedList1_GetFirst(&o->packets_used), struct RouteBuffer_packet, node);
    
    // remove from used packets list
    LinkedList1_Remove(&o->packets_used, &p->node);
    
    // add to free packets list
    LinkedList1_Append(&o->packets_free, &p->node);
}

static void send_used_packet (RouteBuffer *o)
{
    ASSERT(!LinkedList1_IsEmpty(&o->packets_used))
    
    // get packet
    struct RouteBuffer_packet *p = UPPER_OBJECT(LinkedList1_GetFirst(&o->packets_used), struct RouteBuffer_packet, node);
    
    // send
    PacketPassInterface_Sender_Send(o->output, (uint8_t *)(p + 1), p->len);
}

static void output_handler_done (RouteBuffer *o)
{
    ASSERT(!LinkedList1_IsEmpty(&o->packets_used))
    DebugObject_Access(&o->d_obj);
    
    // release packet
    release_used_packet(o);
    
    // send next packet if there is one
    if (!LinkedList1_IsEmpty(&o->packets_used)) {
        send_used_packet(o);
    }
}

int RouteBuffer_Init (RouteBuffer *o, int mtu, PacketPassInterface *output, int buf_size)
{
    ASSERT(mtu >= 0)
    ASSERT(PacketPassInterface_GetMTU(output) >= mtu)
    ASSERT(buf_size > 0)
    
    // init arguments
    o->mtu = mtu;
    o->output = output;
    
    // init output
    PacketPassInterface_Sender_Init(o->output, (PacketPassInterface_handler_done)output_handler_done, o);
    
    // init free packets list
    LinkedList1_Init(&o->packets_free);
    
    // init used packets list
    LinkedList1_Init(&o->packets_used);
    
    // allocate packets
    for (int i = 0; i < buf_size; i++) {
        if (!alloc_free_packet(o)) {
            goto fail1;
        }
    }
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail1:
    free_free_packets(o);
    return 0;
}

void RouteBuffer_Free (RouteBuffer *o)
{
    DebugObject_Free(&o->d_obj);
    
    // release packets so they can be freed
    while (!LinkedList1_IsEmpty(&o->packets_used)) {
        release_used_packet(o);
    }
    
    // free packets
    free_free_packets(o);
}

int RouteBuffer_GetMTU (RouteBuffer *o)
{
    DebugObject_Access(&o->d_obj);
    
    return o->mtu;
}

int RouteBufferSource_Init (RouteBufferSource *o, int mtu)
{
    ASSERT(mtu >= 0)
    
    // init arguments
    o->mtu = mtu;
    
    // allocate current packet
    if (!(o->current_packet = alloc_packet(o->mtu))) {
        goto fail0;
    }
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail0:
    return 0;
}

void RouteBufferSource_Free (RouteBufferSource *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free current packet
    free(o->current_packet);
}

uint8_t * RouteBufferSource_Pointer (RouteBufferSource *o)
{
    DebugObject_Access(&o->d_obj);
    
    return (uint8_t *)(o->current_packet + 1);
}

int RouteBufferSource_Route (RouteBufferSource *o, int len, RouteBuffer *b, int copy_offset, int copy_len)
{
    ASSERT(len >= 0)
    ASSERT(len <= o->mtu)
    ASSERT(b->mtu == o->mtu)
    ASSERT(copy_offset >= 0)
    ASSERT(copy_offset <= o->mtu)
    ASSERT(copy_len >= 0)
    ASSERT(copy_len <= o->mtu - copy_offset)
    DebugObject_Access(&b->d_obj);
    DebugObject_Access(&o->d_obj);
    
    // check if there's space in the buffer
    if (LinkedList1_IsEmpty(&b->packets_free)) {
        return 0;
    }
    
    int was_empty = LinkedList1_IsEmpty(&b->packets_used);
    
    struct RouteBuffer_packet *p = o->current_packet;
    
    // set packet length
    p->len = len;
    
    // append packet to used packets list
    LinkedList1_Append(&b->packets_used, &p->node);
    
    // get a free packet
    struct RouteBuffer_packet *np = UPPER_OBJECT(LinkedList1_GetLast(&b->packets_free), struct RouteBuffer_packet, node);
    
    // remove it from free packets list
    LinkedList1_Remove(&b->packets_free, &np->node);
    
    // make it the current packet
    o->current_packet = np;
    
    // copy packet
    if (copy_len > 0) {
        memcpy((uint8_t *)(np + 1) + copy_offset, (uint8_t *)(p + 1) + copy_offset, copy_len);
    }
    
    // start sending if required
    if (was_empty) {
        send_used_packet(b);
    }
    
    return 1;
}
