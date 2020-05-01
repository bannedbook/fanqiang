/**
 * @file
 * Management Information Base II (RFC1213) SNMP objects and functions.
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
#include "lwip/apps/snmp_scalar.h"

#if LWIP_SNMP && SNMP_LWIP_MIB2

#define MIB2_AUTH_TRAPS_ENABLED  1
#define MIB2_AUTH_TRAPS_DISABLED 2

/* --- snmp .1.3.6.1.2.1.11 ----------------------------------------------------- */
static s16_t
snmp_get_value(const struct snmp_scalar_array_node_def *node, void *value)
{
  u32_t *uint_ptr = (u32_t *)value;
  switch (node->oid) {
    case 1: /* snmpInPkts */
      *uint_ptr = snmp_stats.inpkts;
      break;
    case 2: /* snmpOutPkts */
      *uint_ptr = snmp_stats.outpkts;
      break;
    case 3: /* snmpInBadVersions */
      *uint_ptr = snmp_stats.inbadversions;
      break;
    case 4: /* snmpInBadCommunityNames */
      *uint_ptr = snmp_stats.inbadcommunitynames;
      break;
    case 5: /* snmpInBadCommunityUses */
      *uint_ptr = snmp_stats.inbadcommunityuses;
      break;
    case 6: /* snmpInASNParseErrs */
      *uint_ptr = snmp_stats.inasnparseerrs;
      break;
    case 8: /* snmpInTooBigs */
      *uint_ptr = snmp_stats.intoobigs;
      break;
    case 9: /* snmpInNoSuchNames */
      *uint_ptr = snmp_stats.innosuchnames;
      break;
    case 10: /* snmpInBadValues */
      *uint_ptr = snmp_stats.inbadvalues;
      break;
    case 11: /* snmpInReadOnlys */
      *uint_ptr = snmp_stats.inreadonlys;
      break;
    case 12: /* snmpInGenErrs */
      *uint_ptr = snmp_stats.ingenerrs;
      break;
    case 13: /* snmpInTotalReqVars */
      *uint_ptr = snmp_stats.intotalreqvars;
      break;
    case 14: /* snmpInTotalSetVars */
      *uint_ptr = snmp_stats.intotalsetvars;
      break;
    case 15: /* snmpInGetRequests */
      *uint_ptr = snmp_stats.ingetrequests;
      break;
    case 16: /* snmpInGetNexts */
      *uint_ptr = snmp_stats.ingetnexts;
      break;
    case 17: /* snmpInSetRequests */
      *uint_ptr = snmp_stats.insetrequests;
      break;
    case 18: /* snmpInGetResponses */
      *uint_ptr = snmp_stats.ingetresponses;
      break;
    case 19: /* snmpInTraps */
      *uint_ptr = snmp_stats.intraps;
      break;
    case 20: /* snmpOutTooBigs */
      *uint_ptr = snmp_stats.outtoobigs;
      break;
    case 21: /* snmpOutNoSuchNames */
      *uint_ptr = snmp_stats.outnosuchnames;
      break;
    case 22: /* snmpOutBadValues */
      *uint_ptr = snmp_stats.outbadvalues;
      break;
    case 24: /* snmpOutGenErrs */
      *uint_ptr = snmp_stats.outgenerrs;
      break;
    case 25: /* snmpOutGetRequests */
      *uint_ptr = snmp_stats.outgetrequests;
      break;
    case 26: /* snmpOutGetNexts */
      *uint_ptr = snmp_stats.outgetnexts;
      break;
    case 27: /* snmpOutSetRequests */
      *uint_ptr = snmp_stats.outsetrequests;
      break;
    case 28: /* snmpOutGetResponses */
      *uint_ptr = snmp_stats.outgetresponses;
      break;
    case 29: /* snmpOutTraps */
      *uint_ptr = snmp_stats.outtraps;
      break;
    case 30: /* snmpEnableAuthenTraps */
      if (snmp_get_auth_traps_enabled() == SNMP_AUTH_TRAPS_DISABLED) {
        *uint_ptr = MIB2_AUTH_TRAPS_DISABLED;
      } else {
        *uint_ptr = MIB2_AUTH_TRAPS_ENABLED;
      }
      break;
    case 31: /* snmpSilentDrops */
      *uint_ptr = 0; /* not supported */
      break;
    case 32: /* snmpProxyDrops */
      *uint_ptr = 0; /* not supported */
      break;
    default:
      LWIP_DEBUGF(SNMP_MIB_DEBUG, ("snmp_get_value(): unknown id: %"S32_F"\n", node->oid));
      return 0;
  }

  return sizeof(*uint_ptr);
}

