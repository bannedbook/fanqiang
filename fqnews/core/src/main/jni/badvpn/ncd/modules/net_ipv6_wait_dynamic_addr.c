/**
 * @file net_ipv6_wait_dynamic_addr.c
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
 * 
 * @section DESCRIPTION
 * 
 * Synopsis:
 *   net.ipv6.wait_dynamic_addr(string ifname)
 * 
 * Description:
 *   Waits for a dynamic IPv6 address to be obtained on the interface,
 *   and goes up when it is obtained.
 *   If the address is subsequently lost, goes back down and again waits
 *   for an address.
 * 
 * Variables:
 *   string addr - dynamic address obtained on the interface
 *   string prefix - prefix length
 *   string cidr_addr - address and prefix (addr/prefix)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <misc/get_iface_info.h>
#include <misc/ipaddr6.h>
#include <ncd/extra/NCDInterfaceMonitor.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_ipv6_wait_dynamic_addr.h>

struct instance {
    NCDModuleInst *i;
    NCDInterfaceMonitor monitor;
    struct ipv6_ifaddr ifaddr;
    int up;
};

static void instance_free (struct instance *o, int is_error);

static void monitor_handler (struct instance *o, struct NCDInterfaceMonitor_event event)
{
    if (!o->up && event.event == NCDIFMONITOR_EVENT_IPV6_ADDR_ADDED && (event.u.ipv6_addr.addr_flags & NCDIFMONITOR_ADDR_FLAG_DYNAMIC)) {
        // rememeber address, set up
        o->ifaddr = event.u.ipv6_addr.addr;
        o->up = 1;
        
        // signal up
        NCDModuleInst_Backend_Up(o->i);
    }
    else if (o->up && event.event == NCDIFMONITOR_EVENT_IPV6_ADDR_REMOVED && !memcmp(event.u.ipv6_addr.addr.addr.bytes, o->ifaddr.addr.bytes, 16) && event.u.ipv6_addr.addr.prefix == o->ifaddr.prefix) {
        // set not up
        o->up = 0;
        
        // signal down
        NCDModuleInst_Backend_Down(o->i);
    }
}

static void monitor_handler_error (struct instance *o)
{
    ModuleLog(o->i, BLOG_ERROR, "monitor error");
    
    instance_free(o, 1);
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef ifname_arg;
    if (!NCDVal_ListRead(params->args, 1, &ifname_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(ifname_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // null terminate ifname
    NCDValNullTermString ifname_nts;
    if (!NCDVal_StringNullTerminate(ifname_arg, &ifname_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    // get interface index
    int ifindex;
    int res = badvpn_get_iface_info(ifname_nts.data, NULL, NULL, &ifindex);
    NCDValNullTermString_Free(&ifname_nts);
    if (!res) {
        ModuleLog(o->i, BLOG_ERROR, "failed to get interface index");
        goto fail0;
    }
    
    // init monitor
    if (!NCDInterfaceMonitor_Init(&o->monitor, ifindex, NCDIFMONITOR_WATCH_IPV6_ADDR, i->params->iparams->reactor, o, (NCDInterfaceMonitor_handler)monitor_handler, (NCDInterfaceMonitor_handler_error)monitor_handler_error)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDInterfaceMonitor_Init failed");
        goto fail0;
    }
    
    // set not up
    o->up = 0;
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void instance_free (struct instance *o, int is_error)
{
    // free monitor
    NCDInterfaceMonitor_Free(&o->monitor);
    
    if (is_error) {
        NCDModuleInst_Backend_DeadError(o->i);
    } else {
        NCDModuleInst_Backend_Dead(o->i);
    }
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    instance_free(o, 0);
}

static int func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    ASSERT(o->up)
    
    if (!strcmp(name, "addr")) {
        char str[IPADDR6_PRINT_MAX];
        ipaddr6_print_addr(o->ifaddr.addr, str);
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    if (!strcmp(name, "prefix")) {
        char str[10];
        sprintf(str, "%d", o->ifaddr.prefix);
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    if (!strcmp(name, "cidr_addr")) {
        char str[IPADDR6_PRINT_MAX];
        ipaddr6_print_ifaddr(o->ifaddr, str);
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "net.ipv6.wait_dynamic_addr",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_ipv6_wait_dynamic_addr = {
    .modules = modules
};
