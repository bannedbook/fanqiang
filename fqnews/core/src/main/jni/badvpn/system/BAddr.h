/**
 * @file BAddr.h
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
 * Network address abstractions.
 */

#ifndef BADVPN_SYSTEM_BADDR_H
#define BADVPN_SYSTEM_BADDR_H

#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef BADVPN_USE_WINAPI
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#include <misc/byteorder.h>
#include <misc/debug.h>
#include <misc/print_macros.h>
#include <misc/read_write_int.h>
#include <misc/compare.h>

#define BADDR_TYPE_NONE 0
#define BADDR_TYPE_IPV4 1
#define BADDR_TYPE_IPV6 2
#ifdef BADVPN_LINUX
    #define BADDR_TYPE_PACKET 5
#endif

#define BADDR_MAX_ADDR_LEN 128

#define BIPADDR_MAX_PRINT_LEN 40
#define BADDR_MAX_PRINT_LEN 120

#define BADDR_PACKET_HEADER_TYPE_ETHERNET 1

#define BADDR_PACKET_PACKET_TYPE_HOST 1
#define BADDR_PACKET_PACKET_TYPE_BROADCAST 2
#define BADDR_PACKET_PACKET_TYPE_MULTICAST 3
#define BADDR_PACKET_PACKET_TYPE_OTHERHOST 4
#define BADDR_PACKET_PACKET_TYPE_OUTGOING 5

typedef struct {
    int type;
    union {
        uint32_t ipv4;
        uint8_t ipv6[16];
    };
} BIPAddr;

static void BIPAddr_InitInvalid (BIPAddr *addr);

static void BIPAddr_InitIPv4 (BIPAddr *addr, uint32_t ip);

static void BIPAddr_InitIPv6 (BIPAddr *addr, uint8_t *ip);

static void BIPAddr_Assert (BIPAddr *addr);

static int BIPAddr_IsInvalid (BIPAddr *addr);

static int BIPAddr_Resolve (BIPAddr *addr, char *str, int noresolve) WARN_UNUSED;

static int BIPAddr_Compare (BIPAddr *addr1, BIPAddr *addr2);

/**
 * Converts an IP address to human readable form.
 *
 * @param addr IP address to convert
 * @param out destination buffer. Must be at least BIPADDR_MAX_PRINT_LEN characters long.
 */
static void BIPAddr_Print (BIPAddr *addr, char *out);

/**
 * Socket address - IP address and transport protocol port number
 */
typedef struct {
    int type;
    union {
        struct {
            uint32_t ip;
            uint16_t port;
        } ipv4;
        struct {
            uint8_t ip[16];
            uint16_t port;
        } ipv6;
        struct {
            uint16_t phys_proto;
            int interface_index;
            int header_type;
            int packet_type;
            uint8_t phys_addr[8];
        } packet;
    };
} BAddr;

/**
 * Makes an invalid address.
 */
static BAddr BAddr_MakeNone (void);

/**
 * Makes an IPv4 address.
 *
 * @param ip IP address in network byte order
 * @param port port number in network byte order
 */
static BAddr BAddr_MakeIPv4 (uint32_t ip, uint16_t port);

/**
 * Makes an IPv6 address.
 *
 * @param ip IP address (16 bytes)
 * @param port port number in network byte order
 */
static BAddr BAddr_MakeIPv6 (const uint8_t *ip, uint16_t port);

/**
 * Makes an address from a BIPAddr and port number.
 *
 * @param ipaddr the BIPAddr
 * @param port port number in network byte order
 */
static BAddr BAddr_MakeFromIpaddrAndPort (BIPAddr ipaddr, uint16_t port);

/**
 * Deprecated, use BAddr_MakeNone.
 */
static void BAddr_InitNone (BAddr *addr);

/**
 * Deprecated, use BAddr_MakeIPv4.
 */
static void BAddr_InitIPv4 (BAddr *addr, uint32_t ip, uint16_t port);

/**
 * Deprecated, use BAddr_MakeIPv6.
 */
static void BAddr_InitIPv6 (BAddr *addr, uint8_t *ip, uint16_t port);

/**
 * Deprecated, use BAddr_MakeFromIpaddrAndPort.
 */
