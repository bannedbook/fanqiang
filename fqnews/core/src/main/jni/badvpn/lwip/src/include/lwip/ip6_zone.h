/**
 * @file
 *
 * IPv6 address scopes, zones, and scoping policy.
 *
 * This header provides the means to implement support for IPv6 address scopes,
 * as per RFC 4007. An address scope can be either global or more constrained.
 * In lwIP, we say that an address "has a scope" or "is scoped" when its scope
 * is constrained, in which case the address is meaningful only in a specific
 * "zone." For unicast addresses, only link-local addresses have a scope; in
 * that case, the scope is the link. For multicast addresses, there are various
 * scopes defined by RFC 4007 and others. For any constrained scope, a system
 * must establish a (potentially one-to-many) mapping between zones and local
 * interfaces. For example, a link-local address is valid on only one link (its
 * zone). That link may be attached to one or more local interfaces. The
 * decisions on which scopes are constrained and the mapping between zones and
 * interfaces is together what we refer to as the "scoping policy" - more on
 * this in a bit.
 *
 * In lwIP, each IPv6 address has an associated zone index. This zone index may
 * be set to "no zone" (IP6_NO_ZONE, 0) or an actual zone. We say that an
 * address "has a zone" or "is zoned" when its zone index is *not* set to "no
 * zone." In lwIP, in principle, each address should be "properly zoned," which
 * means that if the address has a zone if and only if has a scope. As such, it
 * is a rule that an unscoped (e.g., global) address must never have a zone.
 * Even though one could argue that there is always one zone even for global
 * scopes, this rule exists for implementation simplicity. Violation of the
 * rule will trigger assertions or otherwise result in undesired behavior.
 *
 * Backward compatibility prevents us from requiring that applications always
 * provide properly zoned addresses. We do enforce the rule that the in the
 * lwIP link layer (everything below netif->output_ip6() and in particular ND6)
 * *all* addresses are properly zoned. Thus, on the output paths down the
 * stack, various places deal with the case of addresses that lack a zone.
 * Some of them are best-effort for efficiency (e.g. the PCB bind and connect
 * API calls' attempts to add missing zones); ultimately the IPv6 output
 * handler (@ref ip6_output_if_src) will set a zone if necessary.
 *
 * Aside from dealing with scoped addresses lacking a zone, a proper IPv6
 * implementation must also ensure that a packet with a scoped source and/or
 * destination address does not leave its zone. This is currently implemented
 * in the input and forward functions. However, for output, these checks are
 * deliberately omitted in order to keep the implementation lightweight. The
 * routing algorithm in @ref ip6_route will take decisions such that it will
 * not cause zone violations unless the application sets bad addresses, though.
 *
 * In terms of scoping policy, lwIP implements the default policy from RFC 4007
 * using macros in this file. This policy considers link-local unicast
 * addresses and (only) interface-local and link-local multicast addresses as
 * having a scope. For all these addresses, the zone is equal to the interface.
 * As shown below in this file, it is possible to implement a custom policy.
 */

/*
 * Copyright (c) 2017 The MINIX 3 Project.
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
 * Author: David van Moolenbroek <david@minix3.org>
 *
 */
#ifndef LWIP_HDR_IP6_ZONE_H
#define LWIP_HDR_IP6_ZONE_H

/**
 * @defgroup ip6_zones IPv6 Zones
 * @ingroup ip6
 * @{
 */

#if LWIP_IPV6  /* don't build if not configured for use in lwipopts.h */

/** Identifier for "no zone". */
#define IP6_NO_ZONE 0

#if LWIP_IPV6_SCOPES

/** Zone initializer for static IPv6 address initialization, including comma. */
#define IPADDR6_ZONE_INIT , IP6_NO_ZONE

/** Return the zone index of the given IPv6 address; possibly "no zone". */
#define ip6_addr_zone(ip6addr) ((ip6addr)->zone)

/** Does the given IPv6 address have a zone set? (0/1) */
#define ip6_addr_has_zone(ip6addr) (ip6_addr_zone(ip6addr) != IP6_NO_ZONE)

/** Set the zone field of an IPv6 address to a particular value. */
#define ip6_addr_set_zone(ip6addr, zone_idx) ((ip6addr)->zone = (zone_idx))

/** Clear the zone field of an IPv6 address, setting it to "no zone". */
#define ip6_addr_clear_zone(ip6addr) ((ip6addr)->zone = IP6_NO_ZONE)

