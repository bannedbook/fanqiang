/**
 * @file NCDUdevMonitor.c
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

#include <stddef.h>

#include <base/BLog.h>
#include <misc/find_program.h>

#include <udevmonitor/NCDUdevMonitor.h>

#include <generated/blog_channel_NCDUdevMonitor.h>

#define PARSER_BUF_SIZE 16384
#define PARSER_MAX_PROPERTIES 256

static void report_error (NCDUdevMonitor *o)
{
    ASSERT(!o->process_running)
    ASSERT(!o->input_running)
    
    DEBUGERROR(&o->d_err, o->handler_error(o->user, (o->process_was_error || o->input_was_error)));
}

static void process_handler_terminated (NCDUdevMonitor *o, int normally, uint8_t normally_exit_status)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->process_running)
    
    BLog(BLOG_INFO, "process terminated");
    
    // set process not running (so we don't try to kill it)
    o->process_running = 0;
    
    // remember process error
    o->process_was_error = !(normally && normally_exit_status == 0);
    
    if (!o->input_running) {
        report_error(o);
        return;
    }
}

static void process_handler_closed (NCDUdevMonitor *o, int is_error)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->input_running)
    
    if (is_error) {
        BLog(BLOG_ERROR, "pipe error");
    } else {
        BLog(BLOG_INFO, "pipe closed");
    }
    
    // disconnect connector
    StreamRecvConnector_DisconnectInput(&o->connector);
    
    // set input not running
    o->input_running = 0;
    
    // remember input error
    o->input_was_error = is_error;
    
    if (!o->process_running) {
        report_error(o);
        return;
    }
}

static void parser_handler (NCDUdevMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    
    o->handler_event(o->user);
    return;
}

int NCDUdevMonitor_Init (NCDUdevMonitor *o, BReactor *reactor, BProcessManager *manager, int mode, void *user,
                         NCDUdevMonitor_handler_event handler_event,
                         NCDUdevMonitor_handler_error handler_error)
{
    ASSERT(mode == NCDUDEVMONITOR_MODE_MONITOR_UDEV || mode == NCDUDEVMONITOR_MODE_INFO || mode == NCDUDEVMONITOR_MODE_MONITOR_KERNEL)
    
    // init arguments
    o->user = user;
    o->handler_event = handler_event;
    o->handler_error = handler_error;
    
    // find programs
    char *stdbuf_exec = badvpn_find_program("stdbuf");
    char *udevadm_exec = badvpn_find_program("udevadm");
    if (!stdbuf_exec) {
        BLog(BLOG_ERROR, "failed to find stdbuf program");
        goto fail0;
    }
    if (!udevadm_exec) {
        BLog(BLOG_ERROR, "failed to find udevadm program");
        goto fail0;
    }
    
    // construct arguments
    const char *argv_monitor_udev[] = {stdbuf_exec, "-o", "L", udevadm_exec, "monitor", "--udev", "--environment", NULL};
    const char *argv_monitor_kernel[] = {stdbuf_exec, "-o", "L", udevadm_exec, "monitor", "--kernel", "--environment", NULL};
    const char *argv_info[] = {stdbuf_exec, "-o", "L", udevadm_exec, "info", "--query", "all", "--export-db", NULL};
    
    // choose arguments based on mode
    const char **argv = NULL; // to remove warning
    switch (mode) {
        case NCDUDEVMONITOR_MODE_MONITOR_UDEV:   argv = argv_monitor_udev; break;
        case NCDUDEVMONITOR_MODE_INFO:           argv = argv_info; break;
        case NCDUDEVMONITOR_MODE_MONITOR_KERNEL: argv = argv_monitor_kernel; break;
        default: ASSERT(0);
    }
    
    // init process
    if (!BInputProcess_Init(&o->process, reactor, manager, o,
                            (BInputProcess_handler_terminated)process_handler_terminated,
                            (BInputProcess_handler_closed)process_handler_closed
    )) {
        BLog(BLOG_ERROR, "BInputProcess_Init failed");
        goto fail0;
    }
    
    // init connector
    StreamRecvConnector_Init(&o->connector, BReactor_PendingGroup(reactor));
    StreamRecvConnector_ConnectInput(&o->connector, BInputProcess_GetInput(&o->process));
    
    // init parser
    if (!NCDUdevMonitorParser_Init(&o->parser, StreamRecvConnector_GetOutput(&o->connector), PARSER_BUF_SIZE, PARSER_MAX_PROPERTIES,
                                   (mode == NCDUDEVMONITOR_MODE_INFO), BReactor_PendingGroup(reactor), o,
                                   (NCDUdevMonitorParser_handler)parser_handler
    )) {
        BLog(BLOG_ERROR, "NCDUdevMonitorParser_Init failed");
        goto fail1;
    }
    
    // start process
    if (!BInputProcess_Start(&o->process, stdbuf_exec, (char **)argv, NULL)) {
        BLog(BLOG_ERROR, "BInputProcess_Start failed");
        goto fail2;
    }
    
    // set process running, input running
    o->process_running = 1;
    o->input_running = 1;
    
    free(udevadm_exec);
    free(stdbuf_exec);
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail2:
    NCDUdevMonitorParser_Free(&o->parser);
fail1:
    StreamRecvConnector_Free(&o->connector);
    BInputProcess_Free(&o->process);
fail0:
    free(udevadm_exec);
    free(stdbuf_exec);
    return 0;
}

void NCDUdevMonitor_Free (NCDUdevMonitor *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    
    // free parser
    NCDUdevMonitorParser_Free(&o->parser);
    
    // free connector
    StreamRecvConnector_Free(&o->connector);
    
    // kill process it it's running
    if (o->process_running) {
        BInputProcess_Kill(&o->process);
    }
    
    // free process
    BInputProcess_Free(&o->process);
}

void NCDUdevMonitor_Done (NCDUdevMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    NCDUdevMonitorParser_AssertReady(&o->parser);
    
    NCDUdevMonitorParser_Done(&o->parser);
}

int NCDUdevMonitor_IsReadyEvent (NCDUdevMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    NCDUdevMonitorParser_AssertReady(&o->parser);
    
    return NCDUdevMonitorParser_IsReadyEvent(&o->parser);
}
void NCDUdevMonitor_AssertReady (NCDUdevMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    NCDUdevMonitorParser_AssertReady(&o->parser);
}

int NCDUdevMonitor_GetNumProperties (NCDUdevMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    NCDUdevMonitorParser_AssertReady(&o->parser);
    
    return NCDUdevMonitorParser_GetNumProperties(&o->parser);
}

void NCDUdevMonitor_GetProperty (NCDUdevMonitor *o, int index, const char **name, const char **value)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    NCDUdevMonitorParser_AssertReady(&o->parser);
    
    NCDUdevMonitorParser_GetProperty(&o->parser, index, name, value);
}
