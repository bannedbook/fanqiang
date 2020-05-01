/**
 * @file
 * Incluse internet checksum functions.\n
 *
 * These are some reference implementations of the checksum algorithm, with the
 * aim of being simple, correct and fully portable. Checksumming is the
 * first thing you would want to optimize for your platform. If you create
 * your own version, link it in and in your cc.h put:
 *
 * \#define LWIP_CHKSUM your_checksum_routine
 *
 * Or you can select from the implementations below by defining
 * LWIP_CHKSUM_ALGORITHM to 1, 2 or 3.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "lwip/opt.h"

#include "lwip/inet_chksum.h"
#include "lwip/def.h"
#include "lwip/ip_addr.h"

#include <string.h>

#ifndef LWIP_CHKSUM
# define LWIP_CHKSUM lwip_standard_chksum
# ifndef LWIP_CHKSUM_ALGORITHM
#  define LWIP_CHKSUM_ALGORITHM 2
# endif
u16_t lwip_standard_chksum(const void *dataptr, int len);
#endif
/* If none set: */
#ifndef LWIP_CHKSUM_ALGORITHM
# define LWIP_CHKSUM_ALGORITHM 0
#endif

#if (LWIP_CHKSUM_ALGORITHM == 1) /* Version #1 */
/**
 * lwip checksum
 *
 * @param dataptr points to start of data to be summed at any boundary
 * @param len length of data to be summed
 * @return host order (!) lwip checksum (non-inverted Internet sum)
 *
 * @note accumulator size limits summable length to 64k
 * @note host endianess is irrelevant (p3 RFC1071)
 */
u16_t
lwip_standard_chksum(const void *dataptr, int len)
{
  u32_t acc;
  u16_t src;
  const u8_t *octetptr;

  acc = 0;
  /* dataptr may be at odd or even addresses */
  octetptr = (const u8_t *)dataptr;
  while (len > 1) {
    /* declare first octet as most significant
       thus assume network order, ignoring host order */
    src = (*octetptr) << 8;
    octetptr++;
    /* declare second octet as least significant */
    src |= (*octetptr);
    octetptr++;
    acc += src;
    len -= 2;
  }
  if (len > 0) {
    /* accumulate remaining octet */
    src = (*octetptr) << 8;
    acc += src;
  }
  /* add deferred carry bits */
  acc = (acc >> 16) + (acc & 0x0000ffffUL);
  if ((acc & 0xffff0000UL) != 0) {
    acc = (acc >> 16) + (acc & 0x0000ffffUL);
  }
  /* This maybe a little confusing: reorder sum using lwip_htons()
     instead of lwip_ntohs() since it has a little less call overhead.
     The caller must invert bits for Internet sum ! */
  return lwip_htons((u16_t)acc);
}
#endif

#if (LWIP_CHKSUM_ALGORITHM == 2) /* Alternative version #2 */
/*
 * Curt McDowell
 * Broadcom Corp.
 * csm@broadcom.com
 *
 * IP checksum two bytes at a time with support for
 * unaligned buffer.
 * Works for len up to and including 0x20000.
 * by Curt McDowell, Broadcom Corp. 12/08/2005
 *
 * @param dataptr points to start of data to be summed at any boundary
 * @param len length of data to be summed
 * @return host order (!) lwip checksum (non-inverted Internet sum)
 */
u16_t
lwip_standard_chksum(const void *dataptr, int len)
{
  const u8_t *pb = (const u8_t *)dataptr;
  const u16_t *ps;
  u16_t t = 0;
  u32_t sum = 0;
  int odd = ((mem_ptr_t)pb & 1);

  /* Get aligned to u16_t */
  if (odd && len > 0) {
    ((u8_t *)&t)[1] = *pb++;
    len--;
  }

  /* Add the bulk of the data */
  ps = (const u16_t *)(const void *)pb;
  while (len > 1) {
    sum += *ps++;
    len -= 2;
  }

  /* Consume left-over byte, if any */
  if (len > 0) {
    ((u8_t *)&t)[0] = *(const u8_t *)ps;
  }

  /* Add end bytes */
  sum += t;

  /* Fold 32-bit sum to 16 bits
     calling this twice is probably faster than if statements... */
  sum = FOLD_U32T(sum);
  sum = FOLD_U32T(sum);

  /* Swap if alignment was odd */
  if (odd) {
    sum = SWAP_BYTES_IN_WORD(sum);
  }

  return (u16_t)sum;
}
#endif

