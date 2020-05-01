/**
 * @file socks_proto.h
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
 * Definitions for the SOCKS protocol.
 */

#ifndef BADVPN_MISC_SOCKS_PROTO_H
#define BADVPN_MISC_SOCKS_PROTO_H

#include <stdint.h>

#include <misc/packed.h>

#define SOCKS_VERSION 0x05

#define SOCKS_METHOD_NO_AUTHENTICATION_REQUIRED 0x00
#define SOCKS_METHOD_GSSAPI 0x01
#define SOCKS_METHOD_USERNAME_PASSWORD 0x02
#define SOCKS_METHOD_NO_ACCEPTABLE_METHODS 0xFF

#define SOCKS_CMD_CONNECT 0x01
#define SOCKS_CMD_BIND 0x02
#define SOCKS_CMD_UDP_ASSOCIATE 0x03

#define SOCKS_ATYP_IPV4 0x01
#define SOCKS_ATYP_DOMAINNAME 0x03
#define SOCKS_ATYP_IPV6 0x04

#define SOCKS_REP_SUCCEEDED 0x00
#define SOCKS_REP_GENERAL_FAILURE 0x01
#define SOCKS_REP_CONNECTION_NOT_ALLOWED 0x02
#define SOCKS_REP_NETWORK_UNREACHABLE 0x03
#define SOCKS_REP_HOST_UNREACHABLE 0x04
#define SOCKS_REP_CONNECTION_REFUSED 0x05
#define SOCKS_REP_TTL_EXPIRED 0x06
#define SOCKS_REP_COMMAND_NOT_SUPPORTED 0x07
#define SOCKS_REP_ADDRESS_TYPE_NOT_SUPPORTED 0x08

B_START_PACKED
struct socks_client_hello_header {
    uint8_t ver;
    uint8_t nmethods;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct socks_client_hello_method {
    uint8_t method;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct socks_server_hello {
    uint8_t ver;
    uint8_t method;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct socks_request_header {
    uint8_t ver;
    uint8_t cmd;
    uint8_t rsv;
    uint8_t atyp;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct socks_reply_header {
    uint8_t ver;
    uint8_t rep;
    uint8_t rsv;
    uint8_t atyp;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct socks_addr_ipv4 {
    uint32_t addr;
    uint16_t port;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct socks_addr_ipv6 {
    uint8_t addr[16];
    uint16_t port;
} B_PACKED;    
B_END_PACKED

#endif
