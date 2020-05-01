/**
 * @file
 * AutoIP Automatic LinkLocal IP Configuration
 *
 * This is a AutoIP implementation for the lwIP TCP/IP stack. It aims to conform
 * with RFC 3927.
 *
 * @defgroup autoip AUTOIP
 * @ingroup ip4
 * AUTOIP related functions
 * USAGE:
 *
 * define @ref LWIP_AUTOIP 1 in your lwipopts.h
 * Options:
 * AUTOIP_TMR_INTERVAL msecs,
 *   I recommend a value of 100. The value must divide 1000 with a remainder almost 0.
 *   Possible values are 1000, 500, 333, 250, 200, 166, 142, 125, 111, 100 ....
 *
 * Without DHCP:
 * - Call autoip_start() after netif_add().
 *
 * With DHCP:
 * - define @ref LWIP_DHCP_AUTOIP_COOP 1 in your lwipopts.h.
 * - Configure your DHCP Client.
 *
 * @see netifapi_autoip
 */

/*
 *
 * Copyright (c) 2007 Dominik Spies <kontakt@dspies.de>
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
 * Author: Dominik Spies <kontakt@dspies.de>
 */

#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_AUTOIP /* don't build if not configured for use in lwipopts.h */

#include "lwip/mem.h"
/* #include "lwip/udp.h" */
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/autoip.h"
#include "lwip/etharp.h"
#include "lwip/prot/autoip.h"

#include <string.h>

/** Pseudo random macro based on netif informations.
 * You could use "rand()" from the C Library if you define LWIP_AUTOIP_RAND in lwipopts.h */
#ifndef LWIP_AUTOIP_RAND
#define LWIP_AUTOIP_RAND(netif) ( (((u32_t)((netif->hwaddr[5]) & 0xff) << 24) | \
                                   ((u32_t)((netif->hwaddr[3]) & 0xff) << 16) | \
                                   ((u32_t)((netif->hwaddr[2]) & 0xff) << 8) | \
                                   ((u32_t)((netif->hwaddr[4]) & 0xff))) + \
                                   (netif_autoip_data(netif)? netif_autoip_data(netif)->tried_llipaddr : 0))
#endif /* LWIP_AUTOIP_RAND */

/**
 * Macro that generates the initial IP address to be tried by AUTOIP.
 * If you want to override this, define it to something else in lwipopts.h.
 */
#ifndef LWIP_AUTOIP_CREATE_SEED_ADDR
#define LWIP_AUTOIP_CREATE_SEED_ADDR(netif) \
  lwip_htonl(AUTOIP_RANGE_START + ((u32_t)(((u8_t)(netif->hwaddr[4])) | \
                 ((u32_t)((u8_t)(netif->hwaddr[5]))) << 8)))
#endif /* LWIP_AUTOIP_CREATE_SEED_ADDR */

/* static functions */
static err_t autoip_arp_announce(struct netif *netif);
static void autoip_start_probing(struct netif *netif);

/**
 * @ingroup autoip
 * Set a statically allocated struct autoip to work with.
 * Using this prevents autoip_start to allocate it using mem_malloc.
 *
 * @param netif the netif for which to set the struct autoip
 * @param autoip (uninitialised) autoip struct allocated by the application
 */
void
autoip_set_struct(struct netif *netif, struct autoip *autoip)
{
  LWIP_ASSERT("netif != NULL", netif != NULL);
  LWIP_ASSERT("autoip != NULL", autoip != NULL);
  LWIP_ASSERT("netif already has a struct autoip set",
              netif_autoip_data(netif) == NULL);

  /* clear data structure */
  memset(autoip, 0, sizeof(struct autoip));
  /* autoip->state = AUTOIP_STATE_OFF; */
  netif_set_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_AUTOIP, autoip);
}

/** Restart AutoIP client and check the next address (conflict detected)
 *
 * @param netif The netif under AutoIP control
 */
static void
autoip_restart(struct netif *netif)
{
  struct autoip *autoip = netif_autoip_data(netif);
  autoip->tried_llipaddr++;
  autoip_start(netif);
}

/**
 * Handle a IP address conflict after an ARP conflict detection
 */
static void
autoip_handle_arp_conflict(struct netif *netif)
{
  struct autoip *autoip = netif_autoip_data(netif);

  /* RFC3927, 2.5 "Conflict Detection and Defense" allows two options where
     a) means retreat on the first conflict and
     b) allows to keep an already configured address when having only one
        conflict in 10 seconds
     We use option b) since it helps to improve the chance that one of the two
     conflicting hosts may be able to retain its address. */

  if (autoip->lastconflict > 0) {
    /* retreat, there was a conflicting ARP in the last DEFEND_INTERVAL seconds */
    LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                ("autoip_handle_arp_conflict(): we are defending, but in DEFEND_INTERVAL, retreating\n"));

    /* Active TCP sessions are aborted when removing the ip addresss */
    autoip_restart(netif);
  } else {
    LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                ("autoip_handle_arp_conflict(): we are defend, send ARP Announce\n"));
    autoip_arp_announce(netif);
    autoip->lastconflict = DEFEND_INTERVAL * AUTOIP_TICKS_PER_SECOND;
  }
}

