/**
 * @file
 *
 * IPv6 addresses.
 */

/*
 * Copyright (c) 2010 Inico Technologies Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Ivan Delamer <delamer@inicotech.com>
 *
 * Structs and macros for handling IPv6 addresses.
 *
 * Please coordinate changes and requests with Ivan Delamer
 * <delamer@inicotech.com>
 */
#ifndef LWIP_HDR_IP6_ADDR_H
#define LWIP_HDR_IP6_ADDR_H

#include "lwip/opt.h"
#include "def.h"

#if LWIP_IPV6  /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip6_zone.h"

#ifdef __cplusplus
extern "C" {
#endif


/** This is the aligned version of ip6_addr_t,
    used as local variable, on the stack, etc. */
struct ip6_addr {
  u32_t addr[4];
#if LWIP_IPV6_SCOPES
  u8_t zone;
#endif /* LWIP_IPV6_SCOPES */
};

/** IPv6 address */
typedef struct ip6_addr ip6_addr_t;

/** Set an IPv6 partial address given by byte-parts */
#define IP6_ADDR_PART(ip6addr, index, a,b,c,d) \
  (ip6addr)->addr[index] = PP_HTONL(LWIP_MAKEU32(a,b,c,d))

/** Set a full IPv6 address by passing the 4 u32_t indices in network byte order
    (use PP_HTONL() for constants) */
#define IP6_ADDR(ip6addr, idx0, idx1, idx2, idx3) do { \
  (ip6addr)->addr[0] = idx0; \
  (ip6addr)->addr[1] = idx1; \
  (ip6addr)->addr[2] = idx2; \
  (ip6addr)->addr[3] = idx3; \
  ip6_addr_clear_zone(ip6addr); } while(0)

/** Access address in 16-bit block */
#define IP6_ADDR_BLOCK1(ip6addr) ((u16_t)((lwip_htonl((ip6addr)->addr[0]) >> 16) & 0xffff))
/** Access address in 16-bit block */
#define IP6_ADDR_BLOCK2(ip6addr) ((u16_t)((lwip_htonl((ip6addr)->addr[0])) & 0xffff))
/** Access address in 16-bit block */
#define IP6_ADDR_BLOCK3(ip6addr) ((u16_t)((lwip_htonl((ip6addr)->addr[1]) >> 16) & 0xffff))
/** Access address in 16-bit block */
#define IP6_ADDR_BLOCK4(ip6addr) ((u16_t)((lwip_htonl((ip6addr)->addr[1])) & 0xffff))
/** Access address in 16-bit block */
#define IP6_ADDR_BLOCK5(ip6addr) ((u16_t)((lwip_htonl((ip6addr)->addr[2]) >> 16) & 0xffff))
/** Access address in 16-bit block */
#define IP6_ADDR_BLOCK6(ip6addr) ((u16_t)((lwip_htonl((ip6addr)->addr[2])) & 0xffff))
/** Access address in 16-bit block */
#define IP6_ADDR_BLOCK7(ip6addr) ((u16_t)((lwip_htonl((ip6addr)->addr[3]) >> 16) & 0xffff))
/** Access address in 16-bit block */
#define IP6_ADDR_BLOCK8(ip6addr) ((u16_t)((lwip_htonl((ip6addr)->addr[3])) & 0xffff))

/** Copy IPv6 address - faster than ip6_addr_set: no NULL check */
#define ip6_addr_copy(dest, src) do{(dest).addr[0] = (src).addr[0]; \
                                    (dest).addr[1] = (src).addr[1]; \
                                    (dest).addr[2] = (src).addr[2]; \
                                    (dest).addr[3] = (src).addr[3]; \
                                    ip6_addr_copy_zone((dest), (src)); }while(0)
/** Safely copy one IPv6 address to another (src may be NULL) */
#define ip6_addr_set(dest, src) do{(dest)->addr[0] = (src) == NULL ? 0 : (src)->addr[0]; \
                                   (dest)->addr[1] = (src) == NULL ? 0 : (src)->addr[1]; \
                                   (dest)->addr[2] = (src) == NULL ? 0 : (src)->addr[2]; \
                                   (dest)->addr[3] = (src) == NULL ? 0 : (src)->addr[3]; \
                                   ip6_addr_set_zone((dest), (src) == NULL ? IP6_NO_ZONE : ip6_addr_zone(src)); }while(0)

