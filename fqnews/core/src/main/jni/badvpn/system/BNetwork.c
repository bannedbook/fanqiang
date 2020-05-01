/**
 * @file BNetwork.c
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
#include <winsock2.h>
#include <mswsock.h>
#else
#include <string.h>
#include <signal.h>
#endif

#include <misc/debug.h>
#include <base/BLog.h>

#include <system/BNetwork.h>

#include <generated/blog_channel_BNetwork.h>

extern int bnetwork_initialized;

#ifndef BADVPN_PLUGIN
int bnetwork_initialized = 0;
#endif

int BNetwork_GlobalInit (void)
{
    ASSERT(!bnetwork_initialized)
    
#ifdef BADVPN_USE_WINAPI
    
    WORD requested = MAKEWORD(2, 2);
    WSADATA wsadata;
    if (WSAStartup(requested, &wsadata) != 0) {
        BLog(BLOG_ERROR, "WSAStartup failed");
        goto fail0;
    }
    if (wsadata.wVersion != requested) {
        BLog(BLOG_ERROR, "WSAStartup returned wrong version");
        goto fail1;
    }
    
#else
    
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGPIPE, &act, NULL) < 0) {
        BLog(BLOG_ERROR, "sigaction failed");
        goto fail0;
    }
    
#endif
    
    bnetwork_initialized = 1;
    
    return 1;
    
#ifdef BADVPN_USE_WINAPI
fail1:
    WSACleanup();
#endif
    
fail0:
    return 0;
}

void BNetwork_Assert (void)
{
    ASSERT(bnetwork_initialized)
}
