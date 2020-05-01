/**
 * @file
 * Implementation of raw protocol PCBs for low-level handling of
 * different types of protocols besides (or overriding) those
 * already available in lwIP.\n
 * See also @ref raw_raw
 *
 * @defgroup raw_raw RAW
 * @ingroup callbackstyle_api
 * Implementation of raw protocol PCBs for low-level handling of
 * different types of protocols besides (or overriding) those
 * already available in lwIP.\n
 * @see @ref raw_api
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/raw.h"
#include "lwip/stats.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/inet_chksum.h"

#include <string.h>

/** The list of RAW PCBs */
static struct raw_pcb *raw_pcbs;

static u8_t
raw_input_local_match(struct raw_pcb *pcb, u8_t broadcast)
{
  LWIP_UNUSED_ARG(broadcast); /* in IPv6 only case */

  /* check if PCB is bound to specific netif */
  if ((pcb->netif_idx != NETIF_NO_INDEX) &&
      (pcb->netif_idx != netif_get_index(ip_data.current_input_netif))) {
    return 0;
  }

#if LWIP_IPV4 && LWIP_IPV6
  /* Dual-stack: PCBs listening to any IP type also listen to any IP address */
  if (IP_IS_ANY_TYPE_VAL(pcb->local_ip)) {
#if IP_SOF_BROADCAST_RECV
    if ((broadcast != 0) && !ip_get_option(pcb, SOF_BROADCAST)) {
      return 0;
    }
#endif /* IP_SOF_BROADCAST_RECV */
    return 1;
  }
#endif /* LWIP_IPV4 && LWIP_IPV6 */

  /* Only need to check PCB if incoming IP version matches PCB IP version */
  if (IP_ADDR_PCB_VERSION_MATCH_EXACT(pcb, ip_current_dest_addr())) {
#if LWIP_IPV4
    /* Special case: IPv4 broadcast: receive all broadcasts
     * Note: broadcast variable can only be 1 if it is an IPv4 broadcast */
    if (broadcast != 0) {
#if IP_SOF_BROADCAST_RECV
      if (ip_get_option(pcb, SOF_BROADCAST))
#endif /* IP_SOF_BROADCAST_RECV */
      {
        if (ip4_addr_isany(ip_2_ip4(&pcb->local_ip))) {
          return 1;
        }
      }
    } else
#endif /* LWIP_IPV4 */
      /* Handle IPv4 and IPv6: catch all or exact match */
      if (ip_addr_isany(&pcb->local_ip) ||
          ip_addr_cmp(&pcb->local_ip, ip_current_dest_addr())) {
        return 1;
      }
  }

  return 0;
}

/**
 * Determine if in incoming IP packet is covered by a RAW PCB
 * and if so, pass it to a user-provided receive callback function.
 *
 * Given an incoming IP datagram (as a chain of pbufs) this function
 * finds a corresponding RAW PCB and calls the corresponding receive
 * callback function.
 *
 * @param p pbuf to be demultiplexed to a RAW PCB.
 * @param inp network interface on which the datagram was received.
 * @return - 1 if the packet has been eaten by a RAW PCB receive
 *           callback function. The caller MAY NOT not reference the
 *           packet any longer, and MAY NOT call pbuf_free().
 * @return - 0 if packet is not eaten (pbuf is still referenced by the
 *           caller).
 *
 */
