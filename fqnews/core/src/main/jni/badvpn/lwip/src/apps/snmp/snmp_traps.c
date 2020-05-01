/**
 * @file
 * SNMPv1 traps implementation.
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
 * Author: Martin Hentschel
 *         Christiaan Simons <christiaan.simons@axon.tv>
 *
 */

#include "lwip/apps/snmp_opts.h"

#if LWIP_SNMP /* don't build if not configured for use in lwipopts.h */

#include <string.h>

#include "lwip/snmp.h"
#include "lwip/sys.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_core.h"
#include "lwip/prot/iana.h"
#include "snmp_msg.h"
#include "snmp_asn1.h"
#include "snmp_core_priv.h"

struct snmp_msg_trap {
  /* source enterprise ID (sysObjectID) */
  const struct snmp_obj_id *enterprise;
  /* source IP address, raw network order format */
  ip_addr_t sip;
  /* generic trap code */
  u32_t gen_trap;
  /* specific trap code */
  u32_t spc_trap;
  /* timestamp */
  u32_t ts;
  /* snmp_version */
  u32_t snmp_version;

  /* output trap lengths used in ASN encoding */
  /* encoding pdu length */
  u16_t pdulen;
  /* encoding community length */
  u16_t comlen;
  /* encoding sequence length */
  u16_t seqlen;
  /* encoding varbinds sequence length */
  u16_t vbseqlen;
};

static u16_t snmp_trap_varbind_sum(struct snmp_msg_trap *trap, struct snmp_varbind *varbinds);
static u16_t snmp_trap_header_sum(struct snmp_msg_trap *trap, u16_t vb_len);
static err_t snmp_trap_header_enc(struct snmp_msg_trap *trap, struct snmp_pbuf_stream *pbuf_stream);
static err_t snmp_trap_varbind_enc(struct snmp_msg_trap *trap, struct snmp_pbuf_stream *pbuf_stream, struct snmp_varbind *varbinds);

#define BUILD_EXEC(code) \
  if ((code) != ERR_OK) { \
    LWIP_DEBUGF(SNMP_DEBUG, ("SNMP error during creation of outbound trap frame!")); \
    return ERR_ARG; \
  }

/** Agent community string for sending traps */
extern const char *snmp_community_trap;

void *snmp_traps_handle;

struct snmp_trap_dst {
  /* destination IP address in network order */
  ip_addr_t dip;
  /* set to 0 when disabled, >0 when enabled */
  u8_t enable;
};
static struct snmp_trap_dst trap_dst[SNMP_TRAP_DESTINATIONS];

static u8_t snmp_auth_traps_enabled = 0;

/**
 * @ingroup snmp_traps
 * Sets enable switch for this trap destination.
 * @param dst_idx index in 0 .. SNMP_TRAP_DESTINATIONS-1
 * @param enable switch if 0 destination is disabled >0 enabled.
 */
void
snmp_trap_dst_enable(u8_t dst_idx, u8_t enable)
{
  if (dst_idx < SNMP_TRAP_DESTINATIONS) {
    trap_dst[dst_idx].enable = enable;
  }
}

/**
 * @ingroup snmp_traps
 * Sets IPv4 address for this trap destination.
 * @param dst_idx index in 0 .. SNMP_TRAP_DESTINATIONS-1
 * @param dst IPv4 address in host order.
 */
void
snmp_trap_dst_ip_set(u8_t dst_idx, const ip_addr_t *dst)
{
  if (dst_idx < SNMP_TRAP_DESTINATIONS) {
    ip_addr_set(&trap_dst[dst_idx].dip, dst);
  }
}

/**
 * @ingroup snmp_traps
 * Enable/disable authentication traps
 */
void
snmp_set_auth_traps_enabled(u8_t enable)
{
  snmp_auth_traps_enabled = enable;
}

/**
 * @ingroup snmp_traps
 * Get authentication traps enabled state
 */
u8_t
snmp_get_auth_traps_enabled(void)
{
  return snmp_auth_traps_enabled;
}


/**
 * @ingroup snmp_traps
 * Sends a generic or enterprise specific trap message.
 *
 * @param eoid points to enterprise object identifier
 * @param generic_trap is the trap code
 * @param specific_trap used for enterprise traps when generic_trap == 6
 * @param varbinds linked list of varbinds to be sent
 * @return ERR_OK when success, ERR_MEM if we're out of memory
 *
 * @note the use of the enterprise identifier field
 * is per RFC1215.
 * Use .iso.org.dod.internet.mgmt.mib-2.snmp for generic traps
 * and .iso.org.dod.internet.private.enterprises.yourenterprise
 * (sysObjectID) for specific traps.
 */
