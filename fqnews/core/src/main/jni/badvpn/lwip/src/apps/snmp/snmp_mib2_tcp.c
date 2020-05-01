/**
 * @file
 * Management Information Base II (RFC1213) TCP objects and functions.
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
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/stats.h"

#include <string.h>

#if LWIP_SNMP && SNMP_LWIP_MIB2 && LWIP_TCP

#if SNMP_USE_NETCONN
#define SYNC_NODE_NAME(node_name) node_name ## _synced
#define CREATE_LWIP_SYNC_NODE(oid, node_name) \
   static const struct snmp_threadsync_node node_name ## _synced = SNMP_CREATE_THREAD_SYNC_NODE(oid, &node_name.node, &snmp_mib2_lwip_locks);
#else
#define SYNC_NODE_NAME(node_name) node_name
#define CREATE_LWIP_SYNC_NODE(oid, node_name)
#endif

/* --- tcp .1.3.6.1.2.1.6 ----------------------------------------------------- */

static s16_t
tcp_get_value(struct snmp_node_instance *instance, void *value)
{
  u32_t *uint_ptr = (u32_t *)value;
  s32_t *sint_ptr = (s32_t *)value;

  switch (instance->node->oid) {
    case 1: /* tcpRtoAlgorithm, vanj(4) */
      *sint_ptr = 4;
      return sizeof(*sint_ptr);
    case 2: /* tcpRtoMin */
      /* @todo not the actual value, a guess,
          needs to be calculated */
      *sint_ptr = 1000;
      return sizeof(*sint_ptr);
    case 3: /* tcpRtoMax */
      /* @todo not the actual value, a guess,
          needs to be calculated */
      *sint_ptr = 60000;
      return sizeof(*sint_ptr);
    case 4: /* tcpMaxConn */
      *sint_ptr = MEMP_NUM_TCP_PCB;
      return sizeof(*sint_ptr);
    case 5: /* tcpActiveOpens */
      *uint_ptr = STATS_GET(mib2.tcpactiveopens);
      return sizeof(*uint_ptr);
    case 6: /* tcpPassiveOpens */
      *uint_ptr = STATS_GET(mib2.tcppassiveopens);
      return sizeof(*uint_ptr);
    case 7: /* tcpAttemptFails */
      *uint_ptr = STATS_GET(mib2.tcpattemptfails);
      return sizeof(*uint_ptr);
    case 8: /* tcpEstabResets */
      *uint_ptr = STATS_GET(mib2.tcpestabresets);
      return sizeof(*uint_ptr);
    case 9: { /* tcpCurrEstab */
      u16_t tcpcurrestab = 0;
      struct tcp_pcb *pcb = tcp_active_pcbs;
      while (pcb != NULL) {
        if ((pcb->state == ESTABLISHED) ||
            (pcb->state == CLOSE_WAIT)) {
          tcpcurrestab++;
        }
        pcb = pcb->next;
      }
      *uint_ptr = tcpcurrestab;
    }
    return sizeof(*uint_ptr);
    case 10: /* tcpInSegs */
      *uint_ptr = STATS_GET(mib2.tcpinsegs);
      return sizeof(*uint_ptr);
    case 11: /* tcpOutSegs */
      *uint_ptr = STATS_GET(mib2.tcpoutsegs);
      return sizeof(*uint_ptr);
    case 12: /* tcpRetransSegs */
      *uint_ptr = STATS_GET(mib2.tcpretranssegs);
      return sizeof(*uint_ptr);
    case 14: /* tcpInErrs */
      *uint_ptr = STATS_GET(mib2.tcpinerrs);
      return sizeof(*uint_ptr);
    case 15: /* tcpOutRsts */
      *uint_ptr = STATS_GET(mib2.tcpoutrsts);
      return sizeof(*uint_ptr);
#if LWIP_HAVE_INT64
    case 17: { /* tcpHCInSegs */
      /* use the 32 bit counter for now... */
      u64_t val64 = STATS_GET(mib2.tcpinsegs);
      *((u64_t *)value) = val64;
    }
    return sizeof(u64_t);
    case 18: { /* tcpHCOutSegs */
      /* use the 32 bit counter for now... */
      u64_t val64 = STATS_GET(mib2.tcpoutsegs);
      *((u64_t *)value) = val64;
    }
    return sizeof(u64_t);
#endif
    default:
      LWIP_DEBUGF(SNMP_MIB_DEBUG, ("tcp_get_value(): unknown id: %"S32_F"\n", instance->node->oid));
      break;
  }

  return 0;
}

