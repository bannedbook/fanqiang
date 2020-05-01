/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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
 * Author: Erik Ekman <erik@kryo.se>
 *
 */

#include "lwip/init.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#if LWIP_IPV6
#include "lwip/ethip6.h"
#include "lwip/nd6.h"
#endif
#include <string.h>
#include <stdio.h>

/* no-op send function */
static err_t lwip_tx_func(struct netif *netif, struct pbuf *p)
{
  LWIP_UNUSED_ARG(netif);
  LWIP_UNUSED_ARG(p);
  return ERR_OK;
}

static err_t testif_init(struct netif *netif)
{
  netif->name[0] = 'f';
  netif->name[1] = 'z';
  netif->output = etharp_output;
  netif->linkoutput = lwip_tx_func;
  netif->mtu = 1500;
  netif->hwaddr_len = 6;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  netif->hwaddr[0] = 0x00;
  netif->hwaddr[1] = 0x23;
  netif->hwaddr[2] = 0xC1;
  netif->hwaddr[3] = 0xDE;
  netif->hwaddr[4] = 0xD0;
  netif->hwaddr[5] = 0x0D;

#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
  netif->ip6_autoconfig_enabled = 1;
  netif_create_ip6_linklocal_address(netif, 1);
  netif->flags |= NETIF_FLAG_MLD6;
#endif

  return ERR_OK;
}

static void input_pkt(struct netif *netif, const u8_t *data, size_t len)
{
  struct pbuf *p, *q;
  err_t err;

  LWIP_ASSERT("pkt too big", len <= 0xFFFF);
  p = pbuf_alloc(PBUF_RAW, (u16_t)len, PBUF_POOL);
  LWIP_ASSERT("alloc failed", p);
  for(q = p; q != NULL; q = q->next) {
    MEMCPY(q->payload, data, q->len);
    data += q->len;
  }
  err = netif->input(p, netif);
  if (err != ERR_OK) {
    pbuf_free(p);
  }
}

int main(int argc, char** argv)
{
  struct netif net_test;
  ip4_addr_t addr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  u8_t pktbuf[2000];
  size_t len;

  lwip_init();

  IP4_ADDR(&addr, 172, 30, 115, 84);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 172, 30, 115, 1);

  netif_add(&net_test, &addr, &netmask, &gw, &net_test, testif_init, ethernet_input);
  netif_set_up(&net_test);

#if LWIP_IPV6
  nd6_tmr(); /* tick nd to join multicast groups */
#endif

  if(argc > 1) {
    FILE* f;
    const char* filename;
    printf("reading input from file... ");
    fflush(stdout);
    filename = argv[1];
    LWIP_ASSERT("invalid filename", filename != NULL);
    f = fopen(filename, "rb");
    LWIP_ASSERT("open failed", f != NULL);
    len = fread(pktbuf, 1, sizeof(pktbuf), f);
    fclose(f);
    printf("testing file: \"%s\"...\r\n", filename);
  } else {
    len = fread(pktbuf, 1, sizeof(pktbuf), stdin);
  }
  input_pkt(&net_test, pktbuf, len);

  return 0;
}