/** Copy the zone field from the second IPv6 address to the first one. */
#define ip6_addr_copy_zone(ip6addr1, ip6addr2) ((ip6addr1).zone = (ip6addr2).zone)

/** Is the zone field of the given IPv6 address equal to the given zone index? (0/1) */
#define ip6_addr_equals_zone(ip6addr, zone_idx) ((ip6addr)->zone == (zone_idx))

/** Are the zone fields of the given IPv6 addresses equal? (0/1)
 * This macro must only be used on IPv6 addresses of the same scope. */
#define ip6_addr_cmp_zone(ip6addr1, ip6addr2) ((ip6addr1)->zone == (ip6addr2)->zone)

/** Symbolic constants for the 'type' parameters in some of the macros.
 * These exist for efficiency only, allowing the macros to avoid certain tests
 * when the address is known not to be of a certain type. Dead code elimination
 * will do the rest. IP6_MULTICAST is supported but currently not optimized.
 * @see ip6_addr_has_scope, ip6_addr_assign_zone, ip6_addr_lacks_zone.
 */
enum lwip_ipv6_scope_type
{
  /** Unknown */
  IP6_UNKNOWN   = 0,
  /** Unicast */
  IP6_UNICAST   = 1,
  /** Multicast */
  IP6_MULTICAST = 2
};

/** IPV6_CUSTOM_SCOPES: together, the following three macro definitions,
 * @ref ip6_addr_has_scope, @ref ip6_addr_assign_zone, and
 * @ref ip6_addr_test_zone, completely define the lwIP scoping policy.
 * The definitions below implement the default policy from RFC 4007 Sec. 6.
 * Should an implementation desire to implement a different policy, it can
 * define IPV6_CUSTOM_SCOPES to 1 and supply its own definitions for the three
 * macros instead.
 */
#ifndef IPV6_CUSTOM_SCOPES
#define IPV6_CUSTOM_SCOPES 0
#endif /* !IPV6_CUSTOM_SCOPES */

#if !IPV6_CUSTOM_SCOPES

/**
 * Determine whether an IPv6 address has a constrained scope, and as such is
 * meaningful only if accompanied by a zone index to identify the scope's zone.
 * The given address type may be used to eliminate at compile time certain
 * checks that will evaluate to false at run time anyway.
 *
 * This default implementation follows the default model of RFC 4007, where
 * only interface-local and link-local scopes are defined.
 *
 * Even though the unicast loopback address does have an implied link-local
 * scope, in this implementation it does not have an explicitly assigned zone
 * index. As such it should not be tested for in this macro.
 *
 * @param ip6addr the IPv6 address (const); only its address part is examined.
 * @param type address type; see @ref lwip_ipv6_scope_type.
 * @return 1 if the address has a constrained scope, 0 if it does not.
 */
#define ip6_addr_has_scope(ip6addr, type) \
  (ip6_addr_islinklocal(ip6addr) || (((type) != IP6_UNICAST) && \
   (ip6_addr_ismulticast_iflocal(ip6addr) || \
    ip6_addr_ismulticast_linklocal(ip6addr))))

/**
 * Assign a zone index to an IPv6 address, based on a network interface. If the
 * given address has a scope, the assigned zone index is that scope's zone of
 * the given netif; otherwise, the assigned zone index is "no zone".
 *
 * This default implementation follows the default model of RFC 4007, where
 * only interface-local and link-local scopes are defined, and the zone index
 * of both of those scopes always equals the index of the network interface.
 * As such, this default implementation need not distinguish between different
 * constrained scopes when assigning the zone.
 *
 * @param ip6addr the IPv6 address; its address part is examined, and its zone
 *                index is assigned.
 * @param type address type; see @ref lwip_ipv6_scope_type.
 * @param netif the network interface (const).
 */
#define ip6_addr_assign_zone(ip6addr, type, netif) \
    (ip6_addr_set_zone((ip6addr), \
      ip6_addr_has_scope((ip6addr), (type)) ? netif_get_index(netif) : 0))

/**
 * Test whether an IPv6 address is "zone-compatible" with a network interface.
 * That is, test whether the network interface is part of the zone associated
 * with the address. For efficiency, this macro is only ever called if the
 * given address is either scoped or zoned, and thus, it need not test this.
 * If an address is scoped but not zoned, or zoned and not scoped, it is
 * considered not zone-compatible with any netif.
 *
 * This default implementation follows the default model of RFC 4007, where
 * only interface-local and link-local scopes are defined, and the zone index
 * of both of those scopes always equals the index of the network interface.
 * As such, there is always only one matching netif for a specific zone index,
 * but all call sites of this macro currently support multiple matching netifs
 * as well (at no additional expense in the common case).
 *
 * @param ip6addr the IPv6 address (const).
 * @param netif the network interface (const).
 * @return 1 if the address is scope-compatible with the netif, 0 if not.
 */
