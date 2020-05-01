/**
 * @file
 * SNMP message processing (RFC1157).
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

#include "lwip/apps/snmp_opts.h"

#if LWIP_SNMP /* don't build if not configured for use in lwipopts.h */

#include "snmp_msg.h"
#include "snmp_asn1.h"
#include "snmp_core_priv.h"
#include "lwip/ip_addr.h"
#include "lwip/stats.h"

#if LWIP_SNMP_V3
#include "lwip/apps/snmpv3.h"
#include "snmpv3_priv.h"
#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif
#endif

#include <string.h>

#define SNMP_V3_AUTH_FLAG      0x01
#define SNMP_V3_PRIV_FLAG      0x02

/* Security levels */
#define SNMP_V3_NOAUTHNOPRIV   0x00
#define SNMP_V3_AUTHNOPRIV     SNMP_V3_AUTH_FLAG
#define SNMP_V3_AUTHPRIV       (SNMP_V3_AUTH_FLAG | SNMP_V3_PRIV_FLAG)

/* public (non-static) constants */
/** SNMP community string */
const char *snmp_community = SNMP_COMMUNITY;
/** SNMP community string for write access */
const char *snmp_community_write = SNMP_COMMUNITY_WRITE;
/** SNMP community string for sending traps */
const char *snmp_community_trap = SNMP_COMMUNITY_TRAP;

snmp_write_callback_fct snmp_write_callback     = NULL;
void                   *snmp_write_callback_arg = NULL;

#if LWIP_SNMP_CONFIGURE_VERSIONS

static u8_t v1_enabled = 1;
static u8_t v2c_enabled = 1;
static u8_t v3_enabled = 1;

static u8_t
snmp_version_enabled(u8_t version)
{
  LWIP_ASSERT("Invalid SNMP version", (version == SNMP_VERSION_1) || (version == SNMP_VERSION_2c)
#if LWIP_SNMP_V3
              || (version == SNMP_VERSION_3)
#endif
             );

  if (version == SNMP_VERSION_1) {
    return v1_enabled;
  } else if (version == SNMP_VERSION_2c) {
    return v2c_enabled;
  }
#if LWIP_SNMP_V3
  else { /* version == SNMP_VERSION_3 */
    return v3_enabled;
  }
#endif
}

u8_t
snmp_v1_enabled(void)
{
  return snmp_version_enabled(SNMP_VERSION_1);
}

u8_t
snmp_v2c_enabled(void)
{
  return snmp_version_enabled(SNMP_VERSION_2c);
}

u8_t
snmp_v3_enabled(void)
{
  return snmp_version_enabled(SNMP_VERSION_3);
}

static void
snmp_version_enable(u8_t version, u8_t enable)
{
  LWIP_ASSERT("Invalid SNMP version", (version == SNMP_VERSION_1) || (version == SNMP_VERSION_2c)
#if LWIP_SNMP_V3
              || (version == SNMP_VERSION_3)
#endif
             );

  if (version == SNMP_VERSION_1) {
    v1_enabled = enable;
  } else if (version == SNMP_VERSION_2c) {
    v2c_enabled = enable;
  }
#if LWIP_SNMP_V3
  else { /* version == SNMP_VERSION_3 */
    v3_enabled = enable;
  }
#endif
}

void
snmp_v1_enable(u8_t enable)
{
  snmp_version_enable(SNMP_VERSION_1, enable);
}

void
snmp_v2c_enable(u8_t enable)
{
  snmp_version_enable(SNMP_VERSION_2c, enable);
}

void
snmp_v3_enable(u8_t enable)
{
  snmp_version_enable(SNMP_VERSION_3, enable);
}

#endif

/**
 * @ingroup snmp_core
 * Returns current SNMP community string.
 * @return current SNMP community string
 */
const char *
snmp_get_community(void)
{
  return snmp_community;
}

/**
 * @ingroup snmp_core
 * Sets SNMP community string.
 * The string itself (its storage) must be valid throughout the whole life of
 * program (or until it is changed to sth else).
 *
 * @param community is a pointer to new community string
 */
void
snmp_set_community(const char *const community)
{
  LWIP_ASSERT("community string is too long!", strlen(community) <= SNMP_MAX_COMMUNITY_STR_LEN);
  snmp_community = community;
}

/**
 * @ingroup snmp_core
 * Returns current SNMP write-access community string.
 * @return current SNMP write-access community string
 */
const char *
snmp_get_community_write(void)
{
  return snmp_community_write;
}

/**
 * @ingroup snmp_traps
 * Returns current SNMP community string used for sending traps.
 * @return current SNMP community string used for sending traps
 */
const char *
snmp_get_community_trap(void)
{
  return snmp_community_trap;
}

/**
 * @ingroup snmp_core
 * Sets SNMP community string for write-access.
 * The string itself (its storage) must be valid throughout the whole life of
 * program (or until it is changed to sth else).
 *
 * @param community is a pointer to new write-access community string
 */
void
snmp_set_community_write(const char *const community)
{
  LWIP_ASSERT("community string must not be NULL", community != NULL);
  LWIP_ASSERT("community string is too long!", strlen(community) <= SNMP_MAX_COMMUNITY_STR_LEN);
  snmp_community_write = community;
}

/**
 * @ingroup snmp_traps
 * Sets SNMP community string used for sending traps.
 * The string itself (its storage) must be valid throughout the whole life of
 * program (or until it is changed to sth else).
 *
 * @param community is a pointer to new trap community string
 */
void
snmp_set_community_trap(const char *const community)
{
  LWIP_ASSERT("community string is too long!", strlen(community) <= SNMP_MAX_COMMUNITY_STR_LEN);
  snmp_community_trap = community;
}

/**
 * @ingroup snmp_core
 * Callback fired on every successful write access
 */
void
snmp_set_write_callback(snmp_write_callback_fct write_callback, void *callback_arg)
{
  snmp_write_callback     = write_callback;
  snmp_write_callback_arg = callback_arg;
}

/* ----------------------------------------------------------------------- */
/* forward declarations */
/* ----------------------------------------------------------------------- */

static err_t snmp_process_get_request(struct snmp_request *request);
static err_t snmp_process_getnext_request(struct snmp_request *request);
static err_t snmp_process_getbulk_request(struct snmp_request *request);
static err_t snmp_process_set_request(struct snmp_request *request);

static err_t snmp_parse_inbound_frame(struct snmp_request *request);
static err_t snmp_prepare_outbound_frame(struct snmp_request *request);
static err_t snmp_complete_outbound_frame(struct snmp_request *request);
static void snmp_execute_write_callbacks(struct snmp_request *request);


/* ----------------------------------------------------------------------- */
/* implementation */
/* ----------------------------------------------------------------------- */

void
snmp_receive(void *handle, struct pbuf *p, const ip_addr_t *source_ip, u16_t port)
{
  err_t err;
  struct snmp_request request;

  memset(&request, 0, sizeof(request));
  request.handle       = handle;
  request.source_ip    = source_ip;
  request.source_port  = port;
  request.inbound_pbuf = p;

  snmp_stats.inpkts++;

  err = snmp_parse_inbound_frame(&request);
  if (err == ERR_OK) {
    err = snmp_prepare_outbound_frame(&request);
    if (err == ERR_OK) {

      if (request.error_status == SNMP_ERR_NOERROR) {
        /* only process frame if we do not already have an error to return (e.g. all readonly) */
        if (request.request_type == SNMP_ASN1_CONTEXT_PDU_GET_REQ) {
          err = snmp_process_get_request(&request);
        } else if (request.request_type == SNMP_ASN1_CONTEXT_PDU_GET_NEXT_REQ) {
          err = snmp_process_getnext_request(&request);
        } else if (request.request_type == SNMP_ASN1_CONTEXT_PDU_GET_BULK_REQ) {
          err = snmp_process_getbulk_request(&request);
        } else if (request.request_type == SNMP_ASN1_CONTEXT_PDU_SET_REQ) {
          err = snmp_process_set_request(&request);
        }
      }
#if LWIP_SNMP_V3
      else {
        struct snmp_varbind vb;

        vb.next = NULL;
        vb.prev = NULL;
        vb.type = SNMP_ASN1_TYPE_COUNTER32;
        vb.value_len = sizeof(u32_t);

        switch (request.error_status) {
          case SNMP_ERR_AUTHORIZATIONERROR: {
            static const u32_t oid[] = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 5, 0 };
            snmp_oid_assign(&vb.oid, oid, LWIP_ARRAYSIZE(oid));
            vb.value = &snmp_stats.wrongdigests;
          }
          break;
          case SNMP_ERR_UNKNOWN_ENGINEID: {
            static const u32_t oid[] = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 4, 0 };
            snmp_oid_assign(&vb.oid, oid, LWIP_ARRAYSIZE(oid));
            vb.value = &snmp_stats.unknownengineids;
          }
          break;
          case SNMP_ERR_UNKNOWN_SECURITYNAME: {
            static const u32_t oid[] = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 3, 0 };
            snmp_oid_assign(&vb.oid, oid, LWIP_ARRAYSIZE(oid));
            vb.value = &snmp_stats.unknownusernames;
          }
          break;
          case SNMP_ERR_UNSUPPORTED_SECLEVEL: {
            static const u32_t oid[] = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 1, 0 };
            snmp_oid_assign(&vb.oid, oid, LWIP_ARRAYSIZE(oid));
            vb.value = &snmp_stats.unsupportedseclevels;
          }
          break;
          case SNMP_ERR_NOTINTIMEWINDOW: {
            static const u32_t oid[] = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 2, 0 };
            snmp_oid_assign(&vb.oid, oid, LWIP_ARRAYSIZE(oid));
            vb.value = &snmp_stats.notintimewindows;
          }
          break;
          case SNMP_ERR_DECRYIPTION_ERROR: {
            static const u32_t oid[] = { 1, 3, 6, 1, 6, 3, 15, 1, 1, 6, 0 };
            snmp_oid_assign(&vb.oid, oid, LWIP_ARRAYSIZE(oid));
            vb.value = &snmp_stats.decryptionerrors;
          }
          break;
          default:
            /* Unknown or unhandled error_status */
            err = ERR_ARG;
        }

        if (err == ERR_OK) {
          snmp_append_outbound_varbind(&(request.outbound_pbuf_stream), &vb);
          request.error_status = SNMP_ERR_NOERROR;
        }

        request.request_out_type = (SNMP_ASN1_CLASS_CONTEXT | SNMP_ASN1_CONTENTTYPE_CONSTRUCTED | SNMP_ASN1_CONTEXT_PDU_REPORT);
        request.request_id = request.msg_id;
      }
