/**
 * @file
 *
 * lwIP Options Configuration
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

/*
 * NOTE: || defined __DOXYGEN__ is a workaround for doxygen bug -
 * without this, doxygen does not see the actual #define
 */

#if !defined LWIP_HDR_OPT_H
#define LWIP_HDR_OPT_H

/*
 * Include user defined options first. Anything not defined in these files
 * will be set to standard values. Override anything you don't like!
 */
#include "lwipopts.h"
#include "lwip/debug.h"

/**
 * @defgroup lwip_opts Options (lwipopts.h)
 * @ingroup lwip
 *
 * @defgroup lwip_opts_debug Debugging
 * @ingroup lwip_opts
 *
 * @defgroup lwip_opts_infrastructure Infrastructure
 * @ingroup lwip_opts
 *
 * @defgroup lwip_opts_callback Callback-style APIs
 * @ingroup lwip_opts
 *
 * @defgroup lwip_opts_threadsafe_apis Thread-safe APIs
 * @ingroup lwip_opts
 */

 /*
   ------------------------------------
   -------------- NO SYS --------------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_nosys NO_SYS
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * NO_SYS==1: Use lwIP without OS-awareness (no thread, semaphores, mutexes or
 * mboxes). This means threaded APIs cannot be used (socket, netconn,
 * i.e. everything in the 'api' folder), only the callback-style raw API is
 * available (and you have to watch out for yourself that you don't access
 * lwIP functions/structures from more than one context at a time!)
 */
#if !defined NO_SYS || defined __DOXYGEN__
#define NO_SYS                          0
#endif
/**
 * @}
 */

/**
 * @defgroup lwip_opts_timers Timers
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * LWIP_TIMERS==0: Drop support for sys_timeout and lwip-internal cyclic timers.
 * (the array of lwip-internal cyclic timers is still provided)
 * (check NO_SYS_NO_TIMERS for compatibility to old versions)
 */
#if !defined LWIP_TIMERS || defined __DOXYGEN__
#ifdef NO_SYS_NO_TIMERS
#define LWIP_TIMERS                     (!NO_SYS || (NO_SYS && !NO_SYS_NO_TIMERS))
#else
#define LWIP_TIMERS                     1
#endif
#endif

/**
 * LWIP_TIMERS_CUSTOM==1: Provide your own timer implementation.
 * Function prototypes in timeouts.h and the array of lwip-internal cyclic timers
 * are still included, but the implementation is not. The following functions
 * will be required: sys_timeouts_init(), sys_timeout(), sys_untimeout(),
 *                   sys_timeouts_mbox_fetch()
 */
#if !defined LWIP_TIMERS_CUSTOM || defined __DOXYGEN__
#define LWIP_TIMERS_CUSTOM              0
#endif
/**
 * @}
 */

/**
 * @defgroup lwip_opts_memcpy memcpy
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * MEMCPY: override this if you have a faster implementation at hand than the
 * one included in your C library
 */
#if !defined MEMCPY || defined __DOXYGEN__
#define MEMCPY(dst,src,len)             memcpy(dst,src,len)
#endif

/**
 * SMEMCPY: override this with care! Some compilers (e.g. gcc) can inline a
 * call to memcpy() if the length is known at compile time and is small.
 */
#if !defined SMEMCPY || defined __DOXYGEN__
#define SMEMCPY(dst,src,len)            memcpy(dst,src,len)
#endif

/**
 * MEMMOVE: override this if you have a faster implementation at hand than the
 * one included in your C library.  lwIP currently uses MEMMOVE only when IPv6
 * fragmentation support is enabled.
 */
#if !defined MEMMOVE || defined __DOXYGEN__
#define MEMMOVE(dst,src,len)            memmove(dst,src,len)
#endif
/**
 * @}
 */

/*
   ------------------------------------
   ----------- Core locking -----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_lock Core locking and MPU
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * LWIP_MPU_COMPATIBLE: enables special memory management mechanism
 * which makes lwip able to work on MPU (Memory Protection Unit) system
 * by not passing stack-pointers to other threads
 * (this decreases performance as memory is allocated from pools instead
 * of keeping it on the stack)
 */
#if !defined LWIP_MPU_COMPATIBLE || defined __DOXYGEN__
#define LWIP_MPU_COMPATIBLE             0
#endif

/**
 * LWIP_TCPIP_CORE_LOCKING
 * Creates a global mutex that is held during TCPIP thread operations.
 * Can be locked by client code to perform lwIP operations without changing
 * into TCPIP thread using callbacks. See LOCK_TCPIP_CORE() and
 * UNLOCK_TCPIP_CORE().
 * Your system should provide mutexes supporting priority inversion to use this.
 */
#if !defined LWIP_TCPIP_CORE_LOCKING || defined __DOXYGEN__
#define LWIP_TCPIP_CORE_LOCKING         1
#endif

/**
 * LWIP_TCPIP_CORE_LOCKING_INPUT: when LWIP_TCPIP_CORE_LOCKING is enabled,
 * this lets tcpip_input() grab the mutex for input packets as well,
 * instead of allocating a message and passing it to tcpip_thread.
 *
 * ATTENTION: this does not work when tcpip_input() is called from
 * interrupt context!
 */
#if !defined LWIP_TCPIP_CORE_LOCKING_INPUT || defined __DOXYGEN__
#define LWIP_TCPIP_CORE_LOCKING_INPUT   0
#endif

/**
 * SYS_LIGHTWEIGHT_PROT==1: enable inter-task protection (and task-vs-interrupt
 * protection) for certain critical regions during buffer allocation, deallocation
 * and memory allocation and deallocation.
 * ATTENTION: This is required when using lwIP from more than one context! If
 * you disable this, you must be sure what you are doing!
 */
#if !defined SYS_LIGHTWEIGHT_PROT || defined __DOXYGEN__
#define SYS_LIGHTWEIGHT_PROT            1
#endif
/**
 * @}
 */

/*
   ------------------------------------
   ---------- Memory options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_mem Heap and memory pools
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
 * instead of the lwip internal allocator. Can save code size if you
 * already use it.
 */
#if !defined MEM_LIBC_MALLOC || defined __DOXYGEN__
#define MEM_LIBC_MALLOC                 0
#endif

/**
 * MEMP_MEM_MALLOC==1: Use mem_malloc/mem_free instead of the lwip pool allocator.
 * Especially useful with MEM_LIBC_MALLOC but handle with care regarding execution
 * speed (heap alloc can be much slower than pool alloc) and usage from interrupts
 * (especially if your netif driver allocates PBUF_POOL pbufs for received frames
 * from interrupt)!
 * ATTENTION: Currently, this uses the heap for ALL pools (also for private pools,
 * not only for internal pools defined in memp_std.h)!
 */
#if !defined MEMP_MEM_MALLOC || defined __DOXYGEN__
#define MEMP_MEM_MALLOC                 0
#endif

/**
 * MEMP_MEM_INIT==1: Force use of memset to initialize pool memory.
 * Useful if pool are moved in uninitialized section of memory. This will ensure
 * default values in pcbs struct are well initialized in all conditions.
 */
#if !defined MEMP_MEM_INIT || defined __DOXYGEN__
#define MEMP_MEM_INIT                 0
#endif

/**
 * MEM_ALIGNMENT: should be set to the alignment of the CPU
 *    4 byte alignment -> \#define MEM_ALIGNMENT 4
 *    2 byte alignment -> \#define MEM_ALIGNMENT 2
 */
#if !defined MEM_ALIGNMENT || defined __DOXYGEN__
#define MEM_ALIGNMENT                   1
#endif

/**
 * MEM_SIZE: the size of the heap memory. If the application will send
 * a lot of data that needs to be copied, this should be set high.
 */
#if !defined MEM_SIZE || defined __DOXYGEN__
#define MEM_SIZE                        1600
#endif

/**
 * MEMP_OVERFLOW_CHECK: memp overflow protection reserves a configurable
 * amount of bytes before and after each memp element in every pool and fills
 * it with a prominent default value.
 *    MEMP_OVERFLOW_CHECK == 0 no checking
 *    MEMP_OVERFLOW_CHECK == 1 checks each element when it is freed
 *    MEMP_OVERFLOW_CHECK >= 2 checks each element in every pool every time
 *      memp_malloc() or memp_free() is called (useful but slow!)
 */
#if !defined MEMP_OVERFLOW_CHECK || defined __DOXYGEN__
#define MEMP_OVERFLOW_CHECK             0
#endif

/**
 * MEMP_SANITY_CHECK==1: run a sanity check after each memp_free() to make
 * sure that there are no cycles in the linked lists.
 */
#if !defined MEMP_SANITY_CHECK || defined __DOXYGEN__
#define MEMP_SANITY_CHECK               0
#endif

/**
 * MEM_USE_POOLS==1: Use an alternative to malloc() by allocating from a set
 * of memory pools of various sizes. When mem_malloc is called, an element of
 * the smallest pool that can provide the length needed is returned.
 * To use this, MEMP_USE_CUSTOM_POOLS also has to be enabled.
 */
#if !defined MEM_USE_POOLS || defined __DOXYGEN__
#define MEM_USE_POOLS                   0
#endif

/**
 * MEM_USE_POOLS_TRY_BIGGER_POOL==1: if one malloc-pool is empty, try the next
 * bigger pool - WARNING: THIS MIGHT WASTE MEMORY but it can make a system more
 * reliable. */
#if !defined MEM_USE_POOLS_TRY_BIGGER_POOL || defined __DOXYGEN__
#define MEM_USE_POOLS_TRY_BIGGER_POOL   0
#endif

/**
 * MEMP_USE_CUSTOM_POOLS==1: whether to include a user file lwippools.h
 * that defines additional pools beyond the "standard" ones required
 * by lwIP. If you set this to 1, you must have lwippools.h in your
 * include path somewhere.
 */
#if !defined MEMP_USE_CUSTOM_POOLS || defined __DOXYGEN__
#define MEMP_USE_CUSTOM_POOLS           0
#endif

/**
 * Set this to 1 if you want to free PBUF_RAM pbufs (or call mem_free()) from
 * interrupt context (or another context that doesn't allow waiting for a
 * semaphore).
 * If set to 1, mem_malloc will be protected by a semaphore and SYS_ARCH_PROTECT,
 * while mem_free will only use SYS_ARCH_PROTECT. mem_malloc SYS_ARCH_UNPROTECTs
 * with each loop so that mem_free can run.
 *
 * ATTENTION: As you can see from the above description, this leads to dis-/
 * enabling interrupts often, which can be slow! Also, on low memory, mem_malloc
 * can need longer.
 *
 * If you don't want that, at least for NO_SYS=0, you can still use the following
 * functions to enqueue a deallocation call which then runs in the tcpip_thread
 * context:
 * - pbuf_free_callback(p);
 * - mem_free_callback(m);
 */
#if !defined LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT || defined __DOXYGEN__
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 0
#endif
/**
 * @}
 */

/*
   ------------------------------------------------
   ---------- Internal Memory Pool Sizes ----------
   ------------------------------------------------
*/
/**
 * @defgroup lwip_opts_memp Internal memory pools
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * MEMP_NUM_PBUF: the number of memp struct pbufs (used for PBUF_ROM and PBUF_REF).
 * If the application sends a lot of data out of ROM (or other static memory),
 * this should be set high.
 */
#if !defined MEMP_NUM_PBUF || defined __DOXYGEN__
#define MEMP_NUM_PBUF                   16
#endif

/**
 * MEMP_NUM_RAW_PCB: Number of raw connection PCBs
 * (requires the LWIP_RAW option)
 */
#if !defined MEMP_NUM_RAW_PCB || defined __DOXYGEN__
#define MEMP_NUM_RAW_PCB                4
#endif

/**
 * MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
 * per active UDP "connection".
 * (requires the LWIP_UDP option)
 */
#if !defined MEMP_NUM_UDP_PCB || defined __DOXYGEN__
#define MEMP_NUM_UDP_PCB                4
#endif

/**
 * MEMP_NUM_TCP_PCB: the number of simultaneously active TCP connections.
 * (requires the LWIP_TCP option)
 */
#if !defined MEMP_NUM_TCP_PCB || defined __DOXYGEN__
#define MEMP_NUM_TCP_PCB                5
#endif

/**
 * MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP connections.
 * (requires the LWIP_TCP option)
 */
#if !defined MEMP_NUM_TCP_PCB_LISTEN || defined __DOXYGEN__
#define MEMP_NUM_TCP_PCB_LISTEN         8
#endif

/**
 * MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments.
 * (requires the LWIP_TCP option)
 */
#if !defined MEMP_NUM_TCP_SEG || defined __DOXYGEN__
#define MEMP_NUM_TCP_SEG                16
#endif

/**
 * MEMP_NUM_ALTCP_PCB: the number of simultaneously active altcp layer pcbs.
 * (requires the LWIP_ALTCP option)
 * Connections with multiple layers require more than one altcp_pcb (e.g. TLS
 * over TCP requires 2 altcp_pcbs, one for TLS and one for TCP).
 */
#if !defined MEMP_NUM_ALTCP_PCB || defined __DOXYGEN__
#define MEMP_NUM_ALTCP_PCB              MEMP_NUM_TCP_PCB
#endif

/**
 * MEMP_NUM_REASSDATA: the number of IP packets simultaneously queued for
 * reassembly (whole packets, not fragments!)
 */
#if !defined MEMP_NUM_REASSDATA || defined __DOXYGEN__
#define MEMP_NUM_REASSDATA              5
#endif

/**
 * MEMP_NUM_FRAG_PBUF: the number of IP fragments simultaneously sent
 * (fragments, not whole packets!).
 * This is only used with LWIP_NETIF_TX_SINGLE_PBUF==0 and only has to be > 1
 * with DMA-enabled MACs where the packet is not yet sent when netif->output
 * returns.
 */
#if !defined MEMP_NUM_FRAG_PBUF || defined __DOXYGEN__
#define MEMP_NUM_FRAG_PBUF              15
#endif

/**
 * MEMP_NUM_ARP_QUEUE: the number of simultaneously queued outgoing
 * packets (pbufs) that are waiting for an ARP request (to resolve
 * their destination address) to finish.
 * (requires the ARP_QUEUEING option)
 */
