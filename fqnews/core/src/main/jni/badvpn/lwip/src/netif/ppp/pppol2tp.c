/**
 * @file
 * Network Point to Point Protocol over Layer 2 Tunneling Protocol program file.
 *
 */

/*
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
 */

/*
 * L2TP Support status:
 *
 * Supported:
 * - L2TPv2 (PPP over L2TP, a.k.a. UDP tunnels)
 * - LAC
 *
 * Not supported:
 * - LNS (require PPP server support)
 * - L2TPv3 ethernet pseudowires
 * - L2TPv3 VLAN pseudowire
 * - L2TPv3 PPP pseudowires
 * - L2TPv3 IP encapsulation
 * - L2TPv3 IP pseudowire
 * - L2TP tunnel switching - http://tools.ietf.org/html/draft-ietf-l2tpext-tunnel-switching-08
 * - Multiple tunnels per UDP socket, as well as multiple sessions per tunnel
 * - Hidden AVPs
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && PPPOL2TP_SUPPORT /* don't build if not configured for use in lwipopts.h */

#include "lwip/err.h"
#include "lwip/memp.h"
#include "lwip/netif.h"
#include "lwip/udp.h"
#include "lwip/snmp.h"

#include "netif/ppp/ppp_impl.h"
#include "netif/ppp/lcp.h"
#include "netif/ppp/ipcp.h"
#include "netif/ppp/pppol2tp.h"
#include "netif/ppp/pppcrypt.h"
#include "netif/ppp/magic.h"

/* Memory pool */
LWIP_MEMPOOL_DECLARE(PPPOL2TP_PCB, MEMP_NUM_PPPOL2TP_INTERFACES, sizeof(pppol2tp_pcb), "PPPOL2TP_PCB")

/* callbacks called from PPP core */
static err_t pppol2tp_write(ppp_pcb *ppp, void *ctx, struct pbuf *p);
static err_t pppol2tp_netif_output(ppp_pcb *ppp, void *ctx, struct pbuf *p, u_short protocol);
static err_t pppol2tp_destroy(ppp_pcb *ppp, void *ctx);    /* Destroy a L2TP control block */
static void pppol2tp_connect(ppp_pcb *ppp, void *ctx);    /* Be a LAC, connect to a LNS. */
static void pppol2tp_disconnect(ppp_pcb *ppp, void *ctx);  /* Disconnect */

 /* Prototypes for procedures local to this file. */
static void pppol2tp_input(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static void pppol2tp_dispatch_control_packet(pppol2tp_pcb *l2tp, u16_t port, struct pbuf *p, u16_t ns, u16_t nr);
static void pppol2tp_timeout(void *arg);
static void pppol2tp_abort_connect(pppol2tp_pcb *l2tp);
static err_t pppol2tp_send_sccrq(pppol2tp_pcb *l2tp);
static err_t pppol2tp_send_scccn(pppol2tp_pcb *l2tp, u16_t ns);
static err_t pppol2tp_send_icrq(pppol2tp_pcb *l2tp, u16_t ns);
static err_t pppol2tp_send_iccn(pppol2tp_pcb *l2tp, u16_t ns);
static err_t pppol2tp_send_zlb(pppol2tp_pcb *l2tp, u16_t ns);
static err_t pppol2tp_send_stopccn(pppol2tp_pcb *l2tp, u16_t ns);
static err_t pppol2tp_xmit(pppol2tp_pcb *l2tp, struct pbuf *pb);
static err_t pppol2tp_udp_send(pppol2tp_pcb *l2tp, struct pbuf *pb);

/* Callbacks structure for PPP core */
static const struct link_callbacks pppol2tp_callbacks = {
  pppol2tp_connect,
#if PPP_SERVER
  NULL,
#endif /* PPP_SERVER */
  pppol2tp_disconnect,
  pppol2tp_destroy,
  pppol2tp_write,
  pppol2tp_netif_output,
  NULL,
  NULL
};


/* Create a new L2TP session. */
ppp_pcb *pppol2tp_create(struct netif *pppif,
       struct netif *netif, const ip_addr_t *ipaddr, u16_t port,
       const u8_t *secret, u8_t secret_len,
       ppp_link_status_cb_fn link_status_cb, void *ctx_cb) {
  ppp_pcb *ppp;
  pppol2tp_pcb *l2tp;
  struct udp_pcb *udp;
#if !PPPOL2TP_AUTH_SUPPORT
  LWIP_UNUSED_ARG(secret);
  LWIP_UNUSED_ARG(secret_len);
#endif /* !PPPOL2TP_AUTH_SUPPORT */

  if (ipaddr == NULL) {
    goto ipaddr_check_failed;
  }

  l2tp = (pppol2tp_pcb *)LWIP_MEMPOOL_ALLOC(PPPOL2TP_PCB);
  if (l2tp == NULL) {
    goto memp_malloc_l2tp_failed;
  }

  udp = udp_new_ip_type(IP_GET_TYPE(ipaddr));
  if (udp == NULL) {
    goto udp_new_failed;
  }
  udp_recv(udp, pppol2tp_input, l2tp);

  ppp = ppp_new(pppif, &pppol2tp_callbacks, l2tp, link_status_cb, ctx_cb);
  if (ppp == NULL) {
    goto ppp_new_failed;
  }

  memset(l2tp, 0, sizeof(pppol2tp_pcb));
  l2tp->phase = PPPOL2TP_STATE_INITIAL;
  l2tp->ppp = ppp;
  l2tp->udp = udp;
  l2tp->netif = netif;
  ip_addr_copy(l2tp->remote_ip, *ipaddr);
  l2tp->remote_port = port;
#if PPPOL2TP_AUTH_SUPPORT
  l2tp->secret = secret;
  l2tp->secret_len = secret_len;
#endif /* PPPOL2TP_AUTH_SUPPORT */

  return ppp;

ppp_new_failed:
  udp_remove(udp);
udp_new_failed:
  LWIP_MEMPOOL_FREE(PPPOL2TP_PCB, l2tp);
memp_malloc_l2tp_failed:
ipaddr_check_failed:
  return NULL;
}

/* Called by PPP core */
static err_t pppol2tp_write(ppp_pcb *ppp, void *ctx, struct pbuf *p) {
  pppol2tp_pcb *l2tp = (pppol2tp_pcb *)ctx;
  struct pbuf *ph; /* UDP + L2TP header */
  err_t ret;
#if MIB2_STATS
  u16_t tot_len;
#else /* MIB2_STATS */
  LWIP_UNUSED_ARG(ppp);
#endif /* MIB2_STATS */

  ph = pbuf_alloc(PBUF_TRANSPORT, (u16_t)(PPPOL2TP_OUTPUT_DATA_HEADER_LEN), PBUF_RAM);
  if(!ph) {
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.proterr);
    MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
    pbuf_free(p);
    return ERR_MEM;
  }

  pbuf_remove_header(ph, PPPOL2TP_OUTPUT_DATA_HEADER_LEN); /* hide L2TP header */
  pbuf_cat(ph, p);
#if MIB2_STATS
  tot_len = ph->tot_len;
#endif /* MIB2_STATS */

  ret = pppol2tp_xmit(l2tp, ph);
  if (ret != ERR_OK) {
    LINK_STATS_INC(link.err);
    MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
    return ret;
  }

  MIB2_STATS_NETIF_ADD(ppp->netif, ifoutoctets, (u16_t)tot_len);
  MIB2_STATS_NETIF_INC(ppp->netif, ifoutucastpkts);
  LINK_STATS_INC(link.xmit);
  return ERR_OK;
}

