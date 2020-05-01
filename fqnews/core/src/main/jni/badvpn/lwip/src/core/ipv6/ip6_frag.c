/**
 * @file
 *
 * IPv6 fragmentation and reassembly.
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
 *
 * Please coordinate changes and requests with Ivan Delamer
 * <delamer@inicotech.com>
 */

#include "lwip/opt.h"
#include "lwip/ip6_frag.h"
#include "lwip/ip6.h"
#include "lwip/icmp6.h"
#include "lwip/nd6.h"
#include "lwip/ip.h"

#include "lwip/pbuf.h"
#include "lwip/memp.h"
#include "lwip/stats.h"

#include <string.h>

#if LWIP_IPV6 && LWIP_IPV6_REASS  /* don't build if not configured for use in lwipopts.h */


/** Setting this to 0, you can turn off checking the fragments for overlapping
 * regions. The code gets a little smaller. Only use this if you know that
 * overlapping won't occur on your network! */
#ifndef IP_REASS_CHECK_OVERLAP
#define IP_REASS_CHECK_OVERLAP 1
#endif /* IP_REASS_CHECK_OVERLAP */

/** Set to 0 to prevent freeing the oldest datagram when the reassembly buffer is
 * full (IP_REASS_MAX_PBUFS pbufs are enqueued). The code gets a little smaller.
 * Datagrams will be freed by timeout only. Especially useful when MEMP_NUM_REASSDATA
 * is set to 1, so one datagram can be reassembled at a time, only. */
#ifndef IP_REASS_FREE_OLDEST
#define IP_REASS_FREE_OLDEST 1
#endif /* IP_REASS_FREE_OLDEST */

#if IPV6_FRAG_COPYHEADER
/* The number of bytes we need to "borrow" from (i.e., overwrite in) the header
 * that precedes the fragment header for reassembly pruposes. */
#define IPV6_FRAG_REQROOM ((s16_t)(sizeof(struct ip6_reass_helper) - IP6_FRAG_HLEN))
#endif

#define IP_REASS_FLAG_LASTFRAG 0x01

/** This is a helper struct which holds the starting
 * offset and the ending offset of this fragment to
 * easily chain the fragments.
 * It has the same packing requirements as the IPv6 header, since it replaces
 * the Fragment Header in memory in incoming fragments to keep
 * track of the various fragments.
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct ip6_reass_helper {
  PACK_STRUCT_FIELD(struct pbuf *next_pbuf);
  PACK_STRUCT_FIELD(u16_t start);
  PACK_STRUCT_FIELD(u16_t end);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* static variables */
static struct ip6_reassdata *reassdatagrams;
static u16_t ip6_reass_pbufcount;

/* Forward declarations. */
static void ip6_reass_free_complete_datagram(struct ip6_reassdata *ipr);
#if IP_REASS_FREE_OLDEST
static void ip6_reass_remove_oldest_datagram(struct ip6_reassdata *ipr, int pbufs_needed);
#endif /* IP_REASS_FREE_OLDEST */

void
ip6_reass_tmr(void)
{
  struct ip6_reassdata *r, *tmp;

#if !IPV6_FRAG_COPYHEADER
  LWIP_ASSERT("sizeof(struct ip6_reass_helper) <= IP6_FRAG_HLEN, set IPV6_FRAG_COPYHEADER to 1",
    sizeof(struct ip6_reass_helper) <= IP6_FRAG_HLEN);
#endif /* !IPV6_FRAG_COPYHEADER */

  r = reassdatagrams;
  while (r != NULL) {
    /* Decrement the timer. Once it reaches 0,
     * clean up the incomplete fragment assembly */
    if (r->timer > 0) {
      r->timer--;
      r = r->next;
    } else {
      /* reassembly timed out */
      tmp = r;
      /* get the next pointer before freeing */
      r = r->next;
      /* free the helper struct and all enqueued pbufs */
      ip6_reass_free_complete_datagram(tmp);
     }
   }
}

/**
 * Free a datagram (struct ip6_reassdata) and all its pbufs.
 * Updates the total count of enqueued pbufs (ip6_reass_pbufcount),
 * sends an ICMP time exceeded packet.
 *
 * @param ipr datagram to free
 */
