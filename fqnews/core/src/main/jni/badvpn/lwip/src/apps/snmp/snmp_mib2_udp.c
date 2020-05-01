/**
 * @file
 * Management Information Base II (RFC1213) UDP objects and functions.
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
#include "lwip/udp.h"
#include "lwip/stats.h"

#include <string.h>

#if LWIP_SNMP && SNMP_LWIP_MIB2 && LWIP_UDP

#if SNMP_USE_NETCONN
#define SYNC_NODE_NAME(node_name) node_name ## _synced
#define CREATE_LWIP_SYNC_NODE(oid, node_name) \
   static const struct snmp_threadsync_node node_name ## _synced = SNMP_CREATE_THREAD_SYNC_NODE(oid, &node_name.node, &snmp_mib2_lwip_locks);
#else
#define SYNC_NODE_NAME(node_name) node_name
#define CREATE_LWIP_SYNC_NODE(oid, node_name)
#endif

/* --- udp .1.3.6.1.2.1.7 ----------------------------------------------------- */

static s16_t
udp_get_value(struct snmp_node_instance *instance, void *value)
{
  u32_t *uint_ptr = (u32_t *)value;

  switch (instance->node->oid) {
    case 1: /* udpInDatagrams */
      *uint_ptr = STATS_GET(mib2.udpindatagrams);
      return sizeof(*uint_ptr);
    case 2: /* udpNoPorts */
      *uint_ptr = STATS_GET(mib2.udpnoports);
      return sizeof(*uint_ptr);
    case 3: /* udpInErrors */
      *uint_ptr = STATS_GET(mib2.udpinerrors);
      return sizeof(*uint_ptr);
    case 4: /* udpOutDatagrams */
      *uint_ptr = STATS_GET(mib2.udpoutdatagrams);
      return sizeof(*uint_ptr);
#if LWIP_HAVE_INT64
    case 8: { /* udpHCInDatagrams */
      /* use the 32 bit counter for now... */
      u64_t val64 = STATS_GET(mib2.udpindatagrams);
      *((u64_t *)value) = val64;
    }
    return sizeof(u64_t);
    case 9: { /* udpHCOutDatagrams */
      /* use the 32 bit counter for now... */
      u64_t val64 = STATS_GET(mib2.udpoutdatagrams);
      *((u64_t *)value) = val64;
    }
    return sizeof(u64_t);
#endif
    default:
      LWIP_DEBUGF(SNMP_MIB_DEBUG, ("udp_get_value(): unknown id: %"S32_F"\n", instance->node->oid));
      break;
  }

  return 0;
}

/* --- udpEndpointTable --- */

static snmp_err_t
udp_endpointTable_get_cell_value_core(const u32_t *column, union snmp_variant_value *value)
{
  /* all items except udpEndpointProcess are declared as not-accessible */
  switch (*column) {
    case 8: /* udpEndpointProcess */
      value->u32 = 0; /* not supported */
      break;
    default:
      return SNMP_ERR_NOSUCHINSTANCE;
  }

  return SNMP_ERR_NOERROR;
}

static snmp_err_t
udp_endpointTable_get_cell_value(const u32_t *column, const u32_t *row_oid, u8_t row_oid_len, union snmp_variant_value *value, u32_t *value_len)
{
  ip_addr_t local_ip, remote_ip;
  u16_t local_port, remote_port;
  struct udp_pcb *pcb;
  u8_t idx = 0;

  LWIP_UNUSED_ARG(value_len);

  /* udpEndpointLocalAddressType + udpEndpointLocalAddress + udpEndpointLocalPort */
  idx += snmp_oid_to_ip_port(&row_oid[idx], row_oid_len - idx, &local_ip, &local_port);
  if (idx == 0) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  /* udpEndpointRemoteAddressType + udpEndpointRemoteAddress + udpEndpointRemotePort */
  idx += snmp_oid_to_ip_port(&row_oid[idx], row_oid_len - idx, &remote_ip, &remote_port);
  if (idx == 0) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  /* udpEndpointInstance */
  if (row_oid_len < (idx + 1)) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }
  if (row_oid[idx] != 0) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  /* find udp_pcb with requested ip and port*/
  pcb = udp_pcbs;
  while (pcb != NULL) {
    if (ip_addr_cmp(&local_ip, &pcb->local_ip) &&
        (local_port == pcb->local_port) &&
        ip_addr_cmp(&remote_ip, &pcb->remote_ip) &&
        (remote_port == pcb->remote_port)) {
      /* fill in object properties */
      return udp_endpointTable_get_cell_value_core(column, value);
    }
    pcb = pcb->next;
  }

  /* not found */
  return SNMP_ERR_NOSUCHINSTANCE;
}