/** Copy packed IPv6 address to unpacked IPv6 address; zone is not set */
#define ip6_addr_copy_from_packed(dest, src) do{(dest).addr[0] = (src).addr[0]; \
                                    (dest).addr[1] = (src).addr[1]; \
                                    (dest).addr[2] = (src).addr[2]; \
                                    (dest).addr[3] = (src).addr[3]; \
                                    ip6_addr_clear_zone(&dest); }while(0)

/** Copy unpacked IPv6 address to packed IPv6 address; zone is lost */
#define ip6_addr_copy_to_packed(dest, src) do{(dest).addr[0] = (src).addr[0]; \
                                    (dest).addr[1] = (src).addr[1]; \
                                    (dest).addr[2] = (src).addr[2]; \
                                    (dest).addr[3] = (src).addr[3]; }while(0)

/** Set complete address to zero */
#define ip6_addr_set_zero(ip6addr)    do{(ip6addr)->addr[0] = 0; \
                                         (ip6addr)->addr[1] = 0; \
                                         (ip6addr)->addr[2] = 0; \
                                         (ip6addr)->addr[3] = 0; \
                                         ip6_addr_clear_zone(ip6addr);}while(0)

/** Set address to ipv6 'any' (no need for lwip_htonl()) */
#define ip6_addr_set_any(ip6addr)       ip6_addr_set_zero(ip6addr)
/** Set address to ipv6 loopback address */
#define ip6_addr_set_loopback(ip6addr) do{(ip6addr)->addr[0] = 0; \
                                          (ip6addr)->addr[1] = 0; \
                                          (ip6addr)->addr[2] = 0; \
                                          (ip6addr)->addr[3] = PP_HTONL(0x00000001UL); \
                                          ip6_addr_clear_zone(ip6addr);}while(0)
/** Safely copy one IPv6 address to another and change byte order
 * from host- to network-order. */
#define ip6_addr_set_hton(dest, src) do{(dest)->addr[0] = (src) == NULL ? 0 : lwip_htonl((src)->addr[0]); \
                                        (dest)->addr[1] = (src) == NULL ? 0 : lwip_htonl((src)->addr[1]); \
                                        (dest)->addr[2] = (src) == NULL ? 0 : lwip_htonl((src)->addr[2]); \
                                        (dest)->addr[3] = (src) == NULL ? 0 : lwip_htonl((src)->addr[3]); \
                                        ip6_addr_set_zone((dest), (src) == NULL ? IP6_NO_ZONE : ip6_addr_zone(src));}while(0)


/** Compare IPv6 networks, ignoring zone information. To be used sparingly! */
#define ip6_addr_netcmp_zoneless(addr1, addr2) (((addr1)->addr[0] == (addr2)->addr[0]) && \
                                               ((addr1)->addr[1] == (addr2)->addr[1]))

/**
 * Determine if two IPv6 address are on the same network.
 *
 * @param addr1 IPv6 address 1
 * @param addr2 IPv6 address 2
 * @return 1 if the network identifiers of both address match, 0 if not
 */
#define ip6_addr_netcmp(addr1, addr2) (ip6_addr_netcmp_zoneless((addr1), (addr2)) && \
                                       ip6_addr_cmp_zone((addr1), (addr2)))

/* Exact-host comparison *after* ip6_addr_netcmp() succeeded, for efficiency. */
#define ip6_addr_nethostcmp(addr1, addr2) (((addr1)->addr[2] == (addr2)->addr[2]) && \
                                           ((addr1)->addr[3] == (addr2)->addr[3]))

/** Compare IPv6 addresses, ignoring zone information. To be used sparingly! */
#define ip6_addr_cmp_zoneless(addr1, addr2) (((addr1)->addr[0] == (addr2)->addr[0]) && \
                                    ((addr1)->addr[1] == (addr2)->addr[1]) && \
                                    ((addr1)->addr[2] == (addr2)->addr[2]) && \
                                    ((addr1)->addr[3] == (addr2)->addr[3]))
/**
 * Determine if two IPv6 addresses are the same. In particular, the address
 * part of both must be the same, and the zone must be compatible.
 *
 * @param addr1 IPv6 address 1
 * @param addr2 IPv6 address 2
 * @return 1 if the addresses are considered equal, 0 if not
 */
