/**
 * @file
 * Management Information Base II (RFC1213) INTERFACES objects and functions.
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

#include "lwip/snmp.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_core.h"
#include "lwip/apps/snmp_mib2.h"
#include "lwip/apps/snmp_table.h"
#include "lwip/apps/snmp_scalar.h"
#include "lwip/netif.h"
#include "lwip/stats.h"

#include <string.h>

#if LWIP_SNMP && SNMP_LWIP_MIB2

#if SNMP_USE_NETCONN
#define SYNC_NODE_NAME(node_name) node_name ## _synced
#define CREATE_LWIP_SYNC_NODE(oid, node_name) \
   static const struct snmp_threadsync_node node_name ## _synced = SNMP_CREATE_THREAD_SYNC_NODE(oid, &node_name.node, &snmp_mib2_lwip_locks);
#else
#define SYNC_NODE_NAME(node_name) node_name
#define CREATE_LWIP_SYNC_NODE(oid, node_name)
#endif


/* --- interfaces .1.3.6.1.2.1.2 ----------------------------------------------------- */

static s16_t
interfaces_get_value(struct snmp_node_instance *instance, void *value)
{
  if (instance->node->oid == 1) {
    s32_t *sint_ptr = (s32_t *)value;
    s32_t num_netifs = 0;

    struct netif *netif;
    NETIF_FOREACH(netif) {
      num_netifs++;
    }

    *sint_ptr = num_netifs;
    return sizeof(*sint_ptr);
  }

  return 0;
}

/* list of allowed value ranges for incoming OID */
static const struct snmp_oid_range interfaces_Table_oid_ranges[] = {
  { 1, 0xff } /* netif->num is u8_t */
};

static const u8_t iftable_ifOutQLen         = 0;

static const u8_t iftable_ifOperStatus_up   = 1;
static const u8_t iftable_ifOperStatus_down = 2;

static const u8_t iftable_ifAdminStatus_up             = 1;
static const u8_t iftable_ifAdminStatus_lowerLayerDown = 7;
static const u8_t iftable_ifAdminStatus_down           = 2;

static snmp_err_t
interfaces_Table_get_cell_instance(const u32_t *column, const u32_t *row_oid, u8_t row_oid_len, struct snmp_node_instance *cell_instance)
{
  u32_t ifIndex;
  struct netif *netif;

  LWIP_UNUSED_ARG(column);

  /* check if incoming OID length and if values are in plausible range */
  if (!snmp_oid_in_range(row_oid, row_oid_len, interfaces_Table_oid_ranges, LWIP_ARRAYSIZE(interfaces_Table_oid_ranges))) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  /* get netif index from incoming OID */
  ifIndex = row_oid[0];

  /* find netif with index */
  NETIF_FOREACH(netif) {
    if (netif_to_num(netif) == ifIndex) {
      /* store netif pointer for subsequent operations (get/test/set) */
      cell_instance->reference.ptr = netif;
      return SNMP_ERR_NOERROR;
    }
  }

  /* not found */
  return SNMP_ERR_NOSUCHINSTANCE;
}

static snmp_err_t
interfaces_Table_get_next_cell_instance(const u32_t *column, struct snmp_obj_id *row_oid, struct snmp_node_instance *cell_instance)
{
  struct netif *netif;
  struct snmp_next_oid_state state;
  u32_t result_temp[LWIP_ARRAYSIZE(interfaces_Table_oid_ranges)];

  LWIP_UNUSED_ARG(column);

  /* init struct to search next oid */
  snmp_next_oid_init(&state, row_oid->id, row_oid->len, result_temp, LWIP_ARRAYSIZE(interfaces_Table_oid_ranges));

  /* iterate over all possible OIDs to find the next one */
  NETIF_FOREACH(netif) {
    u32_t test_oid[LWIP_ARRAYSIZE(interfaces_Table_oid_ranges)];
    test_oid[0] = netif_to_num(netif);

    /* check generated OID: is it a candidate for the next one? */
    snmp_next_oid_check(&state, test_oid, LWIP_ARRAYSIZE(interfaces_Table_oid_ranges), netif);
  }

  /* did we find a next one? */
  if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS) {
    snmp_oid_assign(row_oid, state.next_oid, state.next_oid_len);
    /* store netif pointer for subsequent operations (get/test/set) */
    cell_instance->reference.ptr = /* (struct netif*) */state.reference;
    return SNMP_ERR_NOERROR;
  }

  /* not found */
  return SNMP_ERR_NOSUCHINSTANCE;
}