static snmp_err_t
udp_endpointTable_get_next_cell_instance_and_value(const u32_t *column, struct snmp_obj_id *row_oid, union snmp_variant_value *value, u32_t *value_len)
{
  struct udp_pcb *pcb;
  struct snmp_next_oid_state state;
  /* 1x udpEndpointLocalAddressType  + 1x OID len + 16x udpEndpointLocalAddress  + 1x udpEndpointLocalPort  +
   * 1x udpEndpointRemoteAddressType + 1x OID len + 16x udpEndpointRemoteAddress + 1x udpEndpointRemotePort +
   * 1x udpEndpointInstance = 39
   */
  u32_t  result_temp[39];

  LWIP_UNUSED_ARG(value_len);

  /* init struct to search next oid */
  snmp_next_oid_init(&state, row_oid->id, row_oid->len, result_temp, LWIP_ARRAYSIZE(result_temp));

  /* iterate over all possible OIDs to find the next one */
  pcb = udp_pcbs;
  while (pcb != NULL) {
    u32_t test_oid[LWIP_ARRAYSIZE(result_temp)];
    u8_t idx = 0;

    /* udpEndpointLocalAddressType + udpEndpointLocalAddress + udpEndpointLocalPort */
    idx += snmp_ip_port_to_oid(&pcb->local_ip, pcb->local_port, &test_oid[idx]);

    /* udpEndpointRemoteAddressType + udpEndpointRemoteAddress + udpEndpointRemotePort */
    idx += snmp_ip_port_to_oid(&pcb->remote_ip, pcb->remote_port, &test_oid[idx]);

    test_oid[idx] = 0; /* udpEndpointInstance */
    idx++;

    /* check generated OID: is it a candidate for the next one? */
    snmp_next_oid_check(&state, test_oid, idx, NULL);

    pcb = pcb->next;
  }

  /* did we find a next one? */
  if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS) {
    snmp_oid_assign(row_oid, state.next_oid, state.next_oid_len);
    /* fill in object properties */
    return udp_endpointTable_get_cell_value_core(column, value);
  } else {
    /* not found */
    return SNMP_ERR_NOSUCHINSTANCE;
  }
}

/* --- udpTable --- */

#if LWIP_IPV4

/* list of allowed value ranges for incoming OID */
static const struct snmp_oid_range udp_Table_oid_ranges[] = {
  { 0, 0xff   }, /* IP A        */
  { 0, 0xff   }, /* IP B        */
  { 0, 0xff   }, /* IP C        */
  { 0, 0xff   }, /* IP D        */
  { 1, 0xffff }  /* Port        */
};

static snmp_err_t
udp_Table_get_cell_value_core(struct udp_pcb *pcb, const u32_t *column, union snmp_variant_value *value, u32_t *value_len)
{
  LWIP_UNUSED_ARG(value_len);

  switch (*column) {
    case 1: /* udpLocalAddress */
      /* set reference to PCB local IP and return a generic node that copies IP4 addresses */
      value->u32 = ip_2_ip4(&pcb->local_ip)->addr;
      break;
    case 2: /* udpLocalPort */
      /* set reference to PCB local port and return a generic node that copies u16_t values */
      value->u32 = pcb->local_port;
      break;
    default:
      return SNMP_ERR_NOSUCHINSTANCE;
  }

  return SNMP_ERR_NOERROR;
}

static snmp_err_t
udp_Table_get_cell_value(const u32_t *column, const u32_t *row_oid, u8_t row_oid_len, union snmp_variant_value *value, u32_t *value_len)
{
  ip4_addr_t ip;
  u16_t port;
  struct udp_pcb *pcb;

  /* check if incoming OID length and if values are in plausible range */
  if (!snmp_oid_in_range(row_oid, row_oid_len, udp_Table_oid_ranges, LWIP_ARRAYSIZE(udp_Table_oid_ranges))) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  /* get IP and port from incoming OID */
  snmp_oid_to_ip4(&row_oid[0], &ip); /* we know it succeeds because of oid_in_range check above */
  port = (u16_t)row_oid[4];

  /* find udp_pcb with requested ip and port*/
  pcb = udp_pcbs;
  while (pcb != NULL) {
    if (IP_IS_V4_VAL(pcb->local_ip)) {
      if (ip4_addr_cmp(&ip, ip_2_ip4(&pcb->local_ip)) && (port == pcb->local_port)) {
        /* fill in object properties */
        return udp_Table_get_cell_value_core(pcb, column, value, value_len);
      }
    }
    pcb = pcb->next;
  }

  /* not found */
  return SNMP_ERR_NOSUCHINSTANCE;
}

static snmp_err_t
udp_Table_get_next_cell_instance_and_value(const u32_t *column, struct snmp_obj_id *row_oid, union snmp_variant_value *value, u32_t *value_len)
{
  struct udp_pcb *pcb;
  struct snmp_next_oid_state state;
  u32_t  result_temp[LWIP_ARRAYSIZE(udp_Table_oid_ranges)];

  /* init struct to search next oid */
  snmp_next_oid_init(&state, row_oid->id, row_oid->len, result_temp, LWIP_ARRAYSIZE(udp_Table_oid_ranges));

  /* iterate over all possible OIDs to find the next one */
  pcb = udp_pcbs;
  while (pcb != NULL) {
    u32_t test_oid[LWIP_ARRAYSIZE(udp_Table_oid_ranges)];

    if (IP_IS_V4_VAL(pcb->local_ip)) {
      snmp_ip4_to_oid(ip_2_ip4(&pcb->local_ip), &test_oid[0]);
      test_oid[4] = pcb->local_port;

      /* check generated OID: is it a candidate for the next one? */
      snmp_next_oid_check(&state, test_oid, LWIP_ARRAYSIZE(udp_Table_oid_ranges), pcb);
    }

    pcb = pcb->next;
  }

  /* did we find a next one? */
  if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS) {
    snmp_oid_assign(row_oid, state.next_oid, state.next_oid_len);
    /* fill in object properties */
    return udp_Table_get_cell_value_core((struct udp_pcb *)state.reference, column, value, value_len);
  } else {
    /* not found */
    return SNMP_ERR_NOSUCHINSTANCE;
  }
}

