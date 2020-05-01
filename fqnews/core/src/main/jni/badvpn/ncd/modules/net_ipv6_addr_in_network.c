/**
 * @file net_ipv6_addr_in_network.c
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
 *   net.ipv6.addr_in_network(string addr, string net_addr, string net_prefix)
 *   net.ipv6.addr_in_network(string addr, string cidr_net_addr)
 *   net.ipv6.ifnot_addr_in_network(string addr, string net_addr, string net_prefix)
 *   net.ipv6.ifnot_addr_in_network(string addr, string cidr_net_addr)
 * 
 * Description:
 *   Checks if two IPv6 addresses belong to the same subnet.
 *   The prefix length is given either in the a separate argument or along with
 *   the second address in CIDR notation (address/prefix).
 *   This can be used to check whether an address belongs to a certain
 *   subnet, hence the name.
 * 
 * Variables:
 *   (empty) - "true" if addresses belong to the same subnet, "false" if not
 */

#include <string.h>

#include <misc/ipaddr6.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_ipv6_addr_in_network.h>

struct instance {
    NCDModuleInst *i;
    int value;
};

static void func_new_common (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int is_ifnot)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef arg_addr;
    NCDValRef arg_net_addr;
    NCDValRef arg_net_prefix = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 2, &arg_addr, &arg_net_addr) &&
        !NCDVal_ListRead(params->args, 3, &arg_addr, &arg_net_addr, &arg_net_prefix) 
    ) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(arg_addr) || !NCDVal_IsString(arg_net_addr) ||
        (!NCDVal_IsInvalid(arg_net_prefix) && !NCDVal_IsString(arg_net_prefix))
    ) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // parse addr
    struct ipv6_addr addr;
    if (!ipaddr6_parse_ipv6_addr(NCDVal_StringMemRef(arg_addr), &addr)) {
        ModuleLog(o->i, BLOG_ERROR, "bad address");
        goto fail0;
    }
    
    // parse network
    struct ipv6_ifaddr network;
    if (NCDVal_IsInvalid(arg_net_prefix)) {
        if (!ipaddr6_parse_ipv6_ifaddr(NCDVal_StringMemRef(arg_net_addr), &network)) {
            ModuleLog(o->i, BLOG_ERROR, "bad network in CIDR notation");
            goto fail0;
        }
    } else {
        if (!ipaddr6_parse_ipv6_addr(NCDVal_StringMemRef(arg_net_addr), &network.addr)) {
            ModuleLog(o->i, BLOG_ERROR, "bad network address");
            goto fail0;
        }
        if (!ipaddr6_parse_ipv6_prefix(NCDVal_StringMemRef(arg_net_prefix), &network.prefix)) {
            ModuleLog(o->i, BLOG_ERROR, "bad network prefix");
            goto fail0;
        }
    }
    
    // test
    o->value = ipaddr6_ipv6_addrs_in_network(addr, network.addr, network.prefix);
    
    if (is_ifnot && o->value) {
        ModuleLog(o->i, BLOG_ERROR, "addresses belong to same subnet, not proceeding");
    }
    
    // signal up
    if (!is_ifnot || !o->value) {
        NCDModuleInst_Backend_Up(o->i);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_normal (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_common(vo, i, params, 0);
}

static void func_new_ifnot (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_common(vo, i, params, 1);
}

static int func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (!strcmp(name, "")) {
        *out = ncd_make_boolean(mem, o->value);
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "net.ipv6.addr_in_network",
        .func_new2 = func_new_normal,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.ipv6.ifnot_addr_in_network",
        .func_new2 = func_new_ifnot,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_ipv6_addr_in_network = {
    .modules = modules
};