#if (LWIP_CHKSUM_ALGORITHM == 3) /* Alternative version #3 */
/**
 * An optimized checksum routine. Basically, it uses loop-unrolling on
 * the checksum loop, treating the head and tail bytes specially, whereas
 * the inner loop acts on 8 bytes at a time.
 *
 * @arg start of buffer to be checksummed. May be an odd byte address.
 * @len number of bytes in the buffer to be checksummed.
 * @return host order (!) lwip checksum (non-inverted Internet sum)
 *
 * by Curt McDowell, Broadcom Corp. December 8th, 2005
 */
u16_t
lwip_standard_chksum(const void *dataptr, int len)
{
  const u8_t *pb = (const u8_t *)dataptr;
  const u16_t *ps;
  u16_t t = 0;
  const u32_t *pl;
  u32_t sum = 0, tmp;
  /* starts at odd byte address? */
  int odd = ((mem_ptr_t)pb & 1);

  if (odd && len > 0) {
    ((u8_t *)&t)[1] = *pb++;
    len--;
  }

  ps = (const u16_t *)(const void *)pb;

  if (((mem_ptr_t)ps & 3) && len > 1) {
    sum += *ps++;
    len -= 2;
  }

  pl = (const u32_t *)(const void *)ps;

  while (len > 7)  {
    tmp = sum + *pl++;          /* ping */
    if (tmp < sum) {
      tmp++;                    /* add back carry */
    }

    sum = tmp + *pl++;          /* pong */
    if (sum < tmp) {
      sum++;                    /* add back carry */
    }

    len -= 8;
  }

  /* make room in upper bits */
  sum = FOLD_U32T(sum);

  ps = (const u16_t *)pl;

  /* 16-bit aligned word remaining? */
  while (len > 1) {
    sum += *ps++;
    len -= 2;
  }

  /* dangling tail byte remaining? */
  if (len > 0) {                /* include odd byte */
    ((u8_t *)&t)[0] = *(const u8_t *)ps;
  }

  sum += t;                     /* add end bytes */

  /* Fold 32-bit sum to 16 bits
     calling this twice is probably faster than if statements... */
  sum = FOLD_U32T(sum);
  sum = FOLD_U32T(sum);

  if (odd) {
    sum = SWAP_BYTES_IN_WORD(sum);
  }

  return (u16_t)sum;
}
#endif

/** Parts of the pseudo checksum which are common to IPv4 and IPv6 */
static u16_t
inet_cksum_pseudo_base(struct pbuf *p, u8_t proto, u16_t proto_len, u32_t acc)
{
  struct pbuf *q;
  int swapped = 0;

  /* iterate through all pbuf in chain */
  for (q = p; q != NULL; q = q->next) {
    LWIP_DEBUGF(INET_DEBUG, ("inet_chksum_pseudo(): checksumming pbuf %p (has next %p) \n",
                             (void *)q, (void *)q->next));
    acc += LWIP_CHKSUM(q->payload, q->len);
    /*LWIP_DEBUGF(INET_DEBUG, ("inet_chksum_pseudo(): unwrapped lwip_chksum()=%"X32_F" \n", acc));*/
    /* just executing this next line is probably faster that the if statement needed
       to check whether we really need to execute it, and does no harm */
    acc = FOLD_U32T(acc);
    if (q->len % 2 != 0) {
      swapped = !swapped;
      acc = SWAP_BYTES_IN_WORD(acc);
    }
    /*LWIP_DEBUGF(INET_DEBUG, ("inet_chksum_pseudo(): wrapped lwip_chksum()=%"X32_F" \n", acc));*/
  }

  if (swapped) {
    acc = SWAP_BYTES_IN_WORD(acc);
  }

  acc += (u32_t)lwip_htons((u16_t)proto);
  acc += (u32_t)lwip_htons(proto_len);

  /* Fold 32-bit sum to 16 bits
     calling this twice is probably faster than if statements... */
  acc = FOLD_U32T(acc);
  acc = FOLD_U32T(acc);
  LWIP_DEBUGF(INET_DEBUG, ("inet_chksum_pseudo(): pbuf chain lwip_chksum()=%"X32_F"\n", acc));
  return (u16_t)~(acc & 0xffffUL);
}