/* --- tcpConnTable --- */

#if LWIP_IPV4

/* list of allowed value ranges for incoming OID */
static const struct snmp_oid_range tcp_ConnTable_oid_ranges[] = {
  { 0, 0xff   }, /* IP A */
  { 0, 0xff   }, /* IP B */
  { 0, 0xff   }, /* IP C */
  { 0, 0xff   }, /* IP D */
  { 0, 0xffff }, /* Port */
  { 0, 0xff   }, /* IP A */
  { 0, 0xff   }, /* IP B */
  { 0, 0xff   }, /* IP C */
  { 0, 0xff   }, /* IP D */
  { 0, 0xffff }  /* Port */
};

static snmp_err_t
tcp_ConnTable_get_cell_value_core(struct tcp_pcb *pcb, const u32_t *column, union snmp_variant_value *value, u32_t *value_len)
{
  LWIP_UNUSED_ARG(value_len);

  /* value */
  switch (*column) {
    case 1: /* tcpConnState */
      value->u32 = pcb->state + 1;
      break;
    case 2: /* tcpConnLocalAddress */
      value->u32 = ip_2_ip4(&pcb->local_ip)->addr;
      break;
    case 3: /* tcpConnLocalPort */
      value->u32 = pcb->local_port;
      break;
    case 4: /* tcpConnRemAddress */
      if (pcb->state == LISTEN) {
        value->u32 = IP4_ADDR_ANY4->addr;
      } else {
        value->u32 = ip_2_ip4(&pcb->remote_ip)->addr;
      }
      break;
    case 5: /* tcpConnRemPort */
      if (pcb->state == LISTEN) {
        value->u32 = 0;
      } else {
        value->u32 = pcb->remote_port;
      }
      break;
    default:
      LWIP_ASSERT("invalid id", 0);
      return SNMP_ERR_NOSUCHINSTANCE;
  }

  return SNMP_ERR_NOERROR;
}

static snmp_err_t
tcp_ConnTable_get_cell_value(const u32_t *column, const u32_t *row_oid, u8_t row_oid_len, union snmp_variant_value *value, u32_t *value_len)
{
  u8_t i;
  ip4_addr_t local_ip;
  ip4_addr_t remote_ip;
  u16_t local_port;
  u16_t remote_port;
  struct tcp_pcb *pcb;

  /* check if incoming OID length and if values are in plausible range */
  if (!snmp_oid_in_range(row_oid, row_oid_len, tcp_ConnTable_oid_ranges, LWIP_ARRAYSIZE(tcp_ConnTable_oid_ranges))) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  /* get IPs and ports from incoming OID */
  snmp_oid_to_ip4(&row_oid[0], &local_ip); /* we know it succeeds because of oid_in_range check above */
  local_port = (u16_t)row_oid[4];
  snmp_oid_to_ip4(&row_oid[5], &remote_ip); /* we know it succeeds because of oid_in_range check above */
  remote_port = (u16_t)row_oid[9];

  /* find tcp_pcb with requested ips and ports */
  for (i = 0; i < LWIP_ARRAYSIZE(tcp_pcb_lists); i++) {
    pcb = *tcp_pcb_lists[i];

    while (pcb != NULL) {
      /* do local IP and local port match? */
      if (IP_IS_V4_VAL(pcb->local_ip) &&
          ip4_addr_cmp(&local_ip, ip_2_ip4(&pcb->local_ip)) && (local_port == pcb->local_port)) {

        /* PCBs in state LISTEN are not connected and have no remote_ip or remote_port */
        if (pcb->state == LISTEN) {
          if (ip4_addr_cmp(&remote_ip, IP4_ADDR_ANY4) && (remote_port == 0)) {
            /* fill in object properties */
            return tcp_ConnTable_get_cell_value_core(pcb, column, value, value_len);
          }
        } else {
          if (IP_IS_V4_VAL(pcb->remote_ip) &&
              ip4_addr_cmp(&remote_ip, ip_2_ip4(&pcb->remote_ip)) && (remote_port == pcb->remote_port)) {
            /* fill in object properties */
            return tcp_ConnTable_get_cell_value_core(pcb, column, value, value_len);
          }
        }
      }

      pcb = pcb->next;
    }
  }

  /* not found */
  return SNMP_ERR_NOSUCHINSTANCE;
}

