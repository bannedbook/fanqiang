/**
 * @file
 * SNMP pbuf stream wrapper implementation (internal API, do not use in client code).
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

#include "snmp_pbuf_stream.h"
#include "lwip/def.h"
#include <string.h>

err_t
snmp_pbuf_stream_init(struct snmp_pbuf_stream *pbuf_stream, struct pbuf *p, u16_t offset, u16_t length)
{
  pbuf_stream->offset = offset;
  pbuf_stream->length = length;
  pbuf_stream->pbuf   = p;

  return ERR_OK;
}

err_t
snmp_pbuf_stream_read(struct snmp_pbuf_stream *pbuf_stream, u8_t *data)
{
  if (pbuf_stream->length == 0) {
    return ERR_BUF;
  }

  if (pbuf_copy_partial(pbuf_stream->pbuf, data, 1, pbuf_stream->offset) == 0) {
    return ERR_BUF;
  }

  pbuf_stream->offset++;
  pbuf_stream->length--;

  return ERR_OK;
}

err_t
snmp_pbuf_stream_write(struct snmp_pbuf_stream *pbuf_stream, u8_t data)
{
  return snmp_pbuf_stream_writebuf(pbuf_stream, &data, 1);
}

err_t
snmp_pbuf_stream_writebuf(struct snmp_pbuf_stream *pbuf_stream, const void *buf, u16_t buf_len)
{
  if (pbuf_stream->length < buf_len) {
    return ERR_BUF;
  }

  if (pbuf_take_at(pbuf_stream->pbuf, buf, buf_len, pbuf_stream->offset) != ERR_OK) {
    return ERR_BUF;
  }

  pbuf_stream->offset += buf_len;
  pbuf_stream->length -= buf_len;

  return ERR_OK;
}

err_t
snmp_pbuf_stream_writeto(struct snmp_pbuf_stream *pbuf_stream, struct snmp_pbuf_stream *target_pbuf_stream, u16_t len)
{

  if ((pbuf_stream == NULL) || (target_pbuf_stream == NULL)) {
    return ERR_ARG;
  }
  if ((len > pbuf_stream->length) || (len > target_pbuf_stream->length)) {
    return ERR_ARG;
  }

  if (len == 0) {
    len = LWIP_MIN(pbuf_stream->length, target_pbuf_stream->length);
  }

  while (len > 0) {
    u16_t chunk_len;
    err_t err;
    u16_t target_offset;
    struct pbuf *pbuf = pbuf_skip(pbuf_stream->pbuf, pbuf_stream->offset, &target_offset);

    if ((pbuf == NULL) || (pbuf->len == 0)) {
      return ERR_BUF;
    }

    chunk_len = LWIP_MIN(len, pbuf->len);
    err = snmp_pbuf_stream_writebuf(target_pbuf_stream, &((u8_t *)pbuf->payload)[target_offset], chunk_len);
    if (err != ERR_OK) {
      return err;
    }

    pbuf_stream->offset   += chunk_len;
    pbuf_stream->length   -= chunk_len;
    len -= chunk_len;
  }

  return ERR_OK;
}

err_t
snmp_pbuf_stream_seek(struct snmp_pbuf_stream *pbuf_stream, s32_t offset)
{
  if ((offset < 0) || (offset > pbuf_stream->length)) {
    /* we cannot seek backwards or forward behind stream end */
    return ERR_ARG;
  }

  pbuf_stream->offset += (u16_t)offset;
  pbuf_stream->length -= (u16_t)offset;

  return ERR_OK;
}

err_t
snmp_pbuf_stream_seek_abs(struct snmp_pbuf_stream *pbuf_stream, u32_t offset)
{
  s32_t rel_offset = offset - pbuf_stream->offset;
  return snmp_pbuf_stream_seek(pbuf_stream, rel_offset);
}

#endif /* LWIP_SNMP */