#if !defined MEMP_NUM_ARP_QUEUE || defined __DOXYGEN__
#define MEMP_NUM_ARP_QUEUE              30
#endif

/**
 * MEMP_NUM_IGMP_GROUP: The number of multicast groups whose network interfaces
 * can be members at the same time (one per netif - allsystems group -, plus one
 * per netif membership).
 * (requires the LWIP_IGMP option)
 */
#if !defined MEMP_NUM_IGMP_GROUP || defined __DOXYGEN__
#define MEMP_NUM_IGMP_GROUP             8
#endif

/**
 * The number of sys timeouts used by the core stack (not apps)
 * The default number of timeouts is calculated here for all enabled modules.
 */
#define LWIP_NUM_SYS_TIMEOUT_INTERNAL   (LWIP_TCP + IP_REASSEMBLY + LWIP_ARP + (2*LWIP_DHCP) + LWIP_AUTOIP + LWIP_IGMP + LWIP_DNS + PPP_NUM_TIMEOUTS + (LWIP_IPV6 * (1 + LWIP_IPV6_REASS + LWIP_IPV6_MLD)))

/**
 * MEMP_NUM_SYS_TIMEOUT: the number of simultaneously active timeouts.
 * The default number of timeouts is calculated here for all enabled modules.
 * The formula expects settings to be either '0' or '1'.
 */
#if !defined MEMP_NUM_SYS_TIMEOUT || defined __DOXYGEN__
#define MEMP_NUM_SYS_TIMEOUT            LWIP_NUM_SYS_TIMEOUT_INTERNAL
#endif

/**
 * MEMP_NUM_NETBUF: the number of struct netbufs.
 * (only needed if you use the sequential API, like api_lib.c)
 */
#if !defined MEMP_NUM_NETBUF || defined __DOXYGEN__
#define MEMP_NUM_NETBUF                 2
#endif

/**
 * MEMP_NUM_NETCONN: the number of struct netconns.
 * (only needed if you use the sequential API, like api_lib.c)
 */
#if !defined MEMP_NUM_NETCONN || defined __DOXYGEN__
#define MEMP_NUM_NETCONN                4
#endif

/**
 * MEMP_NUM_SELECT_CB: the number of struct lwip_select_cb.
 * (Only needed if you have LWIP_MPU_COMPATIBLE==1 and use the socket API.
 * In that case, you need one per thread calling lwip_select.)
 */
#if !defined MEMP_NUM_SELECT_CB || defined __DOXYGEN__
#define MEMP_NUM_SELECT_CB              4
#endif

/**
 * MEMP_NUM_TCPIP_MSG_API: the number of struct tcpip_msg, which are used
 * for callback/timeout API communication.
 * (only needed if you use tcpip.c)
 */
#if !defined MEMP_NUM_TCPIP_MSG_API || defined __DOXYGEN__
#define MEMP_NUM_TCPIP_MSG_API          8
#endif

/**
 * MEMP_NUM_TCPIP_MSG_INPKT: the number of struct tcpip_msg, which are used
 * for incoming packets.
 * (only needed if you use tcpip.c)
 */
#if !defined MEMP_NUM_TCPIP_MSG_INPKT || defined __DOXYGEN__
#define MEMP_NUM_TCPIP_MSG_INPKT        8
#endif

/**
 * MEMP_NUM_NETDB: the number of concurrently running lwip_addrinfo() calls
 * (before freeing the corresponding memory using lwip_freeaddrinfo()).
 */
#if !defined MEMP_NUM_NETDB || defined __DOXYGEN__
#define MEMP_NUM_NETDB                  1
#endif

/**
 * MEMP_NUM_LOCALHOSTLIST: the number of host entries in the local host list
 * if DNS_LOCAL_HOSTLIST_IS_DYNAMIC==1.
 */
#if !defined MEMP_NUM_LOCALHOSTLIST || defined __DOXYGEN__
#define MEMP_NUM_LOCALHOSTLIST          1
#endif

/**
 * PBUF_POOL_SIZE: the number of buffers in the pbuf pool.
 */
#if !defined PBUF_POOL_SIZE || defined __DOXYGEN__
#define PBUF_POOL_SIZE                  16
#endif

/** MEMP_NUM_API_MSG: the number of concurrently active calls to various
 * socket, netconn, and tcpip functions
 */
#if !defined MEMP_NUM_API_MSG || defined __DOXYGEN__
#define MEMP_NUM_API_MSG                MEMP_NUM_TCPIP_MSG_API
#endif

/** MEMP_NUM_DNS_API_MSG: the number of concurrently active calls to netconn_gethostbyname
 */
#if !defined MEMP_NUM_DNS_API_MSG || defined __DOXYGEN__
#define MEMP_NUM_DNS_API_MSG            MEMP_NUM_TCPIP_MSG_API
#endif

/** MEMP_NUM_SOCKET_SETGETSOCKOPT_DATA: the number of concurrently active calls
 * to getsockopt/setsockopt
 */
#if !defined MEMP_NUM_SOCKET_SETGETSOCKOPT_DATA || defined __DOXYGEN__
#define MEMP_NUM_SOCKET_SETGETSOCKOPT_DATA MEMP_NUM_TCPIP_MSG_API
#endif

/** MEMP_NUM_NETIFAPI_MSG: the number of concurrently active calls to the
 * netifapi functions
 */
#if !defined MEMP_NUM_NETIFAPI_MSG || defined __DOXYGEN__
#define MEMP_NUM_NETIFAPI_MSG           MEMP_NUM_TCPIP_MSG_API
#endif
/**
 * @}
 */

/*
   ---------------------------------
   ---------- ARP options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_arp ARP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_ARP==1: Enable ARP functionality.
 */
#if !defined LWIP_ARP || defined __DOXYGEN__
#define LWIP_ARP                        1
#endif

/**
 * ARP_TABLE_SIZE: Number of active MAC-IP address pairs cached.
 */
#if !defined ARP_TABLE_SIZE || defined __DOXYGEN__
#define ARP_TABLE_SIZE                  10
#endif

/** the time an ARP entry stays valid after its last update,
 *  for ARP_TMR_INTERVAL = 1000, this is
 *  (60 * 5) seconds = 5 minutes.
 */
#if !defined ARP_MAXAGE || defined __DOXYGEN__
#define ARP_MAXAGE                      300
#endif

/**
 * ARP_QUEUEING==1: Multiple outgoing packets are queued during hardware address
 * resolution. By default, only the most recent packet is queued per IP address.
 * This is sufficient for most protocols and mainly reduces TCP connection
 * startup time. Set this to 1 if you know your application sends more than one
 * packet in a row to an IP address that is not in the ARP cache.
 */
#if !defined ARP_QUEUEING || defined __DOXYGEN__
#define ARP_QUEUEING                    0
#endif

/** The maximum number of packets which may be queued for each
 *  unresolved address by other network layers. Defaults to 3, 0 means disabled.
 *  Old packets are dropped, new packets are queued.
 */
#if !defined ARP_QUEUE_LEN || defined __DOXYGEN__
#define ARP_QUEUE_LEN                   3
#endif

/**
 * ETHARP_SUPPORT_VLAN==1: support receiving and sending ethernet packets with
 * VLAN header. See the description of LWIP_HOOK_VLAN_CHECK and
 * LWIP_HOOK_VLAN_SET hooks to check/set VLAN headers.
 * Additionally, you can define ETHARP_VLAN_CHECK to an u16_t VLAN ID to check.
 * If ETHARP_VLAN_CHECK is defined, only VLAN-traffic for this VLAN is accepted.
 * If ETHARP_VLAN_CHECK is not defined, all traffic is accepted.
 * Alternatively, define a function/define ETHARP_VLAN_CHECK_FN(eth_hdr, vlan)
 * that returns 1 to accept a packet or 0 to drop a packet.
 */
#if !defined ETHARP_SUPPORT_VLAN || defined __DOXYGEN__
#define ETHARP_SUPPORT_VLAN             0
#endif

/** LWIP_ETHERNET==1: enable ethernet support even though ARP might be disabled
 */
#if !defined LWIP_ETHERNET || defined __DOXYGEN__
#define LWIP_ETHERNET                   LWIP_ARP
#endif

/** ETH_PAD_SIZE: number of bytes added before the ethernet header to ensure
 * alignment of payload after that header. Since the header is 14 bytes long,
 * without this padding e.g. addresses in the IP header will not be aligned
 * on a 32-bit boundary, so setting this to 2 can speed up 32-bit-platforms.
 */
#if !defined ETH_PAD_SIZE || defined __DOXYGEN__
#define ETH_PAD_SIZE                    0
#endif

/** ETHARP_SUPPORT_STATIC_ENTRIES==1: enable code to support static ARP table
 * entries (using etharp_add_static_entry/etharp_remove_static_entry).
 */
#if !defined ETHARP_SUPPORT_STATIC_ENTRIES || defined __DOXYGEN__
#define ETHARP_SUPPORT_STATIC_ENTRIES   0
#endif

/** ETHARP_TABLE_MATCH_NETIF==1: Match netif for ARP table entries.
 * If disabled, duplicate IP address on multiple netifs are not supported
 * (but this should only occur for AutoIP).
 */
#if !defined ETHARP_TABLE_MATCH_NETIF || defined __DOXYGEN__
#define ETHARP_TABLE_MATCH_NETIF        !LWIP_SINGLE_NETIF
#endif
/**
 * @}
 */

/*
   --------------------------------
   ---------- IP options ----------
   --------------------------------
*/
/**
 * @defgroup lwip_opts_ipv4 IPv4
 * @ingroup lwip_opts
 * @{
 */
/**
 * LWIP_IPV4==1: Enable IPv4
 */
#if !defined LWIP_IPV4 || defined __DOXYGEN__
#define LWIP_IPV4                       1
#endif

/**
 * IP_FORWARD==1: Enables the ability to forward IP packets across network
 * interfaces. If you are going to run lwIP on a device with only one network
 * interface, define this to 0.
 */
#if !defined IP_FORWARD || defined __DOXYGEN__
#define IP_FORWARD                      0
#endif

/**
 * IP_REASSEMBLY==1: Reassemble incoming fragmented IP packets. Note that
 * this option does not affect outgoing packet sizes, which can be controlled
 * via IP_FRAG.
 */
#if !defined IP_REASSEMBLY || defined __DOXYGEN__
#define IP_REASSEMBLY                   1
#endif

/**
 * IP_FRAG==1: Fragment outgoing IP packets if their size exceeds MTU. Note
 * that this option does not affect incoming packet sizes, which can be
 * controlled via IP_REASSEMBLY.
 */
#if !defined IP_FRAG || defined __DOXYGEN__
#define IP_FRAG                         1
#endif

#if !LWIP_IPV4
/* disable IPv4 extensions when IPv4 is disabled */
#undef IP_FORWARD
#define IP_FORWARD                      0
#undef IP_REASSEMBLY
#define IP_REASSEMBLY                   0
#undef IP_FRAG
#define IP_FRAG                         0
#endif /* !LWIP_IPV4 */

/**
 * IP_OPTIONS_ALLOWED: Defines the behavior for IP options.
 *      IP_OPTIONS_ALLOWED==0: All packets with IP options are dropped.
 *      IP_OPTIONS_ALLOWED==1: IP options are allowed (but not parsed).
 */
#if !defined IP_OPTIONS_ALLOWED || defined __DOXYGEN__
#define IP_OPTIONS_ALLOWED              1
#endif

/**
 * IP_REASS_MAXAGE: Maximum time (in multiples of IP_TMR_INTERVAL - so seconds, normally)
 * a fragmented IP packet waits for all fragments to arrive. If not all fragments arrived
 * in this time, the whole packet is discarded.
 */
#if !defined IP_REASS_MAXAGE || defined __DOXYGEN__
#define IP_REASS_MAXAGE                 15
#endif

/**
 * IP_REASS_MAX_PBUFS: Total maximum amount of pbufs waiting to be reassembled.
 * Since the received pbufs are enqueued, be sure to configure
 * PBUF_POOL_SIZE > IP_REASS_MAX_PBUFS so that the stack is still able to receive
 * packets even if the maximum amount of fragments is enqueued for reassembly!
 */
#if !defined IP_REASS_MAX_PBUFS || defined __DOXYGEN__
#define IP_REASS_MAX_PBUFS              10
#endif

/**
 * IP_DEFAULT_TTL: Default value for Time-To-Live used by transport layers.
 */
#if !defined IP_DEFAULT_TTL || defined __DOXYGEN__
#define IP_DEFAULT_TTL                  255
#endif

/**
 * IP_SOF_BROADCAST=1: Use the SOF_BROADCAST field to enable broadcast
 * filter per pcb on udp and raw send operations. To enable broadcast filter
 * on recv operations, you also have to set IP_SOF_BROADCAST_RECV=1.
 */
#if !defined IP_SOF_BROADCAST || defined __DOXYGEN__
#define IP_SOF_BROADCAST                0
#endif

/**
 * IP_SOF_BROADCAST_RECV (requires IP_SOF_BROADCAST=1) enable the broadcast
 * filter on recv operations.
 */
#if !defined IP_SOF_BROADCAST_RECV || defined __DOXYGEN__
#define IP_SOF_BROADCAST_RECV           0
#endif

/**
 * IP_FORWARD_ALLOW_TX_ON_RX_NETIF==1: allow ip_forward() to send packets back
 * out on the netif where it was received. This should only be used for
 * wireless networks.
 * ATTENTION: When this is 1, make sure your netif driver correctly marks incoming
 * link-layer-broadcast/multicast packets as such using the corresponding pbuf flags!
 */
#if !defined IP_FORWARD_ALLOW_TX_ON_RX_NETIF || defined __DOXYGEN__
#define IP_FORWARD_ALLOW_TX_ON_RX_NETIF 0
#endif
/**
 * @}
 */

/*
   ----------------------------------
   ---------- ICMP options ----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_icmp ICMP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_ICMP==1: Enable ICMP module inside the IP stack.
 * Be careful, disable that make your product non-compliant to RFC1122
 */
#if !defined LWIP_ICMP || defined __DOXYGEN__
#define LWIP_ICMP                       1
#endif