#endif

      if (err == ERR_OK) {
        err = snmp_complete_outbound_frame(&request);

        if (err == ERR_OK) {
          err = snmp_sendto(request.handle, request.outbound_pbuf, request.source_ip, request.source_port);

          if ((request.request_type == SNMP_ASN1_CONTEXT_PDU_SET_REQ)
              && (request.error_status == SNMP_ERR_NOERROR)
              && (snmp_write_callback != NULL)) {
            /* raise write notification for all written objects */
            snmp_execute_write_callbacks(&request);
          }
        }
      }
    }

    if (request.outbound_pbuf != NULL) {
      pbuf_free(request.outbound_pbuf);
    }
  }
}

static u8_t
snmp_msg_getnext_validate_node_inst(struct snmp_node_instance *node_instance, void *validate_arg)
{
  if (((node_instance->access & SNMP_NODE_INSTANCE_ACCESS_READ) != SNMP_NODE_INSTANCE_ACCESS_READ) || (node_instance->get_value == NULL)) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

#if LWIP_HAVE_INT64
  if ((node_instance->asn1_type == SNMP_ASN1_TYPE_COUNTER64) && (((struct snmp_request *)validate_arg)->version == SNMP_VERSION_1)) {
    /* according to RFC 2089 skip Counter64 objects in GetNext requests from v1 clients */
    return SNMP_ERR_NOSUCHINSTANCE;
  }
#endif

  return SNMP_ERR_NOERROR;
}

static void
snmp_process_varbind(struct snmp_request *request, struct snmp_varbind *vb, u8_t get_next)
{
  err_t err;
  struct snmp_node_instance node_instance;
  memset(&node_instance, 0, sizeof(node_instance));

  if (get_next) {
    struct snmp_obj_id result_oid;
    request->error_status = snmp_get_next_node_instance_from_oid(vb->oid.id, vb->oid.len, snmp_msg_getnext_validate_node_inst, request,  &result_oid, &node_instance);

    if (request->error_status == SNMP_ERR_NOERROR) {
      snmp_oid_assign(&vb->oid, result_oid.id, result_oid.len);
    }
  } else {
    request->error_status = snmp_get_node_instance_from_oid(vb->oid.id, vb->oid.len, &node_instance);

    if (request->error_status == SNMP_ERR_NOERROR) {
      /* use 'getnext_validate' method for validation to avoid code duplication (some checks have to be executed here) */
      request->error_status = snmp_msg_getnext_validate_node_inst(&node_instance, request);

      if (request->error_status != SNMP_ERR_NOERROR) {
        if (node_instance.release_instance != NULL) {
          node_instance.release_instance(&node_instance);
        }
      }
    }
  }

  if (request->error_status != SNMP_ERR_NOERROR)  {
    if (request->error_status >= SNMP_VARBIND_EXCEPTION_OFFSET) {
      if ((request->version == SNMP_VERSION_2c) || request->version == SNMP_VERSION_3) {
        /* in SNMP v2c a varbind related exception is stored in varbind and not in frame header */
        vb->type = (SNMP_ASN1_CONTENTTYPE_PRIMITIVE | SNMP_ASN1_CLASS_CONTEXT | (request->error_status & SNMP_VARBIND_EXCEPTION_MASK));
        vb->value_len = 0;

        err = snmp_append_outbound_varbind(&(request->outbound_pbuf_stream), vb);
        if (err == ERR_OK) {
          /* we stored the exception in varbind -> go on */
          request->error_status = SNMP_ERR_NOERROR;
        } else if (err == ERR_BUF) {
          request->error_status = SNMP_ERR_TOOBIG;
        } else {
          request->error_status = SNMP_ERR_GENERROR;
        }
      }
    } else {
      /* according to RFC 1157/1905, all other errors only return genError */
      request->error_status = SNMP_ERR_GENERROR;
    }
  } else {
    s16_t len = node_instance.get_value(&node_instance, vb->value);
    vb->type = node_instance.asn1_type;

    if (len >= 0) {
      vb->value_len = (u16_t)len; /* cast is OK because we checked >= 0 above */

      LWIP_ASSERT("SNMP_MAX_VALUE_SIZE is configured too low", (vb->value_len & ~SNMP_GET_VALUE_RAW_DATA) <= SNMP_MAX_VALUE_SIZE);
      err = snmp_append_outbound_varbind(&request->outbound_pbuf_stream, vb);

      if (err == ERR_BUF) {
        request->error_status = SNMP_ERR_TOOBIG;
      } else if (err != ERR_OK) {
        request->error_status = SNMP_ERR_GENERROR;
      }
    } else {
      request->error_status = SNMP_ERR_GENERROR;
    }

    if (node_instance.release_instance != NULL) {
      node_instance.release_instance(&node_instance);
    }
  }
}


/**
 * Service an internal or external event for SNMP GET.
 *
 * @param request points to the associated message process state
 */
static err_t
snmp_process_get_request(struct snmp_request *request)
{
  snmp_vb_enumerator_err_t err;
  struct snmp_varbind vb;
  vb.value = request->value_buffer;

  LWIP_DEBUGF(SNMP_DEBUG, ("SNMP get request\n"));

  while (request->error_status == SNMP_ERR_NOERROR) {
    err = snmp_vb_enumerator_get_next(&request->inbound_varbind_enumerator, &vb);
    if (err == SNMP_VB_ENUMERATOR_ERR_OK) {
      if ((vb.type == SNMP_ASN1_TYPE_NULL) && (vb.value_len == 0)) {
        snmp_process_varbind(request, &vb, 0);
      } else {
        request->error_status = SNMP_ERR_GENERROR;
      }
    } else if (err == SNMP_VB_ENUMERATOR_ERR_EOVB) {
      /* no more varbinds in request */
      break;
    } else if (err == SNMP_VB_ENUMERATOR_ERR_ASN1ERROR) {
      /* malformed ASN.1, don't answer */
      return ERR_ARG;
    } else {
      request->error_status = SNMP_ERR_GENERROR;
    }
  }

  return ERR_OK;
}

/**
 * Service an internal or external event for SNMP GET.
 *
 * @param request points to the associated message process state
 */
static err_t
snmp_process_getnext_request(struct snmp_request *request)
{
  snmp_vb_enumerator_err_t err;
  struct snmp_varbind vb;
  vb.value = request->value_buffer;

  LWIP_DEBUGF(SNMP_DEBUG, ("SNMP get-next request\n"));

  while (request->error_status == SNMP_ERR_NOERROR) {
    err = snmp_vb_enumerator_get_next(&request->inbound_varbind_enumerator, &vb);
    if (err == SNMP_VB_ENUMERATOR_ERR_OK) {
      if ((vb.type == SNMP_ASN1_TYPE_NULL) && (vb.value_len == 0)) {
        snmp_process_varbind(request, &vb, 1);
      } else {
        request->error_status = SNMP_ERR_GENERROR;
      }
    } else if (err == SNMP_VB_ENUMERATOR_ERR_EOVB) {
      /* no more varbinds in request */
      break;
    } else if (err == SNMP_VB_ENUMERATOR_ERR_ASN1ERROR) {
      /* malformed ASN.1, don't answer */
      return ERR_ARG;
    } else {
      request->error_status = SNMP_ERR_GENERROR;
    }
  }

  return ERR_OK;
}

/**
 * Service an internal or external event for SNMP GETBULKT.
 *
 * @param request points to the associated message process state
 */
