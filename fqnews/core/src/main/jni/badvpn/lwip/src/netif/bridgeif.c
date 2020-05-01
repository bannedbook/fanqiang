/**
 * @file
 * lwIP netif implementing an IEEE 802.1D MAC Bridge
 */

/*
 * Copyright (c) 2017 Simon Goldschmidt.
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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

/**
 * @defgroup bridgeif IEEE 802.1D bridge
 * @ingroup netifs
 * This file implements an IEEE 802.1D bridge by using a multilayer netif approach
 * (one hardware-independent netif for the bridge that uses hardware netifs for its ports).
 * On transmit, the bridge selects the outgoing port(s).
 * On receive, the port netif calls into the bridge (via its netif->input function) and
 * the bridge selects the port(s) (and/or its netif->input function) to pass the received pbuf to.
 *
 * Usage:
 * - add the port netifs just like you would when using them as dedicated netif without a bridge
 *   - only NETIF_FLAG_ETHARP/NETIF_FLAG_ETHERNET netifs are supported as bridge ports
 *   - add the bridge port netifs without IPv4 addresses (i.e. pass 'NULL, NULL, NULL')
 *   - don't add IPv6 addresses to the port netifs!
 * - set up the bridge configuration in a global variable of type 'bridgeif_initdata_t' that contains
 *   - the MAC address of the bridge
 *   - some configuration options controlling the memory consumption (maximum number of ports
 *     and FDB entries)
 *   - e.g. for a bridge MAC address 00-01-02-03-04-05, 2 bridge ports, 1024 FDB entries + 16 static MAC entries:
 *     bridgeif_initdata_t mybridge_initdata = BRIDGEIF_INITDATA1(2, 1024, 16, ETH_ADDR(0, 1, 2, 3, 4, 5));
 * - add the bridge netif (with IPv4 config):
 *   struct netif bridge_netif;
 *   netif_add(&bridge_netif, &my_ip, &my_netmask, &my_gw, &mybridge_initdata, bridgeif_init, tcpip_input);
 *   NOTE: the passed 'input' function depends on BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT setting,
 *         which controls where the forwarding is done (netif low level input context vs. tcpip_thread)
 * - set up all ports netifs and the bridge netif
 *
 * - When adding a port netif, NETIF_FLAG_ETHARP flag will be removed from a port
 *   to prevent ETHARP working on that port netif (we only want one IP per bridge not per port).
 * - When adding a port netif, its input function is changed to call into the bridge.
 *
 *
 * @todo:
 * - compact static FDB entries (instead of walking the whole array)
 * - add FDB query/read access
 * - add FDB change callback (when learning or dropping auto-learned entries)
 * - prefill FDB with MAC classes that should never be forwarded
 * - multicast snooping? (and only forward group addresses to interested ports)
 * - support removing ports
 * - check SNMP integration
 * - VLAN handling / trunk ports
 * - priority handling? (although that largely depends on TX queue limitations and lwIP doesn't provide tx-done handling)
 */

#include "netif/bridgeif.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/snmp.h"
#include "lwip/timeouts.h"
#include <string.h>

#if LWIP_NUM_NETIF_CLIENT_DATA

#define BRIDGEIF_AGE_TIMER_MS 1000

#if BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT
#ifndef BRIDGEIF_DECL_PROTECT
/* define bridgeif protection to sys_arch_protect... */
#include "lwip/sys.h"
#define BRIDGEIF_DECL_PROTECT(lev)    SYS_ARCH_DECL_PROTECT(lev)
#define BRIDGEIF_READ_PROTECT(lev)    SYS_ARCH_PROTECT(lev)
#define BRIDGEIF_READ_UNPROTECT(lev)  SYS_ARCH_UNPROTECT(lev)
#define BRIDGEIF_WRITE_PROTECT(lev)
#define BRIDGEIF_WRITE_UNPROTECT(lev)
#endif
#else /* BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT */
#include "lwip/tcpip.h"
#define BRIDGEIF_DECL_PROTECT(lev)
#define BRIDGEIF_READ_PROTECT(lev)
#define BRIDGEIF_READ_UNPROTECT(lev)
#define BRIDGEIF_WRITE_PROTECT(lev)
#define BRIDGEIF_WRITE_UNPROTECT(lev)
#endif /* BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT */

/* Define those to better describe your network interface. */
#define IFNAME0 'b'
#define IFNAME1 'r'

#define BR_FDB_TIMEOUT_SEC  (60*5) /* 5 minutes FDB timeout */

#if !BRIDGEIF_EXTERNAL_FDB
typedef struct bridgeif_dfdb_entry_s {
  u8_t used;
  u8_t port;
  u32_t ts;
  struct eth_addr addr;
} bridgeif_dfdb_entry_t;

typedef struct bridgeif_dfdb_s {
  u16_t max_fdb_entries;
  bridgeif_dfdb_entry_t *fdb;
} bridgeif_dfdb_t;
#endif /* BRIDGEIF_EXTERNAL_FDB */

struct bridgeif_private_s;
typedef struct bridgeif_port_private_s {
  struct bridgeif_private_s *bridge;
  struct netif *port_netif;
  u8_t port_num;
} bridgeif_port_t;

typedef struct bridgeif_fdb_static_entry_s {
  u8_t used;
  bridgeif_portmask_t dst_ports;
  struct eth_addr addr;
} bridgeif_fdb_static_entry_t;

typedef struct bridgeif_private_s {
  struct netif     *netif;
  struct eth_addr   ethaddr;
  u8_t              max_ports;
  u8_t              num_ports;
  bridgeif_port_t  *ports;
  u16_t             max_fdbs_entries;
  bridgeif_fdb_static_entry_t *fdbs;
  u16_t             max_fdbd_entries;
  void             *fdbd;
} bridgeif_private_t;

/* netif data index to get the bridge on input */
u8_t bridgeif_netif_client_id = 0xff;

#if !BRIDGEIF_EXTERNAL_FDB
/** A real simple and slow implementation of an auto-learning forwarding database that
 * remembers known src mac addresses to know which port to send frames destined for that
 * mac address.
 *
 * ATTENTION: This is meant as an example only, in real-world use, you should override this
 * by setting BRIDGEIF_EXTERNAL_FDB==1 and providing a better implementation :-)
 */