/**
 * ICMP_TTL: Default value for Time-To-Live used by ICMP packets.
 */
#if !defined ICMP_TTL || defined __DOXYGEN__
#define ICMP_TTL                       (IP_DEFAULT_TTL)
#endif

/**
 * LWIP_BROADCAST_PING==1: respond to broadcast pings (default is unicast only)
 */
#if !defined LWIP_BROADCAST_PING || defined __DOXYGEN__
#define LWIP_BROADCAST_PING             0
#endif

/**
 * LWIP_MULTICAST_PING==1: respond to multicast pings (default is unicast only)
 */
#if !defined LWIP_MULTICAST_PING || defined __DOXYGEN__
#define LWIP_MULTICAST_PING             0
#endif
/**
 * @}
 */

/*
   ---------------------------------
   ---------- RAW options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_raw RAW
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_RAW==1: Enable application layer to hook into the IP layer itself.
 */
#if !defined LWIP_RAW || defined __DOXYGEN__
#define LWIP_RAW                        0
#endif

/**
 * LWIP_RAW==1: Enable application layer to hook into the IP layer itself.
 */
#if !defined RAW_TTL || defined __DOXYGEN__
#define RAW_TTL                        (IP_DEFAULT_TTL)
#endif
/**
 * @}
 */

/*
   ----------------------------------
   ---------- DHCP options ----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_dhcp DHCP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_DHCP==1: Enable DHCP module.
 */
#if !defined LWIP_DHCP || defined __DOXYGEN__
#define LWIP_DHCP                       0
#endif
#if !LWIP_IPV4
/* disable DHCP when IPv4 is disabled */
#undef LWIP_DHCP
#define LWIP_DHCP                       0
#endif /* !LWIP_IPV4 */

/**
 * DHCP_DOES_ARP_CHECK==1: Do an ARP check on the offered address.
 */
#if !defined DHCP_DOES_ARP_CHECK || defined __DOXYGEN__
#define DHCP_DOES_ARP_CHECK             ((LWIP_DHCP) && (LWIP_ARP))
#endif

/**
 * LWIP_DHCP_CHECK_LINK_UP==1: dhcp_start() only really starts if the netif has
 * NETIF_FLAG_LINK_UP set in its flags. As this is only an optimization and
 * netif drivers might not set this flag, the default is off. If enabled,
 * netif_set_link_up() must be called to continue dhcp starting.
 */
#if !defined LWIP_DHCP_CHECK_LINK_UP
#define LWIP_DHCP_CHECK_LINK_UP         0
#endif

/**
 * LWIP_DHCP_BOOTP_FILE==1: Store offered_si_addr and boot_file_name.
 */
#if !defined LWIP_DHCP_BOOTP_FILE || defined __DOXYGEN__
#define LWIP_DHCP_BOOTP_FILE            0
#endif

/**
 * LWIP_DHCP_GETS_NTP==1: Request NTP servers with discover/select. For each
 * response packet, an callback is called, which has to be provided by the port:
 * void dhcp_set_ntp_servers(u8_t num_ntp_servers, ip_addr_t* ntp_server_addrs);
*/
#if !defined LWIP_DHCP_GET_NTP_SRV || defined __DOXYGEN__
#define LWIP_DHCP_GET_NTP_SRV           0
#endif

/**
 * The maximum of NTP servers requested
 */
#if !defined LWIP_DHCP_MAX_NTP_SERVERS || defined __DOXYGEN__
#define LWIP_DHCP_MAX_NTP_SERVERS       1
#endif

/**
 * LWIP_DHCP_MAX_DNS_SERVERS > 0: Request DNS servers with discover/select.
 * DHCP servers received in the response are passed to DNS via @ref dns_setserver()
 * (up to the maximum limit defined here).
 */
#if !defined LWIP_DHCP_MAX_DNS_SERVERS || defined __DOXYGEN__
#define LWIP_DHCP_MAX_DNS_SERVERS       DNS_MAX_SERVERS
#endif
/**
 * @}
 */

/*
   ------------------------------------
   ---------- AUTOIP options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_autoip AUTOIP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_AUTOIP==1: Enable AUTOIP module.
 */
#if !defined LWIP_AUTOIP || defined __DOXYGEN__
#define LWIP_AUTOIP                     0
#endif
#if !LWIP_IPV4
/* disable AUTOIP when IPv4 is disabled */
#undef LWIP_AUTOIP
#define LWIP_AUTOIP                     0
#endif /* !LWIP_IPV4 */

/**
 * LWIP_DHCP_AUTOIP_COOP==1: Allow DHCP and AUTOIP to be both enabled on
 * the same interface at the same time.
 */
#if !defined LWIP_DHCP_AUTOIP_COOP || defined __DOXYGEN__
#define LWIP_DHCP_AUTOIP_COOP           0
#endif

/**
 * LWIP_DHCP_AUTOIP_COOP_TRIES: Set to the number of DHCP DISCOVER probes
 * that should be sent before falling back on AUTOIP (the DHCP client keeps
 * running in this case). This can be set as low as 1 to get an AutoIP address
 * very  quickly, but you should be prepared to handle a changing IP address
 * when DHCP overrides AutoIP.
 */
#if !defined LWIP_DHCP_AUTOIP_COOP_TRIES || defined __DOXYGEN__
#define LWIP_DHCP_AUTOIP_COOP_TRIES     9
#endif
/**
 * @}
 */

/*
   ----------------------------------
   ----- SNMP MIB2 support      -----
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_mib2 SNMP MIB2 callbacks
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * LWIP_MIB2_CALLBACKS==1: Turn on SNMP MIB2 callbacks.
 * Turn this on to get callbacks needed to implement MIB2.
 * Usually MIB2_STATS should be enabled, too.
 */
#if !defined LWIP_MIB2_CALLBACKS || defined __DOXYGEN__
#define LWIP_MIB2_CALLBACKS             0
#endif
/**
 * @}
 */

/*
   ----------------------------------
   -------- Multicast options -------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_multicast Multicast
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * LWIP_MULTICAST_TX_OPTIONS==1: Enable multicast TX support like the socket options
 * IP_MULTICAST_TTL/IP_MULTICAST_IF/IP_MULTICAST_LOOP, as well as (currently only)
 * core support for the corresponding IPv6 options.
 */
#if !defined LWIP_MULTICAST_TX_OPTIONS || defined __DOXYGEN__
#define LWIP_MULTICAST_TX_OPTIONS       ((LWIP_IGMP || LWIP_IPV6_MLD) && (LWIP_UDP || LWIP_RAW))
#endif
/**
 * @}
 */

/*
   ----------------------------------
   ---------- IGMP options ----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_igmp IGMP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_IGMP==1: Turn on IGMP module.
 */
#if !defined LWIP_IGMP || defined __DOXYGEN__
#define LWIP_IGMP                       0
#endif
#if !LWIP_IPV4
#undef LWIP_IGMP
#define LWIP_IGMP                       0
#endif
/**
 * @}
 */

/*
   ----------------------------------
   ---------- DNS options -----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_dns DNS
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_DNS==1: Turn on DNS module. UDP must be available for DNS
 * transport.
 */
#if !defined LWIP_DNS || defined __DOXYGEN__
#define LWIP_DNS                        0
#endif

/** DNS maximum number of entries to maintain locally. */
#if !defined DNS_TABLE_SIZE || defined __DOXYGEN__
#define DNS_TABLE_SIZE                  4
#endif

/** DNS maximum host name length supported in the name table. */
#if !defined DNS_MAX_NAME_LENGTH || defined __DOXYGEN__
#define DNS_MAX_NAME_LENGTH             256
#endif

/** The maximum of DNS servers
 * The first server can be initialized automatically by defining
 * DNS_SERVER_ADDRESS(ipaddr), where 'ipaddr' is an 'ip_addr_t*'
 */
#if !defined DNS_MAX_SERVERS || defined __DOXYGEN__
#define DNS_MAX_SERVERS                 2
#endif

/** DNS do a name checking between the query and the response. */
#if !defined DNS_DOES_NAME_CHECK || defined __DOXYGEN__
#define DNS_DOES_NAME_CHECK             1
#endif

/** LWIP_DNS_SECURE: controls the security level of the DNS implementation
 * Use all DNS security features by default.
 * This is overridable but should only be needed by very small targets
 * or when using against non standard DNS servers. */
#if !defined LWIP_DNS_SECURE || defined __DOXYGEN__
#define LWIP_DNS_SECURE (LWIP_DNS_SECURE_RAND_XID | LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING | LWIP_DNS_SECURE_RAND_SRC_PORT)
#endif

/* A list of DNS security features follows */
#define LWIP_DNS_SECURE_RAND_XID                1
#define LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING 2
#define LWIP_DNS_SECURE_RAND_SRC_PORT           4

/** DNS_LOCAL_HOSTLIST: Implements a local host-to-address list. If enabled, you have to define an initializer:
 *  \#define DNS_LOCAL_HOSTLIST_INIT {DNS_LOCAL_HOSTLIST_ELEM("host_ip4", IPADDR4_INIT_BYTES(1,2,3,4)), \
 *                                    DNS_LOCAL_HOSTLIST_ELEM("host_ip6", IPADDR6_INIT_HOST(123, 234, 345, 456)}
 *
 *  Instead, you can also use an external function:
 *  \#define DNS_LOOKUP_LOCAL_EXTERN(x) extern err_t my_lookup_function(const char *name, ip_addr_t *addr, u8_t dns_addrtype)
 *  that looks up the IP address and returns ERR_OK if found (LWIP_DNS_ADDRTYPE_xxx is passed in dns_addrtype).
 */
#if !defined DNS_LOCAL_HOSTLIST || defined __DOXYGEN__
#define DNS_LOCAL_HOSTLIST              0
#endif /* DNS_LOCAL_HOSTLIST */

/** If this is turned on, the local host-list can be dynamically changed
 *  at runtime. */
#if !defined DNS_LOCAL_HOSTLIST_IS_DYNAMIC || defined __DOXYGEN__
#define DNS_LOCAL_HOSTLIST_IS_DYNAMIC   0
#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */

/** Set this to 1 to enable querying ".local" names via mDNS
 *  using a One-Shot Multicast DNS Query */
#if !defined LWIP_DNS_SUPPORT_MDNS_QUERIES || defined __DOXYGEN__
#define LWIP_DNS_SUPPORT_MDNS_QUERIES  0
#endif
/**
 * @}
 */

/*
   ---------------------------------
   ---------- UDP options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_udp UDP
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_UDP==1: Turn on UDP.
 */
#if !defined LWIP_UDP || defined __DOXYGEN__
#define LWIP_UDP                        1
#endif

/**
 * LWIP_UDPLITE==1: Turn on UDP-Lite. (Requires LWIP_UDP)
 */
#if !defined LWIP_UDPLITE || defined __DOXYGEN__
#define LWIP_UDPLITE                    0
#endif

/**
 * UDP_TTL: Default Time-To-Live value.
 */
#if !defined UDP_TTL || defined __DOXYGEN__
#define UDP_TTL                         (IP_DEFAULT_TTL)
#endif

/**
 * LWIP_NETBUF_RECVINFO==1: append destination addr and port to every netbuf.
 */
#if !defined LWIP_NETBUF_RECVINFO || defined __DOXYGEN__
#define LWIP_NETBUF_RECVINFO            0
#endif
/**
 * @}
 */

/*
   ---------------------------------
   ---------- TCP options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_tcp TCP
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_TCP==1: Turn on TCP.
 */
#if !defined LWIP_TCP || defined __DOXYGEN__
#define LWIP_TCP                        1
#endif

/**
 * TCP_TTL: Default Time-To-Live value.
 */
#if !defined TCP_TTL || defined __DOXYGEN__
#define TCP_TTL                         (IP_DEFAULT_TTL)
#endif

/**
 * TCP_WND: The size of a TCP window.  This must be at least
 * (2 * TCP_MSS) for things to work well.
 * ATTENTION: when using TCP_RCV_SCALE, TCP_WND is the total size
 * with scaling applied. Maximum window value in the TCP header
 * will be TCP_WND >> TCP_RCV_SCALE
 */
#if !defined TCP_WND || defined __DOXYGEN__
#define TCP_WND                         (4 * TCP_MSS)
#endif

/**
 * TCP_MAXRTX: Maximum number of retransmissions of data segments.
 */
#if !defined TCP_MAXRTX || defined __DOXYGEN__
#define TCP_MAXRTX                      12
#endif

/**
 * TCP_SYNMAXRTX: Maximum number of retransmissions of SYN segments.
 */
#if !defined TCP_SYNMAXRTX || defined __DOXYGEN__
#define TCP_SYNMAXRTX                   6
#endif

/**
 * TCP_QUEUE_OOSEQ==1: TCP will queue segments that arrive out of order.
 * Define to 0 if your device is low on memory.
 */
#if !defined TCP_QUEUE_OOSEQ || defined __DOXYGEN__
#define TCP_QUEUE_OOSEQ                 (LWIP_TCP)
#endif

/**
 * LWIP_TCP_SACK_OUT==1: TCP will support sending selective acknowledgements (SACKs).
 */
#if !defined LWIP_TCP_SACK_OUT || defined __DOXYGEN__
#define LWIP_TCP_SACK_OUT               0
#endif

/**
 * LWIP_TCP_MAX_SACK_NUM: The maximum number of SACK values to include in TCP segments.
 * Must be at least 1, but is only used if LWIP_TCP_SACK_OUT is enabled.
 * NOTE: Even though we never send more than 3 or 4 SACK ranges in a single segment
 * (depending on other options), setting this option to values greater than 4 is not pointless.
 * This is basically the max number of SACK ranges we want to keep track of.
 * As new data is delivered, some of the SACK ranges may be removed or merged.
 * In that case some of those older SACK ranges may be used again.
 * The amount of memory used to store SACK ranges is LWIP_TCP_MAX_SACK_NUM * 8 bytes for each TCP PCB.
 */
#if !defined LWIP_TCP_MAX_SACK_NUM || defined __DOXYGEN__
#define LWIP_TCP_MAX_SACK_NUM           4
#endif

