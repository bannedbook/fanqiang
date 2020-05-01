/**
 * @file
 * IGMP - Internet Group Management Protocol
 *
 * @defgroup igmp IGMP
 * @ingroup ip4
 * To be called from TCPIP thread
 */

/*
 * Copyright (c) 2002 CITEL Technologies Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of CITEL Technologies Ltd nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CITEL TECHNOLOGIES AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CITEL TECHNOLOGIES OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is a contribution to the lwIP TCP/IP stack.
 * The Swedish Institute of Computer Science and Adam Dunkels
 * are specifically granted permission to redistribute this
 * source code.
*/

/*-------------------------------------------------------------
Note 1)
Although the rfc requires V1 AND V2 capability
we will only support v2 since now V1 is very old (August 1989)
V1 can be added if required

a debug print and statistic have been implemented to
show this up.
-------------------------------------------------------------
-------------------------------------------------------------
Note 2)
A query for a specific group address (as opposed to ALLHOSTS)
has now been implemented as I am unsure if it is required

a debug print and statistic have been implemented to
show this up.
-------------------------------------------------------------
-------------------------------------------------------------
Note 3)
The router alert rfc 2113 is implemented in outgoing packets
but not checked rigorously incoming
-------------------------------------------------------------
Steve Reynolds
------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * RFC 988  - Host extensions for IP multicasting                         - V0
 * RFC 1054 - Host extensions for IP multicasting                         -
 * RFC 1112 - Host extensions for IP multicasting                         - V1
 * RFC 2236 - Internet Group Management Protocol, Version 2               - V2  <- this code is based on this RFC (it's the "de facto" standard)
 * RFC 3376 - Internet Group Management Protocol, Version 3               - V3
 * RFC 4604 - Using Internet Group Management Protocol Version 3...       - V3+
 * RFC 2113 - IP Router Alert Option                                      -
 *----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * Includes
 *----------------------------------------------------------------------------*/

#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_IGMP /* don't build if not configured for use in lwipopts.h */

#include "lwip/igmp.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/ip.h"
#include "lwip/inet_chksum.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/prot/igmp.h"

#include "string.h"

static struct igmp_group *igmp_lookup_group(struct netif *ifp, const ip4_addr_t *addr);
static err_t  igmp_remove_group(struct netif *netif, struct igmp_group *group);
static void   igmp_timeout(struct netif *netif, struct igmp_group *group);
static void   igmp_start_timer(struct igmp_group *group, u8_t max_time);
static void   igmp_delaying_member(struct igmp_group *group, u8_t maxresp);
static err_t  igmp_ip_output_if(struct pbuf *p, const ip4_addr_t *src, const ip4_addr_t *dest, struct netif *netif);
static void   igmp_send(struct netif *netif, struct igmp_group *group, u8_t type);

static ip4_addr_t     allsystems;
static ip4_addr_t     allrouters;

/**
 * Initialize the IGMP module
 */
void
igmp_init(void)
{
  LWIP_DEBUGF(IGMP_DEBUG, ("igmp_init: initializing\n"));

  IP4_ADDR(&allsystems, 224, 0, 0, 1);
  IP4_ADDR(&allrouters, 224, 0, 0, 2);
}

/**
 * Start IGMP processing on interface
 *
 * @param netif network interface on which start IGMP processing
 */
err_t
igmp_start(struct netif *netif)
{
  struct igmp_group *group;

  LWIP_DEBUGF(IGMP_DEBUG, ("igmp_start: starting IGMP processing on if %p\n", (void *)netif));

  group = igmp_lookup_group(netif, &allsystems);

  if (group != NULL) {
    group->group_state = IGMP_GROUP_IDLE_MEMBER;
    group->use++;

    /* Allow the igmp messages at the MAC level */
    if (netif->igmp_mac_filter != NULL) {
      LWIP_DEBUGF(IGMP_DEBUG, ("igmp_start: igmp_mac_filter(ADD "));
      ip4_addr_debug_print_val(IGMP_DEBUG, allsystems);
      LWIP_DEBUGF(IGMP_DEBUG, (") on if %p\n", (void *)netif));
      netif->igmp_mac_filter(netif, &allsystems, NETIF_ADD_MAC_FILTER);
    }

    return ERR_OK;
  }

  return ERR_MEM;
}