static void
ip6_reass_free_complete_datagram(struct ip6_reassdata *ipr)
{
  struct ip6_reassdata *prev;
  u16_t pbufs_freed = 0;
  u16_t clen;
  struct pbuf *p;
  struct ip6_reass_helper *iprh;

#if LWIP_ICMP6
  iprh = (struct ip6_reass_helper *)ipr->p->payload;
  if (iprh->start == 0) {
    /* The first fragment was received, send ICMP time exceeded. */
    /* First, de-queue the first pbuf from r->p. */
    p = ipr->p;
    ipr->p = iprh->next_pbuf;
    /* Restore the part that we've overwritten with our helper structure, or we
     * might send garbage (and disclose a pointer) in the ICMPv6 reply. */
    MEMCPY(p->payload, ipr->orig_hdr, sizeof(iprh));
    /* Then, move back to the original ipv6 header (we are now pointing to Fragment header).
       This cannot fail since we already checked when receiving this fragment. */
    if (pbuf_header_force(p, (s16_t)((u8_t*)p->payload - (u8_t*)ipr->iphdr))) {
      LWIP_ASSERT("ip6_reass_free: moving p->payload to ip6 header failed\n", 0);
    }
    else {
      /* Reconstruct the zoned source and destination addresses, so that we do
       * not end up sending the ICMP response over the wrong link. */
      ip6_addr_t src_addr, dest_addr;
      ip6_addr_copy_from_packed(src_addr, IPV6_FRAG_SRC(ipr));
      ip6_addr_set_zone(&src_addr, ipr->src_zone);
      ip6_addr_copy_from_packed(dest_addr, IPV6_FRAG_DEST(ipr));
      ip6_addr_set_zone(&dest_addr, ipr->dest_zone);
      /* Send the actual ICMP response. */
      icmp6_time_exceeded_with_addrs(p, ICMP6_TE_FRAG, &src_addr, &dest_addr);
    }
    clen = pbuf_clen(p);
    LWIP_ASSERT("pbufs_freed + clen <= 0xffff", pbufs_freed + clen <= 0xffff);
    pbufs_freed = (u16_t)(pbufs_freed + clen);
    pbuf_free(p);
  }
#endif /* LWIP_ICMP6 */

  /* First, free all received pbufs.  The individual pbufs need to be released
     separately as they have not yet been chained */
  p = ipr->p;
  while (p != NULL) {
    struct pbuf *pcur;
    iprh = (struct ip6_reass_helper *)p->payload;
    pcur = p;
    /* get the next pointer before freeing */
    p = iprh->next_pbuf;
    clen = pbuf_clen(pcur);
    LWIP_ASSERT("pbufs_freed + clen <= 0xffff", pbufs_freed + clen <= 0xffff);
    pbufs_freed = (u16_t)(pbufs_freed + clen);
    pbuf_free(pcur);
  }

  /* Then, unchain the struct ip6_reassdata from the list and free it. */
  if (ipr == reassdatagrams) {
    reassdatagrams = ipr->next;
  } else {
    prev = reassdatagrams;
    while (prev != NULL) {
      if (prev->next == ipr) {
        break;
      }
      prev = prev->next;
    }
    if (prev != NULL) {
      prev->next = ipr->next;
    }
  }
  memp_free(MEMP_IP6_REASSDATA, ipr);

  /* Finally, update number of pbufs in reassembly queue */
  LWIP_ASSERT("ip_reass_pbufcount >= clen", ip6_reass_pbufcount >= pbufs_freed);
  ip6_reass_pbufcount = (u16_t)(ip6_reass_pbufcount - pbufs_freed);
}

#if IP_REASS_FREE_OLDEST
/**
 * Free the oldest datagram to make room for enqueueing new fragments.
 * The datagram ipr is not freed!
 *
 * @param ipr ip6_reassdata for the current fragment
 * @param pbufs_needed number of pbufs needed to enqueue
 *        (used for freeing other datagrams if not enough space)
 */
static void
ip6_reass_remove_oldest_datagram(struct ip6_reassdata *ipr, int pbufs_needed)
{
  struct ip6_reassdata *r, *oldest;

  /* Free datagrams until being allowed to enqueue 'pbufs_needed' pbufs,
   * but don't free the current datagram! */
  do {
    r = oldest = reassdatagrams;
    while (r != NULL) {
      if (r != ipr) {
        if (r->timer <= oldest->timer) {
          /* older than the previous oldest */
          oldest = r;
        }
      }
      r = r->next;
    }
    if (oldest == ipr) {
      /* nothing to free, ipr is the only element on the list */
      return;
    }
    if (oldest != NULL) {
      ip6_reass_free_complete_datagram(oldest);
    }
  } while (((ip6_reass_pbufcount + pbufs_needed) > IP_REASS_MAX_PBUFS) && (reassdatagrams != NULL));
}
#endif /* IP_REASS_FREE_OLDEST */

