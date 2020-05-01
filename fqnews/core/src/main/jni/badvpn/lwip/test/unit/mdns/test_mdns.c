/*
 * Copyright (c) 2015 Verisure Innovation AB
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
 * Author: Erik Ekman <erik@kryo.se>
 *
 */

#include "test_mdns.h"

#include "lwip/pbuf.h"
#include "lwip/apps/mdns.h"
#include "lwip/apps/mdns_priv.h"

START_TEST(readname_basic)
{
  static const u8_t data[] = { 0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0x00 };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0, &domain);
  pbuf_free(p);
  fail_unless(offset == sizeof(data));
  fail_unless(domain.length == sizeof(data));
  fail_if(memcmp(&domain.name, data, sizeof(data)));
}
END_TEST

START_TEST(readname_anydata)
{
  static const u8_t data[] = { 0x05, 0x00, 0xFF, 0x08, 0xc0, 0x0f, 0x04, 0x7f, 0x80, 0x82, 0x88, 0x00 };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0, &domain);
  pbuf_free(p);
  fail_unless(offset == sizeof(data));
  fail_unless(domain.length == sizeof(data));
  fail_if(memcmp(&domain.name, data, sizeof(data)));
}
END_TEST

START_TEST(readname_short_buf)
{
  static const u8_t data[] = { 0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a' };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0, &domain);
  pbuf_free(p);
  fail_unless(offset == MDNS_READNAME_ERROR);
}
END_TEST

START_TEST(readname_long_label)
{
  static const u8_t data[] = {
      0x05, 'm', 'u', 'l', 't', 'i',
      0x52, 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
      'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
      'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
      'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
      'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
      'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0, &domain);
  pbuf_free(p);
  fail_unless(offset == MDNS_READNAME_ERROR);
}
END_TEST

START_TEST(readname_overflow)
{
  static const u8_t data[] = {
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0, &domain);
  pbuf_free(p);
  fail_unless(offset == MDNS_READNAME_ERROR);
}
END_TEST

START_TEST(readname_jump_earlier)
{
  static const u8_t data[] = {
      /* Some padding needed, not supported to jump to bytes containing dns header */
      /*  0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      /* 10 */ 0x0f, 0x0e, 0x05, 'l', 'o', 'c', 'a', 'l', 0x00, 0xab,
      /* 20 */ 0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0xc0, 0x0c
  };
  static const u8_t fullname[] = {
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0x05, 'l', 'o', 'c', 'a', 'l', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 20, &domain);
  pbuf_free(p);
  fail_unless(offset == sizeof(data));
  fail_unless(domain.length == sizeof(fullname));

  fail_if(memcmp(&domain.name, fullname, sizeof(fullname)));
}
END_TEST

START_TEST(readname_jump_earlier_jump)
{
  static const u8_t data[] = {
      /* Some padding needed, not supported to jump to bytes containing dns header */
      /* 0x00 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      /* 0x08 */ 0x00, 0x00, 0x00, 0x00, 0x03, 0x0b, 0x0a, 0xf2,
      /* 0x10 */ 0x04, 'c', 'a', 's', 't', 0x00, 0xc0, 0x10,
      /* 0x18 */ 0x05, 'm', 'u', 'l', 't', 'i', 0xc0, 0x16
  };
  static const u8_t fullname[] = {
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0x18, &domain);
  pbuf_free(p);
  fail_unless(offset == sizeof(data));
  fail_unless(domain.length == sizeof(fullname));

  fail_if(memcmp(&domain.name, fullname, sizeof(fullname)));
}
END_TEST

START_TEST(readname_jump_maxdepth)
{
  static const u8_t data[] = {
      /* Some padding needed, not supported to jump to bytes containing dns header */
      /* 0x00 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      /* 0x08 */ 0x00, 0x00, 0x00, 0x00, 0x03, 0x0b, 0x0a, 0xf2,
      /* 0x10 */ 0x04, 'n', 'a', 'm', 'e', 0xc0, 0x27, 0x03,
      /* 0x18 */ 0x03, 'd', 'n', 's', 0xc0, 0x10, 0xc0, 0x10,
      /* 0x20 */ 0x04, 'd', 'e', 'e', 'p', 0xc0, 0x18, 0x00,
      /* 0x28 */ 0x04, 'c', 'a', 's', 't', 0xc0, 0x20, 0xb0,
      /* 0x30 */ 0x05, 'm', 'u', 'l', 't', 'i', 0xc0, 0x28
  };
  static const u8_t fullname[] = {
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't',
      0x04, 'd', 'e', 'e', 'p', 0x03, 'd', 'n', 's',
      0x04, 'n', 'a', 'm', 'e', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0x30, &domain);
  pbuf_free(p);
  fail_unless(offset == sizeof(data));
  fail_unless(domain.length == sizeof(fullname));

  fail_if(memcmp(&domain.name, fullname, sizeof(fullname)));
}
END_TEST

START_TEST(readname_jump_later)
{
  static const u8_t data[] = {
      /* 0x00 */ 0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0xc0, 0x10, 0x00, 0x01, 0x40,
      /* 0x10 */ 0x05, 'l', 'o', 'c', 'a', 'l', 0x00, 0xab
  };
  static const u8_t fullname[] = {
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0x05, 'l', 'o', 'c', 'a', 'l', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0, &domain);
  pbuf_free(p);
  fail_unless(offset == 13);
  fail_unless(domain.length == sizeof(fullname));

  fail_if(memcmp(&domain.name, fullname, sizeof(fullname)));
}
END_TEST

START_TEST(readname_half_jump)
{
  static const u8_t data[] = {
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0xc0
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0, &domain);
  pbuf_free(p);
  fail_unless(offset == MDNS_READNAME_ERROR);
}
END_TEST

START_TEST(readname_jump_toolong)
{
  static const u8_t data[] = {
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0xc2, 0x10, 0x00, 0x01, 0x40
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 0, &domain);
  pbuf_free(p);
  fail_unless(offset == MDNS_READNAME_ERROR);
}
END_TEST

START_TEST(readname_jump_loop_label)
{
  static const u8_t data[] = {
      /*  0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      /* 10 */ 0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0xc0, 0x10
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 10, &domain);
  pbuf_free(p);
  fail_unless(offset == MDNS_READNAME_ERROR);
}
END_TEST

START_TEST(readname_jump_loop_jump)
{
  static const u8_t data[] = {
      /*  0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      /* 10 */ 0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0xc0, 0x15
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;
  offset = mdns_readname(p, 10, &domain);
  pbuf_free(p);
  fail_unless(offset == MDNS_READNAME_ERROR);
}
END_TEST

START_TEST(add_label_basic)
{
  static const u8_t data[] = { 0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0x00 };
  struct mdns_domain domain;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "multi", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, "cast", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == sizeof(data));
  fail_if(memcmp(&domain.name, data, sizeof(data)));
}
END_TEST

START_TEST(add_label_long_label)
{
  static const char *toolong = "abcdefghijklmnopqrstuvwxyz0123456789-abcdefghijklmnopqrstuvwxyz0123456789-abcdefghijklmnopqrstuvwxyz0123456789-";
  struct mdns_domain domain;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "multi", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, toolong, (u8_t)strlen(toolong));
  fail_unless(res == ERR_VAL);
}
END_TEST

START_TEST(add_label_full)
{
  static const char *label = "0123456789abcdef0123456789abcdef";
  struct mdns_domain domain;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, label, (u8_t)strlen(label));
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == 33);
  res = mdns_domain_add_label(&domain, label, (u8_t)strlen(label));
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == 66);
  res = mdns_domain_add_label(&domain, label, (u8_t)strlen(label));
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == 99);
  res = mdns_domain_add_label(&domain, label, (u8_t)strlen(label));
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == 132);
  res = mdns_domain_add_label(&domain, label, (u8_t)strlen(label));
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == 165);
  res = mdns_domain_add_label(&domain, label, (u8_t)strlen(label));
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == 198);
  res = mdns_domain_add_label(&domain, label, (u8_t)strlen(label));
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == 231);
  res = mdns_domain_add_label(&domain, label, (u8_t)strlen(label));
  fail_unless(res == ERR_VAL);
  fail_unless(domain.length == 231);
  res = mdns_domain_add_label(&domain, label, 25);
  fail_unless(res == ERR_VAL);
  fail_unless(domain.length == 231);
  res = mdns_domain_add_label(&domain, label, 24);
  fail_unless(res == ERR_VAL);
  fail_unless(domain.length == 231);
  res = mdns_domain_add_label(&domain, label, 23);
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == 255);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);
  fail_unless(domain.length == 256);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_VAL);
  fail_unless(domain.length == 256);
}
END_TEST