/**
 * Stop IGMP processing on interface
 *
 * @param netif network interface on which stop IGMP processing
 */
err_t
igmp_stop(struct netif *netif)
{
  struct igmp_group *group = netif_igmp_data(netif);

  netif_set_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_IGMP, NULL);

  while (group != NULL) {
    struct igmp_group *next = group->next; /* avoid use-after-free below */

    /* disable the group at the MAC level */
    if (netif->igmp_mac_filter != NULL) {
      LWIP_DEBUGF(IGMP_DEBUG, ("igmp_stop: igmp_mac_filter(DEL "));
      ip4_addr_debug_print(IGMP_DEBUG, &group->group_address);
      LWIP_DEBUGF(IGMP_DEBUG, (") on if %p\n", (void *)netif));
      netif->igmp_mac_filter(netif, &(group->group_address), NETIF_DEL_MAC_FILTER);
    }

    /* free group */
    memp_free(MEMP_IGMP_GROUP, group);

    /* move to "next" */
    group = next;
  }
  return ERR_OK;
}

/**
 * Report IGMP memberships for this interface
 *
 * @param netif network interface on which report IGMP memberships
 */
void
igmp_report_groups(struct netif *netif)
{
  struct igmp_group *group = netif_igmp_data(netif);

  LWIP_DEBUGF(IGMP_DEBUG, ("igmp_report_groups: sending IGMP reports on if %p\n", (void *)netif));

  /* Skip the first group in the list, it is always the allsystems group added in igmp_start() */
  if (group != NULL) {
    group = group->next;
  }

  while (group != NULL) {
    igmp_delaying_member(group, IGMP_JOIN_DELAYING_MEMBER_TMR);
    group = group->next;
  }
}

/**
 * Search for a group in the netif's igmp group list
 *
 * @param ifp the network interface for which to look
 * @param addr the group ip address to search for
 * @return a struct igmp_group* if the group has been found,
 *         NULL if the group wasn't found.
 */
struct igmp_group *
igmp_lookfor_group(struct netif *ifp, const ip4_addr_t *addr)
{
  struct igmp_group *group = netif_igmp_data(ifp);

  while (group != NULL) {
    if (ip4_addr_cmp(&(group->group_address), addr)) {
      return group;
    }
    group = group->next;
  }

  /* to be clearer, we return NULL here instead of
   * 'group' (which is also NULL at this point).
   */
  return NULL;
}

/**
 * Search for a specific igmp group and create a new one if not found-
 *
 * @param ifp the network interface for which to look
 * @param addr the group ip address to search
 * @return a struct igmp_group*,
 *         NULL on memory error.
 */
static struct igmp_group *
igmp_lookup_group(struct netif *ifp, const ip4_addr_t *addr)
{
  struct igmp_group *group;
  struct igmp_group *list_head = netif_igmp_data(ifp);

  /* Search if the group already exists */
  group = igmp_lookfor_group(ifp, addr);
  if (group != NULL) {
    /* Group already exists. */
    return group;
  }

  /* Group doesn't exist yet, create a new one */
  group = (struct igmp_group *)memp_malloc(MEMP_IGMP_GROUP);
  if (group != NULL) {
    ip4_addr_set(&(group->group_address), addr);
    group->timer              = 0; /* Not running */
    group->group_state        = IGMP_GROUP_NON_MEMBER;
    group->last_reporter_flag = 0;
    group->use                = 0;

    /* Ensure allsystems group is always first in list */
    if (list_head == NULL) {
      /* this is the first entry in linked list */
      LWIP_ASSERT("igmp_lookup_group: first group must be allsystems",
                  (ip4_addr_cmp(addr, &allsystems) != 0));
      group->next = NULL;
      netif_set_client_data(ifp, LWIP_NETIF_CLIENT_DATA_INDEX_IGMP, group);
    } else {
      /* append _after_ first entry */
      LWIP_ASSERT("igmp_lookup_group: all except first group must not be allsystems",
                  (ip4_addr_cmp(addr, &allsystems) == 0));
      group->next = list_head->next;
      list_head->next = group;
    }
  }

  LWIP_DEBUGF(IGMP_DEBUG, ("igmp_lookup_group: %sallocated a new group with address ", (group ? "" : "impossible to ")));
  ip4_addr_debug_print(IGMP_DEBUG, addr);
  LWIP_DEBUGF(IGMP_DEBUG, (" on if %p\n", (void *)ifp));

  return group;
}

