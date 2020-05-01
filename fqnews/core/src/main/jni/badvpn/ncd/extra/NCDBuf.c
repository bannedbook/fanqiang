/**
 * @file NCDBuf.c
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

#include "NCDBuf.h"

#include <misc/balloc.h>
#include <misc/offset.h>
#include <misc/debug.h>

static void ref_target_func_release (BRefTarget *ref_target)
{
    NCDBuf *o = UPPER_OBJECT(ref_target, NCDBuf, ref_target);
    NCDBufStore *store = o->store;
    
    if (store) {
        LinkedList0_Remove(&store->used_bufs_list, &o->list_node);
        LinkedList0_Prepend(&store->free_bufs_list, &o->list_node);
    } else {
        BFree(o);
    }
}

void NCDBufStore_Init (NCDBufStore *o, size_t buf_size)
{
    o->buf_size = buf_size;
    LinkedList0_Init(&o->used_bufs_list);
    LinkedList0_Init(&o->free_bufs_list);
    
    DebugObject_Init(&o->d_obj);
}

void NCDBufStore_Free (NCDBufStore *o)
{
    DebugObject_Free(&o->d_obj);
    
    LinkedList0Node *ln;
    
    ln = LinkedList0_GetFirst(&o->used_bufs_list);
    while (ln) {
        NCDBuf *buf = UPPER_OBJECT(ln, NCDBuf, list_node);
        ASSERT(buf->store == o)
        buf->store = NULL;
        ln = LinkedList0Node_Next(ln);
    }
    
    ln = LinkedList0_GetFirst(&o->free_bufs_list);
    while (ln) {
        LinkedList0Node *next_ln = LinkedList0Node_Next(ln);
        NCDBuf *buf = UPPER_OBJECT(ln, NCDBuf, list_node);
        ASSERT(buf->store == o)
        BFree(buf);
        ln = next_ln;
    }
}

size_t NCDBufStore_BufSize (NCDBufStore *o)
{
    DebugObject_Access(&o->d_obj);
    
    return o->buf_size;
}

NCDBuf * NCDBufStore_GetBuf (NCDBufStore *o)
{
    DebugObject_Access(&o->d_obj);
    
    NCDBuf *buf;
    
    LinkedList0Node *ln = LinkedList0_GetFirst(&o->free_bufs_list);
    if (ln) {
        buf = UPPER_OBJECT(ln, NCDBuf, list_node);
        ASSERT(buf->store == o)
        LinkedList0_Remove(&o->free_bufs_list, &buf->list_node);
    } else {
        bsize_t size = bsize_add(bsize_fromsize(sizeof(NCDBuf)), bsize_fromsize(o->buf_size));
        buf = BAllocSize(size);
        if (!buf) {
            return NULL;
        }
        buf->store = o;
    }
    
    LinkedList0_Prepend(&o->used_bufs_list, &buf->list_node);
    BRefTarget_Init(&buf->ref_target, ref_target_func_release);
    
    return buf;
}

BRefTarget * NCDBuf_RefTarget (NCDBuf *o)
{
    return &o->ref_target;
}

char * NCDBuf_Data (NCDBuf *o)
{
    return o->data;
}