/**
 * TCP_MSS: TCP Maximum segment size. (default is 536, a conservative default,
 * you might want to increase this.)
 * For the receive side, this MSS is advertised to the remote side
 * when opening a connection. For the transmit size, this MSS sets
 * an upper limit on the MSS advertised by the remote host.
 */
#if !defined TCP_MSS || defined __DOXYGEN__
#define TCP_MSS                         536
#endif

/**
 * TCP_CALCULATE_EFF_SEND_MSS: "The maximum size of a segment that TCP really
 * sends, the 'effective send MSS,' MUST be the smaller of the send MSS (which
 * reflects the available reassembly buffer size at the remote host) and the
 * largest size permitted by the IP layer" (RFC 1122)
 * Setting this to 1 enables code that checks TCP_MSS against the MTU of the
 * netif used for a connection and limits the MSS if it would be too big otherwise.
 */
#if !defined TCP_CALCULATE_EFF_SEND_MSS || defined __DOXYGEN__
#define TCP_CALCULATE_EFF_SEND_MSS      1
#endif


/**
 * TCP_SND_BUF: TCP sender buffer space (bytes).
 * To achieve good performance, this should be at least 2 * TCP_MSS.
 */
#if !defined TCP_SND_BUF || defined __DOXYGEN__
#define TCP_SND_BUF                     (2 * TCP_MSS)
#endif

/**
 * TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
 * as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work.
 */
#if !defined TCP_SND_QUEUELEN || defined __DOXYGEN__
#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
#endif

/**
 * TCP_SNDLOWAT: TCP writable space (bytes). This must be less than
 * TCP_SND_BUF. It is the amount of space which must be available in the
 * TCP snd_buf for select to return writable (combined with TCP_SNDQUEUELOWAT).
 */
#if !defined TCP_SNDLOWAT || defined __DOXYGEN__
#define TCP_SNDLOWAT                    LWIP_MIN(LWIP_MAX(((TCP_SND_BUF)/2), (2 * TCP_MSS) + 1), (TCP_SND_BUF) - 1)
#endif

/**
 * TCP_SNDQUEUELOWAT: TCP writable bufs (pbuf count). This must be less
 * than TCP_SND_QUEUELEN. If the number of pbufs queued on a pcb drops below
 * this number, select returns writable (combined with TCP_SNDLOWAT).
 */
#if !defined TCP_SNDQUEUELOWAT || defined __DOXYGEN__
#define TCP_SNDQUEUELOWAT               LWIP_MAX(((TCP_SND_QUEUELEN)/2), 5)
#endif

/**
 * TCP_OOSEQ_MAX_BYTES: The default maximum number of bytes queued on ooseq per
 * pcb if TCP_OOSEQ_BYTES_LIMIT is not defined. Default is 0 (no limit).
 * Only valid for TCP_QUEUE_OOSEQ==1.
 */
#if !defined TCP_OOSEQ_MAX_BYTES || defined __DOXYGEN__
#define TCP_OOSEQ_MAX_BYTES             0
#endif

/**
 * TCP_OOSEQ_BYTES_LIMIT(pcb): Return the maximum number of bytes to be queued
 * on ooseq per pcb, given the pcb. Only valid for TCP_QUEUE_OOSEQ==1 &&
 * TCP_OOSEQ_MAX_BYTES==1.
 * Use this to override TCP_OOSEQ_MAX_BYTES to a dynamic value per pcb.
 */
#if !defined TCP_OOSEQ_BYTES_LIMIT
#if TCP_OOSEQ_MAX_BYTES
#define TCP_OOSEQ_BYTES_LIMIT(pcb) TCP_OOSEQ_MAX_BYTES
#elif defined __DOXYGEN__
#define TCP_OOSEQ_BYTES_LIMIT(pcb)
#endif
#endif

/**
 * TCP_OOSEQ_MAX_PBUFS: The default maximum number of pbufs queued on ooseq per
 * pcb if TCP_OOSEQ_BYTES_LIMIT is not defined. Default is 0 (no limit).
 * Only valid for TCP_QUEUE_OOSEQ==1.
 */
#if !defined TCP_OOSEQ_MAX_PBUFS || defined __DOXYGEN__
#define TCP_OOSEQ_MAX_PBUFS             0
#endif

/**
 * TCP_OOSEQ_PBUFS_LIMIT(pcb): Return the maximum number of pbufs to be queued
 * on ooseq per pcb, given the pcb.  Only valid for TCP_QUEUE_OOSEQ==1 &&
 * TCP_OOSEQ_MAX_PBUFS==1.
 * Use this to override TCP_OOSEQ_MAX_PBUFS to a dynamic value per pcb.
 */
#if !defined TCP_OOSEQ_PBUFS_LIMIT
#if TCP_OOSEQ_MAX_PBUFS
#define TCP_OOSEQ_PBUFS_LIMIT(pcb) TCP_OOSEQ_MAX_PBUFS
#elif defined __DOXYGEN__
#define TCP_OOSEQ_PBUFS_LIMIT(pcb)
#endif
#endif

/**
 * TCP_LISTEN_BACKLOG: Enable the backlog option for tcp listen pcb.
 */
#if !defined TCP_LISTEN_BACKLOG || defined __DOXYGEN__
#define TCP_LISTEN_BACKLOG              0
#endif

/**
 * The maximum allowed backlog for TCP listen netconns.
 * This backlog is used unless another is explicitly specified.
 * 0xff is the maximum (u8_t).
 */
#if !defined TCP_DEFAULT_LISTEN_BACKLOG || defined __DOXYGEN__
#define TCP_DEFAULT_LISTEN_BACKLOG      0xff
#endif

/**
 * TCP_OVERSIZE: The maximum number of bytes that tcp_write may
 * allocate ahead of time in an attempt to create shorter pbuf chains
 * for transmission. The meaningful range is 0 to TCP_MSS. Some
 * suggested values are:
 *
 * 0:         Disable oversized allocation. Each tcp_write() allocates a new
              pbuf (old behaviour).
 * 1:         Allocate size-aligned pbufs with minimal excess. Use this if your
 *            scatter-gather DMA requires aligned fragments.
 * 128:       Limit the pbuf/memory overhead to 20%.
 * TCP_MSS:   Try to create unfragmented TCP packets.
 * TCP_MSS/4: Try to create 4 fragments or less per TCP packet.
 */
#if !defined TCP_OVERSIZE || defined __DOXYGEN__
#define TCP_OVERSIZE                    TCP_MSS
#endif

/**
 * LWIP_TCP_TIMESTAMPS==1: support the TCP timestamp option.
 * The timestamp option is currently only used to help remote hosts, it is not
 * really used locally. Therefore, it is only enabled when a TS option is
 * received in the initial SYN packet from a remote host.
 */
#if !defined LWIP_TCP_TIMESTAMPS || defined __DOXYGEN__
#define LWIP_TCP_TIMESTAMPS             0
#endif

/**
 * TCP_WND_UPDATE_THRESHOLD: difference in window to trigger an
 * explicit window update
 */
#if !defined TCP_WND_UPDATE_THRESHOLD || defined __DOXYGEN__
#define TCP_WND_UPDATE_THRESHOLD   LWIP_MIN((TCP_WND / 4), (TCP_MSS * 4))
#endif

/**
 * LWIP_EVENT_API and LWIP_CALLBACK_API: Only one of these should be set to 1.
 *     LWIP_EVENT_API==1: The user defines lwip_tcp_event() to receive all
 *         events (accept, sent, etc) that happen in the system.
 *     LWIP_CALLBACK_API==1: The PCB callback function is called directly
 *         for the event. This is the default.
 */
#if !defined(LWIP_EVENT_API) && !defined(LWIP_CALLBACK_API) || defined __DOXYGEN__
#define LWIP_EVENT_API                  0
#define LWIP_CALLBACK_API               1
#else
#ifndef LWIP_EVENT_API
#define LWIP_EVENT_API                  0
#endif
#ifndef LWIP_CALLBACK_API
#define LWIP_CALLBACK_API               0
#endif
#endif

/**
 * LWIP_WND_SCALE and TCP_RCV_SCALE:
 * Set LWIP_WND_SCALE to 1 to enable window scaling.
 * Set TCP_RCV_SCALE to the desired scaling factor (shift count in the
 * range of [0..14]).
 * When LWIP_WND_SCALE is enabled but TCP_RCV_SCALE is 0, we can use a large
 * send window while having a small receive window only.
 */
#if !defined LWIP_WND_SCALE || defined __DOXYGEN__
#define LWIP_WND_SCALE                  0
#define TCP_RCV_SCALE                   0
#endif

/** LWIP_ALTCP==1: enable the altcp API
 * altcp is an abstraction layer that prevents applications linking against the
 * tcp.h functions but provides the same functionality. It is used to e.g. add
 * SSL/TLS or proxy-connect support to an application written for the tcp callback
 * API without that application knowing the protocol details.
 * Applications written against the altcp API are directly linked against the
 * tcp callback API for LWIP_ALTCP==0, but then cannot use layered protocols.
 */
#ifndef LWIP_ALTCP
#define LWIP_ALTCP                      0
#endif

/** LWIP_ALTCP_TLS==1: enable TLS support for altcp API.
 * This needs a port of the functions in altcp_tls.h to a TLS library.
 * A port to ARM mbedtls is provided with lwIP, see apps/altcp_tls/ directory
 * and LWIP_ALTCP_TLS_MBEDTLS option.
 */
#ifndef LWIP_ALTCP_TLS
#define LWIP_ALTCP_TLS                  0
#endif

/**
 * @}
 */

/*
   ----------------------------------
   ---------- Pbuf options ----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_pbuf PBUF
 * @ingroup lwip_opts
 * @{
 */
/**
 * PBUF_LINK_HLEN: the number of bytes that should be allocated for a
 * link level header. The default is 14, the standard value for
 * Ethernet.
 */
#if !defined PBUF_LINK_HLEN || defined __DOXYGEN__
#if defined LWIP_HOOK_VLAN_SET && !defined __DOXYGEN__
#define PBUF_LINK_HLEN                  (18 + ETH_PAD_SIZE)
#else /* LWIP_HOOK_VLAN_SET */
#define PBUF_LINK_HLEN                  (14 + ETH_PAD_SIZE)
#endif /* LWIP_HOOK_VLAN_SET */
#endif

/**
 * PBUF_LINK_ENCAPSULATION_HLEN: the number of bytes that should be allocated
 * for an additional encapsulation header before ethernet headers (e.g. 802.11)
 */
#if !defined PBUF_LINK_ENCAPSULATION_HLEN || defined __DOXYGEN__
#define PBUF_LINK_ENCAPSULATION_HLEN    0
#endif

/**
 * PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. The default is
 * designed to accommodate single full size TCP frame in one pbuf, including
 * TCP_MSS, IP header, and link header.
 */
#if !defined PBUF_POOL_BUFSIZE || defined __DOXYGEN__
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)
#endif

/**
 * LWIP_PBUF_REF_T: Refcount type in pbuf.
 * Default width of u8_t can be increased if 255 refs are not enough for you.
 */
#ifndef LWIP_PBUF_REF_T
#define LWIP_PBUF_REF_T u8_t
#endif
/**
 * @}
 */

/*
   ------------------------------------------------
   ---------- Network Interfaces options ----------
   ------------------------------------------------
*/
/**
 * @defgroup lwip_opts_netif NETIF
 * @ingroup lwip_opts
 * @{
 */
/**
 * LWIP_SINGLE_NETIF==1: use a single netif only. This is the common case for
 * small real-life targets. Some code like routing etc. can be left out.
 */
#if !defined LWIP_SINGLE_NETIF || defined __DOXYGEN__
#define LWIP_SINGLE_NETIF               0
#endif

/**
 * LWIP_NETIF_HOSTNAME==1: use DHCP_OPTION_HOSTNAME with netif's hostname
 * field.
 */
#if !defined LWIP_NETIF_HOSTNAME || defined __DOXYGEN__
#define LWIP_NETIF_HOSTNAME             0
#endif

/**
 * LWIP_NETIF_API==1: Support netif api (in netifapi.c)
 */
#if !defined LWIP_NETIF_API || defined __DOXYGEN__
#define LWIP_NETIF_API                  0
#endif

/**
 * LWIP_NETIF_STATUS_CALLBACK==1: Support a callback function whenever an interface
 * changes its up/down status (i.e., due to DHCP IP acquisition)
 */
#if !defined LWIP_NETIF_STATUS_CALLBACK || defined __DOXYGEN__
#define LWIP_NETIF_STATUS_CALLBACK      0
#endif

/**
 * LWIP_NETIF_EXT_STATUS_CALLBACK==1: Support an extended callback function 
 * for several netif related event that supports multiple subscribers.
 * @see netif_ext_status_callback
 */
#if !defined LWIP_NETIF_EXT_STATUS_CALLBACK || defined __DOXYGEN__
#define LWIP_NETIF_EXT_STATUS_CALLBACK  0
#endif

/**
 * LWIP_NETIF_LINK_CALLBACK==1: Support a callback function from an interface
 * whenever the link changes (i.e., link down)
 */
#if !defined LWIP_NETIF_LINK_CALLBACK || defined __DOXYGEN__
#define LWIP_NETIF_LINK_CALLBACK        0
#endif

/**
 * LWIP_NETIF_REMOVE_CALLBACK==1: Support a callback function that is called
 * when a netif has been removed
 */
#if !defined LWIP_NETIF_REMOVE_CALLBACK || defined __DOXYGEN__
#define LWIP_NETIF_REMOVE_CALLBACK      0
#endif

/**
 * LWIP_NETIF_HWADDRHINT==1: Cache link-layer-address hints (e.g. table
 * indices) in struct netif. TCP and UDP can make use of this to prevent
 * scanning the ARP table for every sent packet. While this is faster for big
 * ARP tables or many concurrent connections, it might be counterproductive
 * if you have a tiny ARP table or if there never are concurrent connections.
 */
#if !defined LWIP_NETIF_HWADDRHINT || defined __DOXYGEN__
#define LWIP_NETIF_HWADDRHINT           0
#endif

