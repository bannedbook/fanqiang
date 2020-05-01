/**
  * @file
 *
 * 6LowPAN output for IPv6. Uses ND tables for link-layer addressing. Fragments packets to 6LowPAN units.
 */

/*
 * Copyright (c) 2015 Inico Technologies Ltd.
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

/**
 * @defgroup sixlowpan 6LowPAN
 * @ingroup netifs
 * 6LowPAN netif implementation
 */

#include "netif/lowpan6.h"

#if LWIP_IPV6 && LWIP_6LOWPAN

#include "lwip/ip.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/nd6.h"
#include "lwip/mem.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "lwip/snmp.h"

#include <string.h>

struct ieee_802154_addr {
  u8_t addr_len;
  u8_t addr[8];
};

/** This is a helper struct.
 */
struct lowpan6_reass_helper {
  struct pbuf *pbuf;
  struct lowpan6_reass_helper *next_packet;
  u8_t timer;
  struct ieee_802154_addr sender_addr;
  u16_t datagram_size;
  u16_t datagram_tag;
};

static struct lowpan6_reass_helper * reass_list;

#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
static ip6_addr_t lowpan6_context[LWIP_6LOWPAN_NUM_CONTEXTS];
#endif

static u16_t ieee_802154_pan_id;

static const struct ieee_802154_addr ieee_802154_broadcast = {2, {0xff, 0xff}};

#if LWIP_6LOWPAN_INFER_SHORT_ADDRESS
static struct ieee_802154_addr short_mac_addr = {2, {0,0}};
#endif /* LWIP_6LOWPAN_INFER_SHORT_ADDRESS */

static err_t dequeue_datagram(struct lowpan6_reass_helper *lrh);

/**
 * Periodic timer for 6LowPAN functions:
 *
 * - Remove incomplete/old packets
 */
void
lowpan6_tmr(void)
{
  struct lowpan6_reass_helper *lrh, *lrh_temp;

  lrh = reass_list;
  while (lrh != NULL) {
    lrh_temp = lrh->next_packet;
    if ((--lrh->timer) == 0) {
      dequeue_datagram(lrh);
      pbuf_free(lrh->pbuf);
      mem_free(lrh);
    }
    lrh = lrh_temp;
  }
}

/**
 * Removes a datagram from the reassembly queue.
 **/
static err_t
dequeue_datagram(struct lowpan6_reass_helper *lrh)
{
  struct lowpan6_reass_helper *lrh_temp;

  if (reass_list == lrh) {
    reass_list = reass_list->next_packet;
  } else {
    lrh_temp = reass_list;
    while (lrh_temp != NULL) {
      if (lrh_temp->next_packet == lrh) {
        lrh_temp->next_packet = lrh->next_packet;
        break;
      }
      lrh_temp = lrh_temp->next_packet;
    }
  }

  return ERR_OK;
}

static s8_t
lowpan6_context_lookup(const ip6_addr_t *ip6addr)
{
  s8_t i;

  for (i = 0; i < LWIP_6LOWPAN_NUM_CONTEXTS; i++) {
    if (ip6_addr_netcmp(&lowpan6_context[i], ip6addr)) {
      return i;
    }
  }

  return -1;
}

/* Determine compression mode for unicast address. */
static s8_t
lowpan6_get_address_mode(const ip6_addr_t *ip6addr, const struct ieee_802154_addr *mac_addr)
{
  if (mac_addr->addr_len == 2) {
    if ((ip6addr->addr[2] == (u32_t)PP_HTONL(0x000000ff)) &&
      ((ip6addr->addr[3]  & PP_HTONL(0xffff0000)) == PP_NTOHL(0xfe000000))) {
      if ((ip6addr->addr[3]  & PP_HTONL(0x0000ffff)) == lwip_ntohl((mac_addr->addr[0] << 8) | mac_addr->addr[1])) {
        return 3;
      }
    }
  } else if (mac_addr->addr_len == 8) {
    if ((ip6addr->addr[2] == lwip_ntohl(((mac_addr->addr[0] ^ 2) << 24) | (mac_addr->addr[1] << 16) | mac_addr->addr[2] << 8 | mac_addr->addr[3])) &&
      (ip6addr->addr[3] == lwip_ntohl((mac_addr->addr[4] << 24) | (mac_addr->addr[5] << 16) | mac_addr->addr[6] << 8 | mac_addr->addr[7]))) {
      return 3;
    }
  }

  if ((ip6addr->addr[2] == PP_HTONL(0x000000ffUL)) &&
    ((ip6addr->addr[3]  & PP_HTONL(0xffff0000)) == PP_NTOHL(0xfe000000UL))) {
    return 2;
  }

  return 1;
}

/* Determine compression mode for multicast address. */
static s8_t
lowpan6_get_address_mode_mc(const ip6_addr_t *ip6addr)
{
  if ((ip6addr->addr[0] == PP_HTONL(0xff020000)) &&
      (ip6addr->addr[1] == 0) &&
      (ip6addr->addr[2] == 0) &&
      ((ip6addr->addr[3]  & PP_HTONL(0xffffff00)) == 0)) {
    return 3;
  } else if (((ip6addr->addr[0] & PP_HTONL(0xff00ffff)) == PP_HTONL(0xff000000)) &&
              (ip6addr->addr[1] == 0)) {
    if ((ip6addr->addr[2] == 0) &&
        ((ip6addr->addr[3]  & PP_HTONL(0xff000000)) == 0)) {
      return 2;
    } else if ((ip6addr->addr[2]  & PP_HTONL(0xffffff00)) == 0) {
      return 1;
    }
  }

  return 0;
}

/*
 * Encapsulates data into IEEE 802.15.4 frames.
 * Fragments an IPv6 datagram into 6LowPAN units, which fit into IEEE 802.15.4 frames.
 * If configured, will compress IPv6 and or UDP headers.
 * */
