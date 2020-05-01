/**
 * @file exparray.h
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
 * Dynamic array which grows exponentionally on demand.
 */

#ifndef BADVPN_MISC_EXPARRAY_H
#define BADVPN_MISC_EXPARRAY_H

#include <stddef.h>
#include <stdlib.h>
#include <limits.h>

#include <misc/debug.h>

struct ExpArray {
    size_t esize;
    size_t size;
    void *v;
};

static int ExpArray_init (struct ExpArray *o, size_t esize, size_t size)
{
    ASSERT(esize > 0)
    ASSERT(size > 0)
    
    o->esize = esize;
    o->size = size;
    
    if (o->size > SIZE_MAX / o->esize) {
        return 0;
    }
    
    if (!(o->v = malloc(o->size * o->esize))) {
        return 0;
    }
    
    return 1;
}

static int ExpArray_resize (struct ExpArray *o, size_t size)
{
    ASSERT(size > 0)
    
    if (size <= o->size) {
        return 1;
    }
    
    size_t newsize = o->size;
    
    while (newsize < size) {
        if (2 > SIZE_MAX / newsize) {
            return 0;
        }
        
        newsize = 2 * newsize;
    }
    
    if (newsize > SIZE_MAX / o->esize) {
        return 0;
    }
    
    void *newarr = realloc(o->v, newsize * o->esize);
    if (!newarr) {
        return 0;
    }
    
    o->size = newsize;
    o->v = newarr;
    
    return 1;
}

#endif