static snmp_err_t
tcp_ConnTable_get_next_cell_instance_and_value(const u32_t *column, struct snmp_obj_id *row_oid, union snmp_variant_value *value, u32_t *value_len)
{
  u8_t i;
  struct tcp_pcb *pcb;
  struct snmp_next_oid_state state;
  u32_t result_temp[LWIP_ARRAYSIZE(tcp_ConnTable_oid_ranges)];

  /* init struct to search next oid */
  snmp_next_oid_init(&state, row_oid->id, row_oid->len, result_temp, LWIP_ARRAYSIZE(tcp_ConnTable_oid_ranges));

  /* iterate over all possible OIDs to find the next one */
  for (i = 0; i < LWIP_ARRAYSIZE(tcp_pcb_lists); i++) {
    pcb = *tcp_pcb_lists[i];
    while (pcb != NULL) {
      u32_t test_oid[LWIP_ARRAYSIZE(tcp_ConnTable_oid_ranges)];

      if (IP_IS_V4_VAL(pcb->local_ip)) {
        snmp_ip4_to_oid(ip_2_ip4(&pcb->local_ip), &test_oid[0]);
        test_oid[4] = pcb->local_port;

        /* PCBs in state LISTEN are not connected and have no remote_ip or remote_port */
        if (pcb->state == LISTEN) {
          snmp_ip4_to_oid(IP4_ADDR_ANY4, &test_oid[5]);
          test_oid[9] = 0;
        } else {
          if (IP_IS_V6_VAL(pcb->remote_ip)) { /* should never happen */
            continue;
          }
          snmp_ip4_to_oid(ip_2_ip4(&pcb->remote_ip), &test_oid[5]);
          test_oid[9] = pcb->remote_port;
        }

        /* check generated OID: is it a candidate for the next one? */
        snmp_next_oid_check(&state, test_oid, LWIP_ARRAYSIZE(tcp_ConnTable_oid_ranges), pcb);
      }

      pcb = pcb->next;
    }
  }

  /* did we find a next one? */
  if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS) {
    snmp_oid_assign(row_oid, state.next_oid, state.next_oid_len);
    /* fill in object properties */
    return tcp_ConnTable_get_cell_value_core((struct tcp_pcb *)state.reference, column, value, value_len);
  }

  /* not found */
  return SNMP_ERR_NOSUCHINSTANCE;
}

#endif /* LWIP_IPV4 */

/* --- tcpConnectionTable --- */

static snmp_err_t
tcp_ConnectionTable_get_cell_value_core(const u32_t *column, struct tcp_pcb *pcb, union snmp_variant_value *value)
{
  /* all items except tcpConnectionState and tcpConnectionProcess are declared as not-accessible */
  switch (*column) {
    case 7: /* tcpConnectionState */
      value->u32 = pcb->state + 1;
      break;
    case 8: /* tcpConnectionProcess */
      value->u32 = 0; /* not supported */
      break;
    default:
      return SNMP_ERR_NOSUCHINSTANCE;
  }

  return SNMP_ERR_NOERROR;
}