static err_t
lowpan6_frag(struct netif *netif, struct pbuf *p, const struct ieee_802154_addr *src, const struct ieee_802154_addr *dst)
{
  struct pbuf * p_frag;
  u16_t frag_len, remaining_len;
  u8_t * buffer;
  u8_t ieee_header_len;
  u8_t lowpan6_header_len;
  s8_t i;
  static u8_t frame_seq_num;
  static u16_t datagram_tag;
  u16_t datagram_offset;
  err_t err = ERR_IF;

  /* We'll use a dedicated pbuf for building 6LowPAN fragments. */
  p_frag = pbuf_alloc(PBUF_RAW, 127, PBUF_RAM);
  if (p_frag == NULL) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    return ERR_MEM;
  }

  /* Write IEEE 802.15.4 header. */
  buffer  = (u8_t*)p_frag->payload;
  ieee_header_len = 0;
  if (dst == &ieee_802154_broadcast) {
    buffer[ieee_header_len++] = 0x01; /* data packet, no ack required. */
  } else {
    buffer[ieee_header_len++] = 0x21; /* data packet, ack required. */
  }
  buffer[ieee_header_len] = (0x00 << 4); /* 2003 frame version */
  buffer[ieee_header_len] |= (dst->addr_len == 2) ? (0x02 << 2) : (0x03 << 2); /* destination addressing mode  */
  buffer[ieee_header_len] |= (src->addr_len == 2) ? (0x02 << 6) : (0x03 << 6); /* source addressing mode */
  ieee_header_len++;
  buffer[ieee_header_len++] = frame_seq_num++;

  buffer[ieee_header_len++] = ieee_802154_pan_id & 0xff; /* pan id */
  buffer[ieee_header_len++] = (ieee_802154_pan_id >> 8) & 0xff; /* pan id */
  i = dst->addr_len;
  while (i-- > 0) {
    buffer[ieee_header_len++] = dst->addr[i];
  }

  buffer[ieee_header_len++] = ieee_802154_pan_id & 0xff; /* pan id */
  buffer[ieee_header_len++] = (ieee_802154_pan_id >> 8) & 0xff; /* pan id */
  i = src->addr_len;
  while (i-- > 0) {
    buffer[ieee_header_len++] = src->addr[i];
  }

