/**
 * @file
 *
 * Neighbor discovery and stateless address autoconfiguration for IPv6.
 * Aims to be compliant with RFC 4861 (Neighbor discovery) and RFC 4862
 * (Address autoconfiguration).
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

#include "lwip/nd6.h"
#include "lwip/priv/nd6_priv.h"
#include "lwip/prot/nd6.h"
#include "lwip/prot/icmp6.h"
#include "lwip/pbuf.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/inet_chksum.h"
#include "lwip/netif.h"
#include "lwip/icmp6.h"
#include "lwip/mld6.h"
#include "lwip/ip.h"
#include "lwip/stats.h"
#include "lwip/dns.h"

#include <string.h>

#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif

#if LWIP_IPV6_DUP_DETECT_ATTEMPTS > IP6_ADDR_TENTATIVE_COUNT_MASK
#error LWIP_IPV6_DUP_DETECT_ATTEMPTS > IP6_ADDR_TENTATIVE_COUNT_MASK
#endif

/* Router tables. */
struct nd6_neighbor_cache_entry neighbor_cache[LWIP_ND6_NUM_NEIGHBORS];
struct nd6_destination_cache_entry destination_cache[LWIP_ND6_NUM_DESTINATIONS];
struct nd6_prefix_list_entry prefix_list[LWIP_ND6_NUM_PREFIXES];
struct nd6_router_list_entry default_router_list[LWIP_ND6_NUM_ROUTERS];

/* Default values, can be updated by a RA message. */
u32_t reachable_time = LWIP_ND6_REACHABLE_TIME;
u32_t retrans_timer = LWIP_ND6_RETRANS_TIMER; /* @todo implement this value in timer */

/* Index for cache entries. */
static u8_t nd6_cached_neighbor_index;
static u8_t nd6_cached_destination_index;

/* Multicast address holder. */
static ip6_addr_t multicast_address;

/* Static buffer to parse RA packet options */
union ra_options {
  struct lladdr_option  lladdr;
  struct mtu_option     mtu;
  struct prefix_option  prefix;
#if LWIP_ND6_RDNSS_MAX_DNS_SERVERS
  struct rdnss_option   rdnss;
#endif
};
static union ra_options nd6_ra_buffer;

/* Forward declarations. */
static s8_t nd6_find_neighbor_cache_entry(const ip6_addr_t *ip6addr);
static s8_t nd6_new_neighbor_cache_entry(void);
static void nd6_free_neighbor_cache_entry(s8_t i);
static s8_t nd6_find_destination_cache_entry(const ip6_addr_t *ip6addr);
static s8_t nd6_new_destination_cache_entry(void);
static s8_t nd6_is_prefix_in_netif(const ip6_addr_t *ip6addr, struct netif *netif);
static s8_t nd6_select_router(const ip6_addr_t *ip6addr, struct netif *netif);
static s8_t nd6_get_router(const ip6_addr_t *router_addr, struct netif *netif);
static s8_t nd6_new_router(const ip6_addr_t *router_addr, struct netif *netif);
static s8_t nd6_get_onlink_prefix(const ip6_addr_t *prefix, struct netif *netif);
static s8_t nd6_new_onlink_prefix(const ip6_addr_t *prefix, struct netif *netif);
static s8_t nd6_get_next_hop_entry(const ip6_addr_t *ip6addr, struct netif *netif);
static err_t nd6_queue_packet(s8_t neighbor_index, struct pbuf *q);

#define ND6_SEND_FLAG_MULTICAST_DEST 0x01
#define ND6_SEND_FLAG_ALLNODES_DEST 0x02
#define ND6_SEND_FLAG_ANY_SRC 0x04
static void nd6_send_ns(struct netif *netif, const ip6_addr_t *target_addr, u8_t flags);
static void nd6_send_na(struct netif *netif, const ip6_addr_t *target_addr, u8_t flags);
static void nd6_send_neighbor_cache_probe(struct nd6_neighbor_cache_entry *entry, u8_t flags);
#if LWIP_IPV6_SEND_ROUTER_SOLICIT
static err_t nd6_send_rs(struct netif *netif);
#endif /* LWIP_IPV6_SEND_ROUTER_SOLICIT */

#if LWIP_ND6_QUEUEING
static void nd6_free_q(struct nd6_q_entry *q);
#else /* LWIP_ND6_QUEUEING */
#define nd6_free_q(q) pbuf_free(q)
#endif /* LWIP_ND6_QUEUEING */
static void nd6_send_q(s8_t i);


/**
 * A local address has been determined to be a duplicate. Take the appropriate
 * action(s) on the address and the interface as a whole.
 *
 * @param netif the netif that owns the address
 * @param addr_idx the index of the address detected to be a duplicate
 */
static void
nd6_duplicate_addr_detected(struct netif *netif, s8_t addr_idx)
{

  /* Mark the address as duplicate, but leave its lifetimes alone. If this was
   * a manually assigned address, it will remain in existence as duplicate, and
   * as such be unusable for any practical purposes until manual intervention.
   * If this was an autogenerated address, the address will follow normal
   * expiration rules, and thus disappear once its valid lifetime expires. */
  netif_ip6_addr_set_state(netif, addr_idx, IP6_ADDR_DUPLICATED);

#if LWIP_IPV6_AUTOCONFIG
  /* If the affected address was the link-local address that we use to generate
   * all other addresses, then we should not continue to use those derived
   * addresses either, so mark them as duplicate as well. For autoconfig-only
   * setups, this will make the interface effectively unusable, approaching the
   * intention of RFC 4862 Sec. 5.4.5. @todo implement the full requirements */
  if (addr_idx == 0) {
    s8_t i;
    for (i = 1; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (!ip6_addr_isinvalid(netif_ip6_addr_state(netif, i)) &&
          !netif_ip6_addr_isstatic(netif, i)) {
        netif_ip6_addr_set_state(netif, i, IP6_ADDR_DUPLICATED);
      }
    }
  }
#endif /* LWIP_IPV6_AUTOCONFIG */
}

#if LWIP_IPV6_AUTOCONFIG
/**
 * We received a router advertisement that contains a prefix with the
 * autoconfiguration flag set. Add or update an associated autogenerated
 * address.
 *
 * @param netif the netif on which the router advertisement arrived
 * @param prefix_opt a pointer to the prefix option data
 * @param prefix_addr an aligned copy of the prefix address
 */
static void
nd6_process_autoconfig_prefix(struct netif *netif,
  struct prefix_option *prefix_opt, const ip6_addr_t *prefix_addr)
{
  ip6_addr_t ip6addr;
  u32_t valid_life, pref_life;
  u8_t addr_state;
  s8_t i, free_idx;

  /* The caller already checks RFC 4862 Sec. 5.5.3 points (a) and (b). We do
   * the rest, starting with checks for (c) and (d) here. */
  valid_life = lwip_htonl(prefix_opt->valid_lifetime);
  pref_life = lwip_htonl(prefix_opt->preferred_lifetime);
  if (pref_life > valid_life || prefix_opt->prefix_length != 64) {
    return; /* silently ignore this prefix for autoconfiguration purposes */
  }

  /* If an autogenerated address already exists for this prefix, update its
   * lifetimes. An address is considered autogenerated if 1) it is not static
   * (i.e., manually assigned), and 2) there is an advertised autoconfiguration
   * prefix for it (the one we are processing here). This does not necessarily
   * exclude the possibility that the address was actually assigned by, say,
   * DHCPv6. If that distinction becomes important in the future, more state
   * must be kept. As explained elsewhere we also update lifetimes of tentative
   * and duplicate addresses. Skip address slot 0 (the link-local address). */
  for (i = 1; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
    addr_state = netif_ip6_addr_state(netif, i);
    if (!ip6_addr_isinvalid(addr_state) && !netif_ip6_addr_isstatic(netif, i) &&
        ip6_addr_netcmp(prefix_addr, netif_ip6_addr(netif, i))) {
      /* Update the valid lifetime, as per RFC 4862 Sec. 5.5.3 point (e).
       * The valid lifetime will never drop to zero as a result of this. */
      u32_t remaining_life = netif_ip6_addr_valid_life(netif, i);
      if (valid_life > ND6_2HRS || valid_life > remaining_life) {
        netif_ip6_addr_set_valid_life(netif, i, valid_life);
      } else if (remaining_life > ND6_2HRS) {
        netif_ip6_addr_set_valid_life(netif, i, ND6_2HRS);
      }
      LWIP_ASSERT("bad valid lifetime", !netif_ip6_addr_isstatic(netif, i));
      /* Update the preferred lifetime. No bounds checks are needed here. In
       * rare cases the advertisement may un-deprecate the address, though.
       * Deprecation is left to the timer code where it is handled anyway. */
      if (pref_life > 0 && addr_state == IP6_ADDR_DEPRECATED) {
        netif_ip6_addr_set_state(netif, i, IP6_ADDR_PREFERRED);
      }
      netif_ip6_addr_set_pref_life(netif, i, pref_life);
      return; /* there should be at most one matching address */
    }
  }

  /* No autogenerated address exists for this prefix yet. See if we can add a
   * new one. However, if IPv6 autoconfiguration is administratively disabled,
   * do not generate new addresses, but do keep updating lifetimes for existing
   * addresses. Also, when adding new addresses, we must protect explicitly
   * against a valid lifetime of zero, because again, we use that as a special
   * value. The generated address would otherwise expire immediately anyway.
   * Finally, the original link-local address must be usable at all. We start
   * creating addresses even if the link-local address is still in tentative
   * state though, and deal with the fallout of that upon DAD collision. */
  addr_state = netif_ip6_addr_state(netif, 0);
  if (!netif->ip6_autoconfig_enabled || valid_life == IP6_ADDR_LIFE_STATIC ||
      ip6_addr_isinvalid(addr_state) || ip6_addr_isduplicated(addr_state)) {
    return;
  }

  /* Construct the new address that we intend to use, and then see if that
   * address really does not exist. It might have been added manually, after
   * all. As a side effect, find a free slot. Note that we cannot use
   * netif_add_ip6_address() here, as it would return ERR_OK if the address
   * already did exist, resulting in that address being given lifetimes. */
  IP6_ADDR(&ip6addr, prefix_addr->addr[0], prefix_addr->addr[1],
    netif_ip6_addr(netif, 0)->addr[2], netif_ip6_addr(netif, 0)->addr[3]);
  ip6_addr_assign_zone(&ip6addr, IP6_UNICAST, netif);

  free_idx = 0;
  for (i = 1; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
    if (!ip6_addr_isinvalid(netif_ip6_addr_state(netif, i))) {
      if (ip6_addr_cmp(&ip6addr, netif_ip6_addr(netif, i))) {
        return; /* formed address already exists */
      }
    } else if (free_idx == 0) {
      free_idx = i;
    }
  }
  if (free_idx == 0) {
    return; /* no address slots available, try again on next advertisement */
  }

  /* Assign the new address to the interface. */
  ip_addr_copy_from_ip6(netif->ip6_addr[free_idx], ip6addr);
  netif_ip6_addr_set_valid_life(netif, free_idx, valid_life);
  netif_ip6_addr_set_pref_life(netif, free_idx, pref_life);
  netif_ip6_addr_set_state(netif, free_idx, IP6_ADDR_TENTATIVE);
}
#endif /* LWIP_IPV6_AUTOCONFIG */

/**
 * Process an incoming neighbor discovery message
 *
 * @param p the nd packet, p->payload pointing to the icmpv6 header
 * @param inp the netif on which this packet was received
 */
