/**
 * @file
 * SNMP table support implementation.
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

#include "lwip/apps/snmp_core.h"
#include "lwip/apps/snmp_table.h"
#include <string.h>

snmp_err_t snmp_table_get_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance *instance)
{
  snmp_err_t ret = SNMP_ERR_NOSUCHINSTANCE;
  const struct snmp_table_node *table_node = (const struct snmp_table_node *)(const void *)instance->node;

  LWIP_UNUSED_ARG(root_oid);
  LWIP_UNUSED_ARG(root_oid_len);

  /* check min. length (fixed row entry definition, column, row instance oid with at least one entry */
  /* fixed row entry always has oid 1 */
  if ((instance->instance_oid.len >= 3) && (instance->instance_oid.id[0] == 1)) {
    /* search column */
    const struct snmp_table_col_def *col_def = table_node->columns;
    u16_t i = table_node->column_count;
    while (i > 0) {
      if (col_def->index == instance->instance_oid.id[1]) {
        break;
      }

      col_def++;
      i--;
    }

    if (i > 0) {
      /* everything may be overwritten by get_cell_instance_method() in order to implement special handling for single columns/cells */
      instance->asn1_type = col_def->asn1_type;
      instance->access    = col_def->access;
      instance->get_value = table_node->get_value;
      instance->set_test  = table_node->set_test;
      instance->set_value = table_node->set_value;

      ret = table_node->get_cell_instance(
              &(instance->instance_oid.id[1]),
              &(instance->instance_oid.id[2]),
              instance->instance_oid.len - 2,
              instance);
    }
  }

  return ret;
}

snmp_err_t snmp_table_get_next_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance *instance)
{
  const struct snmp_table_node *table_node = (const struct snmp_table_node *)(const void *)instance->node;
  const struct snmp_table_col_def *col_def;
  struct snmp_obj_id row_oid;
  u32_t column = 0;
  snmp_err_t result;

  LWIP_UNUSED_ARG(root_oid);
  LWIP_UNUSED_ARG(root_oid_len);

  /* check that first part of id is 0 or 1, referencing fixed row entry */
  if ((instance->instance_oid.len > 0) && (instance->instance_oid.id[0] > 1)) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }
  if (instance->instance_oid.len > 1) {
    column = instance->instance_oid.id[1];
  }
  if (instance->instance_oid.len > 2) {
    snmp_oid_assign(&row_oid, &(instance->instance_oid.id[2]), instance->instance_oid.len - 2);
  } else {
    row_oid.len = 0;
  }

  instance->get_value    = table_node->get_value;
  instance->set_test     = table_node->set_test;
  instance->set_value    = table_node->set_value;

  /* resolve column and value */
  do {
    u16_t i;
    const struct snmp_table_col_def *next_col_def = NULL;
    col_def = table_node->columns;

    for (i = 0; i < table_node->column_count; i++) {
      if (col_def->index == column) {
        next_col_def = col_def;
        break;
      } else if ((col_def->index > column) && ((next_col_def == NULL) || (col_def->index < next_col_def->index))) {
        next_col_def = col_def;
      }
      col_def++;
    }

    if (next_col_def == NULL) {
      /* no further column found */
      return SNMP_ERR_NOSUCHINSTANCE;
    }

    instance->asn1_type          = next_col_def->asn1_type;
    instance->access             = next_col_def->access;

    result = table_node->get_next_cell_instance(
               &next_col_def->index,
               &row_oid,
               instance);

    if (result == SNMP_ERR_NOERROR) {
      col_def = next_col_def;
      break;
    }

    row_oid.len = 0; /* reset row_oid because we switch to next column and start with the first entry there */
    column = next_col_def->index + 1;
  } while (1);

  /* build resulting oid */
  instance->instance_oid.len   = 2;
  instance->instance_oid.id[0] = 1;
  instance->instance_oid.id[1] = col_def->index;
  snmp_oid_append(&instance->instance_oid, row_oid.id, row_oid.len);

  return SNMP_ERR_NOERROR;
}


snmp_err_t snmp_table_simple_get_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance *instance)
{
  snmp_err_t ret = SNMP_ERR_NOSUCHINSTANCE;
  const struct snmp_table_simple_node *table_node = (const struct snmp_table_simple_node *)(const void *)instance->node;

  LWIP_UNUSED_ARG(root_oid);
  LWIP_UNUSED_ARG(root_oid_len);

  /* check min. length (fixed row entry definition, column, row instance oid with at least one entry */
  /* fixed row entry always has oid 1 */
  if ((instance->instance_oid.len >= 3) && (instance->instance_oid.id[0] == 1)) {
    ret = table_node->get_cell_value(
            &(instance->instance_oid.id[1]),
            &(instance->instance_oid.id[2]),
            instance->instance_oid.len - 2,
            &instance->reference,
            &instance->reference_len);

    if (ret == SNMP_ERR_NOERROR) {
      /* search column */
      const struct snmp_table_simple_col_def *col_def = table_node->columns;
      u32_t i = table_node->column_count;
      while (i > 0) {
        if (col_def->index == instance->instance_oid.id[1]) {
          break;
        }

        col_def++;
        i--;
      }

      if (i > 0) {
        instance->asn1_type = col_def->asn1_type;
        instance->access    = SNMP_NODE_INSTANCE_READ_ONLY;
        instance->set_test  = NULL;
        instance->set_value = NULL;

        switch (col_def->data_type) {
          case SNMP_VARIANT_VALUE_TYPE_U32:
            instance->get_value = snmp_table_extract_value_from_u32ref;
            break;
          case SNMP_VARIANT_VALUE_TYPE_S32:
            instance->get_value = snmp_table_extract_value_from_s32ref;
            break;
          case SNMP_VARIANT_VALUE_TYPE_PTR: /* fall through */
          case SNMP_VARIANT_VALUE_TYPE_CONST_PTR:
            instance->get_value = snmp_table_extract_value_from_refconstptr;
            break;
          default:
            LWIP_DEBUGF(SNMP_DEBUG, ("snmp_table_simple_get_instance(): unknown column data_type: %d\n", col_def->data_type));
            return SNMP_ERR_GENERROR;
        }

        ret = SNMP_ERR_NOERROR;
      } else {
        ret = SNMP_ERR_NOSUCHINSTANCE;
      }
    }
  }

  return ret;
}