static err_t
snmp_process_getbulk_request(struct snmp_request *request)
{
  snmp_vb_enumerator_err_t err;
  s32_t non_repeaters     = request->non_repeaters;
  s32_t repetitions;
  u16_t repetition_offset = 0;
  struct snmp_varbind_enumerator repetition_varbind_enumerator;
  struct snmp_varbind vb;
  vb.value = request->value_buffer;

  if (SNMP_LWIP_GETBULK_MAX_REPETITIONS > 0) {
    repetitions = LWIP_MIN(request->max_repetitions, SNMP_LWIP_GETBULK_MAX_REPETITIONS);
  } else {
    repetitions = request->max_repetitions;
  }

  LWIP_DEBUGF(SNMP_DEBUG, ("SNMP get-bulk request\n"));

  /* process non repeaters and first repetition */
  while (request->error_status == SNMP_ERR_NOERROR) {
    if (non_repeaters == 0) {
      repetition_offset = request->outbound_pbuf_stream.offset;

      if (repetitions == 0) {
        /* do not resolve repeaters when repetitions is set to 0 */
        break;
      }
      repetitions--;
    }

    err = snmp_vb_enumerator_get_next(&request->inbound_varbind_enumerator, &vb);
    if (err == SNMP_VB_ENUMERATOR_ERR_EOVB) {
      /* no more varbinds in request */
      break;
    } else if (err == SNMP_VB_ENUMERATOR_ERR_ASN1ERROR) {
      /* malformed ASN.1, don't answer */
      return ERR_ARG;
    } else if ((err != SNMP_VB_ENUMERATOR_ERR_OK) || (vb.type != SNMP_ASN1_TYPE_NULL) || (vb.value_len != 0)) {
      request->error_status = SNMP_ERR_GENERROR;
    } else {
      snmp_process_varbind(request, &vb, 1);
      non_repeaters--;
    }
  }

  /* process repetitions > 1 */
  while ((request->error_status == SNMP_ERR_NOERROR) && (repetitions > 0) && (request->outbound_pbuf_stream.offset != repetition_offset)) {

    u8_t all_endofmibview = 1;

    snmp_vb_enumerator_init(&repetition_varbind_enumerator, request->outbound_pbuf, repetition_offset, request->outbound_pbuf_stream.offset - repetition_offset);
    repetition_offset = request->outbound_pbuf_stream.offset; /* for next loop */

    while (request->error_status == SNMP_ERR_NOERROR) {
      vb.value = NULL; /* do NOT decode value (we enumerate outbound buffer here, so all varbinds have values assigned) */
      err = snmp_vb_enumerator_get_next(&repetition_varbind_enumerator, &vb);
      if (err == SNMP_VB_ENUMERATOR_ERR_OK) {
        vb.value = request->value_buffer;
        snmp_process_varbind(request, &vb, 1);

        if (request->error_status != SNMP_ERR_NOERROR) {
          /* already set correct error-index (here it cannot be taken from inbound varbind enumerator) */
          request->error_index = request->non_repeaters + repetition_varbind_enumerator.varbind_count;
        } else if (vb.type != (SNMP_ASN1_CONTENTTYPE_PRIMITIVE | SNMP_ASN1_CLASS_CONTEXT | SNMP_ASN1_CONTEXT_VARBIND_END_OF_MIB_VIEW)) {
          all_endofmibview = 0;
        }
      } else if (err == SNMP_VB_ENUMERATOR_ERR_EOVB) {
        /* no more varbinds in request */
        break;
      } else {
        LWIP_DEBUGF(SNMP_DEBUG, ("Very strange, we cannot parse the varbind output that we created just before!"));
        request->error_status = SNMP_ERR_GENERROR;
        request->error_index  = request->non_repeaters + repetition_varbind_enumerator.varbind_count;
      }
    }

    if ((request->error_status == SNMP_ERR_NOERROR) && all_endofmibview) {
      /* stop when all varbinds in a loop return EndOfMibView */
      break;
    }

    repetitions--;
  }

  if (request->error_status == SNMP_ERR_TOOBIG) {
    /* for GetBulk it is ok, if not all requested variables fit into the response -> just return the varbinds added so far */
    request->error_status = SNMP_ERR_NOERROR;
  }

  return ERR_OK;
}

/**
 * Service an internal or external event for SNMP SET.
 *
 * @param request points to the associated message process state
 */
static err_t
snmp_process_set_request(struct snmp_request *request)
{
  snmp_vb_enumerator_err_t err;
  struct snmp_varbind vb;
  vb.value = request->value_buffer;

  LWIP_DEBUGF(SNMP_DEBUG, ("SNMP set request\n"));

  /* perform set test on all objects */
  while (request->error_status == SNMP_ERR_NOERROR) {
    err = snmp_vb_enumerator_get_next(&request->inbound_varbind_enumerator, &vb);
    if (err == SNMP_VB_ENUMERATOR_ERR_OK) {
      struct snmp_node_instance node_instance;
      memset(&node_instance, 0, sizeof(node_instance));

      request->error_status = snmp_get_node_instance_from_oid(vb.oid.id, vb.oid.len, &node_instance);
      if (request->error_status == SNMP_ERR_NOERROR) {
        if (node_instance.asn1_type != vb.type) {
          request->error_status = SNMP_ERR_WRONGTYPE;
        } else if (((node_instance.access & SNMP_NODE_INSTANCE_ACCESS_WRITE) != SNMP_NODE_INSTANCE_ACCESS_WRITE) || (node_instance.set_value == NULL)) {
          request->error_status = SNMP_ERR_NOTWRITABLE;
        } else {
          if (node_instance.set_test != NULL) {
            request->error_status = node_instance.set_test(&node_instance, vb.value_len, vb.value);
          }
        }

        if (node_instance.release_instance != NULL) {
          node_instance.release_instance(&node_instance);
        }
      }
    } else if (err == SNMP_VB_ENUMERATOR_ERR_EOVB) {
      /* no more varbinds in request */
      break;
    } else if (err == SNMP_VB_ENUMERATOR_ERR_INVALIDLENGTH) {
      request->error_status = SNMP_ERR_WRONGLENGTH;
    } else if (err == SNMP_VB_ENUMERATOR_ERR_ASN1ERROR) {
      /* malformed ASN.1, don't answer */
      return ERR_ARG;
    } else {
      request->error_status = SNMP_ERR_GENERROR;
    }
  }

  /* perform real set operation on all objects */
  if (request->error_status == SNMP_ERR_NOERROR) {
    snmp_vb_enumerator_init(&request->inbound_varbind_enumerator, request->inbound_pbuf, request->inbound_varbind_offset, request->inbound_varbind_len);
    while (request->error_status == SNMP_ERR_NOERROR) {
      err = snmp_vb_enumerator_get_next(&request->inbound_varbind_enumerator, &vb);
      if (err == SNMP_VB_ENUMERATOR_ERR_OK) {
        struct snmp_node_instance node_instance;
        memset(&node_instance, 0, sizeof(node_instance));
        request->error_status = snmp_get_node_instance_from_oid(vb.oid.id, vb.oid.len, &node_instance);
        if (request->error_status == SNMP_ERR_NOERROR) {
          if (node_instance.set_value(&node_instance, vb.value_len, vb.value) != SNMP_ERR_NOERROR) {
            if (request->inbound_varbind_enumerator.varbind_count == 1) {
              request->error_status = SNMP_ERR_COMMITFAILED;
            } else {
              /* we cannot undo the set operations done so far */
              request->error_status = SNMP_ERR_UNDOFAILED;
            }
          }

          if (node_instance.release_instance != NULL) {
            node_instance.release_instance(&node_instance);
          }
        }
      } else if (err == SNMP_VB_ENUMERATOR_ERR_EOVB) {
        /* no more varbinds in request */
        break;
      } else {
        /* first time enumerating varbinds work but second time not, although nothing should have changed in between ??? */
        request->error_status = SNMP_ERR_GENERROR;
      }
    }
  }

  return ERR_OK;
}

#define PARSE_EXEC(code, retValue) \
  if ((code) != ERR_OK) { \
    LWIP_DEBUGF(SNMP_DEBUG, ("Malformed ASN.1 detected.\n")); \
    snmp_stats.inasnparseerrs++; \
    return retValue; \
  }

