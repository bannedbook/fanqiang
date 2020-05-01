/**
 * @file address_utils.h
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

#ifndef NCD_ADDRESS_UTILS_H
#define NCD_ADDRESS_UTILS_H

#include <string.h>
#include <limits.h>

#include <misc/debug.h>
#include <misc/ipaddr.h>
#include <misc/ipaddr6.h>
#include <misc/byteorder.h>
#include <system/BAddr.h>
#include <system/BConnectionGeneric.h>
#include <ncd/NCDVal.h>
#include <ncd/extra/value_utils.h>

typedef int (*ncd_read_bconnection_addr_CustomHandler) (void *user, NCDValRef protocol, NCDValRef data);

static int ncd_read_baddr (NCDValRef val, BAddr *out) WARN_UNUSED;
static NCDValRef ncd_make_baddr (BAddr addr, NCDValMem *mem);
static int ncd_read_bconnection_addr (NCDValRef val, struct BConnection_addr *out_addr) WARN_UNUSED;
static int ncd_read_bconnection_addr_ext (NCDValRef val, ncd_read_bconnection_addr_CustomHandler custom_handler, void *user, struct BConnection_addr *out_addr) WARN_UNUSED;

static int ncd_read_baddr (NCDValRef val, BAddr *out)
{
    ASSERT(!NCDVal_IsInvalid(val))
    ASSERT(out)
    
    if (!NCDVal_IsList(val)) {
        goto fail;
    }
    
    NCDValRef type_val;
    if (!NCDVal_ListReadHead(val, 1, &type_val)) {
        goto fail;
    }
    if (!NCDVal_IsString(type_val)) {
        goto fail;
    }
    
    BAddr addr;
    
    if (NCDVal_StringEquals(type_val, "none")) {
        if (!NCDVal_ListRead(val, 1, &type_val)) {
            goto fail;
        }
        
        addr.type = BADDR_TYPE_NONE;
    }
    else if (NCDVal_StringEquals(type_val, "ipv4")) {
        NCDValRef ipaddr_val;
        NCDValRef port_val;
        if (!NCDVal_ListRead(val, 3, &type_val, &ipaddr_val, &port_val)) {
            goto fail;
        }
        if (!NCDVal_IsString(ipaddr_val)) {
            goto fail;
        }
        
        addr.type = BADDR_TYPE_IPV4;
        
        if (!ipaddr_parse_ipv4_addr(NCDVal_StringMemRef(ipaddr_val), &addr.ipv4.ip)) {
            goto fail;
        }
        
        uintmax_t port;
        if (!ncd_read_uintmax(port_val, &port) || port > UINT16_MAX) {
            goto fail;
        }
        addr.ipv4.port = hton16(port);
    }
    else if (NCDVal_StringEquals(type_val, "ipv6")) {
        NCDValRef ipaddr_val;
        NCDValRef port_val;
        if (!NCDVal_ListRead(val, 3, &type_val, &ipaddr_val, &port_val)) {
            goto fail;
        }
        if (!NCDVal_IsString(ipaddr_val)) {
            goto fail;
        }
        
        addr.type = BADDR_TYPE_IPV6;
        
        struct ipv6_addr i6addr;
        if (!ipaddr6_parse_ipv6_addr(NCDVal_StringMemRef(ipaddr_val), &i6addr)) {
            goto fail;
        }
        memcpy(addr.ipv6.ip, i6addr.bytes, 16);
        
        uintmax_t port;
        if (!ncd_read_uintmax(port_val, &port) || port > UINT16_MAX) {
            goto fail;
        }
        addr.ipv6.port = hton16(port);
    }
    else {
        goto fail;
    }
    
    *out = addr;
    return 1;
    
fail:
    return 0;
}

static NCDValRef ncd_make_baddr (BAddr addr, NCDValMem *mem)
{
    BAddr_Assert(&addr);
    ASSERT(mem)
    
    NCDValRef val;
    
    switch (addr.type) {
        default:
        case BADDR_TYPE_NONE: {
            val = NCDVal_NewList(mem, 1);
            if (NCDVal_IsInvalid(val)) {
                goto fail;
            }
            
            const char *str = (addr.type == BADDR_TYPE_NONE ? "none" : "unknown");
            NCDValRef type_val = NCDVal_NewString(mem, str);
            if (NCDVal_IsInvalid(type_val)) {
                goto fail;
            }
            
            if (!NCDVal_ListAppend(val, type_val)) {
                goto fail;
            }
        } break;
        
        case BADDR_TYPE_IPV4: {
            val = NCDVal_NewList(mem, 3);
            if (NCDVal_IsInvalid(val)) {
                goto fail;
            }
            
            NCDValRef type_val = NCDVal_NewString(mem, "ipv4");
            if (NCDVal_IsInvalid(type_val)) {
                goto fail;
            }
            
            char ipaddr_buf[IPADDR_PRINT_MAX];
            ipaddr_print_addr(addr.ipv4.ip, ipaddr_buf);
            NCDValRef ipaddr_val = NCDVal_NewString(mem, ipaddr_buf);
            if (NCDVal_IsInvalid(ipaddr_val)) {
                goto fail;
            }
            
            NCDValRef port_val = ncd_make_uintmax(mem, ntoh16(addr.ipv4.port));
            if (NCDVal_IsInvalid(port_val)) {
                goto fail;
            }
            
            if (!NCDVal_ListAppend(val, type_val)) {
                goto fail;
            }
            if (!NCDVal_ListAppend(val, ipaddr_val)) {
                goto fail;
            }
            if (!NCDVal_ListAppend(val, port_val)) {
                goto fail;
            }
        } break;
        
        case BADDR_TYPE_IPV6: {
            val = NCDVal_NewList(mem, 3);
            if (NCDVal_IsInvalid(val)) {
                goto fail;
            }
            
            NCDValRef type_val = NCDVal_NewString(mem, "ipv6");
            if (NCDVal_IsInvalid(type_val)) {
                goto fail;
            }
            
            char ipaddr_buf[IPADDR6_PRINT_MAX];
            struct ipv6_addr i6addr;
            memcpy(i6addr.bytes, addr.ipv6.ip, 16);
            ipaddr6_print_addr(i6addr, ipaddr_buf);
            NCDValRef ipaddr_val = NCDVal_NewString(mem, ipaddr_buf);
            if (NCDVal_IsInvalid(ipaddr_val)) {
                goto fail;
            }
            
            NCDValRef port_val = ncd_make_uintmax(mem, ntoh16(addr.ipv6.port));
            if (NCDVal_IsInvalid(port_val)) {
                goto fail;
            }
            
            if (!NCDVal_ListAppend(val, type_val)) {
                goto fail;
            }
            if (!NCDVal_ListAppend(val, ipaddr_val)) {
                goto fail;
            }
            if (!NCDVal_ListAppend(val, port_val)) {
                goto fail;
            }
        } break;
    }
    
    return val;
    
fail:
    return NCDVal_NewInvalid();
}

static int ncd_read_bconnection_addr (NCDValRef val, struct BConnection_addr *out_addr)
{
    return ncd_read_bconnection_addr_ext(val, NULL, NULL, out_addr);
}

static int ncd_read_bconnection_addr_ext (NCDValRef val, ncd_read_bconnection_addr_CustomHandler custom_handler, void *user, struct BConnection_addr *out_addr)
{
    ASSERT(!NCDVal_IsInvalid(val))
    
    if (!NCDVal_IsList(val)) {
        goto fail;
    }
    
    NCDValRef protocol_arg;
    NCDValRef data_arg;
    if (!NCDVal_ListRead(val, 2, &protocol_arg, &data_arg)) {
        goto fail;
    }
    
    if (!NCDVal_IsString(protocol_arg)) {
        goto fail;
    }
    
    if (NCDVal_StringEquals(protocol_arg, "unix")) {
        if (!NCDVal_IsStringNoNulls(data_arg)) {
            goto fail;
        }
        
        *out_addr = BConnection_addr_unix(NCDVal_StringMemRef(data_arg));
    }
    else if (NCDVal_StringEquals(protocol_arg, "tcp")) {
        BAddr baddr;
        if (!ncd_read_baddr(data_arg, &baddr)) {
            goto fail;
        }
        
        *out_addr = BConnection_addr_baddr(baddr);
    }
    else {
        if (!custom_handler || !custom_handler(user, protocol_arg, data_arg)) {
            goto fail;
        }
        
        out_addr->type = -1;
    }
    
    return 1;
    
fail:
    return 0;
}

#endif