/**
 * Remove a group from netif's igmp group list, but don't free it yet
 *
 * @param group the group to remove from the netif's igmp group list
 * @return ERR_OK if group was removed from the list, an err_t otherwise
 */
static err_t
igmp_remove_group(struct netif *netif, struct igmp_group *group)
{
  err_t err = ERR_OK;
  struct igmp_group *tmp_group;

  /* Skip the first group in the list, it is always the allsystems group added in igmp_start() */
  for (tmp_group = netif_igmp_data(netif); tmp_group != NULL; tmp_group = tmp_group->next) {
    if (tmp_group->next == group) {
      tmp_group->next = group->next;
      break;
    }
  }
  /* Group not found in netif's igmp group list */
  if (tmp_group == NULL) {
    err = ERR_ARG;
  }

  return err;
}

/**
 * Called from ip_input() if a new IGMP packet is received.
 *
 * @param p received igmp packet, p->payload pointing to the igmp header
 * @param inp network interface on which the packet was received
 * @param dest destination ip address of the igmp packet
 */
void
igmp_input(struct pbuf *p, struct netif *inp, const ip4_addr_t *dest)
{
  struct igmp_msg   *igmp;
  struct igmp_group *group;
  struct igmp_group *groupref;

  IGMP_STATS_INC(igmp.recv);

  /* Note that the length CAN be greater than 8 but only 8 are used - All are included in the checksum */
  if (p->len < IGMP_MINLEN) {
    pbuf_free(p);
    IGMP_STATS_INC(igmp.lenerr);
    LWIP_DEBUGF(IGMP_DEBUG, ("igmp_input: length error\n"));
    return;
  }

  LWIP_DEBUGF(IGMP_DEBUG, ("igmp_input: message from "));
  ip4_addr_debug_print(IGMP_DEBUG, &(ip4_current_header()->src));
  LWIP_DEBUGF(IGMP_DEBUG, (" to address "));
  ip4_addr_debug_print(IGMP_DEBUG, &(ip4_current_header()->dest));
  LWIP_DEBUGF(IGMP_DEBUG, (" on if %p\n", (void *)inp));

  /* Now calculate and check the checksum */
  igmp = (struct igmp_msg *)p->payload;
  if (inet_chksum(igmp, p->len)) {
    pbuf_free(p);
    IGMP_STATS_INC(igmp.chkerr);
    LWIP_DEBUGF(IGMP_DEBUG, ("igmp_input: checksum error\n"));
    return;
  }

  /* Packet is ok so find an existing group */
  group = igmp_lookfor_group(inp, dest); /* use the destination IP address of incoming packet */

  /* If group can be found or create... */
  if (!group) {
    pbuf_free(p);
    IGMP_STATS_INC(igmp.drop);
    LWIP_DEBUGF(IGMP_DEBUG, ("igmp_input: IGMP frame not for us\n"));
    return;
  }

  /* NOW ACT ON THE INCOMING MESSAGE TYPE... */
  switch (igmp->igmp_msgtype) {
    case IGMP_MEMB_QUERY:
      /* IGMP_MEMB_QUERY to the "all systems" address ? */
      if ((ip4_addr_cmp(dest, &allsystems)) && ip4_addr_isany(&igmp->igmp_group_address)) {
        /* THIS IS THE GENERAL QUERY */
        LWIP_DEBUGF(IGMP_DEBUG, ("igmp_input: General IGMP_MEMB_QUERY on \"ALL SYSTEMS\" address (224.0.0.1) [igmp_maxresp=%i]\n", (int)(igmp->igmp_maxresp)));

        if (igmp->igmp_maxresp == 0) {
          IGMP_STATS_INC(igmp.rx_v1);
          LWIP_DEBUGF(IGMP_DEBUG, ("igmp_input: got an all hosts query with time== 0 - this is V1 and not implemented - treat as v2\n"));
          igmp->igmp_maxresp = IGMP_V1_DELAYING_MEMBER_TMR;
        } else {
          IGMP_STATS_INC(igmp.rx_general);
        }

        groupref = netif_igmp_data(inp);

        /* Do not send messages on the all systems group address! */
        /* Skip the first group in the list, it is always the allsystems group added in igmp_start() */
        if (groupref != NULL) {
          groupref = groupref->next;
        }

        while (groupref) {
          igmp_delaying_member(groupref, igmp->igmp_maxresp);
          groupref = groupref->next;
        }
      } else {
        /* IGMP_MEMB_QUERY to a specific group ? */
        if (!ip4_addr_isany(&igmp->igmp_group_address)) {
          LWIP_DEBUGF(IGMP_DEBUG, ("igmp_input: IGMP_MEMB_QUERY to a specific group "));
          ip4_addr_debug_print(IGMP_DEBUG, &igmp->igmp_group_address);
          if (ip4_addr_cmp(dest, &allsystems)) {
            ip4_addr_t groupaddr;
            LWIP_DEBUGF(IGMP_DEBUG, (" using \"ALL SYSTEMS\" address (224.0.0.1) [igmp_maxresp=%i]\n", (int)(igmp->igmp_maxresp)));
            /* we first need to re-look for the group since we used dest last time */
            ip4_addr_copy(groupaddr, igmp->igmp_group_address);
            group = igmp_lookfor_group(inp, &groupaddr);
          } else {
            LWIP_DEBUGF(IGMP_DEBUG, (" with the group address as destination [igmp_maxresp=%i]\n", (int)(igmp->igmp_maxresp)));
          }

          if (group != NULL) {
            IGMP_STATS_INC(igmp.rx_group);
            igmp_delaying_member(group, igmp->igmp_maxresp);
          } else {
            IGMP_STATS_INC(igmp.drop);
          }
        } else {
          IGMP_STATS_INC(igmp.proterr);
        }
      }
      break;
    case IGMP_V2_MEMB_REPORT:
      LWIP_DEBUGF(IGMP_DEBUG, ("igmp_input: IGMP_V2_MEMB_REPORT\n"));
      IGMP_STATS_INC(igmp.rx_report);
      if (group->group_state == IGMP_GROUP_DELAYING_MEMBER) {
        /* This is on a specific group we have already looked up */
        group->timer = 0; /* stopped */
        group->group_state = IGMP_GROUP_IDLE_MEMBER;
        group->last_reporter_flag = 0;
      }
      break;
    default:
      LWIP_DEBUGF(IGMP_DEBUG, ("igmp_input: unexpected msg %d in state %d on group %p on if %p\n",
                               igmp->igmp_msgtype, group->group_state, (void *)&group, (void *)inp));
      IGMP_STATS_INC(igmp.proterr);
      break;
  }

  pbuf_free(p);
  return;
}