#if LWIP_IPV4
/* inet_chksum_pseudo:
 *
 * Calculates the IPv4 pseudo Internet checksum used by TCP and UDP for a pbuf chain.
 * IP addresses are expected to be in network byte order.
 *
 * @param p chain of pbufs over that a checksum should be calculated (ip data part)
 * @param src source ip address (used for checksum of pseudo header)
 * @param dst destination ip address (used for checksum of pseudo header)
 * @param proto ip protocol (used for checksum of pseudo header)
 * @param proto_len length of the ip data part (used for checksum of pseudo header)
 * @return checksum (as u16_t) to be saved directly in the protocol header
 */
u16_t
inet_chksum_pseudo(struct pbuf *p, u8_t proto, u16_t proto_len,
                   const ip4_addr_t *src, const ip4_addr_t *dest)
{
  u32_t acc;
  u32_t addr;

  addr = ip4_addr_get_u32(src);
  acc = (addr & 0xffffUL);
  acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
  addr = ip4_addr_get_u32(dest);
  acc = (u32_t)(acc + (addr & 0xffffUL));
  acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
  /* fold down to 16 bits */
  acc = FOLD_U32T(acc);
  acc = FOLD_U32T(acc);

  return inet_cksum_pseudo_base(p, proto, proto_len, acc);
}
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
/**
 * Calculates the checksum with IPv6 pseudo header used by TCP and UDP for a pbuf chain.
 * IPv6 addresses are expected to be in network byte order.
 *
 * @param p chain of pbufs over that a checksum should be calculated (ip data part)
 * @param proto ipv6 protocol/next header (used for checksum of pseudo header)
 * @param proto_len length of the ipv6 payload (used for checksum of pseudo header)
 * @param src source ipv6 address (used for checksum of pseudo header)
 * @param dest destination ipv6 address (used for checksum of pseudo header)
 * @return checksum (as u16_t) to be saved directly in the protocol header
 */
u16_t
ip6_chksum_pseudo(struct pbuf *p, u8_t proto, u16_t proto_len,
                  const ip6_addr_t *src, const ip6_addr_t *dest)
{
  u32_t acc = 0;
  u32_t addr;
  u8_t addr_part;

  for (addr_part = 0; addr_part < 4; addr_part++) {
    addr = src->addr[addr_part];
    acc = (u32_t)(acc + (addr & 0xffffUL));
    acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
    addr = dest->addr[addr_part];
    acc = (u32_t)(acc + (addr & 0xffffUL));
    acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
  }
  /* fold down to 16 bits */
  acc = FOLD_U32T(acc);
  acc = FOLD_U32T(acc);

  return inet_cksum_pseudo_base(p, proto, proto_len, acc);
}
#endif /* LWIP_IPV6 */

/* ip_chksum_pseudo:
 *
 * Calculates the IPv4 or IPv6 pseudo Internet checksum used by TCP and UDP for a pbuf chain.
 * IP addresses are expected to be in network byte order.
 *
 * @param p chain of pbufs over that a checksum should be calculated (ip data part)
 * @param src source ip address (used for checksum of pseudo header)
 * @param dst destination ip address (used for checksum of pseudo header)
 * @param proto ip protocol (used for checksum of pseudo header)
 * @param proto_len length of the ip data part (used for checksum of pseudo header)
 * @return checksum (as u16_t) to be saved directly in the protocol header
 */
u16_t
ip_chksum_pseudo(struct pbuf *p, u8_t proto, u16_t proto_len,
                 const ip_addr_t *src, const ip_addr_t *dest)
{
#if LWIP_IPV6
  if (IP_IS_V6(dest)) {
    return ip6_chksum_pseudo(p, proto, proto_len, ip_2_ip6(src), ip_2_ip6(dest));
  }
#endif /* LWIP_IPV6 */
#if LWIP_IPV4 && LWIP_IPV6
  else
#endif /* LWIP_IPV4 && LWIP_IPV6 */
#if LWIP_IPV4
  {
    return inet_chksum_pseudo(p, proto, proto_len, ip_2_ip4(src), ip_2_ip4(dest));
  }
#endif /* LWIP_IPV4 */
}