static snmp_err_t
snmp_set_test(const struct snmp_scalar_array_node_def *node, u16_t len, void *value)
{
  snmp_err_t ret = SNMP_ERR_WRONGVALUE;
  LWIP_UNUSED_ARG(len);

  if (node->oid == 30) {
    /* snmpEnableAuthenTraps */
    s32_t *sint_ptr = (s32_t *)value;

    /* we should have writable non-volatile mem here */
    if ((*sint_ptr == MIB2_AUTH_TRAPS_DISABLED) || (*sint_ptr == MIB2_AUTH_TRAPS_ENABLED)) {
      ret = SNMP_ERR_NOERROR;
    }
  }
  return ret;
}

static snmp_err_t
snmp_set_value(const struct snmp_scalar_array_node_def *node, u16_t len, void *value)
{
  LWIP_UNUSED_ARG(len);

  if (node->oid == 30) {
    /* snmpEnableAuthenTraps */
    s32_t *sint_ptr = (s32_t *)value;
    if (*sint_ptr == MIB2_AUTH_TRAPS_DISABLED) {
      snmp_set_auth_traps_enabled(SNMP_AUTH_TRAPS_DISABLED);
    } else {
      snmp_set_auth_traps_enabled(SNMP_AUTH_TRAPS_ENABLED);
    }
  }

  return SNMP_ERR_NOERROR;
}

/* the following nodes access variables in SNMP stack (snmp_stats) from SNMP worker thread -> OK, no sync needed */
static const struct snmp_scalar_array_node_def snmp_nodes[] = {
  { 1, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInPkts */
  { 2, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutPkts */
  { 3, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInBadVersions */
  { 4, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInBadCommunityNames */
  { 5, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInBadCommunityUses */
  { 6, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInASNParseErrs */
  { 8, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInTooBigs */
  { 9, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInNoSuchNames */
  {10, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInBadValues */
  {11, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInReadOnlys */
  {12, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInGenErrs */
  {13, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInTotalReqVars */
  {14, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInTotalSetVars */
  {15, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInGetRequests */
  {16, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInGetNexts */
  {17, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInSetRequests */
  {18, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInGetResponses */
  {19, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpInTraps */
  {20, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutTooBigs */
  {21, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutNoSuchNames */
  {22, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutBadValues */
  {24, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutGenErrs */
  {25, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutGetRequests */
  {26, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutGetNexts */
  {27, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutSetRequests */
  {28, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutGetResponses */
  {29, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpOutTraps */
  {30, SNMP_ASN1_TYPE_INTEGER, SNMP_NODE_INSTANCE_READ_WRITE}, /* snmpEnableAuthenTraps */
  {31, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY},  /* snmpSilentDrops */
  {32, SNMP_ASN1_TYPE_COUNTER, SNMP_NODE_INSTANCE_READ_ONLY}   /* snmpProxyDrops */
};

const struct snmp_scalar_array_node snmp_mib2_snmp_root = SNMP_SCALAR_CREATE_ARRAY_NODE(11, snmp_nodes, snmp_get_value, snmp_set_test, snmp_set_value);

#endif /* LWIP_SNMP && SNMP_LWIP_MIB2 */
