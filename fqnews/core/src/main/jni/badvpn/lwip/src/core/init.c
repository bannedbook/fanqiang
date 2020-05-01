/**
 * @file
 * Modules initialization
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
 */

#include "lwip/opt.h"

#include "lwip/init.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/igmp.h"
#include "lwip/dns.h"
#include "lwip/timeouts.h"
#include "lwip/etharp.h"
#include "lwip/ip6.h"
#include "lwip/nd6.h"
#include "lwip/mld6.h"
#include "lwip/api.h"

#include "netif/ppp/ppp_opts.h"
#include "netif/ppp/ppp_impl.h"

#ifndef LWIP_SKIP_PACKING_CHECK

#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct packed_struct_test {
  PACK_STRUCT_FLD_8(u8_t  dummy1);
  PACK_STRUCT_FIELD(u32_t dummy2);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif
#define PACKED_STRUCT_TEST_EXPECTED_SIZE 5

#endif

/* Compile-time sanity checks for configuration errors.
 * These can be done independently of LWIP_DEBUG, without penalty.
 */
#ifndef BYTE_ORDER
#error "BYTE_ORDER is not defined, you have to define it in your cc.h"
#endif
#if (!IP_SOF_BROADCAST && IP_SOF_BROADCAST_RECV)
#error "If you want to use broadcast filter per pcb on recv operations, you have to define IP_SOF_BROADCAST=1 in your lwipopts.h"
#endif
#if (!LWIP_UDP && LWIP_UDPLITE)
#error "If you want to use UDP Lite, you have to define LWIP_UDP=1 in your lwipopts.h"
#endif
#if (!LWIP_UDP && LWIP_DHCP)
#error "If you want to use DHCP, you have to define LWIP_UDP=1 in your lwipopts.h"
#endif
#if (!LWIP_UDP && !LWIP_RAW && LWIP_MULTICAST_TX_OPTIONS)
#error "If you want to use LWIP_MULTICAST_TX_OPTIONS, you have to define LWIP_UDP=1 and/or LWIP_RAW=1 in your lwipopts.h"
#endif
#if (!LWIP_UDP && LWIP_DNS)
#error "If you want to use DNS, you have to define LWIP_UDP=1 in your lwipopts.h"
#endif
#if !MEMP_MEM_MALLOC /* MEMP_NUM_* checks are disabled when not using the pool allocator */
#if (LWIP_ARP && ARP_QUEUEING && (MEMP_NUM_ARP_QUEUE<=0))
#error "If you want to use ARP Queueing, you have to define MEMP_NUM_ARP_QUEUE>=1 in your lwipopts.h"
#endif
#if (LWIP_RAW && (MEMP_NUM_RAW_PCB<=0))
#error "If you want to use RAW, you have to define MEMP_NUM_RAW_PCB>=1 in your lwipopts.h"
#endif
#if (LWIP_UDP && (MEMP_NUM_UDP_PCB<=0))
#error "If you want to use UDP, you have to define MEMP_NUM_UDP_PCB>=1 in your lwipopts.h"
#endif
#if (LWIP_TCP && (MEMP_NUM_TCP_PCB<=0))
#error "If you want to use TCP, you have to define MEMP_NUM_TCP_PCB>=1 in your lwipopts.h"
#endif
#if (LWIP_IGMP && (MEMP_NUM_IGMP_GROUP<=1))
#error "If you want to use IGMP, you have to define MEMP_NUM_IGMP_GROUP>1 in your lwipopts.h"
#endif
#if (LWIP_IGMP && !LWIP_MULTICAST_TX_OPTIONS)
#error "If you want to use IGMP, you have to define LWIP_MULTICAST_TX_OPTIONS==1 in your lwipopts.h"
#endif
#if (LWIP_IGMP && !LWIP_IPV4)
#error "IGMP needs LWIP_IPV4 enabled in your lwipopts.h"
#endif
#if ((LWIP_NETCONN || LWIP_SOCKET) && (MEMP_NUM_TCPIP_MSG_API<=0))
#error "If you want to use Sequential API, you have to define MEMP_NUM_TCPIP_MSG_API>=1 in your lwipopts.h"
#endif
/* There must be sufficient timeouts, taking into account requirements of the subsystems. */
#if LWIP_TIMERS && (MEMP_NUM_SYS_TIMEOUT < LWIP_NUM_SYS_TIMEOUT_INTERNAL)
#error "MEMP_NUM_SYS_TIMEOUT is too low to accomodate all required timeouts"
#endif
#if (IP_REASSEMBLY && (MEMP_NUM_REASSDATA > IP_REASS_MAX_PBUFS))
#error "MEMP_NUM_REASSDATA > IP_REASS_MAX_PBUFS doesn't make sense since each struct ip_reassdata must hold 2 pbufs at least!"
#endif
#endif /* !MEMP_MEM_MALLOC */
#if LWIP_WND_SCALE
#if (LWIP_TCP && (TCP_WND > 0xffffffff))
#error "If you want to use TCP, TCP_WND must fit in an u32_t, so, you have to reduce it in your lwipopts.h"
#endif
#if (LWIP_TCP && (TCP_RCV_SCALE > 14))
#error "The maximum valid window scale value is 14!"
#endif
#if (LWIP_TCP && (TCP_WND > (0xFFFFU << TCP_RCV_SCALE)))
#error "TCP_WND is bigger than the configured LWIP_WND_SCALE allows!"
#endif
#if (LWIP_TCP && ((TCP_WND >> TCP_RCV_SCALE) == 0))
#error "TCP_WND is too small for the configured LWIP_WND_SCALE (results in zero window)!"
#endif
#else /* LWIP_WND_SCALE */
#if (LWIP_TCP && (TCP_WND > 0xffff))
#error "If you want to use TCP, TCP_WND must fit in an u16_t, so, you have to reduce it in your lwipopts.h (or enable window scaling)"
#endif
#endif /* LWIP_WND_SCALE */
#if (LWIP_TCP && (TCP_SND_QUEUELEN > 0xffff))
#error "If you want to use TCP, TCP_SND_QUEUELEN must fit in an u16_t, so, you have to reduce it in your lwipopts.h"
#endif
#if (LWIP_TCP && (TCP_SND_QUEUELEN < 2))
#error "TCP_SND_QUEUELEN must be at least 2 for no-copy TCP writes to work"
#endif
#if (LWIP_TCP && ((TCP_MAXRTX > 12) || (TCP_SYNMAXRTX > 12)))
#error "If you want to use TCP, TCP_MAXRTX and TCP_SYNMAXRTX must less or equal to 12 (due to tcp_backoff table), so, you have to reduce them in your lwipopts.h"
#endif
#if (LWIP_TCP && TCP_LISTEN_BACKLOG && ((TCP_DEFAULT_LISTEN_BACKLOG < 0) || (TCP_DEFAULT_LISTEN_BACKLOG > 0xff)))
#error "If you want to use TCP backlog, TCP_DEFAULT_LISTEN_BACKLOG must fit into an u8_t"
#endif
#if (LWIP_TCP && LWIP_TCP_SACK_OUT && !TCP_QUEUE_OOSEQ)
#error "To use LWIP_TCP_SACK_OUT, TCP_QUEUE_OOSEQ needs to be enabled"
#endif
#if (LWIP_TCP && LWIP_TCP_SACK_OUT && (LWIP_TCP_MAX_SACK_NUM < 1))
#error "LWIP_TCP_MAX_SACK_NUM must be greater than 0"
#endif
#if (LWIP_NETIF_API && (NO_SYS==1))
#error "If you want to use NETIF API, you have to define NO_SYS=0 in your lwipopts.h"
#endif
#if ((LWIP_SOCKET || LWIP_NETCONN) && (NO_SYS==1))
#error "If you want to use Sequential API, you have to define NO_SYS=0 in your lwipopts.h"
#endif
#if (LWIP_PPP_API && (NO_SYS==1))
#error "If you want to use PPP API, you have to define NO_SYS=0 in your lwipopts.h"
#endif
#if (LWIP_PPP_API && (PPP_SUPPORT==0))
#error "If you want to use PPP API, you have to enable PPP_SUPPORT in your lwipopts.h"
#endif
#if (((!LWIP_DHCP) || (!LWIP_AUTOIP)) && LWIP_DHCP_AUTOIP_COOP)
#error "If you want to use DHCP/AUTOIP cooperation mode, you have to define LWIP_DHCP=1 and LWIP_AUTOIP=1 in your lwipopts.h"
#endif
#if (((!LWIP_DHCP) || (!LWIP_ARP)) && DHCP_DOES_ARP_CHECK)
#error "If you want to use DHCP ARP checking, you have to define LWIP_DHCP=1 and LWIP_ARP=1 in your lwipopts.h"
#endif
#if (!LWIP_ARP && LWIP_AUTOIP)
#error "If you want to use AUTOIP, you have to define LWIP_ARP=1 in your lwipopts.h"
#endif
#if (LWIP_TCP && ((LWIP_EVENT_API && LWIP_CALLBACK_API) || (!LWIP_EVENT_API && !LWIP_CALLBACK_API)))
#error "One and exactly one of LWIP_EVENT_API and LWIP_CALLBACK_API has to be enabled in your lwipopts.h"
#endif
#if (MEM_LIBC_MALLOC && MEM_USE_POOLS)
#error "MEM_LIBC_MALLOC and MEM_USE_POOLS may not both be simultaneously enabled in your lwipopts.h"
#endif
#if (MEM_USE_POOLS && !MEMP_USE_CUSTOM_POOLS)
#error "MEM_USE_POOLS requires custom pools (MEMP_USE_CUSTOM_POOLS) to be enabled in your lwipopts.h"
#endif
#if (PBUF_POOL_BUFSIZE <= MEM_ALIGNMENT)
#error "PBUF_POOL_BUFSIZE must be greater than MEM_ALIGNMENT or the offset may take the full first pbuf"
#endif
#if (DNS_LOCAL_HOSTLIST && !DNS_LOCAL_HOSTLIST_IS_DYNAMIC && !(defined(DNS_LOCAL_HOSTLIST_INIT)))
#error "you have to define define DNS_LOCAL_HOSTLIST_INIT {{'host1', 0x123}, {'host2', 0x234}} to initialize DNS_LOCAL_HOSTLIST"
#endif
#if PPP_SUPPORT && !PPPOS_SUPPORT && !PPPOE_SUPPORT && !PPPOL2TP_SUPPORT
#error "PPP_SUPPORT needs at least one of PPPOS_SUPPORT, PPPOE_SUPPORT or PPPOL2TP_SUPPORT turned on"
#endif
#if PPP_SUPPORT && !PPP_IPV4_SUPPORT && !PPP_IPV6_SUPPORT
#error "PPP_SUPPORT needs PPP_IPV4_SUPPORT and/or PPP_IPV6_SUPPORT turned on"
#endif
#if PPP_SUPPORT && PPP_IPV4_SUPPORT && !LWIP_IPV4
#error "PPP_IPV4_SUPPORT needs LWIP_IPV4 turned on"
#endif
#if PPP_SUPPORT && PPP_IPV6_SUPPORT && !LWIP_IPV6
#error "PPP_IPV6_SUPPORT needs LWIP_IPV6 turned on"
#endif
#if !LWIP_ETHERNET && (LWIP_ARP || PPPOE_SUPPORT)
#error "LWIP_ETHERNET needs to be turned on for LWIP_ARP or PPPOE_SUPPORT"
#endif
#if LWIP_TCPIP_CORE_LOCKING_INPUT && !LWIP_TCPIP_CORE_LOCKING
#error "When using LWIP_TCPIP_CORE_LOCKING_INPUT, LWIP_TCPIP_CORE_LOCKING must be enabled, too"
#endif
#if LWIP_TCP && LWIP_NETIF_TX_SINGLE_PBUF && !TCP_OVERSIZE
#error "LWIP_NETIF_TX_SINGLE_PBUF needs TCP_OVERSIZE enabled to create single-pbuf TCP packets"
#endif
#if LWIP_NETCONN && LWIP_TCP
#if NETCONN_COPY != TCP_WRITE_FLAG_COPY
#error "NETCONN_COPY != TCP_WRITE_FLAG_COPY"
#endif
#if NETCONN_MORE != TCP_WRITE_FLAG_MORE
#error "NETCONN_MORE != TCP_WRITE_FLAG_MORE"
#endif
#endif /* LWIP_NETCONN && LWIP_TCP */
#if LWIP_SOCKET
/* Check that the SO_* socket options and SOF_* lwIP-internal flags match */
#if SO_REUSEADDR != SOF_REUSEADDR
#error "WARNING: SO_REUSEADDR != SOF_REUSEADDR"
#endif
#if SO_KEEPALIVE != SOF_KEEPALIVE
#error "WARNING: SO_KEEPALIVE != SOF_KEEPALIVE"
#endif
#if SO_BROADCAST != SOF_BROADCAST
#error "WARNING: SO_BROADCAST != SOF_BROADCAST"
#endif
#endif /* LWIP_SOCKET */