#if LWIP_6LOWPAN_IPHC
  /* Perform 6LowPAN IPv6 header compression according to RFC 6282 */
  {
    struct ip6_hdr *ip6hdr;

    /* Point to ip6 header and align copies of src/dest addresses. */
    ip6hdr = (struct ip6_hdr *)p->payload;
    ip_addr_copy_from_ip6_packed(ip_data.current_iphdr_dest, ip6hdr->dest);
    ip6_addr_assign_zone(ip_2_ip6(&ip_data.current_iphdr_dest), IP6_UNKNOWN, netif);
    ip_addr_copy_from_ip6_packed(ip_data.current_iphdr_src, ip6hdr->src);
    ip6_addr_assign_zone(ip_2_ip6(&ip_data.current_iphdr_src), IP6_UNKNOWN, netif);

    /* Basic length of 6LowPAN header, set dispatch and clear fields. */
    lowpan6_header_len = 2;
    buffer[ieee_header_len] = 0x60;
    buffer[ieee_header_len + 1] = 0;

    /* Determine whether there will be a Context Identifier Extension byte or not.
    * If so, set it already. */
#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
    buffer[ieee_header_len + 2] = 0;

    i = lowpan6_context_lookup(ip_2_ip6(&ip_data.current_iphdr_src));
    if (i >= 0) {
      /* Stateful source address compression. */
      buffer[ieee_header_len + 1] |= 0x40;
      buffer[ieee_header_len + 2] |= (i & 0x0f) << 4;
    }

    i = lowpan6_context_lookup(ip_2_ip6(&ip_data.current_iphdr_dest));
    if (i >= 0) {
      /* Stateful destination address compression. */
      buffer[ieee_header_len + 1] |= 0x04;
      buffer[ieee_header_len + 2] |= i & 0x0f;
    }

    if (buffer[ieee_header_len + 2] != 0x00) {
      /* Context identifier extension byte is appended. */
      buffer[ieee_header_len + 1] |= 0x80;
      lowpan6_header_len++;
    }
#endif /* LWIP_6LOWPAN_NUM_CONTEXTS > 0 */

    /* Determine TF field: Traffic Class, Flow Label */
    if (IP6H_FL(ip6hdr) == 0) {
      /* Flow label is elided. */
      buffer[ieee_header_len] |= 0x10;
      if (IP6H_TC(ip6hdr) == 0) {
        /* Traffic class (ECN+DSCP) elided too. */
        buffer[ieee_header_len] |= 0x08;
      } else {
        /* Traffic class (ECN+DSCP) appended. */
        buffer[ieee_header_len + lowpan6_header_len++] = IP6H_TC(ip6hdr);
      }
    } else {
      if (((IP6H_TC(ip6hdr) & 0x3f) == 0)) {
        /* DSCP portion of Traffic Class is elided, ECN and FL are appended (3 bytes) */
        buffer[ieee_header_len] |= 0x08;

        buffer[ieee_header_len + lowpan6_header_len] = IP6H_TC(ip6hdr) & 0xc0;
        buffer[ieee_header_len + lowpan6_header_len++] |= (IP6H_FL(ip6hdr) >> 16) & 0x0f;
        buffer[ieee_header_len + lowpan6_header_len++] = (IP6H_FL(ip6hdr) >> 8) & 0xff;
        buffer[ieee_header_len + lowpan6_header_len++] = IP6H_FL(ip6hdr) & 0xff;
      } else {
        /* Traffic class and flow label are appended (4 bytes) */
        buffer[ieee_header_len + lowpan6_header_len++] = IP6H_TC(ip6hdr);
        buffer[ieee_header_len + lowpan6_header_len++] = (IP6H_FL(ip6hdr) >> 16) & 0x0f;
        buffer[ieee_header_len + lowpan6_header_len++] = (IP6H_FL(ip6hdr) >> 8) & 0xff;
        buffer[ieee_header_len + lowpan6_header_len++] = IP6H_FL(ip6hdr) & 0xff;
      }
    }

    /* Compress NH?
    * Only if UDP for now. @todo support other NH compression. */
    if (IP6H_NEXTH(ip6hdr) == IP6_NEXTH_UDP) {
      buffer[ieee_header_len] |= 0x04;
    } else {
      /* append nexth. */
      buffer[ieee_header_len + lowpan6_header_len++] = IP6H_NEXTH(ip6hdr);
    }

    /* Compress hop limit? */
    if (IP6H_HOPLIM(ip6hdr) == 255) {
      buffer[ieee_header_len] |= 0x03;
    } else if (IP6H_HOPLIM(ip6hdr) == 64) {
      buffer[ieee_header_len] |= 0x02;
    } else if (IP6H_HOPLIM(ip6hdr) == 1) {
      buffer[ieee_header_len] |= 0x01;
    } else {
      /* append hop limit */
      buffer[ieee_header_len + lowpan6_header_len++] = IP6H_HOPLIM(ip6hdr);
    }

    /* Compress source address */
    if (((buffer[ieee_header_len + 1] & 0x40) != 0) ||
        (ip6_addr_islinklocal(ip_2_ip6(&ip_data.current_iphdr_src)))) {
      /* Context-based or link-local source address compression. */
      i = lowpan6_get_address_mode(ip_2_ip6(&ip_data.current_iphdr_src), src);
      buffer[ieee_header_len + 1] |= (i & 0x03) << 4;
      if (i == 1) {
        MEMCPY(buffer + ieee_header_len + lowpan6_header_len, (u8_t*)p->payload + 16, 8);
        lowpan6_header_len += 8;
      } else if (i == 2) {
        MEMCPY(buffer + ieee_header_len + lowpan6_header_len, (u8_t*)p->payload + 22, 2);
        lowpan6_header_len += 2;
      }
    } else if (ip6_addr_isany(ip_2_ip6(&ip_data.current_iphdr_src))) {
      /* Special case: mark SAC and leave SAM=0 */
      buffer[ieee_header_len + 1] |= 0x40;
    } else {
      /* Append full address. */
      MEMCPY(buffer + ieee_header_len + lowpan6_header_len, (u8_t*)p->payload + 8, 16);
      lowpan6_header_len += 16;
    }

    /* Compress destination address */
    if (ip6_addr_ismulticast(ip_2_ip6(&ip_data.current_iphdr_dest))) {
      /* @todo support stateful multicast address compression */

      buffer[ieee_header_len + 1] |= 0x08;

      i = lowpan6_get_address_mode_mc(ip_2_ip6(&ip_data.current_iphdr_dest));
      buffer[ieee_header_len + 1] |= i & 0x03;
      if (i == 0) {
        MEMCPY(buffer + ieee_header_len + lowpan6_header_len, (u8_t*)p->payload + 24, 16);
        lowpan6_header_len += 16;
      } else if (i == 1) {
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[25];
        MEMCPY(buffer + ieee_header_len + lowpan6_header_len, (u8_t*)p->payload + 35, 5);
        lowpan6_header_len += 5;
      } else if (i == 2) {
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[25];
        MEMCPY(buffer + ieee_header_len + lowpan6_header_len, (u8_t*)p->payload + 37, 3);
        lowpan6_header_len += 3;
      } else if (i == 3) {
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[39];
      }
    } else if (((buffer[ieee_header_len + 1] & 0x04) != 0) ||
               (ip6_addr_islinklocal(ip_2_ip6(&ip_data.current_iphdr_dest)))) {
      /* Context-based or link-local destination address compression. */
      i = lowpan6_get_address_mode(ip_2_ip6(&ip_data.current_iphdr_dest), dst);
      buffer[ieee_header_len + 1] |= i & 0x03;
      if (i == 1) {
        MEMCPY(buffer + ieee_header_len + lowpan6_header_len, (u8_t*)p->payload + 32, 8);
        lowpan6_header_len += 8;
      } else if (i == 2) {
        MEMCPY(buffer + ieee_header_len + lowpan6_header_len, (u8_t*)p->payload + 38, 2);
        lowpan6_header_len += 2;
      }
    } else {
      /* Append full address. */
      MEMCPY(buffer + ieee_header_len + lowpan6_header_len, (u8_t*)p->payload + 24, 16);
      lowpan6_header_len += 16;
    }

    /* Move to payload. */
    pbuf_remove_header(p, IP6_HLEN);

#if LWIP_UDP
    /* Compress UDP header? */
    if (IP6H_NEXTH(ip6hdr) == IP6_NEXTH_UDP) {
      /* @todo support optional checksum compression */

      buffer[ieee_header_len + lowpan6_header_len] = 0xf0;

      /* determine port compression mode. */
      if ((((u8_t *)p->payload)[0] == 0xf0) && ((((u8_t *)p->payload)[1] & 0xf0) == 0xb0) &&
          (((u8_t *)p->payload)[2] == 0xf0) && ((((u8_t *)p->payload)[3] & 0xf0) == 0xb0)) {
        /* Compress source and dest ports. */
        buffer[ieee_header_len + lowpan6_header_len++] |= 0x03;
        buffer[ieee_header_len + lowpan6_header_len++] = ((((u8_t *)p->payload)[1] & 0x0f) << 4) | (((u8_t *)p->payload)[3] & 0x0f);
      } else if (((u8_t *)p->payload)[0] == 0xf0) {
        /* Compress source port. */
        buffer[ieee_header_len + lowpan6_header_len++] |= 0x02;
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[1];
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[2];
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[3];
      } else if (((u8_t *)p->payload)[2] == 0xf0) {
        /* Compress dest port. */
        buffer[ieee_header_len + lowpan6_header_len++] |= 0x01;
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[0];
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[1];
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[3];
      } else {
        /* append full ports. */
        lowpan6_header_len++;
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[0];
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[1];
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[2];
        buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[3];
      }

      /* elide length and copy checksum */
      buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[6];
      buffer[ieee_header_len + lowpan6_header_len++] = ((u8_t *)p->payload)[7];

      pbuf_remove_header(p, UDP_HLEN);
    }
#endif /* LWIP_UDP */
  }

#else /* LWIP_6LOWPAN_HC */
  /* Send uncompressed IPv6 header with appropriate dispatch byte. */
  lowpan6_header_len = 1;
  buffer[ieee_header_len] = 0x41; /* IPv6 dispatch */
