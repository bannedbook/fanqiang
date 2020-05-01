/**
 * @file
 * ICMP API
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
#ifndef LWIP_HDR_ICMP_H
#define LWIP_HDR_ICMP_H

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/prot/icmp.h"

#if LWIP_IPV6 && LWIP_ICMP6
#include "lwip/icmp6.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** ICMP destination unreachable codes */
enum icmp_dur_type {
  /** net unreachable */
  ICMP_DUR_NET   = 0,
  /** host unreachable */
  ICMP_DUR_HOST  = 1,
  /** protocol unreachable */
  ICMP_DUR_PROTO = 2,
  /** port unreachable */
  ICMP_DUR_PORT  = 3,
  /** fragmentation needed and DF set */
  ICMP_DUR_FRAG  = 4,
  /** source route failed */
  ICMP_DUR_SR    = 5
};

/** ICMP time exceeded codes */
enum icmp_te_type {
  /** time to live exceeded in transit */
  ICMP_TE_TTL  = 0,
  /** fragment reassembly time exceeded */
  ICMP_TE_FRAG = 1
};

#if LWIP_IPV4 && LWIP_ICMP /* don't build if not configured for use in lwipopts.h */

void icmp_input(struct pbuf *p, struct netif *inp);
void icmp_dest_unreach(struct pbuf *p, enum icmp_dur_type t);
void icmp_time_exceeded(struct pbuf *p, enum icmp_te_type t);

#endif /* LWIP_IPV4 && LWIP_ICMP */

#if LWIP_IPV4 && LWIP_IPV6
#if LWIP_ICMP && LWIP_ICMP6
#define icmp_port_unreach(isipv6, pbuf) ((isipv6) ? \
                                         icmp6_dest_unreach(pbuf, ICMP6_DUR_PORT) : \
                                         icmp_dest_unreach(pbuf, ICMP_DUR_PORT))
#elif LWIP_ICMP
#define icmp_port_unreach(isipv6, pbuf) do{ if(!(isipv6)) { icmp_dest_unreach(pbuf, ICMP_DUR_PORT);}}while(0)
#elif LWIP_ICMP6
#define icmp_port_unreach(isipv6, pbuf) do{ if(isipv6) { icmp6_dest_unreach(pbuf, ICMP6_DUR_PORT);}}while(0)
#else
#define icmp_port_unreach(isipv6, pbuf)
#endif
#elif LWIP_IPV6 && LWIP_ICMP6
#define icmp_port_unreach(isipv6, pbuf) icmp6_dest_unreach(pbuf, ICMP6_DUR_PORT)
#elif LWIP_IPV4 && LWIP_ICMP
#define icmp_port_unreach(isipv6, pbuf) icmp_dest_unreach(pbuf, ICMP_DUR_PORT)
#else /* (LWIP_IPV6 && LWIP_ICMP6) || (LWIP_IPV4 && LWIP_ICMP) */
#define icmp_port_unreach(isipv6, pbuf)
#endif /* (LWIP_IPV6 && LWIP_ICMP6) || (LWIP_IPV4 && LWIP_ICMP) LWIP_IPV4*/

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_ICMP_H */
