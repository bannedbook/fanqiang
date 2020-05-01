/**
 * @file BPending.h
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
 * Module for managing a queue of jobs pending execution.
 */

#ifndef BADVPN_BPENDING_H
#define BADVPN_BPENDING_H

#include <stdint.h>

#include <misc/debugcounter.h>
#include <structure/SLinkedList.h>
#include <base/DebugObject.h>

struct BSmallPending_s;

#include "BPending_list.h"
#include <structure/SLinkedList_decl.h>

/**
 * Job execution handler.
 * It is guaranteed that the associated {@link BSmallPending} object was
 * in set state.
 * The {@link BSmallPending} object enters not set state before the handler
 * is called.
 * 
 * @param user as in {@link BSmallPending_Init}
 */
typedef void (*BSmallPending_handler) (void *user);

/**
 * Job execution handler.
 * It is guaranteed that the associated {@link BPending} object was
 * in set state.
 * The {@link BPending} object enters not set state before the handler
 * is called.
 * 
 * @param user as in {@link BPending_Init}
 */
typedef void (*BPending_handler) (void *user);

/**
 * Object that contains a list of jobs pending execution.
 */
typedef struct {
    BPending__List jobs;
    DebugCounter pending_ctr;
    DebugObject d_obj;
} BPendingGroup;

/**
 * Object for queuing a job for execution.
 */
typedef struct BSmallPending_s {
    BPending_handler handler;
    void *user;
    BPending__ListNode pending_node; // optimization: if not pending, .next is this
#ifndef NDEBUG
    uint8_t pending;
#endif
    DebugObject d_obj;
} BSmallPending;

/**
 * Object for queuing a job for execution. This is a convenience wrapper
 * around {@link BSmallPending} with an extra field to remember the
 * {@link BPendingGroup} being used.
 */
typedef struct {
    BSmallPending base;
    BPendingGroup *g;
} BPending;

/**
 * Initializes the object.
 * 
 * @param g the object
 */
void BPendingGroup_Init (BPendingGroup *g);

/**
 * Frees the object.
 * There must be no {@link BPending} or {@link BSmallPending} objects using
 * this group.
 * 
 * @param g the object
 */
void BPendingGroup_Free (BPendingGroup *g);

/**
 * Checks if there is at least one job in the queue.
 * 
 * @param g the object
 * @return 1 if there is at least one job, 0 if not
 */
int BPendingGroup_HasJobs (BPendingGroup *g);

/**
 * Executes the top job on the job list.
 * The job is removed from the list and enters
 * not set state before being executed.
 * There must be at least one job in job list.
 * 
 * @param g the object
 */
void BPendingGroup_ExecuteJob (BPendingGroup *g);

/**
 * Returns the top job on the job list, or NULL if there are none.
 * 
 * @param g the object
 * @return the top job if there is at least one job, NULL if not
 */
BSmallPending * BPendingGroup_PeekJob (BPendingGroup *g);

/**
 * Initializes the object.
 * The object is initialized in not set state.
 * 
 * @param o the object
 * @param g pending group to use
 * @param handler job execution handler
 * @param user value to pass to handler
 */
void BSmallPending_Init (BSmallPending *o, BPendingGroup *g, BSmallPending_handler handler, void *user);

/**
 * Frees the object.
 * The execution handler will not be called after the object
 * is freed.
 * 
 * @param o the object
 * @param g pending group. Must be the same as was used in {@link BSmallPending_Init}.
 */
void BSmallPending_Free (BSmallPending *o, BPendingGroup *g);

/**
 * Changes the job execution handler.
 * 
 * @param o the object
 * @param handler job execution handler
 * @param user value to pass to handler
 */
void BSmallPending_SetHandler (BSmallPending *o, BSmallPending_handler handler, void *user);

/**
 * Enables the job, pushing it to the top of the job list.
 * If the object was already in set state, the job is removed from its
 * current position in the list before being pushed.
 * The object enters set state.
 * 
 * @param o the object
 * @param g pending group. Must be the same as was used in {@link BSmallPending_Init}.
 */
void BSmallPending_Set (BSmallPending *o, BPendingGroup *g);

/**
 * Disables the job, removing it from the job list.
 * If the object was not in set state, nothing is done.
 * The object enters not set state.
 * 
 * @param o the object
 * @param g pending group. Must be the same as was used in {@link BSmallPending_Init}.
 */
void BSmallPending_Unset (BSmallPending *o, BPendingGroup *g);

/**
 * Checks if the job is in set state.
 * 
 * @param o the object
 * @return 1 if in set state, 0 if not
 */
int BSmallPending_IsSet (BSmallPending *o);

/**
 * Initializes the object.
 * The object is initialized in not set state.
 * 
 * @param o the object
 * @param g pending group to use
 * @param handler job execution handler
 * @param user value to pass to handler
 */
void BPending_Init (BPending *o, BPendingGroup *g, BPending_handler handler, void *user);

/**
 * Frees the object.
 * The execution handler will not be called after the object
 * is freed.
 * 
 * @param o the object
 */
void BPending_Free (BPending *o);

/**
 * Enables the job, pushing it to the top of the job list.
 * If the object was already in set state, the job is removed from its
 * current position in the list before being pushed.
 * The object enters set state.
 * 
 * @param o the object
 */
void BPending_Set (BPending *o);

/**
 * Disables the job, removing it from the job list.
 * If the object was not in set state, nothing is done.
 * The object enters not set state.
 * 
 * @param o the object
 */
void BPending_Unset (BPending *o);

/**
 * Checks if the job is in set state.
 * 
 * @param o the object
 * @return 1 if in set state, 0 if not
 */
int BPending_IsSet (BPending *o);

#endif