static void BAddr_InitFromIpaddrAndPort (BAddr *addr, BIPAddr ipaddr, uint16_t port);

/**
 * Initializes a packet socket (data link layer) address.
 * Only Ethernet addresses are supported.
 * 
 * @param addr the object
 * @param phys_proto identifier for the upper protocol, network byte order (EtherType)
 * @param interface_index network interface index
 * @param header_type data link layer header type. Must be BADDR_PACKET_HEADER_TYPE_ETHERNET.
 * @param packet_type the manner in which packets are sent/received. Must be one of
 *   BADDR_PACKET_PACKET_TYPE_HOST, BADDR_PACKET_PACKET_TYPE_BROADCAST,
 *   BADDR_PACKET_PACKET_TYPE_MULTICAST, BADDR_PACKET_PACKET_TYPE_OTHERHOST,
 *   BADDR_PACKET_PACKET_TYPE_OUTGOING.
 * @param phys_addr data link layer address (MAC address)
 */
static void BAddr_InitPacket (BAddr *addr, uint16_t phys_proto, int interface_index, int header_type, int packet_type, uint8_t *phys_addr);

/**
 * Does nothing.
 *
 * @param addr the object
 */
static void BAddr_Assert (BAddr *addr);

/**
 * Determines whether the address is an invalid address.
 *
 * @param addr the object
 * @return 1 if invalid, 0 if invalid
 **/
static int BAddr_IsInvalid (BAddr *addr);

/**
 * Returns the port number in the address.
 *
 * @param addr the object
 *             Must be an IPv4 or IPv6 address.
 * @return port number, in network byte order
 */
static uint16_t BAddr_GetPort (BAddr *addr);

/**
 * Returns the IP address in the address.
 *
 * @param addr the object
 * @param ipaddr IP address will be returned here. If \a addr is not
 *               an IPv4 or IPv6 address, an invalid address will be
 *               returned.
 */
static void BAddr_GetIPAddr (BAddr *addr, BIPAddr *ipaddr);

/**
 * Sets the port number in the address.
 *
 * @param addr the object
 *             Must be an IPv4 or IPv6 address.
 * @param port port number, in network byte order
 */
static void BAddr_SetPort (BAddr *addr, uint16_t port);

/**
 * Converts an IP address to human readable form.
 *
 * @param addr address to convert
 * @param out destination buffer. Must be at least BADDR_MAX_PRINT_LEN characters long.
 */
static void BAddr_Print (BAddr *addr, char *out);

/**
 * Resolves an address string.
 * Format is "addr:port" for IPv4, "[addr]:port" for IPv6.
 * addr is be a numeric address or a name.
 * port is a numeric port number.
 *
 * @param addr output address
 * @param name if not NULL, the name portion of the address will be
 *             stored here
 * @param name_len if name is not NULL, the size of the name buffer
 * @param noresolve only accept numeric addresses. Avoids blocking the caller.
 * @return 1 on success, 0 on parse error
 */
static int BAddr_Parse2 (BAddr *addr, char *str, char *name, int name_len, int noresolve) WARN_UNUSED;

/**
 * Resolves an address string.
 * IPv4 input format is "a.b.c.d:p", where a.b.c.d is the IP address
 * and d is the port number.
 * IPv6 input format is "[addr]:p", where addr is an IPv6 addres in
 * standard notation and p is the port number.
 *
 * @param addr output address
 * @param name if not NULL, the name portion of the address will be
 *             stored here
 * @param name_len if name is not NULL, the size of the name buffer
 * @return 1 on success, 0 on parse error
 */
static int BAddr_Parse (BAddr *addr, char *str, char *name, int name_len) WARN_UNUSED;

static int BAddr_Compare (BAddr *addr1, BAddr *addr2);

static int BAddr_CompareOrder (BAddr *addr1, BAddr *addr2);

void BIPAddr_InitInvalid (BIPAddr *addr)
{
    addr->type = BADDR_TYPE_NONE;
}

void BIPAddr_InitIPv4 (BIPAddr *addr, uint32_t ip)
{
    addr->type = BADDR_TYPE_IPV4;
    addr->ipv4 = ip;
}

