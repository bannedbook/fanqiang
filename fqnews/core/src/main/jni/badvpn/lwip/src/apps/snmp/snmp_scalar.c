/**
 * @file
 * SNMP scalar node support implementation.
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
 * Author: Martin Hentschel <info@cl-soft.de>
 *
 */

#include "lwip/apps/snmp_opts.h"

#if LWIP_SNMP /* don't build if not configured for use in lwipopts.h */

#include "lwip/apps/snmp_scalar.h"
#include "lwip/apps/snmp_core.h"

static s16_t snmp_scalar_array_get_value(struct snmp_node_instance *instance, void *value);
static snmp_err_t  snmp_scalar_array_set_test(struct snmp_node_instance *instance, u16_t value_len, void *value);
static snmp_err_t  snmp_scalar_array_set_value(struct snmp_node_instance *instance, u16_t value_len, void *value);

snmp_err_t
snmp_scalar_get_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance *instance)
{
  const struct snmp_scalar_node *scalar_node = (const struct snmp_scalar_node *)(const void *)instance->node;

  LWIP_UNUSED_ARG(root_oid);
  LWIP_UNUSED_ARG(root_oid_len);

  /* scalar only has one dedicated instance: .0 */
  if ((instance->instance_oid.len != 1) || (instance->instance_oid.id[0] != 0)) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  instance->access    = scalar_node->access;
  instance->asn1_type = scalar_node->asn1_type;
  instance->get_value = scalar_node->get_value;
  instance->set_test  = scalar_node->set_test;
  instance->set_value = scalar_node->set_value;
  return SNMP_ERR_NOERROR;
}

snmp_err_t
snmp_scalar_get_next_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance *instance)
{
  /* because our only instance is .0 we can only return a next instance if no instance oid is passed */
  if (instance->instance_oid.len == 0) {
    instance->instance_oid.len   = 1;
    instance->instance_oid.id[0] = 0;

    return snmp_scalar_get_instance(root_oid, root_oid_len, instance);
  }

  return SNMP_ERR_NOSUCHINSTANCE;
}


snmp_err_t
snmp_scalar_array_get_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance *instance)
{
  LWIP_UNUSED_ARG(root_oid);
  LWIP_UNUSED_ARG(root_oid_len);

  if ((instance->instance_oid.len == 2) && (instance->instance_oid.id[1] == 0)) {
    const struct snmp_scalar_array_node *array_node = (const struct snmp_scalar_array_node *)(const void *)instance->node;
    const struct snmp_scalar_array_node_def *array_node_def = array_node->array_nodes;
    u32_t i = 0;

    while (i < array_node->array_node_count) {
      if (array_node_def->oid == instance->instance_oid.id[0]) {
        break;
      }

      array_node_def++;
      i++;
    }

    if (i < array_node->array_node_count) {
      instance->access              = array_node_def->access;
      instance->asn1_type           = array_node_def->asn1_type;
      instance->get_value           = snmp_scalar_array_get_value;
      instance->set_test            = snmp_scalar_array_set_test;
      instance->set_value           = snmp_scalar_array_set_value;
      instance->reference.const_ptr = array_node_def;

      return SNMP_ERR_NOERROR;
    }
  }

  return SNMP_ERR_NOSUCHINSTANCE;
}

snmp_err_t
snmp_scalar_array_get_next_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance *instance)
{
  const struct snmp_scalar_array_node *array_node = (const struct snmp_scalar_array_node *)(const void *)instance->node;
  const struct snmp_scalar_array_node_def *array_node_def = array_node->array_nodes;
  const struct snmp_scalar_array_node_def *result = NULL;

  LWIP_UNUSED_ARG(root_oid);
  LWIP_UNUSED_ARG(root_oid_len);

  if ((instance->instance_oid.len == 0) && (array_node->array_node_count > 0)) {
    /* return node with lowest OID */
    u16_t i = 0;

    result = array_node_def;
    array_node_def++;

    for (i = 1; i < array_node->array_node_count; i++) {
      if (array_node_def->oid < result->oid) {
        result = array_node_def;
      }
      array_node_def++;
    }
  } else if (instance->instance_oid.len >= 1) {
    if (instance->instance_oid.len == 1) {
      /* if we have the requested OID we return its instance, otherwise we search for the next available */
      u16_t i = 0;
      while (i < array_node->array_node_count) {
        if (array_node_def->oid == instance->instance_oid.id[0]) {
          result = array_node_def;
          break;
        }

        array_node_def++;
        i++;
      }
    }
    if (result == NULL) {
      u32_t oid_dist = 0xFFFFFFFFUL;
      u16_t i        = 0;
      array_node_def = array_node->array_nodes; /* may be already at the end when if case before was executed without result -> reinitialize to start */
      while (i < array_node->array_node_count) {
        if ((array_node_def->oid > instance->instance_oid.id[0]) &&
            ((u32_t)(array_node_def->oid - instance->instance_oid.id[0]) < oid_dist)) {
          result   = array_node_def;
          oid_dist = array_node_def->oid - instance->instance_oid.id[0];
        }

        array_node_def++;
        i++;
      }
    }
  }

  if (result == NULL) {
    /* nothing to return */
    return SNMP_ERR_NOSUCHINSTANCE;
  }

  instance->instance_oid.len   = 2;
  instance->instance_oid.id[0] = result->oid;
  instance->instance_oid.id[1] = 0;

  instance->access              = result->access;
  instance->asn1_type           = result->asn1_type;
  instance->get_value           = snmp_scalar_array_get_value;
  instance->set_test            = snmp_scalar_array_set_test;
  instance->set_value           = snmp_scalar_array_set_value;
  instance->reference.const_ptr = result;

  return SNMP_ERR_NOERROR;
}

static s16_t
snmp_scalar_array_get_value(struct snmp_node_instance *instance, void *value)
{
  const struct snmp_scalar_array_node *array_node = (const struct snmp_scalar_array_node *)(const void *)instance->node;
  const struct snmp_scalar_array_node_def *array_node_def = (const struct snmp_scalar_array_node_def *)instance->reference.const_ptr;

  return array_node->get_value(array_node_def, value);
}

static snmp_err_t
snmp_scalar_array_set_test(struct snmp_node_instance *instance, u16_t value_len, void *value)
{
  const struct snmp_scalar_array_node *array_node = (const struct snmp_scalar_array_node *)(const void *)instance->node;
  const struct snmp_scalar_array_node_def *array_node_def = (const struct snmp_scalar_array_node_def *)instance->reference.const_ptr;

  return array_node->set_test(array_node_def, value_len, value);
}

static snmp_err_t
snmp_scalar_array_set_value(struct snmp_node_instance *instance, u16_t value_len, void *value)
{
  const struct snmp_scalar_array_node *array_node = (const struct snmp_scalar_array_node *)(const void *)instance->node;
  const struct snmp_scalar_array_node_def *array_node_def = (const struct snmp_scalar_array_node_def *)instance->reference.const_ptr;

  return array_node->set_value(array_node_def, value_len, value);
}

#endif /* LWIP_SNMP */