/* Compile-time checks for deprecated options.
 */
#ifdef MEMP_NUM_TCPIP_MSG
#error "MEMP_NUM_TCPIP_MSG option is deprecated. Remove it from your lwipopts.h."
#endif
#ifdef TCP_REXMIT_DEBUG
#error "TCP_REXMIT_DEBUG option is deprecated. Remove it from your lwipopts.h."
#endif
#ifdef RAW_STATS
#error "RAW_STATS option is deprecated. Remove it from your lwipopts.h."
#endif
#ifdef ETHARP_QUEUE_FIRST
#error "ETHARP_QUEUE_FIRST option is deprecated. Remove it from your lwipopts.h."
#endif
#ifdef ETHARP_ALWAYS_INSERT
#error "ETHARP_ALWAYS_INSERT option is deprecated. Remove it from your lwipopts.h."
#endif
#if !NO_SYS && LWIP_TCPIP_CORE_LOCKING && LWIP_COMPAT_MUTEX && !defined(LWIP_COMPAT_MUTEX_ALLOWED)
#error "LWIP_COMPAT_MUTEX cannot prevent priority inversion. It is recommended to implement priority-aware mutexes. (Define LWIP_COMPAT_MUTEX_ALLOWED to disable this error.)"
#endif

#ifndef LWIP_DISABLE_TCP_SANITY_CHECKS
#define LWIP_DISABLE_TCP_SANITY_CHECKS  0
#endif
#ifndef LWIP_DISABLE_MEMP_SANITY_CHECKS
#define LWIP_DISABLE_MEMP_SANITY_CHECKS 0
#endif

