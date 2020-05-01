/**
 * @file BReactor_emscripten.h
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

#include <base/DebugObject.h>
#include <base/BPending.h>
#include <system/BTime.h>

#define BTIMER_SET_RELATIVE 2

typedef struct BReactor_s BReactor;

typedef void (*BTimer_handler) (void *user);

typedef struct BTimer_t {
    btime_t msTime;
    BTimer_handler handler;
    void *handler_pointer;
    uint8_t active;
    int timerid;
    BReactor *reactor;
} BTimer;

void BTimer_Init (BTimer *bt, btime_t msTime, BTimer_handler handler, void *user);
int BTimer_IsRunning (BTimer *bt);

struct BSmallTimer_t;

typedef void (*BSmallTimer_handler) (struct BSmallTimer_t *timer);

typedef struct BSmallTimer_t {
    BTimer timer;
    BSmallTimer_handler handler;
} BSmallTimer;

void BSmallTimer_Init (BSmallTimer *bt, BSmallTimer_handler handler);
int BSmallTimer_IsRunning (BSmallTimer *bt);

struct BReactor_s {
    BPendingGroup pending_jobs;
    DebugObject d_obj;
};

void BReactor_EmscriptenInit (BReactor *bsys);
void BReactor_EmscriptenFree (BReactor *bsys);
void BReactor_EmscriptenSync (BReactor *bsys);

BPendingGroup * BReactor_PendingGroup (BReactor *bsys);

void BReactor_SetTimer (BReactor *bsys, BTimer *bt);
void BReactor_SetTimerAfter (BReactor *bsys, BTimer *bt, btime_t after);
void BReactor_RemoveTimer (BReactor *bsys, BTimer *bt);

void BReactor_SetSmallTimer (BReactor *bsys, BSmallTimer *bt, int mode, btime_t time);
void BReactor_RemoveSmallTimer (BReactor *bsys, BSmallTimer *bt);

#endif