snmp_err_t snmp_table_simple_get_next_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance *instance)
{
  const struct snmp_table_simple_node *table_node = (const struct snmp_table_simple_node *)(const void *)instance->node;
  const struct snmp_table_simple_col_def *col_def;
  struct snmp_obj_id row_oid;
  u32_t column = 0;
  snmp_err_t result;

  LWIP_UNUSED_ARG(root_oid);
  LWIP_UNUSED_ARG(root_oid_len);

  /* check that first part of id is 0 or 1, referencing fixed row entry */
  if ((instance->instance_oid.len > 0) && (instance->instance_oid.id[0] > 1)) {
    return SNMP_ERR_NOSUCHINSTANCE;
  }
  if (instance->instance_oid.len > 1) {
    column = instance->instance_oid.id[1];
  }
  if (instance->instance_oid.len > 2) {
    snmp_oid_assign(&row_oid, &(instance->instance_oid.id[2]), instance->instance_oid.len - 2);
  } else {
    row_oid.len = 0;
  }

  /* resolve column and value */
  do {
    u32_t i;
    const struct snmp_table_simple_col_def *next_col_def = NULL;
    col_def = table_node->columns;

    for (i = 0; i < table_node->column_count; i++) {
      if (col_def->index == column) {
        next_col_def = col_def;
        break;
      } else if ((col_def->index > column) && ((next_col_def == NULL) ||
                 (col_def->index < next_col_def->index))) {
        next_col_def = col_def;
      }
      col_def++;
    }

    if (next_col_def == NULL) {
      /* no further column found */
      return SNMP_ERR_NOSUCHINSTANCE;
    }

    result = table_node->get_next_cell_instance_and_value(
               &next_col_def->index,
               &row_oid,
               &instance->reference,
               &instance->reference_len);

    if (result == SNMP_ERR_NOERROR) {
      col_def = next_col_def;
      break;
    }

    row_oid.len = 0; /* reset row_oid because we switch to next column and start with the first entry there */
    column = next_col_def->index + 1;
  } while (1);

  instance->asn1_type = col_def->asn1_type;
  instance->access    = SNMP_NODE_INSTANCE_READ_ONLY;
  instance->set_test  = NULL;
  instance->set_value = NULL;

  switch (col_def->data_type) {
    case SNMP_VARIANT_VALUE_TYPE_U32:
      instance->get_value = snmp_table_extract_value_from_u32ref;
      break;
    case SNMP_VARIANT_VALUE_TYPE_S32:
      instance->get_value = snmp_table_extract_value_from_s32ref;
      break;
    case SNMP_VARIANT_VALUE_TYPE_PTR: /* fall through */
    case SNMP_VARIANT_VALUE_TYPE_CONST_PTR:
      instance->get_value = snmp_table_extract_value_from_refconstptr;
      break;
    default:
      LWIP_DEBUGF(SNMP_DEBUG, ("snmp_table_simple_get_instance(): unknown column data_type: %d\n", col_def->data_type));
      return SNMP_ERR_GENERROR;
  }

  /* build resulting oid */
  instance->instance_oid.len   = 2;
  instance->instance_oid.id[0] = 1;
  instance->instance_oid.id[1] = col_def->index;
  snmp_oid_append(&instance->instance_oid, row_oid.id, row_oid.len);

  return SNMP_ERR_NOERROR;
}


s16_t
snmp_table_extract_value_from_s32ref(struct snmp_node_instance *instance, void *value)
{
  s32_t *dst = (s32_t *)value;
  *dst = instance->reference.s32;
  return sizeof(*dst);
}

s16_t
snmp_table_extract_value_from_u32ref(struct snmp_node_instance *instance, void *value)
{
  u32_t *dst = (u32_t *)value;
  *dst = instance->reference.u32;
  return sizeof(*dst);
}

s16_t
snmp_table_extract_value_from_refconstptr(struct snmp_node_instance *instance, void *value)
{
  MEMCPY(value, instance->reference.const_ptr, instance->reference_len);
  return (u16_t)instance->reference_len;
}

#endif /* LWIP_SNMP */
