/**
 * @file
 *
 * IPv6 layer.
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
 *
 * Please coordinate changes and requests with Ivan Delamer
 * <delamer@inicotech.com>
 */

#include "lwip/opt.h"

#if LWIP_IPV6  /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/ip.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/ip6_frag.h"
#include "lwip/icmp6.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/dhcp6.h"
#include "lwip/nd6.h"
#include "lwip/mld6.h"
#include "lwip/debug.h"
#include "lwip/stats.h"

#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif

/**
 * Finds the appropriate network interface for a given IPv6 address. It tries to select
 * a netif following a sequence of heuristics:
 * 1) if there is only 1 netif, return it
 * 2) if the destination is a zoned address, match its zone to a netif
 * 3) if the either the source or destination address is a scoped address,
 *    match the source address's zone (if set) or address (if not) to a netif
 * 4) tries to match the destination subnet to a configured address
 * 5) tries to find a router-announced route
 * 6) tries to match the (unscoped) source address to the netif
 * 7) returns the default netif, if configured
 *
 * Note that each of the two given addresses may or may not be properly zoned.
 *
 * @param src the source IPv6 address, if known
 * @param dest the destination IPv6 address for which to find the route
 * @return the netif on which to send to reach dest
 */
struct netif *
ip6_route(const ip6_addr_t *src, const ip6_addr_t *dest)
{
#if LWIP_SINGLE_NETIF
  LWIP_UNUSED_ARG(src);
  LWIP_UNUSED_ARG(dest);
#else /* LWIP_SINGLE_NETIF */
  struct netif *netif;
  s8_t i;

  /* If single netif configuration, fast return. */
  if ((netif_list != NULL) && (netif_list->next == NULL)) {
    if (!netif_is_up(netif_list) || !netif_is_link_up(netif_list) ||
        (ip6_addr_has_zone(dest) && !ip6_addr_test_zone(dest, netif_list))) {
      return NULL;
    }
    return netif_list;
  }

#if LWIP_IPV6_SCOPES
  /* Special processing for zoned destination addresses. This includes link-
   * local unicast addresses and interface/link-local multicast addresses. Use
   * the zone to find a matching netif. If the address is not zoned, then there
   * is technically no "wrong" netif to choose, and we leave routing to other
   * rules; in most cases this should be the scoped-source rule below. */
  if (ip6_addr_has_zone(dest)) {
    IP6_ADDR_ZONECHECK(dest);
    /* Find a netif based on the zone. For custom mappings, one zone may map
     * to multiple netifs, so find one that can actually send a packet. */
    for (netif = netif_list; netif != NULL; netif = netif->next) {
      if (ip6_addr_test_zone(dest, netif) &&
          netif_is_up(netif) && netif_is_link_up(netif)) {
        return netif;
      }
    }
    /* No matching netif found. Do no try to route to a different netif,
     * as that would be a zone violation, resulting in any packets sent to
     * that netif being dropped on output. */
    return NULL;
  }
#endif /* LWIP_IPV6_SCOPES */

  /* Special processing for scoped source and destination addresses. If we get
   * here, the destination address does not have a zone, so either way we need
   * to look at the source address, which may or may not have a zone. If it
   * does, the zone is restrictive: there is (typically) only one matching
   * netif for it, and we should avoid routing to any other netif as that would
   * result in guaranteed zone violations. For scoped source addresses that do
   * not have a zone, use (only) a netif that has that source address locally
   * assigned. This case also applies to the loopback source address, which has
   * an implied link-local scope. If only the destination address is scoped
   * (but, again, not zoned), we still want to use only the source address to
   * determine its zone because that's most likely what the user/application
   * wants, regardless of whether the source address is scoped. Finally, some
   * of this story also applies if scoping is disabled altogether. */
#if LWIP_IPV6_SCOPES
  if (ip6_addr_has_scope(dest, IP6_UNKNOWN) ||
      ip6_addr_has_scope(src, IP6_UNICAST) ||
#else /* LWIP_IPV6_SCOPES */
  if (ip6_addr_islinklocal(dest) || ip6_addr_ismulticast_iflocal(dest) ||
      ip6_addr_ismulticast_linklocal(dest) || ip6_addr_islinklocal(src) ||
#endif /* LWIP_IPV6_SCOPES */
      ip6_addr_isloopback(src)) {
#if LWIP_IPV6_SCOPES
    if (ip6_addr_has_zone(src)) {
      /* Find a netif matching the source zone (relatively cheap). */
      for (netif = netif_list; netif != NULL; netif = netif->next) {
        if (netif_is_up(netif) && netif_is_link_up(netif) &&
            ip6_addr_test_zone(src, netif)) {
          return netif;
        }
      }
    } else
#endif /* LWIP_IPV6_SCOPES */
    {
      /* Find a netif matching the source address (relatively expensive). */
      for (netif = netif_list; netif != NULL; netif = netif->next) {
        if (!netif_is_up(netif) || !netif_is_link_up(netif)) {
          continue;
        }
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
          if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
              ip6_addr_cmp_zoneless(src, netif_ip6_addr(netif, i))) {
            return netif;
          }
        }
      }
    }
    /* Again, do not use any other netif in this case, as that could result in
     * zone boundary violations. */
    return NULL;
  }

  /* We come here only if neither source nor destination is scoped. */
  IP6_ADDR_ZONECHECK(src);

#ifdef LWIP_HOOK_IP6_ROUTE
  netif = LWIP_HOOK_IP6_ROUTE(src, dest);
  if (netif != NULL) {
    return netif;
  }
#endif

  /* See if the destination subnet matches a configured address. In accordance
   * with RFC 5942, dynamically configured addresses do not have an implied
   * local subnet, and thus should be considered /128 assignments. However, as
   * such, the destination address may still match a local address, and so we
   * still need to check for exact matches here. By (lwIP) policy, statically
   * configured addresses do always have an implied local /64 subnet. */
  for (netif = netif_list; netif != NULL; netif = netif->next) {
    if (!netif_is_up(netif) || !netif_is_link_up(netif)) {
      continue;
    }
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
          ip6_addr_netcmp(dest, netif_ip6_addr(netif, i)) &&
          (netif_ip6_addr_isstatic(netif, i) ||
          ip6_addr_nethostcmp(dest, netif_ip6_addr(netif, i)))) {
        return netif;
      }
    }
  }

  /* Get the netif for a suitable router-announced route. */
  netif = nd6_find_route(dest);
  if (netif != NULL) {
    return netif;
  }

  /* Try with the netif that matches the source address. Given the earlier rule
   * for scoped source addresses, this applies to unscoped addresses only. */
  if (!ip6_addr_isany(src)) {
    for (netif = netif_list; netif != NULL; netif = netif->next) {
      if (!netif_is_up(netif) || !netif_is_link_up(netif)) {
        continue;
      }
      for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
            ip6_addr_cmp(src, netif_ip6_addr(netif, i))) {
          return netif;
        }
      }
    }
  }

#if LWIP_NETIF_LOOPBACK && !LWIP_HAVE_LOOPIF
  /* loopif is disabled, loopback traffic is passed through any netif */
  if (ip6_addr_isloopback(dest)) {
    /* don't check for link on loopback traffic */
    if (netif_default != NULL && netif_is_up(netif_default)) {
      return netif_default;
    }
    /* default netif is not up, just use any netif for loopback traffic */
    for (netif = netif_list; netif != NULL; netif = netif->next) {
      if (netif_is_up(netif)) {
        return netif;
      }
    }
    return NULL;
  }