#define ip6_addr_cmp(addr1, addr2) (ip6_addr_cmp_zoneless((addr1), (addr2)) && \
                                    ip6_addr_cmp_zone((addr1), (addr2)))

/** Compare IPv6 address to packed address and zone */
#define ip6_addr_cmp_packed(ip6addr, paddr, zone_idx) (((ip6addr)->addr[0] == (paddr)->addr[0]) && \
                                    ((ip6addr)->addr[1] == (paddr)->addr[1]) && \
                                    ((ip6addr)->addr[2] == (paddr)->addr[2]) && \
                                    ((ip6addr)->addr[3] == (paddr)->addr[3]) && \
                                    ip6_addr_equals_zone((ip6addr), (zone_idx)))

#define ip6_get_subnet_id(ip6addr)   (lwip_htonl((ip6addr)->addr[2]) & 0x0000ffffUL)

#define ip6_addr_isany_val(ip6addr) (((ip6addr).addr[0] == 0) && \
                                     ((ip6addr).addr[1] == 0) && \
                                     ((ip6addr).addr[2] == 0) && \
                                     ((ip6addr).addr[3] == 0))
#define ip6_addr_isany(ip6addr) (((ip6addr) == NULL) || ip6_addr_isany_val(*(ip6addr)))

#define ip6_addr_isloopback(ip6addr) (((ip6addr)->addr[0] == 0UL) && \
                                      ((ip6addr)->addr[1] == 0UL) && \
                                      ((ip6addr)->addr[2] == 0UL) && \
                                      ((ip6addr)->addr[3] == PP_HTONL(0x00000001UL)))

#define ip6_addr_isglobal(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xe0000000UL)) == PP_HTONL(0x20000000UL))

#define ip6_addr_islinklocal(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xffc00000UL)) == PP_HTONL(0xfe800000UL))

#define ip6_addr_issitelocal(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xffc00000UL)) == PP_HTONL(0xfec00000UL))

#define ip6_addr_isuniquelocal(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xfe000000UL)) == PP_HTONL(0xfc000000UL))

#define ip6_addr_isipv4mappedipv6(ip6addr) (((ip6addr)->addr[0] == 0) && ((ip6addr)->addr[1] == 0) && (((ip6addr)->addr[2]) == PP_HTONL(0x0000FFFFUL)))

#define ip6_addr_ismulticast(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xff000000UL)) == PP_HTONL(0xff000000UL))
#define ip6_addr_multicast_transient_flag(ip6addr)  ((ip6addr)->addr[0] & PP_HTONL(0x00100000UL))
#define ip6_addr_multicast_prefix_flag(ip6addr)     ((ip6addr)->addr[0] & PP_HTONL(0x00200000UL))
#define ip6_addr_multicast_rendezvous_flag(ip6addr) ((ip6addr)->addr[0] & PP_HTONL(0x00400000UL))
#define ip6_addr_multicast_scope(ip6addr) ((lwip_htonl((ip6addr)->addr[0]) >> 16) & 0xf)
#define IP6_MULTICAST_SCOPE_RESERVED            0x0
#define IP6_MULTICAST_SCOPE_RESERVED0           0x0
#define IP6_MULTICAST_SCOPE_INTERFACE_LOCAL     0x1
#define IP6_MULTICAST_SCOPE_LINK_LOCAL          0x2
#define IP6_MULTICAST_SCOPE_RESERVED3           0x3
#define IP6_MULTICAST_SCOPE_ADMIN_LOCAL         0x4
#define IP6_MULTICAST_SCOPE_SITE_LOCAL          0x5
#define IP6_MULTICAST_SCOPE_ORGANIZATION_LOCAL  0x8
#define IP6_MULTICAST_SCOPE_GLOBAL              0xe
#define IP6_MULTICAST_SCOPE_RESERVEDF           0xf
#define ip6_addr_ismulticast_iflocal(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xff8f0000UL)) == PP_HTONL(0xff010000UL))
#define ip6_addr_ismulticast_linklocal(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xff8f0000UL)) == PP_HTONL(0xff020000UL))
#define ip6_addr_ismulticast_adminlocal(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xff8f0000UL)) == PP_HTONL(0xff040000UL))
#define ip6_addr_ismulticast_sitelocal(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xff8f0000UL)) == PP_HTONL(0xff050000UL))
#define ip6_addr_ismulticast_orglocal(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xff8f0000UL)) == PP_HTONL(0xff080000UL))
#define ip6_addr_ismulticast_global(ip6addr) (((ip6addr)->addr[0] & PP_HTONL(0xff8f0000UL)) == PP_HTONL(0xff0e0000UL))

