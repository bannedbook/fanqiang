/**
 * @file BConnectionGeneric.h
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

#ifndef BADVPN_BCONNECTION_GENERIC_H
#define BADVPN_BCONNECTION_GENERIC_H

#include <string.h>

#include <misc/debug.h>
#include <misc/strdup.h>
#include <misc/memref.h>
#include <base/BLog.h>
#include <system/BConnection.h>

#define BCONNECTION_ADDR_TYPE_BADDR 1
#define BCONNECTION_ADDR_TYPE_UNIX 2

struct BConnection_addr {
    int type;
    union {
        BAddr baddr;
        struct {
            const char *str;
            size_t len;
        } unix_socket_path;
    } u;
};

static struct BConnection_addr BConnection_addr_baddr (BAddr baddr)
{
    struct BConnection_addr addr;
    addr.type = BCONNECTION_ADDR_TYPE_BADDR;
    addr.u.baddr = baddr;
    return addr;
}

static struct BConnection_addr BConnection_addr_unix (MemRef unix_socket_path)
{
    struct BConnection_addr addr;
    addr.type = BCONNECTION_ADDR_TYPE_UNIX;
    addr.u.unix_socket_path.str = unix_socket_path.ptr;
    addr.u.unix_socket_path.len = unix_socket_path.len;
    return addr;
}

static int BListener_InitGeneric (BListener *o, struct BConnection_addr addr, BReactor *reactor, void *user,
                                  BListener_handler handler) WARN_UNUSED;
static int BConnector_InitGeneric (BConnector *o, struct BConnection_addr addr, BReactor *reactor, void *user,
                                   BConnector_handler handler) WARN_UNUSED;

static int BListener_InitGeneric (BListener *o, struct BConnection_addr addr, BReactor *reactor, void *user,
                                  BListener_handler handler)
{
    ASSERT(handler)
    BNetwork_Assert();
    ASSERT(addr.type != BCONNECTION_ADDR_TYPE_UNIX || !memchr(addr.u.unix_socket_path.str, '\0', addr.u.unix_socket_path.len))
    
    switch (addr.type) {
        case BCONNECTION_ADDR_TYPE_BADDR: {
            return BListener_Init(o, addr.u.baddr, reactor, user, handler);
        } break;
        
        case BCONNECTION_ADDR_TYPE_UNIX: {
#ifdef BADVPN_USE_WINAPI
            BLog_LogToChannel(BLOG_CHANNEL_BConnection, BLOG_ERROR, "unix sockets not supported");
            return 0;
#else
            char *str = b_strdup_bin(addr.u.unix_socket_path.str, addr.u.unix_socket_path.len);
            if (!str) {
                BLog_LogToChannel(BLOG_CHANNEL_BConnection, BLOG_ERROR, "b_strdup_bin failed");
                return 0;
            }
            int res = BListener_InitUnix(o, str, reactor, user, handler);
            free(str);
            return res;
#endif
        } break;
        
        default:
            ASSERT(0);
            return 0;
    }
}

static int BConnector_InitGeneric (BConnector *o, struct BConnection_addr addr, BReactor *reactor, void *user,
                                   BConnector_handler handler)
{
    ASSERT(handler)
    BNetwork_Assert();
    ASSERT(addr.type != BCONNECTION_ADDR_TYPE_UNIX || !memchr(addr.u.unix_socket_path.str, '\0', addr.u.unix_socket_path.len))
    
    switch (addr.type) {
        case BCONNECTION_ADDR_TYPE_BADDR: {
            return BConnector_Init(o, addr.u.baddr, reactor, user, handler);
        } break;
        
        case BCONNECTION_ADDR_TYPE_UNIX: {
#ifdef BADVPN_USE_WINAPI
            BLog_LogToChannel(BLOG_CHANNEL_BConnection, BLOG_ERROR, "unix sockets not supported");
            return 0;
#else
            char *str = b_strdup_bin(addr.u.unix_socket_path.str, addr.u.unix_socket_path.len);
            if (!str) {
                BLog_LogToChannel(BLOG_CHANNEL_BConnection, BLOG_ERROR, "b_strdup_bin failed");
                return 0;
            }
            int res = BConnector_InitUnix(o, str, reactor, user, handler);
            free(str);
            return res;
#endif
        } break;
        
        default:
            ASSERT(0);
            return 0;
    }
}

#endif