START_TEST(domain_eq_basic)
{
  static const u8_t data[] = {
      0x05, 'm', 'u', 'l', 't', 'i', 0x04, 'c', 'a', 's', 't', 0x00
  };
  struct mdns_domain domain1, domain2;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  memset(&domain1, 0, sizeof(domain1));
  res = mdns_domain_add_label(&domain1, "multi", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, "cast", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, NULL, 0);
  fail_unless(res == ERR_OK);
  fail_unless(domain1.length == sizeof(data));

  memset(&domain2, 0, sizeof(domain2));
  res = mdns_domain_add_label(&domain2, "multi", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, "cast", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, NULL, 0);
  fail_unless(res == ERR_OK);

  fail_unless(mdns_domain_eq(&domain1, &domain2));
}
END_TEST

START_TEST(domain_eq_diff)
{
  struct mdns_domain domain1, domain2;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  memset(&domain1, 0, sizeof(domain1));
  res = mdns_domain_add_label(&domain1, "multi", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, "base", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, NULL, 0);
  fail_unless(res == ERR_OK);

  memset(&domain2, 0, sizeof(domain2));
  res = mdns_domain_add_label(&domain2, "multi", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, "cast", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, NULL, 0);
  fail_unless(res == ERR_OK);

  fail_if(mdns_domain_eq(&domain1, &domain2));
}
END_TEST