/* MEMP sanity checks */
#if MEMP_MEM_MALLOC
#if !LWIP_DISABLE_MEMP_SANITY_CHECKS
#if LWIP_NETCONN || LWIP_SOCKET
#if !MEMP_NUM_NETCONN && LWIP_SOCKET
#error "lwip_sanity_check: WARNING: MEMP_NUM_NETCONN cannot be 0 when using sockets!"
#endif
#else /* MEMP_MEM_MALLOC */
#if MEMP_NUM_NETCONN > (MEMP_NUM_TCP_PCB+MEMP_NUM_TCP_PCB_LISTEN+MEMP_NUM_UDP_PCB+MEMP_NUM_RAW_PCB)
#error "lwip_sanity_check: WARNING: MEMP_NUM_NETCONN should be less than the sum of MEMP_NUM_{TCP,RAW,UDP}_PCB+MEMP_NUM_TCP_PCB_LISTEN. If you know what you are doing, define LWIP_DISABLE_MEMP_SANITY_CHECKS to 1 to disable this error."
#endif
#endif /* LWIP_NETCONN || LWIP_SOCKET */
#endif /* !LWIP_DISABLE_MEMP_SANITY_CHECKS */
#if MEM_USE_POOLS
#error "MEMP_MEM_MALLOC and MEM_USE_POOLS cannot be enabled at the same time"
#endif
#ifdef LWIP_HOOK_MEMP_AVAILABLE
#error "LWIP_HOOK_MEMP_AVAILABLE doesn't make sense with MEMP_MEM_MALLOC"
#endif
#endif /* MEMP_MEM_MALLOC */