#endif /* LWIP_NETIF_LOOPBACK && !LWIP_HAVE_LOOPIF */
#endif /* !LWIP_SINGLE_NETIF */

  /* no matching netif found, use default netif, if up */
  if ((netif_default == NULL) || !netif_is_up(netif_default) || !netif_is_link_up(netif_default)) {
    return NULL;
  }
  return netif_default;
}

/**
 * @ingroup ip6
 * Select the best IPv6 source address for a given destination IPv6 address.
 *
 * This implementation follows RFC 6724 Sec. 5 to the following extent:
 * - Rules 1, 2, 3: fully implemented
 * - Rules 4, 5, 5.5: not applicable
 * - Rule 6: not implemented
 * - Rule 7: not applicable
 * - Rule 8: limited to "prefer /64 subnet match over non-match"
 *
 * For Rule 2, we deliberately deviate from RFC 6724 Sec. 3.1 by considering
 * ULAs to be of smaller scope than global addresses, to avoid that a preferred
 * ULA is picked over a deprecated global address when given a global address
 * as destination, as that would likely result in broken two-way communication.
 *
 * As long as temporary addresses are not supported (as used in Rule 7), a
 * proper implementation of Rule 8 would obviate the need to implement Rule 6.
 *
 * @param netif the netif on which to send a packet
 * @param dest the destination we are trying to reach (possibly not properly
 *             zoned)
 * @return the most suitable source address to use, or NULL if no suitable
 *         source address is found
 */
const ip_addr_t *
ip6_select_source_address(struct netif *netif, const ip6_addr_t *dest)
{
  const ip_addr_t *best_addr;
  const ip6_addr_t *cand_addr;
  s8_t dest_scope, cand_scope;
  s8_t best_scope = IP6_MULTICAST_SCOPE_RESERVED;
  u8_t i, cand_pref, cand_bits;
  u8_t best_pref = 0;
  u8_t best_bits = 0;

  /* Start by determining the scope of the given destination address. These
   * tests are hopefully (roughly) in order of likeliness to match. */
  if (ip6_addr_isglobal(dest)) {
    dest_scope = IP6_MULTICAST_SCOPE_GLOBAL;
  } else if (ip6_addr_islinklocal(dest) || ip6_addr_isloopback(dest)) {
    dest_scope = IP6_MULTICAST_SCOPE_LINK_LOCAL;
  } else if (ip6_addr_isuniquelocal(dest)) {
    dest_scope = IP6_MULTICAST_SCOPE_ORGANIZATION_LOCAL;
  } else if (ip6_addr_ismulticast(dest)) {
    dest_scope = ip6_addr_multicast_scope(dest);
  } else if (ip6_addr_issitelocal(dest)) {
    dest_scope = IP6_MULTICAST_SCOPE_SITE_LOCAL;
  } else {
    /* no match, consider scope global */
    dest_scope = IP6_MULTICAST_SCOPE_GLOBAL;
  }

  best_addr = NULL;

  for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
    /* Consider only valid (= preferred and deprecated) addresses. */
    if (!ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
      continue;
    }
    /* Determine the scope of this candidate address. Same ordering idea. */
    cand_addr = netif_ip6_addr(netif, i);
    if (ip6_addr_isglobal(cand_addr)) {
      cand_scope = IP6_MULTICAST_SCOPE_GLOBAL;
    } else if (ip6_addr_islinklocal(cand_addr)) {
      cand_scope = IP6_MULTICAST_SCOPE_LINK_LOCAL;
    } else if (ip6_addr_isuniquelocal(cand_addr)) {
      cand_scope = IP6_MULTICAST_SCOPE_ORGANIZATION_LOCAL;
    } else if (ip6_addr_issitelocal(cand_addr)) {
      cand_scope = IP6_MULTICAST_SCOPE_SITE_LOCAL;
    } else {
      /* no match, treat as low-priority global scope */
      cand_scope = IP6_MULTICAST_SCOPE_RESERVEDF;
    }
    cand_pref = ip6_addr_ispreferred(netif_ip6_addr_state(netif, i));
    /* @todo compute the actual common bits, for longest matching prefix. */
    /* We cannot count on the destination address having a proper zone
     * assignment, so do not compare zones in this case. */
    cand_bits = ip6_addr_netcmp_zoneless(cand_addr, dest); /* just 1 or 0 for now */
    if (cand_bits && ip6_addr_nethostcmp(cand_addr, dest)) {
      return netif_ip_addr6(netif, i); /* Rule 1 */
    }
    if ((best_addr == NULL) || /* no alternative yet */
        ((cand_scope < best_scope) && (cand_scope >= dest_scope)) ||
        ((cand_scope > best_scope) && (best_scope < dest_scope)) || /* Rule 2 */
        ((cand_scope == best_scope) && ((cand_pref > best_pref) || /* Rule 3 */
        ((cand_pref == best_pref) && (cand_bits > best_bits))))) { /* Rule 8 */
      /* We found a new "winning" candidate. */
      best_addr = netif_ip_addr6(netif, i);
      best_scope = cand_scope;
      best_pref = cand_pref;
      best_bits = cand_bits;
    }
  }

  return best_addr; /* may be NULL */
}

#if LWIP_IPV6_FORWARD
/**
 * Forwards an IPv6 packet. It finds an appropriate route for the
 * packet, decrements the HL value of the packet, and outputs
 * the packet on the appropriate interface.
 *
 * @param p the packet to forward (p->payload points to IP header)
 * @param iphdr the IPv6 header of the input packet
 * @param inp the netif on which this packet was received
 */
static void
ip6_forward(struct pbuf *p, struct ip6_hdr *iphdr, struct netif *inp)
{
  struct netif *netif;

  /* do not forward link-local or loopback addresses */
  if (ip6_addr_islinklocal(ip6_current_dest_addr()) ||
      ip6_addr_isloopback(ip6_current_dest_addr())) {
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_forward: not forwarding link-local address.\n"));
    IP6_STATS_INC(ip6.rterr);
    IP6_STATS_INC(ip6.drop);
    return;
  }

  /* Find network interface where to forward this IP packet to. */
  netif = ip6_route(IP6_ADDR_ANY6, ip6_current_dest_addr());
  if (netif == NULL) {
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_forward: no route for %"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F"\n",
        IP6_ADDR_BLOCK1(ip6_current_dest_addr()),
        IP6_ADDR_BLOCK2(ip6_current_dest_addr()),
        IP6_ADDR_BLOCK3(ip6_current_dest_addr()),
        IP6_ADDR_BLOCK4(ip6_current_dest_addr()),
        IP6_ADDR_BLOCK5(ip6_current_dest_addr()),
        IP6_ADDR_BLOCK6(ip6_current_dest_addr()),
        IP6_ADDR_BLOCK7(ip6_current_dest_addr()),
        IP6_ADDR_BLOCK8(ip6_current_dest_addr())));
#if LWIP_ICMP6
    /* Don't send ICMP messages in response to ICMP messages */
    if (IP6H_NEXTH(iphdr) != IP6_NEXTH_ICMP6) {
      icmp6_dest_unreach(p, ICMP6_DUR_NO_ROUTE);
    }
#endif /* LWIP_ICMP6 */
    IP6_STATS_INC(ip6.rterr);
    IP6_STATS_INC(ip6.drop);
    return;
  }
#if LWIP_IPV6_SCOPES
  /* Do not forward packets with a zoned (e.g., link-local) source address
   * outside of their zone. We determined the zone a bit earlier, so we know
   * that the address is properly zoned here, so we can safely use has_zone.
   * Also skip packets with a loopback source address (link-local implied). */
  if ((ip6_addr_has_zone(ip6_current_src_addr()) &&
      !ip6_addr_test_zone(ip6_current_src_addr(), netif)) ||
      ip6_addr_isloopback(ip6_current_src_addr())) {
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_forward: not forwarding packet beyond its source address zone.\n"));
    IP6_STATS_INC(ip6.rterr);
    IP6_STATS_INC(ip6.drop);
    return;
  }
