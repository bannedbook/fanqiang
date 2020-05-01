/**
 * @file BInputProcess.h
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

#ifndef BADVPN_BINPUTPROCESS_H
#define BADVPN_BINPUTPROCESS_H

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <system/BConnection.h>
#include <system/BProcess.h>

typedef void (*BInputProcess_handler_terminated) (void *user, int normally, uint8_t normally_exit_status);
typedef void (*BInputProcess_handler_closed) (void *user, int is_error);

typedef struct {
    BReactor *reactor;
    BProcessManager *manager;
    void *user;
    BInputProcess_handler_terminated handler_terminated;
    BInputProcess_handler_closed handler_closed;
    int pipe_write_fd;
    int started;
    int have_process;
    BProcess process;
    int pipe_fd;
    BConnection pipe_con;
    DebugObject d_obj;
} BInputProcess;

int BInputProcess_Init (BInputProcess *o, BReactor *reactor, BProcessManager *manager, void *user,
                        BInputProcess_handler_terminated handler_terminated,
                        BInputProcess_handler_closed handler_closed) WARN_UNUSED;
void BInputProcess_Free (BInputProcess *o);
int BInputProcess_Start (BInputProcess *o, const char *file, char *const argv[], const char *username);
int BInputProcess_Terminate (BInputProcess *o);
int BInputProcess_Kill (BInputProcess *o);
StreamRecvInterface * BInputProcess_GetInput (BInputProcess *o);

#endif