/**
 * @ingroup igmp
 * Join a group on one network interface.
 *
 * @param ifaddr ip address of the network interface which should join a new group
 * @param groupaddr the ip address of the group which to join
 * @return ERR_OK if group was joined on the netif(s), an err_t otherwise
 */
err_t
igmp_joingroup(const ip4_addr_t *ifaddr, const ip4_addr_t *groupaddr)
{
  err_t err = ERR_VAL; /* no matching interface */
  struct netif *netif;

  /* make sure it is multicast address */
  LWIP_ERROR("igmp_joingroup: attempt to join non-multicast address", ip4_addr_ismulticast(groupaddr), return ERR_VAL;);
  LWIP_ERROR("igmp_joingroup: attempt to join allsystems address", (!ip4_addr_cmp(groupaddr, &allsystems)), return ERR_VAL;);

  /* loop through netif's */
  NETIF_FOREACH(netif) {
    /* Should we join this interface ? */
    if ((netif->flags & NETIF_FLAG_IGMP) && ((ip4_addr_isany(ifaddr) || ip4_addr_cmp(netif_ip4_addr(netif), ifaddr)))) {
      err = igmp_joingroup_netif(netif, groupaddr);
      if (err != ERR_OK) {
        /* Return an error even if some network interfaces are joined */
        /** @todo undo any other netif already joined */
        return err;
      }
    }
  }

  return err;
}

/**
 * @ingroup igmp
 * Join a group on one network interface.
 *
 * @param netif the network interface which should join a new group
 * @param groupaddr the ip address of the group which to join
 * @return ERR_OK if group was joined on the netif, an err_t otherwise
 */