/**
 * Create an IP-Address out of range 169.254.1.0 to 169.254.254.255
 *
 * @param netif network interface on which create the IP-Address
 * @param ipaddr ip address to initialize
 */
static void
autoip_create_addr(struct netif *netif, ip4_addr_t *ipaddr)
{
  struct autoip *autoip = netif_autoip_data(netif);

  /* Here we create an IP-Address out of range 169.254.1.0 to 169.254.254.255
   * compliant to RFC 3927 Section 2.1
   * We have 254 * 256 possibilities */

  u32_t addr = lwip_ntohl(LWIP_AUTOIP_CREATE_SEED_ADDR(netif));
  addr += autoip->tried_llipaddr;
  addr = AUTOIP_NET | (addr & 0xffff);
  /* Now, 169.254.0.0 <= addr <= 169.254.255.255 */

  if (addr < AUTOIP_RANGE_START) {
    addr += AUTOIP_RANGE_END - AUTOIP_RANGE_START + 1;
  }
  if (addr > AUTOIP_RANGE_END) {
    addr -= AUTOIP_RANGE_END - AUTOIP_RANGE_START + 1;
  }
  LWIP_ASSERT("AUTOIP address not in range", (addr >= AUTOIP_RANGE_START) &&
              (addr <= AUTOIP_RANGE_END));
  ip4_addr_set_u32(ipaddr, lwip_htonl(addr));

  LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
              ("autoip_create_addr(): tried_llipaddr=%"U16_F", %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
               (u16_t)(autoip->tried_llipaddr), ip4_addr1_16(ipaddr), ip4_addr2_16(ipaddr),
               ip4_addr3_16(ipaddr), ip4_addr4_16(ipaddr)));
}

/**
 * Sends an ARP probe from a network interface
 *
 * @param netif network interface used to send the probe
 */
static err_t
autoip_arp_probe(struct netif *netif)
{
  struct autoip *autoip = netif_autoip_data(netif);
  /* this works because netif->ip_addr is ANY */
  return etharp_request(netif, &autoip->llipaddr);
}

/**
 * Sends an ARP announce from a network interface
 *
 * @param netif network interface used to send the announce
 */
static err_t
autoip_arp_announce(struct netif *netif)
{
  return etharp_gratuitous(netif);
}

/**
 * Configure interface for use with current LL IP-Address
 *
 * @param netif network interface to configure with current LL IP-Address
 */
static err_t
autoip_bind(struct netif *netif)
{
  struct autoip *autoip = netif_autoip_data(netif);
  ip4_addr_t sn_mask, gw_addr;

  LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE,
              ("autoip_bind(netif=%p) %c%c%"U16_F" %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
               (void *)netif, netif->name[0], netif->name[1], (u16_t)netif->num,
               ip4_addr1_16(&autoip->llipaddr), ip4_addr2_16(&autoip->llipaddr),
               ip4_addr3_16(&autoip->llipaddr), ip4_addr4_16(&autoip->llipaddr)));

  IP4_ADDR(&sn_mask, 255, 255, 0, 0);
  IP4_ADDR(&gw_addr, 0, 0, 0, 0);

  netif_set_addr(netif, &autoip->llipaddr, &sn_mask, &gw_addr);
  /* interface is used by routing now that an address is set */

  return ERR_OK;
}

/**
 * @ingroup autoip
 * Start AutoIP client
 *
 * @param netif network interface on which start the AutoIP client
 */
err_t
autoip_start(struct netif *netif)
{
  struct autoip *autoip = netif_autoip_data(netif);
  err_t result = ERR_OK;

  LWIP_ERROR("netif is not up, old style port?", netif_is_up(netif), return ERR_ARG;);

  /* Set IP-Address, Netmask and Gateway to 0 to make sure that
   * ARP Packets are formed correctly
   */
  netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);

  LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
              ("autoip_start(netif=%p) %c%c%"U16_F"\n", (void *)netif, netif->name[0],
               netif->name[1], (u16_t)netif->num));
  if (autoip == NULL) {
    /* no AutoIP client attached yet? */
    LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE,
                ("autoip_start(): starting new AUTOIP client\n"));
    autoip = (struct autoip *)mem_calloc(1, sizeof(struct autoip));
    if (autoip == NULL) {
      LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE,
                  ("autoip_start(): could not allocate autoip\n"));
      return ERR_MEM;
    }
    /* store this AutoIP client in the netif */
    netif_set_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_AUTOIP, autoip);
    LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE, ("autoip_start(): allocated autoip"));
  } else {
    autoip->state = AUTOIP_STATE_OFF;
    autoip->ttw = 0;
    autoip->sent_num = 0;
    ip4_addr_set_zero(&autoip->llipaddr);
    autoip->lastconflict = 0;
  }

  autoip_create_addr(netif, &(autoip->llipaddr));
  autoip_start_probing(netif);

  return result;
}

