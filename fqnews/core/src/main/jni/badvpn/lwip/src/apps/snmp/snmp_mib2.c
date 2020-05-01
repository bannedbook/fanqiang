/**
 * @file
 * Management Information Base II (RFC1213) objects and functions.
 */

/*
 * Copyright (c) 2006 Axon Digital Design B.V., The Netherlands.
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
 * Author: Dirk Ziegelmeier <dziegel@gmx.de>
 *         Christiaan Simons <christiaan.simons@axon.tv>
 */

/**
 * @defgroup snmp_mib2 MIB2
 * @ingroup snmp
 */

#include "lwip/apps/snmp_opts.h"

#if LWIP_SNMP && SNMP_LWIP_MIB2 /* don't build if not configured for use in lwipopts.h */

#if !LWIP_STATS
#error LWIP_SNMP MIB2 needs LWIP_STATS (for MIB2)
#endif
#if !MIB2_STATS
#error LWIP_SNMP MIB2 needs MIB2_STATS (for MIB2)
#endif

#include "lwip/snmp.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_core.h"
#include "lwip/apps/snmp_mib2.h"
#include "lwip/apps/snmp_scalar.h"

#if SNMP_USE_NETCONN
#include "lwip/tcpip.h"
#include "lwip/priv/tcpip_priv.h"
void
snmp_mib2_lwip_synchronizer(snmp_threadsync_called_fn fn, void *arg)
{
#if LWIP_TCPIP_CORE_LOCKING
  LOCK_TCPIP_CORE();
  fn(arg);
  UNLOCK_TCPIP_CORE();
#else
  tcpip_callback(fn, arg);
#endif
}

struct snmp_threadsync_instance snmp_mib2_lwip_locks;
#endif

/* dot3 and EtherLike MIB not planned. (transmission .1.3.6.1.2.1.10) */
/* historical (some say hysterical). (cmot .1.3.6.1.2.1.9) */
/* lwIP has no EGP, thus may not implement it. (egp .1.3.6.1.2.1.8) */

/* --- mib-2 .1.3.6.1.2.1 ----------------------------------------------------- */
extern const struct snmp_scalar_array_node snmp_mib2_snmp_root;
extern const struct snmp_tree_node snmp_mib2_udp_root;
extern const struct snmp_tree_node snmp_mib2_tcp_root;
extern const struct snmp_scalar_array_node snmp_mib2_icmp_root;
extern const struct snmp_tree_node snmp_mib2_interface_root;
extern const struct snmp_scalar_array_node snmp_mib2_system_node;
extern const struct snmp_tree_node snmp_mib2_at_root;
extern const struct snmp_tree_node snmp_mib2_ip_root;

static const struct snmp_node *const mib2_nodes[] = {
  &snmp_mib2_system_node.node.node,
  &snmp_mib2_interface_root.node,
#if LWIP_ARP && LWIP_IPV4
  &snmp_mib2_at_root.node,
#endif /* LWIP_ARP && LWIP_IPV4 */
#if LWIP_IPV4
  &snmp_mib2_ip_root.node,
#endif /* LWIP_IPV4 */
#if LWIP_ICMP
  &snmp_mib2_icmp_root.node.node,
#endif /* LWIP_ICMP */
#if LWIP_TCP
  &snmp_mib2_tcp_root.node,
#endif /* LWIP_TCP */
#if LWIP_UDP
  &snmp_mib2_udp_root.node,
#endif /* LWIP_UDP */
  &snmp_mib2_snmp_root.node.node
};

static const struct snmp_tree_node mib2_root = SNMP_CREATE_TREE_NODE(1, mib2_nodes);

static const u32_t  mib2_base_oid_arr[] = { 1, 3, 6, 1, 2, 1 };
const struct snmp_mib mib2 = SNMP_MIB_CREATE(mib2_base_oid_arr, &mib2_root.node);

#endif /* LWIP_SNMP && SNMP_LWIP_MIB2 */