/* Called by PPP core */
static err_t pppol2tp_netif_output(ppp_pcb *ppp, void *ctx, struct pbuf *p, u_short protocol) {
  pppol2tp_pcb *l2tp = (pppol2tp_pcb *)ctx;
  struct pbuf *pb;
  u8_t *pl;
  err_t err;
#if MIB2_STATS
  u16_t tot_len;
#else /* MIB2_STATS */
  LWIP_UNUSED_ARG(ppp);
#endif /* MIB2_STATS */

  /* @todo: try to use pbuf_header() here! */
  pb = pbuf_alloc(PBUF_TRANSPORT, PPPOL2TP_OUTPUT_DATA_HEADER_LEN + sizeof(protocol), PBUF_RAM);
  if(!pb) {
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.proterr);
    MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
    return ERR_MEM;
  }

  pbuf_remove_header(pb, PPPOL2TP_OUTPUT_DATA_HEADER_LEN);

  pl = (u8_t*)pb->payload;
  PUTSHORT(protocol, pl);

  pbuf_chain(pb, p);
#if MIB2_STATS
  tot_len = pb->tot_len;
#endif /* MIB2_STATS */

  if( (err = pppol2tp_xmit(l2tp, pb)) != ERR_OK) {
    LINK_STATS_INC(link.err);
    MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
    return err;
  }

  MIB2_STATS_NETIF_ADD(ppp->netif, ifoutoctets, tot_len);
  MIB2_STATS_NETIF_INC(ppp->netif, ifoutucastpkts);
  LINK_STATS_INC(link.xmit);
  return ERR_OK;
}

/* Destroy a L2TP control block */
static err_t pppol2tp_destroy(ppp_pcb *ppp, void *ctx) {
  pppol2tp_pcb *l2tp = (pppol2tp_pcb *)ctx;
  LWIP_UNUSED_ARG(ppp);

  sys_untimeout(pppol2tp_timeout, l2tp);
  udp_remove(l2tp->udp);
  LWIP_MEMPOOL_FREE(PPPOL2TP_PCB, l2tp);
  return ERR_OK;
}

/* Be a LAC, connect to a LNS. */
static void pppol2tp_connect(ppp_pcb *ppp, void *ctx) {
  err_t err;
  pppol2tp_pcb *l2tp = (pppol2tp_pcb *)ctx;
  lcp_options *lcp_wo;
  lcp_options *lcp_ao;
#if PPP_IPV4_SUPPORT && VJ_SUPPORT
  ipcp_options *ipcp_wo;
  ipcp_options *ipcp_ao;
#endif /* PPP_IPV4_SUPPORT && VJ_SUPPORT */

  l2tp->tunnel_port = l2tp->remote_port;
  l2tp->our_ns = 0;
  l2tp->peer_nr = 0;
  l2tp->peer_ns = 0;
  l2tp->source_tunnel_id = 0;
  l2tp->remote_tunnel_id = 0;
  l2tp->source_session_id = 0;
  l2tp->remote_session_id = 0;
  /* l2tp->*_retried are cleared when used */

  lcp_wo = &ppp->lcp_wantoptions;
  lcp_wo->mru = PPPOL2TP_DEFMRU;
  lcp_wo->neg_asyncmap = 0;
  lcp_wo->neg_pcompression = 0;
  lcp_wo->neg_accompression = 0;
  lcp_wo->passive = 0;
  lcp_wo->silent = 0;

  lcp_ao = &ppp->lcp_allowoptions;
  lcp_ao->mru = PPPOL2TP_DEFMRU;
  lcp_ao->neg_asyncmap = 0;
  lcp_ao->neg_pcompression = 0;
  lcp_ao->neg_accompression = 0;

#if PPP_IPV4_SUPPORT && VJ_SUPPORT
  ipcp_wo = &ppp->ipcp_wantoptions;
  ipcp_wo->neg_vj = 0;
  ipcp_wo->old_vj = 0;

  ipcp_ao = &ppp->ipcp_allowoptions;
  ipcp_ao->neg_vj = 0;
  ipcp_ao->old_vj = 0;
#endif /* PPP_IPV4_SUPPORT && VJ_SUPPORT */

  /* Listen to a random source port, we need to do that instead of using udp_connect()
   * because the L2TP LNS might answer with its own random source port (!= 1701)
   */
#if LWIP_IPV6
  if (IP_IS_V6_VAL(l2tp->udp->local_ip)) {
    udp_bind(l2tp->udp, IP6_ADDR_ANY, 0);
  } else
#endif /* LWIP_IPV6 */
  udp_bind(l2tp->udp, IP_ADDR_ANY, 0);

#if PPPOL2TP_AUTH_SUPPORT
  /* Generate random vector */
  if (l2tp->secret != NULL) {
    magic_random_bytes(l2tp->secret_rv, sizeof(l2tp->secret_rv));
  }
#endif /* PPPOL2TP_AUTH_SUPPORT */

  do {
    l2tp->remote_tunnel_id = magic();
  } while(l2tp->remote_tunnel_id == 0);
  /* save state, in case we fail to send SCCRQ */
  l2tp->sccrq_retried = 0;
  l2tp->phase = PPPOL2TP_STATE_SCCRQ_SENT;
  if ((err = pppol2tp_send_sccrq(l2tp)) != 0) {
    PPPDEBUG(LOG_DEBUG, ("pppol2tp: failed to send SCCRQ, error=%d\n", err));
  }
  sys_timeout(PPPOL2TP_CONTROL_TIMEOUT, pppol2tp_timeout, l2tp);
}