/* Scoping note: while interface-local and link-local multicast addresses do
 * have a scope (i.e., they are meaningful only in the context of a particular
 * interface), the following functions are not assigning or comparing zone
 * indices. The reason for this is backward compatibility. Any call site that
 * produces a non-global multicast address must assign a multicast address as
 * appropriate itself. */

#define ip6_addr_isallnodes_iflocal(ip6addr) (((ip6addr)->addr[0] == PP_HTONL(0xff010000UL)) && \
    ((ip6addr)->addr[1] == 0UL) && \
    ((ip6addr)->addr[2] == 0UL) && \
    ((ip6addr)->addr[3] == PP_HTONL(0x00000001UL)))

#define ip6_addr_isallnodes_linklocal(ip6addr) (((ip6addr)->addr[0] == PP_HTONL(0xff020000UL)) && \
    ((ip6addr)->addr[1] == 0UL) && \
    ((ip6addr)->addr[2] == 0UL) && \
    ((ip6addr)->addr[3] == PP_HTONL(0x00000001UL)))
#define ip6_addr_set_allnodes_linklocal(ip6addr) do{(ip6addr)->addr[0] = PP_HTONL(0xff020000UL); \
                (ip6addr)->addr[1] = 0; \
                (ip6addr)->addr[2] = 0; \
                (ip6addr)->addr[3] = PP_HTONL(0x00000001UL); \
                ip6_addr_clear_zone(ip6addr); }while(0)

#define ip6_addr_isallrouters_linklocal(ip6addr) (((ip6addr)->addr[0] == PP_HTONL(0xff020000UL)) && \
    ((ip6addr)->addr[1] == 0UL) && \
    ((ip6addr)->addr[2] == 0UL) && \
    ((ip6addr)->addr[3] == PP_HTONL(0x00000002UL)))
#define ip6_addr_set_allrouters_linklocal(ip6addr) do{(ip6addr)->addr[0] = PP_HTONL(0xff020000UL); \
                (ip6addr)->addr[1] = 0; \
                (ip6addr)->addr[2] = 0; \
                (ip6addr)->addr[3] = PP_HTONL(0x00000002UL); \
                ip6_addr_clear_zone(ip6addr); }while(0)

#define ip6_addr_issolicitednode(ip6addr) ( ((ip6addr)->addr[0] == PP_HTONL(0xff020000UL)) && \
        ((ip6addr)->addr[2] == PP_HTONL(0x00000001UL)) && \
        (((ip6addr)->addr[3] & PP_HTONL(0xff000000UL)) == PP_HTONL(0xff000000UL)) )

#define ip6_addr_set_solicitednode(ip6addr, if_id) do{(ip6addr)->addr[0] = PP_HTONL(0xff020000UL); \
                (ip6addr)->addr[1] = 0; \
                (ip6addr)->addr[2] = PP_HTONL(0x00000001UL); \
                (ip6addr)->addr[3] = (PP_HTONL(0xff000000UL) | (if_id)); \
                ip6_addr_clear_zone(ip6addr); }while(0)

#define ip6_addr_cmp_solicitednode(ip6addr, sn_addr) (((ip6addr)->addr[0] == PP_HTONL(0xff020000UL)) && \
                                    ((ip6addr)->addr[1] == 0) && \
                                    ((ip6addr)->addr[2] == PP_HTONL(0x00000001UL)) && \
                                    ((ip6addr)->addr[3] == (PP_HTONL(0xff000000UL) | (sn_addr)->addr[3])))

/* IPv6 address states. */
#define IP6_ADDR_INVALID      0x00
#define IP6_ADDR_TENTATIVE    0x08
#define IP6_ADDR_TENTATIVE_1  0x09 /* 1 probe sent */
#define IP6_ADDR_TENTATIVE_2  0x0a /* 2 probes sent */
#define IP6_ADDR_TENTATIVE_3  0x0b /* 3 probes sent */
#define IP6_ADDR_TENTATIVE_4  0x0c /* 4 probes sent */
#define IP6_ADDR_TENTATIVE_5  0x0d /* 5 probes sent */
#define IP6_ADDR_TENTATIVE_6  0x0e /* 6 probes sent */
#define IP6_ADDR_TENTATIVE_7  0x0f /* 7 probes sent */
#define IP6_ADDR_VALID        0x10 /* This bit marks an address as valid (preferred or deprecated) */
#define IP6_ADDR_PREFERRED    0x30
#define IP6_ADDR_DEPRECATED   0x10 /* Same as VALID (valid but not preferred) */
#define IP6_ADDR_DUPLICATED   0x40 /* Failed DAD test, not valid */

