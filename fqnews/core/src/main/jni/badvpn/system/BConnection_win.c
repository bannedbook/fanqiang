/**
 * @file BConnection_win.c
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
#include <limits.h>

#include <base/BLog.h>

#include "BConnection.h"

#include <generated/blog_channel_BConnection.h>

#define LISTEN_BACKLOG 128

struct sys_addr {
    int len;
    union {
        struct sockaddr generic;
        struct sockaddr_in ipv4;
        struct sockaddr_in6 ipv6;
    } addr;
};

static void addr_socket_to_sys (struct sys_addr *out, BAddr addr);
static void addr_any_to_sys (struct sys_addr *out, int family);
static void addr_sys_to_socket (BAddr *out, struct sys_addr addr);
static void listener_next_job_handler (BListener *o);
static void listener_olap_handler (BListener *o, int event, DWORD bytes);
static void connector_olap_handler (BConnector *o, int event, DWORD bytes);
static void connector_abort (BConnector *o);
static void connection_report_error (BConnection *o);
static void connection_abort (BConnection *o);
static void connection_send_iface_handler_send (BConnection *o, uint8_t *data, int data_len);
static void connection_recv_iface_handler_recv (BConnection *o, uint8_t *data, int data_len);
static void connection_send_olap_handler (BConnection *o, int event, DWORD bytes);
static void connection_recv_olap_handler (BConnection *o, int event, DWORD bytes);

static void addr_socket_to_sys (struct sys_addr *out, BAddr addr)
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

static void addr_any_to_sys (struct sys_addr *out, int family)
{
    switch (family) {
        case BADDR_TYPE_IPV4: {
            out->len = sizeof(out->addr.ipv4);
            memset(&out->addr.ipv4, 0, sizeof(out->addr.ipv4));
            out->addr.ipv4.sin_family = AF_INET;
            out->addr.ipv4.sin_port = 0;
            out->addr.ipv4.sin_addr.s_addr = INADDR_ANY;
        } break;
        
        case BADDR_TYPE_IPV6: {
            out->len = sizeof(out->addr.ipv6);
            memset(&out->addr.ipv6, 0, sizeof(out->addr.ipv6));
            out->addr.ipv6.sin6_family = AF_INET6;
            out->addr.ipv6.sin6_port = 0;
            out->addr.ipv6.sin6_flowinfo = 0;
            struct in6_addr any = IN6ADDR_ANY_INIT;
            out->addr.ipv6.sin6_addr = any;
            out->addr.ipv6.sin6_scope_id = 0;
        } break;
        
        default: ASSERT(0);
    }
}

static void addr_sys_to_socket (BAddr *out, struct sys_addr addr)
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

static void listener_next_job_handler (BListener *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->busy)
    
    // free ready socket
    if (o->ready) {
        BLog(BLOG_ERROR, "discarding connection");
        
        // close new socket
        if (closesocket(o->newsock) == SOCKET_ERROR) {
            BLog(BLOG_ERROR, "closesocket failed");
        }
        
        // set not ready
        o->ready = 0;
    }
    
    // create new socket
    if ((o->newsock = WSASocket(o->sys_family, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
        BLog(BLOG_ERROR, "WSASocket failed");
        goto fail0;
    }
    
    // start accept operation
    while (1) {
        memset(&o->olap.olap, 0, sizeof(o->olap.olap));
        DWORD bytes;
        BOOL res = o->fnAcceptEx(o->sock, o->newsock, o->addrbuf, 0, sizeof(struct BListener_addrbuf_stub), sizeof(struct BListener_addrbuf_stub), &bytes, &o->olap.olap);
        if (res == FALSE && WSAGetLastError() != ERROR_IO_PENDING) {
            BLog(BLOG_ERROR, "AcceptEx failed");
            continue;
        }
        break;
    }
    
    // set busy
    o->busy = 1;
    
    return;
    
fail0:
    return;
}

static void listener_olap_handler (BListener *o, int event, DWORD bytes)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->busy)
    ASSERT(!o->ready)
    ASSERT(event == BREACTOR_IOCP_EVENT_SUCCEEDED || event == BREACTOR_IOCP_EVENT_FAILED)
    
    // set not busy
    o->busy = 0;
    
    // schedule next accept
    BPending_Set(&o->next_job);
    
    if (event == BREACTOR_IOCP_EVENT_FAILED) {
        BLog(BLOG_ERROR, "accepting failed");
        
        // close new socket
        if (closesocket(o->newsock) == SOCKET_ERROR) {
            BLog(BLOG_ERROR, "closesocket failed");
        }
        
        return;
    }
    
    BLog(BLOG_INFO, "connection accepted");
    
    // set ready
    o->ready = 1;
    
    // call handler
    o->handler(o->user);
    return;
}

static void connector_olap_handler (BConnector *o, int event, DWORD bytes)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->sock != INVALID_SOCKET)
    ASSERT(o->busy)
    ASSERT(!o->ready)
    ASSERT(event == BREACTOR_IOCP_EVENT_SUCCEEDED || event == BREACTOR_IOCP_EVENT_FAILED)
    
    // set not busy
    o->busy = 0;
    
    if (event == BREACTOR_IOCP_EVENT_FAILED) {
        BLog(BLOG_ERROR, "connection failed");
    } else {
        // set ready
        o->ready = 1;
    }
    
    // call handler
    o->handler(o->user, !o->ready);
    return;
}

static void connector_abort (BConnector *o)
{
    if (o->sock != INVALID_SOCKET) {
        // cancel I/O
        if (o->busy) {
            if (!CancelIo((HANDLE)o->sock)) {
                BLog(BLOG_ERROR, "CancelIo failed");
            }
        }
        
        // close socket
        if (closesocket(o->sock) == SOCKET_ERROR) {
            BLog(BLOG_ERROR, "closesocket failed");
        }
    }
    
    // wait for connect operation to finish
    if (o->busy) {
        BReactorIOCPOverlapped_Wait(&o->olap, NULL, NULL);
    }
    
    // free olap
    BReactorIOCPOverlapped_Free(&o->olap);
}

static void connection_report_error (BConnection *o)
{
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->handler)
    
    // report error
    DEBUGERROR(&o->d_err, o->handler(o->user, BCONNECTION_EVENT_ERROR));
    return;
}

static void connection_abort (BConnection *o)
{
    ASSERT(!o->aborted)
    
    // cancel I/O
    if ((o->recv.inited && o->recv.busy) || (o->send.inited && o->send.busy)) {
        if (!CancelIo((HANDLE)o->sock)) {
            BLog(BLOG_ERROR, "CancelIo failed");
        }
    }
    
    // close socket
    if (closesocket(o->sock) == SOCKET_ERROR) {
        BLog(BLOG_ERROR, "closesocket failed");
    }
    
    // wait for receiving to complete
    if (o->recv.inited && o->recv.busy) {
        BReactorIOCPOverlapped_Wait(&o->recv.olap, NULL, NULL);
    }
    
    // wait for sending to complete
    if (o->send.inited && o->send.busy) {
        BReactorIOCPOverlapped_Wait(&o->send.olap, NULL, NULL);
    }
    
    // free recv olap
    BReactorIOCPOverlapped_Free(&o->recv.olap);
    
    // free send olap
    BReactorIOCPOverlapped_Free(&o->send.olap);
    
    // set aborted
    o->aborted = 1;
}

static void connection_send_iface_handler_send (BConnection *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->send.inited)
    ASSERT(!o->send.busy)
    ASSERT(data_len > 0)
    
    if (data_len > ULONG_MAX) {
        data_len = ULONG_MAX;
    }
    
    WSABUF buf;
    buf.buf = (char *)data;
    buf.len = data_len;
    
    memset(&o->send.olap.olap, 0, sizeof(o->send.olap.olap));
    
    // send
    int res = WSASend(o->sock, &buf, 1, NULL, 0, &o->send.olap.olap, NULL);
    if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        BLog(BLOG_ERROR, "WSASend failed (%d)", WSAGetLastError());
        connection_report_error(o);
        return;
    }
    
    // set busy
    o->send.busy = 1;
    o->send.busy_data_len = data_len;
}

static void connection_recv_iface_handler_recv (BConnection *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->recv.closed)
    ASSERT(!o->aborted)
    ASSERT(o->recv.inited)
    ASSERT(!o->recv.busy)
    ASSERT(data_len > 0)
    
    if (data_len > ULONG_MAX) {
        data_len = ULONG_MAX;
    }
    
    WSABUF buf;
    buf.buf = (char *)data;
    buf.len = data_len;
    
    memset(&o->recv.olap.olap, 0, sizeof(o->recv.olap.olap));
    
    // recv
    DWORD flags = 0;
    int res = WSARecv(o->sock, &buf, 1, NULL, &flags, &o->recv.olap.olap, NULL);
    if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        BLog(BLOG_ERROR, "WSARecv failed (%d)", WSAGetLastError());
        connection_report_error(o);
        return;
    }
    
    // set busy
    o->recv.busy = 1;
    o->recv.busy_data_len = data_len;
}

static void connection_send_olap_handler (BConnection *o, int event, DWORD bytes)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(o->send.inited)
    ASSERT(o->send.busy)
    ASSERT(event == BREACTOR_IOCP_EVENT_SUCCEEDED || event == BREACTOR_IOCP_EVENT_FAILED)
    
    // set not busy
    o->send.busy = 0;
    
    if (event == BREACTOR_IOCP_EVENT_FAILED) {
        BLog(BLOG_ERROR, "sending failed");
        connection_report_error(o);
        return;
    }
    
    ASSERT(bytes > 0)
    ASSERT(bytes <= o->send.busy_data_len)
    
    // done
    StreamPassInterface_Done(&o->send.iface, bytes);
}

static void connection_recv_olap_handler (BConnection *o, int event, DWORD bytes)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->recv.closed)
    ASSERT(!o->aborted)
    ASSERT(o->recv.inited)
    ASSERT(o->recv.busy)
    ASSERT(event == BREACTOR_IOCP_EVENT_SUCCEEDED || event == BREACTOR_IOCP_EVENT_FAILED)
    
    // set not busy
    o->recv.busy = 0;
    
    if (event == BREACTOR_IOCP_EVENT_FAILED) {
        BLog(BLOG_ERROR, "receiving failed");
        connection_report_error(o);
        return;
    }
    
    if (bytes == 0) {
        // set closed
        o->recv.closed = 1;
        
        // report recv closed
        o->handler(o->user, BCONNECTION_EVENT_RECVCLOSED);
        return;
    }
    
    ASSERT(bytes > 0)
    ASSERT(bytes <= o->recv.busy_data_len)
    
    // done
    StreamRecvInterface_Done(&o->recv.iface, bytes);
}

int BConnection_AddressSupported (BAddr addr)
{
    BAddr_Assert(&addr);
    
    return (addr.type == BADDR_TYPE_IPV4 || addr.type == BADDR_TYPE_IPV6);
}

int BListener_InitFrom (BListener *o, struct BLisCon_from from,
                        BReactor *reactor, void *user,
                        BListener_handler handler)
{
    ASSERT(from.type == BLISCON_FROM_ADDR)
    ASSERT(handler)
    BNetwork_Assert();
    
    // init arguments
    o->reactor = reactor;
    o->user = user;
    o->handler = handler;
    
    // check address
    if (!BConnection_AddressSupported(from.u.from_addr.addr)) {
        BLog(BLOG_ERROR, "address not supported");
        goto fail0;
    }
    
    // convert address
    struct sys_addr sysaddr;
    addr_socket_to_sys(&sysaddr, from.u.from_addr.addr);
    
    // remember family
    o->sys_family = sysaddr.addr.generic.sa_family;
    
    // init socket
    if ((o->sock = WSASocket(sysaddr.addr.generic.sa_family, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
        BLog(BLOG_ERROR, "WSASocket failed");
        goto fail0;
    }
    
    // associate with IOCP
    if (!CreateIoCompletionPort((HANDLE)o->sock, BReactor_GetIOCPHandle(o->reactor), 0, 0)) {
        BLog(BLOG_ERROR, "CreateIoCompletionPort failed");
        goto fail1;
    }
    
    // bind
    if (bind(o->sock, &sysaddr.addr.generic, sysaddr.len) < 0) {
        BLog(BLOG_ERROR, "bind failed");
        goto fail1;
    }
    
    // listen
    if (listen(o->sock, LISTEN_BACKLOG) < 0) {
        BLog(BLOG_ERROR, "listen failed");
        goto fail1;
    }
    
    DWORD out_bytes;
    
    // obtain AcceptEx
    GUID guid1 = WSAID_ACCEPTEX;
    if (WSAIoctl(o->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid1, sizeof(guid1), &o->fnAcceptEx, sizeof(o->fnAcceptEx), &out_bytes, NULL, NULL) != 0) {
        BLog(BLOG_ERROR, "faild to obtain AcceptEx");
        goto fail1;
    }
    
    // obtain GetAcceptExSockaddrs
    GUID guid2 = WSAID_GETACCEPTEXSOCKADDRS;
    if (WSAIoctl(o->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid2, sizeof(guid2), &o->fnGetAcceptExSockaddrs, sizeof(o->fnGetAcceptExSockaddrs), &out_bytes, NULL, NULL) != 0) {
        BLog(BLOG_ERROR, "faild to obtain GetAcceptExSockaddrs");
        goto fail1;
    }
    
    // init olap
    BReactorIOCPOverlapped_Init(&o->olap, o->reactor, o, (BReactorIOCPOverlapped_handler)listener_olap_handler);
    
    // init next job
    BPending_Init(&o->next_job, BReactor_PendingGroup(o->reactor), (BPending_handler)listener_next_job_handler, o);
    
    // set not busy
    o->busy = 0;
    
    // set not ready
    o->ready = 0;
    
    // set next job
    BPending_Set(&o->next_job);
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    if (closesocket(o->sock) == SOCKET_ERROR) {
        BLog(BLOG_ERROR, "closesocket failed");
    }
fail0:
    return 0;
}

void BListener_Free (BListener *o)
{
    DebugObject_Free(&o->d_obj);
    
    // cancel I/O
    if (o->busy) {
        if (!CancelIo((HANDLE)o->sock)) {
            BLog(BLOG_ERROR, "CancelIo failed");
        }
    }
    
    // close socket
    if (closesocket(o->sock) == SOCKET_ERROR) {
        BLog(BLOG_ERROR, "closesocket failed");
    }
    
    // wait for accept operation to finish
    if (o->busy) {
        BReactorIOCPOverlapped_Wait(&o->olap, NULL, NULL);
    }
    
    // close new socket
    if (o->busy || o->ready) {
        if (closesocket(o->newsock) == SOCKET_ERROR) {
            BLog(BLOG_ERROR, "closesocket failed");
        }
    }
    
    // free next job
    BPending_Free(&o->next_job);
    
    // free olap
    BReactorIOCPOverlapped_Free(&o->olap);
}

int BConnector_InitFrom (BConnector *o, struct BLisCon_from from, BReactor *reactor, void *user,
                         BConnector_handler handler)
{
    ASSERT(from.type == BLISCON_FROM_ADDR)
    ASSERT(handler)
    BNetwork_Assert();
    
    // init arguments
    o->reactor = reactor;
    o->user = user;
    o->handler = handler;
    
    // check address
    if (!BConnection_AddressSupported(from.u.from_addr.addr)) {
        BLog(BLOG_ERROR, "address not supported");
        goto fail0;
    }
    
    // convert address
    struct sys_addr sysaddr;
    addr_socket_to_sys(&sysaddr, from.u.from_addr.addr);
    
    // create local any address
    struct sys_addr local_sysaddr;
    addr_any_to_sys(&local_sysaddr, from.u.from_addr.addr.type);
    
    // init socket
    if ((o->sock = WSASocket(sysaddr.addr.generic.sa_family, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
        BLog(BLOG_ERROR, "WSASocket failed");
        goto fail0;
    }
    
    // associate with IOCP
    if (!CreateIoCompletionPort((HANDLE)o->sock, BReactor_GetIOCPHandle(o->reactor), 0, 0)) {
        BLog(BLOG_ERROR, "CreateIoCompletionPort failed");
        goto fail1;
    }
    
    // bind socket
    if (bind(o->sock, &local_sysaddr.addr.generic, local_sysaddr.len) < 0) {
        BLog(BLOG_ERROR, "bind failed");
        goto fail1;
    }
    
    // obtain ConnectEx
    GUID guid = WSAID_CONNECTEX;
    DWORD out_bytes;
    if (WSAIoctl(o->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &o->fnConnectEx, sizeof(o->fnConnectEx), &out_bytes, NULL, NULL) != 0) {
        BLog(BLOG_ERROR, "faild to get ConnectEx");
        goto fail1;
    }
    
    // init olap
    BReactorIOCPOverlapped_Init(&o->olap, o->reactor, o, (BReactorIOCPOverlapped_handler)connector_olap_handler);
    
    // start connect operation
    BOOL res = o->fnConnectEx(o->sock, &sysaddr.addr.generic, sysaddr.len, NULL, 0, NULL, &o->olap.olap);
    if (res == FALSE && WSAGetLastError() != ERROR_IO_PENDING) {
        BLog(BLOG_ERROR, "ConnectEx failed (%d)", WSAGetLastError());
        goto fail2;
    }
    
    // set busy
    o->busy = 1;
    
    // set not ready
    o->ready = 0;
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail2:
    BReactorIOCPOverlapped_Free(&o->olap);
fail1:
    if (closesocket(o->sock) == SOCKET_ERROR) {
        BLog(BLOG_ERROR, "closesocket failed");
    }
fail0:
    return 0;
}

void BConnector_Free (BConnector *o)
{
    DebugObject_Free(&o->d_obj);
    
    if (o->sock != INVALID_SOCKET) {
        connector_abort(o);
    }
}

int BConnection_Init (BConnection *o, struct BConnection_source source, BReactor *reactor, void *user,
                      BConnection_handler handler)
{
    switch (source.type) {
        case BCONNECTION_SOURCE_TYPE_LISTENER: {
            BListener *listener = source.u.listener.listener;
            DebugObject_Access(&listener->d_obj);
            ASSERT(BPending_IsSet(&listener->next_job))
            ASSERT(!listener->busy)
            ASSERT(listener->ready)
        } break;
        case BCONNECTION_SOURCE_TYPE_CONNECTOR: {
            BConnector *connector = source.u.connector.connector;
            DebugObject_Access(&connector->d_obj);
            ASSERT(connector->reactor == reactor)
            ASSERT(connector->sock != INVALID_SOCKET)
            ASSERT(!connector->busy)
            ASSERT(connector->ready)
        } break;
        default: ASSERT(0);
    }
    ASSERT(handler)
    BNetwork_Assert();
    
    // init arguments
    o->reactor = reactor;
    o->user = user;
    o->handler = handler;
    
    switch (source.type) {
        case BCONNECTION_SOURCE_TYPE_LISTENER: {
            BListener *listener = source.u.listener.listener;
            
            // grab new socket from listener
            o->sock = listener->newsock;
            listener->ready = 0;
            
            // associate with IOCP
            if (!CreateIoCompletionPort((HANDLE)o->sock, BReactor_GetIOCPHandle(o->reactor), 0, 0)) {
                BLog(BLOG_ERROR, "CreateIoCompletionPort failed");
                goto fail1;
            }
            
            // return address
            if (source.u.listener.out_addr) {
                struct sockaddr *addr_local;
                struct sockaddr *addr_remote;
                int len_local;
                int len_remote;
                listener->fnGetAcceptExSockaddrs(listener->addrbuf, 0, sizeof(struct BListener_addrbuf_stub), sizeof(struct BListener_addrbuf_stub),
                                                 &addr_local, &len_local, &addr_remote, &len_remote);
                
                struct sys_addr sysaddr;
                
                ASSERT_FORCE(len_remote >= 0)
                ASSERT_FORCE(len_remote <= sizeof(sysaddr.addr))
                
                memcpy((uint8_t *)&sysaddr.addr, (uint8_t *)addr_remote, len_remote);
                sysaddr.len = len_remote;
                
                addr_sys_to_socket(source.u.listener.out_addr, sysaddr);
            }
        } break;
        
        case BCONNECTION_SOURCE_TYPE_CONNECTOR: {
            BConnector *connector = source.u.connector.connector;
            
            // grab fd from connector
            o->sock = connector->sock;
            connector->sock = INVALID_SOCKET;
            
            // release connector resources
            connector_abort(connector);
        } break;
    }
    
    // set not aborted
    o->aborted = 0;
    
    // init send olap
    BReactorIOCPOverlapped_Init(&o->send.olap, o->reactor, o, (BReactorIOCPOverlapped_handler)connection_send_olap_handler);
    
    // set send not inited
    o->send.inited = 0;
    
    // init recv olap
    BReactorIOCPOverlapped_Init(&o->recv.olap, o->reactor, o, (BReactorIOCPOverlapped_handler)connection_recv_olap_handler);
    
    // set recv not closed
    o->recv.closed = 0;
    
    // set recv not inited
    o->recv.inited = 0;
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(o->reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    if (closesocket(o->sock) == SOCKET_ERROR) {
        BLog(BLOG_ERROR, "closesocket failed");
    }
    return 0;
}

void BConnection_Free (BConnection *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    ASSERT(!o->recv.inited)
    ASSERT(!o->send.inited)
    
    if (!o->aborted) {
        connection_abort(o);
    }
}

void BConnection_SetHandlers (BConnection *o, void *user, BConnection_handler handler)
{
    DebugObject_Access(&o->d_obj);
    
    // set handlers
    o->user = user;
    o->handler = handler;
}

int BConnection_SetSendBuffer (BConnection *o, int buf_size)
{
    DebugObject_Access(&o->d_obj);
    
    if (setsockopt(o->sock, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, sizeof(buf_size)) < 0) {
        BLog(BLOG_ERROR, "setsockopt failed");
        return 0;
    }
    
    return 1;
}

void BConnection_SendAsync_Init (BConnection *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->aborted)
    ASSERT(!o->send.inited)
    
    // init interface
    StreamPassInterface_Init(&o->send.iface, (StreamPassInterface_handler_send)connection_send_iface_handler_send, o, BReactor_PendingGroup(o->reactor));
    
    // set not busy
    o->send.busy = 0;
    
    // set inited
    o->send.inited = 1;
}

void BConnection_SendAsync_Free (BConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->send.inited)
    
    // abort if busy
    if (o->send.busy && !o->aborted) {
        connection_abort(o);
    }
    
    // free interface
    StreamPassInterface_Free(&o->send.iface);
    
    // set not inited
    o->send.inited = 0;
}

StreamPassInterface * BConnection_SendAsync_GetIf (BConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->send.inited)
    
    return &o->send.iface;
}

void BConnection_RecvAsync_Init (BConnection *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->recv.closed)
    ASSERT(!o->aborted)
    ASSERT(!o->recv.inited)
    
    // init interface
    StreamRecvInterface_Init(&o->recv.iface, (StreamRecvInterface_handler_recv)connection_recv_iface_handler_recv, o, BReactor_PendingGroup(o->reactor));
    
    // set not busy
    o->recv.busy = 0;
    
    // set inited
    o->recv.inited = 1;
}

void BConnection_RecvAsync_Free (BConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->recv.inited)
    
    // abort if busy
    if (o->recv.busy && !o->aborted) {
        connection_abort(o);
    }
    
    // free interface
    StreamRecvInterface_Free(&o->recv.iface);
    
    // set not inited
    o->recv.inited = 0;
}

StreamRecvInterface * BConnection_RecvAsync_GetIf (BConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->recv.inited)
    
    return &o->recv.iface;
}
