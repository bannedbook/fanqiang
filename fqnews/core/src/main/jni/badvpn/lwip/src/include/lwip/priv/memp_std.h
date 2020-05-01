/**
 * @file
 * lwIP internal memory pools (do not use in application code)
 * This file is deliberately included multiple times: once with empty
 * definition of LWIP_MEMPOOL() to handle all includes and multiple times
 * to build up various lists of mem pools.
 */

/*
 * SETUP: Make sure we define everything we will need.
 *
 * We have create three types of pools:
 *   1) MEMPOOL - standard pools
 *   2) MALLOC_MEMPOOL - to be used by mem_malloc in mem.c
 *   3) PBUF_MEMPOOL - a mempool of pbuf's, so include space for the pbuf struct
 *
 * If the include'r doesn't require any special treatment of each of the types
 * above, then will declare #2 & #3 to be just standard mempools.
 */
#ifndef LWIP_MALLOC_MEMPOOL
/* This treats "malloc pools" just like any other pool.
   The pools are a little bigger to provide 'size' as the amount of user data. */
#define LWIP_MALLOC_MEMPOOL(num, size) LWIP_MEMPOOL(POOL_##size, num, (size + LWIP_MEM_ALIGN_SIZE(sizeof(struct memp_malloc_helper))), "MALLOC_"#size)
#define LWIP_MALLOC_MEMPOOL_START
#define LWIP_MALLOC_MEMPOOL_END
#endif /* LWIP_MALLOC_MEMPOOL */

#ifndef LWIP_PBUF_MEMPOOL
/* This treats "pbuf pools" just like any other pool.
 * Allocates buffers for a pbuf struct AND a payload size */
#define LWIP_PBUF_MEMPOOL(name, num, payload, desc) LWIP_MEMPOOL(name, num, (MEMP_ALIGN_SIZE(sizeof(struct pbuf)) + MEMP_ALIGN_SIZE(payload)), desc)
#endif /* LWIP_PBUF_MEMPOOL */


/*
 * A list of internal pools used by LWIP.
 *
 * LWIP_MEMPOOL(pool_name, number_elements, element_size, pool_description)
 *     creates a pool name MEMP_pool_name. description is used in stats.c
 */
#if LWIP_RAW
LWIP_MEMPOOL(RAW_PCB,        MEMP_NUM_RAW_PCB,         sizeof(struct raw_pcb),        "RAW_PCB")
#endif /* LWIP_RAW */

#if LWIP_UDP
LWIP_MEMPOOL(UDP_PCB,        MEMP_NUM_UDP_PCB,         sizeof(struct udp_pcb),        "UDP_PCB")
#endif /* LWIP_UDP */

#if LWIP_TCP
LWIP_MEMPOOL(TCP_PCB,        MEMP_NUM_TCP_PCB,         sizeof(struct tcp_pcb),        "TCP_PCB")
LWIP_MEMPOOL(TCP_PCB_LISTEN, MEMP_NUM_TCP_PCB_LISTEN,  sizeof(struct tcp_pcb_listen), "TCP_PCB_LISTEN")
LWIP_MEMPOOL(TCP_SEG,        MEMP_NUM_TCP_SEG,         sizeof(struct tcp_seg),        "TCP_SEG")
#endif /* LWIP_TCP */

#if LWIP_ALTCP && LWIP_TCP
LWIP_MEMPOOL(ALTCP_PCB,      MEMP_NUM_ALTCP_PCB,       sizeof(struct altcp_pcb),      "ALTCP_PCB")
#endif /* LWIP_ALTCP && LWIP_TCP */

#if LWIP_IPV4 && IP_REASSEMBLY
LWIP_MEMPOOL(REASSDATA,      MEMP_NUM_REASSDATA,       sizeof(struct ip_reassdata),   "REASSDATA")
#endif /* LWIP_IPV4 && IP_REASSEMBLY */
#if (IP_FRAG && !LWIP_NETIF_TX_SINGLE_PBUF) || (LWIP_IPV6 && LWIP_IPV6_FRAG)
LWIP_MEMPOOL(FRAG_PBUF,      MEMP_NUM_FRAG_PBUF,       sizeof(struct pbuf_custom_ref),"FRAG_PBUF")
#endif /* IP_FRAG && !LWIP_NETIF_TX_SINGLE_PBUF || (LWIP_IPV6 && LWIP_IPV6_FRAG) */

#if LWIP_NETCONN || LWIP_SOCKET
LWIP_MEMPOOL(NETBUF,         MEMP_NUM_NETBUF,          sizeof(struct netbuf),         "NETBUF")
LWIP_MEMPOOL(NETCONN,        MEMP_NUM_NETCONN,         sizeof(struct netconn),        "NETCONN")
#endif /* LWIP_NETCONN || LWIP_SOCKET */