err_t
snmp_send_trap(const struct snmp_obj_id *eoid, s32_t generic_trap, s32_t specific_trap, struct snmp_varbind *varbinds)
{
  struct snmp_msg_trap trap_msg;
  struct snmp_trap_dst *td;
  struct pbuf *p;
  u16_t i, tot_len;
  err_t err = ERR_OK;

  trap_msg.snmp_version = 0;

  for (i = 0, td = &trap_dst[0]; i < SNMP_TRAP_DESTINATIONS; i++, td++) {
    if ((td->enable != 0) && !ip_addr_isany(&td->dip)) {
      /* lookup current source address for this dst */
      if (snmp_get_local_ip_for_dst(snmp_traps_handle, &td->dip, &trap_msg.sip)) {
        if (eoid == NULL) {
          trap_msg.enterprise = snmp_get_device_enterprise_oid();
        } else {
          trap_msg.enterprise = eoid;
        }

        trap_msg.gen_trap = generic_trap;
        if (generic_trap == SNMP_GENTRAP_ENTERPRISE_SPECIFIC) {
          trap_msg.spc_trap = specific_trap;
        } else {
          trap_msg.spc_trap = 0;
        }

        MIB2_COPY_SYSUPTIME_TO(&trap_msg.ts);

        /* pass 0, calculate length fields */
        tot_len = snmp_trap_varbind_sum(&trap_msg, varbinds);
        tot_len = snmp_trap_header_sum(&trap_msg, tot_len);

        /* allocate pbuf(s) */
        p = pbuf_alloc(PBUF_TRANSPORT, tot_len, PBUF_RAM);
        if (p != NULL) {
          struct snmp_pbuf_stream pbuf_stream;
          snmp_pbuf_stream_init(&pbuf_stream, p, 0, tot_len);

          /* pass 1, encode packet into the pbuf(s) */
          snmp_trap_header_enc(&trap_msg, &pbuf_stream);
          snmp_trap_varbind_enc(&trap_msg, &pbuf_stream, varbinds);

          snmp_stats.outtraps++;
          snmp_stats.outpkts++;

          /** send to the TRAP destination */
          snmp_sendto(snmp_traps_handle, p, &td->dip, LWIP_IANA_PORT_SNMP_TRAP);
          pbuf_free(p);
        } else {
          err = ERR_MEM;
        }
      } else {
        /* routing error */
        err = ERR_RTE;
      }
    }
  }
  return err;
}

/**
 * @ingroup snmp_traps
 * Send generic SNMP trap
 */
err_t
snmp_send_trap_generic(s32_t generic_trap)
{
  static const struct snmp_obj_id oid = { 7, { 1, 3, 6, 1, 2, 1, 11 } };
  return snmp_send_trap(&oid, generic_trap, 0, NULL);
}

/**
 * @ingroup snmp_traps
 * Send specific SNMP trap with variable bindings
 */
err_t
snmp_send_trap_specific(s32_t specific_trap, struct snmp_varbind *varbinds)
{
  return snmp_send_trap(NULL, SNMP_GENTRAP_ENTERPRISE_SPECIFIC, specific_trap, varbinds);
}

/**
 * @ingroup snmp_traps
 * Send coldstart trap
 */
void
snmp_coldstart_trap(void)
{
  snmp_send_trap_generic(SNMP_GENTRAP_COLDSTART);
}

/**
 * @ingroup snmp_traps
 * Send authentication failure trap (used internally by agent)
 */
void
snmp_authfail_trap(void)
{
  if (snmp_auth_traps_enabled != 0) {
    snmp_send_trap_generic(SNMP_GENTRAP_AUTH_FAILURE);
  }
}

static u16_t
snmp_trap_varbind_sum(struct snmp_msg_trap *trap, struct snmp_varbind *varbinds)
{
  struct snmp_varbind *varbind;
  u16_t tot_len;
  u8_t tot_len_len;

  tot_len = 0;
  varbind = varbinds;
  while (varbind != NULL) {
    struct snmp_varbind_len len;

    if (snmp_varbind_length(varbind, &len) == ERR_OK) {
      tot_len += 1 + len.vb_len_len + len.vb_value_len;
    }

    varbind = varbind->next;
  }

  trap->vbseqlen = tot_len;
  snmp_asn1_enc_length_cnt(trap->vbseqlen, &tot_len_len);
  tot_len += 1 + tot_len_len;

  return tot_len;
}

/**
 * Sums trap header field lengths from tail to head and
 * returns trap_header_lengths for second encoding pass.
 *
 * @param trap Trap message
 * @param vb_len varbind-list length
 * @return the required length for encoding the trap header
 */
