/**
 * @file
 * Statistics API (to be used from TCPIP thread)
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
#ifndef LWIP_HDR_STATS_H
#define LWIP_HDR_STATS_H

#include "lwip/opt.h"

#include "lwip/mem.h"
#include "lwip/memp.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LWIP_STATS

#ifndef LWIP_STATS_LARGE
#define LWIP_STATS_LARGE 0
#endif

#if LWIP_STATS_LARGE
#define STAT_COUNTER     u32_t
#define STAT_COUNTER_F   U32_F
#else
#define STAT_COUNTER     u16_t
#define STAT_COUNTER_F   U16_F
#endif

/** Protocol related stats */
struct stats_proto {
  STAT_COUNTER xmit;             /* Transmitted packets. */
  STAT_COUNTER recv;             /* Received packets. */
  STAT_COUNTER fw;               /* Forwarded packets. */
  STAT_COUNTER drop;             /* Dropped packets. */
  STAT_COUNTER chkerr;           /* Checksum error. */
  STAT_COUNTER lenerr;           /* Invalid length error. */
  STAT_COUNTER memerr;           /* Out of memory error. */
  STAT_COUNTER rterr;            /* Routing error. */
  STAT_COUNTER proterr;          /* Protocol error. */
  STAT_COUNTER opterr;           /* Error in options. */
  STAT_COUNTER err;              /* Misc error. */
  STAT_COUNTER cachehit;
};

/** IGMP stats */
struct stats_igmp {
  STAT_COUNTER xmit;             /* Transmitted packets. */
  STAT_COUNTER recv;             /* Received packets. */
  STAT_COUNTER drop;             /* Dropped packets. */
  STAT_COUNTER chkerr;           /* Checksum error. */
  STAT_COUNTER lenerr;           /* Invalid length error. */
  STAT_COUNTER memerr;           /* Out of memory error. */
  STAT_COUNTER proterr;          /* Protocol error. */
  STAT_COUNTER rx_v1;            /* Received v1 frames. */
  STAT_COUNTER rx_group;         /* Received group-specific queries. */
  STAT_COUNTER rx_general;       /* Received general queries. */
  STAT_COUNTER rx_report;        /* Received reports. */
  STAT_COUNTER tx_join;          /* Sent joins. */
  STAT_COUNTER tx_leave;         /* Sent leaves. */
  STAT_COUNTER tx_report;        /* Sent reports. */
};

/** Memory stats */
struct stats_mem {
#if defined(LWIP_DEBUG) || LWIP_STATS_DISPLAY
  const char *name;
#endif /* defined(LWIP_DEBUG) || LWIP_STATS_DISPLAY */
  STAT_COUNTER err;
  mem_size_t avail;
  mem_size_t used;
  mem_size_t max;
  STAT_COUNTER illegal;
};

/** System element stats */
struct stats_syselem {
  STAT_COUNTER used;
  STAT_COUNTER max;
  STAT_COUNTER err;
};

/** System stats */
struct stats_sys {
  struct stats_syselem sem;
  struct stats_syselem mutex;
  struct stats_syselem mbox;
};

/** SNMP MIB2 stats */
struct stats_mib2 {
  /* IP */
  u32_t ipinhdrerrors;
  u32_t ipinaddrerrors;
  u32_t ipinunknownprotos;
  u32_t ipindiscards;
  u32_t ipindelivers;
  u32_t ipoutrequests;
  u32_t ipoutdiscards;
  u32_t ipoutnoroutes;
  u32_t ipreasmoks;
  u32_t ipreasmfails;
  u32_t ipfragoks;
  u32_t ipfragfails;
  u32_t ipfragcreates;
  u32_t ipreasmreqds;
  u32_t ipforwdatagrams;
  u32_t ipinreceives;

  /* TCP */
  u32_t tcpactiveopens;
  u32_t tcppassiveopens;
  u32_t tcpattemptfails;
  u32_t tcpestabresets;
  u32_t tcpoutsegs;
  u32_t tcpretranssegs;
  u32_t tcpinsegs;
  u32_t tcpinerrs;
  u32_t tcpoutrsts;

  /* UDP */
  u32_t udpindatagrams;
  u32_t udpnoports;
  u32_t udpinerrors;
  u32_t udpoutdatagrams;