/* TCP sanity checks */
#if !LWIP_DISABLE_TCP_SANITY_CHECKS
#if LWIP_TCP
#if !MEMP_MEM_MALLOC && (MEMP_NUM_TCP_SEG < TCP_SND_QUEUELEN)
#error "lwip_sanity_check: WARNING: MEMP_NUM_TCP_SEG should be at least as big as TCP_SND_QUEUELEN. If you know what you are doing, define LWIP_DISABLE_TCP_SANITY_CHECKS to 1 to disable this error."
#endif
#if TCP_SND_BUF < (2 * TCP_MSS)
#error "lwip_sanity_check: WARNING: TCP_SND_BUF must be at least as much as (2 * TCP_MSS) for things to work smoothly. If you know what you are doing, define LWIP_DISABLE_TCP_SANITY_CHECKS to 1 to disable this error."
#endif
#if TCP_SND_QUEUELEN < (2 * (TCP_SND_BUF / TCP_MSS))
#error "lwip_sanity_check: WARNING: TCP_SND_QUEUELEN must be at least as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work. If you know what you are doing, define LWIP_DISABLE_TCP_SANITY_CHECKS to 1 to disable this error."
#endif
#if TCP_SNDLOWAT >= TCP_SND_BUF
#error "lwip_sanity_check: WARNING: TCP_SNDLOWAT must be less than TCP_SND_BUF. If you know what you are doing, define LWIP_DISABLE_TCP_SANITY_CHECKS to 1 to disable this error."
#endif
#if TCP_SNDLOWAT >= (0xFFFF - (4 * TCP_MSS))
#error "lwip_sanity_check: WARNING: TCP_SNDLOWAT must at least be 4*MSS below u16_t overflow!"
#endif
#if TCP_SNDQUEUELOWAT >= TCP_SND_QUEUELEN
#error "lwip_sanity_check: WARNING: TCP_SNDQUEUELOWAT must be less than TCP_SND_QUEUELEN. If you know what you are doing, define LWIP_DISABLE_TCP_SANITY_CHECKS to 1 to disable this error."
#endif
#if !MEMP_MEM_MALLOC && PBUF_POOL_SIZE && (PBUF_POOL_BUFSIZE <= (PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN + PBUF_TRANSPORT_HLEN))
#error "lwip_sanity_check: WARNING: PBUF_POOL_BUFSIZE does not provide enough space for protocol headers. If you know what you are doing, define LWIP_DISABLE_TCP_SANITY_CHECKS to 1 to disable this error."
#endif
#if !MEMP_MEM_MALLOC && PBUF_POOL_SIZE && (TCP_WND > (PBUF_POOL_SIZE * (PBUF_POOL_BUFSIZE - (PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN + PBUF_TRANSPORT_HLEN))))
#error "lwip_sanity_check: WARNING: TCP_WND is larger than space provided by PBUF_POOL_SIZE * (PBUF_POOL_BUFSIZE - protocol headers). If you know what you are doing, define LWIP_DISABLE_TCP_SANITY_CHECKS to 1 to disable this error."
#endif
#if TCP_WND < TCP_MSS
#error "lwip_sanity_check: WARNING: TCP_WND is smaller than MSS. If you know what you are doing, define LWIP_DISABLE_TCP_SANITY_CHECKS to 1 to disable this error."
#endif
#endif /* LWIP_TCP */
#endif /* !LWIP_DISABLE_TCP_SANITY_CHECKS */