u8_t
raw_input(struct pbuf *p, struct netif *inp)
{
  struct raw_pcb *pcb, *prev;
  s16_t proto;
  u8_t eaten = 0;
  u8_t broadcast = ip_addr_isbroadcast(ip_current_dest_addr(), ip_current_netif());

  LWIP_UNUSED_ARG(inp);

#if LWIP_IPV6
#if LWIP_IPV4
  if (IP_HDR_GET_VERSION(p->payload) == 6)
#endif /* LWIP_IPV4 */
  {
    struct ip6_hdr *ip6hdr = (struct ip6_hdr *)p->payload;
    proto = IP6H_NEXTH(ip6hdr);
  }
#if LWIP_IPV4
  else
#endif /* LWIP_IPV4 */
#endif /* LWIP_IPV6 */
#if LWIP_IPV4
  {
    proto = IPH_PROTO((struct ip_hdr *)p->payload);
  }
#endif /* LWIP_IPV4 */

  prev = NULL;
  pcb = raw_pcbs;
  /* loop through all raw pcbs until the packet is eaten by one */
  /* this allows multiple pcbs to match against the packet by design */
  while ((eaten == 0) && (pcb != NULL)) {
    if ((pcb->protocol == proto) && raw_input_local_match(pcb, broadcast) &&
        (((pcb->flags & RAW_FLAGS_CONNECTED) == 0) ||
         ip_addr_cmp(&pcb->remote_ip, ip_current_src_addr()))) {
      /* receive callback function available? */
      if (pcb->recv != NULL) {
#ifndef LWIP_NOASSERT
        void *old_payload = p->payload;
#endif
        /* the receive callback function did not eat the packet? */
        eaten = pcb->recv(pcb->recv_arg, pcb, p, ip_current_src_addr());
        if (eaten != 0) {
          /* receive function ate the packet */
          p = NULL;
          eaten = 1;
          if (prev != NULL) {
            /* move the pcb to the front of raw_pcbs so that is
               found faster next time */
            prev->next = pcb->next;
            pcb->next = raw_pcbs;
            raw_pcbs = pcb;
          }
        } else {
          /* sanity-check that the receive callback did not alter the pbuf */
          LWIP_ASSERT("raw pcb recv callback altered pbuf payload pointer without eating packet",
                      p->payload == old_payload);
        }
      }
      /* no receive callback function was set for this raw PCB */
    }
    /* drop the packet */
    prev = pcb;
    pcb = pcb->next;
  }
  return eaten;
}

/**
 * @ingroup raw_raw
 * Bind a RAW PCB.
 *
 * @param pcb RAW PCB to be bound with a local address ipaddr.
 * @param ipaddr local IP address to bind with. Use IP4_ADDR_ANY to
 * bind to all local interfaces.
 *
 * @return lwIP error code.
 * - ERR_OK. Successful. No error occurred.
 * - ERR_USE. The specified IP address is already bound to by
 * another RAW PCB.
 *
 * @see raw_disconnect()
 */
