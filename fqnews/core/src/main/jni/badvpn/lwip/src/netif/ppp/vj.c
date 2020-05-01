/*
 * Routines to compress and uncompess tcp packets (for transmission
 * over low speed serial lines.
 *
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Van Jacobson (van@helios.ee.lbl.gov), Dec 31, 1989:
 *   Initial distribution.
 *
 * Modified June 1993 by Paul Mackerras, paulus@cs.anu.edu.au,
 * so that the entire packet being decompressed doesn't have
 * to be in contiguous memory (just the compressed header).
 *
 * Modified March 1998 by Guy Lancaster, glanca@gesn.com,
 * for a 16 bit processor.
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && VJ_SUPPORT /* don't build if not configured for use in lwipopts.h */

#include "netif/ppp/ppp_impl.h"
#include "netif/ppp/pppdebug.h"

#include "netif/ppp/vj.h"

#include <string.h>

#if LINK_STATS
#define INCR(counter) ++comp->stats.counter
#else
#define INCR(counter)
#endif

void
vj_compress_init(struct vjcompress *comp)
{
  u8_t i;
  struct cstate *tstate = comp->tstate;

#if MAX_SLOTS == 0
  memset((char *)comp, 0, sizeof(*comp));
#endif
  comp->maxSlotIndex = MAX_SLOTS - 1;
  comp->compressSlot = 0;    /* Disable slot ID compression by default. */
  for (i = MAX_SLOTS - 1; i > 0; --i) {
    tstate[i].cs_id = i;
    tstate[i].cs_next = &tstate[i - 1];
  }
  tstate[0].cs_next = &tstate[MAX_SLOTS - 1];
  tstate[0].cs_id = 0;
  comp->last_cs = &tstate[0];
  comp->last_recv = 255;
  comp->last_xmit = 255;
  comp->flags = VJF_TOSS;
}


/* ENCODE encodes a number that is known to be non-zero.  ENCODEZ
 * checks for zero (since zero has to be encoded in the long, 3 byte
 * form).
 */
#define ENCODE(n) { \
  if ((u16_t)(n) >= 256) { \
    *cp++ = 0; \
    cp[1] = (u8_t)(n); \
    cp[0] = (u8_t)((n) >> 8); \
    cp += 2; \
  } else { \
    *cp++ = (u8_t)(n); \
  } \
}
#define ENCODEZ(n) { \
  if ((u16_t)(n) >= 256 || (u16_t)(n) == 0) { \
    *cp++ = 0; \
    cp[1] = (u8_t)(n); \
    cp[0] = (u8_t)((n) >> 8); \
    cp += 2; \
  } else { \
    *cp++ = (u8_t)(n); \
  } \
}

#define DECODEL(f) { \
  if (*cp == 0) {\
    u32_t tmp_ = lwip_ntohl(f) + ((cp[1] << 8) | cp[2]); \
    (f) = lwip_htonl(tmp_); \
    cp += 3; \
  } else { \
    u32_t tmp_ = lwip_ntohl(f) + (u32_t)*cp++; \
    (f) = lwip_htonl(tmp_); \
  } \
}

#define DECODES(f) { \
  if (*cp == 0) {\
    u16_t tmp_ = lwip_ntohs(f) + (((u16_t)cp[1] << 8) | cp[2]); \
    (f) = lwip_htons(tmp_); \
    cp += 3; \
  } else { \
    u16_t tmp_ = lwip_ntohs(f) + (u16_t)*cp++; \
    (f) = lwip_htons(tmp_); \
  } \
}

#define DECODEU(f) { \
  if (*cp == 0) {\
    (f) = lwip_htons(((u16_t)cp[1] << 8) | cp[2]); \
    cp += 3; \
  } else { \
    (f) = lwip_htons((u16_t)*cp++); \
  } \
}