#define IP6_ADDR_TENTATIVE_COUNT_MASK 0x07 /* 1-7 probes sent */

#define ip6_addr_isinvalid(addr_state) (addr_state == IP6_ADDR_INVALID)
#define ip6_addr_istentative(addr_state) (addr_state & IP6_ADDR_TENTATIVE)
#define ip6_addr_isvalid(addr_state) (addr_state & IP6_ADDR_VALID) /* Include valid, preferred, and deprecated. */
#define ip6_addr_ispreferred(addr_state) (addr_state == IP6_ADDR_PREFERRED)
#define ip6_addr_isdeprecated(addr_state) (addr_state == IP6_ADDR_DEPRECATED)
#define ip6_addr_isduplicated(addr_state) (addr_state == IP6_ADDR_DUPLICATED)

#if LWIP_IPV6_ADDRESS_LIFETIMES
#define IP6_ADDR_LIFE_STATIC   (0)
#define IP6_ADDR_LIFE_INFINITE (0xffffffffUL)
#define ip6_addr_life_isstatic(addr_life) ((addr_life) == IP6_ADDR_LIFE_STATIC)
#define ip6_addr_life_isinfinite(addr_life) ((addr_life) == IP6_ADDR_LIFE_INFINITE)
#endif /* LWIP_IPV6_ADDRESS_LIFETIMES */

#define ip6_addr_debug_print_parts(debug, a, b, c, d, e, f, g, h) \
  LWIP_DEBUGF(debug, ("%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F, \
                      a, b, c, d, e, f, g, h))
#define ip6_addr_debug_print(debug, ipaddr) \
  ip6_addr_debug_print_parts(debug, \
                      (u16_t)((ipaddr) != NULL ? IP6_ADDR_BLOCK1(ipaddr) : 0),    \
                      (u16_t)((ipaddr) != NULL ? IP6_ADDR_BLOCK2(ipaddr) : 0),    \
                      (u16_t)((ipaddr) != NULL ? IP6_ADDR_BLOCK3(ipaddr) : 0),    \
                      (u16_t)((ipaddr) != NULL ? IP6_ADDR_BLOCK4(ipaddr) : 0),    \
                      (u16_t)((ipaddr) != NULL ? IP6_ADDR_BLOCK5(ipaddr) : 0),    \
                      (u16_t)((ipaddr) != NULL ? IP6_ADDR_BLOCK6(ipaddr) : 0),    \
                      (u16_t)((ipaddr) != NULL ? IP6_ADDR_BLOCK7(ipaddr) : 0),    \
                      (u16_t)((ipaddr) != NULL ? IP6_ADDR_BLOCK8(ipaddr) : 0))
#define ip6_addr_debug_print_val(debug, ipaddr) \
  ip6_addr_debug_print_parts(debug, \
                      IP6_ADDR_BLOCK1(&(ipaddr)),    \
                      IP6_ADDR_BLOCK2(&(ipaddr)),    \
                      IP6_ADDR_BLOCK3(&(ipaddr)),    \
                      IP6_ADDR_BLOCK4(&(ipaddr)),    \
                      IP6_ADDR_BLOCK5(&(ipaddr)),    \
                      IP6_ADDR_BLOCK6(&(ipaddr)),    \
                      IP6_ADDR_BLOCK7(&(ipaddr)),    \
                      IP6_ADDR_BLOCK8(&(ipaddr)))

#define IP6ADDR_STRLEN_MAX    46

int ip6addr_aton(const char *cp, ip6_addr_t *addr);
/** returns ptr to static buffer; not reentrant! */
char *ip6addr_ntoa(const ip6_addr_t *addr);
char *ip6addr_ntoa_r(const ip6_addr_t *addr, char *buf, int buflen);



#ifdef __cplusplus
}
#endif

#endif /* LWIP_IPV6 */

#endif /* LWIP_HDR_IP6_ADDR_H */
