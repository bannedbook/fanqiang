/**
 * @file DebugObject.h
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
 * Object used for detecting leaks.
 */

#ifndef BADVPN_DEBUGOBJECT_H
#define BADVPN_DEBUGOBJECT_H

#include <stdint.h>

#if !defined(BADVPN_THREAD_SAFE) || (BADVPN_THREAD_SAFE != 0 && BADVPN_THREAD_SAFE != 1)
#error BADVPN_THREAD_SAFE is not defined or incorrect
#endif

#if BADVPN_THREAD_SAFE
#include <pthread.h>
#endif

#include <misc/debug.h>
#include <misc/debugcounter.h>

#define DEBUGOBJECT_VALID UINT32_C(0x31415926)

/**
 * Object used for detecting leaks.
 */
typedef struct {
    #ifndef NDEBUG
    uint32_t c;
    #else
    int dummy_field; // struct must have at least one field
    #endif
} DebugObject;

/**
 * Initializes the object.
 * 
 * @param obj the object
 */
static void DebugObject_Init (DebugObject *obj);

/**
 * Frees the object.
 * 
 * @param obj the object
 */
static void DebugObject_Free (DebugObject *obj);

/**
 * Does nothing.
 * 
 * @param obj the object
 */
static void DebugObject_Access (const DebugObject *obj);

/**
 * Does nothing.
 * There must be no {@link DebugObject}'s initialized.
 */
static void DebugObjectGlobal_Finish (void);

#ifndef NDEBUG
extern DebugCounter debugobject_counter;
#if BADVPN_THREAD_SAFE
extern pthread_mutex_t debugobject_mutex;
#endif
#endif

void DebugObject_Init (DebugObject *obj)
{
    #ifndef NDEBUG
    
    obj->c = DEBUGOBJECT_VALID;
    
    #if BADVPN_THREAD_SAFE
    ASSERT_FORCE(pthread_mutex_lock(&debugobject_mutex) == 0)
    #endif
    
    DebugCounter_Increment(&debugobject_counter);
    
    #if BADVPN_THREAD_SAFE
    ASSERT_FORCE(pthread_mutex_unlock(&debugobject_mutex) == 0)
    #endif
    
    #endif
}

void DebugObject_Free (DebugObject *obj)
{
    ASSERT(obj->c == DEBUGOBJECT_VALID)
    
    #ifndef NDEBUG
    
    obj->c = 0;
    
    #if BADVPN_THREAD_SAFE
    ASSERT_FORCE(pthread_mutex_lock(&debugobject_mutex) == 0)
    #endif
    
    DebugCounter_Decrement(&debugobject_counter);
    
    #if BADVPN_THREAD_SAFE
    ASSERT_FORCE(pthread_mutex_unlock(&debugobject_mutex) == 0)
    #endif
    
    #endif
}

void DebugObject_Access (const DebugObject *obj)
{
    ASSERT(obj->c == DEBUGOBJECT_VALID)
}

void DebugObjectGlobal_Finish (void)
{
    #ifndef NDEBUG
    DebugCounter_Free(&debugobject_counter);
    #endif
}

#endif