#endif /* LWIP_6LOWPAN_HC */

  /* Calculate remaining packet length */
  remaining_len = p->tot_len;

  if (remaining_len > 0x7FF) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    /* datagram_size must fit into 11 bit */
    pbuf_free(p_frag);
    return ERR_VAL;
  }

  /* Fragment, or 1 packet? */
  if (remaining_len > (127 - ieee_header_len - lowpan6_header_len - 3)) { /* 127 - header - 1 byte dispatch - 2 bytes CRC */
    /* We must move the 6LowPAN header to make room for the FRAG header. */
    i = lowpan6_header_len;
    while (i-- != 0) {
      buffer[ieee_header_len + i + 4] = buffer[ieee_header_len + i];
    }

    /* Now we need to fragment the packet. FRAG1 header first */
    buffer[ieee_header_len] = 0xc0 | (((p->tot_len + lowpan6_header_len) >> 8) & 0x7);
    buffer[ieee_header_len + 1] = (p->tot_len + lowpan6_header_len) & 0xff;

    datagram_tag++;
    buffer[ieee_header_len + 2] = datagram_tag & 0xff;
    buffer[ieee_header_len + 3] = (datagram_tag >> 8) & 0xff;

    /* Fragment follows. */
    frag_len = (127 - ieee_header_len - 4 - 2) & 0xf8;

    pbuf_copy_partial(p, buffer + ieee_header_len + lowpan6_header_len + 4, frag_len - lowpan6_header_len, 0);
    remaining_len -= frag_len - lowpan6_header_len;
    datagram_offset = frag_len;

    /* 2 bytes CRC */
#if LWIP_6LOWPAN_HW_CRC
    /* Leave blank, will be filled by HW. */
#else /* LWIP_6LOWPAN_HW_CRC */
    /* @todo calculate CRC */
#endif /* LWIP_6LOWPAN_HW_CRC */

    /* Calculate frame length */
    p_frag->len = p_frag->tot_len = ieee_header_len + 4 + frag_len + 2; /* add 2 dummy bytes for crc*/

    /* send the packet */
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p_frag->tot_len);
    LWIP_DEBUGF(LOWPAN6_DEBUG | LWIP_DBG_TRACE, ("lowpan6_send: sending packet %p\n", (void *)p));
    err = netif->linkoutput(netif, p_frag);

    while ((remaining_len > 0) && (err == ERR_OK)) {
      /* new frame, new seq num for ACK */
      buffer[2] = frame_seq_num++;

      buffer[ieee_header_len] |= 0x20; /* Change FRAG1 to FRAGN */

      buffer[ieee_header_len + 4] = (u8_t)(datagram_offset >> 3); /* datagram offset in FRAGN header (datagram_offset is max. 11 bit) */

      frag_len = (127 - ieee_header_len - 5 - 2) & 0xf8;
      if (frag_len > remaining_len) {
        frag_len = remaining_len;
      }

      pbuf_copy_partial(p, buffer + ieee_header_len + 5, frag_len, p->tot_len - remaining_len);
      remaining_len -= frag_len;
      datagram_offset += frag_len;

      /* 2 bytes CRC */
#if LWIP_6LOWPAN_HW_CRC
      /* Leave blank, will be filled by HW. */
#else /* LWIP_6LOWPAN_HW_CRC */
      /* @todo calculate CRC */
#endif /* LWIP_6LOWPAN_HW_CRC */

      /* Calculate frame length */
      p_frag->len = p_frag->tot_len = frag_len + 5 + ieee_header_len + 2;

      /* send the packet */
      MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p_frag->tot_len);
      LWIP_DEBUGF(LOWPAN6_DEBUG | LWIP_DBG_TRACE, ("lowpan6_send: sending packet %p\n", (void *)p));
      err = netif->linkoutput(netif, p_frag);
    }
  } else {
    /* It fits in one frame. */
    frag_len = remaining_len;

    /* Copy IPv6 packet */
    pbuf_copy_partial(p, buffer + ieee_header_len + lowpan6_header_len, frag_len, 0);
    remaining_len = 0;

    /* 2 bytes CRC */
#if LWIP_6LOWPAN_HW_CRC
    /* Leave blank, will be filled by HW. */
#else /* LWIP_6LOWPAN_HW_CRC */
    /* @todo calculate CRC */
#endif /* LWIP_6LOWPAN_HW_CRC */

    /* Calculate frame length */
    p_frag->len = p_frag->tot_len = frag_len + lowpan6_header_len + ieee_header_len + 2;

    /* send the packet */
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p_frag->tot_len);
    LWIP_DEBUGF(LOWPAN6_DEBUG | LWIP_DBG_TRACE, ("lowpan6_send: sending packet %p\n", (void *)p));
    err = netif->linkoutput(netif, p_frag);
  }

  pbuf_free(p_frag);

  return err;
}

err_t
lowpan6_set_context(u8_t idx, const ip6_addr_t * context)
{
  if (idx >= LWIP_6LOWPAN_NUM_CONTEXTS) {
    return ERR_ARG;
  }

  IP6_ADDR_ZONECHECK(context);

  ip6_addr_set(&lowpan6_context[idx], context);

  return ERR_OK;
}

#if LWIP_6LOWPAN_INFER_SHORT_ADDRESS
err_t
lowpan6_set_short_addr(u8_t addr_high, u8_t addr_low)
{
  short_mac_addr.addr[0] = addr_high;
  short_mac_addr.addr[1] = addr_low;

  return ERR_OK;
}
#endif /* LWIP_6LOWPAN_INFER_SHORT_ADDRESS */

#if LWIP_IPV4
err_t
lowpan4_output(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
  (void)netif;
  (void)q;
  (void)ipaddr;

  return ERR_IF;
}
#endif /* LWIP_IPV4 */

/**
 * Resolve and fill-in IEEE 802.15.4 address header for outgoing IPv6 packet.
 *
 * Perform Header Compression and fragment if necessary.
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param q The pbuf(s) containing the IP packet to be sent.
 * @param ip6addr The IP address of the packet destination.
 *
 * @return err_t
 */