/** Parts of the pseudo checksum which are common to IPv4 and IPv6 */
static u16_t
inet_cksum_pseudo_partial_base(struct pbuf *p, u8_t proto, u16_t proto_len,
                               u16_t chksum_len, u32_t acc)
{
  struct pbuf *q;
  int swapped = 0;
  u16_t chklen;

  /* iterate through all pbuf in chain */
  for (q = p; (q != NULL) && (chksum_len > 0); q = q->next) {
    LWIP_DEBUGF(INET_DEBUG, ("inet_chksum_pseudo(): checksumming pbuf %p (has next %p) \n",
                             (void *)q, (void *)q->next));
    chklen = q->len;
    if (chklen > chksum_len) {
      chklen = chksum_len;
    }
    acc += LWIP_CHKSUM(q->payload, chklen);
    chksum_len = (u16_t)(chksum_len - chklen);
    LWIP_ASSERT("delete me", chksum_len < 0x7fff);
    /*LWIP_DEBUGF(INET_DEBUG, ("inet_chksum_pseudo(): unwrapped lwip_chksum()=%"X32_F" \n", acc));*/
    /* fold the upper bit down */
    acc = FOLD_U32T(acc);
    if (q->len % 2 != 0) {
      swapped = !swapped;
      acc = SWAP_BYTES_IN_WORD(acc);
    }
    /*LWIP_DEBUGF(INET_DEBUG, ("inet_chksum_pseudo(): wrapped lwip_chksum()=%"X32_F" \n", acc));*/
  }

  if (swapped) {
    acc = SWAP_BYTES_IN_WORD(acc);
  }

  acc += (u32_t)lwip_htons((u16_t)proto);
  acc += (u32_t)lwip_htons(proto_len);

  /* Fold 32-bit sum to 16 bits
     calling this twice is probably faster than if statements... */
  acc = FOLD_U32T(acc);
  acc = FOLD_U32T(acc);
  LWIP_DEBUGF(INET_DEBUG, ("inet_chksum_pseudo(): pbuf chain lwip_chksum()=%"X32_F"\n", acc));
  return (u16_t)~(acc & 0xffffUL);
}

#if LWIP_IPV4
/* inet_chksum_pseudo_partial:
 *
 * Calculates the IPv4 pseudo Internet checksum used by TCP and UDP for a pbuf chain.
 * IP addresses are expected to be in network byte order.
 *
 * @param p chain of pbufs over that a checksum should be calculated (ip data part)
 * @param src source ip address (used for checksum of pseudo header)
 * @param dst destination ip address (used for checksum of pseudo header)
 * @param proto ip protocol (used for checksum of pseudo header)
 * @param proto_len length of the ip data part (used for checksum of pseudo header)
 * @return checksum (as u16_t) to be saved directly in the protocol header
 */
u16_t
inet_chksum_pseudo_partial(struct pbuf *p, u8_t proto, u16_t proto_len,
                           u16_t chksum_len, const ip4_addr_t *src, const ip4_addr_t *dest)
{
  u32_t acc;
  u32_t addr;

  addr = ip4_addr_get_u32(src);
  acc = (addr & 0xffffUL);
  acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
  addr = ip4_addr_get_u32(dest);
  acc = (u32_t)(acc + (addr & 0xffffUL));
  acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
  /* fold down to 16 bits */
  acc = FOLD_U32T(acc);
  acc = FOLD_U32T(acc);

  return inet_cksum_pseudo_partial_base(p, proto, proto_len, chksum_len, acc);
}
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
/**
 * Calculates the checksum with IPv6 pseudo header used by TCP and UDP for a pbuf chain.
 * IPv6 addresses are expected to be in network byte order. Will only compute for a
 * portion of the payload.
 *
 * @param p chain of pbufs over that a checksum should be calculated (ip data part)
 * @param proto ipv6 protocol/next header (used for checksum of pseudo header)
 * @param proto_len length of the ipv6 payload (used for checksum of pseudo header)
 * @param chksum_len number of payload bytes used to compute chksum
 * @param src source ipv6 address (used for checksum of pseudo header)
 * @param dest destination ipv6 address (used for checksum of pseudo header)
 * @return checksum (as u16_t) to be saved directly in the protocol header
 */
