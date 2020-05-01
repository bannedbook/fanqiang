/**
 * @file
 * Statistics module
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
 *
 */

#include "lwip/opt.h"

#if LWIP_STATS /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/mem.h"
#include "lwip/debug.h"

#include <string.h>

struct stats_ lwip_stats;

void
stats_init(void)
{
#ifdef LWIP_DEBUG
#if MEM_STATS
  lwip_stats.mem.name = "MEM";
#endif /* MEM_STATS */
#endif /* LWIP_DEBUG */
}

#if LWIP_STATS_DISPLAY
void
stats_display_proto(struct stats_proto *proto, const char *name)
{
  LWIP_PLATFORM_DIAG(("\n%s\n\t", name));
  LWIP_PLATFORM_DIAG(("xmit: %"STAT_COUNTER_F"\n\t", proto->xmit));
  LWIP_PLATFORM_DIAG(("recv: %"STAT_COUNTER_F"\n\t", proto->recv));
  LWIP_PLATFORM_DIAG(("fw: %"STAT_COUNTER_F"\n\t", proto->fw));
  LWIP_PLATFORM_DIAG(("drop: %"STAT_COUNTER_F"\n\t", proto->drop));
  LWIP_PLATFORM_DIAG(("chkerr: %"STAT_COUNTER_F"\n\t", proto->chkerr));
  LWIP_PLATFORM_DIAG(("lenerr: %"STAT_COUNTER_F"\n\t", proto->lenerr));
  LWIP_PLATFORM_DIAG(("memerr: %"STAT_COUNTER_F"\n\t", proto->memerr));
  LWIP_PLATFORM_DIAG(("rterr: %"STAT_COUNTER_F"\n\t", proto->rterr));
  LWIP_PLATFORM_DIAG(("proterr: %"STAT_COUNTER_F"\n\t", proto->proterr));
  LWIP_PLATFORM_DIAG(("opterr: %"STAT_COUNTER_F"\n\t", proto->opterr));
  LWIP_PLATFORM_DIAG(("err: %"STAT_COUNTER_F"\n\t", proto->err));
  LWIP_PLATFORM_DIAG(("cachehit: %"STAT_COUNTER_F"\n", proto->cachehit));
}

#if IGMP_STATS || MLD6_STATS
void
stats_display_igmp(struct stats_igmp *igmp, const char *name)
{
  LWIP_PLATFORM_DIAG(("\n%s\n\t", name));
  LWIP_PLATFORM_DIAG(("xmit: %"STAT_COUNTER_F"\n\t", igmp->xmit));
  LWIP_PLATFORM_DIAG(("recv: %"STAT_COUNTER_F"\n\t", igmp->recv));
  LWIP_PLATFORM_DIAG(("drop: %"STAT_COUNTER_F"\n\t", igmp->drop));
  LWIP_PLATFORM_DIAG(("chkerr: %"STAT_COUNTER_F"\n\t", igmp->chkerr));
  LWIP_PLATFORM_DIAG(("lenerr: %"STAT_COUNTER_F"\n\t", igmp->lenerr));
  LWIP_PLATFORM_DIAG(("memerr: %"STAT_COUNTER_F"\n\t", igmp->memerr));
  LWIP_PLATFORM_DIAG(("proterr: %"STAT_COUNTER_F"\n\t", igmp->proterr));
  LWIP_PLATFORM_DIAG(("rx_v1: %"STAT_COUNTER_F"\n\t", igmp->rx_v1));
  LWIP_PLATFORM_DIAG(("rx_group: %"STAT_COUNTER_F"\n\t", igmp->rx_group));
  LWIP_PLATFORM_DIAG(("rx_general: %"STAT_COUNTER_F"\n\t", igmp->rx_general));
  LWIP_PLATFORM_DIAG(("rx_report: %"STAT_COUNTER_F"\n\t", igmp->rx_report));
  LWIP_PLATFORM_DIAG(("tx_join: %"STAT_COUNTER_F"\n\t", igmp->tx_join));
  LWIP_PLATFORM_DIAG(("tx_leave: %"STAT_COUNTER_F"\n\t", igmp->tx_leave));
  LWIP_PLATFORM_DIAG(("tx_report: %"STAT_COUNTER_F"\n", igmp->tx_report));
}
#endif /* IGMP_STATS || MLD6_STATS */

#if MEM_STATS || MEMP_STATS
void
stats_display_mem(struct stats_mem *mem, const char *name)
{
  LWIP_PLATFORM_DIAG(("\nMEM %s\n\t", name));
  LWIP_PLATFORM_DIAG(("avail: %"MEM_SIZE_F"\n\t", mem->avail));
  LWIP_PLATFORM_DIAG(("used: %"MEM_SIZE_F"\n\t", mem->used));
  LWIP_PLATFORM_DIAG(("max: %"MEM_SIZE_F"\n\t", mem->max));
  LWIP_PLATFORM_DIAG(("err: %"STAT_COUNTER_F"\n", mem->err));
}

#if MEMP_STATS
void
stats_display_memp(struct stats_mem *mem, int idx)
{
  if (idx < MEMP_MAX) {
    stats_display_mem(mem, mem->name);
  }
}
#endif /* MEMP_STATS */
#endif /* MEM_STATS || MEMP_STATS */

#if SYS_STATS
void
stats_display_sys(struct stats_sys *sys)
{
  LWIP_PLATFORM_DIAG(("\nSYS\n\t"));
  LWIP_PLATFORM_DIAG(("sem.used:  %"STAT_COUNTER_F"\n\t", sys->sem.used));
  LWIP_PLATFORM_DIAG(("sem.max:   %"STAT_COUNTER_F"\n\t", sys->sem.max));
  LWIP_PLATFORM_DIAG(("sem.err:   %"STAT_COUNTER_F"\n\t", sys->sem.err));
  LWIP_PLATFORM_DIAG(("mutex.used: %"STAT_COUNTER_F"\n\t", sys->mutex.used));
  LWIP_PLATFORM_DIAG(("mutex.max:  %"STAT_COUNTER_F"\n\t", sys->mutex.max));
  LWIP_PLATFORM_DIAG(("mutex.err:  %"STAT_COUNTER_F"\n\t", sys->mutex.err));
  LWIP_PLATFORM_DIAG(("mbox.used:  %"STAT_COUNTER_F"\n\t", sys->mbox.used));
  LWIP_PLATFORM_DIAG(("mbox.max:   %"STAT_COUNTER_F"\n\t", sys->mbox.max));
  LWIP_PLATFORM_DIAG(("mbox.err:   %"STAT_COUNTER_F"\n", sys->mbox.err));
}
#endif /* SYS_STATS */

void
stats_display(void)
{
  s16_t i;

  LINK_STATS_DISPLAY();
  ETHARP_STATS_DISPLAY();
  IPFRAG_STATS_DISPLAY();
  IP6_FRAG_STATS_DISPLAY();
  IP_STATS_DISPLAY();
  ND6_STATS_DISPLAY();
  IP6_STATS_DISPLAY();
  IGMP_STATS_DISPLAY();
  MLD6_STATS_DISPLAY();
  ICMP_STATS_DISPLAY();
  ICMP6_STATS_DISPLAY();
  UDP_STATS_DISPLAY();
  TCP_STATS_DISPLAY();
  MEM_STATS_DISPLAY();
  for (i = 0; i < MEMP_MAX; i++) {
    MEMP_STATS_DISPLAY(i);
  }
  SYS_STATS_DISPLAY();
}
#endif /* LWIP_STATS_DISPLAY */

#endif /* LWIP_STATS */