static snmp_err_t
tcp_ConnectionTable_get_cell_value(const u32_t *column, const u32_t *row_oid, u8_t row_oid_len, union snmp_variant_value *value, u32_t *value_len)
{
  ip_addr_t local_ip, remote_ip;
  u16_t local_port, remote_port;
  struct tcp_pcb *pcb;
  u8_t idx = 0;
  u8_t i;
  struct tcp_pcb **const tcp_pcb_nonlisten_lists[] = {&tcp_bound_pcbs, &tcp_active_pcbs, &tcp_tw_pcbs};

  LWIP_UNUSED_ARG(value_len);

  /* tcpConnectionLocalAddressType + tcpConnectionLocalAddress + tcpConnectionLocalPort */
  idx += snmp_oid_to_ip_port(&row_oid[idx], row_oid_len - idx, &local_ip, &local_port);
  if (idx == 0) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  /* tcpConnectionRemAddressType + tcpConnectionRemAddress + tcpConnectionRemPort */
  idx += snmp_oid_to_ip_port(&row_oid[idx], row_oid_len - idx, &remote_ip, &remote_port);
  if (idx == 0) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  /* find tcp_pcb with requested ip and port*/
  for (i = 0; i < LWIP_ARRAYSIZE(tcp_pcb_nonlisten_lists); i++) {
    pcb = *tcp_pcb_nonlisten_lists[i];

    while (pcb != NULL) {
      if (ip_addr_cmp(&local_ip, &pcb->local_ip) &&
          (local_port == pcb->local_port) &&
          ip_addr_cmp(&remote_ip, &pcb->remote_ip) &&
          (remote_port == pcb->remote_port)) {
        /* fill in object properties */
        return tcp_ConnectionTable_get_cell_value_core(column, pcb, value);
      }
      pcb = pcb->next;
    }
  }

  /* not found */
  return SNMP_ERR_NOSUCHINSTANCE;
}

static snmp_err_t
tcp_ConnectionTable_get_next_cell_instance_and_value(const u32_t *column, struct snmp_obj_id *row_oid, union snmp_variant_value *value, u32_t *value_len)
{
  struct tcp_pcb *pcb;
  struct snmp_next_oid_state state;
  /* 1x tcpConnectionLocalAddressType + 1x OID len + 16x tcpConnectionLocalAddress  + 1x tcpConnectionLocalPort
   * 1x tcpConnectionRemAddressType   + 1x OID len + 16x tcpConnectionRemAddress    + 1x tcpConnectionRemPort */
  u32_t  result_temp[38];
  u8_t i;
  struct tcp_pcb **const tcp_pcb_nonlisten_lists[] = {&tcp_bound_pcbs, &tcp_active_pcbs, &tcp_tw_pcbs};

  LWIP_UNUSED_ARG(value_len);

  /* init struct to search next oid */
  snmp_next_oid_init(&state, row_oid->id, row_oid->len, result_temp, LWIP_ARRAYSIZE(result_temp));

  /* iterate over all possible OIDs to find the next one */
  for (i = 0; i < LWIP_ARRAYSIZE(tcp_pcb_nonlisten_lists); i++) {
    pcb = *tcp_pcb_nonlisten_lists[i];

    while (pcb != NULL) {
      u8_t idx = 0;
      u32_t test_oid[LWIP_ARRAYSIZE(result_temp)];

      /* tcpConnectionLocalAddressType + tcpConnectionLocalAddress + tcpConnectionLocalPort */
      idx += snmp_ip_port_to_oid(&pcb->local_ip, pcb->local_port, &test_oid[idx]);

      /* tcpConnectionRemAddressType + tcpConnectionRemAddress + tcpConnectionRemPort */
      idx += snmp_ip_port_to_oid(&pcb->remote_ip, pcb->remote_port, &test_oid[idx]);

      /* check generated OID: is it a candidate for the next one? */
      snmp_next_oid_check(&state, test_oid, idx, pcb);

      pcb = pcb->next;
    }
  }

  /* did we find a next one? */
  if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS) {
    snmp_oid_assign(row_oid, state.next_oid, state.next_oid_len);
    /* fill in object properties */
    return tcp_ConnectionTable_get_cell_value_core(column, (struct tcp_pcb *)state.reference, value);
  } else {
    /* not found */
    return SNMP_ERR_NOSUCHINSTANCE;
  }
}

/* --- tcpListenerTable --- */

static snmp_err_t
tcp_ListenerTable_get_cell_value_core(const u32_t *column, union snmp_variant_value *value)
{
  /* all items except tcpListenerProcess are declared as not-accessible */
  switch (*column) {
    case 4: /* tcpListenerProcess */
      value->u32 = 0; /* not supported */
      break;
    default:
      return SNMP_ERR_NOSUCHINSTANCE;
  }

  return SNMP_ERR_NOERROR;
}