void
nd6_input(struct pbuf *p, struct netif *inp)
{
  u8_t msg_type;
  s8_t i;

  ND6_STATS_INC(nd6.recv);

  msg_type = *((u8_t *)p->payload);
  switch (msg_type) {
  case ICMP6_TYPE_NA: /* Neighbor Advertisement. */
  {
    struct na_header *na_hdr;
    struct lladdr_option *lladdr_opt;
    ip6_addr_t target_address;

    /* Check that na header fits in packet. */
    if (p->len < (sizeof(struct na_header))) {
      /* @todo debug message */
      pbuf_free(p);
      ND6_STATS_INC(nd6.lenerr);
      ND6_STATS_INC(nd6.drop);
      return;
    }

    na_hdr = (struct na_header *)p->payload;

    /* Create an aligned, zoned copy of the target address. */
    ip6_addr_copy_from_packed(target_address, na_hdr->target_address);
    ip6_addr_assign_zone(&target_address, IP6_UNICAST, inp);

    /* Check a subset of the other RFC 4861 Sec. 7.1.2 requirements. */
    if (IP6H_HOPLIM(ip6_current_header()) != ND6_HOPLIM || na_hdr->code != 0 ||
        ip6_addr_ismulticast(&target_address)) {
      pbuf_free(p);
      ND6_STATS_INC(nd6.proterr);
      ND6_STATS_INC(nd6.drop);
      return;
    }

    /* @todo RFC MUST: if IP destination is multicast, Solicited flag is zero */
    /* @todo RFC MUST: all included options have a length greater than zero */

    /* Unsolicited NA?*/
    if (ip6_addr_ismulticast(ip6_current_dest_addr())) {
      /* This is an unsolicited NA.
       * link-layer changed?
       * part of DAD mechanism? */

#if LWIP_IPV6_DUP_DETECT_ATTEMPTS
      /* If the target address matches this netif, it is a DAD response. */
      for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (!ip6_addr_isinvalid(netif_ip6_addr_state(inp, i)) &&
            !ip6_addr_isduplicated(netif_ip6_addr_state(inp, i)) &&
            ip6_addr_cmp(&target_address, netif_ip6_addr(inp, i))) {
          /* We are using a duplicate address. */
          nd6_duplicate_addr_detected(inp, i);

          pbuf_free(p);
          return;
        }
      }
#endif /* LWIP_IPV6_DUP_DETECT_ATTEMPTS */

      /* Check that link-layer address option also fits in packet. */
      if (p->len < (sizeof(struct na_header) + 2)) {
        /* @todo debug message */
        pbuf_free(p);
        ND6_STATS_INC(nd6.lenerr);
        ND6_STATS_INC(nd6.drop);
        return;
      }

      lladdr_opt = (struct lladdr_option *)((u8_t*)p->payload + sizeof(struct na_header));

      if (p->len < (sizeof(struct na_header) + (lladdr_opt->length << 3))) {
        /* @todo debug message */
        pbuf_free(p);
        ND6_STATS_INC(nd6.lenerr);
        ND6_STATS_INC(nd6.drop);
        return;
      }

      /* This is an unsolicited NA, most likely there was a LLADDR change. */
      i = nd6_find_neighbor_cache_entry(&target_address);
      if (i >= 0) {
        if (na_hdr->flags & ND6_FLAG_OVERRIDE) {
          MEMCPY(neighbor_cache[i].lladdr, lladdr_opt->addr, inp->hwaddr_len);
        }
      }
    } else {
      /* This is a solicited NA.
       * neighbor address resolution response?
       * neighbor unreachability detection response? */

      /* Find the cache entry corresponding to this na. */
      i = nd6_find_neighbor_cache_entry(&target_address);
      if (i < 0) {
        /* We no longer care about this target address. drop it. */
        pbuf_free(p);
        return;
      }

      /* Update cache entry. */
      if ((na_hdr->flags & ND6_FLAG_OVERRIDE) ||
          (neighbor_cache[i].state == ND6_INCOMPLETE)) {
        /* Check that link-layer address option also fits in packet. */
        if (p->len < (sizeof(struct na_header) + 2)) {
          /* @todo debug message */
          pbuf_free(p);
          ND6_STATS_INC(nd6.lenerr);
          ND6_STATS_INC(nd6.drop);
          return;
        }

        lladdr_opt = (struct lladdr_option *)((u8_t*)p->payload + sizeof(struct na_header));

        if (p->len < (sizeof(struct na_header) + (lladdr_opt->length << 3))) {
          /* @todo debug message */
          pbuf_free(p);
          ND6_STATS_INC(nd6.lenerr);
          ND6_STATS_INC(nd6.drop);
          return;
        }

        MEMCPY(neighbor_cache[i].lladdr, lladdr_opt->addr, inp->hwaddr_len);
      }

      neighbor_cache[i].netif = inp;
      neighbor_cache[i].state = ND6_REACHABLE;
      neighbor_cache[i].counter.reachable_time = reachable_time;

      /* Send queued packets, if any. */
      if (neighbor_cache[i].q != NULL) {
        nd6_send_q(i);
      }
    }

    break; /* ICMP6_TYPE_NA */
  }
  case ICMP6_TYPE_NS: /* Neighbor solicitation. */
  {
    struct ns_header *ns_hdr;
    struct lladdr_option *lladdr_opt;
    ip6_addr_t target_address;
    u8_t accepted;

    /* Check that ns header fits in packet. */
    if (p->len < sizeof(struct ns_header)) {
      /* @todo debug message */
      pbuf_free(p);
      ND6_STATS_INC(nd6.lenerr);
      ND6_STATS_INC(nd6.drop);
      return;
    }

    ns_hdr = (struct ns_header *)p->payload;

    /* Create an aligned, zoned copy of the target address. */
    ip6_addr_copy_from_packed(target_address, ns_hdr->target_address);
    ip6_addr_assign_zone(&target_address, IP6_UNICAST, inp);

    /* Check a subset of the other RFC 4861 Sec. 7.1.1 requirements. */
    if (IP6H_HOPLIM(ip6_current_header()) != ND6_HOPLIM || ns_hdr->code != 0 ||
       ip6_addr_ismulticast(&target_address)) {
      pbuf_free(p);
      ND6_STATS_INC(nd6.proterr);
      ND6_STATS_INC(nd6.drop);
      return;
    }

    /* @todo RFC MUST: all included options have a length greater than zero */
    /* @todo RFC MUST: if IP source is 'any', destination is solicited-node multicast address */
    /* @todo RFC MUST: if IP source is 'any', there is no source LL address option */

    /* Check if there is a link-layer address provided. Only point to it if in this buffer. */
    if (p->len >= (sizeof(struct ns_header) + 2)) {
      lladdr_opt = (struct lladdr_option *)((u8_t*)p->payload + sizeof(struct ns_header));
      if (p->len < (sizeof(struct ns_header) + (lladdr_opt->length << 3))) {
        lladdr_opt = NULL;
      }
    } else {
      lladdr_opt = NULL;
    }

    /* Check if the target address is configured on the receiving netif. */
    accepted = 0;
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
      if ((ip6_addr_isvalid(netif_ip6_addr_state(inp, i)) ||
           (ip6_addr_istentative(netif_ip6_addr_state(inp, i)) &&
            ip6_addr_isany(ip6_current_src_addr()))) &&
          ip6_addr_cmp(&target_address, netif_ip6_addr(inp, i))) {
        accepted = 1;
        break;
      }
    }

    /* NS not for us? */
    if (!accepted) {
      pbuf_free(p);
      return;
    }

    /* Check for ANY address in src (DAD algorithm). */
    if (ip6_addr_isany(ip6_current_src_addr())) {
      /* Sender is validating this address. */
      for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
        if (!ip6_addr_isinvalid(netif_ip6_addr_state(inp, i)) &&
            ip6_addr_cmp(&target_address, netif_ip6_addr(inp, i))) {
          /* Send a NA back so that the sender does not use this address. */
          nd6_send_na(inp, netif_ip6_addr(inp, i), ND6_FLAG_OVERRIDE | ND6_SEND_FLAG_ALLNODES_DEST);
          if (ip6_addr_istentative(netif_ip6_addr_state(inp, i))) {
            /* We shouldn't use this address either. */
            nd6_duplicate_addr_detected(inp, i);
          }
        }
      }
    } else {
      /* Sender is trying to resolve our address. */
      /* Verify that they included their own link-layer address. */
      if (lladdr_opt == NULL) {
        /* Not a valid message. */
        pbuf_free(p);
        ND6_STATS_INC(nd6.proterr);
        ND6_STATS_INC(nd6.drop);
        return;
      }

      i = nd6_find_neighbor_cache_entry(ip6_current_src_addr());
      if (i>= 0) {
        /* We already have a record for the solicitor. */
        if (neighbor_cache[i].state == ND6_INCOMPLETE) {
          neighbor_cache[i].netif = inp;
          MEMCPY(neighbor_cache[i].lladdr, lladdr_opt->addr, inp->hwaddr_len);

          /* Delay probe in case we get confirmation of reachability from upper layer (TCP). */
          neighbor_cache[i].state = ND6_DELAY;
          neighbor_cache[i].counter.delay_time = LWIP_ND6_DELAY_FIRST_PROBE_TIME / ND6_TMR_INTERVAL;
        }
      } else {
        /* Add their IPv6 address and link-layer address to neighbor cache.
         * We will need it at least to send a unicast NA message, but most
         * likely we will also be communicating with this node soon. */
        i = nd6_new_neighbor_cache_entry();
        if (i < 0) {
          /* We couldn't assign a cache entry for this neighbor.
           * we won't be able to reply. drop it. */
          pbuf_free(p);
          ND6_STATS_INC(nd6.memerr);
          return;
        }
        neighbor_cache[i].netif = inp;
        MEMCPY(neighbor_cache[i].lladdr, lladdr_opt->addr, inp->hwaddr_len);
        ip6_addr_set(&(neighbor_cache[i].next_hop_address), ip6_current_src_addr());

        /* Receiving a message does not prove reachability: only in one direction.
         * Delay probe in case we get confirmation of reachability from upper layer (TCP). */
        neighbor_cache[i].state = ND6_DELAY;
        neighbor_cache[i].counter.delay_time = LWIP_ND6_DELAY_FIRST_PROBE_TIME / ND6_TMR_INTERVAL;
      }

      /* Send back a NA for us. Allocate the reply pbuf. */
      nd6_send_na(inp, &target_address, ND6_FLAG_SOLICITED | ND6_FLAG_OVERRIDE);
    }

    break; /* ICMP6_TYPE_NS */
  }
  case ICMP6_TYPE_RA: /* Router Advertisement. */
  {
    struct ra_header *ra_hdr;
    u8_t *buffer; /* Used to copy options. */
    u16_t offset;
#if LWIP_ND6_RDNSS_MAX_DNS_SERVERS
    /* There can be multiple RDNSS options per RA */
    u8_t rdnss_server_idx = 0;
#endif /* LWIP_ND6_RDNSS_MAX_DNS_SERVERS */

    /* Check that RA header fits in packet. */
    if (p->len < sizeof(struct ra_header)) {
      /* @todo debug message */
      pbuf_free(p);
      ND6_STATS_INC(nd6.lenerr);
      ND6_STATS_INC(nd6.drop);
      return;
    }

    ra_hdr = (struct ra_header *)p->payload;

    /* Check a subset of the other RFC 4861 Sec. 6.1.2 requirements. */
    if (!ip6_addr_islinklocal(ip6_current_src_addr()) ||
        IP6H_HOPLIM(ip6_current_header()) != ND6_HOPLIM || ra_hdr->code != 0) {
      pbuf_free(p);
      ND6_STATS_INC(nd6.proterr);
      ND6_STATS_INC(nd6.drop);
      return;
    }

    /* @todo RFC MUST: all included options have a length greater than zero */

    /* If we are sending RS messages, stop. */
#if LWIP_IPV6_SEND_ROUTER_SOLICIT
    /* ensure at least one solicitation is sent */
    if ((inp->rs_count < LWIP_ND6_MAX_MULTICAST_SOLICIT) ||
        (nd6_send_rs(inp) == ERR_OK)) {
      inp->rs_count = 0;
    }
#endif /* LWIP_IPV6_SEND_ROUTER_SOLICIT */

    /* Get the matching default router entry. */
    i = nd6_get_router(ip6_current_src_addr(), inp);
    if (i < 0) {
      /* Create a new router entry. */
      i = nd6_new_router(ip6_current_src_addr(), inp);
    }

    if (i < 0) {
      /* Could not create a new router entry. */
      pbuf_free(p);
      ND6_STATS_INC(nd6.memerr);
      return;
    }

    /* Re-set invalidation timer. */
    default_router_list[i].invalidation_timer = lwip_htons(ra_hdr->router_lifetime);

    /* Re-set default timer values. */
#if LWIP_ND6_ALLOW_RA_UPDATES
    if (ra_hdr->retrans_timer > 0) {
      retrans_timer = lwip_htonl(ra_hdr->retrans_timer);
    }
    if (ra_hdr->reachable_time > 0) {
      reachable_time = lwip_htonl(ra_hdr->reachable_time);
    }
#endif /* LWIP_ND6_ALLOW_RA_UPDATES */

    /* @todo set default hop limit... */
    /* ra_hdr->current_hop_limit;*/

    /* Update flags in local entry (incl. preference). */
    default_router_list[i].flags = ra_hdr->flags;

    /* Offset to options. */
    offset = sizeof(struct ra_header);

    /* Process each option. */
    while ((p->tot_len - offset) >= 2) {
      u8_t option_type;
      u16_t option_len;
      int option_len8 = pbuf_try_get_at(p, offset + 1);
      if (option_len8 <= 0) {
        /* read beyond end or zero length */
        goto lenerr_drop_free_return;
      }
      option_len = ((u8_t)option_len8) << 3;
      if (option_len > p->tot_len - offset) {
        /* short packet (option does not fit in) */
        goto lenerr_drop_free_return;
      }
      if (p->len == p->tot_len) {
        /* no need to copy from contiguous pbuf */
        buffer = &((u8_t*)p->payload)[offset];
      } else {
        /* check if this option fits into our buffer */
        if (option_len > sizeof(nd6_ra_buffer)) {
          option_type = pbuf_get_at(p, offset);
          /* invalid option length */
          if (option_type != ND6_OPTION_TYPE_RDNSS) {
            goto lenerr_drop_free_return;
          }
          /* we allow RDNSS option to be longer - we'll just drop some servers */
          option_len = sizeof(nd6_ra_buffer);
        }
        buffer = (u8_t*)&nd6_ra_buffer;
        option_len = pbuf_copy_partial(p, &nd6_ra_buffer, option_len, offset);
      }
      option_type = buffer[0];
      switch (option_type) {
      case ND6_OPTION_TYPE_SOURCE_LLADDR:
      {
        struct lladdr_option *lladdr_opt;
        if (option_len < sizeof(struct lladdr_option)) {
          goto lenerr_drop_free_return;
        }
        lladdr_opt = (struct lladdr_option *)buffer;
        if ((default_router_list[i].neighbor_entry != NULL) &&
            (default_router_list[i].neighbor_entry->state == ND6_INCOMPLETE)) {
          SMEMCPY(default_router_list[i].neighbor_entry->lladdr, lladdr_opt->addr, inp->hwaddr_len);
          default_router_list[i].neighbor_entry->state = ND6_REACHABLE;
          default_router_list[i].neighbor_entry->counter.reachable_time = reachable_time;
        }
        break;
      }
      case ND6_OPTION_TYPE_MTU:
      {
        struct mtu_option *mtu_opt;
        if (option_len < sizeof(struct mtu_option)) {
          goto lenerr_drop_free_return;
        }
        mtu_opt = (struct mtu_option *)buffer;
        if (lwip_htonl(mtu_opt->mtu) >= 1280) {
#if LWIP_ND6_ALLOW_RA_UPDATES
          inp->mtu = (u16_t)lwip_htonl(mtu_opt->mtu);
#endif /* LWIP_ND6_ALLOW_RA_UPDATES */
        }
        break;
      }
      case ND6_OPTION_TYPE_PREFIX_INFO:
      {
        struct prefix_option *prefix_opt;
        ip6_addr_t prefix_addr;
        if (option_len < sizeof(struct prefix_option)) {
          goto lenerr_drop_free_return;
        }

        prefix_opt = (struct prefix_option *)buffer;

        /* Get a memory-aligned copy of the prefix. */
        ip6_addr_copy_from_packed(prefix_addr, prefix_opt->prefix);
        ip6_addr_assign_zone(&prefix_addr, IP6_UNICAST, inp);

        if (!ip6_addr_islinklocal(&prefix_addr)) {
          if ((prefix_opt->flags & ND6_PREFIX_FLAG_ON_LINK) &&
              (prefix_opt->prefix_length == 64)) {
            /* Add to on-link prefix list. */
            u32_t valid_life;
            s8_t prefix;

            valid_life = lwip_htonl(prefix_opt->valid_lifetime);

            /* find cache entry for this prefix. */
            prefix = nd6_get_onlink_prefix(&prefix_addr, inp);
            if (prefix < 0 && valid_life > 0) {
              /* Create a new cache entry. */
              prefix = nd6_new_onlink_prefix(&prefix_addr, inp);
            }
            if (prefix >= 0) {
              prefix_list[prefix].invalidation_timer = valid_life;
            }
          }
#if LWIP_IPV6_AUTOCONFIG
          if (prefix_opt->flags & ND6_PREFIX_FLAG_AUTONOMOUS) {
            /* Perform processing for autoconfiguration. */
            nd6_process_autoconfig_prefix(inp, prefix_opt, &prefix_addr);
          }
#endif /* LWIP_IPV6_AUTOCONFIG */
        }

        break;
      }
      case ND6_OPTION_TYPE_ROUTE_INFO:
        /* @todo implement preferred routes.
        struct route_option * route_opt;
        route_opt = (struct route_option *)buffer;*/

        break;
#if LWIP_ND6_RDNSS_MAX_DNS_SERVERS
      case ND6_OPTION_TYPE_RDNSS:
      {
        u8_t num, n;
        u16_t copy_offset = offset + SIZEOF_RDNSS_OPTION_BASE;
        struct rdnss_option * rdnss_opt;
        if (option_len < SIZEOF_RDNSS_OPTION_BASE) {
          goto lenerr_drop_free_return;
        }

        rdnss_opt = (struct rdnss_option *)buffer;
        num = (rdnss_opt->length - 1) / 2;
        for (n = 0; (rdnss_server_idx < DNS_MAX_SERVERS) && (n < num); n++) {
          ip_addr_t rdnss_address;

          /* Copy directly from pbuf to get an aligned, zoned copy of the prefix. */
          if (pbuf_copy_partial(p, &rdnss_address, sizeof(ip6_addr_p_t), copy_offset) == sizeof(ip6_addr_p_t)) {
            IP_SET_TYPE_VAL(rdnss_address, IPADDR_TYPE_V6);
            ip6_addr_assign_zone(ip_2_ip6(&rdnss_address), IP6_UNKNOWN, inp);

            if (htonl(rdnss_opt->lifetime) > 0) {
              /* TODO implement Lifetime > 0 */
              dns_setserver(rdnss_server_idx++, &rdnss_address);
            } else {
              /* TODO implement DNS removal in dns.c */
              u8_t s;
              for (s = 0; s < DNS_MAX_SERVERS; s++) {
                const ip_addr_t *addr = dns_getserver(s);
                if(ip_addr_cmp(addr, &rdnss_address)) {
                  dns_setserver(s, NULL);
                }
              }
            }
          }
        }
        break;
      }
#endif /* LWIP_ND6_RDNSS_MAX_DNS_SERVERS */
      default:
        /* Unrecognized option, abort. */
        ND6_STATS_INC(nd6.proterr);
        break;
      }
      /* option length is checked earlier to be non-zero to make sure loop ends */
      offset += 8 * (u8_t)option_len8;
    }

    break; /* ICMP6_TYPE_RA */
  }
  case ICMP6_TYPE_RD: /* Redirect */
  {
    struct redirect_header *redir_hdr;
    struct lladdr_option *lladdr_opt;
    ip6_addr_t destination_address, target_address;

    /* Check that Redir header fits in packet. */
    if (p->len < sizeof(struct redirect_header)) {
      /* @todo debug message */
      pbuf_free(p);
      ND6_STATS_INC(nd6.lenerr);
      ND6_STATS_INC(nd6.drop);
      return;
    }

    redir_hdr = (struct redirect_header *)p->payload;

    /* Create an aligned, zoned copy of the destination address. */
    ip6_addr_copy_from_packed(destination_address, redir_hdr->destination_address);
    ip6_addr_assign_zone(&destination_address, IP6_UNICAST, inp);

    /* Check a subset of the other RFC 4861 Sec. 8.1 requirements. */
    if (!ip6_addr_islinklocal(ip6_current_src_addr()) ||
        IP6H_HOPLIM(ip6_current_header()) != ND6_HOPLIM ||
        redir_hdr->code != 0 || ip6_addr_ismulticast(&destination_address)) {
      pbuf_free(p);
      ND6_STATS_INC(nd6.proterr);
      ND6_STATS_INC(nd6.drop);
      return;
    }

    /* @todo RFC MUST: IP source address equals first-hop router for destination_address */
    /* @todo RFC MUST: ICMP target address is either link-local address or same as destination_address */
    /* @todo RFC MUST: all included options have a length greater than zero */

    if (p->len >= (sizeof(struct redirect_header) + 2)) {
      lladdr_opt = (struct lladdr_option *)((u8_t*)p->payload + sizeof(struct redirect_header));
      if (p->len < (sizeof(struct redirect_header) + (lladdr_opt->length << 3))) {
        lladdr_opt = NULL;
      }
    } else {
      lladdr_opt = NULL;
    }

    /* Find dest address in cache */
    i = nd6_find_destination_cache_entry(&destination_address);
    if (i < 0) {
      /* Destination not in cache, drop packet. */
      pbuf_free(p);
      return;
    }

    /* Create an aligned, zoned copy of the target address. */
    ip6_addr_copy_from_packed(target_address, redir_hdr->target_address);
    ip6_addr_assign_zone(&target_address, IP6_UNICAST, inp);

    /* Set the new target address. */
    ip6_addr_copy(destination_cache[i].next_hop_addr, target_address);

    /* If Link-layer address of other router is given, try to add to neighbor cache. */
    if (lladdr_opt != NULL) {
      if (lladdr_opt->type == ND6_OPTION_TYPE_TARGET_LLADDR) {
        i = nd6_find_neighbor_cache_entry(&target_address);
        if (i < 0) {
          i = nd6_new_neighbor_cache_entry();
          if (i >= 0) {
            neighbor_cache[i].netif = inp;
            MEMCPY(neighbor_cache[i].lladdr, lladdr_opt->addr, inp->hwaddr_len);
            ip6_addr_copy(neighbor_cache[i].next_hop_address, target_address);

            /* Receiving a message does not prove reachability: only in one direction.
             * Delay probe in case we get confirmation of reachability from upper layer (TCP). */
            neighbor_cache[i].state = ND6_DELAY;
            neighbor_cache[i].counter.delay_time = LWIP_ND6_DELAY_FIRST_PROBE_TIME / ND6_TMR_INTERVAL;
          }
        }
        if (i >= 0) {
          if (neighbor_cache[i].state == ND6_INCOMPLETE) {
            MEMCPY(neighbor_cache[i].lladdr, lladdr_opt->addr, inp->hwaddr_len);
            /* Receiving a message does not prove reachability: only in one direction.
             * Delay probe in case we get confirmation of reachability from upper layer (TCP). */
            neighbor_cache[i].state = ND6_DELAY;
            neighbor_cache[i].counter.delay_time = LWIP_ND6_DELAY_FIRST_PROBE_TIME / ND6_TMR_INTERVAL;
          }
        }
      }
    }
    break; /* ICMP6_TYPE_RD */
  }
  case ICMP6_TYPE_PTB: /* Packet too big */
  {
    struct icmp6_hdr *icmp6hdr; /* Packet too big message */
    struct ip6_hdr *ip6hdr; /* IPv6 header of the packet which caused the error */
    u32_t pmtu;
    ip6_addr_t destination_address;

    /* Check that ICMPv6 header + IPv6 header fit in payload */
    if (p->len < (sizeof(struct icmp6_hdr) + IP6_HLEN)) {
      /* drop short packets */
      pbuf_free(p);
      ND6_STATS_INC(nd6.lenerr);
      ND6_STATS_INC(nd6.drop);
      return;
    }

    icmp6hdr = (struct icmp6_hdr *)p->payload;
    ip6hdr = (struct ip6_hdr *)((u8_t*)p->payload + sizeof(struct icmp6_hdr));

    /* Create an aligned, zoned copy of the destination address. */
    ip6_addr_copy_from_packed(destination_address, ip6hdr->dest);
    ip6_addr_assign_zone(&destination_address, IP6_UNKNOWN, inp);

    /* Look for entry in destination cache. */
    i = nd6_find_destination_cache_entry(&destination_address);
    if (i < 0) {
      /* Destination not in cache, drop packet. */
      pbuf_free(p);
      return;
    }

    /* Change the Path MTU. */
    pmtu = lwip_htonl(icmp6hdr->data);
    destination_cache[i].pmtu = (u16_t)LWIP_MIN(pmtu, 0xFFFF);

    break; /* ICMP6_TYPE_PTB */
  }

  default:
    ND6_STATS_INC(nd6.proterr);
    ND6_STATS_INC(nd6.drop);
    break; /* default */
  }

  pbuf_free(p);
  return;
