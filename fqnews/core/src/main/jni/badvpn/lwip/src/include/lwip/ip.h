/**
 * @file
 * IP API
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
#ifndef LWIP_HDR_IP_H
#define LWIP_HDR_IP_H

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/ip4.h"
#include "lwip/ip6.h"
#include "lwip/prot/ip.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is passed as the destination address to ip_output_if (not
   to ip_output), meaning that an IP header already is constructed
   in the pbuf. This is used when TCP retransmits. */
#define LWIP_IP_HDRINCL  NULL

/** pbufs passed to IP must have a ref-count of 1 as their payload pointer
    gets altered as the packet is passed down the stack */
#ifndef LWIP_IP_CHECK_PBUF_REF_COUNT_FOR_TX
#define LWIP_IP_CHECK_PBUF_REF_COUNT_FOR_TX(p) LWIP_ASSERT("p->ref == 1", (p)->ref == 1)
#endif

#if LWIP_NETIF_USE_HINTS
#define IP_PCB_NETIFHINT ;struct netif_hint netif_hints
#else /* LWIP_NETIF_USE_HINTS */
#define IP_PCB_NETIFHINT
#endif /* LWIP_NETIF_USE_HINTS */

/** This is the common part of all PCB types. It needs to be at the
   beginning of a PCB type definition. It is located here so that
   changes to this common part are made in one location instead of
   having to change all PCB structs. */
#define IP_PCB                             \
  /* ip addresses in network byte order */ \
  ip_addr_t local_ip;                      \
  ip_addr_t remote_ip;                     \
  /* Bound netif index */                  \
  u8_t netif_idx;                          \
  /* Socket options */                     \
  u8_t so_options;                         \
  /* Type Of Service */                    \
  u8_t tos;                                \
  /* Time To Live */                       \
  u8_t ttl                                 \
  /* link layer address resolution hint */ \
  IP_PCB_NETIFHINT

struct ip_pcb {
  /* Common members of all PCB types */
  IP_PCB;
};

/*
 * Option flags per-socket. These are the same like SO_XXX in sockets.h
 */
#define SOF_REUSEADDR     0x04U  /* allow local address reuse */
#define SOF_KEEPALIVE     0x08U  /* keep connections alive */
#define SOF_BROADCAST     0x20U  /* permit to send and to receive broadcast messages (see IP_SOF_BROADCAST option) */

/* These flags are inherited (e.g. from a listen-pcb to a connection-pcb): */
#define SOF_INHERITED   (SOF_REUSEADDR|SOF_KEEPALIVE)

/** Global variables of this module, kept in a struct for efficient access using base+index. */
struct ip_globals
{
  /** The interface that accepted the packet for the current callback invocation. */
  struct netif *current_netif;
  /** The interface that received the packet for the current callback invocation. */
  struct netif *current_input_netif;
#if LWIP_IPV4
  /** Header of the input packet currently being processed. */
  const struct ip_hdr *current_ip4_header;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  /** Header of the input IPv6 packet currently being processed. */
  struct ip6_hdr *current_ip6_header;
#endif /* LWIP_IPV6 */
  /** Total header length of current_ip4/6_header (i.e. after this, the UDP/TCP header starts) */
  u16_t current_ip_header_tot_len;
  /** Source IP address of current_header */
  ip_addr_t current_iphdr_src;
  /** Destination IP address of current_header */
  ip_addr_t current_iphdr_dest;
};
extern struct ip_globals ip_data;


/** Get the interface that accepted the current packet.
 * This may or may not be the receiving netif, depending on your netif/network setup.
 * This function must only be called from a receive callback (udp_recv,
 * raw_recv, tcp_accept). It will return NULL otherwise. */
#define ip_current_netif()      (ip_data.current_netif)
/** Get the interface that received the current packet.
 * This function must only be called from a receive callback (udp_recv,
 * raw_recv, tcp_accept). It will return NULL otherwise. */
#define ip_current_input_netif() (ip_data.current_input_netif)
/** Total header length of ip(6)_current_header() (i.e. after this, the UDP/TCP header starts) */
#define ip_current_header_tot_len() (ip_data.current_ip_header_tot_len)
/** Source IP address of current_header */
#define ip_current_src_addr()   (&ip_data.current_iphdr_src)
/** Destination IP address of current_header */
#define ip_current_dest_addr()  (&ip_data.current_iphdr_dest)

#if LWIP_IPV4 && LWIP_IPV6
/** Get the IPv4 header of the current packet.
 * This function must only be called from a receive callback (udp_recv,
 * raw_recv, tcp_accept). It will return NULL otherwise. */
#define ip4_current_header()     ip_data.current_ip4_header
/** Get the IPv6 header of the current packet.
 * This function must only be called from a receive callback (udp_recv,
 * raw_recv, tcp_accept). It will return NULL otherwise. */