static snmp_err_t
tcp_ListenerTable_get_cell_value(const u32_t *column, const u32_t *row_oid, u8_t row_oid_len, union snmp_variant_value *value, u32_t *value_len)
{
  ip_addr_t local_ip;
  u16_t local_port;
  struct tcp_pcb_listen *pcb;
  u8_t idx = 0;

  LWIP_UNUSED_ARG(value_len);

  /* tcpListenerLocalAddressType + tcpListenerLocalAddress + tcpListenerLocalPort */
  idx += snmp_oid_to_ip_port(&row_oid[idx], row_oid_len - idx, &local_ip, &local_port);
  if (idx == 0) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  /* find tcp_pcb with requested ip and port*/
  pcb = tcp_listen_pcbs.listen_pcbs;
  while (pcb != NULL) {
    if (ip_addr_cmp(&local_ip, &pcb->local_ip) &&
        (local_port == pcb->local_port)) {
      /* fill in object properties */
      return tcp_ListenerTable_get_cell_value_core(column, value);
    }
    pcb = pcb->next;
  }

  /* not found */
  return SNMP_ERR_NOSUCHINSTANCE;
}

static snmp_err_t
tcp_ListenerTable_get_next_cell_instance_and_value(const u32_t *column, struct snmp_obj_id *row_oid, union snmp_variant_value *value, u32_t *value_len)
{
  struct tcp_pcb_listen *pcb;
  struct snmp_next_oid_state state;
  /* 1x tcpListenerLocalAddressType + 1x OID len + 16x tcpListenerLocalAddress  + 1x tcpListenerLocalPort */
  u32_t  result_temp[19];

  LWIP_UNUSED_ARG(value_len);

  /* init struct to search next oid */
  snmp_next_oid_init(&state, row_oid->id, row_oid->len, result_temp, LWIP_ARRAYSIZE(result_temp));

  /* iterate over all possible OIDs to find the next one */
  pcb = tcp_listen_pcbs.listen_pcbs;
  while (pcb != NULL) {
    u8_t idx = 0;
    u32_t test_oid[LWIP_ARRAYSIZE(result_temp)];

    /* tcpListenerLocalAddressType + tcpListenerLocalAddress + tcpListenerLocalPort */
    idx += snmp_ip_port_to_oid(&pcb->local_ip, pcb->local_port, &test_oid[idx]);

    /* check generated OID: is it a candidate for the next one? */
    snmp_next_oid_check(&state, test_oid, idx, NULL);

    pcb = pcb->next;
  }

  /* did we find a next one? */
  if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS) {
    snmp_oid_assign(row_oid, state.next_oid, state.next_oid_len);
    /* fill in object properties */
    return tcp_ListenerTable_get_cell_value_core(column, value);
  } else {
    /* not found */
    return SNMP_ERR_NOSUCHINSTANCE;
  }
}

