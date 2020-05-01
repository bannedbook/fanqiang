/**
 * @file grow_array.h
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

// Preprocessor inputs:
// GROWARRAY_NAME - prefix of of functions to define
// GROWARRAY_OBJECT_TYPE - type of structure where array and capacity sizes are
// GROWARRAY_ARRAY_MEMBER - array member
// GROWARRAY_CAPACITY_MEMBER - capacity member
// GROWARRAY_MAX_CAPACITY - max value of capacity member

#include <limits.h>
#include <string.h>
#include <stddef.h>

#include <misc/debug.h>
#include <misc/balloc.h>
#include <misc/merge.h>

#define GrowArrayObject GROWARRAY_OBJECT_TYPE
#define GrowArray_Init MERGE(GROWARRAY_NAME, _Init)
#define GrowArray_InitEmpty MERGE(GROWARRAY_NAME, _InitEmpty)
#define GrowArray_Free MERGE(GROWARRAY_NAME, _Free)
#define GrowArray_DoubleUp MERGE(GROWARRAY_NAME, _DoubleUp)
#define GrowArray_DoubleUpLimit MERGE(GROWARRAY_NAME, _DoubleUpLimit)

static int GrowArray_Init (GrowArrayObject *o, size_t capacity) WARN_UNUSED;
static void GrowArray_InitEmpty (GrowArrayObject *o);
static void GrowArray_Free (GrowArrayObject *o);
static int GrowArray_DoubleUp (GrowArrayObject *o) WARN_UNUSED;
static int GrowArray_DoubleUpLimit (GrowArrayObject *o, size_t limit) WARN_UNUSED;

static int GrowArray_Init (GrowArrayObject *o, size_t capacity)
{
    if (capacity > GROWARRAY_MAX_CAPACITY) {
        return 0;
    }
    
    if (capacity == 0) {
        o->GROWARRAY_ARRAY_MEMBER = NULL;
    } else {
        if (!(o->GROWARRAY_ARRAY_MEMBER = BAllocArray(capacity, sizeof(o->GROWARRAY_ARRAY_MEMBER[0])))) {
            return 0;
        }
    }
    
    o->GROWARRAY_CAPACITY_MEMBER = capacity;
    
    return 1;
}

static void GrowArray_InitEmpty (GrowArrayObject *o)
{
    o->GROWARRAY_ARRAY_MEMBER = NULL;
    o->GROWARRAY_CAPACITY_MEMBER = 0;
}

static void GrowArray_Free (GrowArrayObject *o)
{
    if (o->GROWARRAY_ARRAY_MEMBER) {
        BFree(o->GROWARRAY_ARRAY_MEMBER);
    }
}

static int GrowArray_DoubleUp (GrowArrayObject *o)
{
    return GrowArray_DoubleUpLimit(o, SIZE_MAX);
}

static int GrowArray_DoubleUpLimit (GrowArrayObject *o, size_t limit)
{
    if (o->GROWARRAY_CAPACITY_MEMBER > SIZE_MAX / 2 || o->GROWARRAY_CAPACITY_MEMBER > GROWARRAY_MAX_CAPACITY / 2) {
        return 0;
    }
    
    size_t newcap = 2 * o->GROWARRAY_CAPACITY_MEMBER;
    if (newcap == 0) {
        newcap = 1;
    }
    
    if (newcap > limit) {
        newcap = limit;
        if (newcap == o->GROWARRAY_CAPACITY_MEMBER) {
            return 0;
        }
    }
    
    void *newarr = BAllocArray(newcap, sizeof(o->GROWARRAY_ARRAY_MEMBER[0]));
    if (!newarr) {
        return 0;
    }
    
    memcpy(newarr, o->GROWARRAY_ARRAY_MEMBER, o->GROWARRAY_CAPACITY_MEMBER * sizeof(o->GROWARRAY_ARRAY_MEMBER[0]));
    
    BFree(o->GROWARRAY_ARRAY_MEMBER);
    
    o->GROWARRAY_ARRAY_MEMBER = newarr;
    o->GROWARRAY_CAPACITY_MEMBER = newcap;
    
    return 1;
}

#undef GROWARRAY_NAME
#undef GROWARRAY_OBJECT_TYPE
#undef GROWARRAY_ARRAY_MEMBER
#undef GROWARRAY_CAPACITY_MEMBER
#undef GROWARRAY_MAX_CAPACITY

#undef GrowArrayObject
#undef GrowArray_Init
#undef GrowArray_InitEmpty
#undef GrowArray_Free
#undef GrowArray_DoubleUp
#undef GrowArray_DoubleUpLimit
