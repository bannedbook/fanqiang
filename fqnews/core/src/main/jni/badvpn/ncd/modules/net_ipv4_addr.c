/**
 * @file net_ipv4_addr.c
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
 * IPv4 address module.
 * 
 * Synopsis:
 *     net.ipv4.addr(string ifname, string addr, string prefix)
 *     net.ipv4.addr(string ifname, string cidr_addr)
 * 
 * Description:
 *     Adds the given address to the given network interface on initialization,
 *     and removes it on deinitialization. The second form takes the address and
 *     prefix in CIDR notation (a.b.c.d/n).
 */

#include <stdlib.h>
#include <string.h>

#include <ncd/extra/NCDIfConfig.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_ipv4_addr.h>

struct instance {
    NCDModuleInst *i;
    NCDValNullTermString ifname_nts;
    struct ipv4_ifaddr ifaddr;
};

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef ifname_arg;
    NCDValRef addr_arg;
    NCDValRef prefix_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 2, &ifname_arg, &addr_arg) &&
        !NCDVal_ListRead(params->args, 3, &ifname_arg, &addr_arg, &prefix_arg)
    ) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(ifname_arg) || !NCDVal_IsString(addr_arg) ||
        (!NCDVal_IsInvalid(prefix_arg) && !NCDVal_IsString(prefix_arg))
    ) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // null terminate ifname
    if (!NCDVal_StringNullTerminate(ifname_arg, &o->ifname_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    if (NCDVal_IsInvalid(prefix_arg)) {
        if (!ipaddr_parse_ipv4_ifaddr(NCDVal_StringMemRef(addr_arg), &o->ifaddr)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong CIDR notation address");
            goto fail1;
        }
    } else {
        if (!ipaddr_parse_ipv4_addr(NCDVal_StringMemRef(addr_arg), &o->ifaddr.addr)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong address");
            goto fail1;
        }
        
        if (!ipaddr_parse_ipv4_prefix(NCDVal_StringMemRef(prefix_arg), &o->ifaddr.prefix)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong prefix");
            goto fail1;
        }
    }
    
    // add address
    if (!NCDIfConfig_add_ipv4_addr(o->ifname_nts.data, o->ifaddr)) {
        ModuleLog(o->i, BLOG_ERROR, "failed to add IP address");
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
    
    // remove address
    if (!NCDIfConfig_remove_ipv4_addr(o->ifname_nts.data, o->ifaddr)) {
        ModuleLog(o->i, BLOG_ERROR, "failed to remove IP address");
    }
    
    // free ifname nts
    NCDValNullTermString_Free(&o->ifname_nts);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static struct NCDModule modules[] = {
    {
        .type = "net.ipv4.addr",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_ipv4_addr = {
    .modules = modules
};