lenerr_drop_free_return:
  ND6_STATS_INC(nd6.lenerr);
  ND6_STATS_INC(nd6.drop);
  pbuf_free(p);
}


/**
 * Periodic timer for Neighbor discovery functions:
 *
 * - Update neighbor reachability states
 * - Update destination cache entries age
 * - Update invalidation timers of default routers and on-link prefixes
 * - Update lifetimes of our addresses
 * - Perform duplicate address detection (DAD) for our addresses
 * - Send router solicitations
 */
void
nd6_tmr(void)
{
  s8_t i;
  struct netif *netif;

  /* Process neighbor entries. */
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    switch (neighbor_cache[i].state) {
    case ND6_INCOMPLETE:
      if ((neighbor_cache[i].counter.probes_sent >= LWIP_ND6_MAX_MULTICAST_SOLICIT) &&
          (!neighbor_cache[i].isrouter)) {
        /* Retries exceeded. */
        nd6_free_neighbor_cache_entry(i);
      } else {
        /* Send a NS for this entry. */
        neighbor_cache[i].counter.probes_sent++;
        nd6_send_neighbor_cache_probe(&neighbor_cache[i], ND6_SEND_FLAG_MULTICAST_DEST);
      }
      break;
    case ND6_REACHABLE:
      /* Send queued packets, if any are left. Should have been sent already. */
      if (neighbor_cache[i].q != NULL) {
        nd6_send_q(i);
      }
      if (neighbor_cache[i].counter.reachable_time <= ND6_TMR_INTERVAL) {
        /* Change to stale state. */
        neighbor_cache[i].state = ND6_STALE;
        neighbor_cache[i].counter.stale_time = 0;
      } else {
        neighbor_cache[i].counter.reachable_time -= ND6_TMR_INTERVAL;
      }
      break;
    case ND6_STALE:
      neighbor_cache[i].counter.stale_time++;
      break;
    case ND6_DELAY:
      if (neighbor_cache[i].counter.delay_time <= 1) {
        /* Change to PROBE state. */
        neighbor_cache[i].state = ND6_PROBE;
        neighbor_cache[i].counter.probes_sent = 0;
      } else {
        neighbor_cache[i].counter.delay_time--;
      }
      break;
    case ND6_PROBE:
      if ((neighbor_cache[i].counter.probes_sent >= LWIP_ND6_MAX_MULTICAST_SOLICIT) &&
          (!neighbor_cache[i].isrouter)) {
        /* Retries exceeded. */
        nd6_free_neighbor_cache_entry(i);
      } else {
        /* Send a NS for this entry. */
        neighbor_cache[i].counter.probes_sent++;
        nd6_send_neighbor_cache_probe(&neighbor_cache[i], 0);
      }
      break;
    case ND6_NO_ENTRY:
    default:
      /* Do nothing. */
      break;
    }
  }

  /* Process destination entries. */
  for (i = 0; i < LWIP_ND6_NUM_DESTINATIONS; i++) {
    destination_cache[i].age++;
  }

  /* Process router entries. */
  for (i = 0; i < LWIP_ND6_NUM_ROUTERS; i++) {
    if (default_router_list[i].neighbor_entry != NULL) {
      /* Active entry. */
      if (default_router_list[i].invalidation_timer <= ND6_TMR_INTERVAL / 1000) {
        /* No more than 1 second remaining. Clear this entry. Also clear any of
         * its destination cache entries, as per RFC 4861 Sec. 5.3 and 6.3.5. */
        s8_t j;
        for (j = 0; j < LWIP_ND6_NUM_DESTINATIONS; j++) {
          if (ip6_addr_cmp(&destination_cache[j].next_hop_addr,
               &default_router_list[i].neighbor_entry->next_hop_address)) {
             ip6_addr_set_any(&destination_cache[j].destination_addr);
          }
        }
        default_router_list[i].neighbor_entry->isrouter = 0;
        default_router_list[i].neighbor_entry = NULL;
        default_router_list[i].invalidation_timer = 0;
        default_router_list[i].flags = 0;
      } else {
        default_router_list[i].invalidation_timer -= ND6_TMR_INTERVAL / 1000;
      }
    }
  }

  /* Process prefix entries. */
  for (i = 0; i < LWIP_ND6_NUM_PREFIXES; i++) {
    if (prefix_list[i].netif != NULL) {
      if (prefix_list[i].invalidation_timer <= ND6_TMR_INTERVAL / 1000) {
        /* Entry timed out, remove it */
        prefix_list[i].invalidation_timer = 0;
        prefix_list[i].netif = NULL;
      } else {
        prefix_list[i].invalidation_timer -= ND6_TMR_INTERVAL / 1000;
      }
    }
  }

  /* Process our own addresses, updating address lifetimes and/or DAD state. */
  NETIF_FOREACH(netif) {
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
      u8_t addr_state;
#if LWIP_IPV6_ADDRESS_LIFETIMES
      /* Step 1: update address lifetimes (valid and preferred). */
      addr_state = netif_ip6_addr_state(netif, i);
      /* RFC 4862 is not entirely clear as to whether address lifetimes affect
       * tentative addresses, and is even less clear as to what should happen
       * with duplicate addresses. We choose to track and update lifetimes for
       * both those types, although for different reasons:
       * - for tentative addresses, the line of thought of Sec. 5.7 combined
       *   with the potentially long period that an address may be in tentative
       *   state (due to the interface being down) suggests that lifetimes
       *   should be independent of external factors which would include DAD;
       * - for duplicate addresses, retiring them early could result in a new
       *   but unwanted attempt at marking them as valid, while retiring them
       *   late/never could clog up address slots on the netif.
       * As a result, we may end up expiring addresses of either type here.
       */
      if (!ip6_addr_isinvalid(addr_state) &&
          !netif_ip6_addr_isstatic(netif, i)) {
        u32_t life = netif_ip6_addr_valid_life(netif, i);
        if (life <= ND6_TMR_INTERVAL / 1000) {
          /* The address has expired. */
          netif_ip6_addr_set_valid_life(netif, i, 0);
          netif_ip6_addr_set_pref_life(netif, i, 0);
          netif_ip6_addr_set_state(netif, i, IP6_ADDR_INVALID);
        } else {
          if (!ip6_addr_life_isinfinite(life)) {
            life -= ND6_TMR_INTERVAL / 1000;
            LWIP_ASSERT("bad valid lifetime", life != IP6_ADDR_LIFE_STATIC);
            netif_ip6_addr_set_valid_life(netif, i, life);
          }
          /* The address is still here. Update the preferred lifetime too. */
          life = netif_ip6_addr_pref_life(netif, i);
          if (life <= ND6_TMR_INTERVAL / 1000) {
            /* This case must also trigger if 'life' was already zero, so as to
             * deal correctly with advertised preferred-lifetime reductions. */
            netif_ip6_addr_set_pref_life(netif, i, 0);
            if (addr_state == IP6_ADDR_PREFERRED)
              netif_ip6_addr_set_state(netif, i, IP6_ADDR_DEPRECATED);
          } else if (!ip6_addr_life_isinfinite(life)) {
            life -= ND6_TMR_INTERVAL / 1000;
            netif_ip6_addr_set_pref_life(netif, i, life);
          }
        }
      }
      /* The address state may now have changed, so reobtain it next. */
#endif /* LWIP_IPV6_ADDRESS_LIFETIMES */
      /* Step 2: update DAD state. */
      addr_state = netif_ip6_addr_state(netif, i);
      if (ip6_addr_istentative(addr_state)) {
        if ((addr_state & IP6_ADDR_TENTATIVE_COUNT_MASK) >= LWIP_IPV6_DUP_DETECT_ATTEMPTS) {
          /* No NA received in response. Mark address as valid. For dynamic
           * addresses with an expired preferred lifetime, the state is set to
           * deprecated right away. That should almost never happen, though. */
          addr_state = IP6_ADDR_PREFERRED;
#if LWIP_IPV6_ADDRESS_LIFETIMES
          if (!netif_ip6_addr_isstatic(netif, i) &&
              netif_ip6_addr_pref_life(netif, i) == 0) {
            addr_state = IP6_ADDR_DEPRECATED;
          }
#endif /* LWIP_IPV6_ADDRESS_LIFETIMES */
          netif_ip6_addr_set_state(netif, i, addr_state);
        } else if (netif_is_up(netif) && netif_is_link_up(netif)) {
          /* tentative: set next state by increasing by one */
          netif_ip6_addr_set_state(netif, i, addr_state + 1);
          /* Send a NS for this address. Use the unspecified address as source
           * address in all cases (RFC 4862 Sec. 5.4.2), not in the least
           * because as it is, we only consider multicast replies for DAD. */
          nd6_send_ns(netif, netif_ip6_addr(netif, i),
            ND6_SEND_FLAG_MULTICAST_DEST | ND6_SEND_FLAG_ANY_SRC);
        }
      }
    }
  }