#define PARSE_ASSERT(cond, retValue) \
  if (!(cond)) { \
    LWIP_DEBUGF(SNMP_DEBUG, ("SNMP parse assertion failed!: " # cond)); \
    snmp_stats.inasnparseerrs++; \
    return retValue; \
  }

#define BUILD_EXEC(code, retValue) \
  if ((code) != ERR_OK) { \
    LWIP_DEBUGF(SNMP_DEBUG, ("SNMP error during creation of outbound frame!: " # code)); \
    return retValue; \
  }

#define IF_PARSE_EXEC(code)   PARSE_EXEC(code, ERR_ARG)
#define IF_PARSE_ASSERT(code) PARSE_ASSERT(code, ERR_ARG)

/**
 * Checks and decodes incoming SNMP message header, logs header errors.
 *
 * @param request points to the current message request state return
 * @return
 * - ERR_OK SNMP header is sane and accepted
 * - ERR_VAL SNMP header is either malformed or rejected
 */
static err_t
snmp_parse_inbound_frame(struct snmp_request *request)
{
  struct snmp_pbuf_stream pbuf_stream;
  struct snmp_asn1_tlv tlv;
  s32_t parent_tlv_value_len;
  s32_t s32_value;
  err_t err;
#if LWIP_SNMP_V3
  snmpv3_auth_algo_t auth;
  snmpv3_priv_algo_t priv;
#endif

  IF_PARSE_EXEC(snmp_pbuf_stream_init(&pbuf_stream, request->inbound_pbuf, 0, request->inbound_pbuf->tot_len));

  /* decode main container consisting of version, community and PDU */
  IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
  IF_PARSE_ASSERT((tlv.type == SNMP_ASN1_TYPE_SEQUENCE) && (tlv.value_len == pbuf_stream.length));
  parent_tlv_value_len = tlv.value_len;

  /* decode version */
  IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
  IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_INTEGER);
  parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
  IF_PARSE_ASSERT(parent_tlv_value_len > 0);

  IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &s32_value));

  if (((s32_value != SNMP_VERSION_1) &&
       (s32_value != SNMP_VERSION_2c)
#if LWIP_SNMP_V3
       && (s32_value != SNMP_VERSION_3)
#endif
      )
#if LWIP_SNMP_CONFIGURE_VERSIONS
      || (!snmp_version_enabled(s32_value))
#endif
     ) {
    /* unsupported SNMP version */
    snmp_stats.inbadversions++;
    return ERR_ARG;
  }
  request->version = (u8_t)s32_value;

#if LWIP_SNMP_V3
  if (request->version == SNMP_VERSION_3) {
    u16_t u16_value;
    u16_t inbound_msgAuthenticationParameters_offset;

    /* SNMPv3 doesn't use communities */
    /* @todo: Differentiate read/write access */
    strncpy((char *)request->community, snmp_community, SNMP_MAX_COMMUNITY_STR_LEN);
    request->community[SNMP_MAX_COMMUNITY_STR_LEN] = 0; /* ensure NULL termination (strncpy does NOT guarantee it!) */
    request->community_strlen = (u16_t)strnlen((char *)request->community, SNMP_MAX_COMMUNITY_STR_LEN);

    /* RFC3414 globalData */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_SEQUENCE);
    parent_tlv_value_len -= SNMP_ASN1_TLV_HDR_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    /* decode msgID */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_INTEGER);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &s32_value));
    request->msg_id = s32_value;

    /* decode msgMaxSize */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_INTEGER);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &s32_value));
    request->msg_max_size = s32_value;

    /* decode msgFlags */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &s32_value));
    request->msg_flags = (u8_t)s32_value;

    /* decode msgSecurityModel */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_INTEGER);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &s32_value));
    request->msg_security_model = s32_value;

    /* RFC3414 msgSecurityParameters
     * The User-based Security Model defines the contents of the OCTET
     * STRING as a SEQUENCE.
     *
     * We skip the protective dummy OCTET STRING header
     * to access the SEQUENCE header.
     */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
    parent_tlv_value_len -= SNMP_ASN1_TLV_HDR_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    /* msgSecurityParameters SEQUENCE header */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_SEQUENCE);
    parent_tlv_value_len -= SNMP_ASN1_TLV_HDR_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    /* decode msgAuthoritativeEngineID */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    IF_PARSE_EXEC(snmp_asn1_dec_raw(&pbuf_stream, tlv.value_len, request->msg_authoritative_engine_id,
                                    &u16_value, SNMP_V3_MAX_ENGINE_ID_LENGTH));
    request->msg_authoritative_engine_id_len = (u8_t)u16_value;

    /* msgAuthoritativeEngineBoots */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_INTEGER);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);
    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &request->msg_authoritative_engine_boots));

    /* msgAuthoritativeEngineTime */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_INTEGER);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);
    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &request->msg_authoritative_engine_time));

    /* msgUserName */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    IF_PARSE_EXEC(snmp_asn1_dec_raw(&pbuf_stream, tlv.value_len, request->msg_user_name,
                                    &u16_value, SNMP_V3_MAX_USER_LENGTH));
    request->msg_user_name_len = (u8_t)u16_value;

    /* msgAuthenticationParameters */
    memset(request->msg_authentication_parameters, 0, SNMP_V3_MAX_AUTH_PARAM_LENGTH);
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);
    /* Remember position */
    inbound_msgAuthenticationParameters_offset = pbuf_stream.offset;
    LWIP_UNUSED_ARG(inbound_msgAuthenticationParameters_offset);
    /* Read auth parameters */
    /* IF_PARSE_ASSERT(tlv.value_len <= SNMP_V3_MAX_AUTH_PARAM_LENGTH); */
    IF_PARSE_EXEC(snmp_asn1_dec_raw(&pbuf_stream, tlv.value_len, request->msg_authentication_parameters,
                                    &u16_value, tlv.value_len));
    request->msg_authentication_parameters_len = (u8_t)u16_value;

    /* msgPrivacyParameters */
    memset(request->msg_privacy_parameters, 0, SNMP_V3_MAX_PRIV_PARAM_LENGTH);
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    IF_PARSE_EXEC(snmp_asn1_dec_raw(&pbuf_stream, tlv.value_len, request->msg_privacy_parameters,
                                    &u16_value, SNMP_V3_MAX_PRIV_PARAM_LENGTH));
    request->msg_privacy_parameters_len = (u8_t)u16_value;

    /* validate securityParameters here (do this after decoding because we don't want to increase other counters for wrong frames)
     * 1) securityParameters was correctly serialized if we reach here.
     * 2) securityParameters are already cached.
     * 3) if msgAuthoritativeEngineID is unknown, zero-length or too long:
         b) https://tools.ietf.org/html/rfc3414#section-7
     */
    {
      const char *eid;
      u8_t eid_len;

      snmpv3_get_engine_id(&eid, &eid_len);

      if ((request->msg_authoritative_engine_id_len == 0) ||
          (request->msg_authoritative_engine_id_len != eid_len) ||
          (memcmp(eid, request->msg_authoritative_engine_id, eid_len) != 0)) {
        snmp_stats.unknownengineids++;
        request->msg_flags = 0; /* noauthnopriv */
        request->error_status = SNMP_ERR_UNKNOWN_ENGINEID;
        return ERR_OK;
      }
    }

    /* 4) verify username */
    if (snmpv3_get_user((char *)request->msg_user_name, &auth, NULL, &priv, NULL)) {
      snmp_stats.unknownusernames++;
      request->msg_flags = 0; /* noauthnopriv */
      request->error_status = SNMP_ERR_UNKNOWN_SECURITYNAME;
      return ERR_OK;
    }

    /* 5) verify security level */
    switch (request->msg_flags & (SNMP_V3_AUTH_FLAG | SNMP_V3_PRIV_FLAG)) {
      case SNMP_V3_NOAUTHNOPRIV:
        if ((auth != SNMP_V3_AUTH_ALGO_INVAL) || (priv != SNMP_V3_PRIV_ALGO_INVAL)) {
          /* Invalid security level for user */
          snmp_stats.unsupportedseclevels++;
          request->msg_flags = SNMP_V3_NOAUTHNOPRIV;
          request->error_status = SNMP_ERR_UNSUPPORTED_SECLEVEL;
          return ERR_OK;
        }
        break;
#if LWIP_SNMP_V3_CRYPTO
      case SNMP_V3_AUTHNOPRIV:
        if ((auth == SNMP_V3_AUTH_ALGO_INVAL) || (priv != SNMP_V3_PRIV_ALGO_INVAL)) {
          /* Invalid security level for user */
          snmp_stats.unsupportedseclevels++;
          request->msg_flags = SNMP_V3_NOAUTHNOPRIV;
          request->error_status = SNMP_ERR_UNSUPPORTED_SECLEVEL;
          return ERR_OK;
        }
        break;
      case SNMP_V3_AUTHPRIV:
        if ((auth == SNMP_V3_AUTH_ALGO_INVAL) || (priv == SNMP_V3_PRIV_ALGO_INVAL)) {
          /* Invalid security level for user */
          snmp_stats.unsupportedseclevels++;
          request->msg_flags = SNMP_V3_NOAUTHNOPRIV;
          request->error_status = SNMP_ERR_UNSUPPORTED_SECLEVEL;
          return ERR_OK;
        }
        break;
#endif
      default:
        snmp_stats.unsupportedseclevels++;
        request->msg_flags = SNMP_V3_NOAUTHNOPRIV;
        request->error_status = SNMP_ERR_UNSUPPORTED_SECLEVEL;
        return ERR_OK;
    }

    /* 6) if securitylevel specifies authentication, authenticate message. */
#if LWIP_SNMP_V3_CRYPTO
    if (request->msg_flags & SNMP_V3_AUTH_FLAG) {
      const u8_t zero_arr[SNMP_V3_MAX_AUTH_PARAM_LENGTH] = { 0 };
      u8_t key[20];
      u8_t hmac[LWIP_MAX(SNMP_V3_SHA_LEN, SNMP_V3_MD5_LEN)];
      struct snmp_pbuf_stream auth_stream;

      if (request->msg_authentication_parameters_len > SNMP_V3_MAX_AUTH_PARAM_LENGTH) {
        snmp_stats.wrongdigests++;
        request->msg_flags = SNMP_V3_NOAUTHNOPRIV;
        request->error_status = SNMP_ERR_AUTHORIZATIONERROR;
        return ERR_OK;
      }

      /* Rewind stream */
      IF_PARSE_EXEC(snmp_pbuf_stream_init(&auth_stream, request->inbound_pbuf, 0, request->inbound_pbuf->tot_len));
      IF_PARSE_EXEC(snmp_pbuf_stream_seek_abs(&auth_stream, inbound_msgAuthenticationParameters_offset));
      /* Set auth parameters to zero for verification */
      IF_PARSE_EXEC(snmp_asn1_enc_raw(&auth_stream, zero_arr, request->msg_authentication_parameters_len));

      /* Verify authentication */
      IF_PARSE_EXEC(snmp_pbuf_stream_init(&auth_stream, request->inbound_pbuf, 0, request->inbound_pbuf->tot_len));

      IF_PARSE_EXEC(snmpv3_get_user((char *)request->msg_user_name, &auth, key, NULL, NULL));
      IF_PARSE_EXEC(snmpv3_auth(&auth_stream, request->inbound_pbuf->tot_len, key, auth, hmac));

      if (memcmp(request->msg_authentication_parameters, hmac, SNMP_V3_MAX_AUTH_PARAM_LENGTH)) {
        snmp_stats.wrongdigests++;
        request->msg_flags = SNMP_V3_NOAUTHNOPRIV;
        request->error_status = SNMP_ERR_AUTHORIZATIONERROR;
        return ERR_OK;
      }

      /* 7) if securitylevel specifies authentication, verify engineboots, enginetime and lastenginetime */
      {
        s32_t boots = snmpv3_get_engine_boots_internal();
        if ((request->msg_authoritative_engine_boots != boots) || (boots == 2147483647UL)) {
          snmp_stats.notintimewindows++;
          request->msg_flags = SNMP_V3_AUTHNOPRIV;
          request->error_status = SNMP_ERR_NOTINTIMEWINDOW;
          return ERR_OK;
        }
      }
      {
        s32_t time = snmpv3_get_engine_time_internal();
        if (request->msg_authoritative_engine_time > time) {
          snmp_stats.notintimewindows++;
          request->msg_flags = SNMP_V3_AUTHNOPRIV;
          request->error_status = SNMP_ERR_NOTINTIMEWINDOW;
          return ERR_OK;
        } else if (time > 150) {
          if (request->msg_authoritative_engine_time < time - 150) {
            snmp_stats.notintimewindows++;
            request->msg_flags = SNMP_V3_AUTHNOPRIV;
            request->error_status = SNMP_ERR_NOTINTIMEWINDOW;
            return ERR_OK;
          }
        }
      }
    }
#endif

    /* 8) if securitylevel specifies privacy, decrypt message. */
#if LWIP_SNMP_V3_CRYPTO
    if (request->msg_flags & SNMP_V3_PRIV_FLAG) {
      /* Decrypt message */

      u8_t key[20];

      IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
      IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
      parent_tlv_value_len -= SNMP_ASN1_TLV_HDR_LENGTH(tlv);
      IF_PARSE_ASSERT(parent_tlv_value_len > 0);

      IF_PARSE_EXEC(snmpv3_get_user((char *)request->msg_user_name, NULL, NULL, &priv, key));
      if (snmpv3_crypt(&pbuf_stream, tlv.value_len, key,
                       request->msg_privacy_parameters, request->msg_authoritative_engine_boots,
                       request->msg_authoritative_engine_time, priv, SNMP_V3_PRIV_MODE_DECRYPT) != ERR_OK) {
        snmp_stats.decryptionerrors++;
        request->msg_flags = SNMP_V3_AUTHNOPRIV;
        request->error_status = SNMP_ERR_DECRYIPTION_ERROR;
        return ERR_OK;
      }
    }