void BIPAddr_InitIPv6 (BIPAddr *addr, uint8_t *ip)
{
    addr->type = BADDR_TYPE_IPV6;
    memcpy(addr->ipv6, ip, 16);
}

void BIPAddr_Assert (BIPAddr *addr)
{
    switch (addr->type) {
        case BADDR_TYPE_NONE:
        case BADDR_TYPE_IPV4:
        case BADDR_TYPE_IPV6:
            return;
        default:
            ASSERT(0);
    }
}

int BIPAddr_IsInvalid (BIPAddr *addr)
{
    BIPAddr_Assert(addr);
    
    return (addr->type == BADDR_TYPE_NONE);
}

int BIPAddr_Resolve (BIPAddr *addr, char *str, int noresolve)
{
    int len = strlen(str);
    
    char *addr_start;
    int addr_len;
    
    // determine address type
    if (len >= 1 && str[0] == '[' && str[len - 1] == ']') {
        addr->type = BADDR_TYPE_IPV6;
        addr_start = str + 1;
        addr_len = len - 2;
    } else {
        addr->type = BADDR_TYPE_IPV4;
        addr_start = str;
        addr_len = len;
    }
    
    // copy
    char addr_str[BADDR_MAX_ADDR_LEN + 1];
    if (addr_len > BADDR_MAX_ADDR_LEN) {
        return 0;
    }
    memcpy(addr_str, addr_start, addr_len);
    addr_str[addr_len] = '\0';
    
    // initialize hints
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    switch (addr->type) {
        case BADDR_TYPE_IPV6:
            hints.ai_family = AF_INET6;
            break;
        case BADDR_TYPE_IPV4:
            hints.ai_family = AF_INET;
            break;
    }
    if (noresolve) {
        hints.ai_flags |= AI_NUMERICHOST;
    }
    
    // call getaddrinfo
    struct addrinfo *addrs;
    int res;
    if ((res = getaddrinfo(addr_str, NULL, &hints, &addrs)) != 0) {
        return 0;
    }
    
    // set address
    switch (addr->type) {
        case BADDR_TYPE_IPV6:
            memcpy(addr->ipv6, ((struct sockaddr_in6 *)addrs->ai_addr)->sin6_addr.s6_addr, sizeof(addr->ipv6));
            break;
        case BADDR_TYPE_IPV4:
            addr->ipv4 = ((struct sockaddr_in *)addrs->ai_addr)->sin_addr.s_addr;
            break;
    }
    
    freeaddrinfo(addrs);
    
    return 1;
}

int BIPAddr_Compare (BIPAddr *addr1, BIPAddr *addr2)
{
    BIPAddr_Assert(addr1);
    BIPAddr_Assert(addr2);
    
    if (addr1->type != addr2->type) {
        return 0;
    }
    
    switch (addr1->type) {
        case BADDR_TYPE_NONE:
            return 0;
        case BADDR_TYPE_IPV4:
            return (addr1->ipv4 == addr2->ipv4);
        case BADDR_TYPE_IPV6:
            return (!memcmp(addr1->ipv6, addr2->ipv6, sizeof(addr1->ipv6)));
        default:
            ASSERT(0)
            return 0;
    }
}

uint16_t BAddr_GetPort (BAddr *addr)
{
    BAddr_Assert(addr);
    ASSERT(addr->type == BADDR_TYPE_IPV4 || addr->type == BADDR_TYPE_IPV6)
    
    switch (addr->type) {
        case BADDR_TYPE_IPV4:
            return addr->ipv4.port;
        case BADDR_TYPE_IPV6:
            return addr->ipv6.port;
        default:
            ASSERT(0)
            return 0;
    }
}

void BAddr_GetIPAddr (BAddr *addr, BIPAddr *ipaddr)
{
    BAddr_Assert(addr);
    
    switch (addr->type) {
        case BADDR_TYPE_IPV4:
            BIPAddr_InitIPv4(ipaddr, addr->ipv4.ip);
            return;
        case BADDR_TYPE_IPV6:
            BIPAddr_InitIPv6(ipaddr, addr->ipv6.ip);
            return;
        default:
            BIPAddr_InitInvalid(ipaddr);
    }
}

