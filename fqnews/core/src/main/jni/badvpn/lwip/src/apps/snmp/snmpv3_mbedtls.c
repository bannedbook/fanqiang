/**
 * @file
 * SNMPv3 crypto/auth functions implemented for ARM mbedtls.
 */

/*
 * Copyright (c) 2016 Elias Oenal and Dirk Ziegelmeier.
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
 * Author: Elias Oenal <lwip@eliasoenal.com>
 *         Dirk Ziegelmeier <dirk@ziegelmeier.net>
 */

#include "lwip/apps/snmpv3.h"
#include "snmpv3_priv.h"
#include "lwip/arch.h"
#include "snmp_msg.h"
#include "lwip/sys.h"
#include <string.h>

#if LWIP_SNMP && LWIP_SNMP_V3 && LWIP_SNMP_V3_MBEDTLS

#include "mbedtls/md.h"
#include "mbedtls/cipher.h"

#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"

err_t
snmpv3_auth(struct snmp_pbuf_stream *stream, u16_t length,
            const u8_t *key, snmpv3_auth_algo_t algo, u8_t *hmac_out)
{
  u32_t i;
  u8_t key_len;
  const mbedtls_md_info_t *md_info;
  mbedtls_md_context_t ctx;
  struct snmp_pbuf_stream read_stream;
  snmp_pbuf_stream_init(&read_stream, stream->pbuf, stream->offset, stream->length);

  if (algo == SNMP_V3_AUTH_ALGO_MD5) {
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
    key_len = SNMP_V3_MD5_LEN;
  } else if (algo == SNMP_V3_AUTH_ALGO_SHA) {
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    key_len = SNMP_V3_SHA_LEN;
  } else {
    return ERR_ARG;
  }

  mbedtls_md_init(&ctx);
  if (mbedtls_md_setup(&ctx, md_info, 1) != 0) {
    return ERR_ARG;
  }

  if (mbedtls_md_hmac_starts(&ctx, key, key_len) != 0) {
    goto free_md;
  }

  for (i = 0; i < length; i++) {
    u8_t byte;

    if (snmp_pbuf_stream_read(&read_stream, &byte)) {
      goto free_md;
    }

    if (mbedtls_md_hmac_update(&ctx, &byte, 1) != 0) {
      goto free_md;
    }
  }

  if (mbedtls_md_hmac_finish(&ctx, hmac_out) != 0) {
    goto free_md;
  }

  mbedtls_md_free(&ctx);
  return ERR_OK;

free_md:
  mbedtls_md_free(&ctx);
  return ERR_ARG;
}

#if LWIP_SNMP_V3_CRYPTO