#endif /* LWIP_IPV6_SCOPES */
  /* Do not forward packets onto the same network interface on which
   * they arrived. */
  if (netif == inp) {
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_forward: not bouncing packets back on incoming interface.\n"));
    IP6_STATS_INC(ip6.rterr);
    IP6_STATS_INC(ip6.drop);
    return;
  }

  /* decrement HL */
  IP6H_HOPLIM_SET(iphdr, IP6H_HOPLIM(iphdr) - 1);
  /* send ICMP6 if HL == 0 */
  if (IP6H_HOPLIM(iphdr) == 0) {
#if LWIP_ICMP6
    /* Don't send ICMP messages in response to ICMP messages */
    if (IP6H_NEXTH(iphdr) != IP6_NEXTH_ICMP6) {
      icmp6_time_exceeded(p, ICMP6_TE_HL);
    }
#endif /* LWIP_ICMP6 */
    IP6_STATS_INC(ip6.drop);
    return;
  }

  if (netif->mtu && (p->tot_len > netif->mtu)) {
#if LWIP_ICMP6
    /* Don't send ICMP messages in response to ICMP messages */
    if (IP6H_NEXTH(iphdr) != IP6_NEXTH_ICMP6) {
      icmp6_packet_too_big(p, netif->mtu);
    }
#endif /* LWIP_ICMP6 */
    IP6_STATS_INC(ip6.drop);
    return;
  }

  LWIP_DEBUGF(IP6_DEBUG, ("ip6_forward: forwarding packet to %"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F"\n",
      IP6_ADDR_BLOCK1(ip6_current_dest_addr()),
      IP6_ADDR_BLOCK2(ip6_current_dest_addr()),
      IP6_ADDR_BLOCK3(ip6_current_dest_addr()),
      IP6_ADDR_BLOCK4(ip6_current_dest_addr()),
      IP6_ADDR_BLOCK5(ip6_current_dest_addr()),
      IP6_ADDR_BLOCK6(ip6_current_dest_addr()),
      IP6_ADDR_BLOCK7(ip6_current_dest_addr()),
      IP6_ADDR_BLOCK8(ip6_current_dest_addr())));

  /* transmit pbuf on chosen interface */
  netif->output_ip6(netif, p, ip6_current_dest_addr());
  IP6_STATS_INC(ip6.fw);
  IP6_STATS_INC(ip6.xmit);
  return;
}
#endif /* LWIP_IPV6_FORWARD */

/** Return true if the current input packet should be accepted on this netif */
static int
ip6_input_accept(struct netif *netif)
{
  /* interface is up? */
  if (netif_is_up(netif)) {
    u8_t i;
    /* unicast to this interface address? address configured? */
    /* If custom scopes are used, the destination zone will be tested as
      * part of the local-address comparison, but we need to test the source
      * scope as well (e.g., is this interface on the same link?). */
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
          ip6_addr_cmp(ip6_current_dest_addr(), netif_ip6_addr(netif, i))
#if IPV6_CUSTOM_SCOPES
          && (!ip6_addr_has_zone(ip6_current_src_addr()) ||
              ip6_addr_test_zone(ip6_current_src_addr(), netif))
#endif /* IPV6_CUSTOM_SCOPES */
      ) {
        /* accept on this netif */
        return 1;
      }
    }
  }
  return 0;
}

/**
 * This function is called by the network interface device driver when
 * an IPv6 packet is received. The function does the basic checks of the
 * IP header such as packet size being at least larger than the header
 * size etc. If the packet was not destined for us, the packet is
 * forwarded (using ip6_forward).
 *
 * Finally, the packet is sent to the upper layer protocol input function.
 *
 * @param p the received IPv6 packet (p->payload points to IPv6 header)
 * @param inp the netif on which this packet was received
 * @return ERR_OK if the packet was processed (could return ERR_* if it wasn't
 *         processed, but currently always returns ERR_OK)
 */
err_t
ip6_input(struct pbuf *p, struct netif *inp)
{
  struct ip6_hdr *ip6hdr;
  struct netif *netif;
  const u8_t *nexth;
  u16_t hlen, hlen_tot; /* the current header length */
#if 0 /*IP_ACCEPT_LINK_LAYER_ADDRESSING*/
  @todo
  int check_ip_src=1;
#endif /* IP_ACCEPT_LINK_LAYER_ADDRESSING */

  IP6_STATS_INC(ip6.recv);

  /* identify the IP header */
  ip6hdr = (struct ip6_hdr *)p->payload;
  if (IP6H_V(ip6hdr) != 6) {
    LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_WARNING, ("IPv6 packet dropped due to bad version number %"U32_F"\n",
        IP6H_V(ip6hdr)));
    pbuf_free(p);
    IP6_STATS_INC(ip6.err);
    IP6_STATS_INC(ip6.drop);
    return ERR_OK;
  }

#ifdef LWIP_HOOK_IP6_INPUT
  if (LWIP_HOOK_IP6_INPUT(p, inp)) {
    /* the packet has been eaten */
    return ERR_OK;
  }
#endif

  /* header length exceeds first pbuf length, or ip length exceeds total pbuf length? */
  if ((IP6_HLEN > p->len) || (IP6H_PLEN(ip6hdr) > (p->tot_len - IP6_HLEN))) {
    if (IP6_HLEN > p->len) {
      LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
        ("IPv6 header (len %"U16_F") does not fit in first pbuf (len %"U16_F"), IP packet dropped.\n",
            (u16_t)IP6_HLEN, p->len));
    }
    if ((IP6H_PLEN(ip6hdr) + IP6_HLEN) > p->tot_len) {
      LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
        ("IPv6 (plen %"U16_F") is longer than pbuf (len %"U16_F"), IP packet dropped.\n",
            (u16_t)(IP6H_PLEN(ip6hdr) + IP6_HLEN), p->tot_len));
    }
    /* free (drop) packet pbufs */
    pbuf_free(p);
    IP6_STATS_INC(ip6.lenerr);
    IP6_STATS_INC(ip6.drop);
    return ERR_OK;
  }

  /* Trim pbuf. This should have been done at the netif layer,
   * but we'll do it anyway just to be sure that its done. */
  pbuf_realloc(p, (u16_t)(IP6_HLEN + IP6H_PLEN(ip6hdr)));

  /* copy IP addresses to aligned ip6_addr_t */
  ip_addr_copy_from_ip6_packed(ip_data.current_iphdr_dest, ip6hdr->dest);
  ip_addr_copy_from_ip6_packed(ip_data.current_iphdr_src, ip6hdr->src);

  /* Don't accept virtual IPv4 mapped IPv6 addresses.
   * Don't accept multicast source addresses. */
  if (ip6_addr_isipv4mappedipv6(ip_2_ip6(&ip_data.current_iphdr_dest)) ||
     ip6_addr_isipv4mappedipv6(ip_2_ip6(&ip_data.current_iphdr_src)) ||
     ip6_addr_ismulticast(ip_2_ip6(&ip_data.current_iphdr_src))) {
    IP6_STATS_INC(ip6.err);
    IP6_STATS_INC(ip6.drop);
    return ERR_OK;
  }

  /* Set the appropriate zone identifier on the addresses. */
  ip6_addr_assign_zone(ip_2_ip6(&ip_data.current_iphdr_dest), IP6_UNKNOWN, inp);
  ip6_addr_assign_zone(ip_2_ip6(&ip_data.current_iphdr_src), IP6_UNICAST, inp);

  /* current header pointer. */
  ip_data.current_ip6_header = ip6hdr;

  /* In netif, used in case we need to send ICMPv6 packets back. */
  ip_data.current_netif = inp;
  ip_data.current_input_netif = inp;

  /* match packet against an interface, i.e. is this packet for us? */
  if (ip6_addr_ismulticast(ip6_current_dest_addr())) {
    /* Always joined to multicast if-local and link-local all-nodes group. */
    if (ip6_addr_isallnodes_iflocal(ip6_current_dest_addr()) ||
        ip6_addr_isallnodes_linklocal(ip6_current_dest_addr())) {
      netif = inp;
    }
#if LWIP_IPV6_MLD
    else if (mld6_lookfor_group(inp, ip6_current_dest_addr())) {
      netif = inp;
    }
#else /* LWIP_IPV6_MLD */
    else if (ip6_addr_issolicitednode(ip6_current_dest_addr())) {
      u8_t i;
      /* Filter solicited node packets when MLD is not enabled
       * (for Neighbor discovery). */
      netif = NULL;
      for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(netif_ip6_addr_state(inp, i)) &&
            ip6_addr_cmp_solicitednode(ip6_current_dest_addr(), netif_ip6_addr(inp, i))) {
          netif = inp;
          LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: solicited node packet accepted on interface %c%c\n",
              netif->name[0], netif->name[1]));
          break;
        }
      }
    }