/**
 * Reassembles incoming IPv6 fragments into an IPv6 datagram.
 *
 * @param p points to the IPv6 Fragment Header
 * @return NULL if reassembly is incomplete, pbuf pointing to
 *         IPv6 Header if reassembly is complete
 */
struct pbuf *
ip6_reass(struct pbuf *p)
{
  struct ip6_reassdata *ipr, *ipr_prev;
  struct ip6_reass_helper *iprh, *iprh_tmp, *iprh_prev=NULL;
  struct ip6_frag_hdr *frag_hdr;
  u16_t offset, len, start, end;
  ptrdiff_t hdrdiff;
  u16_t clen;
  u8_t valid = 1;
  struct pbuf *q, *next_pbuf;

  IP6_FRAG_STATS_INC(ip6_frag.recv);

  /* ip6_frag_hdr must be in the first pbuf, not chained. Checked by caller. */
  LWIP_ASSERT("IPv6 fragment header does not fit in first pbuf",
    p->len >= sizeof(struct ip6_frag_hdr));

  frag_hdr = (struct ip6_frag_hdr *) p->payload;

  clen = pbuf_clen(p);

  offset = lwip_ntohs(frag_hdr->_fragment_offset);

  /* Calculate fragment length from IPv6 payload length.
   * Adjust for headers before Fragment Header.
   * And finally adjust by Fragment Header length. */
  len = lwip_ntohs(ip6_current_header()->_plen);
  hdrdiff = (u8_t*)p->payload - (const u8_t*)ip6_current_header();
  LWIP_ASSERT("not a valid pbuf (ip6_input check missing?)", hdrdiff <= 0xFFFF);
  LWIP_ASSERT("not a valid pbuf (ip6_input check missing?)", hdrdiff >= IP6_HLEN);
  hdrdiff -= IP6_HLEN;
  hdrdiff += IP6_FRAG_HLEN;
  if (hdrdiff > len) {
    IP6_FRAG_STATS_INC(ip6_frag.proterr);
    goto nullreturn;
  }
  len = (u16_t)(len - hdrdiff);
  start = (offset & IP6_FRAG_OFFSET_MASK);
  if (start > (0xFFFF - len)) {
    /* u16_t overflow, cannot handle this */
    IP6_FRAG_STATS_INC(ip6_frag.proterr);
    goto nullreturn;
  }

  /* Look for the datagram the fragment belongs to in the current datagram queue,
   * remembering the previous in the queue for later dequeueing. */
  for (ipr = reassdatagrams, ipr_prev = NULL; ipr != NULL; ipr = ipr->next) {
    /* Check if the incoming fragment matches the one currently present
       in the reassembly buffer. If so, we proceed with copying the
       fragment into the buffer. */
    if ((frag_hdr->_identification == ipr->identification) &&
        ip6_addr_cmp_packed(ip6_current_src_addr(), &(IPV6_FRAG_SRC(ipr)), ipr->src_zone) &&
        ip6_addr_cmp_packed(ip6_current_dest_addr(), &(IPV6_FRAG_DEST(ipr)), ipr->dest_zone)) {
      IP6_FRAG_STATS_INC(ip6_frag.cachehit);
      break;
    }
    ipr_prev = ipr;
  }

  if (ipr == NULL) {
  /* Enqueue a new datagram into the datagram queue */
    ipr = (struct ip6_reassdata *)memp_malloc(MEMP_IP6_REASSDATA);
    if (ipr == NULL) {
#if IP_REASS_FREE_OLDEST
      /* Make room and try again. */
      ip6_reass_remove_oldest_datagram(ipr, clen);
      ipr = (struct ip6_reassdata *)memp_malloc(MEMP_IP6_REASSDATA);
      if (ipr != NULL) {
        /* re-search ipr_prev since it might have been removed */
        for (ipr_prev = reassdatagrams; ipr_prev != NULL; ipr_prev = ipr_prev->next) {
          if (ipr_prev->next == ipr) {
            break;
          }
        }
      } else
#endif /* IP_REASS_FREE_OLDEST */
      {
        IP6_FRAG_STATS_INC(ip6_frag.memerr);
        goto nullreturn;
      }
    }

    memset(ipr, 0, sizeof(struct ip6_reassdata));
    ipr->timer = IPV6_REASS_MAXAGE;

    /* enqueue the new structure to the front of the list */
    ipr->next = reassdatagrams;
    reassdatagrams = ipr;

    /* Use the current IPv6 header for src/dest address reference.
     * Eventually, we will replace it when we get the first fragment
     * (it might be this one, in any case, it is done later). */
    /* need to use the none-const pointer here: */
    ipr->iphdr = ip_data.current_ip6_header;
#if IPV6_FRAG_COPYHEADER
    MEMCPY(&ipr->src, &ip6_current_header()->src, sizeof(ipr->src));
    MEMCPY(&ipr->dest, &ip6_current_header()->dest, sizeof(ipr->dest));
#endif /* IPV6_FRAG_COPYHEADER */
#if LWIP_IPV6_SCOPES
    /* Also store the address zone information.
     * @todo It is possible that due to netif destruction and recreation, the
     * stored zones end up resolving to a different interface. In that case, we
     * risk sending a "time exceeded" ICMP response over the wrong link.
     * Ideally, netif destruction would clean up matching pending reassembly
     * structures, but custom zone mappings would make that non-trivial. */
    ipr->src_zone = ip6_addr_zone(ip6_current_src_addr());
    ipr->dest_zone = ip6_addr_zone(ip6_current_dest_addr());
#endif /* LWIP_IPV6_SCOPES */
    /* copy the fragmented packet id. */
    ipr->identification = frag_hdr->_identification;

    /* copy the nexth field */
    ipr->nexth = frag_hdr->_nexth;
  }

  /* Check if we are allowed to enqueue more datagrams. */
  if ((ip6_reass_pbufcount + clen) > IP_REASS_MAX_PBUFS) {
#if IP_REASS_FREE_OLDEST
    ip6_reass_remove_oldest_datagram(ipr, clen);
    if ((ip6_reass_pbufcount + clen) <= IP_REASS_MAX_PBUFS) {
      /* re-search ipr_prev since it might have been removed */
      for (ipr_prev = reassdatagrams; ipr_prev != NULL; ipr_prev = ipr_prev->next) {
        if (ipr_prev->next == ipr) {
          break;
        }
      }
    } else
#endif /* IP_REASS_FREE_OLDEST */
    {
      /* @todo: send ICMPv6 time exceeded here? */
      /* drop this pbuf */
      IP6_FRAG_STATS_INC(ip6_frag.memerr);
      goto nullreturn;
    }
  }

  /* Overwrite Fragment Header with our own helper struct. */
#if IPV6_FRAG_COPYHEADER
  if (IPV6_FRAG_REQROOM > 0) {
    /* Make room for struct ip6_reass_helper (only required if sizeof(void*) > 4).
       This cannot fail since we already checked when receiving this fragment. */
    u8_t hdrerr = pbuf_header_force(p, IPV6_FRAG_REQROOM);
    LWIP_UNUSED_ARG(hdrerr); /* in case of LWIP_NOASSERT */
    LWIP_ASSERT("no room for struct ip6_reass_helper", hdrerr == 0);
  }
#else /* IPV6_FRAG_COPYHEADER */
  LWIP_ASSERT("sizeof(struct ip6_reass_helper) <= IP6_FRAG_HLEN, set IPV6_FRAG_COPYHEADER to 1",
    sizeof(struct ip6_reass_helper) <= IP6_FRAG_HLEN);
#endif /* IPV6_FRAG_COPYHEADER */

  /* Prepare the pointer to the helper structure, and its initial values.
   * Do not yet write to the structure itself, as we still have to make a
   * backup of the original data, and we should not do that until we know for
   * sure that we are going to add this packet to the list. */
  iprh = (struct ip6_reass_helper *)p->payload;
  next_pbuf = NULL;
  end = (u16_t)(start + len);

  /* find the right place to insert this pbuf */
  /* Iterate through until we either get to the end of the list (append),
   * or we find on with a larger offset (insert). */
  for (q = ipr->p; q != NULL;) {
    iprh_tmp = (struct ip6_reass_helper*)q->payload;
    if (start < iprh_tmp->start) {
#if IP_REASS_CHECK_OVERLAP
      if (end > iprh_tmp->start) {
        /* fragment overlaps with following, throw away */
        IP6_FRAG_STATS_INC(ip6_frag.proterr);
        goto nullreturn;
      }
      if (iprh_prev != NULL) {
        if (start < iprh_prev->end) {
          /* fragment overlaps with previous, throw away */
          IP6_FRAG_STATS_INC(ip6_frag.proterr);
          goto nullreturn;
        }
      }
#endif /* IP_REASS_CHECK_OVERLAP */
      /* the new pbuf should be inserted before this */
      next_pbuf = q;
      if (iprh_prev != NULL) {
        /* not the fragment with the lowest offset */
        iprh_prev->next_pbuf = p;
      } else {
        /* fragment with the lowest offset */
        ipr->p = p;
      }
      break;
    } else if (start == iprh_tmp->start) {
      /* received the same datagram twice: no need to keep the datagram */
      goto nullreturn;
#if IP_REASS_CHECK_OVERLAP
    } else if (start < iprh_tmp->end) {
      /* overlap: no need to keep the new datagram */
      IP6_FRAG_STATS_INC(ip6_frag.proterr);
      goto nullreturn;
#endif /* IP_REASS_CHECK_OVERLAP */
    } else {
      /* Check if the fragments received so far have no gaps. */
      if (iprh_prev != NULL) {
        if (iprh_prev->end != iprh_tmp->start) {
          /* There is a fragment missing between the current
           * and the previous fragment */
          valid = 0;
        }
      }
    }
    q = iprh_tmp->next_pbuf;
    iprh_prev = iprh_tmp;
  }

  /* If q is NULL, then we made it to the end of the list. Determine what to do now */
  if (q == NULL) {
    if (iprh_prev != NULL) {
      /* this is (for now), the fragment with the highest offset:
       * chain it to the last fragment */
#if IP_REASS_CHECK_OVERLAP
      LWIP_ASSERT("check fragments don't overlap", iprh_prev->end <= start);
#endif /* IP_REASS_CHECK_OVERLAP */
      iprh_prev->next_pbuf = p;
      if (iprh_prev->end != start) {
        valid = 0;
      }
    } else {
#if IP_REASS_CHECK_OVERLAP
      LWIP_ASSERT("no previous fragment, this must be the first fragment!",
        ipr->p == NULL);
#endif /* IP_REASS_CHECK_OVERLAP */
      /* this is the first fragment we ever received for this ip datagram */
      ipr->p = p;
    }
  }

  /* Track the current number of pbufs current 'in-flight', in order to limit
  the number of fragments that may be enqueued at any one time */
  ip6_reass_pbufcount = (u16_t)(ip6_reass_pbufcount + clen);

  /* Remember IPv6 header if this is the first fragment. */
  if (start == 0) {
    /* need to use the none-const pointer here: */
    ipr->iphdr = ip_data.current_ip6_header;
    /* Make a backup of the part of the packet data that we are about to
     * overwrite, so that we can restore the original later. */
    MEMCPY(ipr->orig_hdr, p->payload, sizeof(*iprh));
    /* For IPV6_FRAG_COPYHEADER there is no need to copy src/dst again, as they
     * will be the same as they were. With LWIP_IPV6_SCOPES, the same applies
     * to the source/destination zones. */
  }
  /* Only after the backup do we get to fill in the actual helper structure. */
  iprh->next_pbuf = next_pbuf;
  iprh->start = start;
  iprh->end = end;

  /* If this is the last fragment, calculate total packet length. */
  if ((offset & IP6_FRAG_MORE_FLAG) == 0) {
    ipr->datagram_len = iprh->end;
  }

  /* Additional validity tests: we have received first and last fragment. */
  iprh_tmp = (struct ip6_reass_helper*)ipr->p->payload;
  if (iprh_tmp->start != 0) {
    valid = 0;
  }
  if (ipr->datagram_len == 0) {
    valid = 0;
  }

  /* Final validity test: no gaps between current and last fragment. */
  iprh_prev = iprh;
  q = iprh->next_pbuf;
  while ((q != NULL) && valid) {
    iprh = (struct ip6_reass_helper*)q->payload;
    if (iprh_prev->end != iprh->start) {
      valid = 0;
      break;
    }
    iprh_prev = iprh;
    q = iprh->next_pbuf;
  }

  if (valid) {
    /* All fragments have been received */
    struct ip6_hdr* iphdr_ptr;

    /* chain together the pbufs contained within the ip6_reassdata list. */
    iprh = (struct ip6_reass_helper*) ipr->p->payload;
    while (iprh != NULL) {
      next_pbuf = iprh->next_pbuf;
      if (next_pbuf != NULL) {
        /* Save next helper struct (will be hidden in next step). */
        iprh_tmp = (struct ip6_reass_helper*)next_pbuf->payload;

        /* hide the fragment header for every succeeding fragment */
        pbuf_remove_header(next_pbuf, IP6_FRAG_HLEN);
#if IPV6_FRAG_COPYHEADER
        if (IPV6_FRAG_REQROOM > 0) {
          /* hide the extra bytes borrowed from ip6_hdr for struct ip6_reass_helper */
          u8_t hdrerr = pbuf_remove_header(next_pbuf, IPV6_FRAG_REQROOM);
          LWIP_UNUSED_ARG(hdrerr); /* in case of LWIP_NOASSERT */
          LWIP_ASSERT("no room for struct ip6_reass_helper", hdrerr == 0);
        }
#endif
        pbuf_cat(ipr->p, next_pbuf);
      }
      else {
        iprh_tmp = NULL;
      }

      iprh = iprh_tmp;
    }

    /* Get the first pbuf. */
    p = ipr->p;

#if IPV6_FRAG_COPYHEADER
    if (IPV6_FRAG_REQROOM > 0) {
      u8_t hdrerr;
      /* Restore (only) the bytes that we overwrote beyond the fragment header.
       * Those bytes may belong to either the IPv6 header or an extension
       * header placed before the fragment header. */
      MEMCPY(p->payload, ipr->orig_hdr, IPV6_FRAG_REQROOM);
      /* get back room for struct ip6_reass_helper (only required if sizeof(void*) > 4) */
      hdrerr = pbuf_remove_header(p, IPV6_FRAG_REQROOM);
      LWIP_UNUSED_ARG(hdrerr); /* in case of LWIP_NOASSERT */
      LWIP_ASSERT("no room for struct ip6_reass_helper", hdrerr == 0);
    }
#endif

    /* We need to get rid of the fragment header itself, which is somewhere in
     * the middle of the packet (but still in the first pbuf of the chain).
     * Getting rid of the header is required by RFC 2460 Sec. 4.5 and necessary
     * in order to be able to reassemble packets that are close to full size
     * (i.e., around 65535 bytes). We simply move up all the headers before the
     * fragment header, including the IPv6 header, and adjust the payload start
     * accordingly. This works because all these headers are in the first pbuf
     * of the chain, and because the caller adjusts all its pointers on
     * successful reassembly. */
    MEMMOVE((u8_t*)ipr->iphdr + sizeof(struct ip6_frag_hdr), ipr->iphdr,
      (size_t)((u8_t*)p->payload - (u8_t*)ipr->iphdr));

    /* This is where the IPv6 header is now. */
    iphdr_ptr = (struct ip6_hdr*)((u8_t*)ipr->iphdr +
      sizeof(struct ip6_frag_hdr));

    /* Adjust datagram length by adding header lengths. */
    ipr->datagram_len = (u16_t)(ipr->datagram_len + ((u8_t*)p->payload - (u8_t*)iphdr_ptr)
                         - IP6_HLEN);

    /* Set payload length in ip header. */
    iphdr_ptr->_plen = lwip_htons(ipr->datagram_len);

    /* With the fragment header gone, we now need to adjust the next-header
     * field of whatever header was originally before it. Since the packet made
     * it through the original header processing routines at least up to the
     * fragment header, we do not need any further sanity checks here. */
    if (IP6H_NEXTH(iphdr_ptr) == IP6_NEXTH_FRAGMENT) {
      iphdr_ptr->_nexth = ipr->nexth;
    } else {
      u8_t *ptr = (u8_t *)iphdr_ptr + IP6_HLEN;
      while (*ptr != IP6_NEXTH_FRAGMENT) {
        ptr += 8 * (1 + ptr[1]);
      }
      *ptr = ipr->nexth;
    }

    /* release the resources allocated for the fragment queue entry */
    if (reassdatagrams == ipr) {
      /* it was the first in the list */
      reassdatagrams = ipr->next;
    } else {
      /* it wasn't the first, so it must have a valid 'prev' */
      LWIP_ASSERT("sanity check linked list", ipr_prev != NULL);
      ipr_prev->next = ipr->next;
    }
    memp_free(MEMP_IP6_REASSDATA, ipr);

    /* adjust the number of pbufs currently queued for reassembly. */
    clen = pbuf_clen(p);
    LWIP_ASSERT("ip6_reass_pbufcount >= clen", ip6_reass_pbufcount >= clen);
    ip6_reass_pbufcount = (u16_t)(ip6_reass_pbufcount - clen);

    /* Move pbuf back to IPv6 header. This should never fail. */
    if (pbuf_header_force(p, (s16_t)((u8_t*)p->payload - (u8_t*)iphdr_ptr))) {
      LWIP_ASSERT("ip6_reass: moving p->payload to ip6 header failed\n", 0);
      pbuf_free(p);
      return NULL;
    }

    /* Return the pbuf chain */
    return p;
  }
  /* the datagram is not (yet?) reassembled completely */
  return NULL;

nullreturn:
  IP6_FRAG_STATS_INC(ip6_frag.drop);
  pbuf_free(p);
  return NULL;
}