#if LWIP_IPV6_SEND_ROUTER_SOLICIT
  /* Send router solicitation messages, if necessary. */
  NETIF_FOREACH(netif) {
    if ((netif->rs_count > 0) && netif_is_up(netif) &&
        netif_is_link_up(netif) &&
        !ip6_addr_isinvalid(netif_ip6_addr_state(netif, 0)) &&
        !ip6_addr_isduplicated(netif_ip6_addr_state(netif, 0))) {
      if (nd6_send_rs(netif) == ERR_OK) {
        netif->rs_count--;
      }
    }
  }
#endif /* LWIP_IPV6_SEND_ROUTER_SOLICIT */

}

/** Send a neighbor solicitation message for a specific neighbor cache entry
 *
 * @param entry the neightbor cache entry for wich to send the message
 * @param flags one of ND6_SEND_FLAG_*
 */
static void
nd6_send_neighbor_cache_probe(struct nd6_neighbor_cache_entry *entry, u8_t flags)
{
  nd6_send_ns(entry->netif, &entry->next_hop_address, flags);
}

/**
 * Send a neighbor solicitation message
 *
 * @param netif the netif on which to send the message
 * @param target_addr the IPv6 target address for the ND message
 * @param flags one of ND6_SEND_FLAG_*
 */
static void
nd6_send_ns(struct netif *netif, const ip6_addr_t *target_addr, u8_t flags)
{
  struct ns_header *ns_hdr;
  struct pbuf *p;
  const ip6_addr_t *src_addr;
  u16_t lladdr_opt_len;

  LWIP_ASSERT("target address is required", target_addr != NULL);

  if (!(flags & ND6_SEND_FLAG_ANY_SRC) &&
      ip6_addr_isvalid(netif_ip6_addr_state(netif,0))) {
    /* Use link-local address as source address. */
    src_addr = netif_ip6_addr(netif, 0);
    /* calculate option length (in 8-byte-blocks) */
    lladdr_opt_len = ((netif->hwaddr_len + 2) + 7) >> 3;
  } else {
    src_addr = IP6_ADDR_ANY6;
    /* Option "MUST NOT be included when the source IP address is the unspecified address." */
    lladdr_opt_len = 0;
  }

  /* Allocate a packet. */
  p = pbuf_alloc(PBUF_IP, sizeof(struct ns_header) + (lladdr_opt_len << 3), PBUF_RAM);
  if (p == NULL) {
    ND6_STATS_INC(nd6.memerr);
    return;
  }

  /* Set fields. */
  ns_hdr = (struct ns_header *)p->payload;

  ns_hdr->type = ICMP6_TYPE_NS;
  ns_hdr->code = 0;
  ns_hdr->chksum = 0;
  ns_hdr->reserved = 0;
  ip6_addr_copy_to_packed(ns_hdr->target_address, *target_addr);

  if (lladdr_opt_len != 0) {
    struct lladdr_option *lladdr_opt = (struct lladdr_option *)((u8_t*)p->payload + sizeof(struct ns_header));
    lladdr_opt->type = ND6_OPTION_TYPE_SOURCE_LLADDR;
    lladdr_opt->length = (u8_t)lladdr_opt_len;
    SMEMCPY(lladdr_opt->addr, netif->hwaddr, netif->hwaddr_len);
  }

  /* Generate the solicited node address for the target address. */
  if (flags & ND6_SEND_FLAG_MULTICAST_DEST) {
    ip6_addr_set_solicitednode(&multicast_address, target_addr->addr[3]);
    ip6_addr_assign_zone(&multicast_address, IP6_MULTICAST, netif);
    target_addr = &multicast_address;
  }

#if CHECKSUM_GEN_ICMP6
  IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_ICMP6) {
    ns_hdr->chksum = ip6_chksum_pseudo(p, IP6_NEXTH_ICMP6, p->len, src_addr,
      target_addr);
  }