#endif /* LWIP_IPV4 */

static const struct snmp_scalar_node udp_inDatagrams    = SNMP_SCALAR_CREATE_NODE_READONLY(1, SNMP_ASN1_TYPE_COUNTER,   udp_get_value);
static const struct snmp_scalar_node udp_noPorts        = SNMP_SCALAR_CREATE_NODE_READONLY(2, SNMP_ASN1_TYPE_COUNTER,   udp_get_value);
static const struct snmp_scalar_node udp_inErrors       = SNMP_SCALAR_CREATE_NODE_READONLY(3, SNMP_ASN1_TYPE_COUNTER,   udp_get_value);
static const struct snmp_scalar_node udp_outDatagrams   = SNMP_SCALAR_CREATE_NODE_READONLY(4, SNMP_ASN1_TYPE_COUNTER,   udp_get_value);
#if LWIP_HAVE_INT64
static const struct snmp_scalar_node udp_HCInDatagrams  = SNMP_SCALAR_CREATE_NODE_READONLY(8, SNMP_ASN1_TYPE_COUNTER64, udp_get_value);
static const struct snmp_scalar_node udp_HCOutDatagrams = SNMP_SCALAR_CREATE_NODE_READONLY(9, SNMP_ASN1_TYPE_COUNTER64, udp_get_value);
#endif

#if LWIP_IPV4
static const struct snmp_table_simple_col_def udp_Table_columns[] = {
  { 1, SNMP_ASN1_TYPE_IPADDR,  SNMP_VARIANT_VALUE_TYPE_U32 }, /* udpLocalAddress */
  { 2, SNMP_ASN1_TYPE_INTEGER, SNMP_VARIANT_VALUE_TYPE_U32 }  /* udpLocalPort */
};
static const struct snmp_table_simple_node udp_Table = SNMP_TABLE_CREATE_SIMPLE(5, udp_Table_columns, udp_Table_get_cell_value, udp_Table_get_next_cell_instance_and_value);
#endif /* LWIP_IPV4 */

static const struct snmp_table_simple_col_def udp_endpointTable_columns[] = {
  /* all items except udpEndpointProcess are declared as not-accessible */
  { 8, SNMP_ASN1_TYPE_UNSIGNED32, SNMP_VARIANT_VALUE_TYPE_U32 }  /* udpEndpointProcess */
};

static const struct snmp_table_simple_node udp_endpointTable = SNMP_TABLE_CREATE_SIMPLE(7, udp_endpointTable_columns, udp_endpointTable_get_cell_value, udp_endpointTable_get_next_cell_instance_and_value);

/* the following nodes access variables in LWIP stack from SNMP worker thread and must therefore be synced to LWIP (TCPIP) thread */
CREATE_LWIP_SYNC_NODE(1, udp_inDatagrams)
CREATE_LWIP_SYNC_NODE(2, udp_noPorts)
CREATE_LWIP_SYNC_NODE(3, udp_inErrors)
CREATE_LWIP_SYNC_NODE(4, udp_outDatagrams)
#if LWIP_IPV4
CREATE_LWIP_SYNC_NODE(5, udp_Table)
#endif /* LWIP_IPV4 */
CREATE_LWIP_SYNC_NODE(7, udp_endpointTable)
#if LWIP_HAVE_INT64
CREATE_LWIP_SYNC_NODE(8, udp_HCInDatagrams)
CREATE_LWIP_SYNC_NODE(9, udp_HCOutDatagrams)
#endif

static const struct snmp_node *const udp_nodes[] = {
  &SYNC_NODE_NAME(udp_inDatagrams).node.node,
  &SYNC_NODE_NAME(udp_noPorts).node.node,
  &SYNC_NODE_NAME(udp_inErrors).node.node,
  &SYNC_NODE_NAME(udp_outDatagrams).node.node,
#if LWIP_IPV4
  &SYNC_NODE_NAME(udp_Table).node.node,
#endif /* LWIP_IPV4 */
  &SYNC_NODE_NAME(udp_endpointTable).node.node
#if LWIP_HAVE_INT64
  ,
  &SYNC_NODE_NAME(udp_HCInDatagrams).node.node,
  &SYNC_NODE_NAME(udp_HCOutDatagrams).node.node
#endif
};

const struct snmp_tree_node snmp_mib2_udp_root = SNMP_CREATE_TREE_NODE(7, udp_nodes);
#endif /* LWIP_SNMP && SNMP_LWIP_MIB2 && LWIP_UDP */