#endif /* LWIP_IPV6_MLD */
    else {
      netif = NULL;
    }
  } else {
    /* start trying with inp. if that's not acceptable, start walking the
       list of configured netifs. */
    if (ip6_input_accept(inp)) {
      netif = inp;
    } else {
      netif = NULL;
#if !IPV6_CUSTOM_SCOPES
      /* Shortcut: stop looking for other interfaces if either the source or
        * the destination has a scope constrained to this interface. Custom
        * scopes may break the 1:1 link/interface mapping, however. */
      if (ip6_addr_islinklocal(ip6_current_dest_addr()) ||
          ip6_addr_islinklocal(ip6_current_src_addr())) {
        goto netif_found;
      }
#endif /* !IPV6_CUSTOM_SCOPES */
#if !LWIP_NETIF_LOOPBACK || LWIP_HAVE_LOOPIF
      /* The loopback address is to be considered link-local. Packets to it
        * should be dropped on other interfaces, as per RFC 4291 Sec. 2.5.3.
        * Its implied scope means packets *from* the loopback address should
        * not be accepted on other interfaces, either. These requirements
        * cannot be implemented in the case that loopback traffic is sent
        * across a non-loopback interface, however. */
      if (ip6_addr_isloopback(ip6_current_dest_addr()) ||
          ip6_addr_isloopback(ip6_current_src_addr())) {
        goto netif_found;
      }
#endif /* !LWIP_NETIF_LOOPBACK || LWIP_HAVE_LOOPIF */
#if !LWIP_SINGLE_NETIF
      NETIF_FOREACH(netif) {
        if (netif == inp) {
          /* we checked that before already */
          continue;
        }
        if (ip6_input_accept(netif)) {
          break;
        }
      }
#endif /* !LWIP_SINGLE_NETIF */
    }
netif_found:
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet accepted on interface %c%c\n",
        netif ? netif->name[0] : 'X', netif? netif->name[1] : 'X'));
  }

  /* "::" packet source address? (used in duplicate address detection) */
  if (ip6_addr_isany(ip6_current_src_addr()) &&
      (!ip6_addr_issolicitednode(ip6_current_dest_addr()))) {
    /* packet source is not valid */
    /* free (drop) packet pbufs */
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with src ANY_ADDRESS dropped\n"));
    pbuf_free(p);
    IP6_STATS_INC(ip6.drop);
    goto ip6_input_cleanup;
  }

  /* if we're pretending we are everyone for TCP, assume the packet is for source interface if it
     isn't for a local address */
  if (netif == NULL && (inp->flags & NETIF_FLAG_PRETEND_TCP) && IP6H_NEXTH(ip6hdr) == IP6_NEXTH_TCP) {
      netif = inp;
  }

  /* packet not for us? */
  if (netif == NULL) {
    /* packet not for us, route or discard */
    LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_TRACE, ("ip6_input: packet not for us.\n"));
#if LWIP_IPV6_FORWARD
    /* non-multicast packet? */
    if (!ip6_addr_ismulticast(ip6_current_dest_addr())) {
      /* try to forward IP packet on (other) interfaces */
      ip6_forward(p, ip6hdr, inp);
    }