#endif /* CHECKSUM_GEN_ICMP6 */

  /* Send the packet out. */
  ND6_STATS_INC(nd6.xmit);
  ip6_output_if(p, (src_addr == IP6_ADDR_ANY6) ? NULL : src_addr, target_addr,
      ND6_HOPLIM, 0, IP6_NEXTH_ICMP6, netif);
  pbuf_free(p);
}

/**
 * Send a neighbor advertisement message
 *
 * @param netif the netif on which to send the message
 * @param target_addr the IPv6 target address for the ND message
 * @param flags one of ND6_SEND_FLAG_*
 */
static void
nd6_send_na(struct netif *netif, const ip6_addr_t *target_addr, u8_t flags)
{
  struct na_header *na_hdr;
  struct lladdr_option *lladdr_opt;
  struct pbuf *p;
  const ip6_addr_t *src_addr;
  const ip6_addr_t *dest_addr;
  u16_t lladdr_opt_len;

  LWIP_ASSERT("target address is required", target_addr != NULL);

  /* Use link-local address as source address. */
  /* src_addr = netif_ip6_addr(netif, 0); */
  /* Use target address as source address. */
  src_addr = target_addr;

  /* Allocate a packet. */
  lladdr_opt_len = ((netif->hwaddr_len + 2) >> 3) + (((netif->hwaddr_len + 2) & 0x07) ? 1 : 0);
  p = pbuf_alloc(PBUF_IP, sizeof(struct na_header) + (lladdr_opt_len << 3), PBUF_RAM);
  if (p == NULL) {
    ND6_STATS_INC(nd6.memerr);
    return;
  }

  /* Set fields. */
  na_hdr = (struct na_header *)p->payload;
  lladdr_opt = (struct lladdr_option *)((u8_t*)p->payload + sizeof(struct na_header));

  na_hdr->type = ICMP6_TYPE_NA;
  na_hdr->code = 0;
  na_hdr->chksum = 0;
  na_hdr->flags = flags & 0xf0;
  na_hdr->reserved[0] = 0;
  na_hdr->reserved[1] = 0;
  na_hdr->reserved[2] = 0;
  ip6_addr_copy_to_packed(na_hdr->target_address, *target_addr);

  lladdr_opt->type = ND6_OPTION_TYPE_TARGET_LLADDR;
  lladdr_opt->length = (u8_t)lladdr_opt_len;
  SMEMCPY(lladdr_opt->addr, netif->hwaddr, netif->hwaddr_len);

  /* Generate the solicited node address for the target address. */
  if (flags & ND6_SEND_FLAG_MULTICAST_DEST) {
    ip6_addr_set_solicitednode(&multicast_address, target_addr->addr[3]);
    ip6_addr_assign_zone(&multicast_address, IP6_MULTICAST, netif);
    dest_addr = &multicast_address;
  } else if (flags & ND6_SEND_FLAG_ALLNODES_DEST) {
    ip6_addr_set_allnodes_linklocal(&multicast_address);
    ip6_addr_assign_zone(&multicast_address, IP6_MULTICAST, netif);
    dest_addr = &multicast_address;
  } else {
    dest_addr = ip6_current_src_addr();
  }

#if CHECKSUM_GEN_ICMP6
  IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_ICMP6) {
    na_hdr->chksum = ip6_chksum_pseudo(p, IP6_NEXTH_ICMP6, p->len, src_addr,
      dest_addr);
  }
#endif /* CHECKSUM_GEN_ICMP6 */

  /* Send the packet out. */
  ND6_STATS_INC(nd6.xmit);
  ip6_output_if(p, src_addr, dest_addr,
      ND6_HOPLIM, 0, IP6_NEXTH_ICMP6, netif);
  pbuf_free(p);
}

#if LWIP_IPV6_SEND_ROUTER_SOLICIT
/**
 * Send a router solicitation message
 *
 * @param netif the netif on which to send the message
 */
static err_t
nd6_send_rs(struct netif *netif)
{
  struct rs_header *rs_hdr;
  struct lladdr_option *lladdr_opt;
  struct pbuf *p;
  const ip6_addr_t *src_addr;
  err_t err;
  u16_t lladdr_opt_len = 0;

  /* Link-local source address, or unspecified address? */
  if (ip6_addr_isvalid(netif_ip6_addr_state(netif, 0))) {
    src_addr = netif_ip6_addr(netif, 0);
  } else {
    src_addr = IP6_ADDR_ANY6;
  }

  /* Generate the all routers target address. */
  ip6_addr_set_allrouters_linklocal(&multicast_address);
  ip6_addr_assign_zone(&multicast_address, IP6_MULTICAST, netif);

  /* Allocate a packet. */
  if (src_addr != IP6_ADDR_ANY6) {
    lladdr_opt_len = ((netif->hwaddr_len + 2) >> 3) + (((netif->hwaddr_len + 2) & 0x07) ? 1 : 0);
  }
  p = pbuf_alloc(PBUF_IP, sizeof(struct rs_header) + (lladdr_opt_len << 3), PBUF_RAM);
  if (p == NULL) {
    ND6_STATS_INC(nd6.memerr);
    return ERR_BUF;
  }

  /* Set fields. */
  rs_hdr = (struct rs_header *)p->payload;

  rs_hdr->type = ICMP6_TYPE_RS;
  rs_hdr->code = 0;
  rs_hdr->chksum = 0;
  rs_hdr->reserved = 0;

  if (src_addr != IP6_ADDR_ANY6) {
    /* Include our hw address. */
    lladdr_opt = (struct lladdr_option *)((u8_t*)p->payload + sizeof(struct rs_header));
    lladdr_opt->type = ND6_OPTION_TYPE_SOURCE_LLADDR;
    lladdr_opt->length = (u8_t)lladdr_opt_len;
    SMEMCPY(lladdr_opt->addr, netif->hwaddr, netif->hwaddr_len);
  }

#if CHECKSUM_GEN_ICMP6
  IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_ICMP6) {
    rs_hdr->chksum = ip6_chksum_pseudo(p, IP6_NEXTH_ICMP6, p->len, src_addr,
      &multicast_address);
  }
#endif /* CHECKSUM_GEN_ICMP6 */

  /* Send the packet out. */
  ND6_STATS_INC(nd6.xmit);

  err = ip6_output_if(p, (src_addr == IP6_ADDR_ANY6) ? NULL : src_addr, &multicast_address,
      ND6_HOPLIM, 0, IP6_NEXTH_ICMP6, netif);
  pbuf_free(p);

  return err;
}
#endif /* LWIP_IPV6_SEND_ROUTER_SOLICIT */

/**
 * Search for a neighbor cache entry
 *
 * @param ip6addr the IPv6 address of the neighbor
 * @return The neighbor cache entry index that matched, -1 if no
 * entry is found
 */
static s8_t
nd6_find_neighbor_cache_entry(const ip6_addr_t *ip6addr)
{
  s8_t i;
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if (ip6_addr_cmp(ip6addr, &(neighbor_cache[i].next_hop_address))) {
      return i;
    }
  }
  return -1;
}

/**
 * Create a new neighbor cache entry.
 *
 * If no unused entry is found, will try to recycle an old entry
 * according to ad-hoc "age" heuristic.
 *
 * @return The neighbor cache entry index that was created, -1 if no
 * entry could be created
 */
static s8_t
nd6_new_neighbor_cache_entry(void)
{
  s8_t i;
  s8_t j;
  u32_t time;


  /* First, try to find an empty entry. */
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if (neighbor_cache[i].state == ND6_NO_ENTRY) {
      return i;
    }
  }

  /* We need to recycle an entry. in general, do not recycle if it is a router. */

  /* Next, try to find a Stale entry. */
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if ((neighbor_cache[i].state == ND6_STALE) &&
        (!neighbor_cache[i].isrouter)) {
      nd6_free_neighbor_cache_entry(i);
      return i;
    }
  }

  /* Next, try to find a Probe entry. */
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if ((neighbor_cache[i].state == ND6_PROBE) &&
        (!neighbor_cache[i].isrouter)) {
      nd6_free_neighbor_cache_entry(i);
      return i;
    }
  }

  /* Next, try to find a Delayed entry. */
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if ((neighbor_cache[i].state == ND6_DELAY) &&
        (!neighbor_cache[i].isrouter)) {
      nd6_free_neighbor_cache_entry(i);
      return i;
    }
  }

  /* Next, try to find the oldest reachable entry. */
  time = 0xfffffffful;
  j = -1;
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if ((neighbor_cache[i].state == ND6_REACHABLE) &&
        (!neighbor_cache[i].isrouter)) {
      if (neighbor_cache[i].counter.reachable_time < time) {
        j = i;
        time = neighbor_cache[i].counter.reachable_time;
      }
    }
  }
  if (j >= 0) {
    nd6_free_neighbor_cache_entry(j);
    return j;
  }

  /* Next, find oldest incomplete entry without queued packets. */
  time = 0;
  j = -1;
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if (
        (neighbor_cache[i].q == NULL) &&
        (neighbor_cache[i].state == ND6_INCOMPLETE) &&
        (!neighbor_cache[i].isrouter)) {
      if (neighbor_cache[i].counter.probes_sent >= time) {
        j = i;
        time = neighbor_cache[i].counter.probes_sent;
      }
    }
  }
  if (j >= 0) {
    nd6_free_neighbor_cache_entry(j);
    return j;
  }

  /* Next, find oldest incomplete entry with queued packets. */
  time = 0;
  j = -1;
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if ((neighbor_cache[i].state == ND6_INCOMPLETE) &&
        (!neighbor_cache[i].isrouter)) {
      if (neighbor_cache[i].counter.probes_sent >= time) {
        j = i;
        time = neighbor_cache[i].counter.probes_sent;
      }
    }
  }
  if (j >= 0) {
    nd6_free_neighbor_cache_entry(j);
    return j;
  }

  /* No more entries to try. */
  return -1;
}