static void
autoip_start_probing(struct netif *netif)
{
  struct autoip *autoip = netif_autoip_data(netif);

  autoip->state = AUTOIP_STATE_PROBING;
  autoip->sent_num = 0;
  LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
              ("autoip_start_probing(): changing state to PROBING: %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
               ip4_addr1_16(&autoip->llipaddr), ip4_addr2_16(&autoip->llipaddr),
               ip4_addr3_16(&autoip->llipaddr), ip4_addr4_16(&autoip->llipaddr)));

  /* time to wait to first probe, this is randomly
   * chosen out of 0 to PROBE_WAIT seconds.
   * compliant to RFC 3927 Section 2.2.1
   */
  autoip->ttw = (u16_t)(LWIP_AUTOIP_RAND(netif) % (PROBE_WAIT * AUTOIP_TICKS_PER_SECOND));

  /*
   * if we tried more then MAX_CONFLICTS we must limit our rate for
   * acquiring and probing address
   * compliant to RFC 3927 Section 2.2.1
   */
  if (autoip->tried_llipaddr > MAX_CONFLICTS) {
    autoip->ttw = RATE_LIMIT_INTERVAL * AUTOIP_TICKS_PER_SECOND;
  }
}

/**
 * Handle a possible change in the network configuration.
 *
 * If there is an AutoIP address configured, take the interface down
 * and begin probing with the same address.
 */
void
autoip_network_changed(struct netif *netif)
{
  struct autoip *autoip = netif_autoip_data(netif);

  if (autoip && (autoip->state != AUTOIP_STATE_OFF)) {
    autoip_start_probing(netif);
  }
}

/**
 * @ingroup autoip
 * Stop AutoIP client
 *
 * @param netif network interface on which stop the AutoIP client
 */
err_t
autoip_stop(struct netif *netif)
{
  struct autoip *autoip = netif_autoip_data(netif);

  if (autoip != NULL) {
    autoip->state = AUTOIP_STATE_OFF;
    if (ip4_addr_islinklocal(netif_ip4_addr(netif))) {
      netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
    }
  }
  return ERR_OK;
}

/**
 * Has to be called in loop every AUTOIP_TMR_INTERVAL milliseconds
 */
void
autoip_tmr(void)
{
  struct netif *netif;
  /* loop through netif's */
  NETIF_FOREACH(netif) {
    struct autoip *autoip = netif_autoip_data(netif);
    /* only act on AutoIP configured interfaces */
    if (autoip != NULL) {
      if (autoip->lastconflict > 0) {
        autoip->lastconflict--;
      }

      LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE,
                  ("autoip_tmr() AutoIP-State: %"U16_F", ttw=%"U16_F"\n",
                   (u16_t)(autoip->state), autoip->ttw));

      if (autoip->ttw > 0) {
        autoip->ttw--;
      }

      switch (autoip->state) {
        case AUTOIP_STATE_PROBING:
          if (autoip->ttw == 0) {
            if (autoip->sent_num >= PROBE_NUM) {
              /* Switch to ANNOUNCING: now we can bind to an IP address and use it */
              autoip->state = AUTOIP_STATE_ANNOUNCING;
              autoip_bind(netif);
              /* autoip_bind() calls netif_set_addr(): this triggers a gratuitous ARP
                 which counts as an announcement */
              autoip->sent_num = 1;
              autoip->ttw = ANNOUNCE_WAIT * AUTOIP_TICKS_PER_SECOND;
              LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                          ("autoip_tmr(): changing state to ANNOUNCING: %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
                           ip4_addr1_16(&autoip->llipaddr), ip4_addr2_16(&autoip->llipaddr),
                           ip4_addr3_16(&autoip->llipaddr), ip4_addr4_16(&autoip->llipaddr)));
            } else {
              autoip_arp_probe(netif);
              LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE, ("autoip_tmr() PROBING Sent Probe\n"));
              autoip->sent_num++;
              if (autoip->sent_num == PROBE_NUM) {
                /* calculate time to wait to for announce */
                autoip->ttw = ANNOUNCE_WAIT * AUTOIP_TICKS_PER_SECOND;
              } else {
                /* calculate time to wait to next probe */
                autoip->ttw = (u16_t)((LWIP_AUTOIP_RAND(netif) %
                                       ((PROBE_MAX - PROBE_MIN) * AUTOIP_TICKS_PER_SECOND) ) +
                                      PROBE_MIN * AUTOIP_TICKS_PER_SECOND);
              }
            }
          }
          break;

        case AUTOIP_STATE_ANNOUNCING:
          if (autoip->ttw == 0) {
            autoip_arp_announce(netif);
            LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE, ("autoip_tmr() ANNOUNCING Sent Announce\n"));
            autoip->ttw = ANNOUNCE_INTERVAL * AUTOIP_TICKS_PER_SECOND;
            autoip->sent_num++;

            if (autoip->sent_num >= ANNOUNCE_NUM) {
              autoip->state = AUTOIP_STATE_BOUND;
              autoip->sent_num = 0;
              autoip->ttw = 0;
              LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                          ("autoip_tmr(): changing state to BOUND: %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
                           ip4_addr1_16(&autoip->llipaddr), ip4_addr2_16(&autoip->llipaddr),
                           ip4_addr3_16(&autoip->llipaddr), ip4_addr4_16(&autoip->llipaddr)));
            }
          }
          break;

        default:
          /* nothing to do in other states */
          break;
      }
    }
  }
}