#endif /* LWIP_IPV6 && LWIP_IPV6_REASS */

#if LWIP_IPV6 && LWIP_IPV6_FRAG

#if !LWIP_NETIF_TX_SINGLE_PBUF
/** Allocate a new struct pbuf_custom_ref */
static struct pbuf_custom_ref*
ip6_frag_alloc_pbuf_custom_ref(void)
{
  return (struct pbuf_custom_ref*)memp_malloc(MEMP_FRAG_PBUF);
}

/** Free a struct pbuf_custom_ref */
static void
ip6_frag_free_pbuf_custom_ref(struct pbuf_custom_ref* p)
{
  LWIP_ASSERT("p != NULL", p != NULL);
  memp_free(MEMP_FRAG_PBUF, p);
}

/** Free-callback function to free a 'struct pbuf_custom_ref', called by
 * pbuf_free. */
static void
ip6_frag_free_pbuf_custom(struct pbuf *p)
{
  struct pbuf_custom_ref *pcr = (struct pbuf_custom_ref*)p;
  LWIP_ASSERT("pcr != NULL", pcr != NULL);
  LWIP_ASSERT("pcr == p", (void*)pcr == (void*)p);
  if (pcr->original != NULL) {
    pbuf_free(pcr->original);
  }
  ip6_frag_free_pbuf_custom_ref(pcr);
}
#endif /* !LWIP_NETIF_TX_SINGLE_PBUF */