err_t
raw_bind(struct raw_pcb *pcb, const ip_addr_t *ipaddr)
{
  if ((pcb == NULL) || (ipaddr == NULL)) {
    return ERR_VAL;
  }
  ip_addr_set_ipaddr(&pcb->local_ip, ipaddr);
#if LWIP_IPV6 && LWIP_IPV6_SCOPES
  /* If the given IP address should have a zone but doesn't, assign one now.
   * This is legacy support: scope-aware callers should always provide properly
   * zoned source addresses. */
  if (IP_IS_V6(&pcb->local_ip) &&
      ip6_addr_lacks_zone(ip_2_ip6(&pcb->local_ip), IP6_UNKNOWN)) {
    ip6_addr_select_zone(ip_2_ip6(&pcb->local_ip), ip_2_ip6(&pcb->local_ip));
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_SCOPES */
  return ERR_OK;
}

/**
 * @ingroup raw_raw
 * Bind an RAW PCB to a specific netif.
 * After calling this function, all packets received via this PCB
 * are guaranteed to have come in via the specified netif, and all
 * outgoing packets will go out via the specified netif.
 *
 * @param pcb RAW PCB to be bound with netif.
 * @param netif netif to bind to. Can be NULL.
 *
 * @see raw_disconnect()
 */
void
raw_bind_netif(struct raw_pcb *pcb, const struct netif *netif)
{
  if (netif != NULL) {
    pcb->netif_idx = netif_get_index(netif);
  } else {
    pcb->netif_idx = NETIF_NO_INDEX;
  }
}

/**
 * @ingroup raw_raw
 * Connect an RAW PCB. This function is required by upper layers
 * of lwip. Using the raw api you could use raw_sendto() instead
 *
 * This will associate the RAW PCB with the remote address.
 *
 * @param pcb RAW PCB to be connected with remote address ipaddr and port.
 * @param ipaddr remote IP address to connect with.
 *
 * @return lwIP error code
 *
 * @see raw_disconnect() and raw_sendto()
 */
err_t
raw_connect(struct raw_pcb *pcb, const ip_addr_t *ipaddr)
{
  if ((pcb == NULL) || (ipaddr == NULL)) {
    return ERR_VAL;
  }
  ip_addr_set_ipaddr(&pcb->remote_ip, ipaddr);
#if LWIP_IPV6 && LWIP_IPV6_SCOPES
  /* If the given IP address should have a zone but doesn't, assign one now,
   * using the bound address to make a more informed decision when possible. */
  if (IP_IS_V6(&pcb->remote_ip) &&
      ip6_addr_lacks_zone(ip_2_ip6(&pcb->remote_ip), IP6_UNKNOWN)) {
    ip6_addr_select_zone(ip_2_ip6(&pcb->remote_ip), ip_2_ip6(&pcb->local_ip));
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_SCOPES */
  raw_set_flags(pcb, RAW_FLAGS_CONNECTED);
  return ERR_OK;
}

/**
 * @ingroup raw_raw
 * Disconnect a RAW PCB.
 *
 * @param pcb the raw pcb to disconnect.
 */
void
raw_disconnect(struct raw_pcb *pcb)
{
  /* reset remote address association */
#if LWIP_IPV4 && LWIP_IPV6
  if (IP_IS_ANY_TYPE_VAL(pcb->local_ip)) {
    ip_addr_copy(pcb->remote_ip, *IP_ANY_TYPE);
  } else {
#endif
    ip_addr_set_any(IP_IS_V6_VAL(pcb->remote_ip), &pcb->remote_ip);
#if LWIP_IPV4 && LWIP_IPV6
  }
#endif
  pcb->netif_idx = NETIF_NO_INDEX;
  /* mark PCB as unconnected */
  raw_clear_flags(pcb, RAW_FLAGS_CONNECTED);
}

/**
 * @ingroup raw_raw
 * Set the callback function for received packets that match the
 * raw PCB's protocol and binding.
 *
 * The callback function MUST either
 * - eat the packet by calling pbuf_free() and returning non-zero. The
 *   packet will not be passed to other raw PCBs or other protocol layers.
 * - not free the packet, and return zero. The packet will be matched
 *   against further PCBs and/or forwarded to another protocol layers.
 */
void
raw_recv(struct raw_pcb *pcb, raw_recv_fn recv, void *recv_arg)
{
  /* remember recv() callback and user data */
  pcb->recv = recv;
  pcb->recv_arg = recv_arg;
}

/**
 * @ingroup raw_raw
 * Send the raw IP packet to the given address. An IP header will be prepended
 * to the packet, unless the RAW_FLAGS_HDRINCL flag is set on the PCB. In that
 * case, the packet must include an IP header, which will then be sent as is.
 *
 * @param pcb the raw pcb which to send
 * @param p the IP payload to send
 * @param ipaddr the destination address of the IP packet
 *
 */
err_t
raw_sendto(struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *ipaddr)
{
  struct netif *netif;
  const ip_addr_t *src_ip;

  if ((pcb == NULL) || (ipaddr == NULL) || !IP_ADDR_PCB_VERSION_MATCH(pcb, ipaddr)) {
    return ERR_VAL;
  }

  LWIP_DEBUGF(RAW_DEBUG | LWIP_DBG_TRACE, ("raw_sendto\n"));

  if (pcb->netif_idx != NETIF_NO_INDEX) {
    netif = netif_get_by_index(pcb->netif_idx);
  } else {
#if LWIP_MULTICAST_TX_OPTIONS
    netif = NULL;
    if (ip_addr_ismulticast(ipaddr)) {
      /* For multicast-destined packets, use the user-provided interface index to
       * determine the outgoing interface, if an interface index is set and a
       * matching netif can be found. Otherwise, fall back to regular routing. */
      netif = netif_get_by_index(pcb->mcast_ifindex);
    }

    if (netif == NULL)
#endif /* LWIP_MULTICAST_TX_OPTIONS */
    {
      netif = ip_route(&pcb->local_ip, ipaddr);
    }
  }

  if (netif == NULL) {
    LWIP_DEBUGF(RAW_DEBUG | LWIP_DBG_LEVEL_WARNING, ("raw_sendto: No route to "));
    ip_addr_debug_print(RAW_DEBUG | LWIP_DBG_LEVEL_WARNING, ipaddr);
    return ERR_RTE;
  }

  if (ip_addr_isany(&pcb->local_ip) || ip_addr_ismulticast(&pcb->local_ip)) {
    /* use outgoing network interface IP address as source address */
    src_ip = ip_netif_get_local_ip(netif, ipaddr);
#if LWIP_IPV6
    if (src_ip == NULL) {
      return ERR_RTE;
    }
#endif /* LWIP_IPV6 */
  } else {
    /* use RAW PCB local IP address as source address */
    src_ip = &pcb->local_ip;
  }

  return raw_sendto_if_src(pcb, p, ipaddr, netif, src_ip);
}

/**
 * @ingroup raw_raw
 * Send the raw IP packet to the given address, using a particular outgoing
 * netif and source IP address. An IP header will be prepended to the packet,
 * unless the RAW_FLAGS_HDRINCL flag is set on the PCB. In that case, the
 * packet must include an IP header, which will then be sent as is.
 *
 * @param pcb RAW PCB used to send the data
 * @param p chain of pbufs to be sent
 * @param dst_ip destination IP address
 * @param netif the netif used for sending
 * @param src_ip source IP address
 */
err_t
raw_sendto_if_src(struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip,
                  struct netif *netif, const ip_addr_t *src_ip)
{
  err_t err;
  struct pbuf *q; /* q will be sent down the stack */
  u16_t header_size;
  u8_t ttl;

  if ((pcb == NULL) || (dst_ip == NULL) || (netif == NULL) || (src_ip == NULL) ||
      !IP_ADDR_PCB_VERSION_MATCH(pcb, src_ip) || !IP_ADDR_PCB_VERSION_MATCH(pcb, dst_ip)) {
    return ERR_VAL;
  }

  header_size = (
#if LWIP_IPV4 && LWIP_IPV6
                  IP_IS_V6(dst_ip) ? IP6_HLEN : IP_HLEN);
#elif LWIP_IPV4
                  IP_HLEN);
#else
                  IP6_HLEN);
#endif

  /* Handle the HDRINCL option as an exception: none of the code below applies
   * to this case, and sending the packet needs to be done differently too. */
  if (pcb->flags & RAW_FLAGS_HDRINCL) {
    /* A full header *must* be present in the first pbuf of the chain, as the
     * output routines may access its fields directly. */
    if (p->len < header_size) {
      return ERR_VAL;
    }
    /* @todo multicast loop support, if at all desired for this scenario.. */
    NETIF_SET_HINTS(netif, &pcb->netif_hints);
    err = ip_output_if_hdrincl(p, src_ip, dst_ip, netif);
    NETIF_RESET_HINTS(netif);
    return err;
  }

  /* packet too large to add an IP header without causing an overflow? */
  if ((u16_t)(p->tot_len + header_size) < p->tot_len) {
    return ERR_MEM;
  }
  /* not enough space to add an IP header to first pbuf in given p chain? */
  if (pbuf_add_header(p, header_size)) {
    /* allocate header in new pbuf */
    q = pbuf_alloc(PBUF_IP, 0, PBUF_RAM);
    /* new header pbuf could not be allocated? */
    if (q == NULL) {
      LWIP_DEBUGF(RAW_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("raw_sendto: could not allocate header\n"));
      return ERR_MEM;
    }
    if (p->tot_len != 0) {
      /* chain header q in front of given pbuf p */
      pbuf_chain(q, p);
    }
    /* { first pbuf q points to header pbuf } */
    LWIP_DEBUGF(RAW_DEBUG, ("raw_sendto: added header pbuf %p before given pbuf %p\n", (void *)q, (void *)p));
  } else {
    /* first pbuf q equals given pbuf */
    q = p;
    if (pbuf_remove_header(q, header_size)) {
      LWIP_ASSERT("Can't restore header we just removed!", 0);
      return ERR_MEM;
    }
  }

#if IP_SOF_BROADCAST
  if (IP_IS_V4(dst_ip)) {
    /* broadcast filter? */
    if (!ip_get_option(pcb, SOF_BROADCAST) && ip_addr_isbroadcast(dst_ip, netif)) {
      LWIP_DEBUGF(RAW_DEBUG | LWIP_DBG_LEVEL_WARNING, ("raw_sendto: SOF_BROADCAST not enabled on pcb %p\n", (void *)pcb));
      /* free any temporary header pbuf allocated by pbuf_header() */
      if (q != p) {
        pbuf_free(q);
      }
      return ERR_VAL;
    }
  }
#endif /* IP_SOF_BROADCAST */

  /* Multicast Loop? */
#if LWIP_MULTICAST_TX_OPTIONS
  if (((pcb->flags & RAW_FLAGS_MULTICAST_LOOP) != 0) && ip_addr_ismulticast(dst_ip)) {
    q->flags |= PBUF_FLAG_MCASTLOOP;
  }
#endif /* LWIP_MULTICAST_TX_OPTIONS */

#if LWIP_IPV6
  /* If requested, based on the IPV6_CHECKSUM socket option per RFC3542,
     compute the checksum and update the checksum in the payload. */
  if (IP_IS_V6(dst_ip) && pcb->chksum_reqd) {
    u16_t chksum = ip6_chksum_pseudo(p, pcb->protocol, p->tot_len, ip_2_ip6(src_ip), ip_2_ip6(dst_ip));
    LWIP_ASSERT("Checksum must fit into first pbuf", p->len >= (pcb->chksum_offset + 2));
    SMEMCPY(((u8_t *)p->payload) + pcb->chksum_offset, &chksum, sizeof(u16_t));
  }
#endif

  /* Determine TTL to use */
#if LWIP_MULTICAST_TX_OPTIONS
  ttl = (ip_addr_ismulticast(dst_ip) ? raw_get_multicast_ttl(pcb) : pcb->ttl);
#else /* LWIP_MULTICAST_TX_OPTIONS */
  ttl = pcb->ttl;
#endif /* LWIP_MULTICAST_TX_OPTIONS */

  NETIF_SET_HINTS(netif, &pcb->netif_hints);
  err = ip_output_if(q, src_ip, dst_ip, ttl, pcb->tos, pcb->protocol, netif);
  NETIF_RESET_HINTS(netif);

  /* did we chain a header earlier? */
  if (q != p) {
    /* free the header */
    pbuf_free(q);
  }
  return err;
}