static s16_t
interfaces_Table_get_value(struct snmp_node_instance *instance, void *value)
{
  struct netif *netif = (struct netif *)instance->reference.ptr;
  u32_t *value_u32 = (u32_t *)value;
  s32_t *value_s32 = (s32_t *)value;
  u16_t value_len;

  switch (SNMP_TABLE_GET_COLUMN_FROM_OID(instance->instance_oid.id)) {
    case 1: /* ifIndex */
      *value_s32 = netif_to_num(netif);
      value_len = sizeof(*value_s32);
      break;
    case 2: /* ifDescr */
      value_len = sizeof(netif->name);
      MEMCPY(value, netif->name, value_len);
      break;
    case 3: /* ifType */
      *value_s32 = netif->link_type;
      value_len = sizeof(*value_s32);
      break;
    case 4: /* ifMtu */
      *value_s32 = netif->mtu;
      value_len = sizeof(*value_s32);
      break;
    case 5: /* ifSpeed */
      *value_u32 = netif->link_speed;
      value_len = sizeof(*value_u32);
      break;
    case 6: /* ifPhysAddress */
      value_len = sizeof(netif->hwaddr);
      MEMCPY(value, &netif->hwaddr, value_len);
      break;
    case 7: /* ifAdminStatus */
      if (netif_is_up(netif)) {
        *value_s32 = iftable_ifOperStatus_up;
      } else {
        *value_s32 = iftable_ifOperStatus_down;
      }
      value_len = sizeof(*value_s32);
      break;
    case 8: /* ifOperStatus */
      if (netif_is_up(netif)) {
        if (netif_is_link_up(netif)) {
          *value_s32 = iftable_ifAdminStatus_up;
        } else {
          *value_s32 = iftable_ifAdminStatus_lowerLayerDown;
        }
      } else {
        *value_s32 = iftable_ifAdminStatus_down;
      }
      value_len = sizeof(*value_s32);
      break;
    case 9: /* ifLastChange */
      *value_u32 = netif->ts;
      value_len = sizeof(*value_u32);
      break;
    case 10: /* ifInOctets */
      *value_u32 = netif->mib2_counters.ifinoctets;
      value_len = sizeof(*value_u32);
      break;
    case 11: /* ifInUcastPkts */
      *value_u32 = netif->mib2_counters.ifinucastpkts;
      value_len = sizeof(*value_u32);
      break;
    case 12: /* ifInNUcastPkts */
      *value_u32 = netif->mib2_counters.ifinnucastpkts;
      value_len = sizeof(*value_u32);
      break;
    case 13: /* ifInDiscards */
      *value_u32 = netif->mib2_counters.ifindiscards;
      value_len = sizeof(*value_u32);
      break;
    case 14: /* ifInErrors */
      *value_u32 = netif->mib2_counters.ifinerrors;
      value_len = sizeof(*value_u32);
      break;
    case 15: /* ifInUnkownProtos */
      *value_u32 = netif->mib2_counters.ifinunknownprotos;
      value_len = sizeof(*value_u32);
      break;
    case 16: /* ifOutOctets */
      *value_u32 = netif->mib2_counters.ifoutoctets;
      value_len = sizeof(*value_u32);
      break;
    case 17: /* ifOutUcastPkts */
      *value_u32 = netif->mib2_counters.ifoutucastpkts;
      value_len = sizeof(*value_u32);
      break;
    case 18: /* ifOutNUcastPkts */
      *value_u32 = netif->mib2_counters.ifoutnucastpkts;
      value_len = sizeof(*value_u32);
      break;
    case 19: /* ifOutDiscarts */
      *value_u32 = netif->mib2_counters.ifoutdiscards;
      value_len = sizeof(*value_u32);
      break;
    case 20: /* ifOutErrors */
      *value_u32 = netif->mib2_counters.ifouterrors;
      value_len = sizeof(*value_u32);
      break;
    case 21: /* ifOutQLen */
      *value_u32 = iftable_ifOutQLen;
      value_len = sizeof(*value_u32);
      break;
    /** @note returning zeroDotZero (0.0) no media specific MIB support */
    case 22: /* ifSpecific */
      value_len = snmp_zero_dot_zero.len * sizeof(u32_t);
      MEMCPY(value, snmp_zero_dot_zero.id, value_len);
      break;
    default:
      return 0;
  }

  return value_len;
}

#if !SNMP_SAFE_REQUESTS

static snmp_err_t
interfaces_Table_set_test(struct snmp_node_instance *instance, u16_t len, void *value)
{
  s32_t *sint_ptr = (s32_t *)value;

  /* stack should never call this method for another column,
  because all other columns are set to readonly */
  LWIP_ASSERT("Invalid column", (SNMP_TABLE_GET_COLUMN_FROM_OID(instance->instance_oid.id) == 7));
  LWIP_UNUSED_ARG(len);

  if (*sint_ptr == 1 || *sint_ptr == 2) {
    return SNMP_ERR_NOERROR;
  }

  return SNMP_ERR_WRONGVALUE;
}

