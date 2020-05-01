/**
 * @file NCDUdevMonitor.h
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

#ifndef BADVPN_UDEVMONITOR_NCDUDEVMONITOR_H
#define BADVPN_UDEVMONITOR_NCDUDEVMONITOR_H

#include <misc/debug.h>
#include <misc/debugerror.h>
#include <flow/StreamRecvConnector.h>
#include <system/BInputProcess.h>
#include <udevmonitor/NCDUdevMonitorParser.h>

#define NCDUDEVMONITOR_MODE_MONITOR_UDEV 0
#define NCDUDEVMONITOR_MODE_INFO 1
#define NCDUDEVMONITOR_MODE_MONITOR_KERNEL 2

typedef void (*NCDUdevMonitor_handler_event) (void *user);
typedef void (*NCDUdevMonitor_handler_error) (void *user, int is_error);

typedef struct {
    void *user;
    NCDUdevMonitor_handler_event handler_event;
    NCDUdevMonitor_handler_error handler_error;
    BInputProcess process;
    int process_running;
    int process_was_error;
    int input_running;
    int input_was_error;
    StreamRecvConnector connector;
    NCDUdevMonitorParser parser;
    DebugObject d_obj;
    DebugError d_err;
} NCDUdevMonitor;

int NCDUdevMonitor_Init (NCDUdevMonitor *o, BReactor *reactor, BProcessManager *manager, int mode, void *user,
                         NCDUdevMonitor_handler_event handler_event,
                         NCDUdevMonitor_handler_error handler_error) WARN_UNUSED;
void NCDUdevMonitor_Free (NCDUdevMonitor *o);
void NCDUdevMonitor_Done (NCDUdevMonitor *o);
void NCDUdevMonitor_AssertReady (NCDUdevMonitor *o);
int NCDUdevMonitor_IsReadyEvent (NCDUdevMonitor *o);
int NCDUdevMonitor_GetNumProperties (NCDUdevMonitor *o);
void NCDUdevMonitor_GetProperty (NCDUdevMonitor *o, int index, const char **name, const char **value);

#endif