/**
 * Will free any resources associated with a neighbor cache
 * entry, and will mark it as unused.
 *
 * @param i the neighbor cache entry index to free
 */
static void
nd6_free_neighbor_cache_entry(s8_t i)
{
  if ((i < 0) || (i >= LWIP_ND6_NUM_NEIGHBORS)) {
    return;
  }
  if (neighbor_cache[i].isrouter) {
    /* isrouter needs to be cleared before deleting a neighbor cache entry */
    return;
  }

  /* Free any queued packets. */
  if (neighbor_cache[i].q != NULL) {
    nd6_free_q(neighbor_cache[i].q);
    neighbor_cache[i].q = NULL;
  }

  neighbor_cache[i].state = ND6_NO_ENTRY;
  neighbor_cache[i].isrouter = 0;
  neighbor_cache[i].netif = NULL;
  neighbor_cache[i].counter.reachable_time = 0;
  ip6_addr_set_zero(&(neighbor_cache[i].next_hop_address));
}

/**
 * Search for a destination cache entry
 *
 * @param ip6addr the IPv6 address of the destination
 * @return The destination cache entry index that matched, -1 if no
 * entry is found
 */
static s8_t
nd6_find_destination_cache_entry(const ip6_addr_t *ip6addr)
{
  s8_t i;

  IP6_ADDR_ZONECHECK(ip6addr);

  for (i = 0; i < LWIP_ND6_NUM_DESTINATIONS; i++) {
    if (ip6_addr_cmp(ip6addr, &(destination_cache[i].destination_addr))) {
      return i;
    }
  }
  return -1;
}

/**
 * Create a new destination cache entry. If no unused entry is found,
 * will recycle oldest entry.
 *
 * @return The destination cache entry index that was created, -1 if no
 * entry was created
 */
static s8_t
nd6_new_destination_cache_entry(void)
{
  s8_t i, j;
  u32_t age;

  /* Find an empty entry. */
  for (i = 0; i < LWIP_ND6_NUM_DESTINATIONS; i++) {
    if (ip6_addr_isany(&(destination_cache[i].destination_addr))) {
      return i;
    }
  }

  /* Find oldest entry. */
  age = 0;
  j = LWIP_ND6_NUM_DESTINATIONS - 1;
  for (i = 0; i < LWIP_ND6_NUM_DESTINATIONS; i++) {
    if (destination_cache[i].age > age) {
      j = i;
    }
  }

  return j;
}

/**
 * Clear the destination cache.
 *
 * This operation may be necessary for consistency in the light of changing
 * local addresses and/or use of the gateway hook.
 */
void
nd6_clear_destination_cache(void)
{
  int i;

  for (i = 0; i < LWIP_ND6_NUM_DESTINATIONS; i++) {
    ip6_addr_set_any(&destination_cache[i].destination_addr);
  }
}

/**
 * Determine whether an address matches an on-link prefix or the subnet of a
 * statically assigned address.
 *
 * @param ip6addr the IPv6 address to match
 * @return 1 if the address is on-link, 0 otherwise
 */
static s8_t
nd6_is_prefix_in_netif(const ip6_addr_t *ip6addr, struct netif *netif)
{
  s8_t i;

  /* Check to see if the address matches an on-link prefix. */
  for (i = 0; i < LWIP_ND6_NUM_PREFIXES; i++) {
    if ((prefix_list[i].netif == netif) &&
        (prefix_list[i].invalidation_timer > 0) &&
        ip6_addr_netcmp(ip6addr, &(prefix_list[i].prefix))) {
      return 1;
    }
  }
  /* Check to see if address prefix matches a manually configured (= static)
   * address. Static addresses have an implied /64 subnet assignment. Dynamic
   * addresses (from autoconfiguration) have no implied subnet assignment, and
   * are thus effectively /128 assignments. See RFC 5942 for more on this. */
  for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
    if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
        netif_ip6_addr_isstatic(netif, i) &&
        ip6_addr_netcmp(ip6addr, netif_ip6_addr(netif, i))) {
      return 1;
    }
  }
  return 0;
}

/**
 * Select a default router for a destination.
 *
 * This function is used both for routing and for finding a next-hop target for
 * a packet. In the former case, the given netif is NULL, and the returned
 * router entry must be for a netif suitable for sending packets (up, link up).
 * In the latter case, the given netif is not NULL and restricts router choice.
 *
 * @param ip6addr the destination address
 * @param netif the netif for the outgoing packet, if known
 * @return the default router entry index, or -1 if no suitable
 *         router is found
 */
static s8_t
nd6_select_router(const ip6_addr_t *ip6addr, struct netif *netif)
{
  struct netif *router_netif;
  s8_t i, j, valid_router;
  static s8_t last_router;

  LWIP_UNUSED_ARG(ip6addr); /* @todo match preferred routes!! (must implement ND6_OPTION_TYPE_ROUTE_INFO) */

  /* @todo: implement default router preference */

  /* Look for valid routers. A reachable router is preferred. */
  valid_router = -1;
  for (i = 0; i < LWIP_ND6_NUM_ROUTERS; i++) {
    /* Is the router netif both set and apppropriate? */
    if (default_router_list[i].neighbor_entry != NULL) {
      router_netif = default_router_list[i].neighbor_entry->netif;
      if ((router_netif != NULL) && (netif != NULL ? netif == router_netif :
          (netif_is_up(router_netif) && netif_is_link_up(router_netif)))) {
        /* Is the router valid, i.e., reachable or probably reachable as per
         * RFC 4861 Sec. 6.3.6? Note that we will never return a router that
         * has no neighbor cache entry, due to the netif association tests. */
        if (default_router_list[i].neighbor_entry->state != ND6_INCOMPLETE) {
          /* Is the router known to be reachable? */
          if (default_router_list[i].neighbor_entry->state == ND6_REACHABLE) {
            return i; /* valid and reachable - done! */
          } else if (valid_router < 0) {
            valid_router = i; /* valid but not known to be reachable */
          }
        }
      }
    }
  }
  if (valid_router >= 0) {
    return valid_router;
  }

  /* Look for any router for which we have any information at all. */
  /* last_router is used for round-robin selection of incomplete routers, as
   * recommended in RFC 4861 Sec. 6.3.6 point (2). Advance only when picking a
   * route, to select the same router as next-hop target in the common case. */
  if ((netif == NULL) && (++last_router >= LWIP_ND6_NUM_ROUTERS)) {
    last_router = 0;
  }
  i = last_router;
  for (j = 0; j < LWIP_ND6_NUM_ROUTERS; j++) {
    if (default_router_list[i].neighbor_entry != NULL) {
      router_netif = default_router_list[i].neighbor_entry->netif;
      if ((router_netif != NULL) && (netif != NULL ? netif == router_netif :
          (netif_is_up(router_netif) && netif_is_link_up(router_netif)))) {
        return i;
      }
    }
    if (++i >= LWIP_ND6_NUM_ROUTERS) {
      i = 0;
    }
  }

  /* no suitable router found. */
  return -1;
}

/**
 * Find a router-announced route to the given destination. This route may be
 * based on an on-link prefix or a default router.
 *
 * If a suitable route is found, the returned netif is guaranteed to be in a
 * suitable state (up, link up) to be used for packet transmission.
 *
 * @param ip6addr the destination IPv6 address
 * @return the netif to use for the destination, or NULL if none found
 */
struct netif *
nd6_find_route(const ip6_addr_t *ip6addr)
{
  struct netif *netif;
  s8_t i;

  /* @todo decide if it makes sense to check the destination cache first */

  /* Check if there is a matching on-link prefix. There may be multiple
   * matches. Pick the first one that is associated with a suitable netif. */
  for (i = 0; i < LWIP_ND6_NUM_PREFIXES; ++i) {
    netif = prefix_list[i].netif;
    if ((netif != NULL) && ip6_addr_netcmp(&prefix_list[i].prefix, ip6addr) &&
        netif_is_up(netif) && netif_is_link_up(netif)) {
      return netif;
    }
  }

  /* No on-link prefix match. Find a router that can forward the packet. */
  i = nd6_select_router(ip6addr, NULL);
  if (i >= 0) {
    LWIP_ASSERT("selected router must have a neighbor entry",
      default_router_list[i].neighbor_entry != NULL);
    return default_router_list[i].neighbor_entry->netif;
  }

  return NULL;
}

/**
 * Find an entry for a default router.
 *
 * @param router_addr the IPv6 address of the router
 * @param netif the netif on which the router is found, if known
 * @return the index of the router entry, or -1 if not found
 */
static s8_t
nd6_get_router(const ip6_addr_t *router_addr, struct netif *netif)
{
  s8_t i;

  IP6_ADDR_ZONECHECK_NETIF(router_addr, netif);

  /* Look for router. */
  for (i = 0; i < LWIP_ND6_NUM_ROUTERS; i++) {
    if ((default_router_list[i].neighbor_entry != NULL) &&
        ((netif != NULL) ? netif == default_router_list[i].neighbor_entry->netif : 1) &&
        ip6_addr_cmp(router_addr, &(default_router_list[i].neighbor_entry->next_hop_address))) {
      return i;
    }
  }

  /* router not found. */
  return -1;
}

/**
 * Create a new entry for a default router.
 *
 * @param router_addr the IPv6 address of the router
 * @param netif the netif on which the router is connected, if known
 * @return the index on the router table, or -1 if could not be created
 */
