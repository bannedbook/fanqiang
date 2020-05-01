/**
 * @file NCDInterfaceMonitor.h
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

#ifndef BADVPN_NCD_NCDINTERFACEMONITOR_H
#define BADVPN_NCD_NCDINTERFACEMONITOR_H

#include <stdint.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include <misc/debug.h>
#include <misc/ipaddr.h>
#include <misc/ipaddr6.h>
#include <misc/debugerror.h>
#include <base/DebugObject.h>
#include <system/BReactor.h>
#include <system/BNetwork.h>

#define NCDIFMONITOR_WATCH_LINK (1 << 0)
#define NCDIFMONITOR_WATCH_IPV4_ADDR (1 << 1)
#define NCDIFMONITOR_WATCH_IPV6_ADDR (1 << 2)

#define NCDIFMONITOR_EVENT_LINK_UP 1
#define NCDIFMONITOR_EVENT_LINK_DOWN 2
#define NCDIFMONITOR_EVENT_IPV4_ADDR_ADDED 3
#define NCDIFMONITOR_EVENT_IPV4_ADDR_REMOVED 4
#define NCDIFMONITOR_EVENT_IPV6_ADDR_ADDED 5
#define NCDIFMONITOR_EVENT_IPV6_ADDR_REMOVED 6

#define NCDIFMONITOR_ADDR_FLAG_DYNAMIC (1 << 0)

struct NCDInterfaceMonitor_event {
    int event;
    union {
        struct {
            struct ipv4_ifaddr addr;
        } ipv4_addr;
        struct {
            struct ipv6_ifaddr addr;
            int addr_flags;
            uint8_t scope;
        } ipv6_addr;
    } u;
};

/**
 * Handler called to report an interface event.
 * Note that the event reporter does not keep any interface state, and as such may
 * report redundant events. You should therefore handle events in an idempotent
 * fashion.
 * 
 * @param event.event event type. One of:
 *        - NCDIFMONITOR_EVENT_LINK_UP, NCDIFMONITOR_EVENT_LINK_DOWN,
 *        - NCDIFMONITOR_EVENT_IPV4_ADDR_ADDED, NCDIFMONITOR_EVENT_IPV4_ADDR_REMOVED,
 *        - NCDIFMONITOR_EVENT_IPV6_ADDR_ADDED, NCDIFMONITOR_EVENT_IPV6_ADDR_REMOVED.
 *        Only events that correspont to enabled watch flags are reported.
 * @param event.ipv4_addr.addr the IPv4 address and prefix length
 * @param event.ipv6_addr.addr the IPv6 address, prefix length and scope
 * @param event.ipv6_addr.addr_flags IPv6 address flags. Valid flags:
 *        - NCDIFMONITOR_ADDR_FLAG_DYNAMIC - this address was assigned dynamically (NDP)
 */
typedef void (*NCDInterfaceMonitor_handler) (void *user, struct NCDInterfaceMonitor_event event);

/**
 * Handler called when an error occurs.
 * The event reporter must be freed from within the job context of this
 * handler, and no other operations must be performed.
 */
typedef void (*NCDInterfaceMonitor_handler_error) (void *user);

/**
 * Watches for network interface events using a Linux rtnetlink socket.
 */
typedef struct {
    int ifindex;
    int watch_events;
    BReactor *reactor;
    void *user;
    NCDInterfaceMonitor_handler handler;
    NCDInterfaceMonitor_handler_error handler_error;
    int netlink_fd;
    int event_netlink_fd;
    int dump_queue;
    uint32_t dump_seq;
    BFileDescriptor bfd;
    int have_bfd;
    union {
        uint8_t buf[4096];
        struct nlmsghdr nlh;
    } buf;
    struct nlmsghdr *buf_nh;
    int buf_left;
    BPending more_job;
    DebugError d_err;
    DebugObject d_obj;
} NCDInterfaceMonitor;

/**
 * Initializes the event reporter.
 * The reporter is not paused initially.
 * {@link BNetwork_GlobalInit} must have been done.
 * 
 * @param ifindex index of network interface to report events for
 * @param watch_events mask specifying what kind of events to report. Valid flags are
 *        NCDIFMONITOR_WATCH_LINK, NCDIFMONITOR_WATCH_IPV4_ADDR, NCDIFMONITOR_WATCH_IPV6_ADDR.
 *        At least one flag must be provided.
 * @param reactor reactor we live in
 * @param user argument to handlers
 * @param handler handler to report interface events to
 * @param handler_error error handler
 * @return 1 on success, 0 on failure
 */
int NCDInterfaceMonitor_Init (NCDInterfaceMonitor *o, int ifindex, int watch_events, BReactor *reactor, void *user,
                              NCDInterfaceMonitor_handler handler,
                              NCDInterfaceMonitor_handler_error handler_error) WARN_UNUSED;

/**
 * Frees the event reporter.
 */
void NCDInterfaceMonitor_Free (NCDInterfaceMonitor *o);

/**
 * Pauses event reporting.
 * This operation is idempotent.
 */
void NCDInterfaceMonitor_Pause (NCDInterfaceMonitor *o);

/**
 * Resumes event reporting.
 * This operation is idempotent.
 */
void NCDInterfaceMonitor_Continue (NCDInterfaceMonitor *o);

#endif