#endif /* LWIP_IPV6_FORWARD */
    pbuf_free(p);
    goto ip6_input_cleanup;
  }

  /* current netif pointer. */
  ip_data.current_netif = netif;

  /* Save next header type. */
  nexth = &IP6H_NEXTH(ip6hdr);

  /* Init header length. */
  hlen = hlen_tot = IP6_HLEN;

  /* Move to payload. */
  pbuf_remove_header(p, IP6_HLEN);

  /* Process known option extension headers, if present. */
  while (*nexth != IP6_NEXTH_NONE)
  {
    switch (*nexth) {
    case IP6_NEXTH_HOPBYHOP:
    {
      s32_t opt_offset;
      struct ip6_hbh_hdr *hbh_hdr;
      struct ip6_opt_hdr *opt_hdr;
      LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with Hop-by-Hop options header\n"));

      /* Get and check the header length, while staying in packet bounds. */
      hbh_hdr = (struct ip6_hbh_hdr *)p->payload;

      /* Get next header type. */
      nexth = &IP6_HBH_NEXTH(hbh_hdr);

      /* Get the header length. */
      hlen = (u16_t)(8 * (1 + hbh_hdr->_hlen));

      if ((p->len < 8) || (hlen > p->len)) {
        LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
          ("IPv6 options header (hlen %"U16_F") does not fit in first pbuf (len %"U16_F"), IPv6 packet dropped.\n",
              hlen, p->len));
        /* free (drop) packet pbufs */
        pbuf_free(p);
        IP6_STATS_INC(ip6.lenerr);
        IP6_STATS_INC(ip6.drop);
        goto ip6_input_cleanup;
      }

      hlen_tot = (u16_t)(hlen_tot + hlen);

      /* The extended option header starts right after Hop-by-Hop header. */
      opt_offset = IP6_HBH_HLEN;
      while (opt_offset < hlen)
      {
        s32_t opt_dlen = 0;

        opt_hdr = (struct ip6_opt_hdr *)((u8_t *)hbh_hdr + opt_offset);

        switch (IP6_OPT_TYPE(opt_hdr)) {
        /* @todo: process IPV6 Hop-by-Hop option data */
        case IP6_PAD1_OPTION:
          /* PAD1 option doesn't have length and value field */
          opt_dlen = -1;
          break;
        case IP6_PADN_OPTION:
          opt_dlen = IP6_OPT_DLEN(opt_hdr);
          break;
        case IP6_ROUTER_ALERT_OPTION:
          opt_dlen = IP6_OPT_DLEN(opt_hdr);
          break;
        case IP6_JUMBO_OPTION:
          opt_dlen = IP6_OPT_DLEN(opt_hdr);
          break;
        default:
          /* Check 2 MSB of Hop-by-Hop header type. */
          switch (IP6_OPT_TYPE_ACTION(opt_hdr)) {
          case 1:
            /* Discard the packet. */
            LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with invalid Hop-by-Hop option type dropped.\n"));
            pbuf_free(p);
            IP6_STATS_INC(ip6.drop);
            goto ip6_input_cleanup;
          case 2:
            /* Send ICMP Parameter Problem */
            icmp6_param_problem(p, ICMP6_PP_OPTION, opt_hdr);
            LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with invalid Hop-by-Hop option type dropped.\n"));
            pbuf_free(p);
            IP6_STATS_INC(ip6.drop);
            goto ip6_input_cleanup;
          case 3:
            /* Send ICMP Parameter Problem if destination address is not a multicast address */
            if (!ip6_addr_ismulticast(ip6_current_dest_addr())) {
              icmp6_param_problem(p, ICMP6_PP_OPTION, opt_hdr);
            }
            LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with invalid Hop-by-Hop option type dropped.\n"));
            pbuf_free(p);
            IP6_STATS_INC(ip6.drop);
            goto ip6_input_cleanup;
          default:
            /* Skip over this option. */
            opt_dlen = IP6_OPT_DLEN(opt_hdr);
            break;
          }
          break;
        }

        /* Adjust the offset to move to the next extended option header */
        opt_offset = opt_offset + IP6_OPT_HLEN + opt_dlen;
      }
      pbuf_remove_header(p, hlen);
      break;
    }
    case IP6_NEXTH_DESTOPTS:
    {
      s32_t opt_offset;
      struct ip6_dest_hdr *dest_hdr;
      struct ip6_opt_hdr *opt_hdr;
      LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with Destination options header\n"));

      dest_hdr = (struct ip6_dest_hdr *)p->payload;

      /* Get next header type. */
      nexth = &IP6_DEST_NEXTH(dest_hdr);

      /* Get the header length. */
      hlen = 8 * (1 + dest_hdr->_hlen);
      if ((p->len < 8) || (hlen > p->len)) {
        LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
          ("IPv6 options header (hlen %"U16_F") does not fit in first pbuf (len %"U16_F"), IPv6 packet dropped.\n",
              hlen, p->len));
        /* free (drop) packet pbufs */
        pbuf_free(p);
        IP6_STATS_INC(ip6.lenerr);
        IP6_STATS_INC(ip6.drop);
        goto ip6_input_cleanup;
      }

      hlen_tot = (u16_t)(hlen_tot + hlen);

      /* The extended option header starts right after Destination header. */
      opt_offset = IP6_DEST_HLEN;
      while (opt_offset < hlen)
      {
        s32_t opt_dlen = 0;

        opt_hdr = (struct ip6_opt_hdr *)((u8_t *)dest_hdr + opt_offset);

        switch (IP6_OPT_TYPE(opt_hdr))
        {
        /* @todo: process IPV6 Destination option data */
        case IP6_PAD1_OPTION:
          /* PAD1 option deosn't have length and value field */
          opt_dlen = -1;
          break;
        case IP6_PADN_OPTION:
          opt_dlen = IP6_OPT_DLEN(opt_hdr);
          break;
        case IP6_ROUTER_ALERT_OPTION:
          opt_dlen = IP6_OPT_DLEN(opt_hdr);
          break;
        case IP6_JUMBO_OPTION:
          opt_dlen = IP6_OPT_DLEN(opt_hdr);
          break;
        case IP6_HOME_ADDRESS_OPTION:
          opt_dlen = IP6_OPT_DLEN(opt_hdr);
          break;
        default:
          /* Check 2 MSB of Destination header type. */
          switch (IP6_OPT_TYPE_ACTION(opt_hdr))
          {
          case 1:
            /* Discard the packet. */
            LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with invalid destination option type dropped.\n"));
            pbuf_free(p);
            IP6_STATS_INC(ip6.drop);
            goto ip6_input_cleanup;
          case 2:
            /* Send ICMP Parameter Problem */
            icmp6_param_problem(p, ICMP6_PP_OPTION, opt_hdr);
            LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with invalid destination option type dropped.\n"));
            pbuf_free(p);
            IP6_STATS_INC(ip6.drop);
            goto ip6_input_cleanup;
          case 3:
            /* Send ICMP Parameter Problem if destination address is not a multicast address */
            if (!ip6_addr_ismulticast(ip6_current_dest_addr())) {
              icmp6_param_problem(p, ICMP6_PP_OPTION, opt_hdr);
            }
            LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with invalid destination option type dropped.\n"));
            pbuf_free(p);
            IP6_STATS_INC(ip6.drop);
            goto ip6_input_cleanup;
          default:
            /* Skip over this option. */
            opt_dlen = IP6_OPT_DLEN(opt_hdr);
            break;
          }
          break;
        }

        /* Adjust the offset to move to the next extended option header */
        opt_offset = opt_offset + IP6_OPT_HLEN + opt_dlen;
      }

      pbuf_remove_header(p, hlen);
      break;
    }
    case IP6_NEXTH_ROUTING:
    {
      struct ip6_rout_hdr *rout_hdr;
      LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with Routing header\n"));

      rout_hdr = (struct ip6_rout_hdr *)p->payload;

      /* Get next header type. */
      nexth = &IP6_ROUT_NEXTH(rout_hdr);

      /* Get the header length. */
      hlen = 8 * (1 + rout_hdr->_hlen);

      if ((p->len < 8) || (hlen > p->len)) {
        LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
          ("IPv6 options header (hlen %"U16_F") does not fit in first pbuf (len %"U16_F"), IPv6 packet dropped.\n",
              hlen, p->len));
        /* free (drop) packet pbufs */
        pbuf_free(p);
        IP6_STATS_INC(ip6.lenerr);
        IP6_STATS_INC(ip6.drop);
        goto ip6_input_cleanup;
      }

      /* Skip over this header. */
      hlen_tot = (u16_t)(hlen_tot + hlen);

      /* if segment left value is 0 in routing header, ignore the option */
      if (IP6_ROUT_SEG_LEFT(rout_hdr)) {
        /* The length field of routing option header must be even */
        if (rout_hdr->_hlen & 0x1) {
          /* Discard and send parameter field error */
          icmp6_param_problem(p, ICMP6_PP_FIELD, &rout_hdr->_hlen);
          LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with invalid routing type dropped\n"));
          pbuf_free(p);
          IP6_STATS_INC(ip6.drop);
          goto ip6_input_cleanup;
        }

        switch (IP6_ROUT_TYPE(rout_hdr))
        {
        /* TODO: process routing by the type */
        case IP6_ROUT_TYPE2:
          break;
        case IP6_ROUT_RPL:
          break;
        default:
          /* Discard unrecognized routing type and send parameter field error */
          icmp6_param_problem(p, ICMP6_PP_FIELD, &IP6_ROUT_TYPE(rout_hdr));
          LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with invalid routing type dropped\n"));
          pbuf_free(p);
          IP6_STATS_INC(ip6.drop);
          goto ip6_input_cleanup;
        }
      }

      pbuf_remove_header(p, hlen);
      break;
    }
    case IP6_NEXTH_FRAGMENT:
    {
      struct ip6_frag_hdr *frag_hdr;
      LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with Fragment header\n"));

      frag_hdr = (struct ip6_frag_hdr *)p->payload;

      /* Get next header type. */
      nexth = &IP6_FRAG_NEXTH(frag_hdr);

      /* Fragment Header length. */
      hlen = 8;

      /* Make sure this header fits in current pbuf. */
      if (hlen > p->len) {
        LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
          ("IPv6 options header (hlen %"U16_F") does not fit in first pbuf (len %"U16_F"), IPv6 packet dropped.\n",
              hlen, p->len));
        /* free (drop) packet pbufs */
        pbuf_free(p);
        IP6_FRAG_STATS_INC(ip6_frag.lenerr);
        IP6_FRAG_STATS_INC(ip6_frag.drop);
        goto ip6_input_cleanup;
      }

      hlen_tot = (u16_t)(hlen_tot + hlen);

      /* check payload length is multiple of 8 octets when mbit is set */
      if (IP6_FRAG_MBIT(frag_hdr) && (IP6H_PLEN(ip6hdr) & 0x7)) {
        /* ipv6 payload length is not multiple of 8 octets */
        icmp6_param_problem(p, ICMP6_PP_FIELD, &ip6hdr->_plen);
        LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with invalid payload length dropped\n"));
        pbuf_free(p);
        IP6_STATS_INC(ip6.drop);
        goto ip6_input_cleanup;
      }

      /* Offset == 0 and more_fragments == 0? */
      if ((frag_hdr->_fragment_offset &
           PP_HTONS(IP6_FRAG_OFFSET_MASK | IP6_FRAG_MORE_FLAG)) == 0) {
        /* This is a 1-fragment packet. Skip this header and continue. */
        pbuf_remove_header(p, hlen);
      } else {
#if LWIP_IPV6_REASS
        /* reassemble the packet */
        ip_data.current_ip_header_tot_len = hlen_tot;
        p = ip6_reass(p);
        /* packet not fully reassembled yet? */
        if (p == NULL) {
          goto ip6_input_cleanup;
        }

        /* Returned p point to IPv6 header.
         * Update all our variables and pointers and continue. */
        ip6hdr = (struct ip6_hdr *)p->payload;
        nexth = &IP6H_NEXTH(ip6hdr);
        hlen = hlen_tot = IP6_HLEN;
        pbuf_remove_header(p, IP6_HLEN);

#else /* LWIP_IPV6_REASS */
        /* free (drop) packet pbufs */
        LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with Fragment header dropped (with LWIP_IPV6_REASS==0)\n"));
        pbuf_free(p);
        IP6_STATS_INC(ip6.opterr);
        IP6_STATS_INC(ip6.drop);
        goto ip6_input_cleanup;
#endif /* LWIP_IPV6_REASS */
      }
      break;
    }
    default:
      goto options_done;
    }

    if (*nexth == IP6_NEXTH_HOPBYHOP) {
      /* Hop-by-Hop header comes only as a first option */
      icmp6_param_problem(p, ICMP6_PP_HEADER, nexth);
      LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: packet with Hop-by-Hop options header dropped (only valid as a first option)\n"));
      pbuf_free(p);
      IP6_STATS_INC(ip6.drop);
      goto ip6_input_cleanup;
    }
  }