void BAddr_SetPort (BAddr *addr, uint16_t port)
{
    BAddr_Assert(addr);
    ASSERT(addr->type == BADDR_TYPE_IPV4 || addr->type == BADDR_TYPE_IPV6)
    
    switch (addr->type) {
        case BADDR_TYPE_IPV4:
            addr->ipv4.port = port;
            break;
        case BADDR_TYPE_IPV6:
            addr->ipv6.port = port;
            break;
        default:
            ASSERT(0);
    }
}

void BIPAddr_Print (BIPAddr *addr, char *out)
{
    switch (addr->type) {
        case BADDR_TYPE_NONE:
            sprintf(out, "(none)");
            break;
        case BADDR_TYPE_IPV4:
            sprintf(out, "%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8,
                *((uint8_t *)&addr->ipv4 + 0),
                *((uint8_t *)&addr->ipv4 + 1),
                *((uint8_t *)&addr->ipv4 + 2),
                *((uint8_t *)&addr->ipv4 + 3)
            );
            break;
        case BADDR_TYPE_IPV6: {
            const char *ptr = (const char *)addr->ipv6;
            sprintf(out,
                "%"PRIx16":%"PRIx16":%"PRIx16":%"PRIx16":"
                "%"PRIx16":%"PRIx16":%"PRIx16":%"PRIx16,
                badvpn_read_be16(ptr + 0),
                badvpn_read_be16(ptr + 2),
                badvpn_read_be16(ptr + 4),
                badvpn_read_be16(ptr + 6),
                badvpn_read_be16(ptr + 8),
                badvpn_read_be16(ptr + 10),
                badvpn_read_be16(ptr + 12),
                badvpn_read_be16(ptr + 14)
            );
        } break;
        default:
            ASSERT(0);
    }
}

BAddr BAddr_MakeNone (void)
{
    BAddr addr;
    addr.type = BADDR_TYPE_NONE;
    return addr;
}

BAddr BAddr_MakeIPv4 (uint32_t ip, uint16_t port)
{
    BAddr addr;
    addr.type = BADDR_TYPE_IPV4;
    addr.ipv4.ip = ip;
    addr.ipv4.port = port;
    return addr;
}

BAddr BAddr_MakeIPv6 (const uint8_t *ip, uint16_t port)
{
    BAddr addr;
    addr.type = BADDR_TYPE_IPV6;
    memcpy(addr.ipv6.ip, ip, 16);
    addr.ipv6.port = port;
    return addr;
}

BAddr BAddr_MakeFromIpaddrAndPort (BIPAddr ipaddr, uint16_t port)
{
    BIPAddr_Assert(&ipaddr);
    
    switch (ipaddr.type) {
        case BADDR_TYPE_NONE:
            return BAddr_MakeNone();
        case BADDR_TYPE_IPV4:
            return BAddr_MakeIPv4(ipaddr.ipv4, port);
        case BADDR_TYPE_IPV6:
            return BAddr_MakeIPv6(ipaddr.ipv6, port);
        default:
            ASSERT(0);
            return BAddr_MakeNone();
    }
}

void BAddr_InitNone (BAddr *addr)
{
    *addr = BAddr_MakeNone();
}

void BAddr_InitIPv4 (BAddr *addr, uint32_t ip, uint16_t port)
{
    *addr = BAddr_MakeIPv4(ip, port);
}

void BAddr_InitIPv6 (BAddr *addr, uint8_t *ip, uint16_t port)
{
    *addr = BAddr_MakeIPv6(ip, port);
}

void BAddr_InitFromIpaddrAndPort (BAddr *addr, BIPAddr ipaddr, uint16_t port)
{
    BIPAddr_Assert(&ipaddr);
    
    *addr = BAddr_MakeFromIpaddrAndPort(ipaddr, port);
}

#ifdef BADVPN_LINUX

