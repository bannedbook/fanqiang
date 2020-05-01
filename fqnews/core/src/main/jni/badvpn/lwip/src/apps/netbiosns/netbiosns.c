/**
 * @file
 * NetBIOS name service responder
 */

/**
 * @defgroup netbiosns NETBIOS responder
 * @ingroup apps
 *
 * This is an example implementation of a NetBIOS name server.
 * It responds to name queries for a configurable name.
 * Name resolving is not supported.
 *
 * Note that the device doesn't broadcast it's own name so can't
 * detect duplicate names!
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

#include "lwip/apps/netbiosns.h"

#if LWIP_IPV4 && LWIP_UDP  /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/udp.h"
#include "lwip/netif.h"
#include "lwip/prot/iana.h"

#include <string.h>

/** size of a NetBIOS name */
#define NETBIOS_NAME_LEN 16

/** The Time-To-Live for NetBIOS name responds (in seconds)
 * Default is 300000 seconds (3 days, 11 hours, 20 minutes) */
#define NETBIOS_NAME_TTL 300000u

/** NetBIOS header flags */
#define NETB_HFLAG_RESPONSE           0x8000U
#define NETB_HFLAG_OPCODE             0x7800U
#define NETB_HFLAG_OPCODE_NAME_QUERY  0x0000U
#define NETB_HFLAG_AUTHORATIVE        0x0400U
#define NETB_HFLAG_TRUNCATED          0x0200U
#define NETB_HFLAG_RECURS_DESIRED     0x0100U
#define NETB_HFLAG_RECURS_AVAILABLE   0x0080U
#define NETB_HFLAG_BROADCAST          0x0010U
#define NETB_HFLAG_REPLYCODE          0x0008U
#define NETB_HFLAG_REPLYCODE_NOERROR  0x0000U

/** NetBIOS name flags */
#define NETB_NFLAG_UNIQUE             0x8000U
#define NETB_NFLAG_NODETYPE           0x6000U
#define NETB_NFLAG_NODETYPE_HNODE     0x6000U
#define NETB_NFLAG_NODETYPE_MNODE     0x4000U
#define NETB_NFLAG_NODETYPE_PNODE     0x2000U
#define NETB_NFLAG_NODETYPE_BNODE     0x0000U