err_t
snmpv3_crypt(struct snmp_pbuf_stream *stream, u16_t length,
             const u8_t *key, const u8_t *priv_param, const u32_t engine_boots,
             const u32_t engine_time, snmpv3_priv_algo_t algo, snmpv3_priv_mode_t mode)
{
  size_t i;
  mbedtls_cipher_context_t ctx;
  const mbedtls_cipher_info_t *cipher_info;

  struct snmp_pbuf_stream read_stream;
  struct snmp_pbuf_stream write_stream;
  snmp_pbuf_stream_init(&read_stream, stream->pbuf, stream->offset, stream->length);
  snmp_pbuf_stream_init(&write_stream, stream->pbuf, stream->offset, stream->length);
  mbedtls_cipher_init(&ctx);

  if (algo == SNMP_V3_PRIV_ALGO_DES) {
    u8_t iv_local[8];
    u8_t out_bytes[8];
    size_t out_len;

    /* RFC 3414 mandates padding for DES */
    if ((length & 0x07) != 0) {
      return ERR_ARG;
    }

    cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_DES_CBC);
    if (mbedtls_cipher_setup(&ctx, cipher_info) != 0) {
      return ERR_ARG;
    }
    if (mbedtls_cipher_set_padding_mode(&ctx, MBEDTLS_PADDING_NONE) != 0) {
      return ERR_ARG;
    }
    if (mbedtls_cipher_setkey(&ctx, key, 8 * 8, (mode == SNMP_V3_PRIV_MODE_ENCRYPT) ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT) != 0) {
      goto error;
    }

    /* Prepare IV */
    for (i = 0; i < LWIP_ARRAYSIZE(iv_local); i++) {
      iv_local[i] = priv_param[i] ^ key[i + 8];
    }
    if (mbedtls_cipher_set_iv(&ctx, iv_local, LWIP_ARRAYSIZE(iv_local)) != 0) {
      goto error;
    }

    for (i = 0; i < length; i += 8) {
      size_t j;
      u8_t in_bytes[8];
      out_len = LWIP_ARRAYSIZE(out_bytes) ;

      for (j = 0; j < LWIP_ARRAYSIZE(in_bytes); j++) {
        if (snmp_pbuf_stream_read(&read_stream, &in_bytes[j]) != ERR_OK) {
          goto error;
        }
      }

      if (mbedtls_cipher_update(&ctx, in_bytes, LWIP_ARRAYSIZE(in_bytes), out_bytes, &out_len) != 0) {
        goto error;
      }

      if (snmp_pbuf_stream_writebuf(&write_stream, out_bytes, (u16_t)out_len) != ERR_OK) {
        goto error;
      }
    }

    out_len = LWIP_ARRAYSIZE(out_bytes);
    if (mbedtls_cipher_finish(&ctx, out_bytes, &out_len) != 0) {
      goto error;
    }

    if (snmp_pbuf_stream_writebuf(&write_stream, out_bytes, (u16_t)out_len) != ERR_OK) {
      goto error;
    }
  } else if (algo == SNMP_V3_PRIV_ALGO_AES) {
    u8_t iv_local[16];

    cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_CFB128);
    if (mbedtls_cipher_setup(&ctx, cipher_info) != 0) {
      return ERR_ARG;
    }
    if (mbedtls_cipher_setkey(&ctx, key, 16 * 8, (mode == SNMP_V3_PRIV_MODE_ENCRYPT) ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT) != 0) {
      goto error;
    }

    /*
     * IV is the big endian concatenation of boots,
     * uptime and priv param - see RFC3826.
     */
    iv_local[0 + 0] = (engine_boots >> 24) & 0xFF;
    iv_local[0 + 1] = (engine_boots >> 16) & 0xFF;
    iv_local[0 + 2] = (engine_boots >>  8) & 0xFF;
    iv_local[0 + 3] = (engine_boots >>  0) & 0xFF;
    iv_local[4 + 0] = (engine_time  >> 24) & 0xFF;
    iv_local[4 + 1] = (engine_time  >> 16) & 0xFF;
    iv_local[4 + 2] = (engine_time  >>  8) & 0xFF;
    iv_local[4 + 3] = (engine_time  >>  0) & 0xFF;
    SMEMCPY(iv_local + 8, priv_param, 8);
    if (mbedtls_cipher_set_iv(&ctx, iv_local, LWIP_ARRAYSIZE(iv_local)) != 0) {
      goto error;
    }

    for (i = 0; i < length; i++) {
      u8_t in_byte;
      u8_t out_byte;
      size_t out_len = sizeof(out_byte);

      if (snmp_pbuf_stream_read(&read_stream, &in_byte) != ERR_OK) {
        goto error;
      }
      if (mbedtls_cipher_update(&ctx, &in_byte, sizeof(in_byte), &out_byte, &out_len) != 0) {
        goto error;
      }
      if (snmp_pbuf_stream_write(&write_stream, out_byte) != ERR_OK) {
        goto error;
      }
    }
  } else {
    return ERR_ARG;
  }

  mbedtls_cipher_free(&ctx);
  return ERR_OK;

error:
  mbedtls_cipher_free(&ctx);
  return ERR_OK;
}

#endif /* LWIP_SNMP_V3_CRYPTO */