  /* ICMP */
  u32_t icmpinmsgs;
  u32_t icmpinerrors;
  u32_t icmpindestunreachs;
  u32_t icmpintimeexcds;
  u32_t icmpinparmprobs;
  u32_t icmpinsrcquenchs;
  u32_t icmpinredirects;
  u32_t icmpinechos;
  u32_t icmpinechoreps;
  u32_t icmpintimestamps;
  u32_t icmpintimestampreps;
  u32_t icmpinaddrmasks;
  u32_t icmpinaddrmaskreps;
  u32_t icmpoutmsgs;
  u32_t icmpouterrors;
  u32_t icmpoutdestunreachs;
  u32_t icmpouttimeexcds;
  u32_t icmpoutechos; /* can be incremented by user application ('ping') */
  u32_t icmpoutechoreps;
};

/**
 * @ingroup netif_mib2
 * SNMP MIB2 interface stats
 */
struct stats_mib2_netif_ctrs {
  /** The total number of octets received on the interface, including framing characters */
  u32_t ifinoctets;
  /** The number of packets, delivered by this sub-layer to a higher (sub-)layer, which were
   * not addressed to a multicast or broadcast address at this sub-layer */
  u32_t ifinucastpkts;
  /** The number of packets, delivered by this sub-layer to a higher (sub-)layer, which were
   * addressed to a multicast or broadcast address at this sub-layer */
  u32_t ifinnucastpkts;
  /** The number of inbound packets which were chosen to be discarded even though no errors had
   * been detected to prevent their being deliverable to a higher-layer protocol. One possible
   * reason for discarding such a packet could be to free up buffer space */
  u32_t ifindiscards;
  /** For packet-oriented interfaces, the number of inbound packets that contained errors
   * preventing them from being deliverable to a higher-layer protocol.  For character-
   * oriented or fixed-length interfaces, the number of inbound transmission units that
   * contained errors preventing them from being deliverable to a higher-layer protocol. */
  u32_t ifinerrors;
  /** For packet-oriented interfaces, the number of packets received via the interface which
   * were discarded because of an unknown or unsupported protocol.  For character-oriented
   * or fixed-length interfaces that support protocol multiplexing the number of transmission
   * units received via the interface which were discarded because of an unknown or unsupported
   * protocol. For any interface that does not support protocol multiplexing, this counter will
   * always be 0 */
  u32_t ifinunknownprotos;
  /** The total number of octets transmitted out of the interface, including framing characters. */
  u32_t ifoutoctets;
  /** The total number of packets that higher-level protocols requested be transmitted, and
   * which were not addressed to a multicast or broadcast address at this sub-layer, including
   * those that were discarded or not sent. */
  u32_t ifoutucastpkts;
  /** The total number of packets that higher-level protocols requested be transmitted, and which
   * were addressed to a multicast or broadcast address at this sub-layer, including
   * those that were discarded or not sent. */
  u32_t ifoutnucastpkts;
  /** The number of outbound packets which were chosen to be discarded even though no errors had
   * been detected to prevent their being transmitted.  One possible reason for discarding
   * such a packet could be to free up buffer space. */
  u32_t ifoutdiscards;
  /** For packet-oriented interfaces, the number of outbound packets that could not be transmitted
   * because of errors. For character-oriented or fixed-length interfaces, the number of outbound
   * transmission units that could not be transmitted because of errors. */
  u32_t ifouterrors;
};

/** lwIP stats container */
struct stats_ {
#if LINK_STATS
  /** Link level */
  struct stats_proto link;
#endif
#if ETHARP_STATS
  /** ARP */
  struct stats_proto etharp;
#endif
#if IPFRAG_STATS
  /** Fragmentation */
  struct stats_proto ip_frag;
#endif
#if IP_STATS
  /** IP */
  struct stats_proto ip;
#endif
#if ICMP_STATS
  /** ICMP */
  struct stats_proto icmp;
#endif
#if IGMP_STATS
  /** IGMP */
  struct stats_igmp igmp;
#endif
#if UDP_STATS
  /** UDP */
  struct stats_proto udp;
#endif
#if TCP_STATS
  /** TCP */
  struct stats_proto tcp;
#endif
#if MEM_STATS
  /** Heap */
  struct stats_mem mem;
#endif
#if MEMP_STATS
  /** Internal memory pools */
  struct stats_mem *memp[MEMP_MAX];
#endif
#if SYS_STATS
  /** System */
  struct stats_sys sys;
#endif
#if IP6_STATS
  /** IPv6 */
  struct stats_proto ip6;
#endif
#if ICMP6_STATS
  /** ICMP6 */
  struct stats_proto icmp6;
#endif
#if IP6_FRAG_STATS
  /** IPv6 fragmentation */
  struct stats_proto ip6_frag;
#endif
#if MLD6_STATS
  /** Multicast listener discovery */
  struct stats_igmp mld6;
#endif
#if ND6_STATS
  /** Neighbor discovery */
  struct stats_proto nd6;
#endif
#if MIB2_STATS
  /** SNMP MIB2 */
  struct stats_mib2 mib2;
#endif
};