/** NetBIOS message header */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_hdr {
  PACK_STRUCT_FIELD(u16_t trans_id);
  PACK_STRUCT_FIELD(u16_t flags);
  PACK_STRUCT_FIELD(u16_t questions);
  PACK_STRUCT_FIELD(u16_t answerRRs);
  PACK_STRUCT_FIELD(u16_t authorityRRs);
  PACK_STRUCT_FIELD(u16_t additionalRRs);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** NetBIOS message name part */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_name_hdr {
  PACK_STRUCT_FLD_8(u8_t  nametype);
  PACK_STRUCT_FLD_8(u8_t  encname[(NETBIOS_NAME_LEN * 2) + 1]);
  PACK_STRUCT_FIELD(u16_t type);
  PACK_STRUCT_FIELD(u16_t cls);
  PACK_STRUCT_FIELD(u32_t ttl);
  PACK_STRUCT_FIELD(u16_t datalen);
  PACK_STRUCT_FIELD(u16_t flags);
  PACK_STRUCT_FLD_S(ip4_addr_p_t addr);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** NetBIOS message */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_resp {
  struct netbios_hdr      resp_hdr;
  struct netbios_name_hdr resp_name;
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#ifdef NETBIOS_LWIP_NAME
#define NETBIOS_LOCAL_NAME NETBIOS_LWIP_NAME
#else
static char netbiosns_local_name[NETBIOS_NAME_LEN];
#define NETBIOS_LOCAL_NAME netbiosns_local_name
#endif

struct udp_pcb *netbiosns_pcb;

/** Decode a NetBIOS name (from packet to string) */
static int
netbiosns_name_decode(char *name_enc, char *name_dec, int name_dec_len)
{
  char *pname;
  char  cname;
  char  cnbname;
  int   idx = 0;

  LWIP_UNUSED_ARG(name_dec_len);

  /* Start decoding netbios name. */
  pname  = name_enc;
  for (;;) {
    /* Every two characters of the first level-encoded name
     * turn into one character in the decoded name. */
    cname = *pname;
    if (cname == '\0') {
      break;  /* no more characters */
    }
    if (cname == '.') {
      break;  /* scope ID follows */
    }
    if (cname < 'A' || cname > 'Z') {
      /* Not legal. */
      return -1;
    }
    cname -= 'A';
    cnbname = cname << 4;
    pname++;

    cname = *pname;
    if (cname == '\0' || cname == '.') {
      /* No more characters in the name - but we're in
       * the middle of a pair.  Not legal. */
      return -1;
    }
    if (cname < 'A' || cname > 'Z') {
      /* Not legal. */
      return -1;
    }
    cname -= 'A';
    cnbname |= cname;
    pname++;

    /* Do we have room to store the character? */
    if (idx < NETBIOS_NAME_LEN) {
      /* Yes - store the character. */
      name_dec[idx++] = (cnbname != ' ' ? cnbname : '\0');
    }
  }

  return 0;
}

#if 0 /* function currently unused */
/** Encode a NetBIOS name (from string to packet) - currently unused because
    we don't ask for names. */
static int
netbiosns_name_encode(char *name_enc, char *name_dec, int name_dec_len)
{
  char         *pname;
  char          cname;
  unsigned char ucname;
  int           idx = 0;

  /* Start encoding netbios name. */
  pname = name_enc;

  for (;;) {
    /* Every two characters of the first level-encoded name
     * turn into one character in the decoded name. */
    cname = *pname;
    if (cname == '\0') {
      break;  /* no more characters */
    }
    if (cname == '.') {
      break;  /* scope ID follows */
    }
    if ((cname < 'A' || cname > 'Z') && (cname < '0' || cname > '9')) {
      /* Not legal. */
      return -1;
    }

    /* Do we have room to store the character? */
    if (idx >= name_dec_len) {
      return -1;
    }

    /* Yes - store the character. */
    ucname = cname;
    name_dec[idx++] = ('A' + ((ucname >> 4) & 0x0F));
    name_dec[idx++] = ('A' + ( ucname     & 0x0F));
    pname++;
  }

  /* Fill with "space" coding */
  for (; idx < name_dec_len - 1;) {
    name_dec[idx++] = 'C';
    name_dec[idx++] = 'A';
  }

  /* Terminate string */
  name_dec[idx] = '\0';

  return 0;
}
#endif /* 0 */

/** NetBIOS Name service recv callback */
static void
netbiosns_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  LWIP_UNUSED_ARG(arg);

  /* if packet is valid */
  if (p != NULL) {
    char   netbios_name[NETBIOS_NAME_LEN + 1];
    struct netbios_hdr      *netbios_hdr      = (struct netbios_hdr *)p->payload;
    struct netbios_name_hdr *netbios_name_hdr = (struct netbios_name_hdr *)(netbios_hdr + 1);

    /* we only answer if we got a default interface */
    if (netif_default != NULL) {
      /* @todo: do we need to check answerRRs/authorityRRs/additionalRRs? */
      /* if the packet is a NetBIOS name query question */
      if (((netbios_hdr->flags & PP_NTOHS(NETB_HFLAG_OPCODE)) == PP_NTOHS(NETB_HFLAG_OPCODE_NAME_QUERY)) &&
          ((netbios_hdr->flags & PP_NTOHS(NETB_HFLAG_RESPONSE)) == 0) &&
          (netbios_hdr->questions == PP_NTOHS(1))) {
        /* decode the NetBIOS name */
        netbiosns_name_decode((char *)(netbios_name_hdr->encname), netbios_name, sizeof(netbios_name));
        /* if the packet is for us */
        if (lwip_strnicmp(netbios_name, NETBIOS_LOCAL_NAME, sizeof(NETBIOS_LOCAL_NAME)) == 0) {
          struct pbuf *q;
          struct netbios_resp *resp;

          q = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct netbios_resp), PBUF_RAM);
          if (q != NULL) {
            resp = (struct netbios_resp *)q->payload;

            /* prepare NetBIOS header response */
            resp->resp_hdr.trans_id      = netbios_hdr->trans_id;
            resp->resp_hdr.flags         = PP_HTONS(NETB_HFLAG_RESPONSE |
                                                    NETB_HFLAG_OPCODE_NAME_QUERY |
                                                    NETB_HFLAG_AUTHORATIVE |
                                                    NETB_HFLAG_RECURS_DESIRED);
            resp->resp_hdr.questions     = 0;
            resp->resp_hdr.answerRRs     = PP_HTONS(1);
            resp->resp_hdr.authorityRRs  = 0;
            resp->resp_hdr.additionalRRs = 0;

            /* prepare NetBIOS header datas */
            MEMCPY( resp->resp_name.encname, netbios_name_hdr->encname, sizeof(netbios_name_hdr->encname));
            resp->resp_name.nametype     = netbios_name_hdr->nametype;
            resp->resp_name.type         = netbios_name_hdr->type;
            resp->resp_name.cls          = netbios_name_hdr->cls;
            resp->resp_name.ttl          = PP_HTONL(NETBIOS_NAME_TTL);
            resp->resp_name.datalen      = PP_HTONS(sizeof(resp->resp_name.flags) + sizeof(resp->resp_name.addr));
            resp->resp_name.flags        = PP_HTONS(NETB_NFLAG_NODETYPE_BNODE);
            ip4_addr_copy(resp->resp_name.addr, *netif_ip4_addr(netif_default));

            /* send the NetBIOS response */
            udp_sendto(upcb, q, addr, port);

            /* free the "reference" pbuf */
            pbuf_free(q);
          }
        }
      }
    }
    /* free the pbuf */
    pbuf_free(p);
  }
}