/* Disconnect */
static void pppol2tp_disconnect(ppp_pcb *ppp, void *ctx) {
  pppol2tp_pcb *l2tp = (pppol2tp_pcb *)ctx;

  l2tp->our_ns++;
  pppol2tp_send_stopccn(l2tp, l2tp->our_ns);

  /* stop any timer, disconnect can be called while initiating is in progress */
  sys_untimeout(pppol2tp_timeout, l2tp);
  l2tp->phase = PPPOL2TP_STATE_INITIAL;
  ppp_link_end(ppp); /* notify upper layers */
}

/* UDP Callback for incoming IPv4 L2TP frames */
static void pppol2tp_input(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
  pppol2tp_pcb *l2tp = (pppol2tp_pcb*)arg;
  u16_t hflags, hlen, len=0, tunnel_id=0, session_id=0, ns=0, nr=0, offset=0;
  u8_t *inp;
  LWIP_UNUSED_ARG(pcb);

  /* we can't unbound a UDP pcb, thus we can still receive UDP frames after the link is closed */
  if (l2tp->phase < PPPOL2TP_STATE_SCCRQ_SENT) {
    goto free_and_return;
  }

  if (!ip_addr_cmp(&l2tp->remote_ip, addr)) {
    goto free_and_return;
  }

  /* discard packet if port mismatch, but only if we received a SCCRP */
  if (l2tp->phase > PPPOL2TP_STATE_SCCRQ_SENT && l2tp->tunnel_port != port) {
    goto free_and_return;
  }

  /* printf("-----------\nL2TP INPUT, %d\n", p->len); */

  /* L2TP header */
  if (p->len < sizeof(hflags) + sizeof(tunnel_id) + sizeof(session_id) ) {
    goto packet_too_short;
  }

  inp = (u8_t*)p->payload;
  GETSHORT(hflags, inp);

  if (hflags & PPPOL2TP_HEADERFLAG_CONTROL) {
    /* check mandatory flags for a control packet */
    if ( (hflags & PPPOL2TP_HEADERFLAG_CONTROL_MANDATORY) != PPPOL2TP_HEADERFLAG_CONTROL_MANDATORY ) {
      PPPDEBUG(LOG_DEBUG, ("pppol2tp: mandatory header flags for control packet not set\n"));
      goto free_and_return;
    }
    /* check forbidden flags for a control packet */
    if (hflags & PPPOL2TP_HEADERFLAG_CONTROL_FORBIDDEN) {
      PPPDEBUG(LOG_DEBUG, ("pppol2tp: forbidden header flags for control packet found\n"));
      goto free_and_return;
    }
  } else {
    /* check mandatory flags for a data packet */
    if ( (hflags & PPPOL2TP_HEADERFLAG_DATA_MANDATORY) != PPPOL2TP_HEADERFLAG_DATA_MANDATORY) {
      PPPDEBUG(LOG_DEBUG, ("pppol2tp: mandatory header flags for data packet not set\n"));
      goto free_and_return;
    }
  }

  /* Expected header size  */
  hlen = sizeof(hflags) + sizeof(tunnel_id) + sizeof(session_id);
  if (hflags & PPPOL2TP_HEADERFLAG_LENGTH) {
    hlen += sizeof(len);
  }
  if (hflags & PPPOL2TP_HEADERFLAG_SEQUENCE) {
    hlen += sizeof(ns) + sizeof(nr);
  }
  if (hflags & PPPOL2TP_HEADERFLAG_OFFSET) {
    hlen += sizeof(offset);
  }
  if (p->len < hlen) {
    goto packet_too_short;
  }

  if (hflags & PPPOL2TP_HEADERFLAG_LENGTH) {
    GETSHORT(len, inp);
    if (p->len < len || len < hlen) {
      goto packet_too_short;
    }
  }
  GETSHORT(tunnel_id, inp);
  GETSHORT(session_id, inp);
  if (hflags & PPPOL2TP_HEADERFLAG_SEQUENCE) {
    GETSHORT(ns, inp);
    GETSHORT(nr, inp);
  }
  if (hflags & PPPOL2TP_HEADERFLAG_OFFSET) {
    GETSHORT(offset, inp)
    if (offset > 4096) { /* don't be fooled with large offset which might overflow hlen */
      PPPDEBUG(LOG_DEBUG, ("pppol2tp: strange packet received, offset=%d\n", offset));
      goto free_and_return;
    }
    hlen += offset;
    if (p->len < hlen) {
      goto packet_too_short;
    }
    INCPTR(offset, inp);
  }

  /* printf("HLEN = %d\n", hlen); */

  /* skip L2TP header */
  if (pbuf_remove_header(p, hlen) != 0) {
    goto free_and_return;
  }

  /* printf("LEN=%d, TUNNEL_ID=%d, SESSION_ID=%d, NS=%d, NR=%d, OFFSET=%d\n", len, tunnel_id, session_id, ns, nr, offset); */
  PPPDEBUG(LOG_DEBUG, ("pppol2tp: input packet, len=%"U16_F", tunnel=%"U16_F", session=%"U16_F", ns=%"U16_F", nr=%"U16_F"\n",
    len, tunnel_id, session_id, ns, nr));

  /* Control packet */
  if (hflags & PPPOL2TP_HEADERFLAG_CONTROL) {
    pppol2tp_dispatch_control_packet(l2tp, port, p, ns, nr);
    goto free_and_return;
  }

  /* Data packet */
  if(l2tp->phase != PPPOL2TP_STATE_DATA) {
    goto free_and_return;
  }
  if(tunnel_id != l2tp->remote_tunnel_id) {
     PPPDEBUG(LOG_DEBUG, ("pppol2tp: tunnel ID mismatch, assigned=%d, received=%d\n", l2tp->remote_tunnel_id, tunnel_id));
     goto free_and_return;
  }
  if(session_id != l2tp->remote_session_id) {
     PPPDEBUG(LOG_DEBUG, ("pppol2tp: session ID mismatch, assigned=%d, received=%d\n", l2tp->remote_session_id, session_id));
     goto free_and_return;
  }
  /*
   * skip address & flags if necessary
   *
   * RFC 2661 does not specify whether the PPP frame in the L2TP payload should
   * have a HDLC header or not. We handle both cases for compatibility.
   */
  if (p->len >= 2) {
    GETSHORT(hflags, inp);
    if (hflags == 0xff03) {
      pbuf_remove_header(p, 2);
    }
  }
  /* Dispatch the packet thereby consuming it. */
  ppp_input(l2tp->ppp, p);
  return;

packet_too_short:
  PPPDEBUG(LOG_DEBUG, ("pppol2tp: packet too short: %d\n", p->len));
free_and_return:
  pbuf_free(p);
}