START_TEST(domain_eq_case)
{
  struct mdns_domain domain1, domain2;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  memset(&domain1, 0, sizeof(domain1));
  res = mdns_domain_add_label(&domain1, "multi", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, "cast", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, NULL, 0);
  fail_unless(res == ERR_OK);

  memset(&domain2, 0, sizeof(domain2));
  res = mdns_domain_add_label(&domain2, "MulTI", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, "casT", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, NULL, 0);
  fail_unless(res == ERR_OK);

  fail_unless(mdns_domain_eq(&domain1, &domain2));
}
END_TEST

START_TEST(domain_eq_anydata)
{
  static const u8_t data1[] = { 0x05, 0xcc, 0xdc, 0x00, 0xa0 };
  static const u8_t data2[] = { 0x7f, 0x8c, 0x01, 0xff, 0xcf };
  struct mdns_domain domain1, domain2;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  memset(&domain1, 0, sizeof(domain1));
  res = mdns_domain_add_label(&domain1, (const char*)data1, sizeof(data1));
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, "cast", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, (const char*)data2, sizeof(data2));
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, NULL, 0);
  fail_unless(res == ERR_OK);

  memset(&domain2, 0, sizeof(domain2));
  res = mdns_domain_add_label(&domain2, (const char*)data1, sizeof(data1));
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, "casT", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, (const char*)data2, sizeof(data2));
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, NULL, 0);
  fail_unless(res == ERR_OK);

  fail_unless(mdns_domain_eq(&domain1, &domain2));
}
END_TEST

