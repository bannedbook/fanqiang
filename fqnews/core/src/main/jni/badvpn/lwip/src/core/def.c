/**
 * @file
 * Common functions used throughout the stack.
 *
 * These are reference implementations of the byte swapping functions.
 * Again with the aim of being simple, correct and fully portable.
 * Byte swapping is the second thing you would want to optimize. You will
 * need to port it to your architecture and in your cc.h:
 *
 * \#define lwip_htons(x) your_htons
 * \#define lwip_htonl(x) your_htonl
 *
 * Note lwip_ntohs() and lwip_ntohl() are merely references to the htonx counterparts.
 *
 * If you \#define them to htons() and htonl(), you should
 * \#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS to prevent lwIP from
 * defining htonx/ntohx compatibility macros.

 * @defgroup sys_nonstandard Non-standard functions
 * @ingroup sys_layer
 * lwIP provides default implementations for non-standard functions.
 * These can be mapped to OS functions to reduce code footprint if desired.
 * All defines related to this section must not be placed in lwipopts.h,
 * but in arch/cc.h!
 * These options cannot be \#defined in lwipopts.h since they are not options
 * of lwIP itself, but options of the lwIP port to your system.
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
 * Author: Simon Goldschmidt
 *
 */

#include "lwip/opt.h"
#include "lwip/def.h"

#include <string.h>

#if BYTE_ORDER == LITTLE_ENDIAN

#if !defined(lwip_htons)
/**
 * Convert an u16_t from host- to network byte order.
 *
 * @param n u16_t in host byte order
 * @return n in network byte order
 */
u16_t
lwip_htons(u16_t n)
{
  return PP_HTONS(n);
}
#endif /* lwip_htons */

#if !defined(lwip_htonl)
/**
 * Convert an u32_t from host- to network byte order.
 *
 * @param n u32_t in host byte order
 * @return n in network byte order
 */
u32_t
lwip_htonl(u32_t n)
{
  return PP_HTONL(n);
}
#endif /* lwip_htonl */

#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#ifndef lwip_strnstr
/**
 * @ingroup sys_nonstandard
 * lwIP default implementation for strnstr() non-standard function.
 * This can be \#defined to strnstr() depending on your platform port.
 */
char *
lwip_strnstr(const char *buffer, const char *token, size_t n)
{
  const char *p;
  size_t tokenlen = strlen(token);
  if (tokenlen == 0) {
    return LWIP_CONST_CAST(char *, buffer);
  }
  for (p = buffer; *p && (p + tokenlen <= buffer + n); p++) {
    if ((*p == *token) && (strncmp(p, token, tokenlen) == 0)) {
      return LWIP_CONST_CAST(char *, p);
    }
  }
  return NULL;
}
#endif

#ifndef lwip_stricmp
/**
 * @ingroup sys_nonstandard
 * lwIP default implementation for stricmp() non-standard function.
 * This can be \#defined to stricmp() depending on your platform port.
 */
int
lwip_stricmp(const char *str1, const char *str2)
{
  char c1, c2;

  do {
    c1 = *str1++;
    c2 = *str2++;
    if (c1 != c2) {
      char c1_upc = c1 | 0x20;
      if ((c1_upc >= 'a') && (c1_upc <= 'z')) {
        /* characters are not equal an one is in the alphabet range:
        downcase both chars and check again */
        char c2_upc = c2 | 0x20;
        if (c1_upc != c2_upc) {
          /* still not equal */
          /* don't care for < or > */
          return 1;
        }
      } else {
        /* characters are not equal but none is in the alphabet range */
        return 1;
      }
    }
  } while (c1 != 0);
  return 0;
}
#endif

#ifndef lwip_strnicmp
/**
 * @ingroup sys_nonstandard
 * lwIP default implementation for strnicmp() non-standard function.
 * This can be \#defined to strnicmp() depending on your platform port.
 */
int
lwip_strnicmp(const char *str1, const char *str2, size_t len)
{
  char c1, c2;

  do {
    c1 = *str1++;
    c2 = *str2++;
    if (c1 != c2) {
      char c1_upc = c1 | 0x20;
      if ((c1_upc >= 'a') && (c1_upc <= 'z')) {
        /* characters are not equal an one is in the alphabet range:
        downcase both chars and check again */
        char c2_upc = c2 | 0x20;
        if (c1_upc != c2_upc) {
          /* still not equal */
          /* don't care for < or > */
          return 1;
        }
      } else {
        /* characters are not equal but none is in the alphabet range */
        return 1;
      }
    }
  } while (len-- && c1 != 0);
  return 0;
}
#endif

#ifndef lwip_itoa
/**
 * @ingroup sys_nonstandard
 * lwIP default implementation for itoa() non-standard function.
 * This can be \#defined to itoa() or snprintf(result, bufsize, "%d", number) depending on your platform port.
 */
void
lwip_itoa(char *result, size_t bufsize, int number)
{
  char *res = result;
  char *tmp = result;
  size_t res_left = bufsize;
  size_t result_len;
  int n = (number > 0) ? number : -number;

  /* handle invalid bufsize */
  if (bufsize < 2) {
    if (bufsize == 1) {
      *result = 0;
    }
    return;
  }

  /* ensure output string is zero terminated */
  result[bufsize - 1] = 0;
  result_len = 1;
  /* create the string in a temporary buffer since we don't know how long
     it will get */
  tmp = &result[bufsize - 2];
  if (n == 0) {
    *tmp = '0';
    tmp--;
    result_len++;
  }
  while ((n != 0) && (result_len < (bufsize - 1))) {
    char val = (char)('0' + (n % 10));
    *tmp = val;
    tmp--;
    n = n / 10;
    result_len++;
  }

  /* output sign first */
  if (number < 0) {
    *res = '-';
    res++;
    res_left--;
  }
  if (result_len > res_left) {
    /* buffer is too small */
    result[0] = '.';
    result[1] = 0;
    return;
  }
  /* copy from temporary buffer to output buffer */
  memmove(res, tmp + 1, result_len);
}
#endif
