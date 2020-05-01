/**
 * @file ncdudevmonitor_test.c
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

#include <string.h>

#include <system/BTime.h>
#include <base/BLog.h>
#include <system/BReactor.h>
#include <system/BSignal.h>
#include <system/BProcess.h>
#include <system/BNetwork.h>
#include <udevmonitor/NCDUdevMonitor.h>

BReactor reactor;
BProcessManager manager;
NCDUdevMonitor monitor;

static void signal_handler (void *user);
static void monitor_handler_event (void *unused);
static void monitor_handler_error (void *unused, int is_error);

int main (int argc, char **argv)
{
    int ret = 1;
    
    if (argc < 2 || (strcmp(argv[1], "monitor_udev") && strcmp(argv[1], "monitor_kernel") && strcmp(argv[1], "info"))) {
        fprintf(stderr, "Usage: %s <monitor_udev/monitor_kernel/info>\n", (argc > 0 ? argv[0] : NULL));
        goto fail0;
    }
    
    int mode;
    if (!strcmp(argv[1], "monitor_udev")) {
        mode = NCDUDEVMONITOR_MODE_MONITOR_UDEV;
    } else if (!strcmp(argv[1], "monitor_kernel")) {
        mode = NCDUDEVMONITOR_MODE_MONITOR_KERNEL;
    } else {
        mode = NCDUDEVMONITOR_MODE_INFO;
    }
    
    if (!BNetwork_GlobalInit()) {
        DEBUG("BNetwork_GlobalInit failed");
        goto fail0;
    }
    
    BTime_Init();
    
    BLog_InitStdout();
    
    if (!BReactor_Init(&reactor)) {
        DEBUG("BReactor_Init failed");
        goto fail1;
    }
    
    if (!BSignal_Init(&reactor, signal_handler, NULL)) {
        DEBUG("BSignal_Init failed");
        goto fail2;
    }
    
    if (!BProcessManager_Init(&manager, &reactor)) {
        DEBUG("BProcessManager_Init failed");
        goto fail3;
    }
    
    if (!NCDUdevMonitor_Init(&monitor, &reactor, &manager, mode, NULL,
        monitor_handler_event,
        monitor_handler_error
    )) {
        DEBUG("NCDUdevMonitor_Init failed");
        goto fail4;
    }
    
    ret = BReactor_Exec(&reactor);
    
    NCDUdevMonitor_Free(&monitor);
fail4:
    BProcessManager_Free(&manager);
fail3:
    BSignal_Finish();
fail2:
    BReactor_Free(&reactor);
fail1:
    BLog_Free();
fail0:
    DebugObjectGlobal_Finish();
    
    return ret;
}

void signal_handler (void *user)
{
    DEBUG("termination requested");
    
    BReactor_Quit(&reactor, 1);
}

void monitor_handler_event (void *unused)
{
    // accept event
    NCDUdevMonitor_Done(&monitor);
    
    if (NCDUdevMonitor_IsReadyEvent(&monitor)) {
        printf("ready\n");
        return;
    }
    
    printf("event\n");
    
    int num_props = NCDUdevMonitor_GetNumProperties(&monitor);
    for (int i = 0; i < num_props; i++) {
        const char *name;
        const char *value;
        NCDUdevMonitor_GetProperty(&monitor, i, &name, &value);
        printf("  %s=%s\n", name, value);
    }
}

void monitor_handler_error (void *unused, int is_error)
{
    if (is_error) {
        DEBUG("monitor error");
    } else {
        DEBUG("monitor finished");
    }
    
    BReactor_Quit(&reactor, (is_error ? 1 : 0));
}
