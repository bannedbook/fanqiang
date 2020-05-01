/**
 * @file
 * MDNS responder private definitions
 */

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
#ifndef LWIP_HDR_MDNS_PRIV_H
#define LWIP_HDR_MDNS_PRIV_H

#include "lwip/apps/mdns_opts.h"
#include "lwip/pbuf.h"

#if LWIP_MDNS_RESPONDER

/* Domain struct and methods - visible for unit tests */

#define MDNS_DOMAIN_MAXLEN 256
#define MDNS_READNAME_ERROR 0xFFFF

struct mdns_domain {
  /* Encoded domain name */
  u8_t name[MDNS_DOMAIN_MAXLEN];
  /* Total length of domain name, including zero */
  u16_t length;
  /* Set if compression of this domain is not allowed */
  u8_t skip_compression;
};

err_t mdns_domain_add_label(struct mdns_domain *domain, const char *label, u8_t len);
u16_t mdns_readname(struct pbuf *p, u16_t offset, struct mdns_domain *domain);
int mdns_domain_eq(struct mdns_domain *a, struct mdns_domain *b);
u16_t mdns_compress_domain(struct pbuf *pbuf, u16_t *offset, struct mdns_domain *domain);

#endif /* LWIP_MDNS_RESPONDER */

#endif /* LWIP_HDR_MDNS_PRIV_H */