/* L2TP Control packet entry point */
static void pppol2tp_dispatch_control_packet(pppol2tp_pcb *l2tp, u16_t port, struct pbuf *p, u16_t ns, u16_t nr) {
  u8_t *inp;
  u16_t avplen, avpflags, vendorid, attributetype, messagetype=0;
  err_t err;
#if PPPOL2TP_AUTH_SUPPORT
  lwip_md5_context md5_ctx;
  u8_t md5_hash[16];
  u8_t challenge_id = 0;
#endif /* PPPOL2TP_AUTH_SUPPORT */

  l2tp->peer_nr = nr;
  l2tp->peer_ns = ns;
  /* printf("L2TP CTRL INPUT, ns=%d, nr=%d, len=%d\n", ns, nr, p->len); */

  /* Handle the special case of the ICCN acknowledge */
  if (l2tp->phase == PPPOL2TP_STATE_ICCN_SENT && l2tp->peer_nr > l2tp->our_ns) {
    l2tp->phase = PPPOL2TP_STATE_DATA;
  }

  /* ZLB packets */
  if (p->tot_len == 0) {
    return;
  }

  p = pbuf_coalesce(p, PBUF_RAW);
  inp = (u8_t*)p->payload;
  /* Decode AVPs */
  while (p->len > 0) {
    if (p->len < sizeof(avpflags) + sizeof(vendorid) + sizeof(attributetype) ) {
      goto packet_too_short;
    }
    GETSHORT(avpflags, inp);
    avplen = avpflags & PPPOL2TP_AVPHEADERFLAG_LENGTHMASK;
    /* printf("AVPLEN = %d\n", avplen); */
    if (p->len < avplen || avplen < sizeof(avpflags) + sizeof(vendorid) + sizeof(attributetype)) {
      goto packet_too_short;
    }
    GETSHORT(vendorid, inp);
    GETSHORT(attributetype, inp);
    avplen -= sizeof(avpflags) + sizeof(vendorid) + sizeof(attributetype);

    /* Message type must be the first AVP */
    if (messagetype == 0) {
      if (attributetype != 0 || vendorid != 0 || avplen != sizeof(messagetype) ) {
        PPPDEBUG(LOG_DEBUG, ("pppol2tp: message type must be the first AVP\n"));
        return;
      }
      GETSHORT(messagetype, inp);
      /* printf("Message type = %d\n", messagetype); */
      switch(messagetype) {
        /* Start Control Connection Reply */
        case PPPOL2TP_MESSAGETYPE_SCCRP:
          /* Only accept SCCRP packet if we sent a SCCRQ */
          if (l2tp->phase != PPPOL2TP_STATE_SCCRQ_SENT) {
            goto send_zlb;
          }
          break;
        /* Incoming Call Reply */
        case PPPOL2TP_MESSAGETYPE_ICRP:
          /* Only accept ICRP packet if we sent a IRCQ */
          if (l2tp->phase != PPPOL2TP_STATE_ICRQ_SENT) {
            goto send_zlb;
          }
          break;
        /* Stop Control Connection Notification */
        case PPPOL2TP_MESSAGETYPE_STOPCCN:
          pppol2tp_send_zlb(l2tp, l2tp->our_ns); /* Ack the StopCCN before we switch to down state */
          if (l2tp->phase < PPPOL2TP_STATE_DATA) {
            pppol2tp_abort_connect(l2tp);
          } else if (l2tp->phase == PPPOL2TP_STATE_DATA) {
            /* Don't disconnect here, we let the LCP Echo/Reply find the fact
             * that PPP session is down. Asking the PPP stack to end the session
             * require strict checking about the PPP phase to prevent endless
             * disconnection loops.
             */
          }
          return;
        default:
          break;
      }
      goto nextavp;
    }

    /* Skip proprietary L2TP extensions */
    if (vendorid != 0) {
      goto skipavp;
    }

    switch (messagetype) {
      /* Start Control Connection Reply */
      case PPPOL2TP_MESSAGETYPE_SCCRP:
       switch (attributetype) {
          case PPPOL2TP_AVPTYPE_TUNNELID:
            if (avplen != sizeof(l2tp->source_tunnel_id) ) {
               PPPDEBUG(LOG_DEBUG, ("pppol2tp: AVP Assign tunnel ID length check failed\n"));
               return;
            }
            GETSHORT(l2tp->source_tunnel_id, inp);
            PPPDEBUG(LOG_DEBUG, ("pppol2tp: Assigned tunnel ID %"U16_F"\n", l2tp->source_tunnel_id));
            goto nextavp;
#if PPPOL2TP_AUTH_SUPPORT
          case PPPOL2TP_AVPTYPE_CHALLENGE:
            if (avplen == 0) {
               PPPDEBUG(LOG_DEBUG, ("pppol2tp: Challenge length check failed\n"));
               return;
            }
            if (l2tp->secret == NULL) {
              PPPDEBUG(LOG_DEBUG, ("pppol2tp: Received challenge from peer and no secret key available\n"));
              pppol2tp_abort_connect(l2tp);
              return;
            }
            /* Generate hash of ID, secret, challenge */
            lwip_md5_init(&md5_ctx);
            lwip_md5_starts(&md5_ctx);
            challenge_id = PPPOL2TP_MESSAGETYPE_SCCCN;
            lwip_md5_update(&md5_ctx, &challenge_id, 1);
            lwip_md5_update(&md5_ctx, l2tp->secret, l2tp->secret_len);
            lwip_md5_update(&md5_ctx, inp, avplen);
            lwip_md5_finish(&md5_ctx, l2tp->challenge_hash);
            lwip_md5_free(&md5_ctx);
            l2tp->send_challenge = 1;
            goto skipavp;
          case PPPOL2TP_AVPTYPE_CHALLENGERESPONSE:
            if (avplen != PPPOL2TP_AVPTYPE_CHALLENGERESPONSE_SIZE) {
               PPPDEBUG(LOG_DEBUG, ("pppol2tp: AVP Challenge Response length check failed\n"));
               return;
            }
            /* Generate hash of ID, secret, challenge */
            lwip_md5_init(&md5_ctx);
            lwip_md5_starts(&md5_ctx);
            challenge_id = PPPOL2TP_MESSAGETYPE_SCCRP;
            lwip_md5_update(&md5_ctx, &challenge_id, 1);
            lwip_md5_update(&md5_ctx, l2tp->secret, l2tp->secret_len);
            lwip_md5_update(&md5_ctx, l2tp->secret_rv, sizeof(l2tp->secret_rv));
            lwip_md5_finish(&md5_ctx, md5_hash);
            lwip_md5_free(&md5_ctx);
            if ( memcmp(inp, md5_hash, sizeof(md5_hash)) ) {
              PPPDEBUG(LOG_DEBUG, ("pppol2tp: Received challenge response from peer and secret key do not match\n"));
              pppol2tp_abort_connect(l2tp);
              return;
            }
            goto skipavp;
#endif /* PPPOL2TP_AUTH_SUPPORT */
          default:
            break;
        }
        break;
      /* Incoming Call Reply */
      case PPPOL2TP_MESSAGETYPE_ICRP:
        switch (attributetype) {
         case PPPOL2TP_AVPTYPE_SESSIONID:
            if (avplen != sizeof(l2tp->source_session_id) ) {
               PPPDEBUG(LOG_DEBUG, ("pppol2tp: AVP Assign session ID length check failed\n"));
               return;
            }
            GETSHORT(l2tp->source_session_id, inp);
            PPPDEBUG(LOG_DEBUG, ("pppol2tp: Assigned session ID %"U16_F"\n", l2tp->source_session_id));
            goto nextavp;
          default:
            break;
        }
        break;
      default:
        break;
    }

skipavp:
    INCPTR(avplen, inp);
nextavp:
    /* printf("AVP Found, vendor=%d, attribute=%d, len=%d\n", vendorid, attributetype, avplen); */
    /* next AVP */
    if (pbuf_remove_header(p, avplen + sizeof(avpflags) + sizeof(vendorid) + sizeof(attributetype)) != 0) {
      return;
    }
  }

  switch(messagetype) {
    /* Start Control Connection Reply */
    case PPPOL2TP_MESSAGETYPE_SCCRP:
      do {
        l2tp->remote_session_id = magic();
      } while(l2tp->remote_session_id == 0);
      l2tp->tunnel_port = port; /* LNS server might have chosen its own local port */
      l2tp->icrq_retried = 0;
      l2tp->phase = PPPOL2TP_STATE_ICRQ_SENT;
      l2tp->our_ns++;
      if ((err = pppol2tp_send_scccn(l2tp, l2tp->our_ns)) != 0) {
        PPPDEBUG(LOG_DEBUG, ("pppol2tp: failed to send SCCCN, error=%d\n", err));
        LWIP_UNUSED_ARG(err); /* if PPPDEBUG is disabled */
      }
      l2tp->our_ns++;
      if ((err = pppol2tp_send_icrq(l2tp, l2tp->our_ns)) != 0) {
        PPPDEBUG(LOG_DEBUG, ("pppol2tp: failed to send ICRQ, error=%d\n", err));
        LWIP_UNUSED_ARG(err); /* if PPPDEBUG is disabled */
      }
      sys_untimeout(pppol2tp_timeout, l2tp);
      sys_timeout(PPPOL2TP_CONTROL_TIMEOUT, pppol2tp_timeout, l2tp);
      break;
    /* Incoming Call Reply */
    case PPPOL2TP_MESSAGETYPE_ICRP:
      l2tp->iccn_retried = 0;
      l2tp->phase = PPPOL2TP_STATE_ICCN_SENT;
      l2tp->our_ns++;
      ppp_start(l2tp->ppp); /* notify upper layers */
      if ((err = pppol2tp_send_iccn(l2tp, l2tp->our_ns)) != 0) {
        PPPDEBUG(LOG_DEBUG, ("pppol2tp: failed to send ICCN, error=%d\n", err));
        LWIP_UNUSED_ARG(err); /* if PPPDEBUG is disabled */
      }
      sys_untimeout(pppol2tp_timeout, l2tp);
      sys_timeout(PPPOL2TP_CONTROL_TIMEOUT, pppol2tp_timeout, l2tp);
      break;
    /* Unhandled packet, send ZLB ACK */
    default:
      goto send_zlb;
  }
  return;

send_zlb:
  pppol2tp_send_zlb(l2tp, l2tp->our_ns);
  return;
packet_too_short:
  PPPDEBUG(LOG_DEBUG, ("pppol2tp: packet too short: %d\n", p->len));
}