options_done:
  if (hlen_tot >= 0x8000) {
    /* s16_t overflow */
    LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("ip6_input: header length overflow: %"U16_F"\n", hlen_tot));
    pbuf_free(p);
    IP6_STATS_INC(ip6.proterr);
    IP6_STATS_INC(ip6.drop);
    goto options_done;
  }

  /* send to upper layers */
  LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: \n"));
  ip6_debug_print(p);
  LWIP_DEBUGF(IP6_DEBUG, ("ip6_input: p->len %"U16_F" p->tot_len %"U16_F"\n", p->len, p->tot_len));

  ip_data.current_ip_header_tot_len = hlen_tot;
  
#if LWIP_RAW
  /* p points to IPv6 header again for raw_input. */
  pbuf_header_force(p, (s16_t)hlen_tot);
  /* raw input did not eat the packet? */
  if (raw_input(p, inp) == 0)
  {
    /* Point to payload. */
    pbuf_remove_header(p, hlen_tot);
#else /* LWIP_RAW */
  {
#endif /* LWIP_RAW */
    switch (*nexth) {
    case IP6_NEXTH_NONE:
      pbuf_free(p);
      break;
#if LWIP_UDP
    case IP6_NEXTH_UDP:
#if LWIP_UDPLITE
    case IP6_NEXTH_UDPLITE:
#endif /* LWIP_UDPLITE */
      udp_input(p, inp);
      break;
#endif /* LWIP_UDP */
#if LWIP_TCP
    case IP6_NEXTH_TCP:
      tcp_input(p, inp);
      break;
#endif /* LWIP_TCP */
#if LWIP_ICMP6
    case IP6_NEXTH_ICMP6:
      icmp6_input(p, inp);
      break;
#endif /* LWIP_ICMP */
    default:
#if LWIP_ICMP6
      /* p points to IPv6 header again for raw_input. */
      pbuf_header_force(p, (s16_t)hlen_tot);
      /* send ICMP parameter problem unless it was a multicast or ICMPv6 */
      if ((!ip6_addr_ismulticast(ip6_current_dest_addr())) &&
          (IP6H_NEXTH(ip6hdr) != IP6_NEXTH_ICMP6)) {
        icmp6_param_problem(p, ICMP6_PP_HEADER, nexth);
      }
#endif /* LWIP_ICMP */
      LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("ip6_input: Unsupported transport protocol %"U16_F"\n", (u16_t)IP6H_NEXTH(ip6hdr)));
      pbuf_free(p);
      IP6_STATS_INC(ip6.proterr);
      IP6_STATS_INC(ip6.drop);
      break;
    }
  }

ip6_input_cleanup:
  ip_data.current_netif = NULL;
  ip_data.current_input_netif = NULL;
  ip_data.current_ip6_header = NULL;
  ip_data.current_ip_header_tot_len = 0;
  ip6_addr_set_zero(ip6_current_src_addr());
  ip6_addr_set_zero(ip6_current_dest_addr());

  return ERR_OK;
}


/**
 * Sends an IPv6 packet on a network interface. This function constructs
 * the IPv6 header. If the source IPv6 address is NULL, the IPv6 "ANY" address is
 * used as source (usually during network startup). If the source IPv6 address it
 * IP6_ADDR_ANY, the most appropriate IPv6 address of the outgoing network
 * interface is filled in as source address. If the destination IPv6 address is
 * LWIP_IP_HDRINCL, p is assumed to already include an IPv6 header and
 * p->payload points to it instead of the data.
 *
 * @param p the packet to send (p->payload points to the data, e.g. next
            protocol header; if dest == LWIP_IP_HDRINCL, p already includes an
            IPv6 header and p->payload points to that IPv6 header)
 * @param src the source IPv6 address to send from (if src == IP6_ADDR_ANY, an
 *         IP address of the netif is selected and used as source address.
 *         if src == NULL, IP6_ADDR_ANY is used as source) (src is possibly not
 *         properly zoned)
 * @param dest the destination IPv6 address to send the packet to (possibly not
 *             properly zoned)
 * @param hl the Hop Limit value to be set in the IPv6 header
 * @param tc the Traffic Class value to be set in the IPv6 header
 * @param nexth the Next Header to be set in the IPv6 header
 * @param netif the netif on which to send this packet
 * @return ERR_OK if the packet was sent OK
 *         ERR_BUF if p doesn't have enough space for IPv6/LINK headers
 *         returns errors returned by netif->output
 */
err_t
ip6_output_if(struct pbuf *p, const ip6_addr_t *src, const ip6_addr_t *dest,
             u8_t hl, u8_t tc,
             u8_t nexth, struct netif *netif)
{
  const ip6_addr_t *src_used = src;
  if (dest != LWIP_IP_HDRINCL) {
    if (src != NULL && ip6_addr_isany(src)) {
      src_used = ip_2_ip6(ip6_select_source_address(netif, dest));
      if ((src_used == NULL) || ip6_addr_isany(src_used)) {
        /* No appropriate source address was found for this packet. */
        LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("ip6_output: No suitable source address for packet.\n"));
        IP6_STATS_INC(ip6.rterr);
        return ERR_RTE;
      }
    }
  }
  return ip6_output_if_src(p, src_used, dest, hl, tc, nexth, netif);
}

/**
 * Same as ip6_output_if() but 'src' address is not replaced by netif address
 * when it is 'any'.
 */