/**
 * Fragment an IPv6 datagram if too large for the netif or path MTU.
 *
 * Chop the datagram in MTU sized chunks and send them in order
 * by pointing PBUF_REFs into p
 *
 * @param p ipv6 packet to send
 * @param netif the netif on which to send
 * @param dest destination ipv6 address to which to send
 *
 * @return ERR_OK if sent successfully, err_t otherwise
 */
err_t
ip6_frag(struct pbuf *p, struct netif *netif, const ip6_addr_t *dest)
{
  struct ip6_hdr *original_ip6hdr;
  struct ip6_hdr *ip6hdr;
  struct ip6_frag_hdr *frag_hdr;
  struct pbuf *rambuf;
#if !LWIP_NETIF_TX_SINGLE_PBUF
  struct pbuf *newpbuf;
  u16_t newpbuflen = 0;
  u16_t left_to_copy;
#endif
  static u32_t identification;
  u16_t left, cop;
  const u16_t mtu = nd6_get_destination_mtu(dest, netif);
  const u16_t nfb = (u16_t)((mtu - (IP6_HLEN + IP6_FRAG_HLEN)) & IP6_FRAG_OFFSET_MASK);
  u16_t fragment_offset = 0;
  u16_t last;
  u16_t poff = IP6_HLEN;

  identification++;

  original_ip6hdr = (struct ip6_hdr *)p->payload;

  /* @todo we assume there are no options in the unfragmentable part (IPv6 header). */
  LWIP_ASSERT("p->tot_len >= IP6_HLEN", p->tot_len >= IP6_HLEN);
  left = (u16_t)(p->tot_len - IP6_HLEN);

  while (left) {
    last = (left <= nfb);

    /* Fill this fragment */
    cop = last ? left : nfb;

#if LWIP_NETIF_TX_SINGLE_PBUF
    rambuf = pbuf_alloc(PBUF_IP, cop + IP6_FRAG_HLEN, PBUF_RAM);
    if (rambuf == NULL) {
      IP6_FRAG_STATS_INC(ip6_frag.memerr);
      return ERR_MEM;
    }
    LWIP_ASSERT("this needs a pbuf in one piece!",
      (rambuf->len == rambuf->tot_len) && (rambuf->next == NULL));
    poff += pbuf_copy_partial(p, (u8_t*)rambuf->payload + IP6_FRAG_HLEN, cop, poff);
    /* make room for the IP header */
    if (pbuf_add_header(rambuf, IP6_HLEN)) {
      pbuf_free(rambuf);
      IP6_FRAG_STATS_INC(ip6_frag.memerr);
      return ERR_MEM;
    }
    /* fill in the IP header */
    SMEMCPY(rambuf->payload, original_ip6hdr, IP6_HLEN);
    ip6hdr = (struct ip6_hdr *)rambuf->payload;
    frag_hdr = (struct ip6_frag_hdr *)((u8_t*)rambuf->payload + IP6_HLEN);
#else
    /* When not using a static buffer, create a chain of pbufs.
     * The first will be a PBUF_RAM holding the link, IPv6, and Fragment header.
     * The rest will be PBUF_REFs mirroring the pbuf chain to be fragged,
     * but limited to the size of an mtu.
     */
    rambuf = pbuf_alloc(PBUF_LINK, IP6_HLEN + IP6_FRAG_HLEN, PBUF_RAM);
    if (rambuf == NULL) {
      IP6_FRAG_STATS_INC(ip6_frag.memerr);
      return ERR_MEM;
    }
    LWIP_ASSERT("this needs a pbuf in one piece!",
                (p->len >= (IP6_HLEN)));
    SMEMCPY(rambuf->payload, original_ip6hdr, IP6_HLEN);
    ip6hdr = (struct ip6_hdr *)rambuf->payload;
    frag_hdr = (struct ip6_frag_hdr *)((u8_t*)rambuf->payload + IP6_HLEN);

    /* Can just adjust p directly for needed offset. */
    p->payload = (u8_t *)p->payload + poff;
    p->len = (u16_t)(p->len - poff);
    p->tot_len = (u16_t)(p->tot_len - poff);

    left_to_copy = cop;
    while (left_to_copy) {
      struct pbuf_custom_ref *pcr;
      newpbuflen = (left_to_copy < p->len) ? left_to_copy : p->len;
      /* Is this pbuf already empty? */
      if (!newpbuflen) {
        p = p->next;
        continue;
      }
      pcr = ip6_frag_alloc_pbuf_custom_ref();
      if (pcr == NULL) {
        pbuf_free(rambuf);
        IP6_FRAG_STATS_INC(ip6_frag.memerr);
        return ERR_MEM;
      }
      /* Mirror this pbuf, although we might not need all of it. */
      newpbuf = pbuf_alloced_custom(PBUF_RAW, newpbuflen, PBUF_REF, &pcr->pc, p->payload, newpbuflen);
      if (newpbuf == NULL) {
        ip6_frag_free_pbuf_custom_ref(pcr);
        pbuf_free(rambuf);
        IP6_FRAG_STATS_INC(ip6_frag.memerr);
        return ERR_MEM;
      }
      pbuf_ref(p);
      pcr->original = p;
      pcr->pc.custom_free_function = ip6_frag_free_pbuf_custom;

      /* Add it to end of rambuf's chain, but using pbuf_cat, not pbuf_chain
       * so that it is removed when pbuf_dechain is later called on rambuf.
       */
      pbuf_cat(rambuf, newpbuf);
      left_to_copy = (u16_t)(left_to_copy - newpbuflen);
      if (left_to_copy) {
        p = p->next;
      }
    }
    poff = newpbuflen;
#endif /* LWIP_NETIF_TX_SINGLE_PBUF */

    /* Set headers */
    frag_hdr->_nexth = original_ip6hdr->_nexth;
    frag_hdr->reserved = 0;
    frag_hdr->_fragment_offset = lwip_htons((u16_t)((fragment_offset & IP6_FRAG_OFFSET_MASK) | (last ? 0 : IP6_FRAG_MORE_FLAG)));
    frag_hdr->_identification = lwip_htonl(identification);

    IP6H_NEXTH_SET(ip6hdr, IP6_NEXTH_FRAGMENT);
    IP6H_PLEN_SET(ip6hdr, (u16_t)(cop + IP6_FRAG_HLEN));

    /* No need for separate header pbuf - we allowed room for it in rambuf
     * when allocated.
     */
    IP6_FRAG_STATS_INC(ip6_frag.xmit);
    netif->output_ip6(netif, rambuf, dest);

    /* Unfortunately we can't reuse rambuf - the hardware may still be
     * using the buffer. Instead we free it (and the ensuing chain) and
     * recreate it next time round the loop. If we're lucky the hardware
     * will have already sent the packet, the free will really free, and
     * there will be zero memory penalty.
     */

    pbuf_free(rambuf);
    left = (u16_t)(left - cop);
    fragment_offset = (u16_t)(fragment_offset + cop);
  }
  return ERR_OK;
}

#endif /* LWIP_IPV6 && LWIP_IPV6_FRAG */