void BAddr_InitPacket (BAddr *addr, uint16_t phys_proto, int interface_index, int header_type, int packet_type, uint8_t *phys_addr)
{
    ASSERT(header_type == BADDR_PACKET_HEADER_TYPE_ETHERNET)
    ASSERT(packet_type == BADDR_PACKET_PACKET_TYPE_HOST || packet_type == BADDR_PACKET_PACKET_TYPE_BROADCAST || 
           packet_type == BADDR_PACKET_PACKET_TYPE_MULTICAST || packet_type == BADDR_PACKET_PACKET_TYPE_OTHERHOST ||
           packet_type == BADDR_PACKET_PACKET_TYPE_OUTGOING)
    
    addr->type = BADDR_TYPE_PACKET;
    addr->packet.phys_proto = phys_proto;
    addr->packet.interface_index = interface_index;
    addr->packet.header_type = header_type;
    addr->packet.packet_type = packet_type;
    memcpy(addr->packet.phys_addr, phys_addr, 6);
}

#endif

void BAddr_Assert (BAddr *addr)
{
    switch (addr->type) {
        case BADDR_TYPE_NONE:
        case BADDR_TYPE_IPV4:
        case BADDR_TYPE_IPV6:
        #ifdef BADVPN_LINUX
        case BADDR_TYPE_PACKET:
        #endif
            return;
        default:
            ASSERT(0);
    }
}

int BAddr_IsInvalid (BAddr *addr)
{
    BAddr_Assert(addr);
    
    return (addr->type == BADDR_TYPE_NONE);
}

void BAddr_Print (BAddr *addr, char *out)
{
    BAddr_Assert(addr);
    
    BIPAddr ipaddr;
    
    switch (addr->type) {
        case BADDR_TYPE_NONE:
            sprintf(out, "(none)");
            break;
        case BADDR_TYPE_IPV4:
            BIPAddr_InitIPv4(&ipaddr, addr->ipv4.ip);
            BIPAddr_Print(&ipaddr, out);
            sprintf(out + strlen(out), ":%"PRIu16, ntoh16(addr->ipv4.port));
            break;
        case BADDR_TYPE_IPV6:
            BIPAddr_InitIPv6(&ipaddr, addr->ipv6.ip);
            BIPAddr_Print(&ipaddr, out);
            sprintf(out + strlen(out), ":%"PRIu16, ntoh16(addr->ipv6.port));
            break;
        #ifdef BADVPN_LINUX
        case BADDR_TYPE_PACKET:
            ASSERT(addr->packet.header_type == BADDR_PACKET_HEADER_TYPE_ETHERNET)
            sprintf(out, "proto=%"PRIu16",ifindex=%d,htype=eth,ptype=%d,addr=%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8,
                    addr->packet.phys_proto, (int)addr->packet.interface_index, (int)addr->packet.packet_type,
                    addr->packet.phys_addr[0], addr->packet.phys_addr[1], addr->packet.phys_addr[2],
                    addr->packet.phys_addr[3], addr->packet.phys_addr[4], addr->packet.phys_addr[5]);
            break;
        #endif
        default:
            ASSERT(0);
    }
}

