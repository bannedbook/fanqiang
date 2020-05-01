/**
 * @file
 * Abstract Syntax Notation One (ISO 8824, 8825) codec.
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

#ifndef LWIP_HDR_APPS_SNMP_ASN1_H
#define LWIP_HDR_APPS_SNMP_ASN1_H

#include "lwip/apps/snmp_opts.h"

#if LWIP_SNMP

#include "lwip/err.h"
#include "lwip/apps/snmp_core.h"
#include "snmp_pbuf_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SNMP_ASN1_TLV_INDEFINITE_LENGTH 0x80

#define SNMP_ASN1_CLASS_MASK        0xC0
#define SNMP_ASN1_CONTENTTYPE_MASK  0x20
#define SNMP_ASN1_DATATYPE_MASK     0x1F
#define SNMP_ASN1_DATATYPE_EXTENDED 0x1F /* DataType indicating that datatype is encoded in following bytes */

/* context specific (SNMP) tags (from SNMP spec. RFC1157 and RFC1905) */
#define SNMP_ASN1_CONTEXT_PDU_GET_REQ      0
#define SNMP_ASN1_CONTEXT_PDU_GET_NEXT_REQ 1
#define SNMP_ASN1_CONTEXT_PDU_GET_RESP     2
#define SNMP_ASN1_CONTEXT_PDU_SET_REQ      3
#define SNMP_ASN1_CONTEXT_PDU_TRAP         4
#define SNMP_ASN1_CONTEXT_PDU_GET_BULK_REQ 5
#define SNMP_ASN1_CONTEXT_PDU_INFORM_REQ   6
#define SNMP_ASN1_CONTEXT_PDU_V2_TRAP      7
#define SNMP_ASN1_CONTEXT_PDU_REPORT       8

#define SNMP_ASN1_CONTEXT_VARBIND_NO_SUCH_OBJECT      0
#define SNMP_ASN1_CONTEXT_VARBIND_END_OF_MIB_VIEW     2

struct snmp_asn1_tlv {
  u8_t  type;       /* only U8 because extended types are not specified by SNMP */
  u8_t  type_len;   /* encoded length of 'type' field (normally 1) */
  u8_t  length_len; /* indicates how many bytes are required to encode the 'value_len' field */
  u16_t value_len;  /* encoded length of the value */
};
#define SNMP_ASN1_TLV_HDR_LENGTH(tlv) ((tlv).type_len + (tlv).length_len)
#define SNMP_ASN1_TLV_LENGTH(tlv) ((tlv).type_len + (tlv).length_len + (tlv).value_len)
#define SNMP_ASN1_SET_TLV_PARAMS(tlv, type_, length_len_, value_len_) do { (tlv).type = (type_); (tlv).type_len = 0; (tlv).length_len = (length_len_); (tlv).value_len = (value_len_); } while (0);

err_t snmp_asn1_dec_tlv(struct snmp_pbuf_stream *pbuf_stream, struct snmp_asn1_tlv *tlv);
err_t snmp_asn1_dec_u32t(struct snmp_pbuf_stream *pbuf_stream, u16_t len, u32_t *value);
err_t snmp_asn1_dec_s32t(struct snmp_pbuf_stream *pbuf_stream, u16_t len, s32_t *value);
err_t snmp_asn1_dec_oid(struct snmp_pbuf_stream *pbuf_stream, u16_t len, u32_t *oid, u8_t *oid_len, u8_t oid_max_len);
err_t snmp_asn1_dec_raw(struct snmp_pbuf_stream *pbuf_stream, u16_t len, u8_t *buf, u16_t *buf_len, u16_t buf_max_len);

err_t snmp_ans1_enc_tlv(struct snmp_pbuf_stream *pbuf_stream, struct snmp_asn1_tlv *tlv);

void snmp_asn1_enc_length_cnt(u16_t length, u8_t *octets_needed);
void snmp_asn1_enc_u32t_cnt(u32_t value, u16_t *octets_needed);
void snmp_asn1_enc_s32t_cnt(s32_t value, u16_t *octets_needed);
void snmp_asn1_enc_oid_cnt(const u32_t *oid, u16_t oid_len, u16_t *octets_needed);
err_t snmp_asn1_enc_oid(struct snmp_pbuf_stream *pbuf_stream, const u32_t *oid, u16_t oid_len);
err_t snmp_asn1_enc_s32t(struct snmp_pbuf_stream *pbuf_stream, u16_t octets_needed, s32_t value);
err_t snmp_asn1_enc_u32t(struct snmp_pbuf_stream *pbuf_stream, u16_t octets_needed, u32_t value);
err_t snmp_asn1_enc_raw(struct snmp_pbuf_stream *pbuf_stream, const u8_t *raw, u16_t raw_len);

#if LWIP_HAVE_INT64
err_t snmp_asn1_dec_u64t(struct snmp_pbuf_stream *pbuf_stream, u16_t len, u64_t *value);
void snmp_asn1_enc_u64t_cnt(u64_t value, u16_t *octets_needed);
err_t snmp_asn1_enc_u64t(struct snmp_pbuf_stream *pbuf_stream, u16_t octets_needed, u64_t value);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LWIP_SNMP */

#endif /* LWIP_HDR_APPS_SNMP_ASN1_H */