static void
bridgeif_fdb_update_src(void *fdb_ptr, struct eth_addr *src_addr, u8_t port_idx)
{
  int i;
  bridgeif_dfdb_t *fdb = (bridgeif_dfdb_t*)fdb_ptr;
  BRIDGEIF_DECL_PROTECT(lev);
  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < fdb->max_fdb_entries; i++) {
    bridgeif_dfdb_entry_t *e = &fdb->fdb[i];
    if (e->used && e->ts) {
      if (!memcmp(&e->addr, src_addr, sizeof(struct eth_addr))) {
        LWIP_DEBUGF(BRIDGEIF_FDB_DEBUG, ("br: update src %02x:%02x:%02x:%02x:%02x:%02x (from %d) @ idx %d\n",
          src_addr->addr[0], src_addr->addr[1], src_addr->addr[2], src_addr->addr[3], src_addr->addr[4], src_addr->addr[5],
          port_idx, i));
        BRIDGEIF_WRITE_PROTECT(lev);
        e->ts = BR_FDB_TIMEOUT_SEC;
        e->port = port_idx;
        BRIDGEIF_WRITE_UNPROTECT(lev);
        BRIDGEIF_READ_UNPROTECT(lev);
        return;
      }
    }
  }
  /* not found, allocate new entry from free */
  for (i = 0; i < fdb->max_fdb_entries; i++) {
    bridgeif_dfdb_entry_t *e = &fdb->fdb[i];
    if (!e->used || !e->ts) {
      BRIDGEIF_WRITE_PROTECT(lev);
      /* check again when protected */
      if (!e->used || !e->ts) {
        LWIP_DEBUGF(BRIDGEIF_FDB_DEBUG, ("br: create src %02x:%02x:%02x:%02x:%02x:%02x (from %d) @ idx %d\n",
          src_addr->addr[0], src_addr->addr[1], src_addr->addr[2], src_addr->addr[3], src_addr->addr[4], src_addr->addr[5],
          port_idx, i));
        memcpy(&e->addr, src_addr, sizeof(struct eth_addr));
        e->ts = BR_FDB_TIMEOUT_SEC;
        e->port = port_idx;
        e->used = 1;
        BRIDGEIF_WRITE_UNPROTECT(lev);
        BRIDGEIF_READ_UNPROTECT(lev);
        return;
      }
      BRIDGEIF_WRITE_UNPROTECT(lev);
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  /* not found, no free entry -> flood */
}

/** Walk our list of auto-learnt fdb entries and return a port to forward or BR_FLOOD if unknown */
static bridgeif_portmask_t
bridgeif_fdb_get_dst_ports(void *fdb_ptr, struct eth_addr *dst_addr)
{
  int i;
  bridgeif_dfdb_t *fdb = (bridgeif_dfdb_t*)fdb_ptr;
  BRIDGEIF_DECL_PROTECT(lev);
  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < fdb->max_fdb_entries; i++) {
    bridgeif_dfdb_entry_t *e = &fdb->fdb[i];
    if (e->used && e->ts) {
      if (!memcmp(&e->addr, dst_addr, sizeof(struct eth_addr))) {
        bridgeif_portmask_t ret = (bridgeif_portmask_t)(1 << e->port);
        BRIDGEIF_READ_UNPROTECT(lev);
        return ret;
      }
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  return BR_FLOOD;
}

/** Init our simple fdb list */
static void*
bridgeif_fdb_init(u16_t max_fdb_entries)
{
  bridgeif_dfdb_t *fdb;
  size_t alloc_len_sizet = sizeof(bridgeif_dfdb_t) + (max_fdb_entries*sizeof(bridgeif_dfdb_entry_t));
  mem_size_t alloc_len = (mem_size_t)alloc_len_sizet;
  LWIP_ASSERT("alloc_len == alloc_len_sizet", alloc_len == alloc_len_sizet);
  LWIP_DEBUGF(BRIDGEIF_DEBUG, ("bridgeif_fdb_init: allocating %d bytes for private FDB data\n", (int)alloc_len));
  fdb = (bridgeif_dfdb_t*)mem_calloc(1, alloc_len);
  if (fdb == NULL) {
    return NULL;
  }
  fdb->max_fdb_entries = max_fdb_entries;
  fdb->fdb = (bridgeif_dfdb_entry_t *)(fdb + 1);
  return fdb;
}

/** Aging implementation of our simple fdb */
static void
bridgeif_fdb_age_one_second(void *fdb_ptr)
{
  int i;
  bridgeif_dfdb_t *fdb;
  BRIDGEIF_DECL_PROTECT(lev);

  fdb = (bridgeif_dfdb_t *)fdb_ptr;
  BRIDGEIF_READ_PROTECT(lev);

  for (i = 0; i < fdb->max_fdb_entries; i++) {
    bridgeif_dfdb_entry_t *e = &fdb->fdb[i];
    if (e->used && e->ts) {
      BRIDGEIF_WRITE_PROTECT(lev);
      /* check again when protected */
      if (e->used && e->ts) {
        if (--e->ts == 0) {
          e->used = 0;
        }
      }
      BRIDGEIF_WRITE_UNPROTECT(lev);
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
}

#endif /* !BRIDGEIF_EXTERNAL_FDB */

/** Timer callback for fdb aging, called once per second */
static void
bridgeif_age_tmr(void *arg)
{
  struct netif *bridgeif;
  bridgeif_private_t *br;
  LWIP_ASSERT("invalid arg", arg != NULL);
  bridgeif = (struct netif *)arg;
  LWIP_ASSERT("invalid netif state", bridgeif->state != NULL);

  br = (bridgeif_private_t *)bridgeif->state;
  bridgeif_fdb_age_one_second(br->fdbd);
  sys_timeout(BRIDGEIF_AGE_TIMER_MS, bridgeif_age_tmr, arg);
}

/**
 * @ingroup bridgeif
 * Add a static entry to the forwarding database.
 * A static entry marks where frames to a specific eth address (unicast or group address) are
 * forwarded.
 * bits [0..(BRIDGEIF_MAX_PORTS-1)]: hw ports
 * bit [BRIDGEIF_MAX_PORTS]: cpu port
 * 0: drop
 */
err_t
bridgeif_fdb_add(struct netif *bridgeif, const struct eth_addr *addr, bridgeif_portmask_t ports)
{
  int i;
  bridgeif_private_t *br;
  BRIDGEIF_DECL_PROTECT(lev);
  LWIP_ASSERT("invalid netif", bridgeif != NULL);
  br = (bridgeif_private_t*)bridgeif->state;
  LWIP_ASSERT("invalid state", br != NULL);

  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < br->max_fdbs_entries; i++) {
    if (!br->fdbs[i].used) {
      BRIDGEIF_WRITE_PROTECT(lev);
      if (!br->fdbs[i].used) {
        br->fdbs[i].used = 1;
        br->fdbs[i].dst_ports = ports;
        memcpy(&br->fdbs[i].addr, addr, sizeof(struct eth_addr));
        BRIDGEIF_WRITE_UNPROTECT(lev);
        BRIDGEIF_READ_UNPROTECT(lev);
        return ERR_OK;
      }
      BRIDGEIF_WRITE_UNPROTECT(lev);
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  return ERR_MEM;
}

/**
 * @ingroup bridgeif
 * Remove a static entry from the forwarding database
 */
err_t
bridgeif_fdb_remove(struct netif *bridgeif, const struct eth_addr *addr)
{
  int i;
  bridgeif_private_t *br;
  BRIDGEIF_DECL_PROTECT(lev);
  LWIP_ASSERT("invalid netif", bridgeif != NULL);
  br = (bridgeif_private_t*)bridgeif->state;
  LWIP_ASSERT("invalid state", br != NULL);

  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < br->max_fdbs_entries; i++) {
    if (br->fdbs[i].used && !memcmp(&br->fdbs[i].addr, addr, sizeof(struct eth_addr))) {
      BRIDGEIF_WRITE_PROTECT(lev);
      if (br->fdbs[i].used && !memcmp(&br->fdbs[i].addr, addr, sizeof(struct eth_addr))) {
        memset(&br->fdbs[i], 0, sizeof(bridgeif_fdb_static_entry_t));
        BRIDGEIF_WRITE_UNPROTECT(lev);
        BRIDGEIF_READ_UNPROTECT(lev);
        return ERR_OK;
      }
      BRIDGEIF_WRITE_UNPROTECT(lev);
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  return ERR_VAL;
}

/** Get the forwarding port(s) (as bit mask) for the specified destination mac address */
static bridgeif_portmask_t
bridgeif_find_dst_ports(bridgeif_private_t *br, struct eth_addr *dst_addr)
{
  int i;
  BRIDGEIF_DECL_PROTECT(lev);
  BRIDGEIF_READ_PROTECT(lev);
  /* first check for static entries */
  for (i = 0; i < br->max_fdbs_entries; i++) {
    if (br->fdbs[i].used) {
      if (!memcmp(&br->fdbs[i].addr, dst_addr, sizeof(struct eth_addr))) {
        bridgeif_portmask_t ret = br->fdbs[i].dst_ports;
        BRIDGEIF_READ_UNPROTECT(lev);
        return ret;
      }
    }
  }
  if (dst_addr->addr[0] & 1) {
    /* no match found: flood remaining group address */
    BRIDGEIF_READ_UNPROTECT(lev);
    return BR_FLOOD;
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  /* no match found: check dynamic fdb for port or fall back to flooding */
  return bridgeif_fdb_get_dst_ports(br->fdbd, dst_addr);
}

/** Helper function to see if a destination mac belongs to the bridge
 * (bridge netif or one of the port netifs), in which case the frame
 * is sent to the cpu only.
 */
static int
bridgeif_is_local_mac(bridgeif_private_t *br, struct eth_addr *addr)
{
  int i;
  BRIDGEIF_DECL_PROTECT(lev);
  if (!memcmp(br->netif->hwaddr, addr, sizeof(struct eth_addr))) {
    return 1;
  }
  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < br->num_ports; i++) {
    struct netif *portif = br->ports[i].port_netif;
    if (portif != NULL) {
      if (!memcmp(portif->hwaddr, addr, sizeof(struct eth_addr))) {
        BRIDGEIF_READ_UNPROTECT(lev);
        return 1;
      }
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  return 0;
}

/* Output helper function */
static err_t
bridgeif_send_to_port(bridgeif_private_t *br, struct pbuf *p, u8_t dstport_idx)
{
  if (dstport_idx < BRIDGEIF_MAX_PORTS) {
    /* possibly an external port */
    if (dstport_idx < br->max_ports) {
      struct netif *portif = br->ports[dstport_idx].port_netif;
      if (br->ports[dstport_idx].port_netif != NULL) {
        if ((portif != NULL) && (portif->linkoutput != NULL)) {
          /* prevent sending out to rx port */
          if (netif_get_index(portif) != p->if_idx) {
            if (netif_is_link_up(portif)) {
              LWIP_DEBUGF(BRIDGEIF_FW_DEBUG, ("br -> flood(%p:%d) -> %d\n", (void*)p, p->if_idx, netif_get_index(portif)));
              return portif->linkoutput(portif, p);
            }
          }
        }
      }
    }
  } else {
    LWIP_ASSERT("invalid port index", dstport_idx == BRIDGEIF_MAX_PORTS);
  }
  return ERR_OK;
}

/** Helper function to pass a pbuf to all ports marked in 'dstports'
 */
static err_t
bridgeif_send_to_ports(bridgeif_private_t *br, struct pbuf *p, bridgeif_portmask_t dstports)
{
  err_t err, ret_err = ERR_OK;
  u8_t i;
  bridgeif_portmask_t mask = 1;
  BRIDGEIF_DECL_PROTECT(lev);
  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < BRIDGEIF_MAX_PORTS; i++, mask = (bridgeif_portmask_t)(mask << 1)) {
    if (dstports & mask) {
      err = bridgeif_send_to_port(br, p, i);
      if (err != ERR_OK) {
        ret_err = err;
      }
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  return ret_err;
}

/** Output function of the application port of the bridge (the one with an ip address).
 * The forwarding port(s) where this pbuf is sent on is/are automatically selected
 * from the FDB.
 */
static err_t
bridgeif_output(struct netif *netif, struct pbuf *p)
{
  err_t err;
  bridgeif_private_t *br = (bridgeif_private_t*)netif->state;
  struct eth_addr *dst = (struct eth_addr *)(p->payload);

  bridgeif_portmask_t dstports = bridgeif_find_dst_ports(br, dst);
  err = bridgeif_send_to_ports(br, p, dstports);

  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  if (((u8_t*)p->payload)[0] & 1) {
    /* broadcast or multicast packet*/
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  } else {
    /* unicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  }
  /* increase ifoutdiscards or ifouterrors on error */

  LINK_STATS_INC(link.xmit);

  return err;
}

/** The actual bridge input function. Port netif's input is changed to call
 * here. This function decides where the frame is forwarded.
 */
static err_t
bridgeif_input(struct pbuf *p, struct netif *netif)
{
  u8_t rx_idx;
  bridgeif_portmask_t dstports;
  struct eth_addr *src, *dst;
  bridgeif_private_t *br;
  bridgeif_port_t *port;
  if (p == NULL || netif == NULL) {
    return ERR_VAL;
  }
  port = (bridgeif_port_t *)netif_get_client_data(netif, bridgeif_netif_client_id);
  LWIP_ASSERT("port data not set", port != NULL);
  if (port == NULL || port->bridge == NULL) {
    return ERR_VAL;
  }
  br = (bridgeif_private_t *)port->bridge;
  rx_idx = netif_get_index(netif);
  /* store receive index in pbuf */
  p->if_idx = rx_idx;

  dst = (struct eth_addr*)p->payload;
  src = (struct eth_addr*)(((u8_t*)p->payload) + sizeof(struct eth_addr));

  if ((src->addr[0] & 1) == 0) {
    /* update src for all non-group addresses */
    bridgeif_fdb_update_src(br->fdbd, src, port->port_num);
  }

  if (dst->addr[0] & 1) {
    /* group address -> flood + cpu? */
    dstports = bridgeif_find_dst_ports(br, dst);
    bridgeif_send_to_ports(br, p, dstports);
    if (dstports & (1 << BRIDGEIF_MAX_PORTS)) {
      /* we pass the reference to ->input or have to free it */
      LWIP_DEBUGF(BRIDGEIF_FW_DEBUG, ("br -> input(%p)\n", (void*)p));
      if (br->netif->input(p, br->netif) != ERR_OK) {
        pbuf_free(p);
      }
    } else {
      /* all references done */
      pbuf_free(p);
    }
    /* always return ERR_OK here to prevent the caller freeing the pbuf */
    return ERR_OK;
  } else {
    /* is this for one of the local ports? */
    if (bridgeif_is_local_mac(br, dst)) {
      /* yes, send to cpu port only */
      LWIP_DEBUGF(BRIDGEIF_FW_DEBUG, ("br -> input(%p)\n", (void*)p));
      return br->netif->input(p, br->netif);
    }

    /* get dst port */
    dstports = bridgeif_find_dst_ports(br, dst);
    bridgeif_send_to_ports(br, p, dstports);
    /* no need to send to cpu, flooding is for external ports only */
    /* by  this, we consumed the pbuf */
    pbuf_free(p);
    /* always return ERR_OK here to prevent the caller freeing the pbuf */
    return ERR_OK;
  }
}

#if !BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT
/** Input function for port netifs used to synchronize into tcpip_thread.
 */
static err_t
bridgeif_tcpip_input(struct pbuf *p, struct netif *netif)
{
  return tcpip_inpkt(p, netif, bridgeif_input);
}
#endif /* BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT */

/**
 * @ingroup bridgeif
 * Initialization function passed to netif_add().
 *
 * ATTENTION: A pointer to a @ref bridgeif_initdata_t must be passed as 'state'
 *            to @ref netif_add when adding the bridge. I supplies MAC address
 *            and controls memory allocation (number of ports, FDB size).
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
bridgeif_init(struct netif *netif)
{
  bridgeif_initdata_t *init_data;
  bridgeif_private_t *br;
  size_t alloc_len_sizet;
  mem_size_t alloc_len;

  LWIP_ASSERT("netif != NULL", (netif != NULL));
#if !BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT
  if (netif->input == tcpip_input) {
    LWIP_DEBUGF(BRIDGEIF_DEBUG|LWIP_DBG_ON, ("bridgeif does not need tcpip_input, use netif_input/ethernet_input instead"));
  }
#endif

  if (bridgeif_netif_client_id == 0xFF) {
    bridgeif_netif_client_id = netif_alloc_client_data_id();
  }

  init_data = (bridgeif_initdata_t *)netif->state;
  LWIP_ASSERT("init_data != NULL", (init_data != NULL));
  LWIP_ASSERT("init_data->max_ports <= BRIDGEIF_MAX_PORTS",
    init_data->max_ports <= BRIDGEIF_MAX_PORTS);

  alloc_len_sizet = sizeof(bridgeif_private_t) + (init_data->max_ports*sizeof(bridgeif_port_t) + (init_data->max_fdb_static_entries*sizeof(bridgeif_fdb_static_entry_t)));
  alloc_len = (mem_size_t)alloc_len_sizet;
  LWIP_ASSERT("alloc_len == alloc_len_sizet", alloc_len == alloc_len_sizet);
  LWIP_DEBUGF(BRIDGEIF_DEBUG, ("bridgeif_init: allocating %d bytes for private data\n", (int)alloc_len));
  br = (bridgeif_private_t*)mem_calloc(1, alloc_len);
  if (br == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("bridgeif_init: out of memory\n"));
    return ERR_MEM;
  }
  memcpy(&br->ethaddr, &init_data->ethaddr, sizeof(br->ethaddr));
  br->netif = netif;

  br->max_ports = init_data->max_ports;
  br->ports = (bridgeif_port_t*)(br + 1);

  br->max_fdbs_entries = init_data->max_fdb_static_entries;
  br->fdbs = (bridgeif_fdb_static_entry_t*)(((u8_t*)(br + 1)) + (init_data->max_ports*sizeof(bridgeif_port_t)));

  br->max_fdbd_entries = init_data->max_fdb_dynamic_entries;
  br->fdbd = bridgeif_fdb_init(init_data->max_fdb_dynamic_entries);
  if (br->fdbd == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("bridgeif_init: out of memory in fdb_init\n"));
    mem_free(br);
    return ERR_MEM;
  }

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 0);

  netif->state = br;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = bridgeif_output;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;

  /* set MAC hardware address */
  memcpy(netif->hwaddr, &br->ethaddr, ETH_HWADDR_LEN);

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6 |NETIF_FLAG_LINK_UP;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  sys_timeout(BRIDGEIF_AGE_TIMER_MS, bridgeif_age_tmr, netif);

  return ERR_OK;
}

/**
 * @ingroup bridgeif
 * Add a port to the bridge
 */
err_t
bridgeif_add_port(struct netif *bridgeif, struct netif *portif)
{
  bridgeif_private_t *br;
  bridgeif_port_t *port;

  LWIP_ASSERT("bridgeif != NULL", bridgeif != NULL);
  LWIP_ASSERT("bridgeif->state != NULL", bridgeif->state != NULL);
  LWIP_ASSERT("portif != NULL", portif != NULL);

  if (!(portif->flags & NETIF_FLAG_ETHARP) || !(portif->flags & NETIF_FLAG_ETHERNET)) {
    /* can only add ETHERNET/ETHARP interfaces */
    return ERR_VAL;
  }

  br = (bridgeif_private_t *)bridgeif->state;

  if (br->num_ports >= br->max_ports) {
    return ERR_VAL;
  }
  port = &br->ports[br->num_ports];
  port->port_netif = portif;
  port->port_num = br->num_ports;
  port->bridge = br;
  br->num_ports++;

  /* let the port call us on input */
#if BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT
  portif->input = bridgeif_input;
#else
  portif->input = bridgeif_tcpip_input;
#endif
  /* store pointer to bridge in netif */
  netif_set_client_data(portif, bridgeif_netif_client_id, port);
  /* remove ETHARP flag to prevent sending report events on netif-up */
  netif_clear_flags(portif, NETIF_FLAG_ETHARP);

  return ERR_OK;
}

#endif /* LWIP_NUM_NETIF_CLIENT_DATA */