/* L2TP Timeout handler */
static void pppol2tp_timeout(void *arg) {
  pppol2tp_pcb *l2tp = (pppol2tp_pcb*)arg;
  err_t err;
  u32_t retry_wait;

  PPPDEBUG(LOG_DEBUG, ("pppol2tp: timeout\n"));

  switch (l2tp->phase) {
    case PPPOL2TP_STATE_SCCRQ_SENT:
      /* backoff wait */
      if (l2tp->sccrq_retried < 0xff) {
        l2tp->sccrq_retried++;
      }
      if (!l2tp->ppp->settings.persist && l2tp->sccrq_retried >= PPPOL2TP_MAXSCCRQ) {
        pppol2tp_abort_connect(l2tp);
        return;
      }
      retry_wait = LWIP_MIN(PPPOL2TP_CONTROL_TIMEOUT * l2tp->sccrq_retried, PPPOL2TP_SLOW_RETRY);
      PPPDEBUG(LOG_DEBUG, ("pppol2tp: sccrq_retried=%d\n", l2tp->sccrq_retried));
      if ((err = pppol2tp_send_sccrq(l2tp)) != 0) {
        l2tp->sccrq_retried--;
        PPPDEBUG(LOG_DEBUG, ("pppol2tp: failed to send SCCRQ, error=%d\n", err));
        LWIP_UNUSED_ARG(err); /* if PPPDEBUG is disabled */
      }
      sys_timeout(retry_wait, pppol2tp_timeout, l2tp);
      break;

    case PPPOL2TP_STATE_ICRQ_SENT:
      l2tp->icrq_retried++;
      if (l2tp->icrq_retried >= PPPOL2TP_MAXICRQ) {
        pppol2tp_abort_connect(l2tp);
        return;
      }
      PPPDEBUG(LOG_DEBUG, ("pppol2tp: icrq_retried=%d\n", l2tp->icrq_retried));
      if (l2tp->peer_nr <= l2tp->our_ns -1) { /* the SCCCN was not acknowledged */
        if ((err = pppol2tp_send_scccn(l2tp, l2tp->our_ns -1)) != 0) {
          l2tp->icrq_retried--;
          PPPDEBUG(LOG_DEBUG, ("pppol2tp: failed to send SCCCN, error=%d\n", err));
          LWIP_UNUSED_ARG(err); /* if PPPDEBUG is disabled */
          sys_timeout(PPPOL2TP_CONTROL_TIMEOUT, pppol2tp_timeout, l2tp);
          break;
        }
      }
      if ((err = pppol2tp_send_icrq(l2tp, l2tp->our_ns)) != 0) {
        l2tp->icrq_retried--;
        PPPDEBUG(LOG_DEBUG, ("pppol2tp: failed to send ICRQ, error=%d\n", err));
        LWIP_UNUSED_ARG(err); /* if PPPDEBUG is disabled */
      }
      sys_timeout(PPPOL2TP_CONTROL_TIMEOUT, pppol2tp_timeout, l2tp);
      break;

    case PPPOL2TP_STATE_ICCN_SENT:
      l2tp->iccn_retried++;
      if (l2tp->iccn_retried >= PPPOL2TP_MAXICCN) {
        pppol2tp_abort_connect(l2tp);
        return;
      }
      PPPDEBUG(LOG_DEBUG, ("pppol2tp: iccn_retried=%d\n", l2tp->iccn_retried));
      if ((err = pppol2tp_send_iccn(l2tp, l2tp->our_ns)) != 0) {
        l2tp->iccn_retried--;
        PPPDEBUG(LOG_DEBUG, ("pppol2tp: failed to send ICCN, error=%d\n", err));
        LWIP_UNUSED_ARG(err); /* if PPPDEBUG is disabled */
      }
      sys_timeout(PPPOL2TP_CONTROL_TIMEOUT, pppol2tp_timeout, l2tp);
      break;

    default:
      return;  /* all done, work in peace */
  }
}