#if NO_SYS==0
LWIP_MEMPOOL(TCPIP_MSG_API,  MEMP_NUM_TCPIP_MSG_API,   sizeof(struct tcpip_msg),      "TCPIP_MSG_API")
#if LWIP_MPU_COMPATIBLE
LWIP_MEMPOOL(API_MSG,        MEMP_NUM_API_MSG,         sizeof(struct api_msg),        "API_MSG")
#if LWIP_DNS
LWIP_MEMPOOL(DNS_API_MSG,    MEMP_NUM_DNS_API_MSG,     sizeof(struct dns_api_msg),    "DNS_API_MSG")
#endif
#if LWIP_SOCKET && !LWIP_TCPIP_CORE_LOCKING
LWIP_MEMPOOL(SOCKET_SETGETSOCKOPT_DATA, MEMP_NUM_SOCKET_SETGETSOCKOPT_DATA, sizeof(struct lwip_setgetsockopt_data), "SOCKET_SETGETSOCKOPT_DATA")
#endif
#if LWIP_SOCKET && (LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL)
LWIP_MEMPOOL(SELECT_CB,      MEMP_NUM_SELECT_CB,       sizeof(struct lwip_select_cb), "SELECT_CB")
#endif /* LWIP_SOCKET && (LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL) */
#if LWIP_NETIF_API
LWIP_MEMPOOL(NETIFAPI_MSG,   MEMP_NUM_NETIFAPI_MSG,    sizeof(struct netifapi_msg),   "NETIFAPI_MSG")
#endif
#endif /* LWIP_MPU_COMPATIBLE */
#if !LWIP_TCPIP_CORE_LOCKING_INPUT
LWIP_MEMPOOL(TCPIP_MSG_INPKT,MEMP_NUM_TCPIP_MSG_INPKT, sizeof(struct tcpip_msg),      "TCPIP_MSG_INPKT")
#endif /* !LWIP_TCPIP_CORE_LOCKING_INPUT */
#endif /* NO_SYS==0 */

#if LWIP_IPV4 && LWIP_ARP && ARP_QUEUEING
LWIP_MEMPOOL(ARP_QUEUE,      MEMP_NUM_ARP_QUEUE,       sizeof(struct etharp_q_entry), "ARP_QUEUE")
#endif /* LWIP_IPV4 && LWIP_ARP && ARP_QUEUEING */

#if LWIP_IGMP
LWIP_MEMPOOL(IGMP_GROUP,     MEMP_NUM_IGMP_GROUP,      sizeof(struct igmp_group),     "IGMP_GROUP")
#endif /* LWIP_IGMP */

#if LWIP_TIMERS && !LWIP_TIMERS_CUSTOM
LWIP_MEMPOOL(SYS_TIMEOUT,    MEMP_NUM_SYS_TIMEOUT,     sizeof(struct sys_timeo),      "SYS_TIMEOUT")
#endif /* LWIP_TIMERS && !LWIP_TIMERS_CUSTOM */

#if LWIP_DNS && LWIP_SOCKET
LWIP_MEMPOOL(NETDB,          MEMP_NUM_NETDB,           NETDB_ELEM_SIZE,               "NETDB")
#endif /* LWIP_DNS && LWIP_SOCKET */
#if LWIP_DNS && DNS_LOCAL_HOSTLIST && DNS_LOCAL_HOSTLIST_IS_DYNAMIC
LWIP_MEMPOOL(LOCALHOSTLIST,  MEMP_NUM_LOCALHOSTLIST,   LOCALHOSTLIST_ELEM_SIZE,       "LOCALHOSTLIST")
#endif /* LWIP_DNS && DNS_LOCAL_HOSTLIST && DNS_LOCAL_HOSTLIST_IS_DYNAMIC */

#if LWIP_IPV6 && LWIP_ND6_QUEUEING
LWIP_MEMPOOL(ND6_QUEUE,      MEMP_NUM_ND6_QUEUE,       sizeof(struct nd6_q_entry), "ND6_QUEUE")
#endif /* LWIP_IPV6 && LWIP_ND6_QUEUEING */

#if LWIP_IPV6 && LWIP_IPV6_REASS
LWIP_MEMPOOL(IP6_REASSDATA,      MEMP_NUM_REASSDATA,       sizeof(struct ip6_reassdata),   "IP6_REASSDATA")
#endif /* LWIP_IPV6 && LWIP_IPV6_REASS */

#if LWIP_IPV6 && LWIP_IPV6_MLD
LWIP_MEMPOOL(MLD6_GROUP,     MEMP_NUM_MLD6_GROUP,      sizeof(struct mld_group),     "MLD6_GROUP")
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */


/*
 * A list of pools of pbuf's used by LWIP.
 *
 * LWIP_PBUF_MEMPOOL(pool_name, number_elements, pbuf_payload_size, pool_description)
 *     creates a pool name MEMP_pool_name. description is used in stats.c
 *     This allocates enough space for the pbuf struct and a payload.
 *     (Example: pbuf_payload_size=0 allocates only size for the struct)
 */
LWIP_PBUF_MEMPOOL(PBUF,      MEMP_NUM_PBUF,            0,                             "PBUF_REF/ROM")
LWIP_PBUF_MEMPOOL(PBUF_POOL, PBUF_POOL_SIZE,           PBUF_POOL_BUFSIZE,             "PBUF_POOL")


/*
 * Allow for user-defined pools; this must be explicitly set in lwipopts.h
 * since the default is to NOT look for lwippools.h
 */
#if MEMP_USE_CUSTOM_POOLS
#include "lwippools.h"
#endif /* MEMP_USE_CUSTOM_POOLS */

/*
 * REQUIRED CLEANUP: Clear up so we don't get "multiply defined" error later
 * (#undef is ignored for something that is not defined)
 */
#undef LWIP_MEMPOOL
#undef LWIP_MALLOC_MEMPOOL
#undef LWIP_MALLOC_MEMPOOL_START
#undef LWIP_MALLOC_MEMPOOL_END
#undef LWIP_PBUF_MEMPOOL