err_t
lowpan6_output(struct netif *netif, struct pbuf *q, const ip6_addr_t *ip6addr)
{
  err_t result;
  const u8_t *hwaddr;
  struct ieee_802154_addr src, dest;
#if LWIP_6LOWPAN_INFER_SHORT_ADDRESS
  ip6_addr_t ip6_src;
  struct ip6_hdr * ip6_hdr;
#endif /* LWIP_6LOWPAN_INFER_SHORT_ADDRESS */

#if LWIP_6LOWPAN_INFER_SHORT_ADDRESS
  /* Check if we can compress source address (use aligned copy) */
  ip6_hdr = (struct ip6_hdr *)q->payload;
  ip6_addr_copy_from_packed(ip6_src, ip6_hdr->src);
  ip6_addr_assign_zone(&ip6_src, IP6_UNICAST, netif);
  if (lowpan6_get_address_mode(&ip6_src, &short_mac_addr) == 3) {
    src.addr_len = 2;
    src.addr[0] = short_mac_addr.addr[0];
    src.addr[1] = short_mac_addr.addr[1];
  } else
#endif /* LWIP_6LOWPAN_INFER_SHORT_ADDRESS */
  {
    src.addr_len = netif->hwaddr_len;
    SMEMCPY(src.addr, netif->hwaddr, netif->hwaddr_len);
  }

  /* multicast destination IP address? */
  if (ip6_addr_ismulticast(ip6addr)) {
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
    /* We need to send to the broadcast address.*/
    return lowpan6_frag(netif, q, &src, &ieee_802154_broadcast);
  }

  /* We have a unicast destination IP address */
  /* @todo anycast? */

#if LWIP_6LOWPAN_INFER_SHORT_ADDRESS
  if (src.addr_len == 2) {
    /* If source address was compressable to short_mac_addr, and dest has same subnet and
    * is also compressable to 2-bytes, assume we can infer dest as a short address too. */
    dest.addr_len = 2;
    dest.addr[0] = ((u8_t *)q->payload)[38];
    dest.addr[1] = ((u8_t *)q->payload)[39];
    if ((src.addr_len == 2) && (ip6_addr_netcmp_zoneless(&ip6_hdr->src, &ip6_hdr->dest)) &&
        (lowpan6_get_address_mode(ip6addr, &dest) == 3)) {
      MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
      return lowpan6_frag(netif, q, &src, &dest);
    }
  }
#endif /* LWIP_6LOWPAN_INFER_SHORT_ADDRESS */

  /* Ask ND6 what to do with the packet. */
  result = nd6_get_next_hop_addr_or_queue(netif, q, ip6addr, &hwaddr);
  if (result != ERR_OK) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    return result;
  }

  /* If no hardware address is returned, nd6 has queued the packet for later. */
  if (hwaddr == NULL) {
    return ERR_OK;
  }

  /* Send out the packet using the returned hardware address. */
  dest.addr_len = netif->hwaddr_len;
  SMEMCPY(dest.addr, hwaddr, netif->hwaddr_len);
  MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  return lowpan6_frag(netif, q, &src, &dest);
}