#endif
    /* 9) calculate max size of scoped pdu?
     * 10) securityname for user is retrieved from usertable?
     * 11) security data is cached?
     * 12)
     */

    /* Scoped PDU
     * Encryption context
     */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_SEQUENCE);
    parent_tlv_value_len -= SNMP_ASN1_TLV_HDR_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    /* contextEngineID */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    IF_PARSE_EXEC(snmp_asn1_dec_raw(&pbuf_stream, tlv.value_len, request->context_engine_id,
                                    &u16_value, SNMP_V3_MAX_ENGINE_ID_LENGTH));
    request->context_engine_id_len = (u8_t)u16_value;
    /* TODO: do we need to verify this contextengineid too? */

    /* contextName */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    IF_PARSE_EXEC(snmp_asn1_dec_raw(&pbuf_stream, tlv.value_len, request->context_name,
                                    &u16_value, SNMP_V3_MAX_ENGINE_ID_LENGTH));
    request->context_name_len = (u8_t)u16_value;
    /* TODO: do we need to verify this contextname too? */
  } else
#endif
  {
    /* decode community */
    IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
    IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_OCTET_STRING);
    parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
    IF_PARSE_ASSERT(parent_tlv_value_len > 0);

    err = snmp_asn1_dec_raw(&pbuf_stream, tlv.value_len, request->community, &request->community_strlen, SNMP_MAX_COMMUNITY_STR_LEN);
    if (err == ERR_MEM) {
      /* community string does not fit in our buffer -> its too long -> its invalid */
      request->community_strlen = 0;
      snmp_pbuf_stream_seek(&pbuf_stream, tlv.value_len);
    } else {
      IF_PARSE_ASSERT(err == ERR_OK);
    }
    /* add zero terminator */
    request->community[request->community_strlen] = 0;
  }

  /* decode PDU type (next container level) */
  IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
  IF_PARSE_ASSERT(tlv.value_len <= pbuf_stream.length);
  request->inbound_padding_len = pbuf_stream.length - tlv.value_len;
  parent_tlv_value_len = tlv.value_len;

  /* validate PDU type */
  switch (tlv.type) {
    case (SNMP_ASN1_CLASS_CONTEXT | SNMP_ASN1_CONTENTTYPE_CONSTRUCTED | SNMP_ASN1_CONTEXT_PDU_GET_REQ):
      /* GetRequest PDU */
      snmp_stats.ingetrequests++;
      break;
    case (SNMP_ASN1_CLASS_CONTEXT | SNMP_ASN1_CONTENTTYPE_CONSTRUCTED | SNMP_ASN1_CONTEXT_PDU_GET_NEXT_REQ):
      /* GetNextRequest PDU */
      snmp_stats.ingetnexts++;
      break;
    case (SNMP_ASN1_CLASS_CONTEXT | SNMP_ASN1_CONTENTTYPE_CONSTRUCTED | SNMP_ASN1_CONTEXT_PDU_GET_BULK_REQ):
      /* GetBulkRequest PDU */
      if (request->version < SNMP_VERSION_2c) {
        /* RFC2089: invalid, drop packet */
        return ERR_ARG;
      }
      break;
    case (SNMP_ASN1_CLASS_CONTEXT | SNMP_ASN1_CONTENTTYPE_CONSTRUCTED | SNMP_ASN1_CONTEXT_PDU_SET_REQ):
      /* SetRequest PDU */
      snmp_stats.insetrequests++;
      break;
    default:
      /* unsupported input PDU for this agent (no parse error) */
      LWIP_DEBUGF(SNMP_DEBUG, ("Unknown/Invalid SNMP PDU type received: %d", tlv.type)); \
      return ERR_ARG;
  }
  request->request_type = tlv.type & SNMP_ASN1_DATATYPE_MASK;
  request->request_out_type = (SNMP_ASN1_CLASS_CONTEXT | SNMP_ASN1_CONTENTTYPE_CONSTRUCTED | SNMP_ASN1_CONTEXT_PDU_GET_RESP);

  /* validate community (do this after decoding PDU type because we don't want to increase 'inbadcommunitynames' for wrong frame types */
  if (request->community_strlen == 0) {
    /* community string was too long or really empty*/
    snmp_stats.inbadcommunitynames++;
    snmp_authfail_trap();
    return ERR_ARG;
  } else if (request->request_type == SNMP_ASN1_CONTEXT_PDU_SET_REQ) {
    if (snmp_community_write[0] == 0) {
      /* our write community is empty, that means all our objects are readonly */
      request->error_status = SNMP_ERR_NOTWRITABLE;
      request->error_index  = 1;
    } else if (strncmp(snmp_community_write, (const char *)request->community, SNMP_MAX_COMMUNITY_STR_LEN) != 0) {
      /* community name does not match */
      snmp_stats.inbadcommunitynames++;
      snmp_authfail_trap();
      return ERR_ARG;
    }
  } else {
    if (strncmp(snmp_community, (const char *)request->community, SNMP_MAX_COMMUNITY_STR_LEN) != 0) {
      /* community name does not match */
      snmp_stats.inbadcommunitynames++;
      snmp_authfail_trap();
      return ERR_ARG;
    }
  }

  /* decode request ID */
  IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
  IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_INTEGER);
  parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
  IF_PARSE_ASSERT(parent_tlv_value_len > 0);

  IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &request->request_id));

  /* decode error status / non-repeaters */
  IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
  IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_INTEGER);
  parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
  IF_PARSE_ASSERT(parent_tlv_value_len > 0);

  if (request->request_type == SNMP_ASN1_CONTEXT_PDU_GET_BULK_REQ) {
    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &request->non_repeaters));
    if (request->non_repeaters < 0) {
      /* RFC 1905, 4.2.3 */
      request->non_repeaters = 0;
    }
  } else {
    /* only check valid value, don't touch 'request->error_status', maybe a response error status was already set to above; */
    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &s32_value));
    IF_PARSE_ASSERT(s32_value == SNMP_ERR_NOERROR);
  }

  /* decode error index / max-repetitions */
  IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
  IF_PARSE_ASSERT(tlv.type == SNMP_ASN1_TYPE_INTEGER);
  parent_tlv_value_len -= SNMP_ASN1_TLV_LENGTH(tlv);
  IF_PARSE_ASSERT(parent_tlv_value_len > 0);

  if (request->request_type == SNMP_ASN1_CONTEXT_PDU_GET_BULK_REQ) {
    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &request->max_repetitions));
    if (request->max_repetitions < 0) {
      /* RFC 1905, 4.2.3 */
      request->max_repetitions = 0;
    }
  } else {
    IF_PARSE_EXEC(snmp_asn1_dec_s32t(&pbuf_stream, tlv.value_len, &request->error_index));
    IF_PARSE_ASSERT(s32_value == 0);
  }

  /* decode varbind-list type (next container level) */
  IF_PARSE_EXEC(snmp_asn1_dec_tlv(&pbuf_stream, &tlv));
  IF_PARSE_ASSERT((tlv.type == SNMP_ASN1_TYPE_SEQUENCE) && (tlv.value_len <= pbuf_stream.length));

  request->inbound_varbind_offset = pbuf_stream.offset;
  request->inbound_varbind_len    = pbuf_stream.length - request->inbound_padding_len;
  snmp_vb_enumerator_init(&(request->inbound_varbind_enumerator), request->inbound_pbuf, request->inbound_varbind_offset, request->inbound_varbind_len);

  return ERR_OK;
}

#define OF_BUILD_EXEC(code) BUILD_EXEC(code, ERR_ARG)

static err_t
snmp_prepare_outbound_frame(struct snmp_request *request)
{
  struct snmp_asn1_tlv tlv;
  struct snmp_pbuf_stream *pbuf_stream = &(request->outbound_pbuf_stream);

  /* try allocating pbuf(s) for maximum response size */
  request->outbound_pbuf = pbuf_alloc(PBUF_TRANSPORT, 1472, PBUF_RAM);
  if (request->outbound_pbuf == NULL) {
    return ERR_MEM;
  }

  snmp_pbuf_stream_init(pbuf_stream, request->outbound_pbuf, 0, request->outbound_pbuf->tot_len);

  /* 'Message' sequence */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 3, 0);
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );

  /* version */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 0);
  snmp_asn1_enc_s32t_cnt(request->version, &tlv.value_len);
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  OF_BUILD_EXEC( snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, request->version) );

#if LWIP_SNMP_V3
  if (request->version < SNMP_VERSION_3) {
#endif
    /* community */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, request->community_strlen);
    OF_BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
    OF_BUILD_EXEC( snmp_asn1_enc_raw(pbuf_stream, request->community, request->community_strlen) );
#if LWIP_SNMP_V3
  } else {
    const char *id;

    /* globalData */
    request->outbound_msg_global_data_offset = pbuf_stream->offset;
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 1, 0);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));

    /* msgID */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 1);
    snmp_asn1_enc_s32t_cnt(request->msg_id, &tlv.value_len);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, request->msg_id));

    /* msgMaxSize */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 1);
    snmp_asn1_enc_s32t_cnt(request->msg_max_size, &tlv.value_len);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, request->msg_max_size));

    /* msgFlags */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, 1);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_raw(pbuf_stream, &request->msg_flags, 1));

    /* msgSecurityModel */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 1);
    snmp_asn1_enc_s32t_cnt(request->msg_security_model, &tlv.value_len);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, request->msg_security_model));

    /* end of msgGlobalData */
    request->outbound_msg_global_data_end = pbuf_stream->offset;

    /* msgSecurityParameters */
    request->outbound_msg_security_parameters_str_offset = pbuf_stream->offset;
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 1, 0);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));

    request->outbound_msg_security_parameters_seq_offset = pbuf_stream->offset;
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 1, 0);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));

    /* msgAuthoritativeEngineID */
    snmpv3_get_engine_id(&id, &request->msg_authoritative_engine_id_len);
    MEMCPY(request->msg_authoritative_engine_id, id, request->msg_authoritative_engine_id_len);
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, request->msg_authoritative_engine_id_len);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_raw(pbuf_stream, request->msg_authoritative_engine_id, request->msg_authoritative_engine_id_len));

    request->msg_authoritative_engine_time = snmpv3_get_engine_time();
    request->msg_authoritative_engine_boots = snmpv3_get_engine_boots();

    /* msgAuthoritativeEngineBoots */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 0);
    snmp_asn1_enc_s32t_cnt(request->msg_authoritative_engine_boots, &tlv.value_len);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, request->msg_authoritative_engine_boots));

    /* msgAuthoritativeEngineTime */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 0);
    snmp_asn1_enc_s32t_cnt(request->msg_authoritative_engine_time, &tlv.value_len);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, request->msg_authoritative_engine_time));

    /* msgUserName */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, request->msg_user_name_len);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_raw(pbuf_stream, request->msg_user_name, request->msg_user_name_len));

