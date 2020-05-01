/**
 * @file bprocess_example.c
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
#include <unistd.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <base/BLog.h>
#include <system/BReactor.h>
#include <system/BUnixSignal.h>
#include <system/BTime.h>
#include <system/BProcess.h>

BReactor reactor;
BUnixSignal unixsignal;
BProcessManager manager;
BProcess process;

static void unixsignal_handler (void *user, int signo);
static void process_handler (void *user, int normally, uint8_t normally_exit_status);

int main (int argc, char **argv)
{
    if (argc <= 0) {
        return 1;
    }
    
    int ret = 1;
    
    if (argc < 2) {
        printf("Usage: %s <program> [argument ...]\n", argv[0]);
        goto fail0;
    }
    
    char *program = argv[1];
    
    // init time
    BTime_Init();
    
    // init logger
    BLog_InitStdout();
    
    // init reactor (event loop)
    if (!BReactor_Init(&reactor)) {
        DEBUG("BReactor_Init failed");
        goto fail1;
    }
    
    // choose signals to catch
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    
    // init BUnixSignal for catching signals
    if (!BUnixSignal_Init(&unixsignal, &reactor, set, unixsignal_handler, NULL)) {
        DEBUG("BUnixSignal_Init failed");
        goto fail2;
    }
    
    // init process manager
    if (!BProcessManager_Init(&manager, &reactor)) {
        DEBUG("BProcessManager_Init failed");
        goto fail3;
    }
    
    char **p_argv = argv + 1;
    
    // map fds 0, 1, 2 in child to fds 0, 1, 2 in parent
    int fds[] = { 0, 1, 2, -1 };
    int fds_map[] = { 0, 1, 2 };
    
    // start child process
    if (!BProcess_InitWithFds(&process, &manager, process_handler, NULL, program, p_argv, NULL, fds, fds_map)) {
        DEBUG("BProcess_Init failed");
        goto fail4;
    }
    
    // enter event loop
    ret = BReactor_Exec(&reactor);
    
    BProcess_Free(&process);
fail4:
    BProcessManager_Free(&manager);
fail3:
    BUnixSignal_Free(&unixsignal, 0);
fail2:
    BReactor_Free(&reactor);
fail1:
    BLog_Free();
fail0:
    DebugObjectGlobal_Finish();
    
    return ret;
}

void unixsignal_handler (void *user, int signo)
{
    DEBUG("received %s, terminating child", (signo == SIGINT ? "SIGINT" : "SIGTERM"));
    
    // send SIGTERM to child
    BProcess_Terminate(&process);
}

void process_handler (void *user, int normally, uint8_t normally_exit_status)
{
    DEBUG("process terminated");
    
    int ret = (normally ? normally_exit_status : 1);
    
    // return from event loop
    BReactor_Quit(&reactor, ret);
}