START_TEST(domain_eq_length)
{
  struct mdns_domain domain1, domain2;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  memset(&domain1, 0, sizeof(domain1));
  memset(domain1.name, 0xAA, sizeof(MDNS_DOMAIN_MAXLEN));
  res = mdns_domain_add_label(&domain1, "multi", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain1, "cast", 4);
  fail_unless(res == ERR_OK);

  memset(&domain2, 0, sizeof(domain2));
  memset(domain2.name, 0xBB, sizeof(MDNS_DOMAIN_MAXLEN));
  res = mdns_domain_add_label(&domain2, "multi", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain2, "cast", 4);
  fail_unless(res == ERR_OK);

  fail_unless(mdns_domain_eq(&domain1, &domain2));
}
END_TEST

START_TEST(compress_full_match)
{
  static const u8_t data[] = {
      0x00, 0x00,
      0x06, 'f', 'o', 'o', 'b', 'a', 'r', 0x05, 'l', 'o', 'c', 'a', 'l', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  u16_t length;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "foobar", 6);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, "local", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);

  offset = 2;
  length = mdns_compress_domain(p, &offset, &domain);
  /* Write 0 bytes, then a jump to addr 2 */
  fail_unless(length == 0);
  fail_unless(offset == 2);

  pbuf_free(p);
}
END_TEST

START_TEST(compress_full_match_subset)
{
  static const u8_t data[] = {
      0x00, 0x00,
      0x02, 'g', 'o', 0x06, 'f', 'o', 'o', 'b', 'a', 'r', 0x05, 'l', 'o', 'c', 'a', 'l', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  u16_t length;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "foobar", 6);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, "local", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);

  offset = 2;
  length = mdns_compress_domain(p, &offset, &domain);
  /* Write 0 bytes, then a jump to addr 5 */
  fail_unless(length == 0);
  fail_unless(offset == 5);

  pbuf_free(p);
}
END_TEST

START_TEST(compress_full_match_jump)
{
  static const u8_t data[] = {
    /* 0x00 */ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
               0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    /* 0x10 */ 0x04, 'l', 'w', 'i', 'p', 0x05, 'l', 'o', 'c', 'a', 'l', 0x00, 0xc0, 0x00, 0x02, 0x00,
    /* 0x20 */ 0x06, 'f', 'o', 'o', 'b', 'a', 'r', 0xc0, 0x15
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  u16_t length;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "foobar", 6);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, "local", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);

  offset = 0x20;
  length = mdns_compress_domain(p, &offset, &domain);
  /* Write 0 bytes, then a jump to addr 0x20 */
  fail_unless(length == 0);
  fail_unless(offset == 0x20);

  pbuf_free(p);
}
END_TEST

START_TEST(compress_no_match)
{
  static const u8_t data[] = {
      0x00, 0x00,
      0x04, 'l', 'w', 'i', 'p', 0x05, 'w', 'i', 'k', 'i', 'a', 0x03, 'c', 'o', 'm', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  u16_t length;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "foobar", 6);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, "local", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);

  offset = 2;
  length = mdns_compress_domain(p, &offset, &domain);
  /* Write all bytes, no jump */
  fail_unless(length == domain.length);

  pbuf_free(p);
}
END_TEST

START_TEST(compress_2nd_label)
{
  static const u8_t data[] = {
      0x00, 0x00,
      0x06, 'f', 'o', 'o', 'b', 'a', 'r', 0x05, 'l', 'o', 'c', 'a', 'l', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  u16_t length;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "lwip", 4);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, "local", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);

  offset = 2;
  length = mdns_compress_domain(p, &offset, &domain);
  /* Write 5 bytes, then a jump to addr 9 */
  fail_unless(length == 5);
  fail_unless(offset == 9);

  pbuf_free(p);
}
END_TEST

