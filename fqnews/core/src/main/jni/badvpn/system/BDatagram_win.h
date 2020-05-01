/**
 * @file BDatagram_win.h
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

struct BDatagram_sys_addr {
    int len;
    union {
        struct sockaddr generic;
        struct sockaddr_in ipv4;
        struct sockaddr_in6 ipv6;
    } addr;
};

struct BDatagram_s {
    BReactor *reactor;
    void *user;
    BDatagram_handler handler;
    SOCKET sock;
    LPFN_WSASENDMSG fnWSASendMsg;
    LPFN_WSARECVMSG fnWSARecvMsg;
    int aborted;
    struct {
        BReactorIOCPOverlapped olap;
        int have_addrs;
        BAddr remote_addr;
        BIPAddr local_addr;
        int inited;
        int mtu;
        PacketPassInterface iface;
        BPending job;
        int data_len;
        uint8_t *data;
        int data_busy;
        struct BDatagram_sys_addr sysaddr;
        union {
            char in[WSA_CMSG_SPACE(sizeof(struct in_pktinfo))];
            char in6[WSA_CMSG_SPACE(sizeof(struct in6_pktinfo))];
        } cdata;
        WSAMSG msg;
    } send;
    struct {
        BReactorIOCPOverlapped olap;
        int started;
        int have_addrs;
        BAddr remote_addr;
        BIPAddr local_addr;
        int inited;
        int mtu;
        PacketRecvInterface iface;
        BPending job;
        int data_have;
        uint8_t *data;
        int data_busy;
        struct BDatagram_sys_addr sysaddr;
        union {
            char in[WSA_CMSG_SPACE(sizeof(struct in_pktinfo))];
            char in6[WSA_CMSG_SPACE(sizeof(struct in6_pktinfo))];
        } cdata;
        WSAMSG msg;
    } recv;
    DebugError d_err;
    DebugObject d_obj;
};