/* Helper structures for unaligned *u32_t and *u16_t accesses */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct vj_u32_t {
  PACK_STRUCT_FIELD(u32_t v);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct vj_u16_t {
  PACK_STRUCT_FIELD(u16_t v);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/*
 * vj_compress_tcp - Attempt to do Van Jacobson header compression on a
 * packet.  This assumes that nb and comp are not null and that the first
 * buffer of the chain contains a valid IP header.
 * Return the VJ type code indicating whether or not the packet was
 * compressed.
 */
u8_t
vj_compress_tcp(struct vjcompress *comp, struct pbuf **pb)
{
  struct pbuf *np = *pb;
  struct ip_hdr *ip = (struct ip_hdr *)np->payload;
  struct cstate *cs = comp->last_cs->cs_next;
  u16_t ilen = IPH_HL(ip);
  u16_t hlen;
  struct tcp_hdr *oth;
  struct tcp_hdr *th;
  u16_t deltaS, deltaA = 0;
  u32_t deltaL;
  u32_t changes = 0;
  u8_t new_seq[16];
  u8_t *cp = new_seq;

  /*
   * Check that the packet is IP proto TCP.
   */
  if (IPH_PROTO(ip) != IP_PROTO_TCP) {
    return (TYPE_IP);
  }

  /*
   * Bail if this is an IP fragment or if the TCP packet isn't
   * `compressible' (i.e., ACK isn't set or some other control bit is
   * set).
   */
  if ((IPH_OFFSET(ip) & PP_HTONS(0x3fff)) || np->tot_len < 40) {
    return (TYPE_IP);
  }
  th = (struct tcp_hdr *)&((struct vj_u32_t*)ip)[ilen];
  if ((TCPH_FLAGS(th) & (TCP_SYN|TCP_FIN|TCP_RST|TCP_ACK)) != TCP_ACK) {
    return (TYPE_IP);
  }

  /* Check that the TCP/IP headers are contained in the first buffer. */
  hlen = ilen + TCPH_HDRLEN(th);
  hlen <<= 2;
  if (np->len < hlen) {
    PPPDEBUG(LOG_INFO, ("vj_compress_tcp: header len %d spans buffers\n", hlen));
    return (TYPE_IP);
  }

  /* TCP stack requires that we don't change the packet payload, therefore we copy
   * the whole packet before compression. */
  np = pbuf_clone(PBUF_RAW, PBUF_POOL, *pb);
  if (!np) {
    return (TYPE_IP);
  }

  *pb = np;
  ip = (struct ip_hdr *)np->payload;

  /*
   * Packet is compressible -- we're going to send either a
   * COMPRESSED_TCP or UNCOMPRESSED_TCP packet.  Either way we need
   * to locate (or create) the connection state.  Special case the
   * most recently used connection since it's most likely to be used
   * again & we don't have to do any reordering if it's used.
   */
  INCR(vjs_packets);
  if (!ip4_addr_cmp(&ip->src, &cs->cs_ip.src)
      || !ip4_addr_cmp(&ip->dest, &cs->cs_ip.dest)
      || (*(struct vj_u32_t*)th).v != (((struct vj_u32_t*)&cs->cs_ip)[IPH_HL(&cs->cs_ip)]).v) {
    /*
     * Wasn't the first -- search for it.
     *
     * States are kept in a circularly linked list with
     * last_cs pointing to the end of the list.  The
     * list is kept in lru order by moving a state to the
     * head of the list whenever it is referenced.  Since
     * the list is short and, empirically, the connection
     * we want is almost always near the front, we locate
     * states via linear search.  If we don't find a state
     * for the datagram, the oldest state is (re-)used.
     */
    struct cstate *lcs;
    struct cstate *lastcs = comp->last_cs;

    do {
      lcs = cs; cs = cs->cs_next;
      INCR(vjs_searches);
      if (ip4_addr_cmp(&ip->src, &cs->cs_ip.src)
          && ip4_addr_cmp(&ip->dest, &cs->cs_ip.dest)
          && (*(struct vj_u32_t*)th).v == (((struct vj_u32_t*)&cs->cs_ip)[IPH_HL(&cs->cs_ip)]).v) {
        goto found;
      }
    } while (cs != lastcs);

    /*
     * Didn't find it -- re-use oldest cstate.  Send an
     * uncompressed packet that tells the other side what
     * connection number we're using for this conversation.
     * Note that since the state list is circular, the oldest
     * state points to the newest and we only need to set
     * last_cs to update the lru linkage.
     */
    INCR(vjs_misses);
    comp->last_cs = lcs;
    goto uncompressed;

    found:
    /*
     * Found it -- move to the front on the connection list.
     */
    if (cs == lastcs) {
      comp->last_cs = lcs;
    } else {
      lcs->cs_next = cs->cs_next;
      cs->cs_next = lastcs->cs_next;
      lastcs->cs_next = cs;
    }
  }

  oth = (struct tcp_hdr *)&((struct vj_u32_t*)&cs->cs_ip)[ilen];
  deltaS = ilen;

  /*
   * Make sure that only what we expect to change changed. The first
   * line of the `if' checks the IP protocol version, header length &
   * type of service.  The 2nd line checks the "Don't fragment" bit.
   * The 3rd line checks the time-to-live and protocol (the protocol
   * check is unnecessary but costless).  The 4th line checks the TCP
   * header length.  The 5th line checks IP options, if any.  The 6th
   * line checks TCP options, if any.  If any of these things are
   * different between the previous & current datagram, we send the
   * current datagram `uncompressed'.
   */
  if ((((struct vj_u16_t*)ip)[0]).v != (((struct vj_u16_t*)&cs->cs_ip)[0]).v
      || (((struct vj_u16_t*)ip)[3]).v != (((struct vj_u16_t*)&cs->cs_ip)[3]).v
      || (((struct vj_u16_t*)ip)[4]).v != (((struct vj_u16_t*)&cs->cs_ip)[4]).v
      || TCPH_HDRLEN(th) != TCPH_HDRLEN(oth)
      || (deltaS > 5 && BCMP(ip + 1, &cs->cs_ip + 1, (deltaS - 5) << 2))
      || (TCPH_HDRLEN(th) > 5 && BCMP(th + 1, oth + 1, (TCPH_HDRLEN(th) - 5) << 2))) {
    goto uncompressed;
  }

  /*
   * Figure out which of the changing fields changed.  The
   * receiver expects changes in the order: urgent, window,
   * ack, seq (the order minimizes the number of temporaries
   * needed in this section of code).
   */
  if (TCPH_FLAGS(th) & TCP_URG) {
    deltaS = lwip_ntohs(th->urgp);
    ENCODEZ(deltaS);
    changes |= NEW_U;
  } else if (th->urgp != oth->urgp) {
    /* argh! URG not set but urp changed -- a sensible
     * implementation should never do this but RFC793
     * doesn't prohibit the change so we have to deal
     * with it. */
    goto uncompressed;
  }

  if ((deltaS = (u16_t)(lwip_ntohs(th->wnd) - lwip_ntohs(oth->wnd))) != 0) {
    ENCODE(deltaS);
    changes |= NEW_W;
  }

  if ((deltaL = lwip_ntohl(th->ackno) - lwip_ntohl(oth->ackno)) != 0) {
    if (deltaL > 0xffff) {
      goto uncompressed;
    }
    deltaA = (u16_t)deltaL;
    ENCODE(deltaA);
    changes |= NEW_A;
  }

  if ((deltaL = lwip_ntohl(th->seqno) - lwip_ntohl(oth->seqno)) != 0) {
    if (deltaL > 0xffff) {
      goto uncompressed;
    }
    deltaS = (u16_t)deltaL;
    ENCODE(deltaS);
    changes |= NEW_S;
  }

  switch(changes) {
  case 0:
    /*
     * Nothing changed. If this packet contains data and the
     * last one didn't, this is probably a data packet following
     * an ack (normal on an interactive connection) and we send
     * it compressed.  Otherwise it's probably a retransmit,
     * retransmitted ack or window probe.  Send it uncompressed
     * in case the other side missed the compressed version.
     */
    if (IPH_LEN(ip) != IPH_LEN(&cs->cs_ip) &&
      lwip_ntohs(IPH_LEN(&cs->cs_ip)) == hlen) {
      break;
    }
    /* no break */
    /* fall through */

  case SPECIAL_I:
  case SPECIAL_D:
    /*
     * actual changes match one of our special case encodings --
     * send packet uncompressed.
     */
    goto uncompressed;

  case NEW_S|NEW_A:
    if (deltaS == deltaA && deltaS == lwip_ntohs(IPH_LEN(&cs->cs_ip)) - hlen) {
      /* special case for echoed terminal traffic */
      changes = SPECIAL_I;
      cp = new_seq;
    }
    break;

  case NEW_S:
    if (deltaS == lwip_ntohs(IPH_LEN(&cs->cs_ip)) - hlen) {
      /* special case for data xfer */
      changes = SPECIAL_D;
      cp = new_seq;
    }
    break;
  default:
     break;
  }

  deltaS = (u16_t)(lwip_ntohs(IPH_ID(ip)) - lwip_ntohs(IPH_ID(&cs->cs_ip)));
  if (deltaS != 1) {
    ENCODEZ(deltaS);
    changes |= NEW_I;
  }
  if (TCPH_FLAGS(th) & TCP_PSH) {
    changes |= TCP_PUSH_BIT;
  }
  /*
   * Grab the cksum before we overwrite it below.  Then update our
   * state with this packet's header.
   */
  deltaA = lwip_ntohs(th->chksum);
  MEMCPY(&cs->cs_ip, ip, hlen);

  /*
   * We want to use the original packet as our compressed packet.
   * (cp - new_seq) is the number of bytes we need for compressed
   * sequence numbers.  In addition we need one byte for the change
   * mask, one for the connection id and two for the tcp checksum.
   * So, (cp - new_seq) + 4 bytes of header are needed.  hlen is how
   * many bytes of the original packet to toss so subtract the two to
   * get the new packet size.
   */
  deltaS = (u16_t)(cp - new_seq);
  if (!comp->compressSlot || comp->last_xmit != cs->cs_id) {
    comp->last_xmit = cs->cs_id;
    hlen -= deltaS + 4;
    if (pbuf_remove_header(np, hlen)){
      /* Can we cope with this failing?  Just assert for now */
      LWIP_ASSERT("pbuf_remove_header failed\n", 0);
    }
    cp = (u8_t*)np->payload;
    *cp++ = (u8_t)(changes | NEW_C);
    *cp++ = cs->cs_id;
  } else {
    hlen -= deltaS + 3;
    if (pbuf_remove_header(np, hlen)) {
      /* Can we cope with this failing?  Just assert for now */
      LWIP_ASSERT("pbuf_remove_header failed\n", 0);
    }
    cp = (u8_t*)np->payload;
    *cp++ = (u8_t)changes;
  }
  *cp++ = (u8_t)(deltaA >> 8);
  *cp++ = (u8_t)deltaA;
  MEMCPY(cp, new_seq, deltaS);
  INCR(vjs_compressed);
  return (TYPE_COMPRESSED_TCP);

  /*
   * Update connection state cs & send uncompressed packet (that is,
   * a regular ip/tcp packet but with the 'conversation id' we hope
   * to use on future compressed packets in the protocol field).
   */
uncompressed:
  MEMCPY(&cs->cs_ip, ip, hlen);
  IPH_PROTO_SET(ip, cs->cs_id);
  comp->last_xmit = cs->cs_id;
  return (TYPE_UNCOMPRESSED_TCP);
}

/*
 * Called when we may have missed a packet.
 */
void
vj_uncompress_err(struct vjcompress *comp)
{
  comp->flags |= VJF_TOSS;
  INCR(vjs_errorin);
}

/*
 * "Uncompress" a packet of type TYPE_UNCOMPRESSED_TCP.
 * Return 0 on success, -1 on failure.
 */
int
vj_uncompress_uncomp(struct pbuf *nb, struct vjcompress *comp)
{
  u32_t hlen;
  struct cstate *cs;
  struct ip_hdr *ip;

  ip = (struct ip_hdr *)nb->payload;
  hlen = IPH_HL(ip) << 2;
  if (IPH_PROTO(ip) >= MAX_SLOTS
      || hlen + sizeof(struct tcp_hdr) > nb->len
      || (hlen += TCPH_HDRLEN_BYTES((struct tcp_hdr *)&((char *)ip)[hlen]))
          > nb->len
      || hlen > MAX_HDR) {
    PPPDEBUG(LOG_INFO, ("vj_uncompress_uncomp: bad cid=%d, hlen=%d buflen=%d\n",
      IPH_PROTO(ip), hlen, nb->len));
    vj_uncompress_err(comp);
    return -1;
  }
  cs = &comp->rstate[comp->last_recv = IPH_PROTO(ip)];
  comp->flags &=~ VJF_TOSS;
  IPH_PROTO_SET(ip, IP_PROTO_TCP);
  /* copy from/to bigger buffers checked above instead of cs->cs_ip and ip
     just to help static code analysis to see this is correct ;-) */
  MEMCPY(&cs->cs_hdr, nb->payload, hlen);
  cs->cs_hlen = (u16_t)hlen;
  INCR(vjs_uncompressedin);
  return 0;
}

/*
 * Uncompress a packet of type TYPE_COMPRESSED_TCP.
 * The packet is composed of a buffer chain and the first buffer
 * must contain an accurate chain length.
 * The first buffer must include the entire compressed TCP/IP header.
 * This procedure replaces the compressed header with the uncompressed
 * header and returns the length of the VJ header.
 */
int
vj_uncompress_tcp(struct pbuf **nb, struct vjcompress *comp)
{
  u8_t *cp;
  struct tcp_hdr *th;
  struct cstate *cs;
  struct vj_u16_t *bp;
  struct pbuf *n0 = *nb;
  u32_t tmp;
  u32_t vjlen, hlen, changes;

  INCR(vjs_compressedin);
  cp = (u8_t*)n0->payload;
  changes = *cp++;
  if (changes & NEW_C) {
    /*
     * Make sure the state index is in range, then grab the state.
     * If we have a good state index, clear the 'discard' flag.
     */
    if (*cp >= MAX_SLOTS) {
      PPPDEBUG(LOG_INFO, ("vj_uncompress_tcp: bad cid=%d\n", *cp));
      goto bad;
    }

    comp->flags &=~ VJF_TOSS;
    comp->last_recv = *cp++;
  } else {
    /*
     * this packet has an implicit state index.  If we've
     * had a line error since the last time we got an
     * explicit state index, we have to toss the packet.
     */
    if (comp->flags & VJF_TOSS) {
      PPPDEBUG(LOG_INFO, ("vj_uncompress_tcp: tossing\n"));
      INCR(vjs_tossed);
      return (-1);
    }
  }
  cs = &comp->rstate[comp->last_recv];
  hlen = IPH_HL(&cs->cs_ip) << 2;
  th = (struct tcp_hdr *)&((u8_t*)&cs->cs_ip)[hlen];
  th->chksum = lwip_htons((*cp << 8) | cp[1]);
  cp += 2;
  if (changes & TCP_PUSH_BIT) {
    TCPH_SET_FLAG(th, TCP_PSH);
  } else {
    TCPH_UNSET_FLAG(th, TCP_PSH);
  }

  switch (changes & SPECIALS_MASK) {
  case SPECIAL_I:
    {
      u32_t i = lwip_ntohs(IPH_LEN(&cs->cs_ip)) - cs->cs_hlen;
      /* some compilers can't nest inline assembler.. */
      tmp = lwip_ntohl(th->ackno) + i;
      th->ackno = lwip_htonl(tmp);
      tmp = lwip_ntohl(th->seqno) + i;
      th->seqno = lwip_htonl(tmp);
    }
    break;

  case SPECIAL_D:
    /* some compilers can't nest inline assembler.. */
    tmp = lwip_ntohl(th->seqno) + lwip_ntohs(IPH_LEN(&cs->cs_ip)) - cs->cs_hlen;
    th->seqno = lwip_htonl(tmp);
    break;

  default:
    if (changes & NEW_U) {
      TCPH_SET_FLAG(th, TCP_URG);
      DECODEU(th->urgp);
    } else {
      TCPH_UNSET_FLAG(th, TCP_URG);
    }
    if (changes & NEW_W) {
      DECODES(th->wnd);
    }
    if (changes & NEW_A) {
      DECODEL(th->ackno);
    }
    if (changes & NEW_S) {
      DECODEL(th->seqno);
    }
    break;
  }
  if (changes & NEW_I) {
    DECODES(cs->cs_ip._id);
  } else {
    IPH_ID_SET(&cs->cs_ip, lwip_ntohs(IPH_ID(&cs->cs_ip)) + 1);
    IPH_ID_SET(&cs->cs_ip, lwip_htons(IPH_ID(&cs->cs_ip)));
  }

  /*
   * At this point, cp points to the first byte of data in the
   * packet.  Fill in the IP total length and update the IP
   * header checksum.
   */
  vjlen = (u16_t)(cp - (u8_t*)n0->payload);
  if (n0->len < vjlen) {
    /*
     * We must have dropped some characters (crc should detect
     * this but the old slip framing won't)
     */
    PPPDEBUG(LOG_INFO, ("vj_uncompress_tcp: head buffer %d too short %d\n",
          n0->len, vjlen));
    goto bad;
  }

#if BYTE_ORDER == LITTLE_ENDIAN
  tmp = n0->tot_len - vjlen + cs->cs_hlen;
  IPH_LEN_SET(&cs->cs_ip, lwip_htons((u16_t)tmp));
#else
  IPH_LEN_SET(&cs->cs_ip, lwip_htons(n0->tot_len - vjlen + cs->cs_hlen));
#endif

  /* recompute the ip header checksum */
  bp = (struct vj_u16_t*) &cs->cs_ip;
  IPH_CHKSUM_SET(&cs->cs_ip, 0);
  for (tmp = 0; hlen > 0; hlen -= 2) {
    tmp += (*bp++).v;
  }
  tmp = (tmp & 0xffff) + (tmp >> 16);
  tmp = (tmp & 0xffff) + (tmp >> 16);
  IPH_CHKSUM_SET(&cs->cs_ip,  (u16_t)(~tmp));

  /* Remove the compressed header and prepend the uncompressed header. */
  if (pbuf_remove_header(n0, vjlen)) {
    /* Can we cope with this failing?  Just assert for now */
    LWIP_ASSERT("pbuf_remove_header failed\n", 0);
    goto bad;
  }

  if(LWIP_MEM_ALIGN(n0->payload) != n0->payload) {
    struct pbuf *np;

#if IP_FORWARD
    /* If IP forwarding is enabled we are using a PBUF_LINK packet type so
     * the packet is being allocated with enough header space to be
     * forwarded (to Ethernet for example).
     */
    np = pbuf_alloc(PBUF_LINK, n0->len + cs->cs_hlen, PBUF_POOL);
#else /* IP_FORWARD */
    np = pbuf_alloc(PBUF_RAW, n0->len + cs->cs_hlen, PBUF_POOL);
#endif /* IP_FORWARD */
    if(!np) {
      PPPDEBUG(LOG_WARNING, ("vj_uncompress_tcp: realign failed\n"));
      goto bad;
    }

    if (pbuf_remove_header(np, cs->cs_hlen)) {
      /* Can we cope with this failing?  Just assert for now */
      LWIP_ASSERT("pbuf_remove_header failed\n", 0);
      goto bad;
    }

    pbuf_take(np, n0->payload, n0->len);

    if(n0->next) {
      pbuf_chain(np, n0->next);
      pbuf_dechain(n0);
    }
    pbuf_free(n0);
    n0 = np;
  }

  if (pbuf_add_header(n0, cs->cs_hlen)) {
    struct pbuf *np;

    LWIP_ASSERT("vj_uncompress_tcp: cs->cs_hlen <= PBUF_POOL_BUFSIZE", cs->cs_hlen <= PBUF_POOL_BUFSIZE);
    np = pbuf_alloc(PBUF_RAW, cs->cs_hlen, PBUF_POOL);
    if(!np) {
      PPPDEBUG(LOG_WARNING, ("vj_uncompress_tcp: prepend failed\n"));
      goto bad;
    }
    pbuf_cat(np, n0);
    n0 = np;
  }
  LWIP_ASSERT("n0->len >= cs->cs_hlen", n0->len >= cs->cs_hlen);
  MEMCPY(n0->payload, &cs->cs_ip, cs->cs_hlen);

  *nb = n0;

  return vjlen;

bad:
  vj_uncompress_err(comp);
  return (-1);
}

#endif /* PPP_SUPPORT && VJ_SUPPORT */