static const struct snmp_scalar_node tcp_RtoAlgorithm  = SNMP_SCALAR_CREATE_NODE_READONLY(1, SNMP_ASN1_TYPE_INTEGER, tcp_get_value);
static const struct snmp_scalar_node tcp_RtoMin        = SNMP_SCALAR_CREATE_NODE_READONLY(2, SNMP_ASN1_TYPE_INTEGER, tcp_get_value);
static const struct snmp_scalar_node tcp_RtoMax        = SNMP_SCALAR_CREATE_NODE_READONLY(3, SNMP_ASN1_TYPE_INTEGER, tcp_get_value);
static const struct snmp_scalar_node tcp_MaxConn       = SNMP_SCALAR_CREATE_NODE_READONLY(4, SNMP_ASN1_TYPE_INTEGER, tcp_get_value);
static const struct snmp_scalar_node tcp_ActiveOpens   = SNMP_SCALAR_CREATE_NODE_READONLY(5, SNMP_ASN1_TYPE_COUNTER, tcp_get_value);
static const struct snmp_scalar_node tcp_PassiveOpens  = SNMP_SCALAR_CREATE_NODE_READONLY(6, SNMP_ASN1_TYPE_COUNTER, tcp_get_value);
static const struct snmp_scalar_node tcp_AttemptFails  = SNMP_SCALAR_CREATE_NODE_READONLY(7, SNMP_ASN1_TYPE_COUNTER, tcp_get_value);
static const struct snmp_scalar_node tcp_EstabResets   = SNMP_SCALAR_CREATE_NODE_READONLY(8, SNMP_ASN1_TYPE_COUNTER, tcp_get_value);
static const struct snmp_scalar_node tcp_CurrEstab     = SNMP_SCALAR_CREATE_NODE_READONLY(9, SNMP_ASN1_TYPE_GAUGE, tcp_get_value);
static const struct snmp_scalar_node tcp_InSegs        = SNMP_SCALAR_CREATE_NODE_READONLY(10, SNMP_ASN1_TYPE_COUNTER, tcp_get_value);
static const struct snmp_scalar_node tcp_OutSegs       = SNMP_SCALAR_CREATE_NODE_READONLY(11, SNMP_ASN1_TYPE_COUNTER, tcp_get_value);
static const struct snmp_scalar_node tcp_RetransSegs   = SNMP_SCALAR_CREATE_NODE_READONLY(12, SNMP_ASN1_TYPE_COUNTER, tcp_get_value);
static const struct snmp_scalar_node tcp_InErrs        = SNMP_SCALAR_CREATE_NODE_READONLY(14, SNMP_ASN1_TYPE_COUNTER, tcp_get_value);
static const struct snmp_scalar_node tcp_OutRsts       = SNMP_SCALAR_CREATE_NODE_READONLY(15, SNMP_ASN1_TYPE_COUNTER, tcp_get_value);
#if LWIP_HAVE_INT64
static const struct snmp_scalar_node tcp_HCInSegs      = SNMP_SCALAR_CREATE_NODE_READONLY(17, SNMP_ASN1_TYPE_COUNTER64, tcp_get_value);
static const struct snmp_scalar_node tcp_HCOutSegs     = SNMP_SCALAR_CREATE_NODE_READONLY(18, SNMP_ASN1_TYPE_COUNTER64, tcp_get_value);
#endif

#if LWIP_IPV4
static const struct snmp_table_simple_col_def tcp_ConnTable_columns[] = {
  {  1, SNMP_ASN1_TYPE_INTEGER, SNMP_VARIANT_VALUE_TYPE_U32 }, /* tcpConnState */
  {  2, SNMP_ASN1_TYPE_IPADDR,  SNMP_VARIANT_VALUE_TYPE_U32 }, /* tcpConnLocalAddress */
  {  3, SNMP_ASN1_TYPE_INTEGER, SNMP_VARIANT_VALUE_TYPE_U32 }, /* tcpConnLocalPort */
  {  4, SNMP_ASN1_TYPE_IPADDR,  SNMP_VARIANT_VALUE_TYPE_U32 }, /* tcpConnRemAddress */
  {  5, SNMP_ASN1_TYPE_INTEGER, SNMP_VARIANT_VALUE_TYPE_U32 }  /* tcpConnRemPort */
};

static const struct snmp_table_simple_node tcp_ConnTable = SNMP_TABLE_CREATE_SIMPLE(13, tcp_ConnTable_columns, tcp_ConnTable_get_cell_value, tcp_ConnTable_get_next_cell_instance_and_value);
#endif /* LWIP_IPV4 */

static const struct snmp_table_simple_col_def tcp_ConnectionTable_columns[] = {
  /* all items except tcpConnectionState and tcpConnectionProcess are declared as not-accessible */
  { 7, SNMP_ASN1_TYPE_INTEGER,    SNMP_VARIANT_VALUE_TYPE_U32 }, /* tcpConnectionState */
  { 8, SNMP_ASN1_TYPE_UNSIGNED32, SNMP_VARIANT_VALUE_TYPE_U32 }  /* tcpConnectionProcess */
};

static const struct snmp_table_simple_node tcp_ConnectionTable = SNMP_TABLE_CREATE_SIMPLE(19, tcp_ConnectionTable_columns, tcp_ConnectionTable_get_cell_value, tcp_ConnectionTable_get_next_cell_instance_and_value);