/**
 * @ingroup netbiosns
 * Init netbios responder
 */
void
netbiosns_init(void)
{
#ifdef NETBIOS_LWIP_NAME
  LWIP_ASSERT("NetBIOS name is too long!", strlen(NETBIOS_LWIP_NAME) < NETBIOS_NAME_LEN);
#endif

  netbiosns_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (netbiosns_pcb != NULL) {
    /* we have to be allowed to send broadcast packets! */
    ip_set_option(netbiosns_pcb, SOF_BROADCAST);
    udp_bind(netbiosns_pcb, IP_ANY_TYPE, LWIP_IANA_PORT_NETBIOS);
    udp_recv(netbiosns_pcb, netbiosns_recv, netbiosns_pcb);
  }
}

#ifndef NETBIOS_LWIP_NAME
/**
 * @ingroup netbiosns
 * Set netbios name. ATTENTION: the hostname must be less than 15 characters!
 */
void
netbiosns_set_name(const char *hostname)
{
  size_t copy_len = strlen(hostname);
  LWIP_ASSERT("NetBIOS name is too long!", copy_len < NETBIOS_NAME_LEN);
  if (copy_len >= NETBIOS_NAME_LEN) {
    copy_len = NETBIOS_NAME_LEN - 1;
  }
  MEMCPY(netbiosns_local_name, hostname, copy_len + 1);
}
#endif

/**
 * @ingroup netbiosns
 * Stop netbios responder
 */
void
netbiosns_stop(void)
{
  if (netbiosns_pcb != NULL) {
    udp_remove(netbiosns_pcb);
    netbiosns_pcb = NULL;
  }
}

#endif /* LWIP_IPV4 && LWIP_UDP */