#if LWIP_SNMP_V3_CRYPTO
    /* msgAuthenticationParameters */
    if (request->msg_flags & SNMP_V3_AUTH_FLAG) {
      memset(request->msg_authentication_parameters, 0, SNMP_V3_MAX_AUTH_PARAM_LENGTH);
      request->outbound_msg_authentication_parameters_offset = pbuf_stream->offset;
      SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 1, SNMP_V3_MAX_AUTH_PARAM_LENGTH);
      OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
      OF_BUILD_EXEC(snmp_asn1_enc_raw(pbuf_stream, request->msg_authentication_parameters, SNMP_V3_MAX_AUTH_PARAM_LENGTH));
    } else
#endif
    {
      SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, 0);
      OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    }

#if LWIP_SNMP_V3_CRYPTO
    /* msgPrivacyParameters */
    if (request->msg_flags & SNMP_V3_PRIV_FLAG) {
      snmpv3_build_priv_param(request->msg_privacy_parameters);

      SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, SNMP_V3_MAX_PRIV_PARAM_LENGTH);
      OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
      OF_BUILD_EXEC(snmp_asn1_enc_raw(pbuf_stream, request->msg_privacy_parameters, SNMP_V3_MAX_PRIV_PARAM_LENGTH));
    } else
#endif
    {
      SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, 0);
      OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
    }

    /* End of msgSecurityParameters, so we can calculate the length of this sequence later */
    request->outbound_msg_security_parameters_end = pbuf_stream->offset;

#if LWIP_SNMP_V3_CRYPTO
    /* For encryption we have to encapsulate the payload in an octet string */
    if (request->msg_flags & SNMP_V3_PRIV_FLAG) {
      request->outbound_scoped_pdu_string_offset = pbuf_stream->offset;
      SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 3, 0);
      OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    }
#endif
    /* Scoped PDU
     * Encryption context
     */
    request->outbound_scoped_pdu_seq_offset = pbuf_stream->offset;
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 3, 0);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));

    /* contextEngineID */
    snmpv3_get_engine_id(&id, &request->context_engine_id_len);
    MEMCPY(request->context_engine_id, id, request->context_engine_id_len);
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, request->context_engine_id_len);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_raw(pbuf_stream, request->context_engine_id, request->context_engine_id_len));

    /* contextName */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 0, request->context_name_len);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_raw(pbuf_stream, request->context_name, request->context_name_len));
  }
#endif

  /* 'PDU' sequence */
  request->outbound_pdu_offset = pbuf_stream->offset;
  SNMP_ASN1_SET_TLV_PARAMS(tlv, request->request_out_type, 3, 0);
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );

  /* request ID */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 0);
  snmp_asn1_enc_s32t_cnt(request->request_id, &tlv.value_len);
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  OF_BUILD_EXEC( snmp_asn1_enc_s32t(pbuf_stream, tlv.value_len, request->request_id) );

  /* error status */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 1);
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  request->outbound_error_status_offset = pbuf_stream->offset;
  OF_BUILD_EXEC( snmp_pbuf_stream_write(pbuf_stream, 0) );

  /* error index */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_INTEGER, 0, 1);
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );
  request->outbound_error_index_offset = pbuf_stream->offset;
  OF_BUILD_EXEC( snmp_pbuf_stream_write(pbuf_stream, 0) );

  /* 'VarBindList' sequence */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 3, 0);
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(pbuf_stream, &tlv) );

  request->outbound_varbind_offset = pbuf_stream->offset;

  return ERR_OK;
}

/** Calculate the length of a varbind list */
err_t
snmp_varbind_length(struct snmp_varbind *varbind, struct snmp_varbind_len *len)
{
  /* calculate required lengths */
  snmp_asn1_enc_oid_cnt(varbind->oid.id, varbind->oid.len, &len->oid_value_len);
  snmp_asn1_enc_length_cnt(len->oid_value_len, &len->oid_len_len);

  if (varbind->value_len == 0) {
    len->value_value_len = 0;
  } else if (varbind->value_len & SNMP_GET_VALUE_RAW_DATA) {
    len->value_value_len = varbind->value_len & (~SNMP_GET_VALUE_RAW_DATA);
  } else {
    switch (varbind->type) {
      case SNMP_ASN1_TYPE_INTEGER:
        if (varbind->value_len != sizeof (s32_t)) {
          return ERR_VAL;
        }
        snmp_asn1_enc_s32t_cnt(*((s32_t *) varbind->value), &len->value_value_len);
        break;
      case SNMP_ASN1_TYPE_COUNTER:
      case SNMP_ASN1_TYPE_GAUGE:
      case SNMP_ASN1_TYPE_TIMETICKS:
        if (varbind->value_len != sizeof (u32_t)) {
          return ERR_VAL;
        }
        snmp_asn1_enc_u32t_cnt(*((u32_t *) varbind->value), &len->value_value_len);
        break;
      case SNMP_ASN1_TYPE_OCTET_STRING:
      case SNMP_ASN1_TYPE_IPADDR:
      case SNMP_ASN1_TYPE_OPAQUE:
        len->value_value_len = varbind->value_len;
        break;
      case SNMP_ASN1_TYPE_NULL:
        if (varbind->value_len != 0) {
          return ERR_VAL;
        }
        len->value_value_len = 0;
        break;
      case SNMP_ASN1_TYPE_OBJECT_ID:
        if ((varbind->value_len & 0x03) != 0) {
          return ERR_VAL;
        }
        snmp_asn1_enc_oid_cnt((u32_t *) varbind->value, varbind->value_len >> 2, &len->value_value_len);
        break;
#if LWIP_HAVE_INT64
      case SNMP_ASN1_TYPE_COUNTER64:
        if (varbind->value_len != sizeof(u64_t)) {
          return ERR_VAL;
        }
        snmp_asn1_enc_u64t_cnt(*(u64_t *)varbind->value, &len->value_value_len);
        break;
#endif
      default:
        /* unsupported type */
        return ERR_VAL;
    }
  }
  snmp_asn1_enc_length_cnt(len->value_value_len, &len->value_len_len);

  len->vb_value_len = 1 + len->oid_len_len + len->oid_value_len + 1 + len->value_len_len + len->value_value_len;
  snmp_asn1_enc_length_cnt(len->vb_value_len, &len->vb_len_len);

  return ERR_OK;
}

#define OVB_BUILD_EXEC(code) BUILD_EXEC(code, ERR_ARG)

err_t
snmp_append_outbound_varbind(struct snmp_pbuf_stream *pbuf_stream, struct snmp_varbind *varbind)
{
  struct snmp_asn1_tlv tlv;
  struct snmp_varbind_len len;
  err_t err;

  err = snmp_varbind_length(varbind, &len);

  if (err != ERR_OK) {
    return err;
  }

  /* check length already before adding first data because in case of GetBulk,
   *  data added so far is returned and therefore no partial data shall be added
   */
  if ((1 + len.vb_len_len + len.vb_value_len) > pbuf_stream->length) {
    return ERR_BUF;
  }

  /* 'VarBind' sequence */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, len.vb_len_len, len.vb_value_len);
  OVB_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));

  /* VarBind OID */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OBJECT_ID, len.oid_len_len, len.oid_value_len);
  OVB_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));
  OVB_BUILD_EXEC(snmp_asn1_enc_oid(pbuf_stream, varbind->oid.id, varbind->oid.len));

  /* VarBind value */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, varbind->type, len.value_len_len, len.value_value_len);
  OVB_BUILD_EXEC(snmp_ans1_enc_tlv(pbuf_stream, &tlv));

  if (len.value_value_len > 0) {
    if (varbind->value_len & SNMP_GET_VALUE_RAW_DATA) {
      OVB_BUILD_EXEC(snmp_asn1_enc_raw(pbuf_stream, (u8_t *) varbind->value, len.value_value_len));
    } else {
      switch (varbind->type) {
        case SNMP_ASN1_TYPE_INTEGER:
          OVB_BUILD_EXEC(snmp_asn1_enc_s32t(pbuf_stream, len.value_value_len, *((s32_t *) varbind->value)));
          break;
        case SNMP_ASN1_TYPE_COUNTER:
        case SNMP_ASN1_TYPE_GAUGE:
        case SNMP_ASN1_TYPE_TIMETICKS:
          OVB_BUILD_EXEC(snmp_asn1_enc_u32t(pbuf_stream, len.value_value_len, *((u32_t *) varbind->value)));
          break;
        case SNMP_ASN1_TYPE_OCTET_STRING:
        case SNMP_ASN1_TYPE_IPADDR:
        case SNMP_ASN1_TYPE_OPAQUE:
          OVB_BUILD_EXEC(snmp_asn1_enc_raw(pbuf_stream, (u8_t *) varbind->value, len.value_value_len));
          len.value_value_len = varbind->value_len;
          break;
        case SNMP_ASN1_TYPE_OBJECT_ID:
          OVB_BUILD_EXEC(snmp_asn1_enc_oid(pbuf_stream, (u32_t *) varbind->value, varbind->value_len / sizeof (u32_t)));
          break;
#if LWIP_HAVE_INT64
        case SNMP_ASN1_TYPE_COUNTER64:
          OVB_BUILD_EXEC(snmp_asn1_enc_u64t(pbuf_stream, len.value_value_len, *(u64_t *) varbind->value));
          break;
#endif
        default:
          LWIP_ASSERT("Unknown variable type", 0);
          break;
      }
    }
  }

  return ERR_OK;
}

