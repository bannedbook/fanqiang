/**
 * @file netmask.c
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
 *   ipv4_prefix_to_mask(string prefix)
 * 
 * Variables:
 *   string (empty) - prefix, converted to dotted decimal format without leading
 *                    zeros
 * 
 * Synopsis:
 *   ipv4_mask_to_prefix(string mask)
 * 
 * Variables:
 *   string (empty) - mask, converted to prefix length
 * 
 * Synopsis:
 *   ipv4_net_from_addr_and_prefix(string addr, string prefix)
 * 
 * Variables:
 *   string (empty) - network part of the address, according to given prefix
 *                    length, in dotted decimal format without leading zeros
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <misc/ipaddr.h>
#include <misc/parse_number.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_netmask.h>

struct addr_instance {
    NCDModuleInst *i;
    uint32_t addr;
};

struct prefix_instance {
    NCDModuleInst *i;
    int prefix;
};

static void addr_func_init_templ (void *vo, NCDModuleInst *i, uint32_t addr)
{
    struct addr_instance *o = vo;
    o->i = i;
    
    // remember address
    o->addr = addr;
    
    // signal up
    NCDModuleInst_Backend_Up(i);
}

static void prefix_to_mask_func_init (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    NCDValRef prefix_arg;
    if (!NCDVal_ListRead(params->args, 1, &prefix_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(prefix_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // parse prefix
    int prefix;
    if (!ipaddr_parse_ipv4_prefix(NCDVal_StringMemRef(prefix_arg), &prefix)) {
        ModuleLog(i, BLOG_ERROR, "bad prefix");
        goto fail0;
    }
    
    // make mask
    uint32_t mask = ipaddr_ipv4_mask_from_prefix(prefix);
    
    addr_func_init_templ(vo, i, mask);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void ipv4_net_from_addr_and_prefix_func_init (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    NCDValRef addr_arg;
    NCDValRef prefix_arg;
    if (!NCDVal_ListRead(params->args, 2, &addr_arg, &prefix_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(addr_arg) || !NCDVal_IsString(prefix_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // parse addr
    uint32_t addr;
    if (!ipaddr_parse_ipv4_addr(NCDVal_StringMemRef(addr_arg), &addr)) {
        ModuleLog(i, BLOG_ERROR, "bad addr");
        goto fail0;
    }
    
    // parse prefix
    int prefix;
    if (!ipaddr_parse_ipv4_prefix(NCDVal_StringMemRef(prefix_arg), &prefix)) {
        ModuleLog(i, BLOG_ERROR, "bad prefix");
        goto fail0;
    }
    
    // make network
    uint32_t network = (addr & ipaddr_ipv4_mask_from_prefix(prefix));
    
    addr_func_init_templ(vo, i, network);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void addr_func_die (void *vo)
{
    struct addr_instance *o = vo;
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int addr_func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct addr_instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        char buf[IPADDR_PRINT_MAX];
        ipaddr_print_addr(o->addr, buf);
        
        *out = NCDVal_NewString(mem, buf);
        return 1;
    }
    
    return 0;
}

static void mask_to_prefix_func_init (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct prefix_instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef mask_arg;
    if (!NCDVal_ListRead(params->args, 1, &mask_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(mask_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // parse mask
    uint32_t mask;
    if (!ipaddr_parse_ipv4_addr(NCDVal_StringMemRef(mask_arg), &mask)) {
        ModuleLog(i, BLOG_ERROR, "bad mask");
        goto fail0;
    }
    
    // build prefix
    if (!ipaddr_ipv4_prefix_from_mask(mask, &o->prefix)) {
        ModuleLog(i, BLOG_ERROR, "bad mask");
        goto fail0;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void prefix_func_die (void *vo)
{
    struct prefix_instance *o = vo;
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int prefix_func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct prefix_instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        char buf[6];
        sprintf(buf, "%d", o->prefix);
        
        *out = NCDVal_NewString(mem, buf);
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "ipv4_prefix_to_mask",
        .func_new2 = prefix_to_mask_func_init,
        .func_die = addr_func_die,
        .func_getvar2 = addr_func_getvar2,
        .alloc_size = sizeof(struct addr_instance)
    }, {
        .type = "ipv4_mask_to_prefix",
        .func_new2 = mask_to_prefix_func_init,
        .func_die = prefix_func_die,
        .func_getvar2 = prefix_func_getvar2,
        .alloc_size = sizeof(struct prefix_instance)
    }, {
        .type = "ipv4_net_from_addr_and_prefix",
        .func_new2 = ipv4_net_from_addr_and_prefix_func_init,
        .func_die = addr_func_die,
        .func_getvar2 = addr_func_getvar2,
        .alloc_size = sizeof(struct addr_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_netmask = {
    .modules = modules
};