err_t
igmp_joingroup_netif(struct netif *netif, const ip4_addr_t *groupaddr)
{
  struct igmp_group *group;

  /* make sure it is multicast address */
  LWIP_ERROR("igmp_joingroup_netif: attempt to join non-multicast address", ip4_addr_ismulticast(groupaddr), return ERR_VAL;);
  LWIP_ERROR("igmp_joingroup_netif: attempt to join allsystems address", (!ip4_addr_cmp(groupaddr, &allsystems)), return ERR_VAL;);

  /* make sure it is an igmp-enabled netif */
  LWIP_ERROR("igmp_joingroup_netif: attempt to join on non-IGMP netif", netif->flags & NETIF_FLAG_IGMP, return ERR_VAL;);

  /* find group or create a new one if not found */
  group = igmp_lookup_group(netif, groupaddr);

  if (group != NULL) {
    /* This should create a new group, check the state to make sure */
    if (group->group_state != IGMP_GROUP_NON_MEMBER) {
      LWIP_DEBUGF(IGMP_DEBUG, ("igmp_joingroup_netif: join to group not in state IGMP_GROUP_NON_MEMBER\n"));
    } else {
      /* OK - it was new group */
      LWIP_DEBUGF(IGMP_DEBUG, ("igmp_joingroup_netif: join to new group: "));
      ip4_addr_debug_print(IGMP_DEBUG, groupaddr);
      LWIP_DEBUGF(IGMP_DEBUG, ("\n"));

      /* If first use of the group, allow the group at the MAC level */
      if ((group->use == 0) && (netif->igmp_mac_filter != NULL)) {
        LWIP_DEBUGF(IGMP_DEBUG, ("igmp_joingroup_netif: igmp_mac_filter(ADD "));
        ip4_addr_debug_print(IGMP_DEBUG, groupaddr);
        LWIP_DEBUGF(IGMP_DEBUG, (") on if %p\n", (void *)netif));
        netif->igmp_mac_filter(netif, groupaddr, NETIF_ADD_MAC_FILTER);
      }

      IGMP_STATS_INC(igmp.tx_join);
      igmp_send(netif, group, IGMP_V2_MEMB_REPORT);

      igmp_start_timer(group, IGMP_JOIN_DELAYING_MEMBER_TMR);

      /* Need to work out where this timer comes from */
      group->group_state = IGMP_GROUP_DELAYING_MEMBER;
    }
    /* Increment group use */
    group->use++;
    /* Join on this interface */
    return ERR_OK;
  } else {
    LWIP_DEBUGF(IGMP_DEBUG, ("igmp_joingroup_netif: Not enough memory to join to group\n"));
    return ERR_MEM;
  }
}

/**
 * @ingroup igmp
 * Leave a group on one network interface.
 *
 * @param ifaddr ip address of the network interface which should leave a group
 * @param groupaddr the ip address of the group which to leave
 * @return ERR_OK if group was left on the netif(s), an err_t otherwise
 */
err_t
igmp_leavegroup(const ip4_addr_t *ifaddr, const ip4_addr_t *groupaddr)
{
  err_t err = ERR_VAL; /* no matching interface */
  struct netif *netif;

  /* make sure it is multicast address */
  LWIP_ERROR("igmp_leavegroup: attempt to leave non-multicast address", ip4_addr_ismulticast(groupaddr), return ERR_VAL;);
  LWIP_ERROR("igmp_leavegroup: attempt to leave allsystems address", (!ip4_addr_cmp(groupaddr, &allsystems)), return ERR_VAL;);

  /* loop through netif's */
  NETIF_FOREACH(netif) {
    /* Should we leave this interface ? */
    if ((netif->flags & NETIF_FLAG_IGMP) && ((ip4_addr_isany(ifaddr) || ip4_addr_cmp(netif_ip4_addr(netif), ifaddr)))) {
      err_t res = igmp_leavegroup_netif(netif, groupaddr);
      if (err != ERR_OK) {
        /* Store this result if we have not yet gotten a success */
        err = res;
      }
    }
  }

  return err;
}

/**
 * @ingroup igmp
 * Leave a group on one network interface.
 *
 * @param netif the network interface which should leave a group
 * @param groupaddr the ip address of the group which to leave
 * @return ERR_OK if group was left on the netif, an err_t otherwise
 */