#define ip6_current_header()      ((const struct ip6_hdr*)(ip_data.current_ip6_header))
/** Returns TRUE if the current IP input packet is IPv6, FALSE if it is IPv4 */
#define ip_current_is_v6()        (ip6_current_header() != NULL)
/** Source IPv6 address of current_header */
#define ip6_current_src_addr()    (ip_2_ip6(&ip_data.current_iphdr_src))
/** Destination IPv6 address of current_header */
#define ip6_current_dest_addr()   (ip_2_ip6(&ip_data.current_iphdr_dest))
/** Get the transport layer protocol */
#define ip_current_header_proto() (ip_current_is_v6() ? \
                                   IP6H_NEXTH(ip6_current_header()) :\
                                   IPH_PROTO(ip4_current_header()))
/** Get the transport layer header */
#define ip_next_header_ptr()     ((const void*)((ip_current_is_v6() ? \
  (const u8_t*)ip6_current_header() : (const u8_t*)ip4_current_header())  + ip_current_header_tot_len()))

/** Source IP4 address of current_header */
#define ip4_current_src_addr()     (ip_2_ip4(&ip_data.current_iphdr_src))
/** Destination IP4 address of current_header */
#define ip4_current_dest_addr()    (ip_2_ip4(&ip_data.current_iphdr_dest))

#elif LWIP_IPV4 /* LWIP_IPV4 && LWIP_IPV6 */

/** Get the IPv4 header of the current packet.
 * This function must only be called from a receive callback (udp_recv,
 * raw_recv, tcp_accept). It will return NULL otherwise. */
#define ip4_current_header()     ip_data.current_ip4_header
/** Always returns FALSE when only supporting IPv4 only */
#define ip_current_is_v6()        0
/** Get the transport layer protocol */
#define ip_current_header_proto() IPH_PROTO(ip4_current_header())
/** Get the transport layer header */
#define ip_next_header_ptr()     ((const void*)((const u8_t*)ip4_current_header() + ip_current_header_tot_len()))
/** Source IP4 address of current_header */
#define ip4_current_src_addr()     (&ip_data.current_iphdr_src)
/** Destination IP4 address of current_header */
#define ip4_current_dest_addr()    (&ip_data.current_iphdr_dest)

#elif LWIP_IPV6 /* LWIP_IPV4 && LWIP_IPV6 */

/** Get the IPv6 header of the current packet.
 * This function must only be called from a receive callback (udp_recv,
 * raw_recv, tcp_accept). It will return NULL otherwise. */
#define ip6_current_header()      ((const struct ip6_hdr*)(ip_data.current_ip6_header))
/** Always returns TRUE when only supporting IPv6 only */
#define ip_current_is_v6()        1
/** Get the transport layer protocol */
#define ip_current_header_proto() IP6H_NEXTH(ip6_current_header())
/** Get the transport layer header */
#define ip_next_header_ptr()     ((const void*)(((const u8_t*)ip6_current_header()) + ip_current_header_tot_len()))
/** Source IP6 address of current_header */
#define ip6_current_src_addr()    (&ip_data.current_iphdr_src)
/** Destination IP6 address of current_header */
#define ip6_current_dest_addr()   (&ip_data.current_iphdr_dest)

#endif /* LWIP_IPV6 */

/** Union source address of current_header */
#define ip_current_src_addr()    (&ip_data.current_iphdr_src)
/** Union destination address of current_header */
#define ip_current_dest_addr()   (&ip_data.current_iphdr_dest)

/** Gets an IP pcb option (SOF_* flags) */
#define ip_get_option(pcb, opt)   ((pcb)->so_options & (opt))
/** Sets an IP pcb option (SOF_* flags) */
#define ip_set_option(pcb, opt)   ((pcb)->so_options = (u8_t)((pcb)->so_options | (opt)))
/** Resets an IP pcb option (SOF_* flags) */
#define ip_reset_option(pcb, opt) ((pcb)->so_options = (u8_t)((pcb)->so_options & ~(opt)))

#if LWIP_IPV4 && LWIP_IPV6
/**
 * @ingroup ip
 * Output IP packet, netif is selected by source address
 */
#define ip_output(p, src, dest, ttl, tos, proto) \
        (IP_IS_V6(dest) ? \
        ip6_output(p, ip_2_ip6(src), ip_2_ip6(dest), ttl, tos, proto) : \
        ip4_output(p, ip_2_ip4(src), ip_2_ip4(dest), ttl, tos, proto))
/**
 * @ingroup ip
 * Output IP packet to specified interface
 */
#define ip_output_if(p, src, dest, ttl, tos, proto, netif) \
        (IP_IS_V6(dest) ? \
        ip6_output_if(p, ip_2_ip6(src), ip_2_ip6(dest), ttl, tos, proto, netif) : \
        ip4_output_if(p, ip_2_ip4(src), ip_2_ip4(dest), ttl, tos, proto, netif))