/* A.2.1. Password to Key Sample Code for MD5 */
void
snmpv3_password_to_key_md5(
  const u8_t *password,    /* IN */
  size_t      passwordlen, /* IN */
  const u8_t *engineID,    /* IN  - pointer to snmpEngineID  */
  u8_t        engineLength,/* IN  - length of snmpEngineID */
  u8_t       *key)         /* OUT - pointer to caller 16-octet buffer */
{
  mbedtls_md5_context MD;
  u8_t *cp, password_buf[64];
  u32_t password_index = 0;
  u8_t i;
  u32_t count = 0;

  mbedtls_md5_init(&MD); /* initialize MD5 */
  mbedtls_md5_starts(&MD);

  /**********************************************/
  /* Use while loop until we've done 1 Megabyte */
  /**********************************************/
  while (count < 1048576) {
    cp = password_buf;
    for (i = 0; i < 64; i++) {
      /*************************************************/
      /* Take the next octet of the password, wrapping */
      /* to the beginning of the password as necessary.*/
      /*************************************************/
      *cp++ = password[password_index++ % passwordlen];
    }
    mbedtls_md5_update(&MD, password_buf, 64);
    count += 64;
  }
  mbedtls_md5_finish(&MD, key); /* tell MD5 we're done */

  /*****************************************************/
  /* Now localize the key with the engineID and pass   */
  /* through MD5 to produce final key                  */
  /* May want to ensure that engineLength <= 32,       */
  /* otherwise need to use a buffer larger than 64     */
  /*****************************************************/
  SMEMCPY(password_buf, key, 16);
  MEMCPY(password_buf + 16, engineID, engineLength);
  SMEMCPY(password_buf + 16 + engineLength, key, 16);

  mbedtls_md5_starts(&MD);
  mbedtls_md5_update(&MD, password_buf, 32 + engineLength);
  mbedtls_md5_finish(&MD, key);

  mbedtls_md5_free(&MD);
  return;
}

/* A.2.2. Password to Key Sample Code for SHA */
void
snmpv3_password_to_key_sha(
  const u8_t *password,    /* IN */
  size_t      passwordlen, /* IN */
  const u8_t *engineID,    /* IN  - pointer to snmpEngineID  */
  u8_t        engineLength,/* IN  - length of snmpEngineID */
  u8_t       *key)         /* OUT - pointer to caller 20-octet buffer */
{
  mbedtls_sha1_context SH;
  u8_t *cp, password_buf[72];
  u32_t password_index = 0;
  u8_t i;
  u32_t count = 0;

  mbedtls_sha1_init(&SH); /* initialize SHA */
  mbedtls_sha1_starts(&SH);

  /**********************************************/
  /* Use while loop until we've done 1 Megabyte */
  /**********************************************/
  while (count < 1048576) {
    cp = password_buf;
    for (i = 0; i < 64; i++) {
      /*************************************************/
      /* Take the next octet of the password, wrapping */
      /* to the beginning of the password as necessary.*/
      /*************************************************/
      *cp++ = password[password_index++ % passwordlen];
    }
    mbedtls_sha1_update(&SH, password_buf, 64);
    count += 64;
  }
  mbedtls_sha1_finish(&SH, key); /* tell SHA we're done */

  /*****************************************************/
  /* Now localize the key with the engineID and pass   */
  /* through SHA to produce final key                  */
  /* May want to ensure that engineLength <= 32,       */
  /* otherwise need to use a buffer larger than 72     */
  /*****************************************************/
  SMEMCPY(password_buf, key, 20);
  MEMCPY(password_buf + 20, engineID, engineLength);
  SMEMCPY(password_buf + 20 + engineLength, key, 20);

  mbedtls_sha1_starts(&SH);
  mbedtls_sha1_update(&SH, password_buf, 40 + engineLength);
  mbedtls_sha1_finish(&SH, key);

  mbedtls_sha1_free(&SH);
  return;
}

#endif /* LWIP_SNMP && LWIP_SNMP_V3 && LWIP_SNMP_V3_MBEDTLS */