START_TEST(compress_2nd_label_short)
{
  static const u8_t data[] = {
      0x00, 0x00,
      0x04, 'l', 'w', 'i', 'p', 0x05, 'l', 'o', 'c', 'a', 'l', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  u16_t length;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "foobar", 6);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, "local", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);

  offset = 2;
  length = mdns_compress_domain(p, &offset, &domain);
  /* Write 5 bytes, then a jump to addr 7 */
  fail_unless(length == 7);
  fail_unless(offset == 7);

  pbuf_free(p);
}
END_TEST

START_TEST(compress_jump_to_jump)
{
  static const u8_t data[] = {
      /* 0x00 */ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
      /* 0x10 */ 0x04, 'l', 'w', 'i', 'p', 0x05, 'l', 'o', 'c', 'a', 'l', 0x00, 0xc0, 0x00, 0x02, 0x00,
      /* 0x20 */ 0x07, 'b', 'a', 'n', 'a', 'n', 'a', 's', 0xc0, 0x15
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  u16_t length;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "foobar", 6);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, "local", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);

  offset = 0x20;
  length = mdns_compress_domain(p, &offset, &domain);
  /* Dont compress if jump would be to a jump */
  fail_unless(length == domain.length);

  offset = 0x10;
  length = mdns_compress_domain(p, &offset, &domain);
  /* Write 7 bytes, then a jump to addr 0x15 */
  fail_unless(length == 7);
  fail_unless(offset == 0x15);

  pbuf_free(p);
}
END_TEST

START_TEST(compress_long_match)
{
  static const u8_t data[] = {
      0x00, 0x00,
      0x06, 'f', 'o', 'o', 'b', 'a', 'r', 0x05, 'l', 'o', 'c', 'a', 'l', 0x03, 'c', 'o', 'm', 0x00
  };
  struct pbuf *p;
  struct mdns_domain domain;
  u16_t offset;
  u16_t length;
  err_t res;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, sizeof(data), PBUF_ROM);
  fail_if(p == NULL);
  p->payload = (void *)(size_t)data;

  memset(&domain, 0, sizeof(domain));
  res = mdns_domain_add_label(&domain, "foobar", 6);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, "local", 5);
  fail_unless(res == ERR_OK);
  res = mdns_domain_add_label(&domain, NULL, 0);
  fail_unless(res == ERR_OK);

  offset = 2;
  length = mdns_compress_domain(p, &offset, &domain);
  fail_unless(length == domain.length);

  pbuf_free(p);
}
END_TEST

Suite* mdns_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(readname_basic),
    TESTFUNC(readname_anydata),
    TESTFUNC(readname_short_buf),
    TESTFUNC(readname_long_label),
    TESTFUNC(readname_overflow),
    TESTFUNC(readname_jump_earlier),
    TESTFUNC(readname_jump_earlier_jump),
    TESTFUNC(readname_jump_maxdepth),
    TESTFUNC(readname_jump_later),
    TESTFUNC(readname_half_jump),
    TESTFUNC(readname_jump_toolong),
    TESTFUNC(readname_jump_loop_label),
    TESTFUNC(readname_jump_loop_jump),

    TESTFUNC(add_label_basic),
    TESTFUNC(add_label_long_label),
    TESTFUNC(add_label_full),

    TESTFUNC(domain_eq_basic),
    TESTFUNC(domain_eq_diff),
    TESTFUNC(domain_eq_case),
    TESTFUNC(domain_eq_anydata),
    TESTFUNC(domain_eq_length),

    TESTFUNC(compress_full_match),
    TESTFUNC(compress_full_match_subset),
    TESTFUNC(compress_full_match_jump),
    TESTFUNC(compress_no_match),
    TESTFUNC(compress_2nd_label),
    TESTFUNC(compress_2nd_label_short),
    TESTFUNC(compress_jump_to_jump),
    TESTFUNC(compress_long_match),
  };
  return create_suite("MDNS", tests, sizeof(tests)/sizeof(testfunc), NULL, NULL);
}
