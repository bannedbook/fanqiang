/**
 * @file BReactor_glib.h
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

#ifndef BADVPN_SYSTEM_BREACTOR_H
#define BADVPN_SYSTEM_BREACTOR_H

#include <stdint.h>

#include <glib.h>

#include <misc/debug.h>
#include <misc/debugcounter.h>
#include <misc/offset.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <base/BPending.h>
#include <system/BTime.h>

typedef struct BReactor_s BReactor;

struct BSmallTimer_t;

#define BTIMER_SET_ABSOLUTE 1
#define BTIMER_SET_RELATIVE 2

typedef void (*BSmallTimer_handler) (struct BSmallTimer_t *timer);
typedef void (*BTimer_handler) (void *user);

typedef struct BSmallTimer_t {
    union {
        BSmallTimer_handler smalll; // MSVC doesn't like "small"
        BTimer_handler heavy;
    } handler;
    BReactor *reactor;
    GSource *source;
    uint8_t active;
    uint8_t is_small;
} BSmallTimer;

void BSmallTimer_Init (BSmallTimer *bt, BSmallTimer_handler handler);
int BSmallTimer_IsRunning (BSmallTimer *bt);

typedef struct {
    BSmallTimer base;
    void *user;
    btime_t msTime;
} BTimer;

void BTimer_Init (BTimer *bt, btime_t msTime, BTimer_handler handler, void *user);
int BTimer_IsRunning (BTimer *bt);

struct BFileDescriptor_t;

#define BREACTOR_READ (1 << 0)
#define BREACTOR_WRITE (1 << 1)
#define BREACTOR_ERROR (1 << 2)
#define BREACTOR_HUP (1 << 3)

typedef void (*BFileDescriptor_handler) (void *user, int events);

typedef struct BFileDescriptor_t {
    int fd;
    BFileDescriptor_handler handler;
    void *user;
    int active;
    int waitEvents;
    BReactor *reactor;
    GSource *source;
    GPollFD pollfd;
} BFileDescriptor;

void BFileDescriptor_Init (BFileDescriptor *bs, int fd, BFileDescriptor_handler handler, void *user);

struct BReactor_s {
    int exiting;
    int exit_code;
    GMainLoop *gloop;
    int unref_gloop_on_free;
    GSourceFuncs fd_source_funcs;
    BPendingGroup pending_jobs;
    LinkedList1 active_limits_list;
    
    DebugCounter d_fds_counter;
    DebugCounter d_limits_ctr;
    DebugCounter d_timers_ctr;
    DebugObject d_obj;
};

int BReactor_Init (BReactor *bsys) WARN_UNUSED;
void BReactor_Free (BReactor *bsys);
int BReactor_Exec (BReactor *bsys);
void BReactor_Quit (BReactor *bsys, int code);
void BReactor_SetSmallTimer (BReactor *bsys, BSmallTimer *bt, int mode, btime_t time);
void BReactor_RemoveSmallTimer (BReactor *bsys, BSmallTimer *bt);
void BReactor_SetTimer (BReactor *bsys, BTimer *bt);
void BReactor_SetTimerAfter (BReactor *bsys, BTimer *bt, btime_t after);
void BReactor_SetTimerAbsolute (BReactor *bsys, BTimer *bt, btime_t time);
void BReactor_RemoveTimer (BReactor *bsys, BTimer *bt);
BPendingGroup * BReactor_PendingGroup (BReactor *bsys);
int BReactor_Synchronize (BReactor *bsys, BSmallPending *ref);
int BReactor_AddFileDescriptor (BReactor *bsys, BFileDescriptor *bs) WARN_UNUSED;
void BReactor_RemoveFileDescriptor (BReactor *bsys, BFileDescriptor *bs);
void BReactor_SetFileDescriptorEvents (BReactor *bsys, BFileDescriptor *bs, int events);

int BReactor_InitFromExistingGMainLoop (BReactor *bsys, GMainLoop *gloop, int unref_gloop_on_free);
GMainLoop * BReactor_GetGMainLoop (BReactor *bsys);
int BReactor_SynchronizeAll (BReactor *bsys);

typedef struct {
    BReactor *reactor;
    int limit;
    int count;
    LinkedList1Node active_limits_list_node;
    DebugObject d_obj;
} BReactorLimit;

void BReactorLimit_Init (BReactorLimit *o, BReactor *reactor, int limit);
void BReactorLimit_Free (BReactorLimit *o);
int BReactorLimit_Increment (BReactorLimit *o);
void BReactorLimit_SetLimit (BReactorLimit *o, int limit);

#endif
