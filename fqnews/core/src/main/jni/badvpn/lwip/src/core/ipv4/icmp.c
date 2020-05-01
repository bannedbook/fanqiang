/**
 * @file
 * ICMP - Internet Control Message Protocol
 *
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

/* Some ICMP messages should be passed to the transport protocols. This
   is not implemented. */

#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_ICMP /* don't build if not configured for use in lwipopts.h */

#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/def.h"
#include "lwip/stats.h"

#include <string.h>

#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif

/** Small optimization: set to 0 if incoming PBUF_POOL pbuf always can be
 * used to modify and send a response packet (and to 1 if this is not the case,
 * e.g. when link header is stripped off when receiving) */
#ifndef LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN
#define LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN 1
#endif /* LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN */

/* The amount of data from the original packet to return in a dest-unreachable */
#define ICMP_DEST_UNREACH_DATASIZE 8

static void icmp_send_response(struct pbuf *p, u8_t type, u8_t code);

/**
 * Processes ICMP input packets, called from ip_input().
 *
 * Currently only processes icmp echo requests and sends
 * out the echo response.
 *
 * @param p the icmp echo request packet, p->payload pointing to the icmp header
 * @param inp the netif on which this packet was received
 */
void
icmp_input(struct pbuf *p, struct netif *inp)
{
  u8_t type;
#ifdef LWIP_DEBUG
  u8_t code;
#endif /* LWIP_DEBUG */
  struct icmp_echo_hdr *iecho;
  const struct ip_hdr *iphdr_in;
  u16_t hlen;
  const ip4_addr_t *src;

  ICMP_STATS_INC(icmp.recv);
  MIB2_STATS_INC(mib2.icmpinmsgs);

  iphdr_in = ip4_current_header();
  hlen = IPH_HL_BYTES(iphdr_in);
  if (hlen < IP_HLEN) {
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: short IP header (%"S16_F" bytes) received\n", hlen));
    goto lenerr;
  }
  if (p->len < sizeof(u16_t) * 2) {
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: short ICMP (%"U16_F" bytes) received\n", p->tot_len));
    goto lenerr;
  }

  type = *((u8_t *)p->payload);
#ifdef LWIP_DEBUG
  code = *(((u8_t *)p->payload) + 1);
  /* if debug is enabled but debug statement below is somehow disabled: */
  LWIP_UNUSED_ARG(code);
#endif /* LWIP_DEBUG */
  switch (type) {
    case ICMP_ER:
      /* This is OK, echo reply might have been parsed by a raw PCB
         (as obviously, an echo request has been sent, too). */
      MIB2_STATS_INC(mib2.icmpinechoreps);
      break;
    case ICMP_ECHO:
      MIB2_STATS_INC(mib2.icmpinechos);
      src = ip4_current_dest_addr();
      /* multicast destination address? */
      if (ip4_addr_ismulticast(ip4_current_dest_addr())) {
#if LWIP_MULTICAST_PING
        /* For multicast, use address of receiving interface as source address */
        src = netif_ip4_addr(inp);
#else /* LWIP_MULTICAST_PING */
        LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: Not echoing to multicast pings\n"));
        goto icmperr;
#endif /* LWIP_MULTICAST_PING */
      }
      /* broadcast destination address? */
      if (ip4_addr_isbroadcast(ip4_current_dest_addr(), ip_current_netif())) {
#if LWIP_BROADCAST_PING
        /* For broadcast, use address of receiving interface as source address */
        src = netif_ip4_addr(inp);
#else /* LWIP_BROADCAST_PING */
        LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: Not echoing to broadcast pings\n"));
        goto icmperr;
#endif /* LWIP_BROADCAST_PING */
      }
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ping\n"));
      if (p->tot_len < sizeof(struct icmp_echo_hdr)) {
        LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: bad ICMP echo received\n"));
        goto lenerr;
      }
#if CHECKSUM_CHECK_ICMP
      IF__NETIF_CHECKSUM_ENABLED(inp, NETIF_CHECKSUM_CHECK_ICMP) {
        if (inet_chksum_pbuf(p) != 0) {
          LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: checksum failed for received ICMP echo\n"));
          pbuf_free(p);
          ICMP_STATS_INC(icmp.chkerr);
          MIB2_STATS_INC(mib2.icmpinerrors);
          return;
        }
      }
#endif
#if LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN
      if (pbuf_add_header(p, hlen + PBUF_LINK_HLEN + PBUF_LINK_ENCAPSULATION_HLEN)) {
        /* p is not big enough to contain link headers
         * allocate a new one and copy p into it
         */
        struct pbuf *r;
        u16_t alloc_len = (u16_t)(p->tot_len + hlen);
        if (alloc_len < p->tot_len) {
          LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: allocating new pbuf failed (tot_len overflow)\n"));
          goto icmperr;
        }
        /* allocate new packet buffer with space for link headers */
        r = pbuf_alloc(PBUF_LINK, alloc_len, PBUF_RAM);
        if (r == NULL) {
          LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: allocating new pbuf failed\n"));
          goto icmperr;
        }
        if (r->len < hlen + sizeof(struct icmp_echo_hdr)) {
          LWIP_DEBUGF(ICMP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("first pbuf cannot hold the ICMP header"));
          pbuf_free(r);
          goto icmperr;
        }
        /* copy the ip header */
        MEMCPY(r->payload, iphdr_in, hlen);
        /* switch r->payload back to icmp header (cannot fail) */
        if (pbuf_remove_header(r, hlen)) {
          LWIP_ASSERT("icmp_input: moving r->payload to icmp header failed\n", 0);
          pbuf_free(r);
          goto icmperr;
        }
        /* copy the rest of the packet without ip header */
        if (pbuf_copy(r, p) != ERR_OK) {
          LWIP_DEBUGF(ICMP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("icmp_input: copying to new pbuf failed"));
          pbuf_free(r);
          goto icmperr;
        }
        /* free the original p */
        pbuf_free(p);
        /* we now have an identical copy of p that has room for link headers */
        p = r;
      } else {
        /* restore p->payload to point to icmp header (cannot fail) */
        if (pbuf_remove_header(p, hlen + PBUF_LINK_HLEN + PBUF_LINK_ENCAPSULATION_HLEN)) {
          LWIP_ASSERT("icmp_input: restoring original p->payload failed\n", 0);
          goto icmperr;
        }
      }
#endif /* LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN */
      /* At this point, all checks are OK. */
      /* We generate an answer by switching the dest and src ip addresses,
       * setting the icmp type to ECHO_RESPONSE and updating the checksum. */
      iecho = (struct icmp_echo_hdr *)p->payload;
      if (pbuf_add_header(p, hlen)) {
        LWIP_DEBUGF(ICMP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("Can't move over header in packet"));
      } else {
        err_t ret;
        struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
        ip4_addr_copy(iphdr->src, *src);
        ip4_addr_copy(iphdr->dest, *ip4_current_src_addr());
        ICMPH_TYPE_SET(iecho, ICMP_ER);
#if CHECKSUM_GEN_ICMP
        IF__NETIF_CHECKSUM_ENABLED(inp, NETIF_CHECKSUM_GEN_ICMP) {
          /* adjust the checksum */
          if (iecho->chksum > PP_HTONS(0xffffU - (ICMP_ECHO << 8))) {
            iecho->chksum = (u16_t)(iecho->chksum + PP_HTONS((u16_t)(ICMP_ECHO << 8)) + 1);
          } else {
            iecho->chksum = (u16_t)(iecho->chksum + PP_HTONS(ICMP_ECHO << 8));
          }
        }
#if LWIP_CHECKSUM_CTRL_PER_NETIF
        else {
          iecho->chksum = 0;
        }
#endif /* LWIP_CHECKSUM_CTRL_PER_NETIF */
#else /* CHECKSUM_GEN_ICMP */
        iecho->chksum = 0;
#endif /* CHECKSUM_GEN_ICMP */

        /* Set the correct TTL and recalculate the header checksum. */
        IPH_TTL_SET(iphdr, ICMP_TTL);
        IPH_CHKSUM_SET(iphdr, 0);
#if CHECKSUM_GEN_IP
        IF__NETIF_CHECKSUM_ENABLED(inp, NETIF_CHECKSUM_GEN_IP) {
          IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, hlen));
        }
#endif /* CHECKSUM_GEN_IP */

        ICMP_STATS_INC(icmp.xmit);
        /* increase number of messages attempted to send */
        MIB2_STATS_INC(mib2.icmpoutmsgs);
        /* increase number of echo replies attempted to send */
        MIB2_STATS_INC(mib2.icmpoutechoreps);

        /* send an ICMP packet */
        ret = ip4_output_if(p, src, LWIP_IP_HDRINCL,
                            ICMP_TTL, 0, IP_PROTO_ICMP, inp);
        if (ret != ERR_OK) {
          LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ip_output_if returned an error: %s\n", lwip_strerr(ret)));
        }
      }
      break;
    default:
      if (type == ICMP_DUR) {
        MIB2_STATS_INC(mib2.icmpindestunreachs);
      } else if (type == ICMP_TE) {
        MIB2_STATS_INC(mib2.icmpintimeexcds);
      } else if (type == ICMP_PP) {
        MIB2_STATS_INC(mib2.icmpinparmprobs);
      } else if (type == ICMP_SQ) {
        MIB2_STATS_INC(mib2.icmpinsrcquenchs);
      } else if (type == ICMP_RD) {
        MIB2_STATS_INC(mib2.icmpinredirects);
      } else if (type == ICMP_TS) {
        MIB2_STATS_INC(mib2.icmpintimestamps);
      } else if (type == ICMP_TSR) {
        MIB2_STATS_INC(mib2.icmpintimestampreps);
      } else if (type == ICMP_AM) {
        MIB2_STATS_INC(mib2.icmpinaddrmasks);
      } else if (type == ICMP_AMR) {
        MIB2_STATS_INC(mib2.icmpinaddrmaskreps);
      }
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ICMP type %"S16_F" code %"S16_F" not supported.\n",
                               (s16_t)type, (s16_t)code));
      ICMP_STATS_INC(icmp.proterr);
      ICMP_STATS_INC(icmp.drop);
  }
  pbuf_free(p);
  return;