/* Connection attempt aborted */
static void pppol2tp_abort_connect(pppol2tp_pcb *l2tp) {
  PPPDEBUG(LOG_DEBUG, ("pppol2tp: could not establish connection\n"));
  l2tp->phase = PPPOL2TP_STATE_INITIAL;
  ppp_link_failed(l2tp->ppp); /* notify upper layers */
}

/* Initiate a new tunnel */
static err_t pppol2tp_send_sccrq(pppol2tp_pcb *l2tp) {
  struct pbuf *pb;
  u8_t *p;
  u16_t len;

  /* calculate UDP packet length */
  len = 12 +8 +8 +10 +10 +6+sizeof(PPPOL2TP_HOSTNAME)-1 +6+sizeof(PPPOL2TP_VENDORNAME)-1 +8 +8;
#if PPPOL2TP_AUTH_SUPPORT
  if (l2tp->secret != NULL) {
    len += 6 + sizeof(l2tp->secret_rv);
  }
#endif /* PPPOL2TP_AUTH_SUPPORT */

  /* allocate a buffer */
  pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (pb == NULL) {
    return ERR_MEM;
  }
  LWIP_ASSERT("pb->tot_len == pb->len", pb->tot_len == pb->len);

  p = (u8_t*)pb->payload;
  /* fill in pkt */
  /* L2TP control header */
  PUTSHORT(PPPOL2TP_HEADERFLAG_CONTROL_MANDATORY, p);
  PUTSHORT(len, p); /* Length */
  PUTSHORT(0, p); /* Tunnel Id */
  PUTSHORT(0, p); /* Session Id */
  PUTSHORT(0, p); /* NS Sequence number - to peer */
  PUTSHORT(0, p); /* NR Sequence number - expected for peer */

  /* AVP - Message type */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_MESSAGE, p); /* Attribute type: Message Type */
  PUTSHORT(PPPOL2TP_MESSAGETYPE_SCCRQ, p); /* Attribute value: Message type: SCCRQ */

  /* AVP - L2TP Version */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_VERSION, p); /* Attribute type: Version */
  PUTSHORT(PPPOL2TP_VERSION, p); /* Attribute value: L2TP Version */

  /* AVP - Framing capabilities */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 10, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_FRAMINGCAPABILITIES, p); /* Attribute type: Framing capabilities */
  PUTLONG(PPPOL2TP_FRAMINGCAPABILITIES, p); /* Attribute value: Framing capabilities */

  /* AVP - Bearer capabilities */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 10, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_BEARERCAPABILITIES, p); /* Attribute type: Bearer capabilities */
  PUTLONG(PPPOL2TP_BEARERCAPABILITIES, p); /* Attribute value: Bearer capabilities */

  /* AVP - Host name */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 6+sizeof(PPPOL2TP_HOSTNAME)-1, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_HOSTNAME, p); /* Attribute type: Hostname */
  MEMCPY(p, PPPOL2TP_HOSTNAME, sizeof(PPPOL2TP_HOSTNAME)-1); /* Attribute value: Hostname */
  INCPTR(sizeof(PPPOL2TP_HOSTNAME)-1, p);

  /* AVP - Vendor name */
  PUTSHORT(6+sizeof(PPPOL2TP_VENDORNAME)-1, p); /* len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_VENDORNAME, p); /* Attribute type: Vendor name */
  MEMCPY(p, PPPOL2TP_VENDORNAME, sizeof(PPPOL2TP_VENDORNAME)-1); /* Attribute value: Vendor name */
  INCPTR(sizeof(PPPOL2TP_VENDORNAME)-1, p);

  /* AVP - Assign tunnel ID */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_TUNNELID, p); /* Attribute type: Tunnel ID */
  PUTSHORT(l2tp->remote_tunnel_id, p); /* Attribute value: Tunnel ID */

  /* AVP - Receive window size */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_RECEIVEWINDOWSIZE, p); /* Attribute type: Receive window size */
  PUTSHORT(PPPOL2TP_RECEIVEWINDOWSIZE, p); /* Attribute value: Receive window size */