static struct pbuf *
lowpan6_decompress(struct pbuf * p, struct ieee_802154_addr * src, struct ieee_802154_addr * dest)
{
  struct pbuf * q;
  u8_t * lowpan6_buffer;
  u16_t lowpan6_offset;
  struct ip6_hdr *ip6hdr;
  s8_t i;
  s8_t ip6_offset = IP6_HLEN;

#if LWIP_UDP
#define UDP_HLEN_ALLOC UDP_HLEN
#else
#define UDP_HLEN_ALLOC 0
#endif

  q = pbuf_alloc(PBUF_IP, p->len + IP6_HLEN + UDP_HLEN_ALLOC, PBUF_POOL);
  if (q == NULL) {
    pbuf_free(p);
    return NULL;
  }

  lowpan6_buffer = (u8_t *)p->payload;
  ip6hdr = (struct ip6_hdr *)q->payload;

  lowpan6_offset = 2;
  if (lowpan6_buffer[1] & 0x80) {
    lowpan6_offset++;
  }

  /* Set IPv6 version, traffic class and flow label. */
  if ((lowpan6_buffer[0] & 0x18) == 0x00) {
    IP6H_VTCFL_SET(ip6hdr, 6, lowpan6_buffer[lowpan6_offset], ((lowpan6_buffer[lowpan6_offset+1] & 0x0f) << 16) | (lowpan6_buffer[lowpan6_offset + 2] << 8) | lowpan6_buffer[lowpan6_offset+3]);
    lowpan6_offset += 4;
  } else if ((lowpan6_buffer[0] & 0x18) == 0x08) {
    IP6H_VTCFL_SET(ip6hdr, 6, lowpan6_buffer[lowpan6_offset] & 0xc0, ((lowpan6_buffer[lowpan6_offset] & 0x0f) << 16) | (lowpan6_buffer[lowpan6_offset + 1] << 8) | lowpan6_buffer[lowpan6_offset+2]);
    lowpan6_offset += 3;
  } else if ((lowpan6_buffer[0] & 0x18) == 0x10) {
    IP6H_VTCFL_SET(ip6hdr, 6, lowpan6_buffer[lowpan6_offset],0);
    lowpan6_offset += 1;
  } else if ((lowpan6_buffer[0] & 0x18) == 0x18) {
    IP6H_VTCFL_SET(ip6hdr, 6, 0, 0);
  }

  /* Set Next Header */
  if ((lowpan6_buffer[0] & 0x04) == 0x00) {
    IP6H_NEXTH_SET(ip6hdr, lowpan6_buffer[lowpan6_offset++]);
  } else {
    /* We should fill this later with NHC decoding */
    IP6H_NEXTH_SET(ip6hdr, 0);
  }

  /* Set Hop Limit */
  if ((lowpan6_buffer[0] & 0x03) == 0x00) {
    IP6H_HOPLIM_SET(ip6hdr, lowpan6_buffer[lowpan6_offset++]);
  } else if ((lowpan6_buffer[0] & 0x03) == 0x01) {
    IP6H_HOPLIM_SET(ip6hdr, 1);
  } else if ((lowpan6_buffer[0] & 0x03) == 0x02) {
    IP6H_HOPLIM_SET(ip6hdr, 64);
  } else if ((lowpan6_buffer[0] & 0x03) == 0x03) {
    IP6H_HOPLIM_SET(ip6hdr, 255);
  }

  /* Source address decoding. */
  if ((lowpan6_buffer[1] & 0x40) == 0x00) {
    /* Stateless compression */
    if ((lowpan6_buffer[1] & 0x30) == 0x00) {
      /* copy full address */
      MEMCPY(&ip6hdr->src.addr[0], lowpan6_buffer + lowpan6_offset, 16);
      lowpan6_offset += 16;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x10) {
      ip6hdr->src.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->src.addr[1] = 0;
      MEMCPY(&ip6hdr->src.addr[2], lowpan6_buffer + lowpan6_offset, 8);
      lowpan6_offset += 8;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x20) {
      ip6hdr->src.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->src.addr[1] = 0;
      ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
      ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (lowpan6_buffer[lowpan6_offset] << 8) |
                                  lowpan6_buffer[lowpan6_offset+1]);
      lowpan6_offset += 2;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x30) {
      ip6hdr->src.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->src.addr[1] = 0;
      if (src->addr_len == 2) {
        ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
        ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (src->addr[0] << 8) | src->addr[1]);
      } else {
        ip6hdr->src.addr[2] = lwip_htonl(((src->addr[0] ^ 2) << 24) | (src->addr[1] << 16) |
                                    (src->addr[2] << 8) | src->addr[3]);
        ip6hdr->src.addr[3] = lwip_htonl((src->addr[4] << 24) | (src->addr[5] << 16) |
                                    (src->addr[6] << 8) | src->addr[7]);
      }
    }
  } else {
    /* Stateful compression */
    if ((lowpan6_buffer[1] & 0x30) == 0x00) {
      /* ANY address */
      ip6hdr->src.addr[0] = 0;
      ip6hdr->src.addr[1] = 0;
      ip6hdr->src.addr[2] = 0;
      ip6hdr->src.addr[3] = 0;
    } else {
      /* Set prefix from context info */
      if (lowpan6_buffer[1] & 0x80) {
        i = (lowpan6_buffer[2] >> 4) & 0x0f;
      } else {
        i = 0;
      }
      if (i >= LWIP_6LOWPAN_NUM_CONTEXTS) {
        /* Error */
        pbuf_free(p);
        pbuf_free(q);
        return NULL;
      }

      ip6hdr->src.addr[0] = lowpan6_context[i].addr[0];
      ip6hdr->src.addr[1] = lowpan6_context[i].addr[1];
    }

    if ((lowpan6_buffer[1] & 0x30) == 0x10) {
      MEMCPY(&ip6hdr->src.addr[2], lowpan6_buffer + lowpan6_offset, 8);
      lowpan6_offset += 8;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x20) {
      ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
      ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (lowpan6_buffer[lowpan6_offset] << 8) | lowpan6_buffer[lowpan6_offset+1]);
      lowpan6_offset += 2;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x30) {
      if (src->addr_len == 2) {
        ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
        ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (src->addr[0] << 8) | src->addr[1]);
      } else {
        ip6hdr->src.addr[2] = lwip_htonl(((src->addr[0] ^ 2) << 24) | (src->addr[1] << 16) | (src->addr[2] << 8) | src->addr[3]);
        ip6hdr->src.addr[3] = lwip_htonl((src->addr[4] << 24) | (src->addr[5] << 16) | (src->addr[6] << 8) | src->addr[7]);
      }
    }
  }

  /* Destination address decoding. */
  if (lowpan6_buffer[1] & 0x08) {
    /* Multicast destination */
    if (lowpan6_buffer[1] & 0x04) {
      /* @todo support stateful multicast addressing */
      pbuf_free(p);
      pbuf_free(q);
      return NULL;
    }

    if ((lowpan6_buffer[1] & 0x03) == 0x00) {
      /* copy full address */
      MEMCPY(&ip6hdr->dest.addr[0], lowpan6_buffer + lowpan6_offset, 16);
      lowpan6_offset += 16;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x01) {
      ip6hdr->dest.addr[0] = lwip_htonl(0xff000000UL | (lowpan6_buffer[lowpan6_offset++] << 16));
      ip6hdr->dest.addr[1] = 0;
      ip6hdr->dest.addr[2] = lwip_htonl(lowpan6_buffer[lowpan6_offset++]);
      ip6hdr->dest.addr[3] = lwip_htonl((lowpan6_buffer[lowpan6_offset] << 24) | (lowpan6_buffer[lowpan6_offset + 1] << 16) | (lowpan6_buffer[lowpan6_offset + 2] << 8) | lowpan6_buffer[lowpan6_offset + 3]);
      lowpan6_offset += 4;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x02) {
      ip6hdr->dest.addr[0] = lwip_htonl(0xff000000UL | (lowpan6_buffer[lowpan6_offset++] << 16)); 
      ip6hdr->dest.addr[1] = 0;
      ip6hdr->dest.addr[2] = 0;
      ip6hdr->dest.addr[3] = lwip_htonl((lowpan6_buffer[lowpan6_offset] << 16) | (lowpan6_buffer[lowpan6_offset + 1] << 8) | lowpan6_buffer[lowpan6_offset + 2]);
      lowpan6_offset += 3;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x03) {
      ip6hdr->dest.addr[0] = PP_HTONL(0xff020000UL);
      ip6hdr->dest.addr[1] = 0;
      ip6hdr->dest.addr[2] = 0;
      ip6hdr->dest.addr[3] = lwip_htonl(lowpan6_buffer[lowpan6_offset++]);
    }

  } else {
    if (lowpan6_buffer[1] & 0x04) {
      /* Stateful destination compression */
      /* Set prefix from context info */
      if (lowpan6_buffer[1] & 0x80) {
        i = lowpan6_buffer[2] & 0x0f;
      } else {
        i = 0;
      }
      if (i >= LWIP_6LOWPAN_NUM_CONTEXTS) {
        /* Error */
        pbuf_free(p);
        pbuf_free(q);
        return NULL;
      }

      ip6hdr->dest.addr[0] = lowpan6_context[i].addr[0];
      ip6hdr->dest.addr[1] = lowpan6_context[i].addr[1];
    } else {
      /* Link local address compression */
      ip6hdr->dest.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->dest.addr[1] = 0;
    }

    if ((lowpan6_buffer[1] & 0x03) == 0x00) {
      /* copy full address */
      MEMCPY(&ip6hdr->dest.addr[0], lowpan6_buffer + lowpan6_offset, 16);
      lowpan6_offset += 16;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x01) {
      MEMCPY(&ip6hdr->dest.addr[2], lowpan6_buffer + lowpan6_offset, 8);
      lowpan6_offset += 8;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x02) {
      ip6hdr->dest.addr[2] = PP_HTONL(0x000000ffUL);
      ip6hdr->dest.addr[3] = lwip_htonl(0xfe000000UL | (lowpan6_buffer[lowpan6_offset] << 8) | lowpan6_buffer[lowpan6_offset + 1]);
      lowpan6_offset += 2;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x03) {
      if (dest->addr_len == 2) {
        ip6hdr->dest.addr[2] = PP_HTONL(0x000000ffUL);
        ip6hdr->dest.addr[3] = lwip_htonl(0xfe000000UL | (dest->addr[0] << 8) | dest->addr[1]);
      } else {
        ip6hdr->dest.addr[2] = lwip_htonl(((dest->addr[0] ^ 2) << 24) | (dest->addr[1] << 16) | dest->addr[2] << 8 | dest->addr[3]);
        ip6hdr->dest.addr[3] = lwip_htonl((dest->addr[4] << 24) | (dest->addr[5] << 16) | dest->addr[6] << 8 | dest->addr[7]);
      }
    }
  }


  /* Next Header Compression (NHC) decoding? */
  if (lowpan6_buffer[0] & 0x04) {
#if LWIP_UDP
    if ((lowpan6_buffer[lowpan6_offset] & 0xf8) == 0xf0) {
      struct udp_hdr *udphdr;

      /* UDP compression */
      IP6H_NEXTH_SET(ip6hdr, IP6_NEXTH_UDP);
      udphdr = (struct udp_hdr *)((u8_t *)q->payload + ip6_offset);

      if (lowpan6_buffer[lowpan6_offset] & 0x04) {
        /* @todo support checksum decompress */
        pbuf_free(p);
        pbuf_free(q);
        return NULL;
      }

      /* Decompress ports */
      i = lowpan6_buffer[lowpan6_offset++] & 0x03;
      if (i == 0) {
        udphdr->src = lwip_htons(lowpan6_buffer[lowpan6_offset] << 8 | lowpan6_buffer[lowpan6_offset + 1]);
        udphdr->dest = lwip_htons(lowpan6_buffer[lowpan6_offset + 2] << 8 | lowpan6_buffer[lowpan6_offset + 3]);
        lowpan6_offset += 4;
      } else if (i == 0x01) {
        udphdr->src = lwip_htons(lowpan6_buffer[lowpan6_offset] << 8 | lowpan6_buffer[lowpan6_offset + 1]);
        udphdr->dest = lwip_htons(0xf000 | lowpan6_buffer[lowpan6_offset + 2]);
        lowpan6_offset += 3;
      } else if (i == 0x02) {
        udphdr->src = lwip_htons(0xf000 | lowpan6_buffer[lowpan6_offset]);
        udphdr->dest = lwip_htons(lowpan6_buffer[lowpan6_offset + 1] << 8 | lowpan6_buffer[lowpan6_offset + 2]);
        lowpan6_offset += 3;
      } else if (i == 0x03) {
        udphdr->src = lwip_htons(0xf0b0 | ((lowpan6_buffer[lowpan6_offset] >> 4) & 0x0f));
        udphdr->dest = lwip_htons(0xf0b0 | (lowpan6_buffer[lowpan6_offset] & 0x0f));
        lowpan6_offset += 1;
      }

      udphdr->chksum = lwip_htons(lowpan6_buffer[lowpan6_offset] << 8 | lowpan6_buffer[lowpan6_offset + 1]);
      lowpan6_offset += 2;
      udphdr->len = lwip_htons(p->tot_len - lowpan6_offset + UDP_HLEN);

      ip6_offset += UDP_HLEN;
    } else
#endif /* LWIP_UDP */
    {
      /* @todo support NHC other than UDP */
      pbuf_free(p);
      pbuf_free(q);
      return NULL;
    }
  }

  /* Now we copy leftover contents from p to q, so we have all L2 and L3 headers (and L4?) in a single PBUF.
  * Replace p with q, and free p */
  pbuf_remove_header(p, lowpan6_offset);
  MEMCPY((u8_t*)q->payload + ip6_offset, p->payload, p->len);
  q->len = q->tot_len = ip6_offset + p->len;
  if (p->next != NULL) {
    pbuf_cat(q, p->next);
  }
  p->next = NULL;
  pbuf_free(p);

  /* Infer IPv6 payload length for header */
  IP6H_PLEN_SET(ip6hdr, q->tot_len - IP6_HLEN);

  /* all done */
  return q;
}