static s8_t
nd6_new_router(const ip6_addr_t *router_addr, struct netif *netif)
{
  s8_t router_index;
  s8_t free_router_index;
  s8_t neighbor_index;

  IP6_ADDR_ZONECHECK_NETIF(router_addr, netif);

  /* Do we have a neighbor entry for this router? */
  neighbor_index = nd6_find_neighbor_cache_entry(router_addr);
  if (neighbor_index < 0) {
    /* Create a neighbor entry for this router. */
    neighbor_index = nd6_new_neighbor_cache_entry();
    if (neighbor_index < 0) {
      /* Could not create neighbor entry for this router. */
      return -1;
    }
    ip6_addr_set(&(neighbor_cache[neighbor_index].next_hop_address), router_addr);
    neighbor_cache[neighbor_index].netif = netif;
    neighbor_cache[neighbor_index].q = NULL;
    neighbor_cache[neighbor_index].state = ND6_INCOMPLETE;
    neighbor_cache[neighbor_index].counter.probes_sent = 1;
    nd6_send_neighbor_cache_probe(&neighbor_cache[neighbor_index], ND6_SEND_FLAG_MULTICAST_DEST);
  }

  /* Mark neighbor as router. */
  neighbor_cache[neighbor_index].isrouter = 1;

  /* Look for empty entry. */
  free_router_index = LWIP_ND6_NUM_ROUTERS;
  for (router_index = LWIP_ND6_NUM_ROUTERS - 1; router_index >= 0; router_index--) {
    /* check if router already exists (this is a special case for 2 netifs on the same subnet
       - e.g. wifi and cable) */
    if(default_router_list[router_index].neighbor_entry == &(neighbor_cache[neighbor_index])){ 
      return router_index; 
    } 
    if (default_router_list[router_index].neighbor_entry == NULL) {
      /* remember lowest free index to create a new entry */
      free_router_index = router_index;
    }
  }
  if (free_router_index < LWIP_ND6_NUM_ROUTERS) {
    default_router_list[free_router_index].neighbor_entry = &(neighbor_cache[neighbor_index]);
    return free_router_index;
  }

  /* Could not create a router entry. */

  /* Mark neighbor entry as not-router. Entry might be useful as neighbor still. */
  neighbor_cache[neighbor_index].isrouter = 0;

  /* router not found. */
  return -1;
}

/**
 * Find the cached entry for an on-link prefix.
 *
 * @param prefix the IPv6 prefix that is on-link
 * @param netif the netif on which the prefix is on-link
 * @return the index on the prefix table, or -1 if not found
 */
static s8_t
nd6_get_onlink_prefix(const ip6_addr_t *prefix, struct netif *netif)
{
  s8_t i;

  /* Look for prefix in list. */
  for (i = 0; i < LWIP_ND6_NUM_PREFIXES; ++i) {
    if ((ip6_addr_netcmp(&(prefix_list[i].prefix), prefix)) &&
        (prefix_list[i].netif == netif)) {
      return i;
    }
  }

  /* Entry not available. */
  return -1;
}

/**
 * Creates a new entry for an on-link prefix.
 *
 * @param prefix the IPv6 prefix that is on-link
 * @param netif the netif on which the prefix is on-link
 * @return the index on the prefix table, or -1 if not created
 */
static s8_t
nd6_new_onlink_prefix(const ip6_addr_t *prefix, struct netif *netif)
{
  s8_t i;

  /* Create new entry. */
  for (i = 0; i < LWIP_ND6_NUM_PREFIXES; ++i) {
    if ((prefix_list[i].netif == NULL) ||
        (prefix_list[i].invalidation_timer == 0)) {
      /* Found empty prefix entry. */
      prefix_list[i].netif = netif;
      ip6_addr_set(&(prefix_list[i].prefix), prefix);
      return i;
    }
  }

  /* Entry not available. */
  return -1;
}

/**
 * Determine the next hop for a destination. Will determine if the
 * destination is on-link, else a suitable on-link router is selected.
 *
 * The last entry index is cached for fast entry search.
 *
 * @param ip6addr the destination address
 * @param netif the netif on which the packet will be sent
 * @return the neighbor cache entry for the next hop, ERR_RTE if no
 *         suitable next hop was found, ERR_MEM if no cache entry
 *         could be created
 */
static s8_t
nd6_get_next_hop_entry(const ip6_addr_t *ip6addr, struct netif *netif)
{
#ifdef LWIP_HOOK_ND6_GET_GW
  const ip6_addr_t *next_hop_addr;
#endif /* LWIP_HOOK_ND6_GET_GW */
  s8_t i;

  IP6_ADDR_ZONECHECK_NETIF(ip6addr, netif);

#if LWIP_NETIF_HWADDRHINT
  if (netif->hints != NULL) {
    /* per-pcb cached entry was given */
    u8_t addr_hint = netif->hints->addr_hint;
    if (addr_hint < LWIP_ND6_NUM_DESTINATIONS) {
      nd6_cached_destination_index = addr_hint;
    }
  }
#endif /* LWIP_NETIF_HWADDRHINT */

  /* Look for ip6addr in destination cache. */
  if (ip6_addr_cmp(ip6addr, &(destination_cache[nd6_cached_destination_index].destination_addr))) {
    /* the cached entry index is the right one! */
    /* do nothing. */
    ND6_STATS_INC(nd6.cachehit);
  } else {
    /* Search destination cache. */
    i = nd6_find_destination_cache_entry(ip6addr);
    if (i >= 0) {
      /* found destination entry. make it our new cached index. */
      nd6_cached_destination_index = i;
    } else {
      /* Not found. Create a new destination entry. */
      i = nd6_new_destination_cache_entry();
      if (i >= 0) {
        /* got new destination entry. make it our new cached index. */
        nd6_cached_destination_index = i;
      } else {
        /* Could not create a destination cache entry. */
        return ERR_MEM;
      }

      /* Copy dest address to destination cache. */
      ip6_addr_set(&(destination_cache[nd6_cached_destination_index].destination_addr), ip6addr);

      /* Now find the next hop. is it a neighbor? */
      if (ip6_addr_islinklocal(ip6addr) ||
          nd6_is_prefix_in_netif(ip6addr, netif)) {
        /* Destination in local link. */
        destination_cache[nd6_cached_destination_index].pmtu = netif->mtu;
        ip6_addr_copy(destination_cache[nd6_cached_destination_index].next_hop_addr, destination_cache[nd6_cached_destination_index].destination_addr);
#ifdef LWIP_HOOK_ND6_GET_GW
      } else if ((next_hop_addr = LWIP_HOOK_ND6_GET_GW(netif, ip6addr)) != NULL) {
        /* Next hop for destination provided by hook function. */
        destination_cache[nd6_cached_destination_index].pmtu = netif->mtu;
        ip6_addr_set(&destination_cache[nd6_cached_destination_index].next_hop_addr, next_hop_addr);
#endif /* LWIP_HOOK_ND6_GET_GW */
      } else {
        /* We need to select a router. */
        i = nd6_select_router(ip6addr, netif);
        if (i < 0) {
          /* No router found. */
          ip6_addr_set_any(&(destination_cache[nd6_cached_destination_index].destination_addr));
          return ERR_RTE;
        }
        destination_cache[nd6_cached_destination_index].pmtu = netif->mtu; /* Start with netif mtu, correct through ICMPv6 if necessary */
        ip6_addr_copy(destination_cache[nd6_cached_destination_index].next_hop_addr, default_router_list[i].neighbor_entry->next_hop_address);
      }
    }
  }

#if LWIP_NETIF_HWADDRHINT
  if (netif->hints != NULL) {
    /* per-pcb cached entry was given */
    netif->hints->addr_hint = nd6_cached_destination_index;
  }
#endif /* LWIP_NETIF_HWADDRHINT */

  /* Look in neighbor cache for the next-hop address. */
  if (ip6_addr_cmp(&(destination_cache[nd6_cached_destination_index].next_hop_addr),
                   &(neighbor_cache[nd6_cached_neighbor_index].next_hop_address))) {
    /* Cache hit. */
    /* Do nothing. */
    ND6_STATS_INC(nd6.cachehit);
  } else {
    i = nd6_find_neighbor_cache_entry(&(destination_cache[nd6_cached_destination_index].next_hop_addr));
    if (i >= 0) {
      /* Found a matching record, make it new cached entry. */
      nd6_cached_neighbor_index = i;
    } else {
      /* Neighbor not in cache. Make a new entry. */
      i = nd6_new_neighbor_cache_entry();
      if (i >= 0) {
        /* got new neighbor entry. make it our new cached index. */
        nd6_cached_neighbor_index = i;
      } else {
        /* Could not create a neighbor cache entry. */
        return ERR_MEM;
      }

      /* Initialize fields. */
      ip6_addr_copy(neighbor_cache[i].next_hop_address,
                   destination_cache[nd6_cached_destination_index].next_hop_addr);
      neighbor_cache[i].isrouter = 0;
      neighbor_cache[i].netif = netif;
      neighbor_cache[i].state = ND6_INCOMPLETE;
      neighbor_cache[i].counter.probes_sent = 1;
      nd6_send_neighbor_cache_probe(&neighbor_cache[i], ND6_SEND_FLAG_MULTICAST_DEST);
    }
  }

  /* Reset this destination's age. */
  destination_cache[nd6_cached_destination_index].age = 0;

  return nd6_cached_neighbor_index;
}

/**
 * Queue a packet for a neighbor.
 *
 * @param neighbor_index the index in the neighbor cache table
 * @param q packet to be queued
 * @return ERR_OK if succeeded, ERR_MEM if out of memory
 */
static err_t
nd6_queue_packet(s8_t neighbor_index, struct pbuf *q)
{
  err_t result = ERR_MEM;
  struct pbuf *p;
  int copy_needed = 0;
#if LWIP_ND6_QUEUEING
  struct nd6_q_entry *new_entry, *r;
#endif /* LWIP_ND6_QUEUEING */

  if ((neighbor_index < 0) || (neighbor_index >= LWIP_ND6_NUM_NEIGHBORS)) {
    return ERR_ARG;
  }

  /* IF q includes a pbuf that must be copied, we have to copy the whole chain
   * into a new PBUF_RAM. See the definition of PBUF_NEEDS_COPY for details. */
  p = q;
  while (p) {
    if (PBUF_NEEDS_COPY(p)) {
      copy_needed = 1;
      break;
    }
    p = p->next;
  }
  if (copy_needed) {
    /* copy the whole packet into new pbufs */
    p = pbuf_alloc(PBUF_LINK, q->tot_len, PBUF_RAM);
    while ((p == NULL) && (neighbor_cache[neighbor_index].q != NULL)) {
      /* Free oldest packet (as per RFC recommendation) */
#if LWIP_ND6_QUEUEING
      r = neighbor_cache[neighbor_index].q;
      neighbor_cache[neighbor_index].q = r->next;
      r->next = NULL;
      nd6_free_q(r);
#else /* LWIP_ND6_QUEUEING */
      pbuf_free(neighbor_cache[neighbor_index].q);
      neighbor_cache[neighbor_index].q = NULL;
#endif /* LWIP_ND6_QUEUEING */
      p = pbuf_alloc(PBUF_LINK, q->tot_len, PBUF_RAM);
    }
    if (p != NULL) {
      if (pbuf_copy(p, q) != ERR_OK) {
        pbuf_free(p);
        p = NULL;
      }
    }
  } else {
    /* referencing the old pbuf is enough */
    p = q;
    pbuf_ref(p);
  }
  /* packet was copied/ref'd? */
  if (p != NULL) {
    /* queue packet ... */
#if LWIP_ND6_QUEUEING
    /* allocate a new nd6 queue entry */
    new_entry = (struct nd6_q_entry *)memp_malloc(MEMP_ND6_QUEUE);
    if ((new_entry == NULL) && (neighbor_cache[neighbor_index].q != NULL)) {
      /* Free oldest packet (as per RFC recommendation) */
      r = neighbor_cache[neighbor_index].q;
      neighbor_cache[neighbor_index].q = r->next;
      r->next = NULL;
      nd6_free_q(r);
      new_entry = (struct nd6_q_entry *)memp_malloc(MEMP_ND6_QUEUE);
    }
    if (new_entry != NULL) {
      new_entry->next = NULL;
      new_entry->p = p;
      if (neighbor_cache[neighbor_index].q != NULL) {
        /* queue was already existent, append the new entry to the end */
        r = neighbor_cache[neighbor_index].q;
        while (r->next != NULL) {
          r = r->next;
        }
        r->next = new_entry;
      } else {
        /* queue did not exist, first item in queue */
        neighbor_cache[neighbor_index].q = new_entry;
      }
      LWIP_DEBUGF(LWIP_DBG_TRACE, ("ipv6: queued packet %p on neighbor entry %"S16_F"\n", (void *)p, (s16_t)neighbor_index));
      result = ERR_OK;
    } else {
      /* the pool MEMP_ND6_QUEUE is empty */
      pbuf_free(p);
      LWIP_DEBUGF(LWIP_DBG_TRACE, ("ipv6: could not queue a copy of packet %p (out of memory)\n", (void *)p));
      /* { result == ERR_MEM } through initialization */
    }
#else /* LWIP_ND6_QUEUEING */
    /* Queue a single packet. If an older packet is already queued, free it as per RFC. */
    if (neighbor_cache[neighbor_index].q != NULL) {
      pbuf_free(neighbor_cache[neighbor_index].q);
    }
    neighbor_cache[neighbor_index].q = p;
    LWIP_DEBUGF(LWIP_DBG_TRACE, ("ipv6: queued packet %p on neighbor entry %"S16_F"\n", (void *)p, (s16_t)neighbor_index));
    result = ERR_OK;
#endif /* LWIP_ND6_QUEUEING */
  } else {
    LWIP_DEBUGF(LWIP_DBG_TRACE, ("ipv6: could not queue a copy of packet %p (out of memory)\n", (void *)q));
    /* { result == ERR_MEM } through initialization */
  }

  return result;
}