/**
 * Handles every incoming ARP Packet, called by etharp_input().
 *
 * @param netif network interface to use for autoip processing
 * @param hdr Incoming ARP packet
 */
void
autoip_arp_reply(struct netif *netif, struct etharp_hdr *hdr)
{
  struct autoip *autoip = netif_autoip_data(netif);

  LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE, ("autoip_arp_reply()\n"));
  if ((autoip != NULL) && (autoip->state != AUTOIP_STATE_OFF)) {
    /* when ip.src == llipaddr && hw.src != netif->hwaddr
     *
     * when probing  ip.dst == llipaddr && hw.src != netif->hwaddr
     * we have a conflict and must solve it
     */
    ip4_addr_t sipaddr, dipaddr;
    struct eth_addr netifaddr;
    SMEMCPY(netifaddr.addr, netif->hwaddr, ETH_HWADDR_LEN);

    /* Copy struct ip4_addr_wordaligned to aligned ip4_addr, to support compilers without
     * structure packing (not using structure copy which breaks strict-aliasing rules).
     */
    IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&sipaddr, &hdr->sipaddr);
    IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&dipaddr, &hdr->dipaddr);

    if (autoip->state == AUTOIP_STATE_PROBING) {
      /* RFC 3927 Section 2.2.1:
       * from beginning to after ANNOUNCE_WAIT
       * seconds we have a conflict if
       * ip.src == llipaddr OR
       * ip.dst == llipaddr && hw.src != own hwaddr
       */
      if ((ip4_addr_cmp(&sipaddr, &autoip->llipaddr)) ||
          (ip4_addr_isany_val(sipaddr) &&
           ip4_addr_cmp(&dipaddr, &autoip->llipaddr) &&
           !eth_addr_cmp(&netifaddr, &hdr->shwaddr))) {
        LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                    ("autoip_arp_reply(): Probe Conflict detected\n"));
        autoip_restart(netif);
      }
    } else {
      /* RFC 3927 Section 2.5:
       * in any state we have a conflict if
       * ip.src == llipaddr && hw.src != own hwaddr
       */
      if (ip4_addr_cmp(&sipaddr, &autoip->llipaddr) &&
          !eth_addr_cmp(&netifaddr, &hdr->shwaddr)) {
        LWIP_DEBUGF(AUTOIP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                    ("autoip_arp_reply(): Conflicting ARP-Packet detected\n"));
        autoip_handle_arp_conflict(netif);
      }
    }
  }
}

/** check if AutoIP supplied netif->ip_addr
 *
 * @param netif the netif to check
 * @return 1 if AutoIP supplied netif->ip_addr (state BOUND or ANNOUNCING),
 *         0 otherwise
 */
u8_t
autoip_supplied_address(const struct netif *netif)
{
  if ((netif != NULL) && (netif_autoip_data(netif) != NULL)) {
    struct autoip *autoip = netif_autoip_data(netif);
    return (autoip->state == AUTOIP_STATE_BOUND) || (autoip->state == AUTOIP_STATE_ANNOUNCING);
  }
  return 0;
}

u8_t
autoip_accept_packet(struct netif *netif, const ip4_addr_t *addr)
{
  struct autoip *autoip = netif_autoip_data(netif);
  return (autoip != NULL) && ip4_addr_cmp(addr, &(autoip->llipaddr));
}

#endif /* LWIP_IPV4 && LWIP_AUTOIP */
