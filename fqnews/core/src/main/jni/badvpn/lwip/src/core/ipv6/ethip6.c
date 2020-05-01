/**
 * @file
 *
 * Ethernet output for IPv6. Uses ND tables for link-layer addressing.
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

#if LWIP_IPV6 && LWIP_ETHERNET

#include "lwip/ethip6.h"
#include "lwip/nd6.h"
#include "lwip/pbuf.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/inet_chksum.h"
#include "lwip/netif.h"
#include "lwip/icmp6.h"
#include "lwip/prot/ethernet.h"
#include "netif/ethernet.h"

#include <string.h>

/**
 * Resolve and fill-in Ethernet address header for outgoing IPv6 packet.
 *
 * For IPv6 multicast, corresponding Ethernet addresses
 * are selected and the packet is transmitted on the link.
 *
 * For unicast addresses, ask the ND6 module what to do. It will either let us
 * send the the packet right away, or queue the packet for later itself, unless
 * an error occurs.
 *
 * @todo anycast addresses
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param q The pbuf(s) containing the IP packet to be sent.
 * @param ip6addr The IP address of the packet destination.
 *
 * @return
 * - ERR_OK or the return value of @ref nd6_get_next_hop_addr_or_queue.
 */
err_t
ethip6_output(struct netif *netif, struct pbuf *q, const ip6_addr_t *ip6addr)
{
  struct eth_addr dest;
  const u8_t *hwaddr;
  err_t result;

  /* The destination IP address must be properly zoned from here on down. */
  IP6_ADDR_ZONECHECK_NETIF(ip6addr, netif);

  /* multicast destination IP address? */
  if (ip6_addr_ismulticast(ip6addr)) {
    /* Hash IP multicast address to MAC address.*/
    dest.addr[0] = 0x33;
    dest.addr[1] = 0x33;
    dest.addr[2] = ((const u8_t *)(&(ip6addr->addr[3])))[0];
    dest.addr[3] = ((const u8_t *)(&(ip6addr->addr[3])))[1];
    dest.addr[4] = ((const u8_t *)(&(ip6addr->addr[3])))[2];
    dest.addr[5] = ((const u8_t *)(&(ip6addr->addr[3])))[3];

    /* Send out. */
    return ethernet_output(netif, q, (const struct eth_addr*)(netif->hwaddr), &dest, ETHTYPE_IPV6);
  }

  /* We have a unicast destination IP address */
  /* @todo anycast? */

  /* Ask ND6 what to do with the packet. */
  result = nd6_get_next_hop_addr_or_queue(netif, q, ip6addr, &hwaddr);
  if (result != ERR_OK) {
    return result;
  }

  /* If no hardware address is returned, nd6 has queued the packet for later. */
  if (hwaddr == NULL) {
    return ERR_OK;
  }

  /* Send out the packet using the returned hardware address. */
  SMEMCPY(dest.addr, hwaddr, 6);
  return ethernet_output(netif, q, (const struct eth_addr*)(netif->hwaddr), &dest, ETHTYPE_IPV6);
}

#endif /* LWIP_IPV6 && LWIP_ETHERNET */
