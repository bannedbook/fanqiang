/**
 * @file BDatagram_unix.c
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef BADVPN_LINUX
#    include <netpacket/packet.h>
#    include <net/ethernet.h>
#endif

#include <misc/nonblocking.h>
#include <base/BLog.h>

#include "BDatagram.h"

#include <generated/blog_channel_BDatagram.h>

struct sys_addr {
    socklen_t len;
    union {
        struct sockaddr generic;
        struct sockaddr_in ipv4;
        struct sockaddr_in6 ipv6;
#ifdef BADVPN_LINUX
        struct sockaddr_ll packet;
#endif
    } addr;
};

static int family_socket_to_sys (int family);
static void addr_socket_to_sys (struct sys_addr *out, BAddr addr);
static void addr_sys_to_socket (BAddr *out, struct sys_addr addr);
static void set_pktinfo (int fd, int family);
static void report_error (BDatagram *o);
static void do_send (BDatagram *o);
static void do_recv (BDatagram *o);
static void fd_handler (BDatagram *o, int events);
static void send_job_handler (BDatagram *o);
static void recv_job_handler (BDatagram *o);
static void send_if_handler_send (BDatagram *o, uint8_t *data, int data_len);
static void recv_if_handler_recv (BDatagram *o, uint8_t *data);

static int family_socket_to_sys (int family)
{
    switch (family) {
        case BADDR_TYPE_IPV4:
            return AF_INET;
        case BADDR_TYPE_IPV6:
            return AF_INET6;
#ifdef BADVPN_LINUX
        case BADDR_TYPE_PACKET:
            return AF_PACKET;
#endif
    }
    
    ASSERT(0);
    return 0;
}

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
        
#ifdef BADVPN_LINUX
        case BADDR_TYPE_PACKET: {
            ASSERT(addr.packet.header_type == BADDR_PACKET_HEADER_TYPE_ETHERNET)
            memset(&out->addr.packet, 0, sizeof(out->addr.packet));
            out->len = sizeof(out->addr.packet);
            out->addr.packet.sll_family = AF_PACKET;
            out->addr.packet.sll_protocol = addr.packet.phys_proto;
            out->addr.packet.sll_ifindex = addr.packet.interface_index;
            out->addr.packet.sll_hatype = 1; // linux/if_arp.h: #define ARPHRD_ETHER 1
            switch (addr.packet.packet_type) {
                case BADDR_PACKET_PACKET_TYPE_HOST:
                    out->addr.packet.sll_pkttype = PACKET_HOST;
                    break;
                case BADDR_PACKET_PACKET_TYPE_BROADCAST:
                    out->addr.packet.sll_pkttype = PACKET_BROADCAST;
                    break;
                case BADDR_PACKET_PACKET_TYPE_MULTICAST:
                    out->addr.packet.sll_pkttype = PACKET_MULTICAST;
                    break;
                case BADDR_PACKET_PACKET_TYPE_OTHERHOST:
                    out->addr.packet.sll_pkttype = PACKET_OTHERHOST;
                    break;
                case BADDR_PACKET_PACKET_TYPE_OUTGOING:
                    out->addr.packet.sll_pkttype = PACKET_OUTGOING;
                    break;
                default:
                    ASSERT(0);
            }
            out->addr.packet.sll_halen = 6;
            memcpy(out->addr.packet.sll_addr, addr.packet.phys_addr, 6);
        } break;
#endif
        
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
        
#ifdef BADVPN_LINUX
        case AF_PACKET: {
            if (addr.len < offsetof(struct sockaddr_ll, sll_addr) + 6) {
                goto fail;
            }
            if (addr.addr.packet.sll_hatype != 1) { // linux/if_arp.h: #define ARPHRD_ETHER 1
                goto fail;
            }
            int packet_type;
            switch (addr.addr.packet.sll_pkttype) {
                case PACKET_HOST:
                    packet_type = BADDR_PACKET_PACKET_TYPE_HOST;
                    break;
                case PACKET_BROADCAST:
                    packet_type = BADDR_PACKET_PACKET_TYPE_BROADCAST;
                    break;
                case PACKET_MULTICAST:
                    packet_type = BADDR_PACKET_PACKET_TYPE_MULTICAST;
                    break;
                case PACKET_OTHERHOST:
                    packet_type = BADDR_PACKET_PACKET_TYPE_OTHERHOST;
                    break;
                case PACKET_OUTGOING:
                    packet_type = BADDR_PACKET_PACKET_TYPE_OUTGOING;
                    break;
                default:
                    goto fail;
            }
            if (addr.addr.packet.sll_halen != 6) {
                goto fail;
            }
            BAddr_InitPacket(out, addr.addr.packet.sll_protocol, addr.addr.packet.sll_ifindex, BADDR_PACKET_HEADER_TYPE_ETHERNET, packet_type, addr.addr.packet.sll_addr);
        } break;
#endif
        
        fail:
        default: {
            BAddr_InitNone(out);
        } break;
    }
}

static void set_pktinfo (int fd, int family)
{
    int opt = 1;
    
    switch (family) {
        case BADDR_TYPE_IPV4: {
#ifdef BADVPN_FREEBSD
            if (setsockopt(fd, IPPROTO_IP, IP_RECVDSTADDR, &opt, sizeof(opt)) < 0) {
                BLog(BLOG_ERROR, "setsockopt(IP_RECVDSTADDR) failed");
            }
#else
            if (setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt)) < 0) {
                BLog(BLOG_ERROR, "setsockopt(IP_PKTINFO) failed");
            }
#endif
        } break;
        
#ifdef IPV6_RECVPKTINFO
        case BADDR_TYPE_IPV6: {
            if (setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &opt, sizeof(opt)) < 0) {
                BLog(BLOG_ERROR, "setsockopt(IPV6_RECVPKTINFO) failed");
            }
        } break;
#endif
    }
}

static void report_error (BDatagram *o)
{
    DebugError_AssertNoError(&o->d_err);
    
    // report error
    DEBUGERROR(&o->d_err, o->handler(o->user, BDATAGRAM_EVENT_ERROR));
    return;
}

static void do_send (BDatagram *o)
{
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->send.inited)
    ASSERT(o->send.busy)
    ASSERT(o->send.have_addrs)
    
    // limit
    if (!BReactorLimit_Increment(&o->send.limit)) {
        // wait for fd
        o->wait_events |= BREACTOR_WRITE;
        BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->wait_events);
        return;
    }
    
    // convert destination address
    struct sys_addr sysaddr;
    addr_socket_to_sys(&sysaddr, o->send.remote_addr);
    
    struct iovec iov;
    iov.iov_base = (uint8_t *)o->send.busy_data;
    iov.iov_len = o->send.busy_data_len;
    
    union {
#ifdef BADVPN_FREEBSD
        char in[CMSG_SPACE(sizeof(struct in_addr))];
#else
        char in[CMSG_SPACE(sizeof(struct in_pktinfo))];
#endif
        char in6[CMSG_SPACE(sizeof(struct in6_pktinfo))];
    } cdata;
    
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &sysaddr.addr.generic;
    msg.msg_namelen = sysaddr.len;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &cdata;
    msg.msg_controllen = sizeof(cdata);
    
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    
    size_t controllen = 0;
    
    switch (o->send.local_addr.type) {
        case BADDR_TYPE_IPV4: {
#ifdef BADVPN_FREEBSD
            memset(cmsg, 0, CMSG_SPACE(sizeof(struct in_addr)));
            cmsg->cmsg_level = IPPROTO_IP;
            cmsg->cmsg_type = IP_SENDSRCADDR;
            cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_addr));
            struct in_addr *addrinfo = (struct in_addr *)CMSG_DATA(cmsg);
            addrinfo->s_addr = o->send.local_addr.ipv4;
            controllen += CMSG_SPACE(sizeof(struct in_addr));
#else
            memset(cmsg, 0, CMSG_SPACE(sizeof(struct in_pktinfo)));
            cmsg->cmsg_level = IPPROTO_IP;
            cmsg->cmsg_type = IP_PKTINFO;
            cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
            struct in_pktinfo *pktinfo = (struct in_pktinfo *)CMSG_DATA(cmsg);
            pktinfo->ipi_spec_dst.s_addr = o->send.local_addr.ipv4;
            controllen += CMSG_SPACE(sizeof(struct in_pktinfo));
#endif
        } break;
        
        case BADDR_TYPE_IPV6: {
            memset(cmsg, 0, CMSG_SPACE(sizeof(struct in6_pktinfo)));
            cmsg->cmsg_level = IPPROTO_IPV6;
            cmsg->cmsg_type = IPV6_PKTINFO;
            cmsg->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
            struct in6_pktinfo *pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
            memcpy(pktinfo->ipi6_addr.s6_addr, o->send.local_addr.ipv6, 16);
            controllen += CMSG_SPACE(sizeof(struct in6_pktinfo));
        } break;
    }
    
    msg.msg_controllen = controllen;
    
    if (msg.msg_controllen == 0) {
        msg.msg_control = NULL;
    }
    
    // send
    int bytes = sendmsg(o->fd, &msg, 0);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // wait for fd
            o->wait_events |= BREACTOR_WRITE;
            BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->wait_events);
            return;
        }
        
        report_error(o);
        return;
    }
    
    ASSERT(bytes >= 0)
    ASSERT(bytes <= o->send.busy_data_len)
    
    if (bytes < o->send.busy_data_len) {
        BLog(BLOG_ERROR, "send sent too little");
    }
    
    // if recv wasn't started yet, start it
    if (!o->recv.started) {
        // set recv started
        o->recv.started = 1;
        
        // continue receiving
        if (o->recv.inited && o->recv.busy) {
            BPending_Set(&o->recv.job);
        }
    }
    
    // set not busy
    o->send.busy = 0;
    
    // done
    PacketPassInterface_Done(&o->send.iface);
}

static void do_recv (BDatagram *o)
{
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->recv.inited)
    ASSERT(o->recv.busy)
    ASSERT(o->recv.started)
    
    // limit
    if (!BReactorLimit_Increment(&o->recv.limit)) {
        // wait for fd
        o->wait_events |= BREACTOR_READ;
        BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->wait_events);
        return;
    }
    
    struct sys_addr sysaddr;
    
    struct iovec iov;
    iov.iov_base = o->recv.busy_data;
    iov.iov_len = o->recv.mtu;
    
    union {
#ifdef BADVPN_FREEBSD
        char in[CMSG_SPACE(sizeof(struct in_addr))];
#else
        char in[CMSG_SPACE(sizeof(struct in_pktinfo))];
#endif
        char in6[CMSG_SPACE(sizeof(struct in6_pktinfo))];
    } cdata;
    
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &sysaddr.addr.generic;
    msg.msg_namelen = sizeof(sysaddr.addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &cdata;
    msg.msg_controllen = sizeof(cdata);
    
    // recv
    int bytes = recvmsg(o->fd, &msg, 0);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // wait for fd
            o->wait_events |= BREACTOR_READ;
            BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->wait_events);
            return;
        }
        
        BLog(BLOG_ERROR, "recv failed");
        report_error(o);
        return;
    }
    
    ASSERT(bytes >= 0)
    ASSERT(bytes <= o->recv.mtu)
    
    // read returned address
    sysaddr.len = msg.msg_namelen;
    addr_sys_to_socket(&o->recv.remote_addr, sysaddr);
    
    // read returned local address
    BIPAddr_InitInvalid(&o->recv.local_addr);
    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
#ifdef BADVPN_FREEBSD
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVDSTADDR) {
            struct in_addr *addrinfo = (struct in_addr *)CMSG_DATA(cmsg);
            BIPAddr_InitIPv4(&o->recv.local_addr, addrinfo->s_addr);
        }
#else
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
            struct in_pktinfo *pktinfo = (struct in_pktinfo *)CMSG_DATA(cmsg);
            BIPAddr_InitIPv4(&o->recv.local_addr, pktinfo->ipi_addr.s_addr);
        }
#endif
        else if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
            struct in6_pktinfo *pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
            BIPAddr_InitIPv6(&o->recv.local_addr, pktinfo->ipi6_addr.s6_addr);
        }
    }
    
    // set have addresses
    o->recv.have_addrs = 1;
    
    // set not busy
    o->recv.busy = 0;
    
    // done
    PacketRecvInterface_Done(&o->recv.iface, bytes);
}

static void fd_handler (BDatagram *o, int events)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    
    // clear handled events
    o->wait_events &= ~events;
    BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->wait_events);
    
    int have_send = 0;
    int have_recv = 0;
    
    if ((events & BREACTOR_WRITE) || ((events & (BREACTOR_ERROR|BREACTOR_HUP)) && o->send.inited && o->send.busy && o->send.have_addrs)) {
        ASSERT(o->send.inited)
        ASSERT(o->send.busy)
        ASSERT(o->send.have_addrs)
        
        have_send = 1;
    }
    
    if ((events & BREACTOR_READ) || ((events & (BREACTOR_ERROR|BREACTOR_HUP)) && o->recv.inited && o->recv.busy && o->recv.started)) {
        ASSERT(o->recv.inited)
        ASSERT(o->recv.busy)
        ASSERT(o->recv.started)
        
        have_recv = 1;
    }
    
    if (have_send) {
        if (have_recv) {
            BPending_Set(&o->recv.job);
        }
        
        do_send(o);
        return;
    }
    
    if (have_recv) {
        do_recv(o);
        return;
    }
    
    BLog(BLOG_ERROR, "fd error event");
    report_error(o);
    return;
}

static void send_job_handler (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->send.inited)
    ASSERT(o->send.busy)
    ASSERT(o->send.have_addrs)
    
    do_send(o);
    return;
}

static void recv_job_handler (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->recv.inited)
    ASSERT(o->recv.busy)
    ASSERT(o->recv.started)
    
    do_recv(o);
    return;
}

static void send_if_handler_send (BDatagram *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->send.inited)
    ASSERT(!o->send.busy)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->send.mtu)
    
    // remember data
    o->send.busy_data = data;
    o->send.busy_data_len = data_len;
    
    // set busy
    o->send.busy = 1;
    
    // if have no addresses, wait
    if (!o->send.have_addrs) {
        return;
    }
    
    // set job
    BPending_Set(&o->send.job);
}

static void recv_if_handler_recv (BDatagram *o, uint8_t *data)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->recv.inited)
    ASSERT(!o->recv.busy)
    
    // remember data
    o->recv.busy_data = data;
    
    // set busy
    o->recv.busy = 1;
    
    // if recv not started yet, wait
    if (!o->recv.started) {
        return;
    }
    
    // set job
    BPending_Set(&o->recv.job);
}

int BDatagram_AddressFamilySupported (int family)
{
    switch (family) {
        case BADDR_TYPE_IPV4:
        case BADDR_TYPE_IPV6:
#ifdef BADVPN_LINUX
        case BADDR_TYPE_PACKET:
#endif
            return 1;
    }
    
    return 0;
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
    
    // init fd
    if ((o->fd = socket(family_socket_to_sys(family), SOCK_DGRAM, 0)) < 0) {
        BLog(BLOG_ERROR, "socket failed");
        goto fail0;
    }
    
    // set fd non-blocking
    if (!badvpn_set_nonblocking(o->fd)) {
        BLog(BLOG_ERROR, "badvpn_set_nonblocking failed");
        goto fail1;
    }
    
    // enable receiving pktinfo
    set_pktinfo(o->fd, family);
    
    // init BFileDescriptor
    BFileDescriptor_Init(&o->bfd, o->fd, (BFileDescriptor_handler)fd_handler, o);
    if (!BReactor_AddFileDescriptor(o->reactor, &o->bfd)) {
        BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
        goto fail1;
    }
    
    // set no wait events
    o->wait_events = 0;
    
    // init limits
    BReactorLimit_Init(&o->send.limit, o->reactor, BDATAGRAM_SEND_LIMIT);
    BReactorLimit_Init(&o->recv.limit, o->reactor, BDATAGRAM_RECV_LIMIT);
    
    // set have no send and recv addresses
    o->send.have_addrs = 0;
    o->recv.have_addrs = 0;
    
    // set recv not started
    o->recv.started = 0;
    
    // set send and recv not inited
    o->send.inited = 0;
    o->recv.inited = 0;
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(o->reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    if (close(o->fd) < 0) {
        BLog(BLOG_ERROR, "close failed");
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
    
    // free limits
    BReactorLimit_Free(&o->recv.limit);
    BReactorLimit_Free(&o->send.limit);
    
    // free BFileDescriptor
    BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
    
    // free fd
    if (close(o->fd) < 0) {
        BLog(BLOG_ERROR, "close failed");
    }
}

int BDatagram_Bind (BDatagram *o, BAddr addr)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(BDatagram_AddressFamilySupported(addr.type))
    
    // translate address
    struct sys_addr sysaddr;
    addr_socket_to_sys(&sysaddr, addr);
    
    // bind
    if (bind(o->fd, &sysaddr.addr.generic, sysaddr.len) < 0) {
        BLog(BLOG_ERROR, "bind failed");
        return 0;
    }
    
    // if recv wasn't started yet, start it
    if (!o->recv.started) {
        // set recv started
        o->recv.started = 1;
        
        // continue receiving
        if (o->recv.inited && o->recv.busy) {
            BPending_Set(&o->recv.job);
        }
    }
    
    return 1;
}

void BDatagram_SetSendAddrs (BDatagram *o, BAddr remote_addr, BIPAddr local_addr)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(BDatagram_AddressFamilySupported(remote_addr.type))
    ASSERT(local_addr.type == BADDR_TYPE_NONE || BDatagram_AddressFamilySupported(local_addr.type))
    
    // set addresses
    o->send.remote_addr = remote_addr;
    o->send.local_addr = local_addr;
    
    if (!o->send.have_addrs) {
        // set have addresses
        o->send.have_addrs = 1;
        
        // start sending
        if (o->send.inited && o->send.busy) {
            BPending_Set(&o->send.job);
        }
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

int BDatagram_GetFd (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    
    return o->fd;
}

int BDatagram_SetReuseAddr (BDatagram *o, int reuse)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(reuse == 0 || reuse == 1)
    
    if (setsockopt(o->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        return 0;
    }
    
    return 1;
}

void BDatagram_SendAsync_Init (BDatagram *o, int mtu)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(!o->send.inited)
    ASSERT(mtu >= 0)
    
    // init arguments
    o->send.mtu = mtu;
    
    // init interface
    PacketPassInterface_Init(&o->send.iface, o->send.mtu, (PacketPassInterface_handler_send)send_if_handler_send, o, BReactor_PendingGroup(o->reactor));
    
    // init job
    BPending_Init(&o->send.job, BReactor_PendingGroup(o->reactor), (BPending_handler)send_job_handler, o);
    
    // set not busy
    o->send.busy = 0;
    
    // set inited
    o->send.inited = 1;
}

void BDatagram_SendAsync_Free (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->send.inited)
    
    // update events
    o->wait_events &= ~BREACTOR_WRITE;
    BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->wait_events);
    
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
    ASSERT(!o->recv.inited)
    ASSERT(mtu >= 0)
    
    // init arguments
    o->recv.mtu = mtu;
    
    // init interface
    PacketRecvInterface_Init(&o->recv.iface, o->recv.mtu, (PacketRecvInterface_handler_recv)recv_if_handler_recv, o, BReactor_PendingGroup(o->reactor));
    
    // init job
    BPending_Init(&o->recv.job, BReactor_PendingGroup(o->reactor), (BPending_handler)recv_job_handler, o);
    
    // set not busy
    o->recv.busy = 0;
    
    // set inited
    o->recv.inited = 1;
}

void BDatagram_RecvAsync_Free (BDatagram *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->recv.inited)
    
    // update events
    o->wait_events &= ~BREACTOR_READ;
    BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->wait_events);
    
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