/**
 * LWIP_NETIF_TX_SINGLE_PBUF: if this is set to 1, lwIP *tries* to put all data
 * to be sent into one single pbuf. This is for compatibility with DMA-enabled
 * MACs that do not support scatter-gather.
 * Beware that this might involve CPU-memcpy before transmitting that would not
 * be needed without this flag! Use this only if you need to!
 *
 * ATTENTION: a driver should *NOT* rely on getting single pbufs but check TX
 * pbufs for being in one piece. If not, @ref pbuf_clone can be used to get
 * a single pbuf:
 *   if (p->next != NULL) {
 *     struct pbuf *q = pbuf_clone(PBUF_RAW, PBUF_RAM, p);
 *     if (q == NULL) {
 *       return ERR_MEM;
 *     }
 *     p = q; ATTENTION: do NOT free the old 'p' as the ref belongs to the caller!
 *   }
 */
#if !defined LWIP_NETIF_TX_SINGLE_PBUF || defined __DOXYGEN__
#define LWIP_NETIF_TX_SINGLE_PBUF             0
#endif /* LWIP_NETIF_TX_SINGLE_PBUF */

/**
 * LWIP_NUM_NETIF_CLIENT_DATA: Number of clients that may store
 * data in client_data member array of struct netif (max. 256).
 */
#if !defined LWIP_NUM_NETIF_CLIENT_DATA || defined __DOXYGEN__
#define LWIP_NUM_NETIF_CLIENT_DATA            0
#endif
/**
 * @}
 */

/*
   ------------------------------------
   ---------- LOOPIF options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_loop Loopback interface
 * @ingroup lwip_opts_netif
 * @{
 */
/**
 * LWIP_HAVE_LOOPIF==1: Support loop interface (127.0.0.1).
 * This is only needed when no real netifs are available. If at least one other
 * netif is available, loopback traffic uses this netif.
 */
#if !defined LWIP_HAVE_LOOPIF || defined __DOXYGEN__
#define LWIP_HAVE_LOOPIF                (LWIP_NETIF_LOOPBACK && !LWIP_SINGLE_NETIF)
#endif

/**
 * LWIP_LOOPIF_MULTICAST==1: Support multicast/IGMP on loop interface (127.0.0.1).
 */
#if !defined LWIP_LOOPIF_MULTICAST || defined __DOXYGEN__
#define LWIP_LOOPIF_MULTICAST               0
#endif

/**
 * LWIP_NETIF_LOOPBACK==1: Support sending packets with a destination IP
 * address equal to the netif IP address, looping them back up the stack.
 */
#if !defined LWIP_NETIF_LOOPBACK || defined __DOXYGEN__
#define LWIP_NETIF_LOOPBACK             0
#endif

/**
 * LWIP_LOOPBACK_MAX_PBUFS: Maximum number of pbufs on queue for loopback
 * sending for each netif (0 = disabled)
 */
#if !defined LWIP_LOOPBACK_MAX_PBUFS || defined __DOXYGEN__
#define LWIP_LOOPBACK_MAX_PBUFS         0
#endif

/**
 * LWIP_NETIF_LOOPBACK_MULTITHREADING: Indicates whether threading is enabled in
 * the system, as netifs must change how they behave depending on this setting
 * for the LWIP_NETIF_LOOPBACK option to work.
 * Setting this is needed to avoid reentering non-reentrant functions like
 * tcp_input().
 *    LWIP_NETIF_LOOPBACK_MULTITHREADING==1: Indicates that the user is using a
 *       multithreaded environment like tcpip.c. In this case, netif->input()
 *       is called directly.
 *    LWIP_NETIF_LOOPBACK_MULTITHREADING==0: Indicates a polling (or NO_SYS) setup.
 *       The packets are put on a list and netif_poll() must be called in
 *       the main application loop.
 */
#if !defined LWIP_NETIF_LOOPBACK_MULTITHREADING || defined __DOXYGEN__
#define LWIP_NETIF_LOOPBACK_MULTITHREADING    (!NO_SYS)
#endif
/**
 * @}
 */

/*
   ------------------------------------
   ---------- Thread options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_thread Threading
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * TCPIP_THREAD_NAME: The name assigned to the main tcpip thread.
 */
#if !defined TCPIP_THREAD_NAME || defined __DOXYGEN__
#define TCPIP_THREAD_NAME              "tcpip_thread"
#endif

/**
 * TCPIP_THREAD_STACKSIZE: The stack size used by the main tcpip thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#if !defined TCPIP_THREAD_STACKSIZE || defined __DOXYGEN__
#define TCPIP_THREAD_STACKSIZE          0
#endif

/**
 * TCPIP_THREAD_PRIO: The priority assigned to the main tcpip thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#if !defined TCPIP_THREAD_PRIO || defined __DOXYGEN__
#define TCPIP_THREAD_PRIO               1
#endif

/**
 * TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called.
 */
#if !defined TCPIP_MBOX_SIZE || defined __DOXYGEN__
#define TCPIP_MBOX_SIZE                 0
#endif

/**
 * Define this to something that triggers a watchdog. This is called from
 * tcpip_thread after processing a message.
 */
#if !defined LWIP_TCPIP_THREAD_ALIVE || defined __DOXYGEN__
#define LWIP_TCPIP_THREAD_ALIVE()
#endif

/**
 * SLIPIF_THREAD_NAME: The name assigned to the slipif_loop thread.
 */
#if !defined SLIPIF_THREAD_NAME || defined __DOXYGEN__
#define SLIPIF_THREAD_NAME             "slipif_loop"
#endif

/**
 * SLIP_THREAD_STACKSIZE: The stack size used by the slipif_loop thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#if !defined SLIPIF_THREAD_STACKSIZE || defined __DOXYGEN__
#define SLIPIF_THREAD_STACKSIZE         0
#endif

/**
 * SLIPIF_THREAD_PRIO: The priority assigned to the slipif_loop thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#if !defined SLIPIF_THREAD_PRIO || defined __DOXYGEN__
#define SLIPIF_THREAD_PRIO              1
#endif

/**
 * DEFAULT_THREAD_NAME: The name assigned to any other lwIP thread.
 */
#if !defined DEFAULT_THREAD_NAME || defined __DOXYGEN__
#define DEFAULT_THREAD_NAME            "lwIP"
#endif

/**
 * DEFAULT_THREAD_STACKSIZE: The stack size used by any other lwIP thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#if !defined DEFAULT_THREAD_STACKSIZE || defined __DOXYGEN__
#define DEFAULT_THREAD_STACKSIZE        0
#endif

/**
 * DEFAULT_THREAD_PRIO: The priority assigned to any other lwIP thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#if !defined DEFAULT_THREAD_PRIO || defined __DOXYGEN__
#define DEFAULT_THREAD_PRIO             1
#endif

/**
 * DEFAULT_RAW_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_RAW. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#if !defined DEFAULT_RAW_RECVMBOX_SIZE || defined __DOXYGEN__
#define DEFAULT_RAW_RECVMBOX_SIZE       0
#endif

/**
 * DEFAULT_UDP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_UDP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#if !defined DEFAULT_UDP_RECVMBOX_SIZE || defined __DOXYGEN__
#define DEFAULT_UDP_RECVMBOX_SIZE       0
#endif

/**
 * DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#if !defined DEFAULT_TCP_RECVMBOX_SIZE || defined __DOXYGEN__
#define DEFAULT_TCP_RECVMBOX_SIZE       0
#endif

/**
 * DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created.
 */
#if !defined DEFAULT_ACCEPTMBOX_SIZE || defined __DOXYGEN__
#define DEFAULT_ACCEPTMBOX_SIZE         0
#endif
/**
 * @}
 */

/*
   ----------------------------------------------
   ---------- Sequential layer options ----------
   ----------------------------------------------
*/
/**
 * @defgroup lwip_opts_netconn Netconn
 * @ingroup lwip_opts_threadsafe_apis
 * @{
 */
/**
 * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
 */
#if !defined LWIP_NETCONN || defined __DOXYGEN__
#define LWIP_NETCONN                    1
#endif

/** LWIP_TCPIP_TIMEOUT==1: Enable tcpip_timeout/tcpip_untimeout to create
 * timers running in tcpip_thread from another thread.
 */
#if !defined LWIP_TCPIP_TIMEOUT || defined __DOXYGEN__
#define LWIP_TCPIP_TIMEOUT              0
#endif

/** LWIP_NETCONN_SEM_PER_THREAD==1: Use one (thread-local) semaphore per
 * thread calling socket/netconn functions instead of allocating one
 * semaphore per netconn (and per select etc.)
 * ATTENTION: a thread-local semaphore for API calls is needed:
 * - LWIP_NETCONN_THREAD_SEM_GET() returning a sys_sem_t*
 * - LWIP_NETCONN_THREAD_SEM_ALLOC() creating the semaphore
 * - LWIP_NETCONN_THREAD_SEM_FREE() freeing the semaphore
 * The latter 2 can be invoked up by calling netconn_thread_init()/netconn_thread_cleanup().
 * Ports may call these for threads created with sys_thread_new().
 */
#if !defined LWIP_NETCONN_SEM_PER_THREAD || defined __DOXYGEN__
#define LWIP_NETCONN_SEM_PER_THREAD     0
#endif

/** LWIP_NETCONN_FULLDUPLEX==1: Enable code that allows reading from one thread,
 * writing from a 2nd thread and closing from a 3rd thread at the same time.
 * ATTENTION: This is currently really alpha! Some requirements:
 * - LWIP_NETCONN_SEM_PER_THREAD==1 is required to use one socket/netconn from
 *   multiple threads at once
 * - sys_mbox_free() has to unblock receive tasks waiting on recvmbox/acceptmbox
 *   and prevent a task pending on this during/after deletion
 */
#if !defined LWIP_NETCONN_FULLDUPLEX || defined __DOXYGEN__
#define LWIP_NETCONN_FULLDUPLEX         0
#endif
/**
 * @}
 */