err_t
ip6_output_if_src(struct pbuf *p, const ip6_addr_t *src, const ip6_addr_t *dest,
             u8_t hl, u8_t tc,
             u8_t nexth, struct netif *netif)
{
  struct ip6_hdr *ip6hdr;
  ip6_addr_t dest_addr;

  LWIP_IP_CHECK_PBUF_REF_COUNT_FOR_TX(p);

  /* Should the IPv6 header be generated or is it already included in p? */
  if (dest != LWIP_IP_HDRINCL) {
#if LWIP_IPV6_SCOPES
    /* If the destination address is scoped but lacks a zone, add a zone now,
     * based on the outgoing interface. The lower layers (e.g., nd6) absolutely
     * require addresses to be properly zoned for correctness. In some cases,
     * earlier attempts will have been made to add a zone to the destination,
     * but this function is the only one that is called in all (other) cases,
     * so we must do this here. */
    if (ip6_addr_lacks_zone(dest, IP6_UNKNOWN)) {
      ip6_addr_copy(dest_addr, *dest);
      ip6_addr_assign_zone(&dest_addr, IP6_UNKNOWN, netif);
      dest = &dest_addr;
    }
#endif /* LWIP_IPV6_SCOPES */

    /* generate IPv6 header */
    if (pbuf_add_header(p, IP6_HLEN)) {
      LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("ip6_output: not enough room for IPv6 header in pbuf\n"));
      IP6_STATS_INC(ip6.err);
      return ERR_BUF;
    }

    ip6hdr = (struct ip6_hdr *)p->payload;
    LWIP_ASSERT("check that first pbuf can hold struct ip6_hdr",
               (p->len >= sizeof(struct ip6_hdr)));

    IP6H_HOPLIM_SET(ip6hdr, hl);
    IP6H_NEXTH_SET(ip6hdr, nexth);

    /* dest cannot be NULL here */
    ip6_addr_copy_to_packed(ip6hdr->dest, *dest);

    IP6H_VTCFL_SET(ip6hdr, 6, tc, 0);
    IP6H_PLEN_SET(ip6hdr, (u16_t)(p->tot_len - IP6_HLEN));

    if (src == NULL) {
      src = IP6_ADDR_ANY6;
    }
    /* src cannot be NULL here */
    ip6_addr_copy_to_packed(ip6hdr->src, *src);

  } else {
    /* IP header already included in p */
    ip6hdr = (struct ip6_hdr *)p->payload;
    ip6_addr_copy_from_packed(dest_addr, ip6hdr->dest);
    ip6_addr_assign_zone(&dest_addr, IP6_UNKNOWN, netif);
    dest = &dest_addr;
  }

  IP6_STATS_INC(ip6.xmit);

  LWIP_DEBUGF(IP6_DEBUG, ("ip6_output_if: %c%c%"U16_F"\n", netif->name[0], netif->name[1], (u16_t)netif->num));
  ip6_debug_print(p);

#if ENABLE_LOOPBACK
  {
    int i;
#if !LWIP_HAVE_LOOPIF
    if (ip6_addr_isloopback(dest)) {
      return netif_loop_output(netif, p);
    }
#endif /* !LWIP_HAVE_LOOPIF */
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
          ip6_addr_cmp(dest, netif_ip6_addr(netif, i))) {
        /* Packet to self, enqueue it for loopback */
        LWIP_DEBUGF(IP6_DEBUG, ("netif_loop_output()\n"));
        return netif_loop_output(netif, p);
      }
    }
  }
#if LWIP_MULTICAST_TX_OPTIONS
  if ((p->flags & PBUF_FLAG_MCASTLOOP) != 0) {
    netif_loop_output(netif, p);
  }
#endif /* LWIP_MULTICAST_TX_OPTIONS */
#endif /* ENABLE_LOOPBACK */
#if LWIP_IPV6_FRAG
  /* don't fragment if interface has mtu set to 0 [loopif] */
  if (netif->mtu && (p->tot_len > nd6_get_destination_mtu(dest, netif))) {
    return ip6_frag(p, netif, dest);
  }
#endif /* LWIP_IPV6_FRAG */

  LWIP_DEBUGF(IP6_DEBUG, ("netif->output_ip6()\n"));
  return netif->output_ip6(netif, p, dest);
}

/**
 * Simple interface to ip6_output_if. It finds the outgoing network
 * interface and calls upon ip6_output_if to do the actual work.
 *
 * @param p the packet to send (p->payload points to the data, e.g. next
            protocol header; if dest == LWIP_IP_HDRINCL, p already includes an
            IPv6 header and p->payload points to that IPv6 header)
 * @param src the source IPv6 address to send from (if src == IP6_ADDR_ANY, an
 *         IP address of the netif is selected and used as source address.
 *         if src == NULL, IP6_ADDR_ANY is used as source)
 * @param dest the destination IPv6 address to send the packet to
 * @param hl the Hop Limit value to be set in the IPv6 header
 * @param tc the Traffic Class value to be set in the IPv6 header
 * @param nexth the Next Header to be set in the IPv6 header
 *
 * @return ERR_RTE if no route is found
 *         see ip_output_if() for more return values
 */
err_t
ip6_output(struct pbuf *p, const ip6_addr_t *src, const ip6_addr_t *dest,
          u8_t hl, u8_t tc, u8_t nexth)
{
  struct netif *netif;
  struct ip6_hdr *ip6hdr;
  ip6_addr_t src_addr, dest_addr;

  LWIP_IP_CHECK_PBUF_REF_COUNT_FOR_TX(p);

  if (dest != LWIP_IP_HDRINCL) {
    netif = ip6_route(src, dest);
  } else {
    /* IP header included in p, read addresses. */
    ip6hdr = (struct ip6_hdr *)p->payload;
    ip6_addr_copy_from_packed(src_addr, ip6hdr->src);
    ip6_addr_copy_from_packed(dest_addr, ip6hdr->dest);
    netif = ip6_route(&src_addr, &dest_addr);
  }

  if (netif == NULL) {
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_output: no route for %"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F"\n",
        IP6_ADDR_BLOCK1(dest),
        IP6_ADDR_BLOCK2(dest),
        IP6_ADDR_BLOCK3(dest),
        IP6_ADDR_BLOCK4(dest),
        IP6_ADDR_BLOCK5(dest),
        IP6_ADDR_BLOCK6(dest),
        IP6_ADDR_BLOCK7(dest),
        IP6_ADDR_BLOCK8(dest)));
    IP6_STATS_INC(ip6.rterr);
    return ERR_RTE;
  }

  return ip6_output_if(p, src, dest, hl, tc, nexth, netif);
}


#if LWIP_NETIF_USE_HINTS
/** Like ip6_output, but takes and addr_hint pointer that is passed on to netif->addr_hint
 *  before calling ip6_output_if.
 *
 * @param p the packet to send (p->payload points to the data, e.g. next
            protocol header; if dest == LWIP_IP_HDRINCL, p already includes an
            IPv6 header and p->payload points to that IPv6 header)
 * @param src the source IPv6 address to send from (if src == IP6_ADDR_ANY, an
 *         IP address of the netif is selected and used as source address.
 *         if src == NULL, IP6_ADDR_ANY is used as source)
 * @param dest the destination IPv6 address to send the packet to
 * @param hl the Hop Limit value to be set in the IPv6 header
 * @param tc the Traffic Class value to be set in the IPv6 header
 * @param nexth the Next Header to be set in the IPv6 header
 * @param netif_hint netif output hint pointer set to netif->hint before
 *        calling ip_output_if()
 *
 * @return ERR_RTE if no route is found
 *         see ip_output_if() for more return values
 */