static const struct snmp_table_simple_col_def tcp_ListenerTable_columns[] = {
  /* all items except tcpListenerProcess are declared as not-accessible */
  { 4, SNMP_ASN1_TYPE_UNSIGNED32, SNMP_VARIANT_VALUE_TYPE_U32 }  /* tcpListenerProcess */
};

static const struct snmp_table_simple_node tcp_ListenerTable = SNMP_TABLE_CREATE_SIMPLE(20, tcp_ListenerTable_columns, tcp_ListenerTable_get_cell_value, tcp_ListenerTable_get_next_cell_instance_and_value);

/* the following nodes access variables in LWIP stack from SNMP worker thread and must therefore be synced to LWIP (TCPIP) thread */
CREATE_LWIP_SYNC_NODE( 1, tcp_RtoAlgorithm)
CREATE_LWIP_SYNC_NODE( 2, tcp_RtoMin)
CREATE_LWIP_SYNC_NODE( 3, tcp_RtoMax)
CREATE_LWIP_SYNC_NODE( 4, tcp_MaxConn)
CREATE_LWIP_SYNC_NODE( 5, tcp_ActiveOpens)
CREATE_LWIP_SYNC_NODE( 6, tcp_PassiveOpens)
CREATE_LWIP_SYNC_NODE( 7, tcp_AttemptFails)
CREATE_LWIP_SYNC_NODE( 8, tcp_EstabResets)
CREATE_LWIP_SYNC_NODE( 9, tcp_CurrEstab)
CREATE_LWIP_SYNC_NODE(10, tcp_InSegs)
CREATE_LWIP_SYNC_NODE(11, tcp_OutSegs)
CREATE_LWIP_SYNC_NODE(12, tcp_RetransSegs)
#if LWIP_IPV4
CREATE_LWIP_SYNC_NODE(13, tcp_ConnTable)
#endif /* LWIP_IPV4 */
CREATE_LWIP_SYNC_NODE(14, tcp_InErrs)
CREATE_LWIP_SYNC_NODE(15, tcp_OutRsts)
#if LWIP_HAVE_INT64
CREATE_LWIP_SYNC_NODE(17, tcp_HCInSegs)
CREATE_LWIP_SYNC_NODE(18, tcp_HCOutSegs)
#endif
CREATE_LWIP_SYNC_NODE(19, tcp_ConnectionTable)
CREATE_LWIP_SYNC_NODE(20, tcp_ListenerTable)

static const struct snmp_node *const tcp_nodes[] = {
  &SYNC_NODE_NAME(tcp_RtoAlgorithm).node.node,
  &SYNC_NODE_NAME(tcp_RtoMin).node.node,
  &SYNC_NODE_NAME(tcp_RtoMax).node.node,
  &SYNC_NODE_NAME(tcp_MaxConn).node.node,
  &SYNC_NODE_NAME(tcp_ActiveOpens).node.node,
  &SYNC_NODE_NAME(tcp_PassiveOpens).node.node,
  &SYNC_NODE_NAME(tcp_AttemptFails).node.node,
  &SYNC_NODE_NAME(tcp_EstabResets).node.node,
  &SYNC_NODE_NAME(tcp_CurrEstab).node.node,
  &SYNC_NODE_NAME(tcp_InSegs).node.node,
  &SYNC_NODE_NAME(tcp_OutSegs).node.node,
  &SYNC_NODE_NAME(tcp_RetransSegs).node.node,
#if LWIP_IPV4
  &SYNC_NODE_NAME(tcp_ConnTable).node.node,
#endif /* LWIP_IPV4 */
  &SYNC_NODE_NAME(tcp_InErrs).node.node,
  &SYNC_NODE_NAME(tcp_OutRsts).node.node,
  &SYNC_NODE_NAME(tcp_HCInSegs).node.node,
#if LWIP_HAVE_INT64
  &SYNC_NODE_NAME(tcp_HCOutSegs).node.node,
  &SYNC_NODE_NAME(tcp_ConnectionTable).node.node,
#endif
  &SYNC_NODE_NAME(tcp_ListenerTable).node.node
};

const struct snmp_tree_node snmp_mib2_tcp_root = SNMP_CREATE_TREE_NODE(6, tcp_nodes);
#endif /* LWIP_SNMP && SNMP_LWIP_MIB2 && LWIP_TCP */