lenerr:
  pbuf_free(p);
  ICMP_STATS_INC(icmp.lenerr);
  MIB2_STATS_INC(mib2.icmpinerrors);
  return;
#if LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN || !LWIP_MULTICAST_PING || !LWIP_BROADCAST_PING
icmperr:
  pbuf_free(p);
  ICMP_STATS_INC(icmp.err);
  MIB2_STATS_INC(mib2.icmpinerrors);
  return;
#endif /* LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN || !LWIP_MULTICAST_PING || !LWIP_BROADCAST_PING */
}

/**
 * Send an icmp 'destination unreachable' packet, called from ip_input() if
 * the transport layer protocol is unknown and from udp_input() if the local
 * port is not bound.
 *
 * @param p the input packet for which the 'unreachable' should be sent,
 *          p->payload pointing to the IP header
 * @param t type of the 'unreachable' packet
 */
void
icmp_dest_unreach(struct pbuf *p, enum icmp_dur_type t)
{
  MIB2_STATS_INC(mib2.icmpoutdestunreachs);
  icmp_send_response(p, ICMP_DUR, t);
}

#if IP_FORWARD || IP_REASSEMBLY
/**
 * Send a 'time exceeded' packet, called from ip_forward() if TTL is 0.
 *
 * @param p the input packet for which the 'time exceeded' should be sent,
 *          p->payload pointing to the IP header
 * @param t type of the 'time exceeded' packet
 */
