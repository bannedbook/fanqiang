/**
 * @file parse.c
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
 *   parse_number(string str)
 *   parse_hex_number(string str)
 *   parse_value(string str)
 *   parse_ipv4_addr(string str)
 *   parse_ipv6_addr(string str)
 *   
 * Variables:
 *   succeeded - "true" or "false", reflecting success of the parsing
 *   (empty) - normalized parsed value (only if succeeded)
 * 
 * Synopsis:
 *   parse_ipv4_cidr_addr(string str)
 *   parse_ipv6_cidr_addr(string str)
 * 
 * Variables:
 *   succeeded - "true" or "false", reflecting success of the parsing
 *   (empty) - normalized CIDR notation address (only if succeeded)
 *   addr - normalized address without prefix (only if succeeded)
 *   prefix - normalized prefix without address (only if succeeded)
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <misc/parse_number.h>
#include <misc/ipaddr.h>
#include <misc/ipaddr6.h>
#include <ncd/NCDValParser.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_parse.h>

struct instance {
    NCDModuleInst *i;
    NCDValMem mem;
    NCDValRef value;
    int succeeded;
};

struct ipv4_cidr_instance {
    NCDModuleInst *i;
    int succeeded;
    struct ipv4_ifaddr ifaddr;
};

struct ipv6_cidr_instance {
    NCDModuleInst *i;
    int succeeded;
    struct ipv6_ifaddr ifaddr;
};

enum {STRING_ADDR, STRING_PREFIX};

static const char *strings[] = {
    "addr", "prefix", NULL
};

typedef int (*parse_func) (NCDModuleInst *i, MemRef str, NCDValMem *mem, NCDValRef *out);

static int parse_number (NCDModuleInst *i, MemRef str, NCDValMem *mem, NCDValRef *out)
{
    uintmax_t n;
    if (!parse_unsigned_integer(str, &n)) {
        ModuleLog(i, BLOG_ERROR, "failed to parse number");
        return 0;
    }
    
    *out = ncd_make_uintmax(mem, n);
    if (NCDVal_IsInvalid(*out)) {
        return 0;
    }
    
    return 1;
}

static int parse_hex_number (NCDModuleInst *i, MemRef str, NCDValMem *mem, NCDValRef *out)
{
    uintmax_t n;
    if (!parse_unsigned_hex_integer(str, &n)) {
        ModuleLog(i, BLOG_ERROR, "failed to parse hex number");
        return 0;
    }
    
    *out = ncd_make_uintmax(mem, n);
    if (NCDVal_IsInvalid(*out)) {
        return 0;
    }
    
    return 1;
}

static int parse_value (NCDModuleInst *i, MemRef str, NCDValMem *mem, NCDValRef *out)
{
    if (!NCDValParser_Parse(str, mem, out)) {
        ModuleLog(i, BLOG_ERROR, "failed to parse value");
        return 0;
    }
    
    return 1;
}

static int parse_ipv4_addr (NCDModuleInst *i, MemRef str, NCDValMem *mem, NCDValRef *out)
{
    uint32_t addr;
    if (!ipaddr_parse_ipv4_addr(str, &addr)) {
        ModuleLog(i, BLOG_ERROR, "failed to parse ipv4 addresss");
        return 0;
    }
    
    char buf[IPADDR_PRINT_MAX];
    ipaddr_print_addr(addr, buf);
    
    *out = NCDVal_NewString(mem, buf);
    if (NCDVal_IsInvalid(*out)) {
        return 0;
    }
    
    return 1;
}

static int parse_ipv6_addr (NCDModuleInst *i, MemRef str, NCDValMem *mem, NCDValRef *out)
{
    struct ipv6_addr addr;
    if (!ipaddr6_parse_ipv6_addr(str, &addr)) {
        ModuleLog(i, BLOG_ERROR, "failed to parse ipv6 addresss");
        return 0;
    }
    
    char buf[IPADDR6_PRINT_MAX];
    ipaddr6_print_addr(addr, buf);
    
    *out = NCDVal_NewString(mem, buf);
    if (NCDVal_IsInvalid(*out)) {
        return 0;
    }
    
    return 1;
}

static void new_templ (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, parse_func pfunc)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef str_arg;
    if (!NCDVal_ListRead(params->args, 1, &str_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(str_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // init mem
    NCDValMem_Init(&o->mem, i->params->iparams->string_index);
    
    // parse
    o->succeeded = pfunc(i, NCDVal_StringMemRef(str_arg), &o->mem, &o->value);
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    // free mem
    NCDValMem_Free(&o->mem);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (name == NCD_STRING_SUCCEEDED) {
        *out = ncd_make_boolean(mem, o->succeeded);
        return 1;
    }
    
    if (o->succeeded && name == NCD_STRING_EMPTY) {
        *out = NCDVal_NewCopy(mem, o->value);
        return 1;
    }
    
    return 0;
}

static void func_new_parse_number (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, parse_number);
}

static void func_new_parse_hex_number (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, parse_hex_number);
}

static void func_new_parse_value (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, parse_value);
}

static void func_new_parse_ipv4_addr (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, parse_ipv4_addr);
}

static void func_new_parse_ipv6_addr (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    new_templ(vo, i, params, parse_ipv6_addr);
}

static void ipv4_cidr_addr_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct ipv4_cidr_instance *o = vo;
    o->i = i;
    
    NCDValRef str_arg;
    if (!NCDVal_ListRead(params->args, 1, &str_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(str_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    o->succeeded = ipaddr_parse_ipv4_ifaddr(NCDVal_StringMemRef(str_arg), &o->ifaddr);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static int ipv4_cidr_addr_func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct ipv4_cidr_instance *o = vo;
    
    if (name == NCD_STRING_SUCCEEDED) {
        *out = ncd_make_boolean(mem, o->succeeded);
        return 1;
    }
    
    if (!o->succeeded) {
        return 0;
    }
    
    char str[IPADDR_PRINT_MAX];
    
    if (name == NCD_STRING_EMPTY) {
        ipaddr_print_ifaddr(o->ifaddr, str);
    }
    else if (name == ModuleString(o->i, STRING_ADDR)) {
        ipaddr_print_addr(o->ifaddr.addr, str);
    }
    else if (name == ModuleString(o->i, STRING_PREFIX)) {
        sprintf(str, "%d", o->ifaddr.prefix);
    }
    else {
        return 0;
    }
    
    *out = NCDVal_NewString(mem, str);
    return 1;
}

static void ipv6_cidr_addr_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct ipv6_cidr_instance *o = vo;
    o->i = i;
    
    NCDValRef str_arg;
    if (!NCDVal_ListRead(params->args, 1, &str_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(str_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    o->succeeded = ipaddr6_parse_ipv6_ifaddr(NCDVal_StringMemRef(str_arg), &o->ifaddr);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static int ipv6_cidr_addr_func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct ipv6_cidr_instance *o = vo;
    
    if (name == NCD_STRING_SUCCEEDED) {
        *out = ncd_make_boolean(mem, o->succeeded);
        return 1;
    }
    
    if (!o->succeeded) {
        return 0;
    }
    
    char str[IPADDR6_PRINT_MAX];
    
    if (name == NCD_STRING_EMPTY) {
        ipaddr6_print_ifaddr(o->ifaddr, str);
    }
    else if (name == ModuleString(o->i, STRING_ADDR)) {
        ipaddr6_print_addr(o->ifaddr.addr, str);
    }
    else if (name == ModuleString(o->i, STRING_PREFIX)) {
        sprintf(str, "%d", o->ifaddr.prefix);
    }
    else {
        return 0;
    }
    
    *out = NCDVal_NewString(mem, str);
    return 1;
}

static struct NCDModule modules[] = {
    {
        .type = "parse_number",
        .func_new2 = func_new_parse_number,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "parse_hex_number",
        .func_new2 = func_new_parse_hex_number,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "parse_value",
        .func_new2 = func_new_parse_value,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "parse_ipv4_addr",
        .func_new2 = func_new_parse_ipv4_addr,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "parse_ipv6_addr",
        .func_new2 = func_new_parse_ipv6_addr,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "parse_ipv4_cidr_addr",
        .func_new2 = ipv4_cidr_addr_func_new,
        .func_getvar2 = ipv4_cidr_addr_func_getvar2,
        .alloc_size = sizeof(struct ipv4_cidr_instance)
    }, {
        .type = "parse_ipv6_cidr_addr",
        .func_new2 = ipv6_cidr_addr_func_new,
        .func_getvar2 = ipv6_cidr_addr_func_getvar2,
        .alloc_size = sizeof(struct ipv6_cidr_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_parse = {
    .modules = modules,
    .strings = strings
};