err_t
igmp_leavegroup_netif(struct netif *netif, const ip4_addr_t *groupaddr)
{
  struct igmp_group *group;

  /* make sure it is multicast address */
  LWIP_ERROR("igmp_leavegroup_netif: attempt to leave non-multicast address", ip4_addr_ismulticast(groupaddr), return ERR_VAL;);
  LWIP_ERROR("igmp_leavegroup_netif: attempt to leave allsystems address", (!ip4_addr_cmp(groupaddr, &allsystems)), return ERR_VAL;);

  /* make sure it is an igmp-enabled netif */
  LWIP_ERROR("igmp_leavegroup_netif: attempt to leave on non-IGMP netif", netif->flags & NETIF_FLAG_IGMP, return ERR_VAL;);

  /* find group */
  group = igmp_lookfor_group(netif, groupaddr);

  if (group != NULL) {
    /* Only send a leave if the flag is set according to the state diagram */
    LWIP_DEBUGF(IGMP_DEBUG, ("igmp_leavegroup_netif: Leaving group: "));
    ip4_addr_debug_print(IGMP_DEBUG, groupaddr);
    LWIP_DEBUGF(IGMP_DEBUG, ("\n"));

    /* If there is no other use of the group */
    if (group->use <= 1) {
      /* Remove the group from the list */
      igmp_remove_group(netif, group);

      /* If we are the last reporter for this group */
      if (group->last_reporter_flag) {
        LWIP_DEBUGF(IGMP_DEBUG, ("igmp_leavegroup_netif: sending leaving group\n"));
        IGMP_STATS_INC(igmp.tx_leave);
        igmp_send(netif, group, IGMP_LEAVE_GROUP);
      }

      /* Disable the group at the MAC level */
      if (netif->igmp_mac_filter != NULL) {
        LWIP_DEBUGF(IGMP_DEBUG, ("igmp_leavegroup_netif: igmp_mac_filter(DEL "));
        ip4_addr_debug_print(IGMP_DEBUG, groupaddr);
        LWIP_DEBUGF(IGMP_DEBUG, (") on if %p\n", (void *)netif));
        netif->igmp_mac_filter(netif, groupaddr, NETIF_DEL_MAC_FILTER);
      }

      /* Free group struct */
      memp_free(MEMP_IGMP_GROUP, group);
    } else {
      /* Decrement group use */
      group->use--;
    }
    return ERR_OK;
  } else {
    LWIP_DEBUGF(IGMP_DEBUG, ("igmp_leavegroup_netif: not member of group\n"));
    return ERR_VAL;
  }
}

/**
 * The igmp timer function (both for NO_SYS=1 and =0)
 * Should be called every IGMP_TMR_INTERVAL milliseconds (100 ms is default).
 */
void
igmp_tmr(void)
{
  struct netif *netif;

  NETIF_FOREACH(netif) {
    struct igmp_group *group = netif_igmp_data(netif);

    while (group != NULL) {
      if (group->timer > 0) {
        group->timer--;
        if (group->timer == 0) {
          igmp_timeout(netif, group);
        }
      }
      group = group->next;
    }
  }
}

/**
 * Called if a timeout for one group is reached.
 * Sends a report for this group.
 *
 * @param group an igmp_group for which a timeout is reached
 */
static void
igmp_timeout(struct netif *netif, struct igmp_group *group)
{
  /* If the state is IGMP_GROUP_DELAYING_MEMBER then we send a report for this group
     (unless it is the allsystems group) */
  if ((group->group_state == IGMP_GROUP_DELAYING_MEMBER) &&
      (!(ip4_addr_cmp(&(group->group_address), &allsystems)))) {
    LWIP_DEBUGF(IGMP_DEBUG, ("igmp_timeout: report membership for group with address "));
    ip4_addr_debug_print(IGMP_DEBUG, &(group->group_address));
    LWIP_DEBUGF(IGMP_DEBUG, (" on if %p\n", (void *)netif));

    group->group_state = IGMP_GROUP_IDLE_MEMBER;

    IGMP_STATS_INC(igmp.tx_report);
    igmp_send(netif, group, IGMP_V2_MEMB_REPORT);
  }
}

/**
 * Start a timer for an igmp group
 *
 * @param group the igmp_group for which to start a timer
 * @param max_time the time in multiples of IGMP_TMR_INTERVAL (decrease with
 *        every call to igmp_tmr())
 */
