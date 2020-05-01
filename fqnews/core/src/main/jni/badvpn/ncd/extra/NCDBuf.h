/**
 * @file NCDBuf.h
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

#ifndef NCD_NCDBUF_H
#define NCD_NCDBUF_H

#include <stddef.h>

#include <misc/BRefTarget.h>
#include <structure/LinkedList0.h>
#include <base/DebugObject.h>

typedef struct {
    size_t buf_size;
    LinkedList0 used_bufs_list;
    LinkedList0 free_bufs_list;
    DebugObject d_obj;
} NCDBufStore;

typedef struct {
    NCDBufStore *store;
    LinkedList0Node list_node;
    BRefTarget ref_target;
    char data[];
} NCDBuf;

void NCDBufStore_Init (NCDBufStore *o, size_t buf_size);
void NCDBufStore_Free (NCDBufStore *o);
size_t NCDBufStore_BufSize (NCDBufStore *o);
NCDBuf * NCDBufStore_GetBuf (NCDBufStore *o);

BRefTarget * NCDBuf_RefTarget (NCDBuf *o);
char * NCDBuf_Data (NCDBuf *o);

#endif