err_t
lowpan6_input(struct pbuf * p, struct netif *netif)
{
  u8_t * puc;
  s8_t i;
  struct ieee_802154_addr src, dest;
  u16_t datagram_size, datagram_offset, datagram_tag;
  struct lowpan6_reass_helper *lrh, *lrh_temp;

  MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);

  /* Analyze header. @todo validate. */
  puc = (u8_t*)p->payload;
  datagram_offset = 5;
  if ((puc[1] & 0x0c) == 0x0c) {
    dest.addr_len = 8;
    for (i = 0; i < 8; i++) {
      dest.addr[i] = puc[datagram_offset + 7 - i];
    }
    datagram_offset += 8;
  } else {
    dest.addr_len = 2;
    dest.addr[0] = puc[datagram_offset + 1];
    dest.addr[1] = puc[datagram_offset];
    datagram_offset += 2;
  }

  datagram_offset += 2; /* skip PAN ID. */

  if ((puc[1] & 0xc0) == 0xc0) {
    src.addr_len = 8;
    for (i = 0; i < 8; i++) {
      src.addr[i] = puc[datagram_offset + 7 - i];
    }
    datagram_offset += 8;
  } else {
    src.addr_len = 2;
    src.addr[0] = puc[datagram_offset + 1];
    src.addr[1] = puc[datagram_offset];
    datagram_offset += 2;
  }

  pbuf_remove_header(p, datagram_offset); /* hide IEEE802.15.4 header. */

  /* Check dispatch. */
  puc = (u8_t*)p->payload;

  if ((*puc & 0xf8) == 0xc0) {
    /* FRAG1 dispatch. add this packet to reassembly list. */
    datagram_size = ((u16_t)(puc[0] & 0x07) << 8) | (u16_t)puc[1];
    datagram_tag = ((u16_t)puc[2] << 8) | (u16_t)puc[3];

    /* check for duplicate */
    lrh = reass_list;
    while (lrh != NULL) {
      if ((lrh->sender_addr.addr_len == src.addr_len) &&
          (memcmp(lrh->sender_addr.addr, src.addr, src.addr_len) == 0)) {
        /* address match with packet in reassembly. */
        if ((datagram_tag == lrh->datagram_tag) && (datagram_size == lrh->datagram_size)) {
          MIB2_STATS_NETIF_INC(netif, ifindiscards);
          /* duplicate fragment. */
          pbuf_free(p);
          return ERR_OK;
        } else {
          /* We are receiving the start of a new datagram. Discard old one (incomplete). */
          lrh_temp = lrh->next_packet;
          dequeue_datagram(lrh);
          pbuf_free(lrh->pbuf);
          mem_free(lrh);

          /* Check next datagram in queue. */
          lrh = lrh_temp;
        }
      } else {
        /* Check next datagram in queue. */
        lrh = lrh->next_packet;
      }
    }

    pbuf_remove_header(p, 4); /* hide frag1 dispatch */

    lrh = (struct lowpan6_reass_helper *) mem_malloc(sizeof(struct lowpan6_reass_helper));
    if (lrh == NULL) {
      MIB2_STATS_NETIF_INC(netif, ifindiscards);
      pbuf_free(p);
      return ERR_MEM;
    }

    lrh->sender_addr.addr_len = src.addr_len;
    for (i = 0; i < src.addr_len; i++) {
      lrh->sender_addr.addr[i] = src.addr[i];
    }
    lrh->datagram_size = datagram_size;
    lrh->datagram_tag = datagram_tag;
    lrh->pbuf = p;
    lrh->next_packet = reass_list;
    lrh->timer = 2;
    reass_list = lrh;

    return ERR_OK;
  } else if ((*puc & 0xf8) == 0xe0) {
    /* FRAGN dispatch, find packet being reassembled. */
    datagram_size = ((u16_t)(puc[0] & 0x07) << 8) | (u16_t)puc[1];
    datagram_tag = ((u16_t)puc[2] << 8) | (u16_t)puc[3];
    datagram_offset = (u16_t)puc[4] << 3;
    pbuf_remove_header(p, 5); /* hide frag1 dispatch */

    for (lrh = reass_list; lrh != NULL; lrh = lrh->next_packet) {
      if ((lrh->sender_addr.addr_len == src.addr_len) &&
          (memcmp(lrh->sender_addr.addr, src.addr, src.addr_len) == 0) &&
          (datagram_tag == lrh->datagram_tag) &&
          (datagram_size == lrh->datagram_size)) {
        break;
      }
    }
    if (lrh == NULL) {
      /* rogue fragment */
      MIB2_STATS_NETIF_INC(netif, ifindiscards);
      pbuf_free(p);
      return ERR_OK;
    }

    if (lrh->pbuf->tot_len < datagram_offset) {
      /* duplicate, ignore. */
      pbuf_free(p);
      return ERR_OK;
    } else if (lrh->pbuf->tot_len > datagram_offset) {
      MIB2_STATS_NETIF_INC(netif, ifindiscards);
      /* We have missed a fragment. Delete whole reassembly. */
      dequeue_datagram(lrh);
      pbuf_free(lrh->pbuf);
      mem_free(lrh);
      pbuf_free(p);
      return ERR_OK;
    }
    pbuf_cat(lrh->pbuf, p);
    p = NULL;

    /* is packet now complete?*/
    if (lrh->pbuf->tot_len >= lrh->datagram_size) {
      /* dequeue from reass list. */
      dequeue_datagram(lrh);

      /* get pbuf */
      p = lrh->pbuf;

      /* release helper */
      mem_free(lrh);
    } else {
      return ERR_OK;
    }
  }

  if (p == NULL) {
    return ERR_OK;
  }

  /* We have a complete packet, check dispatch for headers. */
  puc = (u8_t*)p->payload;

  if (*puc == 0x41) {
    /* This is a complete IPv6 packet, just skip dispatch byte. */
    pbuf_remove_header(p, 1); /* hide dispatch byte. */
  } else if ((*puc & 0xe0 )== 0x60) {
    /* IPv6 headers are compressed using IPHC. */
    p = lowpan6_decompress(p, &src, &dest);
    if (p == NULL) {
      MIB2_STATS_NETIF_INC(netif, ifindiscards);
      return ERR_OK;
    }
  } else {
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
    pbuf_free(p);
    return ERR_OK;
  }

  /* @todo: distinguish unicast/multicast */
  MIB2_STATS_NETIF_INC(netif, ifinucastpkts);

  return ip6_input(p, netif);
}

err_t
lowpan6_if_init(struct netif *netif)
{
  netif->name[0] = 'L';
  netif->name[1] = '6';
#if LWIP_IPV4
  netif->output = lowpan4_output;
#endif /* LWIP_IPV4 */
  netif->output_ip6 = lowpan6_output;

  MIB2_INIT_NETIF(netif, snmp_ifType_other, 0);

  /* maximum transfer unit */
  netif->mtu = 1280;

  /* broadcast capability */
  netif->flags = NETIF_FLAG_BROADCAST /* | NETIF_FLAG_LOWPAN6 */;

  return ERR_OK;
}

err_t
lowpan6_set_pan_id(u16_t pan_id)
{
  ieee_802154_pan_id = pan_id;

  return ERR_OK;
}

#if !NO_SYS
/**
 * Pass a received packet to tcpip_thread for input processing
 *
 * @param p the received packet, p->payload pointing to the
 *          IEEE 802.15.4 header.
 * @param inp the network interface on which the packet was received
 */
err_t
tcpip_6lowpan_input(struct pbuf *p, struct netif *inp)
{
  return tcpip_inpkt(p, inp, lowpan6_input);
}
#endif /* !NO_SYS */

#endif /* LWIP_IPV6 && LWIP_6LOWPAN */