#define ip6_addr_test_zone(ip6addr, netif) \
    (ip6_addr_equals_zone((ip6addr), netif_get_index(netif)))

#endif /* !IPV6_CUSTOM_SCOPES */

/** Does the given IPv6 address have a scope, and as such should also have a
 * zone to be meaningful, but does not actually have a zone? (0/1) */
#define ip6_addr_lacks_zone(ip6addr, type) \
    (!ip6_addr_has_zone(ip6addr) && ip6_addr_has_scope((ip6addr), (type)))

/**
 * Try to select a zone for a scoped address that does not yet have a zone.
 * Called from PCB bind and connect routines, for two reasons: 1) to save on
 * this (relatively expensive) selection for every individual packet route
 * operation and 2) to allow the application to obtain the selected zone from
 * the PCB as is customary for e.g. getsockname/getpeername BSD socket calls.
 *
 * Ideally, callers would always supply a properly zoned address, in which case
 * this function would not be needed. It exists both for compatibility with the
 * BSD socket API (which accepts zoneless destination addresses) and for
 * backward compatibility with pre-scoping lwIP code.
 *
 * It may be impossible to select a zone, e.g. if there are no netifs.  In that
 * case, the address's zone field will be left as is.
 *
 * @param dest the IPv6 address for which to select and set a zone.
 * @param src source IPv6 address (const); may be equal to dest.
 */
#define ip6_addr_select_zone(dest, src) do { struct netif *selected_netif; \
  selected_netif = ip6_route((src), (dest)); \
  if (selected_netif != NULL) { \
    ip6_addr_assign_zone((dest), IP6_UNKNOWN, selected_netif); \
  } } while (0)

/**
 * @}
 */

#else /* LWIP_IPV6_SCOPES */

#define IPADDR6_ZONE_INIT
#define ip6_addr_zone(ip6addr) (IP6_NO_ZONE)
#define ip6_addr_has_zone(ip6addr) (0)
#define ip6_addr_set_zone(ip6addr, zone_idx)
#define ip6_addr_clear_zone(ip6addr)
#define ip6_addr_copy_zone(ip6addr1, ip6addr2)
#define ip6_addr_equals_zone(ip6addr, zone_idx) (1)
#define ip6_addr_cmp_zone(ip6addr1, ip6addr2) (1)
#define IPV6_CUSTOM_SCOPES 0
#define ip6_addr_has_scope(ip6addr, type) (0)
#define ip6_addr_assign_zone(ip6addr, type, netif)
#define ip6_addr_test_zone(ip6addr, netif) (1)
#define ip6_addr_lacks_zone(ip6addr, type) (0)
#define ip6_addr_select_zone(ip6addr, src)

#endif /* LWIP_IPV6_SCOPES */

#if LWIP_IPV6_SCOPES && LWIP_IPV6_SCOPES_DEBUG

/** Verify that the given IPv6 address is properly zoned. */
#define IP6_ADDR_ZONECHECK(ip6addr) LWIP_ASSERT("IPv6 zone check failed", \
    ip6_addr_has_scope(ip6addr, IP6_UNKNOWN) == ip6_addr_has_zone(ip6addr))

/** Verify that the given IPv6 address is properly zoned for the given netif. */
#define IP6_ADDR_ZONECHECK_NETIF(ip6addr, netif) LWIP_ASSERT("IPv6 netif zone check failed", \
    ip6_addr_has_scope(ip6addr, IP6_UNKNOWN) ? \
    (ip6_addr_has_zone(ip6addr) && \
     (((netif) == NULL) || ip6_addr_test_zone((ip6addr), (netif)))) : \
    !ip6_addr_has_zone(ip6addr))

#else /* LWIP_IPV6_SCOPES && LWIP_IPV6_SCOPES_DEBUG */

#define IP6_ADDR_ZONECHECK(ip6addr)
#define IP6_ADDR_ZONECHECK_NETIF(ip6addr, netif)

#endif /* LWIP_IPV6_SCOPES && LWIP_IPV6_SCOPES_DEBUG */

#endif /* LWIP_IPV6 */

#endif /* LWIP_HDR_IP6_ADDR_H */
