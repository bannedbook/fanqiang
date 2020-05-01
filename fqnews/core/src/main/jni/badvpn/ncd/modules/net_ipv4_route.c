/**
 * @file net_ipv4_route.c
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
 * IPv4 route module.
 * 
 * Synopsis:
 *     net.ipv4.route(string dest, string dest_prefix, string gateway, string metric, string ifname)
 *     net.ipv4.route(string cidr_dest, string gateway, string metric, string ifname)
 * 
 * Description:
 *     Adds an IPv4 route to the system's routing table on initiailzation, and
 *     removes it on deinitialization. The second form takes the destination in
 *     CIDR notation (a.b.c.d/n).
 *     If 'gateway' is "none", the route will only be associated with an interface.
 *     If 'gateway' is "blackhole", the route will be a blackhole route (and 'ifname' is unused).
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <misc/debug.h>
#include <ncd/extra/NCDIfConfig.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_ipv4_route.h>

#define TYPE_NORMAL 1
#define TYPE_IFONLY 2
#define TYPE_BLACKHOLE 3

struct instance {
    NCDModuleInst *i;
    struct ipv4_ifaddr dest;
    int type;
    uint32_t gateway;
    int metric;
    NCDValNullTermString ifname_nts;
};

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef dest_arg;
    NCDValRef dest_prefix_arg = NCDVal_NewInvalid();
    NCDValRef gateway_arg;
    NCDValRef metric_arg;
    NCDValRef ifname_arg;
    if (!NCDVal_ListRead(params->args, 4, &dest_arg, &gateway_arg, &metric_arg, &ifname_arg) &&
        !NCDVal_ListRead(params->args, 5, &dest_arg, &dest_prefix_arg, &gateway_arg, &metric_arg, &ifname_arg)
    ) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(dest_arg) || !NCDVal_IsString(gateway_arg) ||
        !NCDVal_IsStringNoNulls(ifname_arg) ||
        (!NCDVal_IsInvalid(dest_prefix_arg) && !NCDVal_IsString(dest_prefix_arg))
    ) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // read dest
    if (NCDVal_IsInvalid(dest_prefix_arg)) {
        if (!ipaddr_parse_ipv4_ifaddr(NCDVal_StringMemRef(dest_arg), &o->dest)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong CIDR notation dest");
            goto fail0;
        }
    } else {
        if (!ipaddr_parse_ipv4_addr(NCDVal_StringMemRef(dest_arg), &o->dest.addr)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong dest addr");
            goto fail0;
        }
        if (!ipaddr_parse_ipv4_prefix(NCDVal_StringMemRef(dest_prefix_arg), &o->dest.prefix)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong dest prefix");
            goto fail0;
        }
    }
    
    // read gateway and choose type
    if (NCDVal_StringEquals(gateway_arg, "none")) {
        o->type = TYPE_IFONLY;
    }
    else if (NCDVal_StringEquals(gateway_arg, "blackhole")) {
        o->type = TYPE_BLACKHOLE;
    } else {
        if (!ipaddr_parse_ipv4_addr(NCDVal_StringMemRef(gateway_arg), &o->gateway)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong gateway");
            goto fail0;
        }
        o->type = TYPE_NORMAL;
    }
    
    // read metric
    uintmax_t metric;
    if (!ncd_read_uintmax(metric_arg, &metric) || metric > INT_MAX) {
        ModuleLog(i, BLOG_ERROR, "bad metric");
        goto fail0;
    }
    o->metric = metric;
    
    // null terminate ifname
    if (!NCDVal_StringNullTerminate(ifname_arg, &o->ifname_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    // add route
    int res = 0; // to remove warning
    switch (o->type) {
        case TYPE_NORMAL:
            res = NCDIfConfig_add_ipv4_route(o->dest, &o->gateway, o->metric, o->ifname_nts.data);
            break;
        case TYPE_IFONLY:
            res = NCDIfConfig_add_ipv4_route(o->dest, NULL, o->metric, o->ifname_nts.data);
            break;
        case TYPE_BLACKHOLE:
            res = NCDIfConfig_add_ipv4_blackhole_route(o->dest, o->metric);
            break;
        default: ASSERT(0);
    }
    if (!res) {
        ModuleLog(o->i, BLOG_ERROR, "failed to add route");
        goto fail1;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail1:
    NCDValNullTermString_Free(&o->ifname_nts);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    // remove route
    int res = 0; // to remove warning
    switch (o->type) {
        case TYPE_NORMAL:
            res = NCDIfConfig_remove_ipv4_route(o->dest, &o->gateway, o->metric, o->ifname_nts.data);
            break;
        case TYPE_IFONLY:
            res = NCDIfConfig_remove_ipv4_route(o->dest, NULL, o->metric, o->ifname_nts.data);
            break;
        case TYPE_BLACKHOLE:
            res = NCDIfConfig_remove_ipv4_blackhole_route(o->dest, o->metric);
            break;
        default: ASSERT(0);
    }
    if (!res) {
        ModuleLog(o->i, BLOG_ERROR, "failed to remove route");
    }
    
    // free ifname nts
    NCDValNullTermString_Free(&o->ifname_nts);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static struct NCDModule modules[] = {
    {
        .type = "net.ipv4.route",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_ipv4_route = {
    .modules = modules
};
