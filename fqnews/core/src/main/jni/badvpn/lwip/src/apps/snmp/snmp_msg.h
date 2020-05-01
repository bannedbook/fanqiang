/**
 * @file
 * SNMP Agent message handling structures (internal API, do not use in client code).
 */

/*
 * Copyright (c) 2006 Axon Digital Design B.V., The Netherlands.
 * Copyright (c) 2016 Elias Oenal.
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
 * Author: Christiaan Simons <christiaan.simons@axon.tv>
 *         Martin Hentschel <info@cl-soft.de>
 *         Elias Oenal <lwip@eliasoenal.com>
 */

#ifndef LWIP_HDR_APPS_SNMP_MSG_H
#define LWIP_HDR_APPS_SNMP_MSG_H

#include "lwip/apps/snmp_opts.h"

#if LWIP_SNMP

#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_core.h"
#include "snmp_pbuf_stream.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"

#if LWIP_SNMP_V3
#include "snmpv3_priv.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* version defines used in PDU */
#define SNMP_VERSION_1  0
#define SNMP_VERSION_2c 1
#define SNMP_VERSION_3  3

struct snmp_varbind_enumerator {
  struct snmp_pbuf_stream pbuf_stream;
  u16_t varbind_count;
};

typedef enum {
  SNMP_VB_ENUMERATOR_ERR_OK            = 0,
  SNMP_VB_ENUMERATOR_ERR_EOVB          = 1,
  SNMP_VB_ENUMERATOR_ERR_ASN1ERROR     = 2,
  SNMP_VB_ENUMERATOR_ERR_INVALIDLENGTH = 3
} snmp_vb_enumerator_err_t;

void snmp_vb_enumerator_init(struct snmp_varbind_enumerator *enumerator, struct pbuf *p, u16_t offset, u16_t length);
snmp_vb_enumerator_err_t snmp_vb_enumerator_get_next(struct snmp_varbind_enumerator *enumerator, struct snmp_varbind *varbind);

struct snmp_request {
  /* Communication handle */
  void *handle;
  /* source IP address */
  const ip_addr_t *source_ip;
  /* source UDP port */
  u16_t source_port;
  /* incoming snmp version */
  u8_t version;
  /* community name (zero terminated) */
  u8_t community[SNMP_MAX_COMMUNITY_STR_LEN + 1];
  /* community string length (exclusive zero term) */
  u16_t community_strlen;
  /* request type */
  u8_t request_type;
  /* request ID */
  s32_t request_id;
  /* error status */
  s32_t error_status;
  /* error index */
  s32_t error_index;
  /* non-repeaters (getBulkRequest (SNMPv2c)) */
  s32_t non_repeaters;
  /* max-repetitions (getBulkRequest (SNMPv2c)) */
  s32_t max_repetitions;

  /* Usually response-pdu (2). When snmpv3 errors are detected report-pdu(8) */
  u8_t request_out_type;

#if LWIP_SNMP_V3
  s32_t msg_id;
  s32_t msg_max_size;
  u8_t  msg_flags;
  s32_t msg_security_model;
  u8_t  msg_authoritative_engine_id[SNMP_V3_MAX_ENGINE_ID_LENGTH];
  u8_t  msg_authoritative_engine_id_len;
  s32_t msg_authoritative_engine_boots;
  s32_t msg_authoritative_engine_time;
  u8_t  msg_user_name[SNMP_V3_MAX_USER_LENGTH];
  u8_t  msg_user_name_len;
  u8_t  msg_authentication_parameters[SNMP_V3_MAX_AUTH_PARAM_LENGTH];
  u8_t  msg_authentication_parameters_len;
  u8_t  msg_privacy_parameters[SNMP_V3_MAX_PRIV_PARAM_LENGTH];
  u8_t  msg_privacy_parameters_len;
  u8_t  context_engine_id[SNMP_V3_MAX_ENGINE_ID_LENGTH];
  u8_t  context_engine_id_len;
  u8_t  context_name[SNMP_V3_MAX_ENGINE_ID_LENGTH];
  u8_t  context_name_len;
#endif

  struct pbuf *inbound_pbuf;
  struct snmp_varbind_enumerator inbound_varbind_enumerator;
  u16_t inbound_varbind_offset;
  u16_t inbound_varbind_len;
  u16_t inbound_padding_len;

  struct pbuf *outbound_pbuf;
  struct snmp_pbuf_stream outbound_pbuf_stream;
  u16_t outbound_pdu_offset;
  u16_t outbound_error_status_offset;
  u16_t outbound_error_index_offset;
  u16_t outbound_varbind_offset;
#if LWIP_SNMP_V3
  u16_t outbound_msg_global_data_offset;
  u16_t outbound_msg_global_data_end;
  u16_t outbound_msg_security_parameters_str_offset;
  u16_t outbound_msg_security_parameters_seq_offset;
  u16_t outbound_msg_security_parameters_end;
  u16_t outbound_msg_authentication_parameters_offset;
  u16_t outbound_scoped_pdu_seq_offset;
  u16_t outbound_scoped_pdu_string_offset;
#endif

  u8_t value_buffer[SNMP_MAX_VALUE_SIZE];
};

/** A helper struct keeping length information about varbinds */
struct snmp_varbind_len {
  u8_t  vb_len_len;
  u16_t vb_value_len;
  u8_t  oid_len_len;
  u16_t oid_value_len;
  u8_t  value_len_len;
  u16_t value_value_len;
};

/** Agent community string */
extern const char *snmp_community;
/** Agent community string for write access */
extern const char *snmp_community_write;
/** handle for sending traps */
extern void *snmp_traps_handle;

void snmp_receive(void *handle, struct pbuf *p, const ip_addr_t *source_ip, u16_t port);
err_t snmp_sendto(void *handle, struct pbuf *p, const ip_addr_t *dst, u16_t port);
u8_t snmp_get_local_ip_for_dst(void *handle, const ip_addr_t *dst, ip_addr_t *result);
err_t snmp_varbind_length(struct snmp_varbind *varbind, struct snmp_varbind_len *len);
err_t snmp_append_outbound_varbind(struct snmp_pbuf_stream *pbuf_stream, struct snmp_varbind *varbind);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_SNMP */

#endif /* LWIP_HDR_APPS_SNMP_MSG_H */