static err_t
snmp_complete_outbound_frame(struct snmp_request *request)
{
  struct snmp_asn1_tlv tlv;
  u16_t frame_size;
  u8_t outbound_padding = 0;

  if (request->version == SNMP_VERSION_1) {
    if (request->error_status != SNMP_ERR_NOERROR) {
      /* map v2c error codes to v1 compliant error code (according to RFC 2089) */
      switch (request->error_status) {
        /* mapping of implementation specific "virtual" error codes
         * (during processing of frame we already stored them in error_status field,
         * so no need to check all varbinds here for those exceptions as suggested by RFC) */
        case SNMP_ERR_NOSUCHINSTANCE:
        case SNMP_ERR_NOSUCHOBJECT:
        case SNMP_ERR_ENDOFMIBVIEW:
          request->error_status = SNMP_ERR_NOSUCHNAME;
          break;
        /* mapping according to RFC */
        case SNMP_ERR_WRONGVALUE:
        case SNMP_ERR_WRONGENCODING:
        case SNMP_ERR_WRONGTYPE:
        case SNMP_ERR_WRONGLENGTH:
        case SNMP_ERR_INCONSISTENTVALUE:
          request->error_status = SNMP_ERR_BADVALUE;
          break;
        case SNMP_ERR_NOACCESS:
        case SNMP_ERR_NOTWRITABLE:
        case SNMP_ERR_NOCREATION:
        case SNMP_ERR_INCONSISTENTNAME:
        case SNMP_ERR_AUTHORIZATIONERROR:
          request->error_status = SNMP_ERR_NOSUCHNAME;
          break;
        case SNMP_ERR_RESOURCEUNAVAILABLE:
        case SNMP_ERR_COMMITFAILED:
        case SNMP_ERR_UNDOFAILED:
        default:
          request->error_status = SNMP_ERR_GENERROR;
          break;
      }
    }
  } else {
    if (request->request_type == SNMP_ASN1_CONTEXT_PDU_SET_REQ) {
      /* map error codes to according to RFC 1905 (4.2.5.  The SetRequest-PDU) return 'NotWritable' for unknown OIDs) */
      switch (request->error_status) {
        case SNMP_ERR_NOSUCHINSTANCE:
        case SNMP_ERR_NOSUCHOBJECT:
        case SNMP_ERR_ENDOFMIBVIEW:
          request->error_status = SNMP_ERR_NOTWRITABLE;
          break;
        default:
          break;
      }
    }

    if (request->error_status >= SNMP_VARBIND_EXCEPTION_OFFSET) {
      /* should never occur because v2 frames store exceptions directly inside varbinds and not as frame error_status */
      LWIP_DEBUGF(SNMP_DEBUG, ("snmp_complete_outbound_frame() > Found v2 request with varbind exception code stored as error status!\n"));
      return ERR_ARG;
    }
  }

  if ((request->error_status != SNMP_ERR_NOERROR) || (request->request_type == SNMP_ASN1_CONTEXT_PDU_SET_REQ)) {
    /* all inbound vars are returned in response without any modification for error responses and successful set requests*/
    struct snmp_pbuf_stream inbound_stream;
    OF_BUILD_EXEC( snmp_pbuf_stream_init(&inbound_stream, request->inbound_pbuf, request->inbound_varbind_offset, request->inbound_varbind_len) );
    OF_BUILD_EXEC( snmp_pbuf_stream_init(&(request->outbound_pbuf_stream), request->outbound_pbuf, request->outbound_varbind_offset, request->outbound_pbuf->tot_len - request->outbound_varbind_offset) );
    OF_BUILD_EXEC( snmp_pbuf_stream_writeto(&inbound_stream, &(request->outbound_pbuf_stream), 0) );
  }

  frame_size = request->outbound_pbuf_stream.offset;

#if LWIP_SNMP_V3 && LWIP_SNMP_V3_CRYPTO
  /* Calculate padding for encryption */
  if (request->version == SNMP_VERSION_3 && (request->msg_flags & SNMP_V3_PRIV_FLAG)) {
    u8_t i;
    outbound_padding = (8 - (u8_t)((frame_size - request->outbound_scoped_pdu_seq_offset) & 0x07)) & 0x07;
    for (i = 0; i < outbound_padding; i++) {
      OF_BUILD_EXEC( snmp_pbuf_stream_write(&request->outbound_pbuf_stream, 0) );
    }
  }
#endif

  /* complete missing length in 'Message' sequence ; 'Message' tlv is located at the beginning (offset 0) */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 3, frame_size + outbound_padding - 1 - 3); /* - type - length_len(fixed, see snmp_prepare_outbound_frame()) */
  OF_BUILD_EXEC( snmp_pbuf_stream_init(&(request->outbound_pbuf_stream), request->outbound_pbuf, 0, request->outbound_pbuf->tot_len) );
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(&(request->outbound_pbuf_stream), &tlv) );

#if LWIP_SNMP_V3
  if (request->version == SNMP_VERSION_3) {
    /* complete missing length in 'globalData' sequence */
    /* - type - length_len(fixed, see snmp_prepare_outbound_frame()) */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 1, request->outbound_msg_global_data_end
                             - request->outbound_msg_global_data_offset - 1 - 1);
    OF_BUILD_EXEC(snmp_pbuf_stream_seek_abs(&(request->outbound_pbuf_stream), request->outbound_msg_global_data_offset));
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(&(request->outbound_pbuf_stream), &tlv));

    /* complete missing length in 'msgSecurityParameters' sequence */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 1, request->outbound_msg_security_parameters_end
                             - request->outbound_msg_security_parameters_str_offset - 1 - 1);
    OF_BUILD_EXEC(snmp_pbuf_stream_seek_abs(&(request->outbound_pbuf_stream), request->outbound_msg_security_parameters_str_offset));
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(&(request->outbound_pbuf_stream), &tlv));

    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 1, request->outbound_msg_security_parameters_end
                             - request->outbound_msg_security_parameters_seq_offset - 1 - 1);
    OF_BUILD_EXEC(snmp_pbuf_stream_seek_abs(&(request->outbound_pbuf_stream), request->outbound_msg_security_parameters_seq_offset));
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(&(request->outbound_pbuf_stream), &tlv));

    /* complete missing length in scoped PDU sequence */
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 3, frame_size - request->outbound_scoped_pdu_seq_offset - 1 - 3);
    OF_BUILD_EXEC(snmp_pbuf_stream_seek_abs(&(request->outbound_pbuf_stream), request->outbound_scoped_pdu_seq_offset));
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(&(request->outbound_pbuf_stream), &tlv));
  }
#endif

  /* complete missing length in 'PDU' sequence */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, request->request_out_type, 3,
                           frame_size - request->outbound_pdu_offset - 1 - 3); /* - type - length_len(fixed, see snmp_prepare_outbound_frame()) */
  OF_BUILD_EXEC( snmp_pbuf_stream_seek_abs(&(request->outbound_pbuf_stream), request->outbound_pdu_offset) );
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(&(request->outbound_pbuf_stream), &tlv) );

  /* process and encode final error status */
  if (request->error_status != 0) {
    u16_t len;
    snmp_asn1_enc_s32t_cnt(request->error_status, &len);
    if (len != 1) {
      /* error, we only reserved one byte for it */
      return ERR_ARG;
    }
    OF_BUILD_EXEC( snmp_pbuf_stream_seek_abs(&(request->outbound_pbuf_stream), request->outbound_error_status_offset) );
    OF_BUILD_EXEC( snmp_asn1_enc_s32t(&(request->outbound_pbuf_stream), len, request->error_status) );

    /* for compatibility to v1, log statistics; in v2 (RFC 1907) these statistics are obsoleted */
    switch (request->error_status) {
      case SNMP_ERR_TOOBIG:
        snmp_stats.outtoobigs++;
        break;
      case SNMP_ERR_NOSUCHNAME:
        snmp_stats.outnosuchnames++;
        break;
      case SNMP_ERR_BADVALUE:
        snmp_stats.outbadvalues++;
        break;
      case SNMP_ERR_GENERROR:
      default:
        snmp_stats.outgenerrs++;
        break;
    }

    if (request->error_status == SNMP_ERR_TOOBIG) {
      request->error_index = 0; /* defined by RFC 1157 */
    } else if (request->error_index == 0) {
      /* set index to varbind where error occured (if not already set before, e.g. during GetBulk processing) */
      request->error_index = request->inbound_varbind_enumerator.varbind_count;
    }
  } else {
    if (request->request_type == SNMP_ASN1_CONTEXT_PDU_SET_REQ) {
      snmp_stats.intotalsetvars += request->inbound_varbind_enumerator.varbind_count;
    } else {
      snmp_stats.intotalreqvars += request->inbound_varbind_enumerator.varbind_count;
    }
  }

  /* encode final error index*/
  if (request->error_index != 0) {
    u16_t len;
    snmp_asn1_enc_s32t_cnt(request->error_index, &len);
    if (len != 1) {
      /* error, we only reserved one byte for it */
      return ERR_VAL;
    }
    OF_BUILD_EXEC( snmp_pbuf_stream_seek_abs(&(request->outbound_pbuf_stream), request->outbound_error_index_offset) );
    OF_BUILD_EXEC( snmp_asn1_enc_s32t(&(request->outbound_pbuf_stream), len, request->error_index) );
  }

  /* complete missing length in 'VarBindList' sequence ; 'VarBindList' tlv is located directly before varbind offset */
  SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_SEQUENCE, 3, frame_size - request->outbound_varbind_offset);
  OF_BUILD_EXEC( snmp_pbuf_stream_seek_abs(&(request->outbound_pbuf_stream), request->outbound_varbind_offset - 1 - 3) ); /* - type - length_len(fixed, see snmp_prepare_outbound_frame()) */
  OF_BUILD_EXEC( snmp_ans1_enc_tlv(&(request->outbound_pbuf_stream), &tlv) );

  /* Authenticate response */