err_t
ip6_output_hinted(struct pbuf *p, const ip6_addr_t *src, const ip6_addr_t *dest,
          u8_t hl, u8_t tc, u8_t nexth, struct netif_hint *netif_hint)
{
  struct netif *netif;
  struct ip6_hdr *ip6hdr;
  ip6_addr_t src_addr, dest_addr;
  err_t err;

  LWIP_IP_CHECK_PBUF_REF_COUNT_FOR_TX(p);

  if (dest != LWIP_IP_HDRINCL) {
    netif = ip6_route(src, dest);
  } else {
    /* IP header included in p, read addresses. */
    ip6hdr = (struct ip6_hdr *)p->payload;
    ip6_addr_copy_from_packed(src_addr, ip6hdr->src);
    ip6_addr_copy_from_packed(dest_addr, ip6hdr->dest);
    netif = ip6_route(&src_addr, &dest_addr);
  }

  if (netif == NULL) {
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_output: no route for %"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F":%"X16_F"\n",
        IP6_ADDR_BLOCK1(dest),
        IP6_ADDR_BLOCK2(dest),
        IP6_ADDR_BLOCK3(dest),
        IP6_ADDR_BLOCK4(dest),
        IP6_ADDR_BLOCK5(dest),
        IP6_ADDR_BLOCK6(dest),
        IP6_ADDR_BLOCK7(dest),
        IP6_ADDR_BLOCK8(dest)));
    IP6_STATS_INC(ip6.rterr);
    return ERR_RTE;
  }

  NETIF_SET_HINTS(netif, netif_hint);
  err = ip6_output_if(p, src, dest, hl, tc, nexth, netif);
  NETIF_RESET_HINTS(netif);

  return err;
}
#endif /* LWIP_NETIF_USE_HINTS*/

#if LWIP_IPV6_MLD
/**
 * Add a hop-by-hop options header with a router alert option and padding.
 *
 * Used by MLD when sending a Multicast listener report/done message.
 *
 * @param p the packet to which we will prepend the options header
 * @param nexth the next header protocol number (e.g. IP6_NEXTH_ICMP6)
 * @param value the value of the router alert option data (e.g. IP6_ROUTER_ALERT_VALUE_MLD)
 * @return ERR_OK if hop-by-hop header was added, ERR_* otherwise
 */
err_t
ip6_options_add_hbh_ra(struct pbuf *p, u8_t nexth, u8_t value)
{
  u8_t *opt_data;
  u32_t offset = 0;
  struct ip6_hbh_hdr *hbh_hdr;
  struct ip6_opt_hdr *opt_hdr;

  /* fixed 4 bytes for router alert option and 2 bytes padding */
  const u8_t hlen = (sizeof(struct ip6_opt_hdr) * 2) + IP6_ROUTER_ALERT_DLEN;
  /* Move pointer to make room for hop-by-hop options header. */
  if (pbuf_add_header(p, sizeof(struct ip6_hbh_hdr) + hlen)) {
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_options: no space for options header\n"));
    IP6_STATS_INC(ip6.err);
    return ERR_BUF;
  }

  /* Set fields of Hop-by-Hop header */
  hbh_hdr = (struct ip6_hbh_hdr *)p->payload;
  IP6_HBH_NEXTH(hbh_hdr) = nexth;
  hbh_hdr->_hlen = 0;
  offset = IP6_HBH_HLEN;

  /* Set router alert options to Hop-by-Hop extended option header */
  opt_hdr = (struct ip6_opt_hdr *)((u8_t *)hbh_hdr + offset);
  IP6_OPT_TYPE(opt_hdr) = IP6_ROUTER_ALERT_OPTION;
  IP6_OPT_DLEN(opt_hdr) = IP6_ROUTER_ALERT_DLEN;
  offset += IP6_OPT_HLEN;

  /* Set router alert option data */
  opt_data = (u8_t *)hbh_hdr + offset;
  opt_data[0] = value;
  opt_data[1] = 0;
  offset += IP6_OPT_DLEN(opt_hdr);

  /* add 2 bytes padding to make 8 bytes Hop-by-Hop header length */
  opt_hdr = (struct ip6_opt_hdr *)((u8_t *)hbh_hdr + offset);
  IP6_OPT_TYPE(opt_hdr) = IP6_PADN_OPTION;
  IP6_OPT_DLEN(opt_hdr) = 0;

  return ERR_OK;
}
#endif /* LWIP_IPV6_MLD */

#if IP6_DEBUG
/* Print an IPv6 header by using LWIP_DEBUGF
 * @param p an IPv6 packet, p->payload pointing to the IPv6 header
 */
void
ip6_debug_print(struct pbuf *p)
{
  struct ip6_hdr *ip6hdr = (struct ip6_hdr *)p->payload;

  LWIP_DEBUGF(IP6_DEBUG, ("IPv6 header:\n"));
  LWIP_DEBUGF(IP6_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP6_DEBUG, ("| %2"U16_F" |  %3"U16_F"  |      %7"U32_F"     | (ver, class, flow)\n",
                    IP6H_V(ip6hdr),
                    IP6H_TC(ip6hdr),
                    IP6H_FL(ip6hdr)));
  LWIP_DEBUGF(IP6_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP6_DEBUG, ("|     %5"U16_F"     |  %3"U16_F"  |  %3"U16_F"  | (plen, nexth, hopl)\n",
                    IP6H_PLEN(ip6hdr),
                    IP6H_NEXTH(ip6hdr),
                    IP6H_HOPLIM(ip6hdr)));
  LWIP_DEBUGF(IP6_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP6_DEBUG, ("|  %4"X32_F" |  %4"X32_F" |  %4"X32_F" |  %4"X32_F" | (src)\n",
                    IP6_ADDR_BLOCK1(&(ip6hdr->src)),
                    IP6_ADDR_BLOCK2(&(ip6hdr->src)),
                    IP6_ADDR_BLOCK3(&(ip6hdr->src)),
                    IP6_ADDR_BLOCK4(&(ip6hdr->src))));
  LWIP_DEBUGF(IP6_DEBUG, ("|  %4"X32_F" |  %4"X32_F" |  %4"X32_F" |  %4"X32_F" |\n",
                    IP6_ADDR_BLOCK5(&(ip6hdr->src)),
                    IP6_ADDR_BLOCK6(&(ip6hdr->src)),
                    IP6_ADDR_BLOCK7(&(ip6hdr->src)),
                    IP6_ADDR_BLOCK8(&(ip6hdr->src))));
  LWIP_DEBUGF(IP6_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP6_DEBUG, ("|  %4"X32_F" |  %4"X32_F" |  %4"X32_F" |  %4"X32_F" | (dest)\n",
                    IP6_ADDR_BLOCK1(&(ip6hdr->dest)),
                    IP6_ADDR_BLOCK2(&(ip6hdr->dest)),
                    IP6_ADDR_BLOCK3(&(ip6hdr->dest)),
                    IP6_ADDR_BLOCK4(&(ip6hdr->dest))));
  LWIP_DEBUGF(IP6_DEBUG, ("|  %4"X32_F" |  %4"X32_F" |  %4"X32_F" |  %4"X32_F" |\n",
                    IP6_ADDR_BLOCK5(&(ip6hdr->dest)),
                    IP6_ADDR_BLOCK6(&(ip6hdr->dest)),
                    IP6_ADDR_BLOCK7(&(ip6hdr->dest)),
                    IP6_ADDR_BLOCK8(&(ip6hdr->dest))));
  LWIP_DEBUGF(IP6_DEBUG, ("+-------------------------------+\n"));
}
#endif /* IP6_DEBUG */

#endif /* LWIP_IPV6 */