/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_socket Sockets
 * @ingroup lwip_opts_threadsafe_apis
 * @{
 */
/**
 * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
 */
#if !defined LWIP_SOCKET || defined __DOXYGEN__
#define LWIP_SOCKET                     1
#endif

/**
 * LWIP_COMPAT_SOCKETS==1: Enable BSD-style sockets functions names through defines.
 * LWIP_COMPAT_SOCKETS==2: Same as ==1 but correctly named functions are created.
 * While this helps code completion, it might conflict with existing libraries.
 * (only used if you use sockets.c)
 */
#if !defined LWIP_COMPAT_SOCKETS || defined __DOXYGEN__
#define LWIP_COMPAT_SOCKETS             1
#endif

/**
 * LWIP_POSIX_SOCKETS_IO_NAMES==1: Enable POSIX-style sockets functions names.
 * Disable this option if you use a POSIX operating system that uses the same
 * names (read, write & close). (only used if you use sockets.c)
 */
#if !defined LWIP_POSIX_SOCKETS_IO_NAMES || defined __DOXYGEN__
#define LWIP_POSIX_SOCKETS_IO_NAMES     1
#endif

/**
 * LWIP_SOCKET_OFFSET==n: Increases the file descriptor number created by LwIP with n.
 * This can be useful when there are multiple APIs which create file descriptors.
 * When they all start with a different offset and you won't make them overlap you can
 * re implement read/write/close/ioctl/fnctl to send the requested action to the right
 * library (sharing select will need more work though).
 */
#if !defined LWIP_SOCKET_OFFSET || defined __DOXYGEN__
#define LWIP_SOCKET_OFFSET              0
#endif

/**
 * LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT
 * options processing. Note that TCP_KEEPIDLE and TCP_KEEPINTVL have to be set
 * in seconds. (does not require sockets.c, and will affect tcp.c)
 */
#if !defined LWIP_TCP_KEEPALIVE || defined __DOXYGEN__
#define LWIP_TCP_KEEPALIVE              0
#endif

/**
 * LWIP_SO_SNDTIMEO==1: Enable send timeout for sockets/netconns and
 * SO_SNDTIMEO processing.
 */
#if !defined LWIP_SO_SNDTIMEO || defined __DOXYGEN__
#define LWIP_SO_SNDTIMEO                0
#endif

/**
 * LWIP_SO_RCVTIMEO==1: Enable receive timeout for sockets/netconns and
 * SO_RCVTIMEO processing.
 */
#if !defined LWIP_SO_RCVTIMEO || defined __DOXYGEN__
#define LWIP_SO_RCVTIMEO                0
#endif

/**
 * LWIP_SO_SNDRCVTIMEO_NONSTANDARD==1: SO_RCVTIMEO/SO_SNDTIMEO take an int
 * (milliseconds, much like winsock does) instead of a struct timeval (default).
 */
#if !defined LWIP_SO_SNDRCVTIMEO_NONSTANDARD || defined __DOXYGEN__
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD 0
#endif

/**
 * LWIP_SO_RCVBUF==1: Enable SO_RCVBUF processing.
 */
#if !defined LWIP_SO_RCVBUF || defined __DOXYGEN__
#define LWIP_SO_RCVBUF                  0
#endif

/**
 * LWIP_SO_LINGER==1: Enable SO_LINGER processing.
 */
#if !defined LWIP_SO_LINGER || defined __DOXYGEN__
#define LWIP_SO_LINGER                  0
#endif

/**
 * If LWIP_SO_RCVBUF is used, this is the default value for recv_bufsize.
 */
#if !defined RECV_BUFSIZE_DEFAULT || defined __DOXYGEN__
#define RECV_BUFSIZE_DEFAULT            INT_MAX
#endif

/**
 * By default, TCP socket/netconn close waits 20 seconds max to send the FIN
 */
#if !defined LWIP_TCP_CLOSE_TIMEOUT_MS_DEFAULT || defined __DOXYGEN__
#define LWIP_TCP_CLOSE_TIMEOUT_MS_DEFAULT 20000
#endif

/**
 * SO_REUSE==1: Enable SO_REUSEADDR option.
 */
#if !defined SO_REUSE || defined __DOXYGEN__
#define SO_REUSE                        0
#endif

/**
 * SO_REUSE_RXTOALL==1: Pass a copy of incoming broadcast/multicast packets
 * to all local matches if SO_REUSEADDR is turned on.
 * WARNING: Adds a memcpy for every packet if passing to more than one pcb!
 */
#if !defined SO_REUSE_RXTOALL || defined __DOXYGEN__
#define SO_REUSE_RXTOALL                0
#endif

/**
 * LWIP_FIONREAD_LINUXMODE==0 (default): ioctl/FIONREAD returns the amount of
 * pending data in the network buffer. This is the way windows does it. It's
 * the default for lwIP since it is smaller.
 * LWIP_FIONREAD_LINUXMODE==1: ioctl/FIONREAD returns the size of the next
 * pending datagram in bytes. This is the way linux does it. This code is only
 * here for compatibility.
 */
#if !defined LWIP_FIONREAD_LINUXMODE || defined __DOXYGEN__
#define LWIP_FIONREAD_LINUXMODE         0
#endif

/**
 * LWIP_SOCKET_SELECT==1 (default): enable select() for sockets (uses a netconn
 * callback to keep track of events).
 * This saves RAM (counters per socket) and code (netconn event callback), which
 * should improve performance a bit).
 */
#if !defined LWIP_SOCKET_SELECT || defined __DOXYGEN__
#define LWIP_SOCKET_SELECT              1
#endif

/**
 * LWIP_SOCKET_POLL==1 (default): enable poll() for sockets (including
 * struct pollfd, nfds_t, and constants)
 */
#if !defined LWIP_SOCKET_POLL || defined __DOXYGEN__
#define LWIP_SOCKET_POLL                1
#endif
/**
 * @}
 */

/*
   ----------------------------------------
   ---------- Statistics options ----------
   ----------------------------------------
*/
/**
 * @defgroup lwip_opts_stats Statistics
 * @ingroup lwip_opts_debug
 * @{
 */
/**
 * LWIP_STATS==1: Enable statistics collection in lwip_stats.
 */
#if !defined LWIP_STATS || defined __DOXYGEN__
#define LWIP_STATS                      1
#endif

#if LWIP_STATS

/**
 * LWIP_STATS_DISPLAY==1: Compile in the statistics output functions.
 */
#if !defined LWIP_STATS_DISPLAY || defined __DOXYGEN__
#define LWIP_STATS_DISPLAY              0
#endif

/**
 * LINK_STATS==1: Enable link stats.
 */
#if !defined LINK_STATS || defined __DOXYGEN__
#define LINK_STATS                      1
#endif

/**
 * ETHARP_STATS==1: Enable etharp stats.
 */
#if !defined ETHARP_STATS || defined __DOXYGEN__
#define ETHARP_STATS                    (LWIP_ARP)
#endif

/**
 * IP_STATS==1: Enable IP stats.
 */
#if !defined IP_STATS || defined __DOXYGEN__
#define IP_STATS                        1
#endif

/**
 * IPFRAG_STATS==1: Enable IP fragmentation stats. Default is
 * on if using either frag or reass.
 */
#if !defined IPFRAG_STATS || defined __DOXYGEN__
#define IPFRAG_STATS                    (IP_REASSEMBLY || IP_FRAG)
#endif

/**
 * ICMP_STATS==1: Enable ICMP stats.
 */
#if !defined ICMP_STATS || defined __DOXYGEN__
#define ICMP_STATS                      1
#endif

/**
 * IGMP_STATS==1: Enable IGMP stats.
 */
#if !defined IGMP_STATS || defined __DOXYGEN__
#define IGMP_STATS                      (LWIP_IGMP)
#endif

/**
 * UDP_STATS==1: Enable UDP stats. Default is on if
 * UDP enabled, otherwise off.
 */
#if !defined UDP_STATS || defined __DOXYGEN__
#define UDP_STATS                       (LWIP_UDP)
#endif

/**
 * TCP_STATS==1: Enable TCP stats. Default is on if TCP
 * enabled, otherwise off.
 */
#if !defined TCP_STATS || defined __DOXYGEN__
#define TCP_STATS                       (LWIP_TCP)
#endif

/**
 * MEM_STATS==1: Enable mem.c stats.
 */
#if !defined MEM_STATS || defined __DOXYGEN__
#define MEM_STATS                       ((MEM_LIBC_MALLOC == 0) && (MEM_USE_POOLS == 0))
#endif

/**
 * MEMP_STATS==1: Enable memp.c pool stats.
 */
#if !defined MEMP_STATS || defined __DOXYGEN__
#define MEMP_STATS                      (MEMP_MEM_MALLOC == 0)
#endif

/**
 * SYS_STATS==1: Enable system stats (sem and mbox counts, etc).
 */
#if !defined SYS_STATS || defined __DOXYGEN__
#define SYS_STATS                       (NO_SYS == 0)
#endif

/**
 * IP6_STATS==1: Enable IPv6 stats.
 */
#if !defined IP6_STATS || defined __DOXYGEN__
#define IP6_STATS                       (LWIP_IPV6)
#endif

/**
 * ICMP6_STATS==1: Enable ICMP for IPv6 stats.
 */
#if !defined ICMP6_STATS || defined __DOXYGEN__
#define ICMP6_STATS                     (LWIP_IPV6 && LWIP_ICMP6)
#endif

/**
 * IP6_FRAG_STATS==1: Enable IPv6 fragmentation stats.
 */
#if !defined IP6_FRAG_STATS || defined __DOXYGEN__
#define IP6_FRAG_STATS                  (LWIP_IPV6 && (LWIP_IPV6_FRAG || LWIP_IPV6_REASS))
#endif

/**
 * MLD6_STATS==1: Enable MLD for IPv6 stats.
 */
#if !defined MLD6_STATS || defined __DOXYGEN__
#define MLD6_STATS                      (LWIP_IPV6 && LWIP_IPV6_MLD)
#endif

/**
 * ND6_STATS==1: Enable Neighbor discovery for IPv6 stats.
 */
#if !defined ND6_STATS || defined __DOXYGEN__
#define ND6_STATS                       (LWIP_IPV6)
#endif

/**
 * MIB2_STATS==1: Stats for SNMP MIB2.
 */
#if !defined MIB2_STATS || defined __DOXYGEN__
#define MIB2_STATS                      0
#endif

#else

#define LINK_STATS                      0
#define ETHARP_STATS                    0
#define IP_STATS                        0
#define IPFRAG_STATS                    0
#define ICMP_STATS                      0
#define IGMP_STATS                      0
#define UDP_STATS                       0
#define TCP_STATS                       0
#define MEM_STATS                       0
#define MEMP_STATS                      0
#define SYS_STATS                       0
#define LWIP_STATS_DISPLAY              0
#define IP6_STATS                       0
#define ICMP6_STATS                     0
#define IP6_FRAG_STATS                  0
#define MLD6_STATS                      0
#define ND6_STATS                       0
#define MIB2_STATS                      0

#endif /* LWIP_STATS */
/**
 * @}
 */

/*
   --------------------------------------
   ---------- Checksum options ----------
   --------------------------------------
*/
/**
 * @defgroup lwip_opts_checksum Checksum
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * LWIP_CHECKSUM_CTRL_PER_NETIF==1: Checksum generation/check can be enabled/disabled
 * per netif.
 * ATTENTION: if enabled, the CHECKSUM_GEN_* and CHECKSUM_CHECK_* defines must be enabled!
 */
#if !defined LWIP_CHECKSUM_CTRL_PER_NETIF || defined __DOXYGEN__
#define LWIP_CHECKSUM_CTRL_PER_NETIF    0
#endif

/**
 * CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.
 */
#if !defined CHECKSUM_GEN_IP || defined __DOXYGEN__
#define CHECKSUM_GEN_IP                 1
#endif

/**
 * CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.
 */
#if !defined CHECKSUM_GEN_UDP || defined __DOXYGEN__
#define CHECKSUM_GEN_UDP                1
#endif

/**
 * CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.
 */
#if !defined CHECKSUM_GEN_TCP || defined __DOXYGEN__
#define CHECKSUM_GEN_TCP                1
#endif

/**
 * CHECKSUM_GEN_ICMP==1: Generate checksums in software for outgoing ICMP packets.
 */
#if !defined CHECKSUM_GEN_ICMP || defined __DOXYGEN__
#define CHECKSUM_GEN_ICMP               1
#endif

/**
 * CHECKSUM_GEN_ICMP6==1: Generate checksums in software for outgoing ICMP6 packets.
 */
#if !defined CHECKSUM_GEN_ICMP6 || defined __DOXYGEN__
#define CHECKSUM_GEN_ICMP6              1
#endif

/**
 * CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.
 */
#if !defined CHECKSUM_CHECK_IP || defined __DOXYGEN__
#define CHECKSUM_CHECK_IP               1
#endif

/**
 * CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.
 */
#if !defined CHECKSUM_CHECK_UDP || defined __DOXYGEN__
#define CHECKSUM_CHECK_UDP              1
#endif

/**
 * CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.
 */
#if !defined CHECKSUM_CHECK_TCP || defined __DOXYGEN__
#define CHECKSUM_CHECK_TCP              1
#endif

/**
 * CHECKSUM_CHECK_ICMP==1: Check checksums in software for incoming ICMP packets.
 */
#if !defined CHECKSUM_CHECK_ICMP || defined __DOXYGEN__
#define CHECKSUM_CHECK_ICMP             1
#endif

/**
 * CHECKSUM_CHECK_ICMP6==1: Check checksums in software for incoming ICMPv6 packets
 */
#if !defined CHECKSUM_CHECK_ICMP6 || defined __DOXYGEN__
#define CHECKSUM_CHECK_ICMP6            1
#endif

/**
 * LWIP_CHECKSUM_ON_COPY==1: Calculate checksum when copying data from
 * application buffers to pbufs.
 */
#if !defined LWIP_CHECKSUM_ON_COPY || defined __DOXYGEN__
#define LWIP_CHECKSUM_ON_COPY           0
#endif
/**
 * @}
 */

/*
   ---------------------------------------
   ---------- IPv6 options ---------------
   ---------------------------------------
*/
/**
 * @defgroup lwip_opts_ipv6 IPv6
 * @ingroup lwip_opts
 * @{
 */
/**
 * LWIP_IPV6==1: Enable IPv6
 */
#if !defined LWIP_IPV6 || defined __DOXYGEN__
#define LWIP_IPV6                       0
#endif

/**
 * IPV6_REASS_MAXAGE: Maximum time (in multiples of IP6_REASS_TMR_INTERVAL - so seconds, normally)
 * a fragmented IP packet waits for all fragments to arrive. If not all fragments arrived
 * in this time, the whole packet is discarded.
 */
#if !defined IPV6_REASS_MAXAGE || defined __DOXYGEN__
#define IPV6_REASS_MAXAGE               60
#endif

/**
 * LWIP_IPV6_SCOPES==1: Enable support for IPv6 address scopes, ensuring that
 * e.g. link-local addresses are really treated as link-local. Disable this
 * setting only for single-interface configurations.
 */
#if !defined LWIP_IPV6_SCOPES || defined __DOXYGEN__
#define LWIP_IPV6_SCOPES                (LWIP_IPV6 && !LWIP_SINGLE_NETIF)
#endif

/**
 * LWIP_IPV6_SCOPES_DEBUG==1: Perform run-time checks to verify that addresses
 * are properly zoned (see ip6_zone.h on what that means) where it matters.
 * Enabling this setting is highly recommended when upgrading from an existing
 * installation that is not yet scope-aware; otherwise it may be too expensive.
 */
#if !defined LWIP_IPV6_SCOPES_DEBUG || defined __DOXYGEN__
#define LWIP_IPV6_SCOPES_DEBUG          0
#endif

/**
 * LWIP_IPV6_NUM_ADDRESSES: Number of IPv6 addresses per netif.
 */
#if !defined LWIP_IPV6_NUM_ADDRESSES || defined __DOXYGEN__
#define LWIP_IPV6_NUM_ADDRESSES         3
#endif

/**
 * LWIP_IPV6_FORWARD==1: Forward IPv6 packets across netifs
 */
#if !defined LWIP_IPV6_FORWARD || defined __DOXYGEN__
#define LWIP_IPV6_FORWARD               0
#endif

/**
 * LWIP_IPV6_FRAG==1: Fragment outgoing IPv6 packets that are too big.
 */
#if !defined LWIP_IPV6_FRAG || defined __DOXYGEN__
#define LWIP_IPV6_FRAG                  0
#endif

/**
 * LWIP_IPV6_REASS==1: reassemble incoming IPv6 packets that fragmented
 */
#if !defined LWIP_IPV6_REASS || defined __DOXYGEN__
#define LWIP_IPV6_REASS                 (LWIP_IPV6)
#endif

/**
 * LWIP_IPV6_SEND_ROUTER_SOLICIT==1: Send router solicitation messages during
 * network startup.
 */
#if !defined LWIP_IPV6_SEND_ROUTER_SOLICIT || defined __DOXYGEN__
#define LWIP_IPV6_SEND_ROUTER_SOLICIT   1
#endif

/**
 * LWIP_IPV6_AUTOCONFIG==1: Enable stateless address autoconfiguration as per RFC 4862.
 */
#if !defined LWIP_IPV6_AUTOCONFIG || defined __DOXYGEN__
#define LWIP_IPV6_AUTOCONFIG            (LWIP_IPV6)
#endif

/**
 * LWIP_IPV6_ADDRESS_LIFETIMES==1: Keep valid and preferred lifetimes for each
 * IPv6 address. Required for LWIP_IPV6_AUTOCONFIG. May still be enabled
 * otherwise, in which case the application may assign address lifetimes with
 * the appropriate macros. Addresses with no lifetime are assumed to be static.
 * If this option is disabled, all addresses are assumed to be static.
 */
#if !defined LWIP_IPV6_ADDRESS_LIFETIMES || defined __DOXYGEN__
#define LWIP_IPV6_ADDRESS_LIFETIMES     (LWIP_IPV6_AUTOCONFIG)
#endif

/**
 * LWIP_IPV6_DUP_DETECT_ATTEMPTS=[0..7]: Number of duplicate address detection attempts.
 */
#if !defined LWIP_IPV6_DUP_DETECT_ATTEMPTS || defined __DOXYGEN__
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS   1
#endif
/**
 * @}
 */

/**
 * @defgroup lwip_opts_icmp6 ICMP6
 * @ingroup lwip_opts_ipv6
 * @{
 */
/**
 * LWIP_ICMP6==1: Enable ICMPv6 (mandatory per RFC)
 */
#if !defined LWIP_ICMP6 || defined __DOXYGEN__
#define LWIP_ICMP6                      (LWIP_IPV6)
#endif

/**
 * LWIP_ICMP6_DATASIZE: bytes from original packet to send back in
 * ICMPv6 error messages.
 */
#if !defined LWIP_ICMP6_DATASIZE || defined __DOXYGEN__
#define LWIP_ICMP6_DATASIZE             8
#endif

/**
 * LWIP_ICMP6_HL: default hop limit for ICMPv6 messages
 */
#if !defined LWIP_ICMP6_HL || defined __DOXYGEN__
#define LWIP_ICMP6_HL                   255
#endif
/**
 * @}
 */

/**
 * @defgroup lwip_opts_mld6 Multicast listener discovery
 * @ingroup lwip_opts_ipv6
 * @{
 */
/**
 * LWIP_IPV6_MLD==1: Enable multicast listener discovery protocol.
 * If LWIP_IPV6 is enabled but this setting is disabled, the MAC layer must
 * indiscriminately pass all inbound IPv6 multicast traffic to lwIP.
 */
#if !defined LWIP_IPV6_MLD || defined __DOXYGEN__
#define LWIP_IPV6_MLD                   (LWIP_IPV6)
#endif

/**
 * MEMP_NUM_MLD6_GROUP: Max number of IPv6 multicast groups that can be joined.
 * There must be enough groups so that each netif can join the solicited-node
 * multicast group for each of its local addresses, plus one for MDNS if
 * applicable, plus any number of groups to be joined on UDP sockets.
 */
#if !defined MEMP_NUM_MLD6_GROUP || defined __DOXYGEN__
#define MEMP_NUM_MLD6_GROUP             4
#endif
/**
 * @}
 */

/**
 * @defgroup lwip_opts_nd6 Neighbor discovery
 * @ingroup lwip_opts_ipv6
 * @{
 */
/**
 * LWIP_ND6_QUEUEING==1: queue outgoing IPv6 packets while MAC address
 * is being resolved.
 */
#if !defined LWIP_ND6_QUEUEING || defined __DOXYGEN__
#define LWIP_ND6_QUEUEING               (LWIP_IPV6)
#endif

/**
 * MEMP_NUM_ND6_QUEUE: Max number of IPv6 packets to queue during MAC resolution.
 */
#if !defined MEMP_NUM_ND6_QUEUE || defined __DOXYGEN__
#define MEMP_NUM_ND6_QUEUE              20
#endif

/**
 * LWIP_ND6_NUM_NEIGHBORS: Number of entries in IPv6 neighbor cache
 */
#if !defined LWIP_ND6_NUM_NEIGHBORS || defined __DOXYGEN__
#define LWIP_ND6_NUM_NEIGHBORS          10
#endif

/**
 * LWIP_ND6_NUM_DESTINATIONS: number of entries in IPv6 destination cache
 */
#if !defined LWIP_ND6_NUM_DESTINATIONS || defined __DOXYGEN__
#define LWIP_ND6_NUM_DESTINATIONS       10
#endif

/**
 * LWIP_ND6_NUM_PREFIXES: number of entries in IPv6 on-link prefixes cache
 */
#if !defined LWIP_ND6_NUM_PREFIXES || defined __DOXYGEN__
#define LWIP_ND6_NUM_PREFIXES           5
#endif

/**
 * LWIP_ND6_NUM_ROUTERS: number of entries in IPv6 default router cache
 */
#if !defined LWIP_ND6_NUM_ROUTERS || defined __DOXYGEN__
#define LWIP_ND6_NUM_ROUTERS            3
#endif

/**
 * LWIP_ND6_MAX_MULTICAST_SOLICIT: max number of multicast solicit messages to send
 * (neighbor solicit and router solicit)
 */
#if !defined LWIP_ND6_MAX_MULTICAST_SOLICIT || defined __DOXYGEN__
#define LWIP_ND6_MAX_MULTICAST_SOLICIT  3
#endif

/**
 * LWIP_ND6_MAX_UNICAST_SOLICIT: max number of unicast neighbor solicitation messages
 * to send during neighbor reachability detection.
 */
#if !defined LWIP_ND6_MAX_UNICAST_SOLICIT || defined __DOXYGEN__
#define LWIP_ND6_MAX_UNICAST_SOLICIT    3
#endif

/**
 * Unused: See ND RFC (time in milliseconds).
 */
#if !defined LWIP_ND6_MAX_ANYCAST_DELAY_TIME || defined __DOXYGEN__
#define LWIP_ND6_MAX_ANYCAST_DELAY_TIME 1000
#endif

/**
 * Unused: See ND RFC
 */
#if !defined LWIP_ND6_MAX_NEIGHBOR_ADVERTISEMENT || defined __DOXYGEN__
#define LWIP_ND6_MAX_NEIGHBOR_ADVERTISEMENT  3
#endif

/**
 * LWIP_ND6_REACHABLE_TIME: default neighbor reachable time (in milliseconds).
 * May be updated by router advertisement messages.
 */
#if !defined LWIP_ND6_REACHABLE_TIME || defined __DOXYGEN__
#define LWIP_ND6_REACHABLE_TIME         30000
#endif

/**
 * LWIP_ND6_RETRANS_TIMER: default retransmission timer for solicitation messages
 */
#if !defined LWIP_ND6_RETRANS_TIMER || defined __DOXYGEN__
#define LWIP_ND6_RETRANS_TIMER          1000
#endif

/**
 * LWIP_ND6_DELAY_FIRST_PROBE_TIME: Delay before first unicast neighbor solicitation
 * message is sent, during neighbor reachability detection.
 */
#if !defined LWIP_ND6_DELAY_FIRST_PROBE_TIME || defined __DOXYGEN__
#define LWIP_ND6_DELAY_FIRST_PROBE_TIME 5000
#endif

/**
 * LWIP_ND6_ALLOW_RA_UPDATES==1: Allow Router Advertisement messages to update
 * Reachable time and retransmission timers, and netif MTU.
 */
#if !defined LWIP_ND6_ALLOW_RA_UPDATES || defined __DOXYGEN__
#define LWIP_ND6_ALLOW_RA_UPDATES       1
#endif

/**
 * LWIP_ND6_TCP_REACHABILITY_HINTS==1: Allow TCP to provide Neighbor Discovery
 * with reachability hints for connected destinations. This helps avoid sending
 * unicast neighbor solicitation messages.
 */
#if !defined LWIP_ND6_TCP_REACHABILITY_HINTS || defined __DOXYGEN__
#define LWIP_ND6_TCP_REACHABILITY_HINTS 1
#endif

/**
 * LWIP_ND6_RDNSS_MAX_DNS_SERVERS > 0: Use IPv6 Router Advertisement Recursive
 * DNS Server Option (as per RFC 6106) to copy a defined maximum number of DNS
 * servers to the DNS module.
 */
#if !defined LWIP_ND6_RDNSS_MAX_DNS_SERVERS || defined __DOXYGEN__
#define LWIP_ND6_RDNSS_MAX_DNS_SERVERS  0
#endif
/**
 * @}
 */

/**
 * LWIP_IPV6_DHCP6==1: enable DHCPv6 stateful address autoconfiguration.
 */
#if !defined LWIP_IPV6_DHCP6 || defined __DOXYGEN__
#define LWIP_IPV6_DHCP6                 0
#endif

/*
   ---------------------------------------
   ---------- Hook options ---------------
   ---------------------------------------
*/

/**
 * @defgroup lwip_opts_hooks Hooks
 * @ingroup lwip_opts_infrastructure
 * Hooks are undefined by default, define them to a function if you need them.
 * @{
 */

/**
 * LWIP_HOOK_FILENAME: Custom filename to \#include in files that provide hooks.
 * Declare your hook function prototypes in there, you may also \#include all headers
 * providing data types that are need in this file.
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_FILENAME "path/to/my/lwip_hooks.h"
#endif

/**
 * LWIP_HOOK_TCP_ISN:
 * Hook for generation of the Initial Sequence Number (ISN) for a new TCP
 * connection. The default lwIP ISN generation algorithm is very basic and may
 * allow for TCP spoofing attacks. This hook provides the means to implement
 * the standardized ISN generation algorithm from RFC 6528 (see contrib/adons/tcp_isn),
 * or any other desired algorithm as a replacement.
 * Called from tcp_connect() and tcp_listen_input() when an ISN is needed for
 * a new TCP connection, if TCP support (@ref LWIP_TCP) is enabled.\n
 * Signature:
 * u32_t my_hook_tcp_isn(const ip_addr_t* local_ip, u16_t local_port, const ip_addr_t* remote_ip, u16_t remote_port);
 * - it may be necessary to use "struct ip_addr" (ip4_addr, ip6_addr) instead of "ip_addr_t" in function declarations\n
 * Arguments:
 * - local_ip: pointer to the local IP address of the connection
 * - local_port: local port number of the connection (host-byte order)
 * - remote_ip: pointer to the remote IP address of the connection
 * - remote_port: remote port number of the connection (host-byte order)\n
 * Return value:
 * - the 32-bit Initial Sequence Number to use for the new TCP connection.
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_TCP_ISN(local_ip, local_port, remote_ip, remote_port)
#endif

/**
 * LWIP_HOOK_IP4_INPUT(pbuf, input_netif):
 * Called from ip_input() (IPv4)
 * Signature:
 *   int my_hook(struct pbuf *pbuf, struct netif *input_netif);
 * Arguments:
 * - pbuf: received struct pbuf passed to ip_input()
 * - input_netif: struct netif on which the packet has been received
 * Return values:
 * - 0: Hook has not consumed the packet, packet is processed as normal
 * - != 0: Hook has consumed the packet.
 * If the hook consumed the packet, 'pbuf' is in the responsibility of the hook
 * (i.e. free it when done).
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_IP4_INPUT(pbuf, input_netif)
#endif

/**
 * LWIP_HOOK_IP4_ROUTE(dest):
 * Called from ip_route() (IPv4)
 * Signature:
 *   struct netif *my_hook(const ip4_addr_t *dest);
 * Arguments:
 * - dest: destination IPv4 address
 * Returns values:
 * - the destination netif
 * - NULL if no destination netif is found. In that case, ip_route() continues as normal.
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_IP4_ROUTE()
#endif

/**
 * LWIP_HOOK_IP4_ROUTE_SRC(src, dest):
 * Source-based routing for IPv4 - called from ip_route() (IPv4)
 * Signature:
 *   struct netif *my_hook(const ip4_addr_t *src, const ip4_addr_t *dest);
 * Arguments:
 * - src: local/source IPv4 address
 * - dest: destination IPv4 address
 * Returns values:
 * - the destination netif
 * - NULL if no destination netif is found. In that case, ip_route() continues as normal.
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_IP4_ROUTE_SRC(src, dest)
#endif

/**
 * LWIP_HOOK_ETHARP_GET_GW(netif, dest):
 * Called from etharp_output() (IPv4)
 * Signature:
 *   const ip4_addr_t *my_hook(struct netif *netif, const ip4_addr_t *dest);
 * Arguments:
 * - netif: the netif used for sending
 * - dest: the destination IPv4 address
 * Return values:
 * - the IPv4 address of the gateway to handle the specified destination IPv4 address
 * - NULL, in which case the netif's default gateway is used
 *
 * The returned address MUST be directly reachable on the specified netif!
 * This function is meant to implement advanced IPv4 routing together with
 * LWIP_HOOK_IP4_ROUTE(). The actual routing/gateway table implementation is
 * not part of lwIP but can e.g. be hidden in the netif's state argument.
*/
#ifdef __DOXYGEN__
#define LWIP_HOOK_ETHARP_GET_GW(netif, dest)
#endif

/**
 * LWIP_HOOK_IP6_INPUT(pbuf, input_netif):
 * Called from ip6_input() (IPv6)
 * Signature:
 *   int my_hook(struct pbuf *pbuf, struct netif *input_netif);
 * Arguments:
 * - pbuf: received struct pbuf passed to ip6_input()
 * - input_netif: struct netif on which the packet has been received
 * Return values:
 * - 0: Hook has not consumed the packet, packet is processed as normal
 * - != 0: Hook has consumed the packet.
 * If the hook consumed the packet, 'pbuf' is in the responsibility of the hook
 * (i.e. free it when done).
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_IP6_INPUT(pbuf, input_netif)
#endif

/**
 * LWIP_HOOK_IP6_ROUTE(src, dest):
 * Called from ip_route() (IPv6)
 * Signature:
 *   struct netif *my_hook(const ip6_addr_t *dest, const ip6_addr_t *src);
 * Arguments:
 * - src: source IPv6 address
 * - dest: destination IPv6 address
 * Return values:
 * - the destination netif
 * - NULL if no destination netif is found. In that case, ip6_route() continues as normal.
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_IP6_ROUTE(src, dest)
#endif

/**
 * LWIP_HOOK_ND6_GET_GW(netif, dest):
 * Called from nd6_get_next_hop_entry() (IPv6)
 * Signature:
 *   const ip6_addr_t *my_hook(struct netif *netif, const ip6_addr_t *dest);
 * Arguments:
 * - netif: the netif used for sending
 * - dest: the destination IPv6 address
 * Return values:
 * - the IPv6 address of the next hop to handle the specified destination IPv6 address
 * - NULL, in which case a NDP-discovered router is used instead
 *
 * The returned address MUST be directly reachable on the specified netif!
 * This function is meant to implement advanced IPv6 routing together with
 * LWIP_HOOK_IP6_ROUTE(). The actual routing/gateway table implementation is
 * not part of lwIP but can e.g. be hidden in the netif's state argument.
*/
#ifdef __DOXYGEN__
#define LWIP_HOOK_ND6_GET_GW(netif, dest)
#endif

/**
 * LWIP_HOOK_VLAN_CHECK(netif, eth_hdr, vlan_hdr):
 * Called from ethernet_input() if VLAN support is enabled
 * Signature:
 *   int my_hook(struct netif *netif, struct eth_hdr *eth_hdr, struct eth_vlan_hdr *vlan_hdr);
 * Arguments:
 * - netif: struct netif on which the packet has been received
 * - eth_hdr: struct eth_hdr of the packet
 * - vlan_hdr: struct eth_vlan_hdr of the packet
 * Return values:
 * - 0: Packet must be dropped.
 * - != 0: Packet must be accepted.
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_VLAN_CHECK(netif, eth_hdr, vlan_hdr)
#endif

/**
 * LWIP_HOOK_VLAN_SET:
 * Hook can be used to set prio_vid field of vlan_hdr. If you need to store data
 * on per-netif basis to implement this callback, see @ref netif_cd.
 * Called from ethernet_output() if VLAN support (@ref ETHARP_SUPPORT_VLAN) is enabled.\n
 * Signature:
 *   s32_t my_hook_vlan_set(struct netif* netif, struct pbuf* pbuf, const struct eth_addr* src, const struct eth_addr* dst, u16_t eth_type);\n
 * Arguments:
 * - netif: struct netif that the packet will be sent through
 * - p: struct pbuf packet to be sent
 * - src: source eth address
 * - dst: destination eth address
 * - eth_type: ethernet type to packet to be sent\n
 * 
 * 
 * Return values:
 * - &lt;0: Packet shall not contain VLAN header.
 * - 0 &lt;= return value &lt;= 0xFFFF: Packet shall contain VLAN header. Return value is prio_vid in host byte order.
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_VLAN_SET(netif, p, src, dst, eth_type)
#endif

/**
 * LWIP_HOOK_MEMP_AVAILABLE(memp_t_type):
 * Called from memp_free() when a memp pool was empty and an item is now available
 * Signature:
 *   void my_hook(memp_t type);
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_MEMP_AVAILABLE(memp_t_type)
#endif

/**
 * LWIP_HOOK_UNKNOWN_ETH_PROTOCOL(pbuf, netif):
 * Called from ethernet_input() when an unknown eth type is encountered.
 * Signature:
 *   err_t my_hook(struct pbuf* pbuf, struct netif* netif);
 * Arguments:
 * - p: rx packet with unknown eth type
 * - netif: netif on which the packet has been received
 * Return values:
 * - ERR_OK if packet is accepted (hook function now owns the pbuf)
 * - any error code otherwise (pbuf is freed)
 *
 * Payload points to ethernet header!
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_UNKNOWN_ETH_PROTOCOL(pbuf, netif)
#endif

/**
 * LWIP_HOOK_DHCP_APPEND_OPTIONS(netif, dhcp, state, msg, msg_type, options_len_ptr):
 * Called from various dhcp functions when sending a DHCP message.
 * This hook is called just before the DHCP message trailer is added, so the
 * options are at the end of a DHCP message.
 * Signature:
 *   void my_hook(struct netif *netif, struct dhcp *dhcp, u8_t state, struct dhcp_msg *msg,
 *                u8_t msg_type, u16_t *options_len_ptr);
 * Arguments:
 * - netif: struct netif that the packet will be sent through
 * - dhcp: struct dhcp on that netif
 * - state: current dhcp state (dhcp_state_enum_t as an u8_t)
 * - msg: struct dhcp_msg that will be sent
 * - msg_type: dhcp message type to be sent (u8_t)
 * - options_len_ptr: pointer to the current length of options in the dhcp_msg "msg"
 *                    (must be increased when options are added!)
 *
 * Options need to appended like this:
 *   LWIP_ASSERT("dhcp option overflow", *options_len_ptr + option_len + 2 <= DHCP_OPTIONS_LEN);
 *   msg->options[(*options_len_ptr)++] = &lt;option_number&gt;;
 *   msg->options[(*options_len_ptr)++] = &lt;option_len&gt;;
 *   msg->options[(*options_len_ptr)++] = &lt;option_bytes&gt;;
 *   [...]
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_DHCP_APPEND_OPTIONS(netif, dhcp, state, msg, msg_type, options_len_ptr)
#endif

/**
 * LWIP_HOOK_DHCP_PARSE_OPTION(netif, dhcp, state, msg, msg_type, option, len, pbuf, option_value_offset):
 * Called from dhcp_parse_reply when receiving a DHCP message.
 * This hook is called for every option in the received message that is not handled internally.
 * Signature:
 *   void my_hook(struct netif *netif, struct dhcp *dhcp, u8_t state, struct dhcp_msg *msg,
 *                u8_t msg_type, u8_t option, u8_t option_len, struct pbuf *pbuf, u16_t option_value_offset);
 * Arguments:
 * - netif: struct netif that the packet will be sent through
 * - dhcp: struct dhcp on that netif
 * - state: current dhcp state (dhcp_state_enum_t as an u8_t)
 * - msg: struct dhcp_msg that was received
 * - msg_type: dhcp message type received (u8_t, ATTENTION: only valid after
 *             the message type option has been parsed!)
 * - option: option value (u8_t)
 * - len: option data length (u8_t)
 * - pbuf: pbuf where option data is contained
 * - option_value_offset: offset in pbuf where option *data* begins
 *
 * A nice way to get the option contents is pbuf_get_contiguous():
 *  u8_t buf[32];
 *  u8_t *ptr = (u8_t*)pbuf_get_contiguous(p, buf, sizeof(buf), LWIP_MIN(option_len, sizeof(buf)), offset);
 */
#ifdef __DOXYGEN__
#define LWIP_HOOK_DHCP_PARSE_OPTION(netif, dhcp, state, msg, msg_type, option, len, pbuf, offset)
#endif
/**
 * @}
 */

/*
   ---------------------------------------
   ---------- Debugging options ----------
   ---------------------------------------
*/
/**
 * @defgroup lwip_opts_debugmsg Debug messages
 * @ingroup lwip_opts_debug
 * @{
 */
/**
 * LWIP_DBG_MIN_LEVEL: After masking, the value of the debug is
 * compared against this value. If it is smaller, then debugging
 * messages are written.
 * @see debugging_levels
 */
#if !defined LWIP_DBG_MIN_LEVEL || defined __DOXYGEN__
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL
#endif

/**
 * LWIP_DBG_TYPES_ON: A mask that can be used to globally enable/disable
 * debug messages of certain types.
 * @see debugging_levels
 */
#if !defined LWIP_DBG_TYPES_ON || defined __DOXYGEN__
#define LWIP_DBG_TYPES_ON               LWIP_DBG_ON
#endif

/**
 * ETHARP_DEBUG: Enable debugging in etharp.c.
 */
#if !defined ETHARP_DEBUG || defined __DOXYGEN__
#define ETHARP_DEBUG                    LWIP_DBG_OFF
#endif

/**
 * NETIF_DEBUG: Enable debugging in netif.c.
 */
#if !defined NETIF_DEBUG || defined __DOXYGEN__
#define NETIF_DEBUG                     LWIP_DBG_OFF
#endif

/**
 * PBUF_DEBUG: Enable debugging in pbuf.c.
 */
#if !defined PBUF_DEBUG || defined __DOXYGEN__
#define PBUF_DEBUG                      LWIP_DBG_OFF
#endif

/**
 * API_LIB_DEBUG: Enable debugging in api_lib.c.
 */
#if !defined API_LIB_DEBUG || defined __DOXYGEN__
#define API_LIB_DEBUG                   LWIP_DBG_OFF
#endif

/**
 * API_MSG_DEBUG: Enable debugging in api_msg.c.
 */
#if !defined API_MSG_DEBUG || defined __DOXYGEN__
#define API_MSG_DEBUG                   LWIP_DBG_OFF
#endif

/**
 * SOCKETS_DEBUG: Enable debugging in sockets.c.
 */
#if !defined SOCKETS_DEBUG || defined __DOXYGEN__
#define SOCKETS_DEBUG                   LWIP_DBG_OFF
#endif

/**
 * ICMP_DEBUG: Enable debugging in icmp.c.
 */
#if !defined ICMP_DEBUG || defined __DOXYGEN__
#define ICMP_DEBUG                      LWIP_DBG_OFF
#endif

/**
 * IGMP_DEBUG: Enable debugging in igmp.c.
 */
#if !defined IGMP_DEBUG || defined __DOXYGEN__
#define IGMP_DEBUG                      LWIP_DBG_OFF
#endif

/**
 * INET_DEBUG: Enable debugging in inet.c.
 */
#if !defined INET_DEBUG || defined __DOXYGEN__
#define INET_DEBUG                      LWIP_DBG_OFF
#endif

/**
 * IP_DEBUG: Enable debugging for IP.
 */
#if !defined IP_DEBUG || defined __DOXYGEN__
#define IP_DEBUG                        LWIP_DBG_OFF
#endif

/**
 * IP_REASS_DEBUG: Enable debugging in ip_frag.c for both frag & reass.
 */
#if !defined IP_REASS_DEBUG || defined __DOXYGEN__
#define IP_REASS_DEBUG                  LWIP_DBG_OFF
#endif

/**
 * RAW_DEBUG: Enable debugging in raw.c.
 */
#if !defined RAW_DEBUG || defined __DOXYGEN__
#define RAW_DEBUG                       LWIP_DBG_OFF
#endif

/**
 * MEM_DEBUG: Enable debugging in mem.c.
 */
#if !defined MEM_DEBUG || defined __DOXYGEN__
#define MEM_DEBUG                       LWIP_DBG_OFF
#endif

/**
 * MEMP_DEBUG: Enable debugging in memp.c.
 */
#if !defined MEMP_DEBUG || defined __DOXYGEN__
#define MEMP_DEBUG                      LWIP_DBG_OFF
#endif

/**
 * SYS_DEBUG: Enable debugging in sys.c.
 */
#if !defined SYS_DEBUG || defined __DOXYGEN__
#define SYS_DEBUG                       LWIP_DBG_OFF
#endif

/**
 * TIMERS_DEBUG: Enable debugging in timers.c.
 */
#if !defined TIMERS_DEBUG || defined __DOXYGEN__
#define TIMERS_DEBUG                    LWIP_DBG_OFF
#endif

/**
 * TCP_DEBUG: Enable debugging for TCP.
 */
#if !defined TCP_DEBUG || defined __DOXYGEN__
#define TCP_DEBUG                       LWIP_DBG_OFF
#endif

/**
 * TCP_INPUT_DEBUG: Enable debugging in tcp_in.c for incoming debug.
 */
#if !defined TCP_INPUT_DEBUG || defined __DOXYGEN__
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF
#endif

/**
 * TCP_FR_DEBUG: Enable debugging in tcp_in.c for fast retransmit.
 */
#if !defined TCP_FR_DEBUG || defined __DOXYGEN__
#define TCP_FR_DEBUG                    LWIP_DBG_OFF
#endif

/**
 * TCP_RTO_DEBUG: Enable debugging in TCP for retransmit
 * timeout.
 */
#if !defined TCP_RTO_DEBUG || defined __DOXYGEN__
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF
#endif

/**
 * TCP_CWND_DEBUG: Enable debugging for TCP congestion window.
 */
#if !defined TCP_CWND_DEBUG || defined __DOXYGEN__
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF
#endif

/**
 * TCP_WND_DEBUG: Enable debugging in tcp_in.c for window updating.
 */
#if !defined TCP_WND_DEBUG || defined __DOXYGEN__
#define TCP_WND_DEBUG                   LWIP_DBG_OFF
#endif

/**
 * TCP_OUTPUT_DEBUG: Enable debugging in tcp_out.c output functions.
 */
#if !defined TCP_OUTPUT_DEBUG || defined __DOXYGEN__
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF
#endif

/**
 * TCP_RST_DEBUG: Enable debugging for TCP with the RST message.
 */
#if !defined TCP_RST_DEBUG || defined __DOXYGEN__
#define TCP_RST_DEBUG                   LWIP_DBG_OFF
#endif

/**
 * TCP_QLEN_DEBUG: Enable debugging for TCP queue lengths.
 */
#if !defined TCP_QLEN_DEBUG || defined __DOXYGEN__
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF
#endif

/**
 * UDP_DEBUG: Enable debugging in UDP.
 */
#if !defined UDP_DEBUG || defined __DOXYGEN__
#define UDP_DEBUG                       LWIP_DBG_OFF
#endif

/**
 * TCPIP_DEBUG: Enable debugging in tcpip.c.
 */
#if !defined TCPIP_DEBUG || defined __DOXYGEN__
#define TCPIP_DEBUG                     LWIP_DBG_OFF
#endif

/**
 * SLIP_DEBUG: Enable debugging in slipif.c.
 */
#if !defined SLIP_DEBUG || defined __DOXYGEN__
#define SLIP_DEBUG                      LWIP_DBG_OFF
#endif

/**
 * DHCP_DEBUG: Enable debugging in dhcp.c.
 */
#if !defined DHCP_DEBUG || defined __DOXYGEN__
#define DHCP_DEBUG                      LWIP_DBG_OFF
#endif

/**
 * AUTOIP_DEBUG: Enable debugging in autoip.c.
 */
#if !defined AUTOIP_DEBUG || defined __DOXYGEN__
#define AUTOIP_DEBUG                    LWIP_DBG_OFF
#endif

/**
 * DNS_DEBUG: Enable debugging for DNS.
 */
#if !defined DNS_DEBUG || defined __DOXYGEN__
#define DNS_DEBUG                       LWIP_DBG_OFF
#endif

/**
 * IP6_DEBUG: Enable debugging for IPv6.
 */
#if !defined IP6_DEBUG || defined __DOXYGEN__
#define IP6_DEBUG                       LWIP_DBG_OFF
#endif
/**
 * @}
 */

/*
   --------------------------------------------------
   ---------- Performance tracking options ----------
   --------------------------------------------------
*/
/**
 * @defgroup lwip_opts_perf Performance
 * @ingroup lwip_opts_debug
 * @{
 */
/**
 * LWIP_PERF: Enable performance testing for lwIP
 * (if enabled, arch/perf.h is included)
 */
#if !defined LWIP_PERF || defined __DOXYGEN__
#define LWIP_PERF                       0
#endif
/**
 * @}
 */

#endif /* LWIP_HDR_OPT_H */