void
icmp_time_exceeded(struct pbuf *p, enum icmp_te_type t)
{
  MIB2_STATS_INC(mib2.icmpouttimeexcds);
  icmp_send_response(p, ICMP_TE, t);
}

#endif /* IP_FORWARD || IP_REASSEMBLY */

/**
 * Send an icmp packet in response to an incoming packet.
 *
 * @param p the input packet for which the 'unreachable' should be sent,
 *          p->payload pointing to the IP header
 * @param type Type of the ICMP header
 * @param code Code of the ICMP header
 */
static void
icmp_send_response(struct pbuf *p, u8_t type, u8_t code)
{
  struct pbuf *q;
  struct ip_hdr *iphdr;
  /* we can use the echo header here */
  struct icmp_echo_hdr *icmphdr;
  ip4_addr_t iphdr_src;
  struct netif *netif;

  /* increase number of messages attempted to send */
  MIB2_STATS_INC(mib2.icmpoutmsgs);

  /* ICMP header + IP header + 8 bytes of data */
  q = pbuf_alloc(PBUF_IP, sizeof(struct icmp_echo_hdr) + IP_HLEN + ICMP_DEST_UNREACH_DATASIZE,
                 PBUF_RAM);
  if (q == NULL) {
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_time_exceeded: failed to allocate pbuf for ICMP packet.\n"));
    MIB2_STATS_INC(mib2.icmpouterrors);
    return;
  }
  LWIP_ASSERT("check that first pbuf can hold icmp message",
              (q->len >= (sizeof(struct icmp_echo_hdr) + IP_HLEN + ICMP_DEST_UNREACH_DATASIZE)));

  iphdr = (struct ip_hdr *)p->payload;
  LWIP_DEBUGF(ICMP_DEBUG, ("icmp_time_exceeded from "));
  ip4_addr_debug_print_val(ICMP_DEBUG, iphdr->src);
  LWIP_DEBUGF(ICMP_DEBUG, (" to "));
  ip4_addr_debug_print_val(ICMP_DEBUG, iphdr->dest);
  LWIP_DEBUGF(ICMP_DEBUG, ("\n"));

  icmphdr = (struct icmp_echo_hdr *)q->payload;
  icmphdr->type = type;
  icmphdr->code = code;
  icmphdr->id = 0;
  icmphdr->seqno = 0;

  /* copy fields from original packet */
  SMEMCPY((u8_t *)q->payload + sizeof(struct icmp_echo_hdr), (u8_t *)p->payload,
          IP_HLEN + ICMP_DEST_UNREACH_DATASIZE);

  ip4_addr_copy(iphdr_src, iphdr->src);
#ifdef LWIP_HOOK_IP4_ROUTE_SRC
  {
    ip4_addr_t iphdr_dst;
    ip4_addr_copy(iphdr_dst, iphdr->dest);
    netif = ip4_route_src(&iphdr_src, &iphdr_dst);
  }
#else
  netif = ip4_route(&iphdr_src);
#endif
  if (netif != NULL) {
    /* calculate checksum */
    icmphdr->chksum = 0;
#if CHECKSUM_GEN_ICMP
    IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_ICMP) {
      icmphdr->chksum = inet_chksum(icmphdr, q->len);
    }
#endif
    ICMP_STATS_INC(icmp.xmit);
    ip4_output_if(q, NULL, &iphdr_src, ICMP_TTL, 0, IP_PROTO_ICMP, netif);
  }
  pbuf_free(q);
}

#endif /* LWIP_IPV4 && LWIP_ICMP */