/**
 * @ingroup raw_raw
 * Send the raw IP packet to the address given by raw_connect()
 *
 * @param pcb the raw pcb which to send
 * @param p the IP payload to send
 *
 */
err_t
raw_send(struct raw_pcb *pcb, struct pbuf *p)
{
  return raw_sendto(pcb, p, &pcb->remote_ip);
}

/**
 * @ingroup raw_raw
 * Remove an RAW PCB.
 *
 * @param pcb RAW PCB to be removed. The PCB is removed from the list of
 * RAW PCB's and the data structure is freed from memory.
 *
 * @see raw_new()
 */
void
raw_remove(struct raw_pcb *pcb)
{
  struct raw_pcb *pcb2;
  /* pcb to be removed is first in list? */
  if (raw_pcbs == pcb) {
    /* make list start at 2nd pcb */
    raw_pcbs = raw_pcbs->next;
    /* pcb not 1st in list */
  } else {
    for (pcb2 = raw_pcbs; pcb2 != NULL; pcb2 = pcb2->next) {
      /* find pcb in raw_pcbs list */
      if (pcb2->next != NULL && pcb2->next == pcb) {
        /* remove pcb from list */
        pcb2->next = pcb->next;
        break;
      }
    }
  }
  memp_free(MEMP_RAW_PCB, pcb);
}

