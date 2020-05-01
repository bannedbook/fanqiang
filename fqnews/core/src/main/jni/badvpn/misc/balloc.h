/**
 * @file balloc.h
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
 * Memory allocation functions.
 */

#ifndef BADVPN_MISC_BALLOC_H
#define BADVPN_MISC_BALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include <misc/debug.h>
#include <misc/bsize.h>
#include <misc/maxalign.h>

/**
 * Allocates memory.
 * 
 * @param bytes number of bytes to allocate.
 * @return a non-NULL pointer to the memory, or NULL on failure.
 *         The memory allocated can be freed using {@link BFree}.
 */
static void * BAlloc (size_t bytes);

/**
 * Frees memory.
 * 
 * @param m memory to free. Must have been obtained with {@link BAlloc},
 *          {@link BAllocArray}, or {@link BAllocArray2}. May be NULL;
 *          in this case, this function does nothing.
 */
static void BFree (void *m);

/**
 * Changes the size of a memory block. On success, the memory block
 * may be moved to a different address.
 * 
 * @param m pointer to a memory block obtained from {@link BAlloc}
 *          or other functions in this group. If this is NULL, the
 *          call is equivalent to {@link BAlloc}(bytes).
 * @param bytes new size of the memory block
 * @return new pointer to the memory block, or NULL on failure
 */
static void * BRealloc (void *m, size_t bytes);

/**
 * Allocates memory, with size given as a {@link bsize_t}.
 * 
 * @param bytes number of bytes to allocate. If the size is overflow,
 *              this function will return NULL.
 * @return a non-NULL pointer to the memory, or NULL on failure.
 *         The memory allocated can be freed using {@link BFree}.
 */
static void * BAllocSize (bsize_t bytes);

/**
 * Allocates memory for an array.
 * A check is first done to make sure the multiplication doesn't overflow;
 * otherwise, this is equivalent to {@link BAlloc}(count * bytes).
 * This may be slightly faster if 'bytes' is constant, because a division
 * with 'bytes' is performed.
 * 
 * @param count number of elements.
 * @param bytes size of one array element.
 * @return a non-NULL pointer to the memory, or NULL on failure.
 *         The memory allocated can be freed using {@link BFree}.
 */
static void * BAllocArray (size_t count, size_t bytes);

/**
 * Reallocates memory that was allocated using one of the allocation
 * functions in this file. On success, the memory may be moved to a
 * different address, leaving the old address invalid.
 * 
 * @param mem pointer to an existing memory block. May be NULL, in which
 *            case this is equivalent to {@link BAllocArray}.
 * @param count number of elements for reallocation
 * @param bytes size of one array element for reallocation
 * @return a non-NULL pointer to the address of the reallocated memory
 *         block, or NULL on failure. On failure, the original memory
 *         block is left intact.
 */
static void * BReallocArray (void *mem, size_t count, size_t bytes);

/**
 * Allocates memory for a two-dimensional array.
 * 
 * Checks are first done to make sure the multiplications don't overflow;
 * otherwise, this is equivalent to {@link BAlloc}((count2 * (count1 * bytes)).
 * 
 * @param count2 number of elements in one dimension.
 * @param count1 number of elements in the other dimension.
 * @param bytes size of one array element.
 * @return a non-NULL pointer to the memory, or NULL on failure.
 *         The memory allocated can be freed using {@link BFree}.
 */
static void * BAllocArray2 (size_t count2, size_t count1, size_t bytes);

/**
 * Adds to a size_t with overflow detection.
 * 
 * @param s pointer to a size_t to add to
 * @param add number to add
 * @return 1 on success, 0 on failure
 */
static int BSizeAdd (size_t *s, size_t add);

/**
 * Aligns a size_t upwards with overflow detection.
 * 
 * @param s pointer to a size_t to align
 * @param align alignment value. Must be >0.
 * @return 1 on success, 0 on failure
 */
static int BSizeAlign (size_t *s, size_t align);

void * BAlloc (size_t bytes)
{
    if (bytes == 0) {
        return malloc(1);
    }
    
    return malloc(bytes);
}

void BFree (void *m)
{
    free(m);
}

void * BRealloc (void *m, size_t bytes)
{
    if (bytes == 0) {
        return realloc(m, 1);
    }
    
    return realloc(m, bytes);
}

void * BAllocSize (bsize_t bytes)
{
    if (bytes.is_overflow) {
        return NULL;
    }
    
    return BAlloc(bytes.value);
}

void * BAllocArray (size_t count, size_t bytes)
{
    if (count == 0 || bytes == 0) {
        return malloc(1);
    }
    
    if (count > SIZE_MAX / bytes) {
        return NULL;
    }
    
    return BAlloc(count * bytes);
}

void * BReallocArray (void *mem, size_t count, size_t bytes)
{
    if (count == 0 || bytes == 0) {
        return realloc(mem, 1);
    }
    
    if (count > SIZE_MAX / bytes) {
        return NULL;
    }
    
    return realloc(mem, count * bytes);
}

void * BAllocArray2 (size_t count2, size_t count1, size_t bytes)
{
    if (count2 == 0 || count1 == 0 || bytes == 0) {
        return malloc(1);
    }
    
    if (count1 > SIZE_MAX / bytes) {
        return NULL;
    }
    
    if (count2 > SIZE_MAX / (count1 * bytes)) {
        return NULL;
    }
    
    return BAlloc(count2 * (count1 * bytes));
}

int BSizeAdd (size_t *s, size_t add)
{
    ASSERT(s)
    
    if (add > SIZE_MAX - *s) {
        return 0;
    }
    *s += add;
    return 1;
}

int BSizeAlign (size_t *s, size_t align)
{
    ASSERT(s)
    ASSERT(align > 0)
    
    size_t mod = *s % align;
    if (mod > 0) {
        if (align - mod > SIZE_MAX - *s) {
            return 0;
        }
        *s += align - mod;
    }
    return 1;
}

#endif
