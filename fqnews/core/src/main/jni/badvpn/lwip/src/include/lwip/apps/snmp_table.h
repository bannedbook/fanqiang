/**
 * @file
 * SNMP server MIB API to implement table nodes
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

#ifndef LWIP_HDR_APPS_SNMP_TABLE_H
#define LWIP_HDR_APPS_SNMP_TABLE_H

#include "lwip/apps/snmp_opts.h"
#include "lwip/apps/snmp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LWIP_SNMP /* don't build if not configured for use in lwipopts.h */

/** default (customizable) read/write table */
struct snmp_table_col_def
{
  u32_t index;
  u8_t asn1_type;
  snmp_access_t access;
};

/** table node */
struct snmp_table_node
{
  /** inherited "base class" members */
  struct snmp_leaf_node node;
  u16_t column_count;
  const struct snmp_table_col_def* columns;
  snmp_err_t (*get_cell_instance)(const u32_t* column, const u32_t* row_oid, u8_t row_oid_len, struct snmp_node_instance* cell_instance);
  snmp_err_t (*get_next_cell_instance)(const u32_t* column, struct snmp_obj_id* row_oid, struct snmp_node_instance* cell_instance);
  /** returns object value for the given object identifier */
  node_instance_get_value_method get_value;
  /** tests length and/or range BEFORE setting */
  node_instance_set_test_method set_test;
  /** sets object value, only called when set_test() was successful */
  node_instance_set_value_method set_value;
};

snmp_err_t snmp_table_get_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance* instance);
snmp_err_t snmp_table_get_next_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance* instance);

#define SNMP_TABLE_CREATE(oid, columns, get_cell_instance_method, get_next_cell_instance_method, get_value_method, set_test_method, set_value_method) \
  {{{ SNMP_NODE_TABLE, (oid) }, \
  snmp_table_get_instance, \
  snmp_table_get_next_instance }, \
  (u16_t)LWIP_ARRAYSIZE(columns), (columns), \
  (get_cell_instance_method), (get_next_cell_instance_method), \
  (get_value_method), (set_test_method), (set_value_method)}

#define SNMP_TABLE_GET_COLUMN_FROM_OID(oid) ((oid)[1]) /* first array value is (fixed) row entry (fixed to 1) and 2nd value is column, follow3ed by instance */


/** simple read-only table */
typedef enum {
  SNMP_VARIANT_VALUE_TYPE_U32,
  SNMP_VARIANT_VALUE_TYPE_S32,
  SNMP_VARIANT_VALUE_TYPE_PTR,
  SNMP_VARIANT_VALUE_TYPE_CONST_PTR
} snmp_table_column_data_type_t;

struct snmp_table_simple_col_def
{
  u32_t index;
  u8_t asn1_type;
  snmp_table_column_data_type_t data_type; /* depending of what union member is used to store the value*/
};

/** simple read-only table node */
struct snmp_table_simple_node
{
  /* inherited "base class" members */
  struct snmp_leaf_node node;
  u16_t column_count;
  const struct snmp_table_simple_col_def* columns;
  snmp_err_t (*get_cell_value)(const u32_t* column, const u32_t* row_oid, u8_t row_oid_len, union snmp_variant_value* value, u32_t* value_len);
  snmp_err_t (*get_next_cell_instance_and_value)(const u32_t* column, struct snmp_obj_id* row_oid, union snmp_variant_value* value, u32_t* value_len);
};

snmp_err_t snmp_table_simple_get_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance* instance);
snmp_err_t snmp_table_simple_get_next_instance(const u32_t *root_oid, u8_t root_oid_len, struct snmp_node_instance* instance);

#define SNMP_TABLE_CREATE_SIMPLE(oid, columns, get_cell_value_method, get_next_cell_instance_and_value_method) \
  {{{ SNMP_NODE_TABLE, (oid) }, \
  snmp_table_simple_get_instance, \
  snmp_table_simple_get_next_instance }, \
  (u16_t)LWIP_ARRAYSIZE(columns), (columns), (get_cell_value_method), (get_next_cell_instance_and_value_method) }

s16_t snmp_table_extract_value_from_s32ref(struct snmp_node_instance* instance, void* value);
s16_t snmp_table_extract_value_from_u32ref(struct snmp_node_instance* instance, void* value);
s16_t snmp_table_extract_value_from_refconstptr(struct snmp_node_instance* instance, void* value);

#endif /* LWIP_SNMP */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_APPS_SNMP_TABLE_H */