/** Global variable containing lwIP internal statistics. Add this to your debugger's watchlist. */
extern struct stats_ lwip_stats;

/** Init statistics */
void stats_init(void);

#define STATS_INC(x) ++lwip_stats.x
#define STATS_DEC(x) --lwip_stats.x
#define STATS_INC_USED(x, y, type) do { lwip_stats.x.used = (type)(lwip_stats.x.used + y); \
                                if (lwip_stats.x.max < lwip_stats.x.used) { \
                                    lwip_stats.x.max = lwip_stats.x.used; \
                                } \
                             } while(0)
#define STATS_GET(x) lwip_stats.x
#else /* LWIP_STATS */
#define stats_init()
#define STATS_INC(x)
#define STATS_DEC(x)
#define STATS_INC_USED(x, y, type)
#endif /* LWIP_STATS */

#if TCP_STATS
#define TCP_STATS_INC(x) STATS_INC(x)
#define TCP_STATS_DISPLAY() stats_display_proto(&lwip_stats.tcp, "TCP")
#else
#define TCP_STATS_INC(x)
#define TCP_STATS_DISPLAY()
#endif

#if UDP_STATS
#define UDP_STATS_INC(x) STATS_INC(x)
#define UDP_STATS_DISPLAY() stats_display_proto(&lwip_stats.udp, "UDP")
#else
#define UDP_STATS_INC(x)
#define UDP_STATS_DISPLAY()
#endif

#if ICMP_STATS
#define ICMP_STATS_INC(x) STATS_INC(x)
#define ICMP_STATS_DISPLAY() stats_display_proto(&lwip_stats.icmp, "ICMP")
#else
#define ICMP_STATS_INC(x)
#define ICMP_STATS_DISPLAY()
#endif

#if IGMP_STATS
#define IGMP_STATS_INC(x) STATS_INC(x)
#define IGMP_STATS_DISPLAY() stats_display_igmp(&lwip_stats.igmp, "IGMP")
#else
#define IGMP_STATS_INC(x)
#define IGMP_STATS_DISPLAY()
#endif

#if IP_STATS
#define IP_STATS_INC(x) STATS_INC(x)
#define IP_STATS_DISPLAY() stats_display_proto(&lwip_stats.ip, "IP")
#else
#define IP_STATS_INC(x)
#define IP_STATS_DISPLAY()
#endif

#if IPFRAG_STATS
#define IPFRAG_STATS_INC(x) STATS_INC(x)
#define IPFRAG_STATS_DISPLAY() stats_display_proto(&lwip_stats.ip_frag, "IP_FRAG")
#else
#define IPFRAG_STATS_INC(x)
#define IPFRAG_STATS_DISPLAY()
#endif

#if ETHARP_STATS
#define ETHARP_STATS_INC(x) STATS_INC(x)
#define ETHARP_STATS_DISPLAY() stats_display_proto(&lwip_stats.etharp, "ETHARP")
#else
#define ETHARP_STATS_INC(x)
#define ETHARP_STATS_DISPLAY()
#endif

#if LINK_STATS
#define LINK_STATS_INC(x) STATS_INC(x)
#define LINK_STATS_DISPLAY() stats_display_proto(&lwip_stats.link, "LINK")
#else
#define LINK_STATS_INC(x)
#define LINK_STATS_DISPLAY()
#endif