#if PPPOL2TP_AUTH_SUPPORT
  /* AVP - Challenge */
  if (l2tp->secret != NULL) {
    PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 6 + sizeof(l2tp->secret_rv), p); /* Mandatory flag + len field */
    PUTSHORT(0, p); /* Vendor ID */
    PUTSHORT(PPPOL2TP_AVPTYPE_CHALLENGE, p); /* Attribute type: Challenge */
    MEMCPY(p, l2tp->secret_rv, sizeof(l2tp->secret_rv)); /* Attribute value: Random vector */
    INCPTR(sizeof(l2tp->secret_rv), p);
  }
#endif /* PPPOL2TP_AUTH_SUPPORT */

  return pppol2tp_udp_send(l2tp, pb);
}

/* Complete tunnel establishment */
static err_t pppol2tp_send_scccn(pppol2tp_pcb *l2tp, u16_t ns) {
  struct pbuf *pb;
  u8_t *p;
  u16_t len;

  /* calculate UDP packet length */
  len = 12 +8;
#if PPPOL2TP_AUTH_SUPPORT
  if (l2tp->send_challenge) {
    len += 6 + sizeof(l2tp->challenge_hash);
  }
#endif /* PPPOL2TP_AUTH_SUPPORT */

  /* allocate a buffer */
  pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (pb == NULL) {
    return ERR_MEM;
  }
  LWIP_ASSERT("pb->tot_len == pb->len", pb->tot_len == pb->len);

  p = (u8_t*)pb->payload;
  /* fill in pkt */
  /* L2TP control header */
  PUTSHORT(PPPOL2TP_HEADERFLAG_CONTROL_MANDATORY, p);
  PUTSHORT(len, p); /* Length */
  PUTSHORT(l2tp->source_tunnel_id, p); /* Tunnel Id */
  PUTSHORT(0, p); /* Session Id */
  PUTSHORT(ns, p); /* NS Sequence number - to peer */
  PUTSHORT(l2tp->peer_ns+1, p); /* NR Sequence number - expected for peer */

  /* AVP - Message type */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_MESSAGE, p); /* Attribute type: Message Type */
  PUTSHORT(PPPOL2TP_MESSAGETYPE_SCCCN, p); /* Attribute value: Message type: SCCCN */

#if PPPOL2TP_AUTH_SUPPORT
  /* AVP - Challenge response */
  if (l2tp->send_challenge) {
    PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 6 + sizeof(l2tp->challenge_hash), p); /* Mandatory flag + len field */
    PUTSHORT(0, p); /* Vendor ID */
    PUTSHORT(PPPOL2TP_AVPTYPE_CHALLENGERESPONSE, p); /* Attribute type: Challenge response */
    MEMCPY(p, l2tp->challenge_hash, sizeof(l2tp->challenge_hash)); /* Attribute value: Computed challenge */
    INCPTR(sizeof(l2tp->challenge_hash), p);
  }
#endif /* PPPOL2TP_AUTH_SUPPORT */

  return pppol2tp_udp_send(l2tp, pb);
}

/* Initiate a new session */
static err_t pppol2tp_send_icrq(pppol2tp_pcb *l2tp, u16_t ns) {
  struct pbuf *pb;
  u8_t *p;
  u16_t len;
  u32_t serialnumber;

  /* calculate UDP packet length */
  len = 12 +8 +8 +10;

  /* allocate a buffer */
  pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (pb == NULL) {
    return ERR_MEM;
  }
  LWIP_ASSERT("pb->tot_len == pb->len", pb->tot_len == pb->len);

  p = (u8_t*)pb->payload;
  /* fill in pkt */
  /* L2TP control header */
  PUTSHORT(PPPOL2TP_HEADERFLAG_CONTROL_MANDATORY, p);
  PUTSHORT(len, p); /* Length */
  PUTSHORT(l2tp->source_tunnel_id, p); /* Tunnel Id */
  PUTSHORT(0, p); /* Session Id */
  PUTSHORT(ns, p); /* NS Sequence number - to peer */
  PUTSHORT(l2tp->peer_ns+1, p); /* NR Sequence number - expected for peer */

  /* AVP - Message type */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_MESSAGE, p); /* Attribute type: Message Type */
  PUTSHORT(PPPOL2TP_MESSAGETYPE_ICRQ, p); /* Attribute value: Message type: ICRQ */

  /* AVP - Assign session ID */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_SESSIONID, p); /* Attribute type: Session ID */
  PUTSHORT(l2tp->remote_session_id, p); /* Attribute value: Session ID */

  /* AVP - Call Serial Number */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 10, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_CALLSERIALNUMBER, p); /* Attribute type: Serial number */
  serialnumber = magic();
  PUTLONG(serialnumber, p); /* Attribute value: Serial number */

  return pppol2tp_udp_send(l2tp, pb);
}

