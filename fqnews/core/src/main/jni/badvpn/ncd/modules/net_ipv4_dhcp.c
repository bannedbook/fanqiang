/**
 * @file net_ipv4_dhcp.c
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
 * DHCP client module.
 * 
 * Synopsis:
 *   net.ipv4.dhcp(string ifname [, list opts])
 * 
 * Description:
 *   Runs a DHCP client on a network interface. When an address is obtained,
 *   transitions up (but does not assign anything). If the lease times out,
 *   transitions down.
 *   The interface must already be up.
 *   Supported options (in the opts argument):
 *   - "hostname", (string value): send this hostname to the DHCP server
 *   - "vendorclassid", (string value): send this vendor class identifier
 *   - "auto_clientid": send a client identifier generated from the MAC address
 * 
 * Variables:
 *   string addr - assigned IP address ("A.B.C.D")
 *   string prefix - address prefix length ("N")
 *   string cidr_addr - address and prefix in CIDR notation ("A.B.C.D/N")
 *   string gateway - router address ("A.B.C.D"), or "none" if not provided
 *   list(string) dns_servers - DNS server addresses ("A.B.C.D" ...)
 *   string server_mac - MAC address of the DHCP server (6 two-digit caps hexadecimal values
 *     separated with colons, e.g."AB:CD:EF:01:02:03")
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <misc/debug.h>
#include <misc/ipaddr.h>
#include <dhcpclient/BDHCPClient.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_ipv4_dhcp.h>

struct instance {
    NCDModuleInst *i;
    BDHCPClient dhcp;
    int up;
};

static void instance_free (struct instance *o, int is_error);

static void dhcp_handler (struct instance *o, int event)
{
    switch (event) {
        case BDHCPCLIENT_EVENT_UP: {
            ASSERT(!o->up)
            o->up = 1;
            NCDModuleInst_Backend_Up(o->i);
        } break;
        
        case BDHCPCLIENT_EVENT_DOWN: {
            ASSERT(o->up)
            o->up = 0;
            NCDModuleInst_Backend_Down(o->i);
        } break;
        
        case BDHCPCLIENT_EVENT_ERROR: {
            instance_free(o, 1);
            return;
        } break;
        
        default: ASSERT(0);
    }
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef ifname_arg;
    NCDValRef opts_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 1, &ifname_arg) && !NCDVal_ListRead(params->args, 2, &ifname_arg, &opts_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(ifname_arg) || (!NCDVal_IsInvalid(opts_arg) && !NCDVal_IsList(opts_arg))) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    NCDValNullTermString hostname_nts = NCDValNullTermString_NewDummy();
    NCDValNullTermString vendorclassid_nts = NCDValNullTermString_NewDummy();
    
    struct BDHCPClient_opts opts = {};
    
    // read options
    size_t count = NCDVal_IsInvalid(opts_arg) ? 0 : NCDVal_ListCount(opts_arg);
    for (size_t j = 0; j < count; j++) {
        NCDValRef opt = NCDVal_ListGet(opts_arg, j);
        
        // read name
        if (!NCDVal_IsString(opt)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong option name type");
            goto fail1;
        }
        
        if (NCDVal_StringEquals(opt, "hostname") || NCDVal_StringEquals(opt, "vendorclassid")) {
            int is_hostname = NCDVal_StringEquals(opt, "hostname");
            
            // read value
            if (j == count) {
                ModuleLog(o->i, BLOG_ERROR, "option value missing");
                goto fail1;
            }
            NCDValRef val = NCDVal_ListGet(opts_arg, j + 1);
            if (!NCDVal_IsStringNoNulls(val)) {
                ModuleLog(o->i, BLOG_ERROR, "wrong option value type");
                goto fail1;
            }
            
            // null terminate
            NCDValNullTermString nts;
            if (!NCDVal_StringNullTerminate(val, &nts)) {
                ModuleLog(o->i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
                goto fail1;
            }
            NCDValNullTermString *nts_ptr = (is_hostname ? &hostname_nts : &vendorclassid_nts);
            NCDValNullTermString_Free(nts_ptr);
            *nts_ptr = nts;
            
            if (is_hostname) {
                opts.hostname = nts.data;
            } else {
                opts.vendorclassid = nts.data;
            }
            
            j++;
        }
        else if (NCDVal_StringEquals(opt, "auto_clientid")) {
            opts.auto_clientid = 1;
        }
        else {
            ModuleLog(o->i, BLOG_ERROR, "unknown option name");
            goto fail1;
        }
    }
    
    // null terminate ifname
    NCDValNullTermString ifname_nts;
    if (!NCDVal_StringNullTerminate(ifname_arg, &ifname_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail1;
    }
    
    // init DHCP
    int res = BDHCPClient_Init(&o->dhcp, ifname_nts.data, opts, o->i->params->iparams->reactor, o->i->params->iparams->random2, (BDHCPClient_handler)dhcp_handler, o);
    NCDValNullTermString_Free(&ifname_nts);
    if (!res) {
        ModuleLog(o->i, BLOG_ERROR, "BDHCPClient_Init failed");
        goto fail1;
    }
    
    // set not up
    o->up = 0;
    
    // free options nts's
    NCDValNullTermString_Free(&hostname_nts);
    NCDValNullTermString_Free(&vendorclassid_nts);
    return;
    
fail1:
    NCDValNullTermString_Free(&hostname_nts);
    NCDValNullTermString_Free(&vendorclassid_nts);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void instance_free (struct instance *o, int is_error)
{
    // free DHCP
    BDHCPClient_Free(&o->dhcp);
    
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
        uint32_t addr;
        BDHCPClient_GetClientIP(&o->dhcp, &addr);
        
        char str[IPADDR_PRINT_MAX];
        ipaddr_print_addr(addr, str);
        
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    if (!strcmp(name, "prefix")) {
        uint32_t addr;
        BDHCPClient_GetClientIP(&o->dhcp, &addr);
        uint32_t mask;
        BDHCPClient_GetClientMask(&o->dhcp, &mask);
        
        struct ipv4_ifaddr ifaddr;
        if (!ipaddr_ipv4_ifaddr_from_addr_mask(addr, mask, &ifaddr)) {
            ModuleLog(o->i, BLOG_ERROR, "bad netmask");
            return 0;
        }
        
        char str[10];
        sprintf(str, "%d", ifaddr.prefix);
        
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    if (!strcmp(name, "cidr_addr")) {
        uint32_t addr;
        BDHCPClient_GetClientIP(&o->dhcp, &addr);
        uint32_t mask;
        BDHCPClient_GetClientMask(&o->dhcp, &mask);
        
        struct ipv4_ifaddr ifaddr;
        if (!ipaddr_ipv4_ifaddr_from_addr_mask(addr, mask, &ifaddr)) {
            ModuleLog(o->i, BLOG_ERROR, "bad netmask");
            return 0;
        }
        
        char str[IPADDR_PRINT_MAX];
        ipaddr_print_ifaddr(ifaddr, str);
        
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    if (!strcmp(name, "gateway")) {
        char str[IPADDR_PRINT_MAX];
        
        uint32_t addr;
        if (!BDHCPClient_GetRouter(&o->dhcp, &addr)) {
            strcpy(str, "none");
        } else {
            ipaddr_print_addr(addr, str);
        }
        
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    if (!strcmp(name, "dns_servers")) {
        uint32_t servers[BDHCPCLIENT_MAX_DOMAIN_NAME_SERVERS];
        int num_servers = BDHCPClient_GetDNS(&o->dhcp, servers, BDHCPCLIENT_MAX_DOMAIN_NAME_SERVERS);
        
        *out = NCDVal_NewList(mem, num_servers);
        if (NCDVal_IsInvalid(*out)) {
            goto fail;
        }
        
        for (int i = 0; i < num_servers; i++) {
            char str[IPADDR_PRINT_MAX];
            ipaddr_print_addr(servers[i], str);
            
            NCDValRef server = NCDVal_NewString(mem, str);
            if (NCDVal_IsInvalid(server)) {
                goto fail;
            }
            
            if (!NCDVal_ListAppend(*out, server)) {
                goto fail;
            }
        }
        
        return 1;
    }
    
    if (!strcmp(name, "server_mac")) {
        uint8_t mac[6];
        BDHCPClient_GetServerMAC(&o->dhcp, mac);
        
        char str[18];
        sprintf(str, "%02"PRIX8":%02"PRIX8":%02"PRIX8":%02"PRIX8":%02"PRIX8":%02"PRIX8,
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    return 0;
    
fail:
    *out = NCDVal_NewInvalid();
    return 1;
}

static struct NCDModule modules[] = {
    {
        .type = "net.ipv4.dhcp",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_ipv4_dhcp = {
    .modules = modules
};
