/**
 * @file BConnection_win.h
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

#include <windows.h>
#include <winsock2.h>
#ifdef BADVPN_USE_SHIPPED_MSWSOCK
#    include <misc/mswsock.h>
#else
#    include <mswsock.h>
#endif

#include <misc/debugerror.h>
#include <base/DebugObject.h>

struct BListener_addrbuf_stub {
    union {
        struct sockaddr_in ipv4;
        struct sockaddr_in6 ipv6;
    } addr;
    uint8_t extra[16];
};

struct BListener_s {
    BReactor *reactor;
    void *user;
    BListener_handler handler;
    int sys_family;
    SOCKET sock;
    LPFN_ACCEPTEX fnAcceptEx;
    LPFN_GETACCEPTEXSOCKADDRS fnGetAcceptExSockaddrs;
    BReactorIOCPOverlapped olap;
    SOCKET newsock;
    uint8_t addrbuf[2 * sizeof(struct BListener_addrbuf_stub)];
    BPending next_job;
    int busy;
    int ready;
    DebugObject d_obj;
};

struct BConnector_s {
    BReactor *reactor;
    void *user;
    BConnector_handler handler;
    SOCKET sock;
    LPFN_CONNECTEX fnConnectEx;
    BReactorIOCPOverlapped olap;
    int busy;
    int ready;
    DebugObject d_obj;
};

struct BConnection_s {
    BReactor *reactor;
    void *user;
    BConnection_handler handler;
    SOCKET sock;
    int aborted;
    struct {
        BReactorIOCPOverlapped olap;
        int inited;
        StreamPassInterface iface;
        int busy;
        int busy_data_len;
    } send;
    struct {
        BReactorIOCPOverlapped olap;
        int closed;
        int inited;
        StreamRecvInterface iface;
        int busy;
        int busy_data_len;
    } recv;
    DebugError d_err;
    DebugObject d_obj;
};