static snmp_err_t
interfaces_Table_set_value(struct snmp_node_instance *instance, u16_t len, void *value)
{
  struct netif *netif = (struct netif *)instance->reference.ptr;
  s32_t *sint_ptr = (s32_t *)value;

  /* stack should never call this method for another column,
  because all other columns are set to readonly */
  LWIP_ASSERT("Invalid column", (SNMP_TABLE_GET_COLUMN_FROM_OID(instance->instance_oid.id) == 7));
  LWIP_UNUSED_ARG(len);

  if (*sint_ptr == 1) {
    netif_set_up(netif);
  } else if (*sint_ptr == 2) {
    netif_set_down(netif);
  }

  return SNMP_ERR_NOERROR;
}

#endif /* SNMP_SAFE_REQUESTS */

static const struct snmp_scalar_node interfaces_Number = SNMP_SCALAR_CREATE_NODE_READONLY(1, SNMP_ASN1_TYPE_INTEGER, interfaces_get_value);

static const struct snmp_table_col_def interfaces_Table_columns[] = {
  {  1, SNMP_ASN1_TYPE_INTEGER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifIndex */
  {  2, SNMP_ASN1_TYPE_OCTET_STRING, SNMP_NODE_INSTANCE_READ_ONLY }, /* ifDescr */
  {  3, SNMP_ASN1_TYPE_INTEGER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifType */
  {  4, SNMP_ASN1_TYPE_INTEGER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifMtu */
  {  5, SNMP_ASN1_TYPE_GAUGE,        SNMP_NODE_INSTANCE_READ_ONLY }, /* ifSpeed */
  {  6, SNMP_ASN1_TYPE_OCTET_STRING, SNMP_NODE_INSTANCE_READ_ONLY }, /* ifPhysAddress */
#if !SNMP_SAFE_REQUESTS
  {  7, SNMP_ASN1_TYPE_INTEGER,      SNMP_NODE_INSTANCE_READ_WRITE }, /* ifAdminStatus */
#else
  {  7, SNMP_ASN1_TYPE_INTEGER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifAdminStatus */
#endif
  {  8, SNMP_ASN1_TYPE_INTEGER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifOperStatus */
  {  9, SNMP_ASN1_TYPE_TIMETICKS,    SNMP_NODE_INSTANCE_READ_ONLY }, /* ifLastChange */
  { 10, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifInOctets */
  { 11, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifInUcastPkts */
  { 12, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifInNUcastPkts */
  { 13, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifInDiscarts */
  { 14, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifInErrors */
  { 15, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifInUnkownProtos */
  { 16, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifOutOctets */
  { 17, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifOutUcastPkts */
  { 18, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifOutNUcastPkts */
  { 19, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifOutDiscarts */
  { 20, SNMP_ASN1_TYPE_COUNTER,      SNMP_NODE_INSTANCE_READ_ONLY }, /* ifOutErrors */
  { 21, SNMP_ASN1_TYPE_GAUGE,        SNMP_NODE_INSTANCE_READ_ONLY }, /* ifOutQLen */
  { 22, SNMP_ASN1_TYPE_OBJECT_ID,    SNMP_NODE_INSTANCE_READ_ONLY }  /* ifSpecific */
};

#if !SNMP_SAFE_REQUESTS
static const struct snmp_table_node interfaces_Table = SNMP_TABLE_CREATE(
      2, interfaces_Table_columns,
      interfaces_Table_get_cell_instance, interfaces_Table_get_next_cell_instance,
      interfaces_Table_get_value, interfaces_Table_set_test, interfaces_Table_set_value);
#else
static const struct snmp_table_node interfaces_Table = SNMP_TABLE_CREATE(
      2, interfaces_Table_columns,
      interfaces_Table_get_cell_instance, interfaces_Table_get_next_cell_instance,
      interfaces_Table_get_value, NULL, NULL);
#endif

/* the following nodes access variables in LWIP stack from SNMP worker thread and must therefore be synced to LWIP (TCPIP) thread */
CREATE_LWIP_SYNC_NODE(1, interfaces_Number)
CREATE_LWIP_SYNC_NODE(2, interfaces_Table)

static const struct snmp_node *const interface_nodes[] = {
  &SYNC_NODE_NAME(interfaces_Number).node.node,
  &SYNC_NODE_NAME(interfaces_Table).node.node
};

const struct snmp_tree_node snmp_mib2_interface_root = SNMP_CREATE_TREE_NODE(2, interface_nodes);

#endif /* LWIP_SNMP && SNMP_LWIP_MIB2 */
