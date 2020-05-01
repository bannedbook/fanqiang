/**
 * @file BSignal.c
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

#ifdef BADVPN_USE_WINAPI
#include <windows.h>
#else
#include <signal.h>
#include <system/BUnixSignal.h>
#endif

#include <misc/debug.h>
#include <base/BLog.h>

#include <system/BSignal.h>

#include <generated/blog_channel_BSignal.h>

static struct {
    int initialized;
    int finished;
    BReactor *reactor;
    BSignal_handler handler;
    void *user;
    #ifdef BADVPN_USE_WINAPI
    BReactorIOCPOverlapped olap;
    CRITICAL_SECTION iocp_handle_mutex;
    HANDLE iocp_handle;
    #else
    BUnixSignal signal;
    #endif
} bsignal_global = {
    0
};

#ifdef BADVPN_USE_WINAPI

static void olap_handler (void *user, int event, DWORD bytes)
{
    ASSERT(bsignal_global.initialized)
    ASSERT(!(event == BREACTOR_IOCP_EVENT_EXITING) || bsignal_global.finished)
    
    if (event == BREACTOR_IOCP_EVENT_EXITING) {
        BReactorIOCPOverlapped_Free(&bsignal_global.olap);
        return;
    }
    
    if (!bsignal_global.finished) {
        // call handler
        bsignal_global.handler(bsignal_global.user);
        return;
    }
}

static BOOL WINAPI ctrl_handler (DWORD type)
{
    EnterCriticalSection(&bsignal_global.iocp_handle_mutex);
    
    if (bsignal_global.iocp_handle) {
        PostQueuedCompletionStatus(bsignal_global.iocp_handle, 0, 0, &bsignal_global.olap.olap);
    }
    
    LeaveCriticalSection(&bsignal_global.iocp_handle_mutex);
    
    return TRUE;
}

#else

static void unix_signal_handler (void *user, int signo)
{
    ASSERT(signo == SIGTERM || signo == SIGINT || signo == SIGHUP)
    ASSERT(bsignal_global.initialized)
    ASSERT(!bsignal_global.finished)
    
    BLog(BLOG_DEBUG, "Dispatching signal");
    
    // call handler
    bsignal_global.handler(bsignal_global.user);
    return;
}

#endif

int BSignal_Init (BReactor *reactor, BSignal_handler handler, void *user) 
{
    ASSERT(!bsignal_global.initialized)
    
    // init arguments
    bsignal_global.reactor = reactor;
    bsignal_global.handler = handler;
    bsignal_global.user = user;
    
    BLog(BLOG_DEBUG, "BSignal initializing");
    
    #ifdef BADVPN_USE_WINAPI
    
    // init olap
    BReactorIOCPOverlapped_Init(&bsignal_global.olap, bsignal_global.reactor, NULL, olap_handler);
    
    // init handler mutex
    InitializeCriticalSection(&bsignal_global.iocp_handle_mutex);
    
    // remember IOCP handle
    bsignal_global.iocp_handle = BReactor_GetIOCPHandle(bsignal_global.reactor);
    
    // configure ctrl handler
    if (!SetConsoleCtrlHandler(ctrl_handler, TRUE)) {
        BLog(BLOG_ERROR, "SetConsoleCtrlHandler failed");
        goto fail1;
    }
    
    #else
    
    sigset_t sset;
    ASSERT_FORCE(sigemptyset(&sset) == 0)
    ASSERT_FORCE(sigaddset(&sset, SIGTERM) == 0)
    ASSERT_FORCE(sigaddset(&sset, SIGINT) == 0)
    ASSERT_FORCE(sigaddset(&sset, SIGHUP) == 0)
    
    // init BUnixSignal
    if (!BUnixSignal_Init(&bsignal_global.signal, bsignal_global.reactor, sset, unix_signal_handler, NULL)) {
        BLog(BLOG_ERROR, "BUnixSignal_Init failed");
        goto fail0;
    }
    
    #endif
    
    bsignal_global.initialized = 1;
    bsignal_global.finished = 0;
    
    return 1;
    
    #ifdef BADVPN_USE_WINAPI
fail1:
    DeleteCriticalSection(&bsignal_global.iocp_handle_mutex);
    BReactorIOCPOverlapped_Free(&bsignal_global.olap);
    #endif
    
fail0:
    return 0;
}

void BSignal_Finish (void)
{
    ASSERT(bsignal_global.initialized)
    ASSERT(!bsignal_global.finished)
    
    #ifdef BADVPN_USE_WINAPI
    
    // forget IOCP handle
    EnterCriticalSection(&bsignal_global.iocp_handle_mutex);
    bsignal_global.iocp_handle = NULL;
    LeaveCriticalSection(&bsignal_global.iocp_handle_mutex);
    
    #else
    
    // free BUnixSignal
    BUnixSignal_Free(&bsignal_global.signal, 0);
    
    #endif
    
    bsignal_global.finished = 1;
}
