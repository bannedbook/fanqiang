/**
 * @file ncdinterfacemonitor_test.c
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
#include <inttypes.h>
#include <stdio.h>

#include <misc/get_iface_info.h>
#include <misc/ipaddr6.h>
#include <misc/debug.h>
#include <base/BLog.h>
#include <system/BTime.h>
#include <system/BReactor.h>
#include <system/BSignal.h>
#include <ncd/extra/NCDInterfaceMonitor.h>

BReactor reactor;
NCDInterfaceMonitor monitor;

static void signal_handler (void *user);
static void monitor_handler (void *unused, struct NCDInterfaceMonitor_event event);
static void monitor_handler_error (void *unused);

int main (int argc, char **argv)
{
    int ret = 1;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <interface>\n", (argc > 0 ? argv[0] : ""));
        goto fail0;
    }
    
    int ifindex;
    if (!badvpn_get_iface_info(argv[1], NULL, NULL, &ifindex)) {
        DEBUG("get_iface_info failed");
        goto fail0;
    }
    
    BTime_Init();
    
    BLog_InitStdout();
    
    if (!BNetwork_GlobalInit()) {
        DEBUG("BNetwork_GlobalInit failed");
        goto fail1;
    }
    
    if (!BReactor_Init(&reactor)) {
        DEBUG("BReactor_Init failed");
        goto fail1;
    }
    
    if (!BSignal_Init(&reactor, signal_handler, NULL)) {
        DEBUG("BSignal_Init failed");
        goto fail2;
    }
    
    int watch_flags = NCDIFMONITOR_WATCH_LINK|NCDIFMONITOR_WATCH_IPV4_ADDR|NCDIFMONITOR_WATCH_IPV6_ADDR;
    
    if (!NCDInterfaceMonitor_Init(&monitor, ifindex, watch_flags, &reactor, NULL, monitor_handler, monitor_handler_error)) {
        DEBUG("NCDInterfaceMonitor_Init failed");
        goto fail3;
    }
    
    ret = BReactor_Exec(&reactor);
    
    NCDInterfaceMonitor_Free(&monitor);
fail3:
    BSignal_Finish();
fail2:
    BReactor_Free(&reactor);
fail1:
    BLog_Free();
fail0:
    DebugObjectGlobal_Finish();
    
    return ret;
}

void signal_handler (void *user)
{
    DEBUG("termination requested");
    
    BReactor_Quit(&reactor, 1);
}

void monitor_handler (void *unused, struct NCDInterfaceMonitor_event event)
{
    switch (event.event) {
        case NCDIFMONITOR_EVENT_LINK_UP:
        case NCDIFMONITOR_EVENT_LINK_DOWN: {
            const char *type = (event.event == NCDIFMONITOR_EVENT_LINK_UP) ? "up" : "down";
            printf("link %s\n", type);
        } break;
        
        case NCDIFMONITOR_EVENT_IPV4_ADDR_ADDED:
        case NCDIFMONITOR_EVENT_IPV4_ADDR_REMOVED: {
            const char *type = (event.event == NCDIFMONITOR_EVENT_IPV4_ADDR_ADDED) ? "added" : "removed";
            uint8_t *addr = (uint8_t *)&event.u.ipv4_addr.addr;
            printf("ipv4 addr %s %d.%d.%d.%d/%d\n", type, (int)addr[0], (int)addr[1], (int)addr[2], (int)addr[3], event.u.ipv4_addr.addr.prefix);
        } break;
        
        case NCDIFMONITOR_EVENT_IPV6_ADDR_ADDED:
        case NCDIFMONITOR_EVENT_IPV6_ADDR_REMOVED: {
            const char *type = (event.event == NCDIFMONITOR_EVENT_IPV6_ADDR_ADDED) ? "added" : "removed";
            
            char str[IPADDR6_PRINT_MAX];
            ipaddr6_print_addr(event.u.ipv6_addr.addr.addr, str);
            
            int dynamic = !!(event.u.ipv6_addr.addr_flags & NCDIFMONITOR_ADDR_FLAG_DYNAMIC);
            
            printf("ipv6 addr %s %s/%d scope=%"PRIu8" dynamic=%d\n", type, str, event.u.ipv6_addr.addr.prefix, event.u.ipv6_addr.scope, dynamic);
        } break;
        
        default: ASSERT(0);
    }
}

void monitor_handler_error (void *unused)
{
    DEBUG("monitor error");
    
    BReactor_Quit(&reactor, 1);
}