#if LWIP_ND6_QUEUEING
/**
 * Free a complete queue of nd6 q entries
 *
 * @param q a queue of nd6_q_entry to free
 */
static void
nd6_free_q(struct nd6_q_entry *q)
{
  struct nd6_q_entry *r;
  LWIP_ASSERT("q != NULL", q != NULL);
  LWIP_ASSERT("q->p != NULL", q->p != NULL);
  while (q) {
    r = q;
    q = q->next;
    LWIP_ASSERT("r->p != NULL", (r->p != NULL));
    pbuf_free(r->p);
    memp_free(MEMP_ND6_QUEUE, r);
  }
}
#endif /* LWIP_ND6_QUEUEING */

/**
 * Send queued packets for a neighbor
 *
 * @param i the neighbor to send packets to
 */
static void
nd6_send_q(s8_t i)
{
  struct ip6_hdr *ip6hdr;
  ip6_addr_t dest;
#if LWIP_ND6_QUEUEING
  struct nd6_q_entry *q;
#endif /* LWIP_ND6_QUEUEING */

  if ((i < 0) || (i >= LWIP_ND6_NUM_NEIGHBORS)) {
    return;
  }

#if LWIP_ND6_QUEUEING
  while (neighbor_cache[i].q != NULL) {
    /* remember first in queue */
    q = neighbor_cache[i].q;
    /* pop first item off the queue */
    neighbor_cache[i].q = q->next;
    /* Get ipv6 header. */
    ip6hdr = (struct ip6_hdr *)(q->p->payload);
    /* Create an aligned copy. */
    ip6_addr_copy_from_packed(dest, ip6hdr->dest);
    /* Restore the zone, if applicable. */
    ip6_addr_assign_zone(&dest, IP6_UNKNOWN, neighbor_cache[i].netif);
    /* send the queued IPv6 packet */
    (neighbor_cache[i].netif)->output_ip6(neighbor_cache[i].netif, q->p, &dest);
    /* free the queued IP packet */
    pbuf_free(q->p);
    /* now queue entry can be freed */
    memp_free(MEMP_ND6_QUEUE, q);
  }
#else /* LWIP_ND6_QUEUEING */
  if (neighbor_cache[i].q != NULL) {
    /* Get ipv6 header. */
    ip6hdr = (struct ip6_hdr *)(neighbor_cache[i].q->payload);
    /* Create an aligned copy. */
    ip6_addr_copy_from_packed(dest, ip6hdr->dest);
    /* Restore the zone, if applicable. */
    ip6_addr_assign_zone(&dest, IP6_UNKNOWN, neighbor_cache[i].netif);
    /* send the queued IPv6 packet */
    (neighbor_cache[i].netif)->output_ip6(neighbor_cache[i].netif, neighbor_cache[i].q, &dest);
    /* free the queued IP packet */
    pbuf_free(neighbor_cache[i].q);
    neighbor_cache[i].q = NULL;
  }
#endif /* LWIP_ND6_QUEUEING */
}

/**
 * A packet is to be transmitted to a specific IPv6 destination on a specific
 * interface. Check if we can find the hardware address of the next hop to use
 * for the packet. If so, give the hardware address to the caller, which should
 * use it to send the packet right away. Otherwise, enqueue the packet for
 * later transmission while looking up the hardware address, if possible.
 *
 * As such, this function returns one of three different possible results:
 *
 * - ERR_OK with a non-NULL 'hwaddrp': the caller should send the packet now.
 * - ERR_OK with a NULL 'hwaddrp': the packet has been enqueued for later.
 * - not ERR_OK: something went wrong; forward the error upward in the stack.
 *
 * @param netif The lwIP network interface on which the IP packet will be sent.
 * @param q The pbuf(s) containing the IP packet to be sent.
 * @param ip6addr The destination IPv6 address of the packet.
 * @param hwaddrp On success, filled with a pointer to a HW address or NULL (meaning
 *        the packet has been queued).
 * @return
 * - ERR_OK on success, ERR_RTE if no route was found for the packet,
 * or ERR_MEM if low memory conditions prohibit sending the packet at all.
 */
err_t
nd6_get_next_hop_addr_or_queue(struct netif *netif, struct pbuf *q, const ip6_addr_t *ip6addr, const u8_t **hwaddrp)
{
  s8_t i;

  /* Get next hop record. */
  i = nd6_get_next_hop_entry(ip6addr, netif);
  if (i < 0) {
    /* failed to get a next hop neighbor record. */
    return i;
  }

  /* Now that we have a destination record, send or queue the packet. */
  if (neighbor_cache[i].state == ND6_STALE) {
    /* Switch to delay state. */
    neighbor_cache[i].state = ND6_DELAY;
    neighbor_cache[i].counter.delay_time = LWIP_ND6_DELAY_FIRST_PROBE_TIME / ND6_TMR_INTERVAL;
  }
  /* @todo should we send or queue if PROBE? send for now, to let unicast NS pass. */
  if ((neighbor_cache[i].state == ND6_REACHABLE) ||
      (neighbor_cache[i].state == ND6_DELAY) ||
      (neighbor_cache[i].state == ND6_PROBE)) {

    /* Tell the caller to send out the packet now. */
    *hwaddrp = neighbor_cache[i].lladdr;
    return ERR_OK;
  }

  /* We should queue packet on this interface. */
  *hwaddrp = NULL;
  return nd6_queue_packet(i, q);
}


/**
 * Get the Path MTU for a destination.
 *
 * @param ip6addr the destination address
 * @param netif the netif on which the packet will be sent
 * @return the Path MTU, if known, or the netif default MTU
 */
u16_t
nd6_get_destination_mtu(const ip6_addr_t *ip6addr, struct netif *netif)
{
  s8_t i;

  i = nd6_find_destination_cache_entry(ip6addr);
  if (i >= 0) {
    if (destination_cache[i].pmtu > 0) {
      return destination_cache[i].pmtu;
    }
  }

  if (netif != NULL) {
    return netif->mtu;
  }

  return 1280; /* Minimum MTU */
}


#if LWIP_ND6_TCP_REACHABILITY_HINTS
/**
 * Provide the Neighbor discovery process with a hint that a
 * destination is reachable. Called by tcp_receive when ACKs are
 * received or sent (as per RFC). This is useful to avoid sending
 * NS messages every 30 seconds.
 *
 * @param ip6addr the destination address which is know to be reachable
 *                by an upper layer protocol (TCP)
 */
void
nd6_reachability_hint(const ip6_addr_t *ip6addr)
{
  s8_t i;

  /* Find destination in cache. */
  if (ip6_addr_cmp(ip6addr, &(destination_cache[nd6_cached_destination_index].destination_addr))) {
    i = nd6_cached_destination_index;
    ND6_STATS_INC(nd6.cachehit);
  } else {
    i = nd6_find_destination_cache_entry(ip6addr);
  }
  if (i < 0) {
    return;
  }

  /* Find next hop neighbor in cache. */
  if (ip6_addr_cmp(&(destination_cache[i].next_hop_addr), &(neighbor_cache[nd6_cached_neighbor_index].next_hop_address))) {
    i = nd6_cached_neighbor_index;
    ND6_STATS_INC(nd6.cachehit);
  } else {
    i = nd6_find_neighbor_cache_entry(&(destination_cache[i].next_hop_addr));
  }
  if (i < 0) {
    return;
  }

  /* For safety: don't set as reachable if we don't have a LL address yet. Misuse protection. */
  if (neighbor_cache[i].state == ND6_INCOMPLETE || neighbor_cache[i].state == ND6_NO_ENTRY) {
    return;
  }

  /* Set reachability state. */
  neighbor_cache[i].state = ND6_REACHABLE;
  neighbor_cache[i].counter.reachable_time = reachable_time;
}
#endif /* LWIP_ND6_TCP_REACHABILITY_HINTS */

/**
 * Remove all prefix, neighbor_cache and router entries of the specified netif.
 *
 * @param netif points to a network interface
 */
void
nd6_cleanup_netif(struct netif *netif)
{
  u8_t i;
  s8_t router_index;
  for (i = 0; i < LWIP_ND6_NUM_PREFIXES; i++) {
    if (prefix_list[i].netif == netif) {
      prefix_list[i].netif = NULL;
    }
  }
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if (neighbor_cache[i].netif == netif) {
      for (router_index = 0; router_index < LWIP_ND6_NUM_ROUTERS; router_index++) {
        if (default_router_list[router_index].neighbor_entry == &neighbor_cache[i]) {
          default_router_list[router_index].neighbor_entry = NULL;
          default_router_list[router_index].flags = 0;
        }
      }
      neighbor_cache[i].isrouter = 0;
      nd6_free_neighbor_cache_entry(i);
    }
  }
  /* Clear the destination cache, since many entries may now have become
   * invalid for one of several reasons. As destination cache entries have no
   * netif association, use a sledgehammer approach (this can be improved). */
  nd6_clear_destination_cache();
}

#if LWIP_IPV6_MLD
/**
 * The state of a local IPv6 address entry is about to change. If needed, join
 * or leave the solicited-node multicast group for the address.
 *
 * @param netif The netif that owns the address.
 * @param addr_idx The index of the address.
 * @param new_state The new (IP6_ADDR_) state for the address.
 */
void
nd6_adjust_mld_membership(struct netif *netif, s8_t addr_idx, u8_t new_state)
{
  u8_t old_state, old_member, new_member;

  old_state = netif_ip6_addr_state(netif, addr_idx);

  /* Determine whether we were, and should be, a member of the solicited-node
   * multicast group for this address. For tentative addresses, the group is
   * not joined until the address enters the TENTATIVE_1 (or VALID) state. */
  old_member = (old_state != IP6_ADDR_INVALID && old_state != IP6_ADDR_DUPLICATED && old_state != IP6_ADDR_TENTATIVE);
  new_member = (new_state != IP6_ADDR_INVALID && new_state != IP6_ADDR_DUPLICATED && new_state != IP6_ADDR_TENTATIVE);

  if (old_member != new_member) {
    ip6_addr_set_solicitednode(&multicast_address, netif_ip6_addr(netif, addr_idx)->addr[3]);
    ip6_addr_assign_zone(&multicast_address, IP6_MULTICAST, netif);

    if (new_member) {
      mld6_joingroup_netif(netif, &multicast_address);
    } else {
      mld6_leavegroup_netif(netif, &multicast_address);
    }
  }
}
#endif /* LWIP_IPV6_MLD */

#endif /* LWIP_IPV6 */