#if LWIP_SNMP_V3 && LWIP_SNMP_V3_CRYPTO
  /* Encrypt response */
  if (request->version == SNMP_VERSION_3 && (request->msg_flags & SNMP_V3_PRIV_FLAG)) {
    u8_t key[20];
    snmpv3_priv_algo_t algo;

    /* complete missing length in PDU sequence */
    OF_BUILD_EXEC(snmp_pbuf_stream_init(&request->outbound_pbuf_stream, request->outbound_pbuf, 0, request->outbound_pbuf->tot_len));
    OF_BUILD_EXEC(snmp_pbuf_stream_seek_abs(&(request->outbound_pbuf_stream), request->outbound_scoped_pdu_string_offset));
    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 3, frame_size + outbound_padding
                             - request->outbound_scoped_pdu_string_offset - 1 - 3);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(&(request->outbound_pbuf_stream), &tlv));

    OF_BUILD_EXEC(snmpv3_get_user((char *)request->msg_user_name, NULL, NULL, &algo, key));

    OF_BUILD_EXEC(snmpv3_crypt(&request->outbound_pbuf_stream, tlv.value_len, key,
                               request->msg_privacy_parameters, request->msg_authoritative_engine_boots,
                               request->msg_authoritative_engine_time, algo, SNMP_V3_PRIV_MODE_ENCRYPT));
  }

  if (request->version == SNMP_VERSION_3 && (request->msg_flags & SNMP_V3_AUTH_FLAG)) {
    u8_t key[20];
    snmpv3_auth_algo_t algo;
    u8_t hmac[20];

    OF_BUILD_EXEC(snmpv3_get_user((char *)request->msg_user_name, &algo, key, NULL, NULL));
    OF_BUILD_EXEC(snmp_pbuf_stream_init(&(request->outbound_pbuf_stream),
                                        request->outbound_pbuf, 0, request->outbound_pbuf->tot_len));
    OF_BUILD_EXEC(snmpv3_auth(&request->outbound_pbuf_stream, frame_size + outbound_padding, key, algo, hmac));

    MEMCPY(request->msg_authentication_parameters, hmac, SNMP_V3_MAX_AUTH_PARAM_LENGTH);
    OF_BUILD_EXEC(snmp_pbuf_stream_init(&request->outbound_pbuf_stream,
                                        request->outbound_pbuf, 0, request->outbound_pbuf->tot_len));
    OF_BUILD_EXEC(snmp_pbuf_stream_seek_abs(&request->outbound_pbuf_stream,
                                            request->outbound_msg_authentication_parameters_offset));

    SNMP_ASN1_SET_TLV_PARAMS(tlv, SNMP_ASN1_TYPE_OCTET_STRING, 1, SNMP_V3_MAX_AUTH_PARAM_LENGTH);
    OF_BUILD_EXEC(snmp_ans1_enc_tlv(&request->outbound_pbuf_stream, &tlv));
    OF_BUILD_EXEC(snmp_asn1_enc_raw(&request->outbound_pbuf_stream,
                                    request->msg_authentication_parameters, SNMP_V3_MAX_AUTH_PARAM_LENGTH));
  }
#endif

  pbuf_realloc(request->outbound_pbuf, frame_size + outbound_padding);

  snmp_stats.outgetresponses++;
  snmp_stats.outpkts++;

  return ERR_OK;
}

static void
snmp_execute_write_callbacks(struct snmp_request *request)
{
  struct snmp_varbind_enumerator inbound_varbind_enumerator;
  struct snmp_varbind vb;

  snmp_vb_enumerator_init(&inbound_varbind_enumerator, request->inbound_pbuf, request->inbound_varbind_offset, request->inbound_varbind_len);
  vb.value = NULL; /* do NOT decode value (we enumerate outbound buffer here, so all varbinds have values assigned, which we don't need here) */

  while (snmp_vb_enumerator_get_next(&inbound_varbind_enumerator, &vb) == SNMP_VB_ENUMERATOR_ERR_OK) {
    snmp_write_callback(vb.oid.id, vb.oid.len, snmp_write_callback_arg);
  }
}


/* ----------------------------------------------------------------------- */
/* VarBind enumerator methods */
/* ----------------------------------------------------------------------- */

void
snmp_vb_enumerator_init(struct snmp_varbind_enumerator *enumerator, struct pbuf *p, u16_t offset, u16_t length)
{
  snmp_pbuf_stream_init(&(enumerator->pbuf_stream), p, offset, length);
  enumerator->varbind_count = 0;
}

#define VB_PARSE_EXEC(code)   PARSE_EXEC(code, SNMP_VB_ENUMERATOR_ERR_ASN1ERROR)
#define VB_PARSE_ASSERT(code) PARSE_ASSERT(code, SNMP_VB_ENUMERATOR_ERR_ASN1ERROR)

snmp_vb_enumerator_err_t
snmp_vb_enumerator_get_next(struct snmp_varbind_enumerator *enumerator, struct snmp_varbind *varbind)
{
  struct snmp_asn1_tlv tlv;
  u16_t  varbind_len;
  err_t  err;

  if (enumerator->pbuf_stream.length == 0) {
    return SNMP_VB_ENUMERATOR_ERR_EOVB;
  }
  enumerator->varbind_count++;

  /* decode varbind itself (parent container of a varbind) */
  VB_PARSE_EXEC(snmp_asn1_dec_tlv(&(enumerator->pbuf_stream), &tlv));
  VB_PARSE_ASSERT((tlv.type == SNMP_ASN1_TYPE_SEQUENCE) && (tlv.value_len <= enumerator->pbuf_stream.length));
  varbind_len = tlv.value_len;

  /* decode varbind name (object id) */
  VB_PARSE_EXEC(snmp_asn1_dec_tlv(&(enumerator->pbuf_stream), &tlv));
  VB_PARSE_ASSERT((tlv.type == SNMP_ASN1_TYPE_OBJECT_ID) && (SNMP_ASN1_TLV_LENGTH(tlv) < varbind_len) && (tlv.value_len < enumerator->pbuf_stream.length));

  VB_PARSE_EXEC(snmp_asn1_dec_oid(&(enumerator->pbuf_stream), tlv.value_len, varbind->oid.id, &(varbind->oid.len), SNMP_MAX_OBJ_ID_LEN));
  varbind_len -= SNMP_ASN1_TLV_LENGTH(tlv);

  /* decode varbind value (object id) */
  VB_PARSE_EXEC(snmp_asn1_dec_tlv(&(enumerator->pbuf_stream), &tlv));
  VB_PARSE_ASSERT((SNMP_ASN1_TLV_LENGTH(tlv) == varbind_len) && (tlv.value_len <= enumerator->pbuf_stream.length));
  varbind->type = tlv.type;

  /* shall the value be decoded ? */
  if (varbind->value != NULL) {
    switch (varbind->type) {
      case SNMP_ASN1_TYPE_INTEGER:
        VB_PARSE_EXEC(snmp_asn1_dec_s32t(&(enumerator->pbuf_stream), tlv.value_len, (s32_t *)varbind->value));
        varbind->value_len = sizeof(s32_t);
        break;
      case SNMP_ASN1_TYPE_COUNTER:
      case SNMP_ASN1_TYPE_GAUGE:
      case SNMP_ASN1_TYPE_TIMETICKS:
        VB_PARSE_EXEC(snmp_asn1_dec_u32t(&(enumerator->pbuf_stream), tlv.value_len, (u32_t *)varbind->value));
        varbind->value_len = sizeof(u32_t);
        break;
      case SNMP_ASN1_TYPE_OCTET_STRING:
      case SNMP_ASN1_TYPE_OPAQUE:
        err = snmp_asn1_dec_raw(&(enumerator->pbuf_stream), tlv.value_len, (u8_t *)varbind->value, &varbind->value_len, SNMP_MAX_VALUE_SIZE);
        if (err == ERR_MEM) {
          return SNMP_VB_ENUMERATOR_ERR_INVALIDLENGTH;
        }
        VB_PARSE_ASSERT(err == ERR_OK);
        break;
      case SNMP_ASN1_TYPE_NULL:
        varbind->value_len = 0;
        break;
      case SNMP_ASN1_TYPE_OBJECT_ID:
        /* misuse tlv.length_len as OID_length transporter */
        err = snmp_asn1_dec_oid(&(enumerator->pbuf_stream), tlv.value_len, (u32_t *)varbind->value, &tlv.length_len, SNMP_MAX_OBJ_ID_LEN);
        if (err == ERR_MEM) {
          return SNMP_VB_ENUMERATOR_ERR_INVALIDLENGTH;
        }
        VB_PARSE_ASSERT(err == ERR_OK);
        varbind->value_len = tlv.length_len * sizeof(u32_t);
        break;
      case SNMP_ASN1_TYPE_IPADDR:
        if (tlv.value_len == 4) {
          /* must be exactly 4 octets! */
          VB_PARSE_EXEC(snmp_asn1_dec_raw(&(enumerator->pbuf_stream), tlv.value_len, (u8_t *)varbind->value, &varbind->value_len, SNMP_MAX_VALUE_SIZE));
        } else {
          VB_PARSE_ASSERT(0);
        }
        break;
#if LWIP_HAVE_INT64
      case SNMP_ASN1_TYPE_COUNTER64:
        VB_PARSE_EXEC(snmp_asn1_dec_u64t(&(enumerator->pbuf_stream), tlv.value_len, (u64_t *)varbind->value));
        varbind->value_len = sizeof(u64_t);
        break;
#endif
      default:
        VB_PARSE_ASSERT(0);
        break;
    }
  } else {
    snmp_pbuf_stream_seek(&(enumerator->pbuf_stream), tlv.value_len);
    varbind->value_len = tlv.value_len;
  }

  return SNMP_VB_ENUMERATOR_ERR_OK;
}

#endif /* LWIP_SNMP */
