/**
 * @file BDatagram_win.c
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

#include <stdlib.h>

#include <base/BLog.h>

#include "BDatagram.h"

#include <generated/blog_channel_BDatagram.h>

static int family_socket_to_sys (int family);
static void addr_socket_to_sys (struct BDatagram_sys_addr *out, BAddr addr);
static void addr_sys_to_socket (BAddr *out, struct BDatagram_sys_addr addr);
static void set_pktinfo (SOCKET sock, int family);
static void report_error (BDatagram *o);
static void datagram_abort (BDatagram *o);
static void start_send (BDatagram *o);
static void start_recv (BDatagram *o);
static void send_job_handler (BDatagram *o);
static void recv_job_handler (BDatagram *o);
static void send_if_handler_send (BDatagram *o, uint8_t *data, int data_len);
static void recv_if_handler_recv (BDatagram *o, uint8_t *data);
static void send_olap_handler (BDatagram *o, int event, DWORD bytes);
static void recv_olap_handler (BDatagram *o, int event, DWORD bytes);

static int family_socket_to_sys (int family)
{
    switch (family) {
        case BADDR_TYPE_IPV4:
            return AF_INET;
        case BADDR_TYPE_IPV6:
            return AF_INET6;
    }
    
    ASSERT(0);
    return 0;
}

static void addr_socket_to_sys (struct BDatagram_sys_addr *out, BAddr addr)
{
    switch (addr.type) {
        case BADDR_TYPE_IPV4: {
            out->len = sizeof(out->addr.ipv4);
            memset(&out->addr.ipv4, 0, sizeof(out->addr.ipv4));
            out->addr.ipv4.sin_family = AF_INET;
            out->addr.ipv4.sin_port = addr.ipv4.port;
            out->addr.ipv4.sin_addr.s_addr = addr.ipv4.ip;
        } break;
        
        case BADDR_TYPE_IPV6: {
            out->len = sizeof(out->addr.ipv6);
            memset(&out->addr.ipv6, 0, sizeof(out->addr.ipv6));
            out->addr.ipv6.sin6_family = AF_INET6;
            out->addr.ipv6.sin6_port = addr.ipv6.port;
            out->addr.ipv6.sin6_flowinfo = 0;
            memcpy(out->addr.ipv6.sin6_addr.s6_addr, addr.ipv6.ip, 16);
            out->addr.ipv6.sin6_scope_id = 0;
        } break;
        
        default: ASSERT(0);
    }
}

static void addr_sys_to_socket (BAddr *out, struct BDatagram_sys_addr addr)
{
    switch (addr.addr.generic.sa_family) {
        case AF_INET: {
            ASSERT(addr.len == sizeof(struct sockaddr_in))
            BAddr_InitIPv4(out, addr.addr.ipv4.sin_addr.s_addr, addr.addr.ipv4.sin_port);
        } break;
        
        case AF_INET6: {
            ASSERT(addr.len == sizeof(struct sockaddr_in6))
            BAddr_InitIPv6(out, addr.addr.ipv6.sin6_addr.s6_addr, addr.addr.ipv6.sin6_port);
        } break;
        
        default: {
            BAddr_InitNone(out);
        } break;
    }
}

static void set_pktinfo (SOCKET sock, int family)
{
    DWORD opt = 1;
    
    switch (family) {
        case BADDR_TYPE_IPV4: {
            if (setsockopt(sock, IPPROTO_IP, IP_PKTINFO, (char *)&opt, sizeof(opt)) < 0) {
                BLog(BLOG_ERROR, "setsockopt(IP_PKTINFO) failed");
            }
        } break;
        
        case BADDR_TYPE_IPV6: {
            if (setsockopt(sock, IPPROTO_IPV6, IPV6_PKTINFO, (char *)&opt, sizeof(opt)) < 0) {
                BLog(BLOG_ERROR, "setsockopt(IPV6_PKTINFO) failed");
            }
        } break;
    }
}

static void report_error (BDatagram *o)
{
    DebugError_AssertNoError(&o->d_err);
    
    // report error
    DEBUGERROR(&o->d_err, o->handler(o->user, BDATAGRAM_EVENT_ERROR));
    return;
}

static void datagram_abort (BDatagram *o)
{
    ASSERT(!o->aborted)
    
    // cancel I/O
    if ((o->recv.inited && o->recv.data_have && o->recv.data_busy) || (o->send.inited && o->send.data_len >= 0 && o->send.data_busy)) {
        if (!CancelIo((HANDLE)o->sock)) {
            BLog(BLOG_ERROR, "CancelIo failed");
        }
    }
    
    // close socket
    if (closesocket(o->sock) == SOCKET_ERROR) {
        BLog(BLOG_ERROR, "closesocket failed");
    }
    
    // wait for receiving to complete
    if (o->recv.inited && o->recv.data_have && o->recv.data_busy) {
        BReactorIOCPOverlapped_Wait(&o->recv.olap, NULL, NULL);
    }
    
    // wait for sending to complete
    if (o->send.inited && o->send.data_len >= 0 && o->send.data_busy) {
        BReactorIOCPOverlapped_Wait(&o->send.olap, NULL, NULL);
    }
    
    // free recv olap
    BReactorIOCPOverlapped_Free(&o->recv.olap);
    
    // free send olap
    BReactorIOCPOverlapped_Free(&o->send.olap);
    
    // set aborted
    o->aborted = 1;
}

static void start_send (BDatagram *o)
{
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->send.inited)
    ASSERT(o->send.data_len >= 0)
    ASSERT(!o->send.data_busy)
    ASSERT(o->send.have_addrs)
    
    // convert destination address
    addr_socket_to_sys(&o->send.sysaddr, o->send.remote_addr);
    
    WSABUF buf;
    buf.buf = (char *)o->send.data;
    buf.len = (o->send.data_len > ULONG_MAX ? ULONG_MAX : o->send.data_len);
    
    memset(&o->send.olap.olap, 0, sizeof(o->send.olap.olap));
    
    if (o->fnWSASendMsg) {
        o->send.msg.name = &o->send.sysaddr.addr.generic;
        o->send.msg.namelen = o->send.sysaddr.len;
        o->send.msg.lpBuffers = &buf;
        o->send.msg.dwBufferCount = 1;
        o->send.msg.Control.buf = (char *)&o->send.cdata;
        o->send.msg.Control.len = sizeof(o->send.cdata);
        o->send.msg.dwFlags = 0;
        
        int sum = 0;
        
        WSACMSGHDR *cmsg = WSA_CMSG_FIRSTHDR(&o->send.msg);
        
        switch (o->send.local_addr.type) {
            case BADDR_TYPE_IPV4: {
                memset(cmsg, 0, WSA_CMSG_SPACE(sizeof(struct in_pktinfo)));
                cmsg->cmsg_level = IPPROTO_IP;
                cmsg->cmsg_type = IP_PKTINFO;
                cmsg->cmsg_len = WSA_CMSG_LEN(sizeof(struct in_pktinfo));
                struct in_pktinfo *pktinfo = (struct in_pktinfo *)WSA_CMSG_DATA(cmsg);
                pktinfo->ipi_addr.s_addr = o->send.local_addr.ipv4;
                sum += WSA_CMSG_SPACE(sizeof(struct in_pktinfo));
            } break;
            case BADDR_TYPE_IPV6: {
                memset(cmsg, 0, WSA_CMSG_SPACE(sizeof(struct in6_pktinfo)));
                cmsg->cmsg_level = IPPROTO_IPV6;
                cmsg->cmsg_type = IPV6_PKTINFO;
                cmsg->cmsg_len = WSA_CMSG_LEN(sizeof(struct in6_pktinfo));
                struct in6_pktinfo *pktinfo = (struct in6_pktinfo *)WSA_CMSG_DATA(cmsg);
                memcpy(pktinfo->ipi6_addr.s6_addr, o->send.local_addr.ipv6, 16);
                sum += WSA_CMSG_SPACE(sizeof(struct in6_pktinfo));
            } break;
        }
        
        o->send.msg.Control.len = sum;
        
        if (o->send.msg.Control.len == 0) {
            o->send.msg.Control.buf = NULL;
        }
        
        // send
        int res = o->fnWSASendMsg(o->sock, &o->send.msg, 0, NULL, &o->send.olap.olap, NULL);
        if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            report_error(o);
            return;
        }
    } else {
        // send
        int res = WSASendTo(o->sock, &buf, 1, NULL, 0, &o->send.sysaddr.addr.generic, o->send.sysaddr.len, &o->send.olap.olap, NULL);
        if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            report_error(o);
            return;
        }
    }
    
    // set busy
    o->send.data_busy = 1;
}

static void start_recv (BDatagram *o)
{
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->recv.inited)
    ASSERT(o->recv.data_have)
    ASSERT(!o->recv.data_busy)
    ASSERT(o->recv.started)
    
    WSABUF buf;
    buf.buf = (char *)o->recv.data;
    buf.len = (o->recv.mtu > ULONG_MAX ? ULONG_MAX : o->recv.mtu);
    
    memset(&o->recv.olap.olap, 0, sizeof(o->recv.olap.olap));
    
    if (o->fnWSARecvMsg) {
        o->recv.msg.name = &o->recv.sysaddr.addr.generic;
        o->recv.msg.namelen = sizeof(o->recv.sysaddr.addr);
        o->recv.msg.lpBuffers = &buf;
        o->recv.msg.dwBufferCount = 1;
        o->recv.msg.Control.buf = (char *)&o->recv.cdata;
        o->recv.msg.Control.len = sizeof(o->recv.cdata);
        o->recv.msg.dwFlags = 0;
        
        // recv
        int res = o->fnWSARecvMsg(o->sock, &o->recv.msg, NULL, &o->recv.olap.olap, NULL);
        if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            BLog(BLOG_ERROR, "WSARecvMsg failed (%d)", WSAGetLastError());
            report_error(o);
            return;
        }
    } else {
        o->recv.sysaddr.len = sizeof(o->recv.sysaddr.addr);
        
        // recv
        DWORD flags = 0;
        int res = WSARecvFrom(o->sock, &buf, 1, NULL, &flags, &o->recv.sysaddr.addr.generic, &o->recv.sysaddr.len, &o->recv.olap.olap, NULL);
        if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            BLog(BLOG_ERROR, "WSARecvFrom failed (%d)", WSAGetLastError());
            report_error(o);
            return;
        }
    }
    
    // set busy
    o->recv.data_busy = 1;
}

static void send_job_handler (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->send.inited)
    ASSERT(o->send.data_len >= 0)
    ASSERT(!o->send.data_busy)
    ASSERT(o->send.have_addrs)
    
    // send
    start_send(o);
    return;
}

static void recv_job_handler (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->recv.inited)
    ASSERT(o->recv.data_have)
    ASSERT(!o->recv.data_busy)
    ASSERT(o->recv.started)
    
    // recv
    start_recv(o);
    return;
}

static void send_if_handler_send (BDatagram *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->send.inited)
    ASSERT(o->send.data_len == -1)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->send.mtu)
    
    // remember data
    o->send.data = data;
    o->send.data_len = data_len;
    o->send.data_busy = 0;
    
    // if have no addresses, wait
    if (!o->send.have_addrs) {
        return;
    }
    
    // send
    start_send(o);
    return;
}

static void recv_if_handler_recv (BDatagram *o, uint8_t *data)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->recv.inited)
    ASSERT(!o->recv.data_have)
    
    // remember data
    o->recv.data = data;
    o->recv.data_have = 1;
    o->recv.data_busy = 0;
    
    // if recv not started yet, wait
    if (!o->recv.started) {
        return;
    }
    
    // recv
    start_recv(o);
    return;
}

static void send_olap_handler (BDatagram *o, int event, DWORD bytes)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->send.inited)
    ASSERT(o->send.data_len >= 0)
    ASSERT(o->send.data_busy)
    ASSERT(event == BREACTOR_IOCP_EVENT_SUCCEEDED || event == BREACTOR_IOCP_EVENT_FAILED)
    
    // set not busy
    o->send.data_busy = 0;
    
    if (event == BREACTOR_IOCP_EVENT_FAILED) {
        report_error(o);
        return;
    }
    
    ASSERT(bytes >= 0)
    ASSERT(bytes <= o->send.data_len)
    
    if (bytes < o->send.data_len) {
        BLog(BLOG_ERROR, "sent too little");
    }
    
    // if recv wasn't started yet, start it
    if (!o->recv.started) {
        // set recv started
        o->recv.started = 1;
        
        // continue receiving
        if (o->recv.inited && o->recv.data_have) {
            ASSERT(!o->recv.data_busy)
            
            BPending_Set(&o->recv.job);
        }
    }
    
    // set no data
    o->send.data_len = -1;
    
    // done
    PacketPassInterface_Done(&o->send.iface);
}

static void recv_olap_handler (BDatagram *o, int event, DWORD bytes)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->recv.inited)
    ASSERT(o->recv.data_have)
    ASSERT(o->recv.data_busy)
    ASSERT(event == BREACTOR_IOCP_EVENT_SUCCEEDED || event == BREACTOR_IOCP_EVENT_FAILED)
    
    // set not busy
    o->recv.data_busy = 0;
    
    if (event == BREACTOR_IOCP_EVENT_FAILED) {
        BLog(BLOG_ERROR, "receiving failed");
        report_error(o);
        return;
    }
    
    ASSERT(bytes >= 0)
    ASSERT(bytes <= o->recv.mtu)
    
    if (o->fnWSARecvMsg) {
        o->recv.sysaddr.len = o->recv.msg.namelen;
    }
    
    // read remote address
    addr_sys_to_socket(&o->recv.remote_addr, o->recv.sysaddr);
    
    // read local address
    BIPAddr_InitInvalid(&o->recv.local_addr);
    if (o->fnWSARecvMsg) {
        for (WSACMSGHDR *cmsg = WSA_CMSG_FIRSTHDR(&o->recv.msg); cmsg; cmsg = WSA_CMSG_NXTHDR(&o->recv.msg, cmsg)) {
            if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
                struct in_pktinfo *pktinfo = (struct in_pktinfo *)WSA_CMSG_DATA(cmsg);
                BIPAddr_InitIPv4(&o->recv.local_addr, pktinfo->ipi_addr.s_addr);
            }
            else if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
                struct in6_pktinfo *pktinfo = (struct in6_pktinfo *)WSA_CMSG_DATA(cmsg);
                BIPAddr_InitIPv6(&o->recv.local_addr, pktinfo->ipi6_addr.s6_addr);
            }
        }
    }
    
    // set have addresses
    o->recv.have_addrs = 1;
    
    // set no data
    o->recv.data_have = 0;
    
    // done
    PacketRecvInterface_Done(&o->recv.iface, bytes);
}

int BDatagram_AddressFamilySupported (int family)
{
    return (family == BADDR_TYPE_IPV4 || family == BADDR_TYPE_IPV6);
}

int BDatagram_Init (BDatagram *o, int family, BReactor *reactor, void *user,
                    BDatagram_handler handler)
{
    ASSERT(BDatagram_AddressFamilySupported(family))
    ASSERT(handler)
    BNetwork_Assert();
    
    // init arguments
    o->reactor = reactor;
    o->user = user;
    o->handler = handler;
    
    // init socket
    if ((o->sock = WSASocket(family_socket_to_sys(family), SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
        BLog(BLOG_ERROR, "WSASocket failed");
        goto fail0;
    }
    
    DWORD out_bytes;
    
    // obtain WSASendMsg
    GUID guid1 = WSAID_WSASENDMSG;
    if (WSAIoctl(o->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid1, sizeof(guid1), &o->fnWSASendMsg, sizeof(o->fnWSASendMsg), &out_bytes, NULL, NULL) != 0) {
        o->fnWSASendMsg = NULL;
    }
    
    // obtain WSARecvMsg
    GUID guid2 = WSAID_WSARECVMSG;
    if (WSAIoctl(o->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid2, sizeof(guid2), &o->fnWSARecvMsg, sizeof(o->fnWSARecvMsg), &out_bytes, NULL, NULL) != 0) {
        BLog(BLOG_ERROR, "failed to obtain WSARecvMsg");
        o->fnWSARecvMsg = NULL;
    }
    
    // associate with IOCP
    if (!CreateIoCompletionPort((HANDLE)o->sock, BReactor_GetIOCPHandle(o->reactor), 0, 0)) {
        BLog(BLOG_ERROR, "CreateIoCompletionPort failed");
        goto fail1;
    }
    
    // enable receiving pktinfo
    set_pktinfo(o->sock, family);
    
    // set not aborted
    o->aborted = 0;
    
    // init send olap
    BReactorIOCPOverlapped_Init(&o->send.olap, o->reactor, o, (BReactorIOCPOverlapped_handler)send_olap_handler);
    
    // set have no send addrs
    o->send.have_addrs = 0;
    
    // set send not inited
    o->send.inited = 0;
    
    // init recv olap
    BReactorIOCPOverlapped_Init(&o->recv.olap, o->reactor, o, (BReactorIOCPOverlapped_handler)recv_olap_handler);
    
    // set recv not started
    o->recv.started = 0;
    
    // set have no recv addrs
    o->recv.have_addrs = 0;
    
    // set recv not inited
    o->recv.inited = 0;
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(o->reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    if (closesocket(o->sock) == SOCKET_ERROR) {
        BLog(BLOG_ERROR, "closesocket failed");
    }
fail0:
    return 0;
}

void BDatagram_Free (BDatagram *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    ASSERT(!o->recv.inited)
    ASSERT(!o->send.inited)
    
    if (!o->aborted) {
        datagram_abort(o);
    }
}

int BDatagram_Bind (BDatagram *o, BAddr addr)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(BDatagram_AddressFamilySupported(addr.type))
    
    // translate address
    struct BDatagram_sys_addr sysaddr;
    addr_socket_to_sys(&sysaddr, addr);
    
    // bind
    if (bind(o->sock, &sysaddr.addr.generic, sysaddr.len) < 0) {
        BLog(BLOG_ERROR, "bind failed");
        return 0;
    }
    
    // if recv wasn't started yet, start it
    if (!o->recv.started) {
        // set recv started
        o->recv.started = 1;
        
        // continue receiving
        if (o->recv.inited && o->recv.data_have) {
            ASSERT(!o->recv.data_busy)
            
            BPending_Set(&o->recv.job);
        }
    }
    
    return 1;
}

void BDatagram_SetSendAddrs (BDatagram *o, BAddr remote_addr, BIPAddr local_addr)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(BDatagram_AddressFamilySupported(remote_addr.type))
    ASSERT(local_addr.type == BADDR_TYPE_NONE || BDatagram_AddressFamilySupported(local_addr.type))
    
    // set addresses
    o->send.remote_addr = remote_addr;
    o->send.local_addr = local_addr;
    
    // set have addresses
    o->send.have_addrs = 1;
    
    // start sending
    if (o->send.inited && o->send.data_len >= 0 && !o->send.data_busy) {
        BPending_Set(&o->send.job);
    }
}

int BDatagram_GetLastReceiveAddrs (BDatagram *o, BAddr *remote_addr, BIPAddr *local_addr)
{
    DebugObject_Access(&o->d_obj);
    
    if (!o->recv.have_addrs) {
        return 0;
    }
    
    *remote_addr = o->recv.remote_addr;
    *local_addr = o->recv.local_addr;
    return 1;
}

int BDatagram_SetReuseAddr (BDatagram *o, int reuse)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(reuse == 0 || reuse == 1)
    
    if (setsockopt(o->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        return 0;
    }
    
    return 1;
}

void BDatagram_SendAsync_Init (BDatagram *o, int mtu)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(!o->send.inited)
    ASSERT(mtu >= 0)
    
    // init arguments
    o->send.mtu = mtu;
    
    // init interface
    PacketPassInterface_Init(&o->send.iface, o->send.mtu, (PacketPassInterface_handler_send)send_if_handler_send, o, BReactor_PendingGroup(o->reactor));
    
    // init job
    BPending_Init(&o->send.job, BReactor_PendingGroup(o->reactor), (BPending_handler)send_job_handler, o);
    
    // set have no data
    o->send.data_len = -1;
    
    // set inited
    o->send.inited = 1;
}

void BDatagram_SendAsync_Free (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->send.inited)
    
    // abort if busy
    if (o->send.data_len >= 0 && o->send.data_busy && !o->aborted) {
        datagram_abort(o);
    }
    
    // free job
    BPending_Free(&o->send.job);
    
    // free interface
    PacketPassInterface_Free(&o->send.iface);
    
    // set not inited
    o->send.inited = 0;
}

PacketPassInterface * BDatagram_SendAsync_GetIf (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->send.inited)
    
    return &o->send.iface;
}

void BDatagram_RecvAsync_Init (BDatagram *o, int mtu)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(!o->recv.inited)
    ASSERT(mtu >= 0)
    
    // init arguments
    o->recv.mtu = mtu;
    
    // init interface
    PacketRecvInterface_Init(&o->recv.iface, o->recv.mtu, (PacketRecvInterface_handler_recv)recv_if_handler_recv, o, BReactor_PendingGroup(o->reactor));
    
    // init job
    BPending_Init(&o->recv.job, BReactor_PendingGroup(o->reactor), (BPending_handler)recv_job_handler, o);
    
    // set have no data
    o->recv.data_have = 0;
    
    // set inited
    o->recv.inited = 1;
}

void BDatagram_RecvAsync_Free (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->recv.inited)
    
    // abort if busy
    if (o->recv.data_have && o->recv.data_busy && !o->aborted) {
        datagram_abort(o);
    }
    
    // free job
    BPending_Free(&o->recv.job);
    
    // free interface
    PacketRecvInterface_Free(&o->recv.iface);
    
    // set not inited
    o->recv.inited = 0;
}

PacketRecvInterface * BDatagram_RecvAsync_GetIf (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->recv.inited)
    
    return &o->recv.iface;
}
