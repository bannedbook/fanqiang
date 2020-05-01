/**
 * @file BEventLock.h
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
 * A FIFO lock for events using the job queue ({@link BPending}).
 */

#ifndef BADVPN_BEVENTLOCK_H
#define BADVPN_BEVENTLOCK_H

#include <misc/debugcounter.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <base/BPending.h>

/**
 * Event context handler called when the lock job has acquired the lock
 * after requesting the lock with {@link BEventLockJob_Wait}.
 * The object was in waiting state.
 * The object enters locked state before the handler is called.
 * 
 * @param user as in {@link BEventLockJob_Init}
 */
typedef void (*BEventLock_handler) (void *user);

/**
 * A FIFO lock for events using the job queue ({@link BPending}).
 */
typedef struct {
    LinkedList1 jobs;
    BPending exec_job;
    DebugObject d_obj;
    DebugCounter pending_ctr;
} BEventLock;

/**
 * An object that can request a {@link BEventLock} lock.
 */
typedef struct {
    BEventLock *l;
    BEventLock_handler handler;
    void *user;
    int pending;
    LinkedList1Node pending_node;
    DebugObject d_obj;
} BEventLockJob;

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param pg pending group
 */
void BEventLock_Init (BEventLock *o, BPendingGroup *pg);

/**
 * Frees the object.
 * There must be no {@link BEventLockJob} objects using this lock
 * (regardless of their state).
 * 
 * @param o the object
 */
void BEventLock_Free (BEventLock *o);

/**
 * Initializes the object.
 * The object is initialized in idle state.
 * 
 * @param o the object
 * @param l the lock
 * @param handler handler to call when the lock is aquired
 * @param user value to pass to handler
 */
void BEventLockJob_Init (BEventLockJob *o, BEventLock *l, BEventLock_handler handler, void *user);

/**
 * Frees the object.
 * 
 * @param o the object
 */
void BEventLockJob_Free (BEventLockJob *o);

/**
 * Requests the lock.
 * The object must be in idle state.
 * The object enters waiting state.
 * 
 * @param o the object
 */
void BEventLockJob_Wait (BEventLockJob *o);

/**
 * Aborts the lock request or releases the lock.
 * The object must be in waiting or locked state.
 * The object enters idle state.
 * 
 * @param o the object
 */
void BEventLockJob_Release (BEventLockJob *o);

#endif