int BAddr_Parse2 (BAddr *addr, char *str, char *name, int name_len, int noresolve)
{
    int len = strlen(str);
    if (len < 1 || len > 1000) {
        return 0;
    }
    
    int addr_start;
    int addr_len;
    int port_start;
    int port_len;
    
    // leading '[' indicates an IPv6 address
    if (str[0] == '[') {
        addr->type = BADDR_TYPE_IPV6;
        // find ']'
        int i=1;
        while (i < len && str[i] != ']') i++;
        if (i >= len) {
            return 0;
        }
        addr_start = 1;
        addr_len = i - addr_start;
        // follows ':' and port number
        if (i + 1 >= len || str[i + 1] != ':') {
            return 0;
        }
        port_start = i + 2;
        port_len = len - port_start;
    }
    // otherwise it's an IPv4 address
    else {
        addr->type = BADDR_TYPE_IPV4;
        // find ':'
        int i=0;
        while (i < len && str[i] != ':') i++;
        if (i >= len) {
            return 0;
        }
        addr_start = 0;
        addr_len = i - addr_start;
        port_start = i + 1;
        port_len = len - port_start;
    }
    
    // copy address and port to zero-terminated buffers
    
    char addr_str[128];
    if (addr_len >= sizeof(addr_str)) {
        return 0;
    }
    memcpy(addr_str, str + addr_start, addr_len);
    addr_str[addr_len] = '\0';
    
    char port_str[6];
    if (port_len >= sizeof(port_str)) {
        return 0;
    }
    memcpy(port_str, str + port_start, port_len);
    port_str[port_len] = '\0';
    
    // parse port
    char *err;
    long int conv_res = strtol(port_str, &err, 10);
    if (port_str[0] == '\0' || *err != '\0') {
        return 0;
    }
    if (conv_res < 0 || conv_res > UINT16_MAX) {
        return 0;
    }
    uint16_t port = conv_res;
    port = hton16(port);
    
    // initialize hints
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    switch (addr->type) {
        case BADDR_TYPE_IPV6:
            hints.ai_family = AF_INET6;
            break;
        case BADDR_TYPE_IPV4:
            hints.ai_family = AF_INET;
            break;
    }
    if (noresolve) {
        hints.ai_flags |= AI_NUMERICHOST;
    }
    
    // call getaddrinfo
    struct addrinfo *addrs;
    int res;
    if ((res = getaddrinfo(addr_str, NULL, &hints, &addrs)) != 0) {
        return 0;
    }
    
    // set address
    switch (addr->type) {
        case BADDR_TYPE_IPV6:
            memcpy(addr->ipv6.ip, ((struct sockaddr_in6 *)addrs->ai_addr)->sin6_addr.s6_addr, sizeof(addr->ipv6.ip));
            addr->ipv6.port = port;
            break;
        case BADDR_TYPE_IPV4:
            addr->ipv4.ip = ((struct sockaddr_in *)addrs->ai_addr)->sin_addr.s_addr;
            addr->ipv4.port = port;
            break;
    }
    
    freeaddrinfo(addrs);
    
    if (name) {
        if (strlen(addr_str) >= name_len) {
            return 0;
        }
        strcpy(name, addr_str);
    }
    
    return 1;
}

int BAddr_Parse (BAddr *addr, char *str, char *name, int name_len)
{
    return BAddr_Parse2(addr, str, name, name_len, 0);
}

int BAddr_Compare (BAddr *addr1, BAddr *addr2)
{
    BAddr_Assert(addr1);
    BAddr_Assert(addr2);
    
    if (addr1->type != addr2->type) {
        return 0;
    }
    
    switch (addr1->type) {
        case BADDR_TYPE_IPV4:
            return (addr1->ipv4.ip == addr2->ipv4.ip && addr1->ipv4.port == addr2->ipv4.port);
        case BADDR_TYPE_IPV6:
            return (!memcmp(addr1->ipv6.ip, addr2->ipv6.ip, sizeof(addr1->ipv6.ip)) && addr1->ipv6.port == addr2->ipv6.port);
        default:
            return 0;
    }
}

int BAddr_CompareOrder (BAddr *addr1, BAddr *addr2)
{
    BAddr_Assert(addr1);
    BAddr_Assert(addr2);
    
    int cmp = B_COMPARE(addr1->type, addr2->type);
    if (cmp) {
        return cmp;
    }
    
    switch (addr1->type) {
        case BADDR_TYPE_NONE: {
            return 0;
        } break;
        case BADDR_TYPE_IPV4: {
            uint32_t ip1 = ntoh32(addr1->ipv4.ip);
            uint32_t ip2 = ntoh32(addr2->ipv4.ip);
            cmp = B_COMPARE(ip1, ip2);
            if (cmp) {
                return cmp;
            }
            uint16_t port1 = ntoh16(addr1->ipv4.port);
            uint16_t port2 = ntoh16(addr2->ipv4.port);
            return B_COMPARE(port1, port2);
        } break;
        case BADDR_TYPE_IPV6: {
            cmp = memcmp(addr1->ipv6.ip, addr2->ipv6.ip, sizeof(addr1->ipv6.ip));
            if (cmp) {
                return B_COMPARE(cmp, 0);
            }
            uint16_t port1 = ntoh16(addr1->ipv6.port);
            uint16_t port2 = ntoh16(addr2->ipv6.port);
            return B_COMPARE(port1, port2);
        } break;
        default: {
            return 0;
        } break;
    }
}

#endif