/**
 * @ingroup ip
 * Output IP packet to interface specifying source address
 */
#define ip_output_if_src(p, src, dest, ttl, tos, proto, netif) \
        (IP_IS_V6(dest) ? \
        ip6_output_if_src(p, ip_2_ip6(src), ip_2_ip6(dest), ttl, tos, proto, netif) : \
        ip4_output_if_src(p, ip_2_ip4(src), ip_2_ip4(dest), ttl, tos, proto, netif))
/** Output IP packet that already includes an IP header. */
#define ip_output_if_hdrincl(p, src, dest, netif) \
        (IP_IS_V6(dest) ? \
        ip6_output_if(p, ip_2_ip6(src), LWIP_IP_HDRINCL, 0, 0, 0, netif) : \
        ip4_output_if(p, ip_2_ip4(src), LWIP_IP_HDRINCL, 0, 0, 0, netif))
/** Output IP packet with netif_hint */
#define ip_output_hinted(p, src, dest, ttl, tos, proto, netif_hint) \
        (IP_IS_V6(dest) ? \
        ip6_output_hinted(p, ip_2_ip6(src), ip_2_ip6(dest), ttl, tos, proto, netif_hint) : \
        ip4_output_hinted(p, ip_2_ip4(src), ip_2_ip4(dest), ttl, tos, proto, netif_hint))
/**
 * @ingroup ip
 * Get netif for address combination. See \ref ip6_route and \ref ip4_route
 */
#define ip_route(src, dest) \
        (IP_IS_V6(dest) ? \
        ip6_route(ip_2_ip6(src), ip_2_ip6(dest)) : \
        ip4_route_src(ip_2_ip4(src), ip_2_ip4(dest)))
/**
 * @ingroup ip
 * Get netif for IP.
 */
#define ip_netif_get_local_ip(netif, dest) (IP_IS_V6(dest) ? \
        ip6_netif_get_local_ip(netif, ip_2_ip6(dest)) : \
        ip4_netif_get_local_ip(netif))
#define ip_debug_print(is_ipv6, p) ((is_ipv6) ? ip6_debug_print(p) : ip4_debug_print(p))

err_t ip_input(struct pbuf *p, struct netif *inp);

#elif LWIP_IPV4 /* LWIP_IPV4 && LWIP_IPV6 */

#define ip_output(p, src, dest, ttl, tos, proto) \
        ip4_output(p, src, dest, ttl, tos, proto)
#define ip_output_if(p, src, dest, ttl, tos, proto, netif) \
        ip4_output_if(p, src, dest, ttl, tos, proto, netif)
#define ip_output_if_src(p, src, dest, ttl, tos, proto, netif) \
        ip4_output_if_src(p, src, dest, ttl, tos, proto, netif)
#define ip_output_hinted(p, src, dest, ttl, tos, proto, netif_hint) \
        ip4_output_hinted(p, src, dest, ttl, tos, proto, netif_hint)
#define ip_output_if_hdrincl(p, src, dest, netif) \
        ip4_output_if(p, src, LWIP_IP_HDRINCL, 0, 0, 0, netif)
#define ip_route(src, dest) \
        ip4_route_src(src, dest)
#define ip_netif_get_local_ip(netif, dest) \
        ip4_netif_get_local_ip(netif)
#define ip_debug_print(is_ipv6, p) ip4_debug_print(p)

#define ip_input ip4_input

#elif LWIP_IPV6 /* LWIP_IPV4 && LWIP_IPV6 */

#define ip_output(p, src, dest, ttl, tos, proto) \
        ip6_output(p, src, dest, ttl, tos, proto)
#define ip_output_if(p, src, dest, ttl, tos, proto, netif) \
        ip6_output_if(p, src, dest, ttl, tos, proto, netif)
#define ip_output_if_src(p, src, dest, ttl, tos, proto, netif) \
        ip6_output_if_src(p, src, dest, ttl, tos, proto, netif)
#define ip_output_hinted(p, src, dest, ttl, tos, proto, netif_hint) \
        ip6_output_hinted(p, src, dest, ttl, tos, proto, netif_hint)
#define ip_output_if_hdrincl(p, src, dest, netif) \
        ip6_output_if(p, src, LWIP_IP_HDRINCL, 0, 0, 0, netif)
#define ip_route(src, dest) \
        ip6_route(src, dest)
#define ip_netif_get_local_ip(netif, dest) \
        ip6_netif_get_local_ip(netif, dest)
#define ip_debug_print(is_ipv6, p) ip6_debug_print(p)

#define ip_input ip6_input

#endif /* LWIP_IPV6 */

#define ip_route_get_local_ip(src, dest, netif, ipaddr) do { \
  (netif) = ip_route(src, dest); \
  (ipaddr) = ip_netif_get_local_ip(netif, dest); \
}while(0)

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_IP_H */