/**
 * @ingroup raw_raw
 * Create a RAW PCB.
 *
 * @return The RAW PCB which was created. NULL if the PCB data structure
 * could not be allocated.
 *
 * @param proto the protocol number of the IPs payload (e.g. IP_PROTO_ICMP)
 *
 * @see raw_remove()
 */
struct raw_pcb *
raw_new(u8_t proto)
{
  struct raw_pcb *pcb;

  LWIP_DEBUGF(RAW_DEBUG | LWIP_DBG_TRACE, ("raw_new\n"));

  pcb = (struct raw_pcb *)memp_malloc(MEMP_RAW_PCB);
  /* could allocate RAW PCB? */
  if (pcb != NULL) {
    /* initialize PCB to all zeroes */
    memset(pcb, 0, sizeof(struct raw_pcb));
    pcb->protocol = proto;
    pcb->ttl = RAW_TTL;
#if LWIP_MULTICAST_TX_OPTIONS
    raw_set_multicast_ttl(pcb, RAW_TTL);
#endif /* LWIP_MULTICAST_TX_OPTIONS */
    pcb->next = raw_pcbs;
    raw_pcbs = pcb;
  }
  return pcb;
}

/**
 * @ingroup raw_raw
 * Create a RAW PCB for specific IP type.
 *
 * @return The RAW PCB which was created. NULL if the PCB data structure
 * could not be allocated.
 *
 * @param type IP address type, see @ref lwip_ip_addr_type definitions.
 * If you want to listen to IPv4 and IPv6 (dual-stack) packets,
 * supply @ref IPADDR_TYPE_ANY as argument and bind to @ref IP_ANY_TYPE.
 * @param proto the protocol number (next header) of the IPv6 packet payload
 *              (e.g. IP6_NEXTH_ICMP6)
 *
 * @see raw_remove()
 */