static void
igmp_start_timer(struct igmp_group *group, u8_t max_time)
{
#ifdef LWIP_RAND
  group->timer = (u16_t)(max_time > 2 ? (LWIP_RAND() % max_time) : 1);
#else /* LWIP_RAND */
  /* ATTENTION: use this only if absolutely necessary! */
  group->timer = max_time / 2;
#endif /* LWIP_RAND */

  if (group->timer == 0) {
    group->timer = 1;
  }
}

/**
 * Delaying membership report for a group if necessary
 *
 * @param group the igmp_group for which "delaying" membership report
 * @param maxresp query delay
 */
static void
igmp_delaying_member(struct igmp_group *group, u8_t maxresp)
{
  if ((group->group_state == IGMP_GROUP_IDLE_MEMBER) ||
      ((group->group_state == IGMP_GROUP_DELAYING_MEMBER) &&
       ((group->timer == 0) || (maxresp < group->timer)))) {
    igmp_start_timer(group, maxresp);
    group->group_state = IGMP_GROUP_DELAYING_MEMBER;
  }
}


/**
 * Sends an IP packet on a network interface. This function constructs the IP header
 * and calculates the IP header checksum. If the source IP address is NULL,
 * the IP address of the outgoing network interface is filled in as source address.
 *
 * @param p the packet to send (p->payload points to the data, e.g. next
            protocol header; if dest == LWIP_IP_HDRINCL, p already includes an
            IP header and p->payload points to that IP header)
 * @param src the source IP address to send from (if src == IP4_ADDR_ANY, the
 *         IP  address of the netif used to send is used as source address)
 * @param dest the destination IP address to send the packet to
 * @param netif the netif on which to send this packet
 * @return ERR_OK if the packet was sent OK
 *         ERR_BUF if p doesn't have enough space for IP/LINK headers
 *         returns errors returned by netif->output
 */
static err_t
igmp_ip_output_if(struct pbuf *p, const ip4_addr_t *src, const ip4_addr_t *dest, struct netif *netif)
{
  /* This is the "router alert" option */
  u16_t ra[2];
  ra[0] = PP_HTONS(ROUTER_ALERT);
  ra[1] = 0x0000; /* Router shall examine packet */
  IGMP_STATS_INC(igmp.xmit);
  return ip4_output_if_opt(p, src, dest, IGMP_TTL, 0, IP_PROTO_IGMP, netif, ra, ROUTER_ALERTLEN);
}

/**
 * Send an igmp packet to a specific group.
 *
 * @param group the group to which to send the packet
 * @param type the type of igmp packet to send
 */
static void
igmp_send(struct netif *netif, struct igmp_group *group, u8_t type)
{
  struct pbuf     *p    = NULL;
  struct igmp_msg *igmp = NULL;
  ip4_addr_t   src  = *IP4_ADDR_ANY4;
  ip4_addr_t  *dest = NULL;

  /* IP header + "router alert" option + IGMP header */
  p = pbuf_alloc(PBUF_TRANSPORT, IGMP_MINLEN, PBUF_RAM);

  if (p) {
    igmp = (struct igmp_msg *)p->payload;
    LWIP_ASSERT("igmp_send: check that first pbuf can hold struct igmp_msg",
                (p->len >= sizeof(struct igmp_msg)));
    ip4_addr_copy(src, *netif_ip4_addr(netif));

    if (type == IGMP_V2_MEMB_REPORT) {
      dest = &(group->group_address);
      ip4_addr_copy(igmp->igmp_group_address, group->group_address);
      group->last_reporter_flag = 1; /* Remember we were the last to report */
    } else {
      if (type == IGMP_LEAVE_GROUP) {
        dest = &allrouters;
        ip4_addr_copy(igmp->igmp_group_address, group->group_address);
      }
    }

    if ((type == IGMP_V2_MEMB_REPORT) || (type == IGMP_LEAVE_GROUP)) {
      igmp->igmp_msgtype  = type;
      igmp->igmp_maxresp  = 0;
      igmp->igmp_checksum = 0;
      igmp->igmp_checksum = inet_chksum(igmp, IGMP_MINLEN);

      igmp_ip_output_if(p, &src, dest, netif);
    }

    pbuf_free(p);
  } else {
    LWIP_DEBUGF(IGMP_DEBUG, ("igmp_send: not enough memory for igmp_send\n"));
    IGMP_STATS_INC(igmp.memerr);
  }
}

#endif /* LWIP_IPV4 && LWIP_IGMP */
