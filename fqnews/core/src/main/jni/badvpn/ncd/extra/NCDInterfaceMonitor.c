/**
 * @file NCDInterfaceMonitor.c
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
#include <unistd.h>
#include <stdint.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <asm/types.h>
#include <asm/types.h>

#include <misc/debug.h>
#include <misc/nonblocking.h>
#include <base/BLog.h>

#include <ncd/extra/NCDInterfaceMonitor.h>

#include <generated/blog_channel_NCDInterfaceMonitor.h>

#define IFA_RTA(r) ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))

static int send_wilddump_request (NCDInterfaceMonitor *o, int fd, uint32_t seq, int family, int type);
static int get_attr (int type, struct rtattr *rta, int rta_len, void **out_attr, int *out_attr_len);
static void report_error (NCDInterfaceMonitor *o);
static int send_next_dump_request (NCDInterfaceMonitor *o);
static void netlink_fd_handler (NCDInterfaceMonitor *o, int events);
static void process_buffer (NCDInterfaceMonitor *o);
static void more_job_handler (NCDInterfaceMonitor *o);

static int send_wilddump_request (NCDInterfaceMonitor *o, int fd, uint32_t seq, int family, int type)
{
    struct {
        struct nlmsghdr nlh;
        struct rtgenmsg g;
    } req;
    
    memset(&req, 0, sizeof(req));
    req.nlh.nlmsg_len = sizeof(req);
    req.nlh.nlmsg_type = type;
    req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
    req.nlh.nlmsg_pid = 0;
    req.nlh.nlmsg_seq = seq;
    req.g.rtgen_family = family;
    
    int res = write(fd, &req, sizeof(req));
    if (res < 0) {
        BLog(BLOG_ERROR, "write failed");
        return 0;
    }
    if (res != sizeof(req)) {
        BLog(BLOG_ERROR, "write short");
        return 0;
    }
    
    return 1;
}

static int get_attr (int type, struct rtattr *rta, int rta_len, void **out_attr, int *out_attr_len)
{
    for (; RTA_OK(rta, rta_len); rta = RTA_NEXT(rta, rta_len)) {
        uint8_t *attr = RTA_DATA(rta);
        int attr_len = RTA_PAYLOAD(rta);
        
        if (rta->rta_type == type) {
            *out_attr = attr;
            *out_attr_len = attr_len;
            return 1;
        }
    }
    
    return 0;
}

static void report_error (NCDInterfaceMonitor *o)
{
    DEBUGERROR(&o->d_err, o->handler_error(o->user))
}

static int send_next_dump_request (NCDInterfaceMonitor *o)
{
    ASSERT(o->dump_queue)
    
    if (o->dump_queue & NCDIFMONITOR_WATCH_LINK) {
        o->dump_queue &= ~NCDIFMONITOR_WATCH_LINK;
        return send_wilddump_request(o, o->netlink_fd, o->dump_seq, 0, RTM_GETLINK);
    }
    else if (o->dump_queue & NCDIFMONITOR_WATCH_IPV4_ADDR) {
        o->dump_queue &= ~NCDIFMONITOR_WATCH_IPV4_ADDR;
        return send_wilddump_request(o, o->netlink_fd, o->dump_seq, AF_INET, RTM_GETADDR);
    }
    else if (o->dump_queue & NCDIFMONITOR_WATCH_IPV6_ADDR) {
        o->dump_queue &= ~NCDIFMONITOR_WATCH_IPV6_ADDR;
        return send_wilddump_request(o, o->netlink_fd, o->dump_seq, AF_INET6, RTM_GETADDR);
    }
    
    ASSERT(0)
    return 0;
}

void netlink_fd_handler (NCDInterfaceMonitor *o, int events)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_bfd)
    
    // handler fd error
    if (o->buf_left >= 0) {
        BLog(BLOG_ERROR, "file descriptor error");
        goto fail;
    }
    
    // read from netlink fd
    int len = read(o->netlink_fd, o->buf.buf, sizeof(o->buf));
    if (len < 0) {
        BLog(BLOG_ERROR, "read failed");
        goto fail;
    }
    
    // stop receiving fd events
    BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, 0);
    
    // set buffer
    o->buf_nh = &o->buf.nlh;
    o->buf_left = len;
    
    // process buffer
    process_buffer(o);
    return;
    
fail:
    report_error(o);
}

void process_buffer (NCDInterfaceMonitor *o)
{
    ASSERT(o->buf_left >= 0)
    ASSERT(o->have_bfd)
    
    int done = 0;
    
    for (; NLMSG_OK(o->buf_nh, o->buf_left); o->buf_nh = NLMSG_NEXT(o->buf_nh, o->buf_left)) {
        if (o->buf_nh->nlmsg_type == NLMSG_DONE) {
            done = 1;
            break;
        }
        
        struct nlmsghdr *buf = o->buf_nh;
        void *pl = NLMSG_DATA(buf);
        int pl_len = NLMSG_PAYLOAD(buf, 0);
        
        struct NCDInterfaceMonitor_event ev;
        
        switch (buf->nlmsg_type) {
            case RTM_NEWLINK: { // not RTM_DELLINK! who knows what these mean...
                if (pl_len < sizeof(struct ifinfomsg)) {
                    BLog(BLOG_ERROR, "ifinfomsg too short");
                    goto fail;
                }
                struct ifinfomsg *msg = pl;
                
                if (msg->ifi_index == o->ifindex && (o->watch_events & NCDIFMONITOR_WATCH_LINK)) {
                    ev.event = (buf->nlmsg_type == RTM_NEWLINK && (msg->ifi_flags & IFF_RUNNING)) ? NCDIFMONITOR_EVENT_LINK_UP : NCDIFMONITOR_EVENT_LINK_DOWN;
                    goto dispatch;
                }
            } break;
            
            case RTM_NEWADDR:
            case RTM_DELADDR: {
                if (pl_len < sizeof(struct ifaddrmsg)) {
                    BLog(BLOG_ERROR, "ifaddrmsg too short");
                    goto fail;
                }
                struct ifaddrmsg *msg = pl;
                
                void *addr;
                int addr_len;
                if (!get_attr(IFA_ADDRESS, IFA_RTA(msg), buf->nlmsg_len - NLMSG_LENGTH(sizeof(*msg)), &addr, &addr_len)) {
                    break;
                }
                
                if (msg->ifa_index == o->ifindex && msg->ifa_family == AF_INET && (o->watch_events & NCDIFMONITOR_WATCH_IPV4_ADDR)) {
                    if (addr_len != 4 || msg->ifa_prefixlen > 32) {
                        BLog(BLOG_ERROR, "bad ipv4 ifaddrmsg");
                        goto fail;
                    }
                    
                    ev.event = (buf->nlmsg_type == RTM_NEWADDR) ? NCDIFMONITOR_EVENT_IPV4_ADDR_ADDED : NCDIFMONITOR_EVENT_IPV4_ADDR_REMOVED;
                    ev.u.ipv4_addr.addr.addr = ((struct in_addr *)addr)->s_addr;
                    ev.u.ipv4_addr.addr.prefix = msg->ifa_prefixlen;
                    goto dispatch;
                }
                
                if (msg->ifa_index == o->ifindex && msg->ifa_family == AF_INET6 && (o->watch_events & NCDIFMONITOR_WATCH_IPV6_ADDR)) {
                    if (addr_len != 16 || msg->ifa_prefixlen > 128) {
                        BLog(BLOG_ERROR, "bad ipv6 ifaddrmsg");
                        goto fail;
                    }
                    
                    ev.event = (buf->nlmsg_type == RTM_NEWADDR) ? NCDIFMONITOR_EVENT_IPV6_ADDR_ADDED : NCDIFMONITOR_EVENT_IPV6_ADDR_REMOVED;
                    memcpy(ev.u.ipv6_addr.addr.addr.bytes, ((struct in6_addr *)addr)->s6_addr, 16);
                    ev.u.ipv6_addr.addr.prefix = msg->ifa_prefixlen;
                    ev.u.ipv6_addr.addr_flags = 0;
                    ev.u.ipv6_addr.scope = msg->ifa_scope;
                    if (!(msg->ifa_flags & IFA_F_PERMANENT)) {
                        ev.u.ipv6_addr.addr_flags |= NCDIFMONITOR_ADDR_FLAG_DYNAMIC;
                    }
                    goto dispatch;
                }
            } break;
        }
        
        continue;
        
    dispatch:
        // move to next message
        o->buf_nh = NLMSG_NEXT(o->buf_nh, o->buf_left);
        
        // schedule more job
        BPending_Set(&o->more_job);
        
        // dispatch event
        o->handler(o->user, ev);
        return;
    }
    
    if (done) {
        if (o->dump_queue) {
            // increment dump request sequence number
            o->dump_seq++;
            
            // send next dump request
            if (!send_next_dump_request(o)) {
                goto fail;
            }
        }
        else if (o->event_netlink_fd >= 0) {
            // stop watching dump fd
            BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
            o->have_bfd = 0;
            
            // close dump fd, make event fd current
            close(o->netlink_fd);
            o->netlink_fd = o->event_netlink_fd;
            o->event_netlink_fd = -1;
            
            // start watching event fd
            BFileDescriptor_Init(&o->bfd, o->netlink_fd, (BFileDescriptor_handler)netlink_fd_handler, o);
            if (!BReactor_AddFileDescriptor(o->reactor, &o->bfd)) {
                BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
                goto fail;
            }
            o->have_bfd = 1;
        }
    }
    
    // set no buffer
    o->buf_left = -1;
    
    // continue receiving fd events
    BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, BREACTOR_READ);
    return;
    
fail:
    report_error(o);
}

void more_job_handler (NCDInterfaceMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->buf_left >= 0)
    
    // process buffer
    process_buffer(o);
    return;
}

int NCDInterfaceMonitor_Init (NCDInterfaceMonitor *o, int ifindex, int watch_events, BReactor *reactor, void *user,
                              NCDInterfaceMonitor_handler handler,
                              NCDInterfaceMonitor_handler_error handler_error)
{
    ASSERT(watch_events)
    ASSERT((watch_events&~(NCDIFMONITOR_WATCH_LINK|NCDIFMONITOR_WATCH_IPV4_ADDR|NCDIFMONITOR_WATCH_IPV6_ADDR)) == 0)
    ASSERT(handler)
    ASSERT(handler_error)
    BNetwork_Assert();
    
    // init arguments
    o->ifindex = ifindex;
    o->watch_events = watch_events;
    o->reactor = reactor;
    o->user = user;
    o->handler = handler;
    o->handler_error = handler_error;
    
    // init dump netlink fd
    if ((o->netlink_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        BLog(BLOG_ERROR, "socket failed");
        goto fail0;
    }
    if (!badvpn_set_nonblocking(o->netlink_fd)) {
        BLog(BLOG_ERROR, "badvpn_set_nonblocking failed");
        goto fail1;
    }
    
    // init event netlink fd
    if ((o->event_netlink_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        BLog(BLOG_ERROR, "socket failed");
        goto fail1;
    }
    if (!badvpn_set_nonblocking(o->event_netlink_fd)) {
        BLog(BLOG_ERROR, "badvpn_set_nonblocking failed");
        goto fail2;
    }
    
    // build bind address
    struct sockaddr_nl sa;
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = 0;
    if (watch_events & NCDIFMONITOR_WATCH_LINK) sa.nl_groups |= RTMGRP_LINK;
    if (watch_events & NCDIFMONITOR_WATCH_IPV4_ADDR) sa.nl_groups |= RTMGRP_IPV4_IFADDR;
    if (watch_events & NCDIFMONITOR_WATCH_IPV6_ADDR) sa.nl_groups |= RTMGRP_IPV6_IFADDR;
    
    // bind event netlink fd
    if (bind(o->event_netlink_fd, (void *)&sa, sizeof(sa)) < 0) {
        BLog(BLOG_ERROR, "bind failed");
        goto fail2;
    }
    
    // set dump state
    o->dump_queue = watch_events;
    o->dump_seq = 0;
    
    // init BFileDescriptor
    BFileDescriptor_Init(&o->bfd, o->netlink_fd, (BFileDescriptor_handler)netlink_fd_handler, o);
    if (!BReactor_AddFileDescriptor(reactor, &o->bfd)) {
        BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
        goto fail2;
    }
    o->have_bfd = 1;
    
    // set nothing in buffer
    o->buf_left = -1;
    
    // init more job
    BPending_Init(&o->more_job, BReactor_PendingGroup(reactor), (BPending_handler)more_job_handler, o);
    
    // send first dump request
    if (!send_next_dump_request(o)) {
        goto fail3;
    }
    
    // wait for reading fd
    BReactor_SetFileDescriptorEvents(reactor, &o->bfd, BREACTOR_READ);
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail3:
    BPending_Free(&o->more_job);
    BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
fail2:
    close(o->event_netlink_fd);
fail1:
    close(o->netlink_fd);
fail0:
    return 0;
}

void NCDInterfaceMonitor_Free (NCDInterfaceMonitor *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    
    // free more job
    BPending_Free(&o->more_job);
    
    // free BFileDescriptor
    if (o->have_bfd) {
        BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
    }
    
    // close event fd, in case we're still dumping
    if (o->event_netlink_fd >= 0) {
        close(o->event_netlink_fd);
    }
    
    // close fd
    close(o->netlink_fd);
}

void NCDInterfaceMonitor_Pause (NCDInterfaceMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->have_bfd)
    
    if (o->buf_left >= 0) {
        BPending_Unset(&o->more_job);
    } else {
        BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, 0);
    }
}

void NCDInterfaceMonitor_Continue (NCDInterfaceMonitor *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(o->have_bfd)
    
    if (o->buf_left >= 0) {
        BPending_Set(&o->more_job);
    } else {
        BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, BREACTOR_READ);
    }
}