static u16_t
snmp_trap_header_sum(struct snmp_msg_trap *trap, u16_t vb_len)
{
  u16_t tot_len;
  u16_t len;
  u8_t lenlen;

  tot_len = vb_len;

  snmp_asn1_enc_u32t_cnt(trap->ts, &len);
  snmp_asn1_enc_length_cnt(len, &lenlen);
  tot_len += 1 + len + lenlen;

  snmp_asn1_enc_s32t_cnt(trap->spc_trap, &len);
  snmp_asn1_enc_length_cnt(len, &lenlen);
  tot_len += 1 + len + lenlen;

  snmp_asn1_enc_s32t_cnt(trap->gen_trap, &len);
  snmp_asn1_enc_length_cnt(len, &lenlen);
  tot_len += 1 + len + lenlen;

  if (IP_IS_V6_VAL(trap->sip)) {
#if LWIP_IPV6
    len = sizeof(ip_2_ip6(&trap->sip)->addr);
#endif
  } else {
#if LWIP_IPV4
    len = sizeof(ip_2_ip4(&trap->sip)->addr);
#endif
  }
  snmp_asn1_enc_length_cnt(len, &lenlen);
  tot_len += 1 + len + lenlen;

  snmp_asn1_enc_oid_cnt(trap->enterprise->id, trap->enterprise->len, &len);
  snmp_asn1_enc_length_cnt(len, &lenlen);
  tot_len += 1 + len + lenlen;

  trap->pdulen = tot_len;
  snmp_asn1_enc_length_cnt(trap->pdulen, &lenlen);
  tot_len += 1 + lenlen;

  trap->comlen = (u16_t)LWIP_MIN(strlen(snmp_community_trap), 0xFFFF);
  snmp_asn1_enc_length_cnt(trap->comlen, &lenlen);
  tot_len += 1 + lenlen + trap->comlen;

  snmp_asn1_enc_s32t_cnt(trap->snmp_version, &len);
  snmp_asn1_enc_length_cnt(len, &lenlen);
  tot_len += 1 + len + lenlen;

  trap->seqlen = tot_len;
  snmp_asn1_enc_length_cnt(trap->seqlen, &lenlen);
  tot_len += 1 + lenlen;

  return tot_len;
}

static err_t
snmp_trap_varbind_enc(struct snmp_msg_trap *trap, struct snmp_pbuf_stream *pbuf_stream, struct snmp_varbind *varbinds)
{
  struct snmp_asn1_tlv tlv;
  struct snmp_varbind *varbind;

  varbind = varbinds;

  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 0, trap->vbseqlen);
  BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );

  while (varbind != NULL) {
    BUILD_EXEC( snmp_append_outbound_varbind(pbuf_stream, varbind) );

    varbind = varbind->next;
  }

  return ERR_OK;
}

/**
 * Encodes trap header from head to tail.
 */
static err_t
snmp_trap_header_enc(struct snmp_msg_trap *trap, struct snmp_pbuf_stream *pbuf_stream)
{
  struct snmp_asn1_tlv tlv;

  /* 'Message' sequence */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 0, trap->seqlen);
  BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );

  /* version */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 0);
  snmp_asn1_enc_s32t_cnt(trap->snmp_version, &tlv.value_len);
  BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  BUILD_EXEC( snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, trap->snmp_version) );

  /* community */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, trap->comlen);
  BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  BUILD_EXEC( snmp_asn1_enc_raw(pbuf_stream,  (const u8_t *)snmp_community_trap, trap->comlen) );

  /* 'PDU' sequence */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, (SNMP_ASN1_CLASS_CONTEXT | SNMP_ASN1_CONTENTTYPE_CONSTRUCTED | SNMP_ASN1_CONTEXT_PDU_TRAP), 0, trap->pdulen);
  BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );

  /* object ID */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OBJECT_ID, 0, 0);
  snmp_asn1_enc_oid_cnt(trap->enterprise->id, trap->enterprise->len, &tlv.value_len);
  BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  BUILD_EXEC( snmp_asn1_enc_oid(pbuf_stream, trap->enterprise->id, trap->enterprise->len) );

  /* IP addr */
  if (IP_IS_V6_VAL(trap->sip)) {
#if LWIP_IPV6
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_IPADDR, 0, sizeof(ip_2_ip6(&trap->sip)->addr));
    BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
    BUILD_EXEC( snmp_asn1_enc_raw(pbuf_stream, (const u8_t *)&ip_2_ip6(&trap->sip)->addr, sizeof(ip_2_ip6(&trap->sip)->addr)) );
#endif
  } else {
#if LWIP_IPV4
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_IPADDR, 0, sizeof(ip_2_ip4(&trap->sip)->addr));
    BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
    BUILD_EXEC( snmp_asn1_enc_raw(pbuf_stream, (const u8_t *)&ip_2_ip4(&trap->sip)->addr, sizeof(ip_2_ip4(&trap->sip)->addr)) );
#endif
  }

  /* trap length */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 0);
  snmp_asn1_enc_s32t_cnt(trap->gen_trap, &tlv.value_len);
  BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  BUILD_EXEC( snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, trap->gen_trap) );

  /* specific trap */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 0);
  snmp_asn1_enc_s32t_cnt(trap->spc_trap, &tlv.value_len);
  BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  BUILD_EXEC( snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, trap->spc_trap) );

  /* timestamp */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_TIMETICKS, 0, 0);
  snmp_asn1_enc_s32t_cnt(trap->ts, &tlv.value_len);
  BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  BUILD_EXEC( snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, trap->ts) );

  return ERR_OK;
}

#endif /* LWIP_SNMP */
