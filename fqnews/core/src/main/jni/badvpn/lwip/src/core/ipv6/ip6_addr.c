/**
 * @file
 *
 * IPv6 addresses.
 */

/*
 * Copyright (c) 2010 Inico Technologies Ltd.
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
 * Author: Ivan Delamer <delamer@inicotech.com>
 *
 * Functions for handling IPv6 addresses.
 *
 * Please coordinate changes and requests with Ivan Delamer
 * <delamer@inicotech.com>
 */

#include "lwip/opt.h"

#if LWIP_IPV6  /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip_addr.h"
#include "lwip/def.h"

/* used by IP6_ADDR_ANY(6) in ip6_addr.h */
const ip_addr_t ip6_addr_any = IPADDR6_INIT(0ul, 0ul, 0ul, 0ul);

#ifndef isprint
#define in_range(c, lo, up)  ((u8_t)c >= lo && (u8_t)c <= up)
#define isprint(c)           in_range(c, 0x20, 0x7f)
#define isdigit(c)           in_range(c, '0', '9')
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#define islower(c)           in_range(c, 'a', 'z')
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#define xchar(i)             ((char)((i) < 10 ? '0' + (i) : 'A' + (i) - 10))
#endif

/**
 * Check whether "cp" is a valid ascii representation
 * of an IPv6 address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 *
 * @param cp IPv6 address in ascii representation (e.g. "FF01::1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int
ip6addr_aton(const char *cp, ip6_addr_t *addr)
{
  u32_t addr_index, zero_blocks, current_block_index, current_block_value;
  const char *s;

  /* Count the number of colons, to count the number of blocks in a "::" sequence
     zero_blocks may be 1 even if there are no :: sequences */
  zero_blocks = 8;
  for (s = cp; *s != 0; s++) {
    if (*s == ':') {
      zero_blocks--;
    } else if (!isxdigit(*s)) {
      break;
    }
  }

  /* parse each block */
  addr_index = 0;
  current_block_index = 0;
  current_block_value = 0;
  for (s = cp; *s != 0; s++) {
    if (*s == ':') {
      if (addr) {
        if (current_block_index & 0x1) {
          addr->addr[addr_index++] |= current_block_value;
        }
        else {
          addr->addr[addr_index] = current_block_value << 16;
        }
      }
      current_block_index++;
      current_block_value = 0;
      if (current_block_index > 7) {
        /* address too long! */
        return 0;
      }
      if (s[1] == ':') {
        if (s[2] == ':') {
          /* invalid format: three successive colons */
          return 0;
        }
        s++;
        /* "::" found, set zeros */
        while (zero_blocks > 0) {
          zero_blocks--;
          if (current_block_index & 0x1) {
            addr_index++;
          } else {
            if (addr) {
              addr->addr[addr_index] = 0;
            }
          }
          current_block_index++;
          if (current_block_index > 7) {
            /* address too long! */
            return 0;
          }
        }
      }
    } else if (isxdigit(*s)) {
      /* add current digit */
      current_block_value = (current_block_value << 4) +
          (isdigit(*s) ? (u32_t)(*s - '0') :
          (u32_t)(10 + (islower(*s) ? *s - 'a' : *s - 'A')));
    } else {
      /* unexpected digit, space? CRLF? */
      break;
    }
  }

  if (addr) {
    if (current_block_index & 0x1) {
      addr->addr[addr_index++] |= current_block_value;
    }
    else {
      addr->addr[addr_index] = current_block_value << 16;
    }
  }

  /* convert to network byte order. */
  if (addr) {
    for (addr_index = 0; addr_index < 4; addr_index++) {
      addr->addr[addr_index] = lwip_htonl(addr->addr[addr_index]);
    }
  }

  if (current_block_index != 7) {
    return 0;
  }

  ip6_addr_clear_zone(addr);

  return 1;
}

/**
 * Convert numeric IPv6 address into ASCII representation.
 * returns ptr to static buffer; not reentrant!
 *
 * @param addr ip6 address in network order to convert
 * @return pointer to a global static (!) buffer that holds the ASCII
 *         representation of addr
 */
char *
ip6addr_ntoa(const ip6_addr_t *addr)
{
  static char str[40];
  return ip6addr_ntoa_r(addr, str, 40);
}

/**
 * Same as ipaddr_ntoa, but reentrant since a user-supplied buffer is used.
 *
 * @param addr ip6 address in network order to convert
 * @param buf target buffer where the string is stored
 * @param buflen length of buf
 * @return either pointer to buf which now holds the ASCII
 *         representation of addr or NULL if buf was too small
 */
char *
ip6addr_ntoa_r(const ip6_addr_t *addr, char *buf, int buflen)
{
  u32_t current_block_index, current_block_value, next_block_value;
  s32_t i;
  u8_t zero_flag, empty_block_flag;

  i = 0;
  empty_block_flag = 0; /* used to indicate a zero chain for "::' */

  for (current_block_index = 0; current_block_index < 8; current_block_index++) {
    /* get the current 16-bit block */
    current_block_value = lwip_htonl(addr->addr[current_block_index >> 1]);
    if ((current_block_index & 0x1) == 0) {
      current_block_value = current_block_value >> 16;
    }
    current_block_value &= 0xffff;

    /* Check for empty block. */
    if (current_block_value == 0) {
      if (current_block_index == 7 && empty_block_flag == 1) {
        /* special case, we must render a ':' for the last block. */
        buf[i++] = ':';
        if (i >= buflen) {
          return NULL;
        }
        break;
      }
      if (empty_block_flag == 0) {
        /* generate empty block "::", but only if more than one contiguous zero block,
         * according to current formatting suggestions RFC 5952. */
        next_block_value = lwip_htonl(addr->addr[(current_block_index + 1) >> 1]);
        if ((current_block_index & 0x1) == 0x01) {
            next_block_value = next_block_value >> 16;
        }
        next_block_value &= 0xffff;
        if (next_block_value == 0) {
          empty_block_flag = 1;
          buf[i++] = ':';
          if (i >= buflen) {
            return NULL;
          }
          continue; /* move on to next block. */
        }
      } else if (empty_block_flag == 1) {
        /* move on to next block. */
        continue;
      }
    } else if (empty_block_flag == 1) {
      /* Set this flag value so we don't produce multiple empty blocks. */
      empty_block_flag = 2;
    }

    if (current_block_index > 0) {
      buf[i++] = ':';
      if (i >= buflen) {
        return NULL;
      }
    }

    if ((current_block_value & 0xf000) == 0) {
      zero_flag = 1;
    } else {
      buf[i++] = xchar(((current_block_value & 0xf000) >> 12));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    if (((current_block_value & 0xf00) == 0) && (zero_flag)) {
      /* do nothing */
    } else {
      buf[i++] = xchar(((current_block_value & 0xf00) >> 8));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    if (((current_block_value & 0xf0) == 0) && (zero_flag)) {
      /* do nothing */
    }
    else {
      buf[i++] = xchar(((current_block_value & 0xf0) >> 4));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    buf[i++] = xchar((current_block_value & 0xf));
    if (i >= buflen) {
      return NULL;
    }
  }

  buf[i] = 0;

  return buf;
}

#endif /* LWIP_IPV6 */