u16_t
ip6_chksum_pseudo_partial(struct pbuf *p, u8_t proto, u16_t proto_len,
                          u16_t chksum_len, const ip6_addr_t *src, const ip6_addr_t *dest)
{
  u32_t acc = 0;
  u32_t addr;
  u8_t addr_part;

  for (addr_part = 0; addr_part < 4; addr_part++) {
    addr = src->addr[addr_part];
    acc = (u32_t)(acc + (addr & 0xffffUL));
    acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
    addr = dest->addr[addr_part];
    acc = (u32_t)(acc + (addr & 0xffffUL));
    acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
  }
  /* fold down to 16 bits */
  acc = FOLD_U32T(acc);
  acc = FOLD_U32T(acc);

  return inet_cksum_pseudo_partial_base(p, proto, proto_len, chksum_len, acc);
}
#endif /* LWIP_IPV6 */

/* ip_chksum_pseudo_partial:
 *
 * Calculates the IPv4 or IPv6 pseudo Internet checksum used by TCP and UDP for a pbuf chain.
 *
 * @param p chain of pbufs over that a checksum should be calculated (ip data part)
 * @param src source ip address (used for checksum of pseudo header)
 * @param dst destination ip address (used for checksum of pseudo header)
 * @param proto ip protocol (used for checksum of pseudo header)
 * @param proto_len length of the ip data part (used for checksum of pseudo header)
 * @return checksum (as u16_t) to be saved directly in the protocol header
 */
u16_t
ip_chksum_pseudo_partial(struct pbuf *p, u8_t proto, u16_t proto_len,
                         u16_t chksum_len, const ip_addr_t *src, const ip_addr_t *dest)
{
#if LWIP_IPV6
  if (IP_IS_V6(dest)) {
    return ip6_chksum_pseudo_partial(p, proto, proto_len, chksum_len, ip_2_ip6(src), ip_2_ip6(dest));
  }
#endif /* LWIP_IPV6 */
#if LWIP_IPV4 && LWIP_IPV6
  else
#endif /* LWIP_IPV4 && LWIP_IPV6 */
#if LWIP_IPV4
  {
    return inet_chksum_pseudo_partial(p, proto, proto_len, chksum_len, ip_2_ip4(src), ip_2_ip4(dest));
  }
#endif /* LWIP_IPV4 */
}

/* inet_chksum:
 *
 * Calculates the Internet checksum over a portion of memory. Used primarily for IP
 * and ICMP.
 *
 * @param dataptr start of the buffer to calculate the checksum (no alignment needed)
 * @param len length of the buffer to calculate the checksum
 * @return checksum (as u16_t) to be saved directly in the protocol header
 */

u16_t
inet_chksum(const void *dataptr, u16_t len)
{
  return (u16_t)~(unsigned int)LWIP_CHKSUM(dataptr, len);
}

/**
 * Calculate a checksum over a chain of pbufs (without pseudo-header, much like
 * inet_chksum only pbufs are used).
 *
 * @param p pbuf chain over that the checksum should be calculated
 * @return checksum (as u16_t) to be saved directly in the protocol header
 */
u16_t
inet_chksum_pbuf(struct pbuf *p)
{
  u32_t acc;
  struct pbuf *q;
  int swapped = 0;

  acc = 0;
  for (q = p; q != NULL; q = q->next) {
    acc += LWIP_CHKSUM(q->payload, q->len);
    acc = FOLD_U32T(acc);
    if (q->len % 2 != 0) {
      swapped = !swapped;
      acc = SWAP_BYTES_IN_WORD(acc);
    }
  }

  if (swapped) {
    acc = SWAP_BYTES_IN_WORD(acc);
  }
  return (u16_t)~(acc & 0xffffUL);
}

/* These are some implementations for LWIP_CHKSUM_COPY, which copies data
 * like MEMCPY but generates a checksum at the same time. Since this is a
 * performance-sensitive function, you might want to create your own version
 * in assembly targeted at your hardware by defining it in lwipopts.h:
 *   #define LWIP_CHKSUM_COPY(dst, src, len) your_chksum_copy(dst, src, len)
 */

#if (LWIP_CHKSUM_COPY_ALGORITHM == 1) /* Version #1 */
/** Safe but slow: first call MEMCPY, then call LWIP_CHKSUM.
 * For architectures with big caches, data might still be in cache when
 * generating the checksum after copying.
 */
u16_t
lwip_chksum_copy(void *dst, const void *src, u16_t len)
{
  MEMCPY(dst, src, len);
  return LWIP_CHKSUM(dst, len);
}
#endif /* (LWIP_CHKSUM_COPY_ALGORITHM == 1) */
