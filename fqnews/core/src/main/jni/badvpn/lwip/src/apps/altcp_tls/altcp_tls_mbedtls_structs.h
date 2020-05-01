/**
 * @file
 * Application layered TCP/TLS connection API (to be used from TCPIP thread)
 *
 * This file contains structure definitions for a TLS layer using mbedTLS.
 */

/*
 * Copyright (c) 2017 Simon Goldschmidt
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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */
#ifndef LWIP_HDR_ALTCP_MBEDTLS_STRUCTS_H
#define LWIP_HDR_ALTCP_MBEDTLS_STRUCTS_H

#include "lwip/opt.h"

#if LWIP_ALTCP /* don't build if not configured for use in lwipopts.h */

#include "lwip/apps/altcp_tls_mbedtls_opts.h"

#if LWIP_ALTCP_TLS && LWIP_ALTCP_TLS_MBEDTLS

#include "lwip/altcp.h"
#include "lwip/pbuf.h"

#include "mbedtls/ssl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE  0x01
#define ALTCP_MBEDTLS_FLAGS_APPLDATA_SENT   0x02
#define ALTCP_MBEDTLS_FLAGS_RX_CLOSE_QUEUED 0x04
#define ALTCP_MBEDTLS_FLAGS_RX_CLOSED       0x08
#define ALTCP_MBEDTLS_FLAGS_TX_CLOSED       0x10
#define ALTCP_MBEDTLS_FLAGS_CLOSED          (ALTCP_MBEDTLS_FLAGS_RX_CLOSED|ALTCP_MBEDTLS_FLAGS_TX_CLOSED)
#define ALTCP_MBEDTLS_FLAGS_UPPER_CALLED    0x20

typedef struct altcp_mbedtls_state_s {
  void *conf;
  mbedtls_ssl_context ssl_context;
  /* chain of rx pbufs (before decryption) */
  struct pbuf *rx;
  struct pbuf *rx_app;
  u8_t flags;
  int rx_passed_unrecved;
  int bio_bytes_read;
  int bio_bytes_appl;
} altcp_mbedtls_state_t;

#ifdef __cplusplus
}
#endif

#endif /* LWIP_ALTCP_TLS && LWIP_ALTCP_TLS_MBEDTLS */
#endif /* LWIP_ALTCP */
#endif /* LWIP_HDR_ALTCP_MBEDTLS_STRUCTS_H */
