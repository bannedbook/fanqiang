/**
 * @file debugcounter.h
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
 * Counter for detecting leaks.
 */

#ifndef BADVPN_MISC_DEBUGCOUNTER_H
#define BADVPN_MISC_DEBUGCOUNTER_H

#include <stdint.h>

#include <misc/debug.h>

/**
 * Counter for detecting leaks.
 */
typedef struct {
#ifndef NDEBUG
    int32_t c;
#else
    int dummy_field; // struct must have at least one field
#endif
} DebugCounter;

#define DEBUGCOUNTER_STATIC { 0 }

/**
 * Initializes the object.
 * The object is initialized with counter value zero.
 * 
 * @param obj the object
 */
static void DebugCounter_Init (DebugCounter *obj)
{
#ifndef NDEBUG
    obj->c = 0;
#endif
}

/**
 * Frees the object.
 * This does not have to be called when the counter is no longer needed.
 * The counter value must be zero.
 * 
 * @param obj the object
 */
static void DebugCounter_Free (DebugCounter *obj)
{
#ifndef NDEBUG
    ASSERT(obj->c == 0 || obj->c == INT32_MAX)
#endif
}

/**
 * Increments the counter.
 * Increments the counter value by one.
 * 
 * @param obj the object
 */
static void DebugCounter_Increment (DebugCounter *obj)
{
#ifndef NDEBUG
    ASSERT(obj->c >= 0)
    
    if (obj->c != INT32_MAX) {
        obj->c++;
    }
#endif
}

/**
 * Decrements the counter.
 * The counter value must be >0.
 * Decrements the counter value by one.
 * 
 * @param obj the object
 */
static void DebugCounter_Decrement (DebugCounter *obj)
{
#ifndef NDEBUG
    ASSERT(obj->c > 0)
    
    if (obj->c != INT32_MAX) {
        obj->c--;
    }
#endif
}

#endif