/* Complete tunnel establishment */
static err_t pppol2tp_send_iccn(pppol2tp_pcb *l2tp, u16_t ns) {
  struct pbuf *pb;
  u8_t *p;
  u16_t len;

  /* calculate UDP packet length */
  len = 12 +8 +10 +10;

  /* allocate a buffer */
  pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (pb == NULL) {
    return ERR_MEM;
  }
  LWIP_ASSERT("pb->tot_len == pb->len", pb->tot_len == pb->len);

  p = (u8_t*)pb->payload;
  /* fill in pkt */
  /* L2TP control header */
  PUTSHORT(PPPOL2TP_HEADERFLAG_CONTROL_MANDATORY, p);
  PUTSHORT(len, p); /* Length */
  PUTSHORT(l2tp->source_tunnel_id, p); /* Tunnel Id */
  PUTSHORT(l2tp->source_session_id, p); /* Session Id */
  PUTSHORT(ns, p); /* NS Sequence number - to peer */
  PUTSHORT(l2tp->peer_ns+1, p); /* NR Sequence number - expected for peer */

  /* AVP - Message type */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_MESSAGE, p); /* Attribute type: Message Type */
  PUTSHORT(PPPOL2TP_MESSAGETYPE_ICCN, p); /* Attribute value: Message type: ICCN */

  /* AVP - Framing type */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 10, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_FRAMINGTYPE, p); /* Attribute type: Framing type */
  PUTLONG(PPPOL2TP_FRAMINGTYPE, p); /* Attribute value: Framing type */

  /* AVP - TX Connect speed */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 10, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_TXCONNECTSPEED, p); /* Attribute type: TX Connect speed */
  PUTLONG(PPPOL2TP_TXCONNECTSPEED, p); /* Attribute value: TX Connect speed */

  return pppol2tp_udp_send(l2tp, pb);
}

/* Send a ZLB ACK packet */
static err_t pppol2tp_send_zlb(pppol2tp_pcb *l2tp, u16_t ns) {
  struct pbuf *pb;
  u8_t *p;
  u16_t len;

  /* calculate UDP packet length */
  len = 12;

  /* allocate a buffer */
  pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (pb == NULL) {
    return ERR_MEM;
  }
  LWIP_ASSERT("pb->tot_len == pb->len", pb->tot_len == pb->len);

  p = (u8_t*)pb->payload;
  /* fill in pkt */
  /* L2TP control header */
  PUTSHORT(PPPOL2TP_HEADERFLAG_CONTROL_MANDATORY, p);
  PUTSHORT(len, p); /* Length */
  PUTSHORT(l2tp->source_tunnel_id, p); /* Tunnel Id */
  PUTSHORT(0, p); /* Session Id */
  PUTSHORT(ns, p); /* NS Sequence number - to peer */
  PUTSHORT(l2tp->peer_ns+1, p); /* NR Sequence number - expected for peer */

  return pppol2tp_udp_send(l2tp, pb);
}

/* Send a StopCCN packet */
static err_t pppol2tp_send_stopccn(pppol2tp_pcb *l2tp, u16_t ns) {
  struct pbuf *pb;
  u8_t *p;
  u16_t len;

  /* calculate UDP packet length */
  len = 12 +8 +8 +8;

  /* allocate a buffer */
  pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (pb == NULL) {
    return ERR_MEM;
  }
  LWIP_ASSERT("pb->tot_len == pb->len", pb->tot_len == pb->len);

  p = (u8_t*)pb->payload;
  /* fill in pkt */
  /* L2TP control header */
  PUTSHORT(PPPOL2TP_HEADERFLAG_CONTROL_MANDATORY, p);
  PUTSHORT(len, p); /* Length */
  PUTSHORT(l2tp->source_tunnel_id, p); /* Tunnel Id */
  PUTSHORT(0, p); /* Session Id */
  PUTSHORT(ns, p); /* NS Sequence number - to peer */
  PUTSHORT(l2tp->peer_ns+1, p); /* NR Sequence number - expected for peer */

  /* AVP - Message type */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_MESSAGE, p); /* Attribute type: Message Type */
  PUTSHORT(PPPOL2TP_MESSAGETYPE_STOPCCN, p); /* Attribute value: Message type: StopCCN */

  /* AVP - Assign tunnel ID */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_TUNNELID, p); /* Attribute type: Tunnel ID */
  PUTSHORT(l2tp->remote_tunnel_id, p); /* Attribute value: Tunnel ID */

  /* AVP - Result code */
  PUTSHORT(PPPOL2TP_AVPHEADERFLAG_MANDATORY + 8, p); /* Mandatory flag + len field */
  PUTSHORT(0, p); /* Vendor ID */
  PUTSHORT(PPPOL2TP_AVPTYPE_RESULTCODE, p); /* Attribute type: Result code */
  PUTSHORT(PPPOL2TP_RESULTCODE, p); /* Attribute value: Result code */

  return pppol2tp_udp_send(l2tp, pb);
}

static err_t pppol2tp_xmit(pppol2tp_pcb *l2tp, struct pbuf *pb) {
  u8_t *p;

  /* make room for L2TP header - should not fail */
  if (pbuf_add_header(pb, PPPOL2TP_OUTPUT_DATA_HEADER_LEN) != 0) {
    /* bail out */
    PPPDEBUG(LOG_ERR, ("pppol2tp: pppol2tp_pcb: could not allocate room for L2TP header\n"));
    LINK_STATS_INC(link.lenerr);
    pbuf_free(pb);
    return ERR_BUF;
  }

  p = (u8_t*)pb->payload;
  PUTSHORT(PPPOL2TP_HEADERFLAG_DATA_MANDATORY, p);
  PUTSHORT(l2tp->source_tunnel_id, p); /* Tunnel Id */
  PUTSHORT(l2tp->source_session_id, p); /* Session Id */

  return pppol2tp_udp_send(l2tp, pb);
}

static err_t pppol2tp_udp_send(pppol2tp_pcb *l2tp, struct pbuf *pb) {
  err_t err;
  if (l2tp->netif) {
    err = udp_sendto_if(l2tp->udp, pb, &l2tp->remote_ip, l2tp->tunnel_port, l2tp->netif);
  } else {
    err = udp_sendto(l2tp->udp, pb, &l2tp->remote_ip, l2tp->tunnel_port);
  }
  pbuf_free(pb);
  return err;
}

#endif /* PPP_SUPPORT && PPPOL2TP_SUPPORT */