#if MEM_STATS
#define MEM_STATS_AVAIL(x, y) lwip_stats.mem.x = y
#define MEM_STATS_INC(x) STATS_INC(mem.x)
#define MEM_STATS_INC_USED(x, y) STATS_INC_USED(mem, y, mem_size_t)
#define MEM_STATS_DEC_USED(x, y) lwip_stats.mem.x = (mem_size_t)((lwip_stats.mem.x) - (y))
#define MEM_STATS_DISPLAY() stats_display_mem(&lwip_stats.mem, "HEAP")
#else
#define MEM_STATS_AVAIL(x, y)
#define MEM_STATS_INC(x)
#define MEM_STATS_INC_USED(x, y)
#define MEM_STATS_DEC_USED(x, y)
#define MEM_STATS_DISPLAY()
#endif

 #if MEMP_STATS
#define MEMP_STATS_DEC(x, i) STATS_DEC(memp[i]->x)
#define MEMP_STATS_DISPLAY(i) stats_display_memp(lwip_stats.memp[i], i)
#define MEMP_STATS_GET(x, i) STATS_GET(memp[i]->x)
 #else
#define MEMP_STATS_DEC(x, i)
#define MEMP_STATS_DISPLAY(i)
#define MEMP_STATS_GET(x, i) 0
#endif

#if SYS_STATS
#define SYS_STATS_INC(x) STATS_INC(sys.x)
#define SYS_STATS_DEC(x) STATS_DEC(sys.x)
#define SYS_STATS_INC_USED(x) STATS_INC_USED(sys.x, 1, STAT_COUNTER)
#define SYS_STATS_DISPLAY() stats_display_sys(&lwip_stats.sys)
#else
#define SYS_STATS_INC(x)
#define SYS_STATS_DEC(x)
#define SYS_STATS_INC_USED(x)
#define SYS_STATS_DISPLAY()
#endif

#if IP6_STATS
#define IP6_STATS_INC(x) STATS_INC(x)
#define IP6_STATS_DISPLAY() stats_display_proto(&lwip_stats.ip6, "IPv6")
#else
#define IP6_STATS_INC(x)
#define IP6_STATS_DISPLAY()
#endif

#if ICMP6_STATS
#define ICMP6_STATS_INC(x) STATS_INC(x)
#define ICMP6_STATS_DISPLAY() stats_display_proto(&lwip_stats.icmp6, "ICMPv6")
#else
#define ICMP6_STATS_INC(x)
#define ICMP6_STATS_DISPLAY()
#endif

#if IP6_FRAG_STATS
#define IP6_FRAG_STATS_INC(x) STATS_INC(x)
#define IP6_FRAG_STATS_DISPLAY() stats_display_proto(&lwip_stats.ip6_frag, "IPv6 FRAG")
#else
#define IP6_FRAG_STATS_INC(x)
#define IP6_FRAG_STATS_DISPLAY()
#endif

#if MLD6_STATS
#define MLD6_STATS_INC(x) STATS_INC(x)
#define MLD6_STATS_DISPLAY() stats_display_igmp(&lwip_stats.mld6, "MLDv1")
#else
#define MLD6_STATS_INC(x)
#define MLD6_STATS_DISPLAY()
#endif

#if ND6_STATS
#define ND6_STATS_INC(x) STATS_INC(x)
#define ND6_STATS_DISPLAY() stats_display_proto(&lwip_stats.nd6, "ND")
#else
#define ND6_STATS_INC(x)
#define ND6_STATS_DISPLAY()
#endif

#if MIB2_STATS
#define MIB2_STATS_INC(x) STATS_INC(x)
#else
#define MIB2_STATS_INC(x)
#endif

/* Display of statistics */
#if LWIP_STATS_DISPLAY
void stats_display(void);
void stats_display_proto(struct stats_proto *proto, const char *name);
void stats_display_igmp(struct stats_igmp *igmp, const char *name);
void stats_display_mem(struct stats_mem *mem, const char *name);
void stats_display_memp(struct stats_mem *mem, int index);
void stats_display_sys(struct stats_sys *sys);
#else /* LWIP_STATS_DISPLAY */
#define stats_display()
#define stats_display_proto(proto, name)
#define stats_display_igmp(igmp, name)
#define stats_display_mem(mem, name)
#define stats_display_memp(mem, index)
#define stats_display_sys(sys)
#endif /* LWIP_STATS_DISPLAY */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_STATS_H */