/**
 * @ingroup lwip_nosys
 * Initialize all modules.
 * Use this in NO_SYS mode. Use tcpip_init() otherwise.
 */
void
lwip_init(void)
{
#ifndef LWIP_SKIP_CONST_CHECK
  int a = 0;
  LWIP_UNUSED_ARG(a);
  LWIP_ASSERT("LWIP_CONST_CAST not implemented correctly. Check your lwIP port.", LWIP_CONST_CAST(void *, &a) == &a);
#endif
#ifndef LWIP_SKIP_PACKING_CHECK
  LWIP_ASSERT("Struct packing not implemented correctly. Check your lwIP port.", sizeof(struct packed_struct_test) == PACKED_STRUCT_TEST_EXPECTED_SIZE);
#endif

  /* Modules initialization */
  stats_init();
#if !NO_SYS
  sys_init();
#endif /* !NO_SYS */
  mem_init();
  memp_init();
  pbuf_init();
  netif_init();
#if LWIP_IPV4
  ip_init();
#if LWIP_ARP
  etharp_init();
#endif /* LWIP_ARP */
#endif /* LWIP_IPV4 */
#if LWIP_RAW
  raw_init();
#endif /* LWIP_RAW */
#if LWIP_UDP
  udp_init();
#endif /* LWIP_UDP */
#if LWIP_TCP
  tcp_init();
#endif /* LWIP_TCP */
#if LWIP_IGMP
  igmp_init();
#endif /* LWIP_IGMP */
#if LWIP_DNS
  dns_init();
#endif /* LWIP_DNS */
#if PPP_SUPPORT
  ppp_init();
#endif

#if LWIP_TIMERS
  sys_timeouts_init();
#endif /* LWIP_TIMERS */
}