struct raw_pcb *
raw_new_ip_type(u8_t type, u8_t proto)
{
  struct raw_pcb *pcb;
  pcb = raw_new(proto);
#if LWIP_IPV4 && LWIP_IPV6
  if (pcb != NULL) {
    IP_SET_TYPE_VAL(pcb->local_ip,  type);
    IP_SET_TYPE_VAL(pcb->remote_ip, type);
  }
#else /* LWIP_IPV4 && LWIP_IPV6 */
  LWIP_UNUSED_ARG(type);
#endif /* LWIP_IPV4 && LWIP_IPV6 */
  return pcb;
}

/** This function is called from netif.c when address is changed
 *
 * @param old_addr IP address of the netif before change
 * @param new_addr IP address of the netif after change
 */
void raw_netif_ip_addr_changed(const ip_addr_t *old_addr, const ip_addr_t *new_addr)
{
  struct raw_pcb *rpcb;

  if (!ip_addr_isany(old_addr) && !ip_addr_isany(new_addr)) {
    for (rpcb = raw_pcbs; rpcb != NULL; rpcb = rpcb->next) {
      /* PCB bound to current local interface address? */
      if (ip_addr_cmp(&rpcb->local_ip, old_addr)) {
        /* The PCB is bound to the old ipaddr and
         * is set to bound to the new one instead */
        ip_addr_copy(rpcb->local_ip, *new_addr);
      }
    }
  }
}

#endif /* LWIP_RAW */
