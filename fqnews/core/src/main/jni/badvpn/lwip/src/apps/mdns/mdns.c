/**
 * @file
 * MDNS responder implementation
 *
 * @defgroup mdns MDNS
 * @ingroup apps
 *
 * RFC 6762 - Multicast DNS\n
 * RFC 6763 - DNS-Based Service Discovery\n
 *
 * @verbinclude mdns.txt
 *
 * Things left to implement:
 * -------------------------
 *
 * - Probing/conflict resolution
 * - Sending goodbye messages (zero ttl) - shutdown, DHCP lease about to expire, DHCP turned off...
 * - Checking that source address of unicast requests are on the same network
 * - Limiting multicast responses to 1 per second per resource record
 * - Fragmenting replies if required
 * - Handling multi-packet known answers
 * - Individual known answer detection for all local IPv6 addresses
 * - Dynamic size of outgoing packet
 */

/*
 * Copyright (c) 2015 Verisure Innovation AB
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
 * Author: Erik Ekman <erik@kryo.se>
 *
 */

#include "lwip/apps/mdns.h"
#include "lwip/apps/mdns_priv.h"
#include "lwip/netif.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"
#include "lwip/prot/dns.h"
#include "lwip/prot/iana.h"

#include <string.h>

#if LWIP_MDNS_RESPONDER

#if (LWIP_IPV4 && !LWIP_IGMP)
#error "If you want to use MDNS with IPv4, you have to define LWIP_IGMP=1 in your lwipopts.h"
#endif
#if (LWIP_IPV6 && !LWIP_IPV6_MLD)
#error "If you want to use MDNS with IPv6, you have to define LWIP_IPV6_MLD=1 in your lwipopts.h"
#endif
#if (!LWIP_UDP)
#error "If you want to use MDNS, you have to define LWIP_UDP=1 in your lwipopts.h"
#endif

#if LWIP_IPV4
#include "lwip/igmp.h"
/* IPv4 multicast group 224.0.0.251 */
static const ip_addr_t v4group = DNS_MQUERY_IPV4_GROUP_INIT;
#endif

#if LWIP_IPV6
#include "lwip/mld6.h"
/* IPv6 multicast group FF02::FB */
static const ip_addr_t v6group = DNS_MQUERY_IPV6_GROUP_INIT;
#endif

#define MDNS_TTL  255

/* Stored offsets to beginning of domain names
 * Used for compression.
 */
#define NUM_DOMAIN_OFFSETS 10
#define DOMAIN_JUMP_SIZE 2
#define DOMAIN_JUMP 0xc000

static u8_t mdns_netif_client_id;
static struct udp_pcb *mdns_pcb;
NETIF_DECLARE_EXT_CALLBACK(netif_callback)

#define NETIF_TO_HOST(netif) (struct mdns_host*)(netif_get_client_data(netif, mdns_netif_client_id))

#define TOPDOMAIN_LOCAL "local"

#define REVERSE_PTR_TOPDOMAIN "arpa"
#define REVERSE_PTR_V4_DOMAIN "in-addr"
#define REVERSE_PTR_V6_DOMAIN "ip6"

#define SRV_PRIORITY 0
#define SRV_WEIGHT   0

/* Payload size allocated for each outgoing UDP packet */
#define OUTPACKET_SIZE 500

/* Lookup from hostname -> IPv4 */
#define REPLY_HOST_A            0x01
/* Lookup from IPv4/v6 -> hostname */
#define REPLY_HOST_PTR_V4       0x02
/* Lookup from hostname -> IPv6 */
#define REPLY_HOST_AAAA         0x04
/* Lookup from hostname -> IPv6 */
#define REPLY_HOST_PTR_V6       0x08

/* Lookup for service types */
#define REPLY_SERVICE_TYPE_PTR  0x10
/* Lookup for instances of service */
#define REPLY_SERVICE_NAME_PTR  0x20
/* Lookup for location of service instance */
#define REPLY_SERVICE_SRV       0x40
/* Lookup for text info on service instance */
#define REPLY_SERVICE_TXT       0x80

static const char *dnssd_protos[] = {
  "_udp", /* DNSSD_PROTO_UDP */
  "_tcp", /* DNSSD_PROTO_TCP */
};

/** Description of a service */
struct mdns_service {
  /** TXT record to answer with */
  struct mdns_domain txtdata;
  /** Name of service, like 'myweb' */
  char name[MDNS_LABEL_MAXLEN + 1];
  /** Type of service, like '_http' */
  char service[MDNS_LABEL_MAXLEN + 1];
  /** Callback function and userdata
   * to update txtdata buffer */
  service_get_txt_fn_t txt_fn;
  void *txt_userdata;
  /** TTL in seconds of SRV/TXT replies */
  u32_t dns_ttl;
  /** Protocol, TCP or UDP */
  u16_t proto;
  /** Port of the service */
  u16_t port;
};

/** Description of a host/netif */
struct mdns_host {
  /** Hostname */
  char name[MDNS_LABEL_MAXLEN + 1];
  /** Pointer to services */
  struct mdns_service *services[MDNS_MAX_SERVICES];
  /** TTL in seconds of A/AAAA/PTR replies */
  u32_t dns_ttl;
};

/** Information about received packet */
struct mdns_packet {
  /** Sender IP/port */
  ip_addr_t source_addr;
  u16_t source_port;
  /** If packet was received unicast */
  u16_t recv_unicast;
  /** Netif that received the packet */
  struct netif *netif;
  /** Packet data */
  struct pbuf *pbuf;
  /** Current parsing offset in packet */
  u16_t parse_offset;
  /** Identifier. Used in legacy queries */
  u16_t tx_id;
  /** Number of questions in packet,
   *  read from packet header */
  u16_t questions;
  /** Number of unparsed questions */
  u16_t questions_left;
  /** Number of answers in packet,
   *  (sum of normal, authorative and additional answers)
   *  read from packet header */
  u16_t answers;
  /** Number of unparsed answers */
  u16_t answers_left;
};

/** Information about outgoing packet */
struct mdns_outpacket {
  /** Netif to send the packet on */
  struct netif *netif;
  /** Packet data */
  struct pbuf *pbuf;
  /** Current write offset in packet */
  u16_t write_offset;
  /** Identifier. Used in legacy queries */
  u16_t tx_id;
  /** Destination IP/port if sent unicast */
  ip_addr_t dest_addr;
  u16_t dest_port;
  /** Number of questions written */
  u16_t questions;
  /** Number of normal answers written */
  u16_t answers;
  /** Number of additional answers written */
  u16_t additional;
  /** Offsets for written domain names in packet.
   *  Used for compression */
  u16_t domain_offsets[NUM_DOMAIN_OFFSETS];
  /** If all answers in packet should set cache_flush bit */
  u8_t cache_flush;
  /** If reply should be sent unicast */
  u8_t unicast_reply;
  /** If legacy query. (tx_id needed, and write
   *  question again in reply before answer) */
  u8_t legacy_query;
  /* Reply bitmask for host information */
  u8_t host_replies;
  /* Bitmask for which reverse IPv6 hosts to answer */
  u8_t host_reverse_v6_replies;
  /* Reply bitmask per service */
  u8_t serv_replies[MDNS_MAX_SERVICES];
};

/** Domain, type and class.
 *  Shared between questions and answers */
struct mdns_rr_info {
  struct mdns_domain domain;
  u16_t type;
  u16_t klass;
};

struct mdns_question {
  struct mdns_rr_info info;
  /** unicast reply requested */
  u16_t unicast;
};

struct mdns_answer {
  struct mdns_rr_info info;
  /** cache flush command bit */
  u16_t cache_flush;
  /* Validity time in seconds */
  u32_t ttl;
  /** Length of variable answer */
  u16_t rd_length;
  /** Offset of start of variable answer in packet */
  u16_t rd_offset;
};

static err_t
mdns_domain_add_label_base(struct mdns_domain *domain, u8_t len)
{
  if (len > MDNS_LABEL_MAXLEN) {
    return ERR_VAL;
  }
  if (len > 0 && (1 + len + domain->length >= MDNS_DOMAIN_MAXLEN)) {
    return ERR_VAL;
  }
  /* Allow only zero marker on last byte */
  if (len == 0 && (1 + domain->length > MDNS_DOMAIN_MAXLEN)) {
    return ERR_VAL;
  }
  domain->name[domain->length] = len;
  domain->length++;
  return ERR_OK;
}

/**
 * Add a label part to a domain
 * @param domain The domain to add a label to
 * @param label The label to add, like &lt;hostname&gt;, 'local', 'com' or ''
 * @param len The length of the label
 * @return ERR_OK on success, an err_t otherwise if label too long
 */
err_t
mdns_domain_add_label(struct mdns_domain *domain, const char *label, u8_t len)
{
  err_t err = mdns_domain_add_label_base(domain, len);
  if (err != ERR_OK) {
    return err;
  }
  if (len) {
    MEMCPY(&domain->name[domain->length], label, len);
    domain->length += len;
  }
  return ERR_OK;
}

/**
 * Add a label part to a domain (@see mdns_domain_add_label but copy directly from pbuf)
 */
static err_t
mdns_domain_add_label_pbuf(struct mdns_domain *domain, const struct pbuf *p, u16_t offset, u8_t len)
{
  err_t err = mdns_domain_add_label_base(domain, len);
  if (err != ERR_OK) {
    return err;
  }
  if (len) {
    if (pbuf_copy_partial(p, &domain->name[domain->length], len, offset) != len) {
      /* take back the ++ done before */
      domain->length--;
      return ERR_ARG;
    }
    domain->length += len;
  }
  return ERR_OK;
}

/**
 * Internal readname function with max 6 levels of recursion following jumps
 * while decompressing name
 */
static u16_t
mdns_readname_loop(struct pbuf *p, u16_t offset, struct mdns_domain *domain, unsigned depth)
{
  u8_t c;

  do {
    if (depth > 5) {
      /* Too many jumps */
      return MDNS_READNAME_ERROR;
    }

    c = pbuf_get_at(p, offset);
    offset++;

    /* is this a compressed label? */
    if ((c & 0xc0) == 0xc0) {
      u16_t jumpaddr;
      if (offset >= p->tot_len) {
        /* Make sure both jump bytes fit in the packet */
        return MDNS_READNAME_ERROR;
      }
      jumpaddr = (((c & 0x3f) << 8) | (pbuf_get_at(p, offset) & 0xff));
      offset++;
      if (jumpaddr >= SIZEOF_DNS_HDR && jumpaddr < p->tot_len) {
        u16_t res;
        /* Recursive call, maximum depth will be checked */
        res = mdns_readname_loop(p, jumpaddr, domain, depth + 1);
        /* Dont return offset since new bytes were not read (jumped to somewhere in packet) */
        if (res == MDNS_READNAME_ERROR) {
          return res;
        }
      } else {
        return MDNS_READNAME_ERROR;
      }
      break;
    }

    /* normal label */
    if (c <= MDNS_LABEL_MAXLEN) {
      err_t res;

      if (c + domain->length >= MDNS_DOMAIN_MAXLEN) {
        return MDNS_READNAME_ERROR;
      }
      res = mdns_domain_add_label_pbuf(domain, p, offset, c);
      if (res != ERR_OK) {
        return MDNS_READNAME_ERROR;
      }
      offset += c;
    } else {
      /* bad length byte */
      return MDNS_READNAME_ERROR;
    }
  } while (c != 0);

  return offset;
}

/**
 * Read possibly compressed domain name from packet buffer
 * @param p The packet
 * @param offset start position of domain name in packet
 * @param domain The domain name destination
 * @return The new offset after the domain, or MDNS_READNAME_ERROR
 *         if reading failed
 */
u16_t
mdns_readname(struct pbuf *p, u16_t offset, struct mdns_domain *domain)
{
  memset(domain, 0, sizeof(struct mdns_domain));
  return mdns_readname_loop(p, offset, domain, 0);
}

/**
 * Print domain name to debug output
 * @param domain The domain name
 */
static void
mdns_domain_debug_print(struct mdns_domain *domain)
{
  u8_t *src = domain->name;
  u8_t i;

  while (*src) {
    u8_t label_len = *src;
    src++;
    for (i = 0; i < label_len; i++) {
      LWIP_DEBUGF(MDNS_DEBUG, ("%c", src[i]));
    }
    src += label_len;
    LWIP_DEBUGF(MDNS_DEBUG, ("."));
  }
}

/**
 * Return 1 if contents of domains match (case-insensitive)
 * @param a Domain name to compare 1
 * @param b Domain name to compare 2
 * @return 1 if domains are equal ignoring case, 0 otherwise
 */
int
mdns_domain_eq(struct mdns_domain *a, struct mdns_domain *b)
{
  u8_t *ptra, *ptrb;
  u8_t len;
  int res;

  if (a->length != b->length) {
    return 0;
  }

  ptra = a->name;
  ptrb = b->name;
  while (*ptra && *ptrb && ptra < &a->name[a->length]) {
    if (*ptra != *ptrb) {
      return 0;
    }
    len = *ptra;
    ptra++;
    ptrb++;
    res = lwip_strnicmp((char *) ptra, (char *) ptrb, len);
    if (res != 0) {
      return 0;
    }
    ptra += len;
    ptrb += len;
  }
  if (*ptra != *ptrb && ptra < &a->name[a->length]) {
    return 0;
  }
  return 1;
}

/**
 * Call user supplied function to setup TXT data
 * @param service The service to build TXT record for
 */
static void
mdns_prepare_txtdata(struct mdns_service *service)
{
  memset(&service->txtdata, 0, sizeof(struct mdns_domain));
  if (service->txt_fn) {
    service->txt_fn(service, service->txt_userdata);
  }
}

#if LWIP_IPV4
/**
 * Build domain for reverse lookup of IPv4 address
 * like 12.0.168.192.in-addr.arpa. for 192.168.0.12
 * @param domain Where to write the domain name
 * @param addr Pointer to an IPv4 address to encode
 * @return ERR_OK if domain was written, an err_t otherwise
 */
static err_t
mdns_build_reverse_v4_domain(struct mdns_domain *domain, const ip4_addr_t *addr)
{
  int i;
  err_t res;
  const u8_t *ptr;
  if (!domain || !addr) {
    return ERR_ARG;
  }
  memset(domain, 0, sizeof(struct mdns_domain));
  ptr = (const u8_t *) addr;
  for (i = sizeof(ip4_addr_t) - 1; i >= 0; i--) {
    char buf[4];
    u8_t val = ptr[i];

    lwip_itoa(buf, sizeof(buf), val);
    res = mdns_domain_add_label(domain, buf, (u8_t)strlen(buf));
    LWIP_ERROR("mdns_build_reverse_v4_domain: Failed to add label", (res == ERR_OK), return res);
  }
  res = mdns_domain_add_label(domain, REVERSE_PTR_V4_DOMAIN, (u8_t)(sizeof(REVERSE_PTR_V4_DOMAIN) - 1));
  LWIP_ERROR("mdns_build_reverse_v4_domain: Failed to add label", (res == ERR_OK), return res);
  res = mdns_domain_add_label(domain, REVERSE_PTR_TOPDOMAIN, (u8_t)(sizeof(REVERSE_PTR_TOPDOMAIN) - 1));
  LWIP_ERROR("mdns_build_reverse_v4_domain: Failed to add label", (res == ERR_OK), return res);
  res = mdns_domain_add_label(domain, NULL, 0);
  LWIP_ERROR("mdns_build_reverse_v4_domain: Failed to add label", (res == ERR_OK), return res);

  return ERR_OK;
}
#endif

#if LWIP_IPV6
/**
 * Build domain for reverse lookup of IP address
 * like b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa. for 2001:db8::567:89ab
 * @param domain Where to write the domain name
 * @param addr Pointer to an IPv6 address to encode
 * @return ERR_OK if domain was written, an err_t otherwise
 */
static err_t
mdns_build_reverse_v6_domain(struct mdns_domain *domain, const ip6_addr_t *addr)
{
  int i;
  err_t res;
  const u8_t *ptr;
  if (!domain || !addr) {
    return ERR_ARG;
  }
  memset(domain, 0, sizeof(struct mdns_domain));
  ptr = (const u8_t *) addr;
  for (i = sizeof(ip6_addr_p_t) - 1; i >= 0; i--) {
    char buf;
    u8_t byte = ptr[i];
    int j;
    for (j = 0; j < 2; j++) {
      if ((byte & 0x0F) < 0xA) {
        buf = '0' + (byte & 0x0F);
      } else {
        buf = 'a' + (byte & 0x0F) - 0xA;
      }
      res = mdns_domain_add_label(domain, &buf, sizeof(buf));
      LWIP_ERROR("mdns_build_reverse_v6_domain: Failed to add label", (res == ERR_OK), return res);
      byte >>= 4;
    }
  }
  res = mdns_domain_add_label(domain, REVERSE_PTR_V6_DOMAIN, (u8_t)(sizeof(REVERSE_PTR_V6_DOMAIN) - 1));
  LWIP_ERROR("mdns_build_reverse_v6_domain: Failed to add label", (res == ERR_OK), return res);
  res = mdns_domain_add_label(domain, REVERSE_PTR_TOPDOMAIN, (u8_t)(sizeof(REVERSE_PTR_TOPDOMAIN) - 1));
  LWIP_ERROR("mdns_build_reverse_v6_domain: Failed to add label", (res == ERR_OK), return res);
  res = mdns_domain_add_label(domain, NULL, 0);
  LWIP_ERROR("mdns_build_reverse_v6_domain: Failed to add label", (res == ERR_OK), return res);

  return ERR_OK;
}
#endif

/* Add .local. to domain */
static err_t
mdns_add_dotlocal(struct mdns_domain *domain)
{
  err_t res = mdns_domain_add_label(domain, TOPDOMAIN_LOCAL, (u8_t)(sizeof(TOPDOMAIN_LOCAL) - 1));
  LWIP_ERROR("mdns_add_dotlocal: Failed to add label", (res == ERR_OK), return res);
  return mdns_domain_add_label(domain, NULL, 0);
}

/**
 * Build the <hostname>.local. domain name
 * @param domain Where to write the domain name
 * @param mdns TMDNS netif descriptor.
 * @return ERR_OK if domain <hostname>.local. was written, an err_t otherwise
 */
static err_t
mdns_build_host_domain(struct mdns_domain *domain, struct mdns_host *mdns)
{
  err_t res;
  memset(domain, 0, sizeof(struct mdns_domain));
  LWIP_ERROR("mdns_build_host_domain: mdns != NULL", (mdns != NULL), return ERR_VAL);
  res = mdns_domain_add_label(domain, mdns->name, (u8_t)strlen(mdns->name));
  LWIP_ERROR("mdns_build_host_domain: Failed to add label", (res == ERR_OK), return res);
  return mdns_add_dotlocal(domain);
}

/**
 * Build the lookup-all-services special DNS-SD domain name
 * @param domain Where to write the domain name
 * @return ERR_OK if domain _services._dns-sd._udp.local. was written, an err_t otherwise
 */
static err_t
mdns_build_dnssd_domain(struct mdns_domain *domain)
{
  err_t res;
  memset(domain, 0, sizeof(struct mdns_domain));
  res = mdns_domain_add_label(domain, "_services", (u8_t)(sizeof("_services") - 1));
  LWIP_ERROR("mdns_build_dnssd_domain: Failed to add label", (res == ERR_OK), return res);
  res = mdns_domain_add_label(domain, "_dns-sd", (u8_t)(sizeof("_dns-sd") - 1));
  LWIP_ERROR("mdns_build_dnssd_domain: Failed to add label", (res == ERR_OK), return res);
  res = mdns_domain_add_label(domain, dnssd_protos[DNSSD_PROTO_UDP], (u8_t)strlen(dnssd_protos[DNSSD_PROTO_UDP]));
  LWIP_ERROR("mdns_build_dnssd_domain: Failed to add label", (res == ERR_OK), return res);
  return mdns_add_dotlocal(domain);
}

/**
 * Build domain name for a service
 * @param domain Where to write the domain name
 * @param service The service struct, containing service name, type and protocol
 * @param include_name Whether to include the service name in the domain
 * @return ERR_OK if domain was written. If service name is included,
 *         <name>.<type>.<proto>.local. will be written, otherwise <type>.<proto>.local.
 *         An err_t is returned on error.
 */
static err_t
mdns_build_service_domain(struct mdns_domain *domain, struct mdns_service *service, int include_name)
{
  err_t res;
  memset(domain, 0, sizeof(struct mdns_domain));
  if (include_name) {
    res = mdns_domain_add_label(domain, service->name, (u8_t)strlen(service->name));
    LWIP_ERROR("mdns_build_service_domain: Failed to add label", (res == ERR_OK), return res);
  }
  res = mdns_domain_add_label(domain, service->service, (u8_t)strlen(service->service));
  LWIP_ERROR("mdns_build_service_domain: Failed to add label", (res == ERR_OK), return res);
  res = mdns_domain_add_label(domain, dnssd_protos[service->proto], (u8_t)strlen(dnssd_protos[service->proto]));
  LWIP_ERROR("mdns_build_service_domain: Failed to add label", (res == ERR_OK), return res);
  return mdns_add_dotlocal(domain);
}

/**
 * Check which replies we should send for a host/netif based on question
 * @param netif The network interface that received the question
 * @param rr Domain/type/class from a question
 * @param reverse_v6_reply Bitmask of which IPv6 addresses to send reverse PTRs for
 *                         if reply bit has REPLY_HOST_PTR_V6 set
 * @return Bitmask of which replies to send
 */
static int
check_host(struct netif *netif, struct mdns_rr_info *rr, u8_t *reverse_v6_reply)
{
  err_t res;
  int replies = 0;
  struct mdns_domain mydomain;

  LWIP_UNUSED_ARG(reverse_v6_reply); /* if ipv6 is disabled */

  if (rr->klass != DNS_RRCLASS_IN && rr->klass != DNS_RRCLASS_ANY) {
    /* Invalid class */
    return replies;
  }

  /* Handle PTR for our addresses */
  if (rr->type == DNS_RRTYPE_PTR || rr->type == DNS_RRTYPE_ANY) {
#if LWIP_IPV6
    int i;
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
        res = mdns_build_reverse_v6_domain(&mydomain, netif_ip6_addr(netif, i));
        if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain)) {
          replies |= REPLY_HOST_PTR_V6;
          /* Mark which addresses where requested */
          if (reverse_v6_reply) {
            *reverse_v6_reply |= (1 << i);
          }
        }
      }
    }
#endif
#if LWIP_IPV4
    if (!ip4_addr_isany_val(*netif_ip4_addr(netif))) {
      res = mdns_build_reverse_v4_domain(&mydomain, netif_ip4_addr(netif));
      if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain)) {
        replies |= REPLY_HOST_PTR_V4;
      }
    }
#endif
  }

  res = mdns_build_host_domain(&mydomain, NETIF_TO_HOST(netif));
  /* Handle requests for our hostname */
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain)) {
    /* TODO return NSEC if unsupported protocol requested */
#if LWIP_IPV4
    if (!ip4_addr_isany_val(*netif_ip4_addr(netif))
        && (rr->type == DNS_RRTYPE_A || rr->type == DNS_RRTYPE_ANY)) {
      replies |= REPLY_HOST_A;
    }
#endif
#if LWIP_IPV6
    if (rr->type == DNS_RRTYPE_AAAA || rr->type == DNS_RRTYPE_ANY) {
      replies |= REPLY_HOST_AAAA;
    }
#endif
  }

  return replies;
}

/**
 * Check which replies we should send for a service based on question
 * @param service A registered MDNS service
 * @param rr Domain/type/class from a question
 * @return Bitmask of which replies to send
 */
static int
check_service(struct mdns_service *service, struct mdns_rr_info *rr)
{
  err_t res;
  int replies = 0;
  struct mdns_domain mydomain;

  if (rr->klass != DNS_RRCLASS_IN && rr->klass != DNS_RRCLASS_ANY) {
    /* Invalid class */
    return 0;
  }

  res = mdns_build_dnssd_domain(&mydomain);
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain) &&
      (rr->type == DNS_RRTYPE_PTR || rr->type == DNS_RRTYPE_ANY)) {
    /* Request for all service types */
    replies |= REPLY_SERVICE_TYPE_PTR;
  }

  res = mdns_build_service_domain(&mydomain, service, 0);
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain) &&
      (rr->type == DNS_RRTYPE_PTR || rr->type == DNS_RRTYPE_ANY)) {
    /* Request for the instance of my service */
    replies |= REPLY_SERVICE_NAME_PTR;
  }

  res = mdns_build_service_domain(&mydomain, service, 1);
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain)) {
    /* Request for info about my service */
    if (rr->type == DNS_RRTYPE_SRV || rr->type == DNS_RRTYPE_ANY) {
      replies |= REPLY_SERVICE_SRV;
    }
    if (rr->type == DNS_RRTYPE_TXT || rr->type == DNS_RRTYPE_ANY) {
      replies |= REPLY_SERVICE_TXT;
    }
  }

  return replies;
}

/**
 * Return bytes needed to write before jump for best result of compressing supplied domain
 * against domain in outpacket starting at specified offset.
 * If a match is found, offset is updated to where to jump to
 * @param pbuf Pointer to pbuf with the partially constructed DNS packet
 * @param offset Start position of a domain written earlier. If this location is suitable
 *               for compression, the pointer is updated to where in the domain to jump to.
 * @param domain The domain to write
 * @return Number of bytes to write of the new domain before writing a jump to the offset.
 *         If compression can not be done against this previous domain name, the full new
 *         domain length is returned.
 */
u16_t
mdns_compress_domain(struct pbuf *pbuf, u16_t *offset, struct mdns_domain *domain)
{
  struct mdns_domain target;
  u16_t target_end;
  u8_t target_len;
  u8_t writelen = 0;
  u8_t *ptr;
  if (pbuf == NULL) {
    return domain->length;
  }
  target_end = mdns_readname(pbuf, *offset, &target);
  if (target_end == MDNS_READNAME_ERROR) {
    return domain->length;
  }
  target_len = (u8_t)(target_end - *offset);
  ptr = domain->name;
  while (writelen < domain->length) {
    u8_t domainlen = (u8_t)(domain->length - writelen);
    u8_t labellen;
    if (domainlen <= target.length && domainlen > DOMAIN_JUMP_SIZE) {
      /* Compare domains if target is long enough, and we have enough left of the domain */
      u8_t targetpos = (u8_t)(target.length - domainlen);
      if ((targetpos + DOMAIN_JUMP_SIZE) >= target_len) {
        /* We are checking at or beyond a jump in the original, stop looking */
        break;
      }
      if (target.length >= domainlen &&
          memcmp(&domain->name[writelen], &target.name[targetpos], domainlen) == 0) {
        *offset += targetpos;
        return writelen;
      }
    }
    /* Skip to next label in domain */
    labellen = *ptr;
    writelen += 1 + labellen;
    ptr += 1 + labellen;
  }
  /* Nothing found */
  return domain->length;
}

/**
 * Write domain to outpacket. Compression will be attempted,
 * unless domain->skip_compression is set.
 * @param outpkt The outpacket to write to
 * @param domain The domain name to write
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_write_domain(struct mdns_outpacket *outpkt, struct mdns_domain *domain)
{
  int i;
  err_t res;
  u16_t writelen = domain->length;
  u16_t jump_offset = 0;
  u16_t jump;

  if (!domain->skip_compression) {
    for (i = 0; i < NUM_DOMAIN_OFFSETS; ++i) {
      u16_t offset = outpkt->domain_offsets[i];
      if (offset) {
        u16_t len = mdns_compress_domain(outpkt->pbuf, &offset, domain);
        if (len < writelen) {
          writelen = len;
          jump_offset = offset;
        }
      }
    }
  }

  if (writelen) {
    /* Write uncompressed part of name */
    res = pbuf_take_at(outpkt->pbuf, domain->name, writelen, outpkt->write_offset);
    if (res != ERR_OK) {
      return res;
    }

    /* Store offset of this new domain */
    for (i = 0; i < NUM_DOMAIN_OFFSETS; ++i) {
      if (outpkt->domain_offsets[i] == 0) {
        outpkt->domain_offsets[i] = outpkt->write_offset;
        break;
      }
    }

    outpkt->write_offset += writelen;
  }
  if (jump_offset) {
    /* Write jump */
    jump = lwip_htons(DOMAIN_JUMP | jump_offset);
    res = pbuf_take_at(outpkt->pbuf, &jump, DOMAIN_JUMP_SIZE, outpkt->write_offset);
    if (res != ERR_OK) {
      return res;
    }
    outpkt->write_offset += DOMAIN_JUMP_SIZE;
  }
  return ERR_OK;
}

/**
 * Write a question to an outpacket
 * A question contains domain, type and class. Since an answer also starts with these fields this function is also
 * called from mdns_add_answer().
 * @param outpkt The outpacket to write to
 * @param domain The domain name the answer is for
 * @param type The DNS type of the answer (like 'AAAA', 'SRV')
 * @param klass The DNS type of the answer (like 'IN')
 * @param unicast If highest bit in class should be set, to instruct the responder to
 *                reply with a unicast packet
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_add_question(struct mdns_outpacket *outpkt, struct mdns_domain *domain, u16_t type, u16_t klass, u16_t unicast)
{
  u16_t question_len;
  u16_t field16;
  err_t res;

  if (!outpkt->pbuf) {
    /* If no pbuf is active, allocate one */
    outpkt->pbuf = pbuf_alloc(PBUF_TRANSPORT, OUTPACKET_SIZE, PBUF_RAM);
    if (!outpkt->pbuf) {
      return ERR_MEM;
    }
    outpkt->write_offset = SIZEOF_DNS_HDR;
  }

  /* Worst case calculation. Domain string might be compressed */
  question_len = domain->length + sizeof(type) + sizeof(klass);
  if (outpkt->write_offset + question_len > outpkt->pbuf->tot_len) {
    /* No space */
    return ERR_MEM;
  }

  /* Write name */
  res = mdns_write_domain(outpkt, domain);
  if (res != ERR_OK) {
    return res;
  }

  /* Write type */
  field16 = lwip_htons(type);
  res = pbuf_take_at(outpkt->pbuf, &field16, sizeof(field16), outpkt->write_offset);
  if (res != ERR_OK) {
    return res;
  }
  outpkt->write_offset += sizeof(field16);

  /* Write class */
  if (unicast) {
    klass |= 0x8000;
  }
  field16 = lwip_htons(klass);
  res = pbuf_take_at(outpkt->pbuf, &field16, sizeof(field16), outpkt->write_offset);
  if (res != ERR_OK) {
    return res;
  }
  outpkt->write_offset += sizeof(field16);

  return ERR_OK;
}

/**
 * Write answer to reply packet.
 * buf or answer_domain can be null. The rd_length written will be buf_length +
 * size of (compressed) domain. Most uses will need either buf or answer_domain,
 * special case is SRV that starts with 3 u16 and then a domain name.
 * @param reply The outpacket to write to
 * @param domain The domain name the answer is for
 * @param type The DNS type of the answer (like 'AAAA', 'SRV')
 * @param klass The DNS type of the answer (like 'IN')
 * @param cache_flush If highest bit in class should be set, to instruct receiver that
 *                    this reply replaces any earlier answer for this domain/type/class
 * @param ttl Validity time in seconds to send out for IP address data in DNS replies
 * @param buf Pointer to buffer of answer data
 * @param buf_length Length of variable data
 * @param answer_domain A domain to write after any buffer data as answer
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_add_answer(struct mdns_outpacket *reply, struct mdns_domain *domain, u16_t type, u16_t klass, u16_t cache_flush,
                u32_t ttl, const u8_t *buf, size_t buf_length, struct mdns_domain *answer_domain)
{
  u16_t answer_len;
  u16_t field16;
  u16_t rdlen_offset;
  u16_t answer_offset;
  u32_t field32;
  err_t res;

  if (!reply->pbuf) {
    /* If no pbuf is active, allocate one */
    reply->pbuf = pbuf_alloc(PBUF_TRANSPORT, OUTPACKET_SIZE, PBUF_RAM);
    if (!reply->pbuf) {
      return ERR_MEM;
    }
    reply->write_offset = SIZEOF_DNS_HDR;
  }

  /* Worst case calculation. Domain strings might be compressed */
  answer_len = domain->length + sizeof(type) + sizeof(klass) + sizeof(ttl) + sizeof(field16)/*rd_length*/;
  if (buf) {
    answer_len += (u16_t)buf_length;
  }
  if (answer_domain) {
    answer_len += answer_domain->length;
  }
  if (reply->write_offset + answer_len > reply->pbuf->tot_len) {
    /* No space */
    return ERR_MEM;
  }

  /* Answer starts with same data as question, then more fields */
  mdns_add_question(reply, domain, type, klass, cache_flush);

  /* Write TTL */
  field32 = lwip_htonl(ttl);
  res = pbuf_take_at(reply->pbuf, &field32, sizeof(field32), reply->write_offset);
  if (res != ERR_OK) {
    return res;
  }
  reply->write_offset += sizeof(field32);

  /* Store offsets and skip forward to the data */
  rdlen_offset = reply->write_offset;
  reply->write_offset += sizeof(field16);
  answer_offset = reply->write_offset;

  if (buf) {
    /* Write static data */
    res = pbuf_take_at(reply->pbuf, buf, (u16_t)buf_length, reply->write_offset);
    if (res != ERR_OK) {
      return res;
    }
    reply->write_offset += (u16_t)buf_length;
  }

  if (answer_domain) {
    /* Write name answer (compressed if possible) */
    res = mdns_write_domain(reply, answer_domain);
    if (res != ERR_OK) {
      return res;
    }
  }

  /* Write rd_length after when we know the answer size */
  field16 = lwip_htons(reply->write_offset - answer_offset);
  res = pbuf_take_at(reply->pbuf, &field16, sizeof(field16), rdlen_offset);

  return res;
}

/**
 * Helper function for mdns_read_question/mdns_read_answer
 * Reads a domain, type and class from the packet
 * @param pkt The MDNS packet to read from. The parse_offset field will be
 *            incremented to point to the next unparsed byte.
 * @param info The struct to fill with domain, type and class
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_read_rr_info(struct mdns_packet *pkt, struct mdns_rr_info *info)
{
  u16_t field16, copied;
  pkt->parse_offset = mdns_readname(pkt->pbuf, pkt->parse_offset, &info->domain);
  if (pkt->parse_offset == MDNS_READNAME_ERROR) {
    return ERR_VAL;
  }

  copied = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), pkt->parse_offset);
  if (copied != sizeof(field16)) {
    return ERR_VAL;
  }
  pkt->parse_offset += copied;
  info->type = lwip_ntohs(field16);

  copied = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), pkt->parse_offset);
  if (copied != sizeof(field16)) {
    return ERR_VAL;
  }
  pkt->parse_offset += copied;
  info->klass = lwip_ntohs(field16);

  return ERR_OK;
}

/**
 * Read a question from the packet.
 * All questions have to be read before the answers.
 * @param pkt The MDNS packet to read from. The questions_left field will be decremented
 *            and the parse_offset will be updated.
 * @param question The struct to fill with question data
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_read_question(struct mdns_packet *pkt, struct mdns_question *question)
{
  /* Safety check */
  if (pkt->pbuf->tot_len < pkt->parse_offset) {
    return ERR_VAL;
  }

  if (pkt->questions_left) {
    err_t res;
    pkt->questions_left--;

    memset(question, 0, sizeof(struct mdns_question));
    res = mdns_read_rr_info(pkt, &question->info);
    if (res != ERR_OK) {
      return res;
    }

    /* Extract unicast flag from class field */
    question->unicast = question->info.klass & 0x8000;
    question->info.klass &= 0x7FFF;

    return ERR_OK;
  }
  return ERR_VAL;
}

/**
 * Read an answer from the packet
 * The variable length reply is not copied, its pbuf offset and length is stored instead.
 * @param pkt The MDNS packet to read. The answers_left field will be decremented and
 *            the parse_offset will be updated.
 * @param answer The struct to fill with answer data
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_read_answer(struct mdns_packet *pkt, struct mdns_answer *answer)
{
  /* Read questions first */
  if (pkt->questions_left) {
    return ERR_VAL;
  }

  /* Safety check */
  if (pkt->pbuf->tot_len < pkt->parse_offset) {
    return ERR_VAL;
  }

  if (pkt->answers_left) {
    u16_t copied, field16;
    u32_t ttl;
    err_t res;
    pkt->answers_left--;

    memset(answer, 0, sizeof(struct mdns_answer));
    res = mdns_read_rr_info(pkt, &answer->info);
    if (res != ERR_OK) {
      return res;
    }

    /* Extract cache_flush flag from class field */
    answer->cache_flush = answer->info.klass & 0x8000;
    answer->info.klass &= 0x7FFF;

    copied = pbuf_copy_partial(pkt->pbuf, &ttl, sizeof(ttl), pkt->parse_offset);
    if (copied != sizeof(ttl)) {
      return ERR_VAL;
    }
    pkt->parse_offset += copied;
    answer->ttl = lwip_ntohl(ttl);

    copied = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), pkt->parse_offset);
    if (copied != sizeof(field16)) {
      return ERR_VAL;
    }
    pkt->parse_offset += copied;
    answer->rd_length = lwip_ntohs(field16);

    answer->rd_offset = pkt->parse_offset;
    pkt->parse_offset += answer->rd_length;

    return ERR_OK;
  }
  return ERR_VAL;
}

#if LWIP_IPV4
/** Write an IPv4 address (A) RR to outpacket */
static err_t
mdns_add_a_answer(struct mdns_outpacket *reply, u16_t cache_flush, struct netif *netif)
{
  struct mdns_domain host;
  mdns_build_host_domain(&host, NETIF_TO_HOST(netif));
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with A record\n"));
  return mdns_add_answer(reply, &host, DNS_RRTYPE_A, DNS_RRCLASS_IN, cache_flush, (NETIF_TO_HOST(netif))->dns_ttl, (const u8_t *) netif_ip4_addr(netif), sizeof(ip4_addr_t), NULL);
}

/** Write a 4.3.2.1.in-addr.arpa -> hostname.local PTR RR to outpacket */
static err_t
mdns_add_hostv4_ptr_answer(struct mdns_outpacket *reply, u16_t cache_flush, struct netif *netif)
{
  struct mdns_domain host, revhost;
  mdns_build_host_domain(&host, NETIF_TO_HOST(netif));
  mdns_build_reverse_v4_domain(&revhost, netif_ip4_addr(netif));
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with v4 PTR record\n"));
  return mdns_add_answer(reply, &revhost, DNS_RRTYPE_PTR, DNS_RRCLASS_IN, cache_flush, (NETIF_TO_HOST(netif))->dns_ttl, NULL, 0, &host);
}
#endif

#if LWIP_IPV6
/** Write an IPv6 address (AAAA) RR to outpacket */
static err_t
mdns_add_aaaa_answer(struct mdns_outpacket *reply, u16_t cache_flush, struct netif *netif, int addrindex)
{
  struct mdns_domain host;
  mdns_build_host_domain(&host, NETIF_TO_HOST(netif));
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with AAAA record\n"));
  return mdns_add_answer(reply, &host, DNS_RRTYPE_AAAA, DNS_RRCLASS_IN, cache_flush, (NETIF_TO_HOST(netif))->dns_ttl, (const u8_t *) netif_ip6_addr(netif, addrindex), sizeof(ip6_addr_p_t), NULL);
}

/** Write a x.y.z.ip6.arpa -> hostname.local PTR RR to outpacket */
static err_t
mdns_add_hostv6_ptr_answer(struct mdns_outpacket *reply, u16_t cache_flush, struct netif *netif, int addrindex)
{
  struct mdns_domain host, revhost;
  mdns_build_host_domain(&host, NETIF_TO_HOST(netif));
  mdns_build_reverse_v6_domain(&revhost, netif_ip6_addr(netif, addrindex));
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with v6 PTR record\n"));
  return mdns_add_answer(reply, &revhost, DNS_RRTYPE_PTR, DNS_RRCLASS_IN, cache_flush, (NETIF_TO_HOST(netif))->dns_ttl, NULL, 0, &host);
}
#endif

/** Write an all-services -> servicetype PTR RR to outpacket */
static err_t
mdns_add_servicetype_ptr_answer(struct mdns_outpacket *reply, struct mdns_service *service)
{
  struct mdns_domain service_type, service_dnssd;
  mdns_build_service_domain(&service_type, service, 0);
  mdns_build_dnssd_domain(&service_dnssd);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with service type PTR record\n"));
  return mdns_add_answer(reply, &service_dnssd, DNS_RRTYPE_PTR, DNS_RRCLASS_IN, 0, service->dns_ttl, NULL, 0, &service_type);
}

/** Write a servicetype -> servicename PTR RR to outpacket */
static err_t
mdns_add_servicename_ptr_answer(struct mdns_outpacket *reply, struct mdns_service *service)
{
  struct mdns_domain service_type, service_instance;
  mdns_build_service_domain(&service_type, service, 0);
  mdns_build_service_domain(&service_instance, service, 1);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with service name PTR record\n"));
  return mdns_add_answer(reply, &service_type, DNS_RRTYPE_PTR, DNS_RRCLASS_IN, 0, service->dns_ttl, NULL, 0, &service_instance);
}

/** Write a SRV RR to outpacket */
static err_t
mdns_add_srv_answer(struct mdns_outpacket *reply, u16_t cache_flush, struct mdns_host *mdns, struct mdns_service *service)
{
  struct mdns_domain service_instance, srvhost;
  u16_t srvdata[3];
  mdns_build_service_domain(&service_instance, service, 1);
  mdns_build_host_domain(&srvhost, mdns);
  if (reply->legacy_query) {
    /* RFC 6762 section 18.14:
     * In legacy unicast responses generated to answer legacy queries,
     * name compression MUST NOT be performed on SRV records.
     */
    srvhost.skip_compression = 1;
  }
  srvdata[0] = lwip_htons(SRV_PRIORITY);
  srvdata[1] = lwip_htons(SRV_WEIGHT);
  srvdata[2] = lwip_htons(service->port);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with SRV record\n"));
  return mdns_add_answer(reply, &service_instance, DNS_RRTYPE_SRV, DNS_RRCLASS_IN, cache_flush, service->dns_ttl,
                         (const u8_t *) &srvdata, sizeof(srvdata), &srvhost);
}

/** Write a TXT RR to outpacket */
static err_t
mdns_add_txt_answer(struct mdns_outpacket *reply, u16_t cache_flush, struct mdns_service *service)
{
  struct mdns_domain service_instance;
  mdns_build_service_domain(&service_instance, service, 1);
  mdns_prepare_txtdata(service);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with TXT record\n"));
  return mdns_add_answer(reply, &service_instance, DNS_RRTYPE_TXT, DNS_RRCLASS_IN, cache_flush, service->dns_ttl,
                         (u8_t *) &service->txtdata.name, service->txtdata.length, NULL);
}

/**
 * Setup outpacket as a reply to the incoming packet
 */
static void
mdns_init_outpacket(struct mdns_outpacket *out, struct mdns_packet *in)
{
  memset(out, 0, sizeof(struct mdns_outpacket));
  out->cache_flush = 1;
  out->netif = in->netif;

  /* Copy source IP/port to use when responding unicast, or to choose
   * which pcb to use for multicast (IPv4/IPv6)
   */
  SMEMCPY(&out->dest_addr, &in->source_addr, sizeof(ip_addr_t));
  out->dest_port = in->source_port;

  if (in->source_port != LWIP_IANA_PORT_MDNS) {
    out->unicast_reply = 1;
    out->cache_flush = 0;
    if (in->questions == 1) {
      out->legacy_query = 1;
      out->tx_id = in->tx_id;
    }
  }

  if (in->recv_unicast) {
    out->unicast_reply = 1;
  }
}

/**
 * Send chosen answers as a reply
 *
 * Add all selected answers (first write will allocate pbuf)
 * Add additional answers based on the selected answers
 * Send the packet
 */
static void
mdns_send_outpacket(struct mdns_outpacket *outpkt)
{
  struct mdns_service *service;
  err_t res;
  int i;
  struct mdns_host *mdns = NETIF_TO_HOST(outpkt->netif);

  /* Write answers to host questions */
#if LWIP_IPV4
  if (outpkt->host_replies & REPLY_HOST_A) {
    res = mdns_add_a_answer(outpkt, outpkt->cache_flush, outpkt->netif);
    if (res != ERR_OK) {
      goto cleanup;
    }
    outpkt->answers++;
  }
  if (outpkt->host_replies & REPLY_HOST_PTR_V4) {
    res = mdns_add_hostv4_ptr_answer(outpkt, outpkt->cache_flush, outpkt->netif);
    if (res != ERR_OK) {
      goto cleanup;
    }
    outpkt->answers++;
  }
#endif
#if LWIP_IPV6
  if (outpkt->host_replies & REPLY_HOST_AAAA) {
    int addrindex;
    for (addrindex = 0; addrindex < LWIP_IPV6_NUM_ADDRESSES; ++addrindex) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(outpkt->netif, addrindex))) {
        res = mdns_add_aaaa_answer(outpkt, outpkt->cache_flush, outpkt->netif, addrindex);
        if (res != ERR_OK) {
          goto cleanup;
        }
        outpkt->answers++;
      }
    }
  }
  if (outpkt->host_replies & REPLY_HOST_PTR_V6) {
    u8_t rev_addrs = outpkt->host_reverse_v6_replies;
    int addrindex = 0;
    while (rev_addrs) {
      if (rev_addrs & 1) {
        res = mdns_add_hostv6_ptr_answer(outpkt, outpkt->cache_flush, outpkt->netif, addrindex);
        if (res != ERR_OK) {
          goto cleanup;
        }
        outpkt->answers++;
      }
      addrindex++;
      rev_addrs >>= 1;
    }
  }
#endif

  /* Write answers to service questions */
  for (i = 0; i < MDNS_MAX_SERVICES; ++i) {
    service = mdns->services[i];
    if (!service) {
      continue;
    }

    if (outpkt->serv_replies[i] & REPLY_SERVICE_TYPE_PTR) {
      res = mdns_add_servicetype_ptr_answer(outpkt, service);
      if (res != ERR_OK) {
        goto cleanup;
      }
      outpkt->answers++;
    }

    if (outpkt->serv_replies[i] & REPLY_SERVICE_NAME_PTR) {
      res = mdns_add_servicename_ptr_answer(outpkt, service);
      if (res != ERR_OK) {
        goto cleanup;
      }
      outpkt->answers++;
    }

    if (outpkt->serv_replies[i] & REPLY_SERVICE_SRV) {
      res = mdns_add_srv_answer(outpkt, outpkt->cache_flush, mdns, service);
      if (res != ERR_OK) {
        goto cleanup;
      }
      outpkt->answers++;
    }

    if (outpkt->serv_replies[i] & REPLY_SERVICE_TXT) {
      res = mdns_add_txt_answer(outpkt, outpkt->cache_flush, service);
      if (res != ERR_OK) {
        goto cleanup;
      }
      outpkt->answers++;
    }
  }

  /* All answers written, add additional RRs */
  for (i = 0; i < MDNS_MAX_SERVICES; ++i) {
    service = mdns->services[i];
    if (!service) {
      continue;
    }

    if (outpkt->serv_replies[i] & REPLY_SERVICE_NAME_PTR) {
      /* Our service instance requested, include SRV & TXT
       * if they are already not requested. */
      if (!(outpkt->serv_replies[i] & REPLY_SERVICE_SRV)) {
        res = mdns_add_srv_answer(outpkt, outpkt->cache_flush, mdns, service);
        if (res != ERR_OK) {
          goto cleanup;
        }
        outpkt->additional++;
      }

      if (!(outpkt->serv_replies[i] & REPLY_SERVICE_TXT)) {
        res = mdns_add_txt_answer(outpkt, outpkt->cache_flush, service);
        if (res != ERR_OK) {
          goto cleanup;
        }
        outpkt->additional++;
      }
    }

    /* If service instance, SRV, record or an IP address is requested,
     * supply all addresses for the host
     */
    if ((outpkt->serv_replies[i] & (REPLY_SERVICE_NAME_PTR | REPLY_SERVICE_SRV)) ||
        (outpkt->host_replies & (REPLY_HOST_A | REPLY_HOST_AAAA))) {
#if LWIP_IPV6
      if (!(outpkt->host_replies & REPLY_HOST_AAAA)) {
        int addrindex;
        for (addrindex = 0; addrindex < LWIP_IPV6_NUM_ADDRESSES; ++addrindex) {
          if (ip6_addr_isvalid(netif_ip6_addr_state(outpkt->netif, addrindex))) {
            res = mdns_add_aaaa_answer(outpkt, outpkt->cache_flush, outpkt->netif, addrindex);
            if (res != ERR_OK) {
              goto cleanup;
            }
            outpkt->additional++;
          }
        }
      }
#endif
#if LWIP_IPV4
      if (!(outpkt->host_replies & REPLY_HOST_A)) {
        res = mdns_add_a_answer(outpkt, outpkt->cache_flush, outpkt->netif);
        if (res != ERR_OK) {
          goto cleanup;
        }
        outpkt->additional++;
      }
#endif
    }
  }

  if (outpkt->pbuf) {
    const ip_addr_t *mcast_destaddr;
    struct dns_hdr hdr;

    /* Write header */
    memset(&hdr, 0, sizeof(hdr));
    hdr.flags1 = DNS_FLAG1_RESPONSE | DNS_FLAG1_AUTHORATIVE;
    hdr.numanswers = lwip_htons(outpkt->answers);
    hdr.numextrarr = lwip_htons(outpkt->additional);
    if (outpkt->legacy_query) {
      hdr.numquestions = lwip_htons(1);
      hdr.id = lwip_htons(outpkt->tx_id);
    }
    pbuf_take(outpkt->pbuf, &hdr, sizeof(hdr));

    /* Shrink packet */
    pbuf_realloc(outpkt->pbuf, outpkt->write_offset);

    if (IP_IS_V6_VAL(outpkt->dest_addr)) {
#if LWIP_IPV6
      mcast_destaddr = &v6group;
#endif
    } else {
#if LWIP_IPV4
      mcast_destaddr = &v4group;
#endif
    }
    /* Send created packet */
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Sending packet, len=%d, unicast=%d\n", outpkt->write_offset, outpkt->unicast_reply));
    if (outpkt->unicast_reply) {
      udp_sendto_if(mdns_pcb, outpkt->pbuf, &outpkt->dest_addr, outpkt->dest_port, outpkt->netif);
    } else {
      udp_sendto_if(mdns_pcb, outpkt->pbuf, mcast_destaddr, LWIP_IANA_PORT_MDNS, outpkt->netif);
    }
  }

cleanup:
  if (outpkt->pbuf) {
    pbuf_free(outpkt->pbuf);
    outpkt->pbuf = NULL;
  }
}

/**
 * Send unsolicited answer containing all our known data
 * @param netif The network interface to send on
 * @param destination The target address to send to (usually multicast address)
 */
static void
mdns_announce(struct netif *netif, const ip_addr_t *destination)
{
  struct mdns_outpacket announce;
  int i;
  struct mdns_host *mdns = NETIF_TO_HOST(netif);

  memset(&announce, 0, sizeof(announce));
  announce.netif = netif;
  announce.cache_flush = 1;
#if LWIP_IPV4
  if (!ip4_addr_isany_val(*netif_ip4_addr(netif))) {
    announce.host_replies = REPLY_HOST_A | REPLY_HOST_PTR_V4;
  }
#endif
#if LWIP_IPV6
  for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
    if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
      announce.host_replies |= REPLY_HOST_AAAA | REPLY_HOST_PTR_V6;
      announce.host_reverse_v6_replies |= (1 << i);
    }
  }
#endif

  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    struct mdns_service *serv = mdns->services[i];
    if (serv) {
      announce.serv_replies[i] = REPLY_SERVICE_TYPE_PTR | REPLY_SERVICE_NAME_PTR |
                                 REPLY_SERVICE_SRV | REPLY_SERVICE_TXT;
    }
  }

  announce.dest_port = LWIP_IANA_PORT_MDNS;
  SMEMCPY(&announce.dest_addr, destination, sizeof(announce.dest_addr));
  mdns_send_outpacket(&announce);
}

/**
 * Handle question MDNS packet
 * 1. Parse all questions and set bits what answers to send
 * 2. Clear pending answers if known answers are supplied
 * 3. Put chosen answers in new packet and send as reply
 */
static void
mdns_handle_question(struct mdns_packet *pkt)
{
  struct mdns_service *service;
  struct mdns_outpacket reply;
  int replies = 0;
  int i;
  err_t res;
  struct mdns_host *mdns = NETIF_TO_HOST(pkt->netif);

  mdns_init_outpacket(&reply, pkt);

  while (pkt->questions_left) {
    struct mdns_question q;

    res = mdns_read_question(pkt, &q);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse question, skipping query packet\n"));
      return;
    }

    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Query for domain "));
    mdns_domain_debug_print(&q.info.domain);
    LWIP_DEBUGF(MDNS_DEBUG, (" type %d class %d\n", q.info.type, q.info.klass));

    if (q.unicast) {
      /* Reply unicast if any question is unicast */
      reply.unicast_reply = 1;
    }

    reply.host_replies |= check_host(pkt->netif, &q.info, &reply.host_reverse_v6_replies);
    replies |= reply.host_replies;

    for (i = 0; i < MDNS_MAX_SERVICES; ++i) {
      service = mdns->services[i];
      if (!service) {
        continue;
      }
      reply.serv_replies[i] |= check_service(service, &q.info);
      replies |= reply.serv_replies[i];
    }

    if (replies && reply.legacy_query) {
      /* Add question to reply packet (legacy packet only has 1 question) */
      res = mdns_add_question(&reply, &q.info.domain, q.info.type, q.info.klass, 0);
      if (res != ERR_OK) {
        goto cleanup;
      }
    }
  }

  /* Handle known answers */
  while (pkt->answers_left) {
    struct mdns_answer ans;
    u8_t rev_v6;
    int match;

    res = mdns_read_answer(pkt, &ans);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping query packet\n"));
      goto cleanup;
    }

    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Known answer for domain "));
    mdns_domain_debug_print(&ans.info.domain);
    LWIP_DEBUGF(MDNS_DEBUG, (" type %d class %d\n", ans.info.type, ans.info.klass));


    if (ans.info.type == DNS_RRTYPE_ANY || ans.info.klass == DNS_RRCLASS_ANY) {
      /* Skip known answers for ANY type & class */
      continue;
    }

    rev_v6 = 0;
    match = reply.host_replies & check_host(pkt->netif, &ans.info, &rev_v6);
    if (match && (ans.ttl > (mdns->dns_ttl / 2))) {
      /* The RR in the known answer matches an RR we are planning to send,
       * and the TTL is less than half gone.
       * If the payload matches we should not send that answer.
       */
      if (ans.info.type == DNS_RRTYPE_PTR) {
        /* Read domain and compare */
        struct mdns_domain known_ans, my_ans;
        u16_t len;
        len = mdns_readname(pkt->pbuf, ans.rd_offset, &known_ans);
        res = mdns_build_host_domain(&my_ans, mdns);
        if (len != MDNS_READNAME_ERROR && res == ERR_OK && mdns_domain_eq(&known_ans, &my_ans)) {
#if LWIP_IPV4
          if (match & REPLY_HOST_PTR_V4) {
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: v4 PTR\n"));
            reply.host_replies &= ~REPLY_HOST_PTR_V4;
          }
#endif
#if LWIP_IPV6
          if (match & REPLY_HOST_PTR_V6) {
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: v6 PTR\n"));
            reply.host_reverse_v6_replies &= ~rev_v6;
            if (reply.host_reverse_v6_replies == 0) {
              reply.host_replies &= ~REPLY_HOST_PTR_V6;
            }
          }
#endif
        }
      } else if (match & REPLY_HOST_A) {
#if LWIP_IPV4
        if (ans.rd_length == sizeof(ip4_addr_t) &&
            pbuf_memcmp(pkt->pbuf, ans.rd_offset, netif_ip4_addr(pkt->netif), ans.rd_length) == 0) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: A\n"));
          reply.host_replies &= ~REPLY_HOST_A;
        }
#endif
      } else if (match & REPLY_HOST_AAAA) {
#if LWIP_IPV6
        if (ans.rd_length == sizeof(ip6_addr_p_t) &&
            /* TODO this clears all AAAA responses if first addr is set as known */
            pbuf_memcmp(pkt->pbuf, ans.rd_offset, netif_ip6_addr(pkt->netif, 0), ans.rd_length) == 0) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: AAAA\n"));
          reply.host_replies &= ~REPLY_HOST_AAAA;
        }
#endif
      }
    }

    for (i = 0; i < MDNS_MAX_SERVICES; ++i) {
      service = mdns->services[i];
      if (!service) {
        continue;
      }
      match = reply.serv_replies[i] & check_service(service, &ans.info);
      if (match && (ans.ttl > (service->dns_ttl / 2))) {
        /* The RR in the known answer matches an RR we are planning to send,
         * and the TTL is less than half gone.
         * If the payload matches we should not send that answer.
         */
        if (ans.info.type == DNS_RRTYPE_PTR) {
          /* Read domain and compare */
          struct mdns_domain known_ans, my_ans;
          u16_t len;
          len = mdns_readname(pkt->pbuf, ans.rd_offset, &known_ans);
          if (len != MDNS_READNAME_ERROR) {
            if (match & REPLY_SERVICE_TYPE_PTR) {
              res = mdns_build_service_domain(&my_ans, service, 0);
              if (res == ERR_OK && mdns_domain_eq(&known_ans, &my_ans)) {
                LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: service type PTR\n"));
                reply.serv_replies[i] &= ~REPLY_SERVICE_TYPE_PTR;
              }
            }
            if (match & REPLY_SERVICE_NAME_PTR) {
              res = mdns_build_service_domain(&my_ans, service, 1);
              if (res == ERR_OK && mdns_domain_eq(&known_ans, &my_ans)) {
                LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: service name PTR\n"));
                reply.serv_replies[i] &= ~REPLY_SERVICE_NAME_PTR;
              }
            }
          }
        } else if (match & REPLY_SERVICE_SRV) {
          /* Read and compare to my SRV record */
          u16_t field16, len, read_pos;
          struct mdns_domain known_ans, my_ans;
          read_pos = ans.rd_offset;
          do {
            /* Check priority field */
            len = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), read_pos);
            if (len != sizeof(field16) || lwip_ntohs(field16) != SRV_PRIORITY) {
              break;
            }
            read_pos += len;
            /* Check weight field */
            len = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), read_pos);
            if (len != sizeof(field16) || lwip_ntohs(field16) != SRV_WEIGHT) {
              break;
            }
            read_pos += len;
            /* Check port field */
            len = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), read_pos);
            if (len != sizeof(field16) || lwip_ntohs(field16) != service->port) {
              break;
            }
            read_pos += len;
            /* Check host field */
            len = mdns_readname(pkt->pbuf, read_pos, &known_ans);
            mdns_build_host_domain(&my_ans, mdns);
            if (len == MDNS_READNAME_ERROR || !mdns_domain_eq(&known_ans, &my_ans)) {
              break;
            }
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: SRV\n"));
            reply.serv_replies[i] &= ~REPLY_SERVICE_SRV;
          } while (0);
        } else if (match & REPLY_SERVICE_TXT) {
          mdns_prepare_txtdata(service);
          if (service->txtdata.length == ans.rd_length &&
              pbuf_memcmp(pkt->pbuf, ans.rd_offset, service->txtdata.name, ans.rd_length) == 0) {
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: TXT\n"));
            reply.serv_replies[i] &= ~REPLY_SERVICE_TXT;
          }
        }
      }
    }
  }

  mdns_send_outpacket(&reply);

cleanup:
  if (reply.pbuf) {
    /* This should only happen if we fail to alloc/write question for legacy query */
    pbuf_free(reply.pbuf);
    reply.pbuf = NULL;
  }
}

/**
 * Handle response MDNS packet
 * Only prints debug for now. Will need more code to do conflict resolution.
 */
static void
mdns_handle_response(struct mdns_packet *pkt)
{
  /* Ignore all questions */
  while (pkt->questions_left) {
    struct mdns_question q;
    err_t res;

    res = mdns_read_question(pkt, &q);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse question, skipping response packet\n"));
      return;
    }
  }

  while (pkt->answers_left) {
    struct mdns_answer ans;
    err_t res;

    res = mdns_read_answer(pkt, &ans);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping response packet\n"));
      return;
    }

    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Answer for domain "));
    mdns_domain_debug_print(&ans.info.domain);
    LWIP_DEBUGF(MDNS_DEBUG, (" type %d class %d\n", ans.info.type, ans.info.klass));
  }
}

/**
 * Receive input function for MDNS packets.
 * Handles both IPv4 and IPv6 UDP pcbs.
 */
static void
mdns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  struct dns_hdr hdr;
  struct mdns_packet packet;
  struct netif *recv_netif = ip_current_input_netif();
  u16_t offset = 0;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);

  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Received IPv%d MDNS packet, len %d\n", IP_IS_V6(addr) ? 6 : 4, p->tot_len));

  if (NETIF_TO_HOST(recv_netif) == NULL) {
    /* From netif not configured for MDNS */
    goto dealloc;
  }

  if (pbuf_copy_partial(p, &hdr, SIZEOF_DNS_HDR, offset) < SIZEOF_DNS_HDR) {
    /* Too small */
    goto dealloc;
  }
  offset += SIZEOF_DNS_HDR;

  if (DNS_HDR_GET_OPCODE(&hdr)) {
    /* Ignore non-standard queries in multicast packets (RFC 6762, section 18.3) */
    goto dealloc;
  }

  memset(&packet, 0, sizeof(packet));
  SMEMCPY(&packet.source_addr, addr, sizeof(packet.source_addr));
  packet.source_port = port;
  packet.netif = recv_netif;
  packet.pbuf = p;
  packet.parse_offset = offset;
  packet.tx_id = lwip_ntohs(hdr.id);
  packet.questions = packet.questions_left = lwip_ntohs(hdr.numquestions);
  packet.answers = packet.answers_left = lwip_ntohs(hdr.numanswers) + lwip_ntohs(hdr.numauthrr) + lwip_ntohs(hdr.numextrarr);

#if LWIP_IPV6
  if (IP_IS_V6(ip_current_dest_addr())) {
    if (!ip_addr_cmp(ip_current_dest_addr(), &v6group)) {
      packet.recv_unicast = 1;
    }
  }
#endif
#if LWIP_IPV4
  if (!IP_IS_V6(ip_current_dest_addr())) {
    if (!ip_addr_cmp(ip_current_dest_addr(), &v4group)) {
      packet.recv_unicast = 1;
    }
  }
#endif

  if (hdr.flags1 & DNS_FLAG1_RESPONSE) {
    mdns_handle_response(&packet);
  } else {
    mdns_handle_question(&packet);
  }

dealloc:
  pbuf_free(p);
}

/**
 * @ingroup mdns
 * Announce IP settings have changed on netif.
 * Call this in your callback registered by netif_set_status_callback().
 * No need to call this function when LWIP_NETIF_EXT_STATUS_CALLBACK==1,
 * this handled automatically for you.
 * @param netif The network interface where settings have changed.
 */
void
mdns_resp_netif_settings_changed(struct netif *netif)
{
  LWIP_ERROR("mdns_resp_netif_ip_changed: netif != NULL", (netif != NULL), return);

  if (NETIF_TO_HOST(netif) == NULL) {
    return;
  }

  /* Announce on IPv6 and IPv4 */
#if LWIP_IPV6
  mdns_announce(netif, IP6_ADDR_ANY);
#endif
#if LWIP_IPV4
  mdns_announce(netif, IP4_ADDR_ANY);
#endif
}

#if LWIP_NETIF_EXT_STATUS_CALLBACK
static void
mdns_netif_ext_status_callback(struct netif *netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t *args)
{
  LWIP_UNUSED_ARG(args);

  /* MDNS enabled on netif? */
  if (NETIF_TO_HOST(netif) == NULL) {
    return;
  }

  switch (reason) {
    case LWIP_NSC_STATUS_CHANGED:
      if (args->status_changed.state != 0) {
        mdns_resp_netif_settings_changed(netif);
      }
      /* TODO: send goodbye message */
      break;
    case LWIP_NSC_LINK_CHANGED:
      if (args->link_changed.state != 0) {
        mdns_resp_netif_settings_changed(netif);
      }
      break;
    case LWIP_NSC_IPV4_ADDRESS_CHANGED:  /* fall through */
    case LWIP_NSC_IPV4_GATEWAY_CHANGED:  /* fall through */
    case LWIP_NSC_IPV4_NETMASK_CHANGED:  /* fall through */
    case LWIP_NSC_IPV4_SETTINGS_CHANGED: /* fall through */
    case LWIP_NSC_IPV6_SET:              /* fall through */
    case LWIP_NSC_IPV6_ADDR_STATE_CHANGED:
      mdns_resp_netif_settings_changed(netif);
      break;
    default:
      break;
  }
}
#endif

/**
 * @ingroup mdns
 * Activate MDNS responder for a network interface and send announce packets.
 * @param netif The network interface to activate.
 * @param hostname Name to use. Queries for &lt;hostname&gt;.local will be answered
 *                 with the IP addresses of the netif. The hostname will be copied, the
 *                 given pointer can be on the stack.
 * @param dns_ttl Validity time in seconds to send out for IP address data in DNS replies
 * @return ERR_OK if netif was added, an err_t otherwise
 */
err_t
mdns_resp_add_netif(struct netif *netif, const char *hostname, u32_t dns_ttl)
{
  err_t res;
  struct mdns_host *mdns;

  LWIP_ERROR("mdns_resp_add_netif: netif != NULL", (netif != NULL), return ERR_VAL);
  LWIP_ERROR("mdns_resp_add_netif: Hostname too long", (strlen(hostname) <= MDNS_LABEL_MAXLEN), return ERR_VAL);

  LWIP_ASSERT("mdns_resp_add_netif: Double add", NETIF_TO_HOST(netif) == NULL);
  mdns = (struct mdns_host *) mem_calloc(1, sizeof(struct mdns_host));
  LWIP_ERROR("mdns_resp_add_netif: Alloc failed", (mdns != NULL), return ERR_MEM);

  netif_set_client_data(netif, mdns_netif_client_id, mdns);

  MEMCPY(&mdns->name, hostname, LWIP_MIN(MDNS_LABEL_MAXLEN, strlen(hostname)));
  mdns->dns_ttl = dns_ttl;

  /* Join multicast groups */
#if LWIP_IPV4
  res = igmp_joingroup_netif(netif, ip_2_ip4(&v4group));
  if (res != ERR_OK) {
    goto cleanup;
  }
#endif
#if LWIP_IPV6
  res = mld6_joingroup_netif(netif, ip_2_ip6(&v6group));
  if (res != ERR_OK) {
    goto cleanup;
  }
#endif

  mdns_resp_netif_settings_changed(netif);
  return ERR_OK;

cleanup:
  mem_free(mdns);
  netif_set_client_data(netif, mdns_netif_client_id, NULL);
  return res;
}

/**
 * @ingroup mdns
 * Stop responding to MDNS queries on this interface, leave multicast groups,
 * and free the helper structure and any of its services.
 * @param netif The network interface to remove.
 * @return ERR_OK if netif was removed, an err_t otherwise
 */
err_t
mdns_resp_remove_netif(struct netif *netif)
{
  int i;
  struct mdns_host *mdns;

  LWIP_ASSERT("mdns_resp_remove_netif: Null pointer", netif);
  mdns = NETIF_TO_HOST(netif);
  LWIP_ERROR("mdns_resp_remove_netif: Not an active netif", (mdns != NULL), return ERR_VAL);

  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    struct mdns_service *service = mdns->services[i];
    if (service) {
      mem_free(service);
    }
  }

  /* Leave multicast groups */
#if LWIP_IPV4
  igmp_leavegroup_netif(netif, ip_2_ip4(&v4group));
#endif
#if LWIP_IPV6
  mld6_leavegroup_netif(netif, ip_2_ip6(&v6group));
#endif

  mem_free(mdns);
  netif_set_client_data(netif, mdns_netif_client_id, NULL);
  return ERR_OK;
}

/**
 * @ingroup mdns
 * Add a service to the selected network interface.
 * @param netif The network interface to publish this service on
 * @param name The name of the service
 * @param service The service type, like "_http"
 * @param proto The service protocol, DNSSD_PROTO_TCP for TCP ("_tcp") and DNSSD_PROTO_UDP
 *              for others ("_udp")
 * @param port The port the service listens to
 * @param dns_ttl Validity time in seconds to send out for service data in DNS replies
 * @param txt_fn Callback to get TXT data. Will be called each time a TXT reply is created to
 *               allow dynamic replies.
 * @param txt_data Userdata pointer for txt_fn
 * @return service_id if the service was added to the netif, an err_t otherwise
 */
s8_t
mdns_resp_add_service(struct netif *netif, const char *name, const char *service, enum mdns_sd_proto proto, u16_t port, u32_t dns_ttl, service_get_txt_fn_t txt_fn, void *txt_data)
{
  s8_t i;
  s8_t slot = -1;
  struct mdns_service *srv;
  struct mdns_host *mdns;

  LWIP_ASSERT("mdns_resp_add_service: netif != NULL", netif);
  mdns = NETIF_TO_HOST(netif);
  LWIP_ERROR("mdns_resp_add_service: Not an mdns netif", (mdns != NULL), return ERR_VAL);

  LWIP_ERROR("mdns_resp_add_service: Name too long", (strlen(name) <= MDNS_LABEL_MAXLEN), return ERR_VAL);
  LWIP_ERROR("mdns_resp_add_service: Service too long", (strlen(service) <= MDNS_LABEL_MAXLEN), return ERR_VAL);
  LWIP_ERROR("mdns_resp_add_service: Bad proto (need TCP or UDP)", (proto == DNSSD_PROTO_TCP || proto == DNSSD_PROTO_UDP), return ERR_VAL);

  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    if (mdns->services[i] == NULL) {
      slot = i;
      break;
    }
  }
  LWIP_ERROR("mdns_resp_add_service: Service list full (increase MDNS_MAX_SERVICES)", (slot >= 0), return ERR_MEM);

  srv = (struct mdns_service *)mem_calloc(1, sizeof(struct mdns_service));
  LWIP_ERROR("mdns_resp_add_service: Alloc failed", (srv != NULL), return ERR_MEM);

  MEMCPY(&srv->name, name, LWIP_MIN(MDNS_LABEL_MAXLEN, strlen(name)));
  MEMCPY(&srv->service, service, LWIP_MIN(MDNS_LABEL_MAXLEN, strlen(service)));
  srv->txt_fn = txt_fn;
  srv->txt_userdata = txt_data;
  srv->proto = (u16_t)proto;
  srv->port = port;
  srv->dns_ttl = dns_ttl;

  mdns->services[slot] = srv;

  /* Announce on IPv6 and IPv4 */
#if LWIP_IPV6
  mdns_announce(netif, IP6_ADDR_ANY);
#endif
#if LWIP_IPV4
  mdns_announce(netif, IP4_ADDR_ANY);
#endif
  return slot;
}

/**
 * @ingroup mdns
 * Delete a service on the selected network interface.
 * @param netif The network interface on which service should be removed
 * @param slot The service slot number returned by mdns_resp_add_service
 * @return ERR_OK if the service was removed from the netif, an err_t otherwise
 */
err_t
mdns_resp_del_service(struct netif *netif, s8_t slot)
{
  struct mdns_host *mdns;
  struct mdns_service *srv;
  LWIP_ASSERT("mdns_resp_del_service: netif != NULL", netif);
  mdns = NETIF_TO_HOST(netif);
  LWIP_ERROR("mdns_resp_del_service: Not an mdns netif", (mdns != NULL), return ERR_VAL);
  LWIP_ERROR("mdns_resp_del_service: Invalid Service ID", (slot >= 0) && (slot < MDNS_MAX_SERVICES), return ERR_VAL);
  LWIP_ERROR("mdns_resp_del_service: Invalid Service ID", (mdns->services[slot] != NULL), return ERR_VAL);

  srv = mdns->services[slot];
  mdns->services[slot] = NULL;
  mem_free(srv);
  return ERR_OK;
}

/**
 * @ingroup mdns
 * Call this function from inside the service_get_txt_fn_t callback to add text data.
 * Buffer for TXT data is 256 bytes, and each field is prefixed with a length byte.
 * @param service The service provided to the get_txt callback
 * @param txt String to add to the TXT field.
 * @param txt_len Length of string
 * @return ERR_OK if the string was added to the reply, an err_t otherwise
 */
err_t
mdns_resp_add_service_txtitem(struct mdns_service *service, const char *txt, u8_t txt_len)
{
  LWIP_ASSERT("mdns_resp_add_service_txtitem: service != NULL", service);

  /* Use a mdns_domain struct to store txt chunks since it is the same encoding */
  return mdns_domain_add_label(&service->txtdata, txt, txt_len);
}

/**
 * @ingroup mdns
 * Initiate MDNS responder. Will open UDP sockets on port 5353
 */
void
mdns_resp_init(void)
{
  err_t res;

  mdns_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  LWIP_ASSERT("Failed to allocate pcb", mdns_pcb != NULL);
#if LWIP_MULTICAST_TX_OPTIONS
  udp_set_multicast_ttl(mdns_pcb, MDNS_TTL);
#else
  mdns_pcb->ttl = MDNS_TTL;
#endif
  res = udp_bind(mdns_pcb, IP_ANY_TYPE, LWIP_IANA_PORT_MDNS);
  LWIP_UNUSED_ARG(res); /* in case of LWIP_NOASSERT */
  LWIP_ASSERT("Failed to bind pcb", res == ERR_OK);
  udp_recv(mdns_pcb, mdns_recv, NULL);

  mdns_netif_client_id = netif_alloc_client_data_id();

  /* register for netif events when started on first netif */
  netif_add_ext_callback(&netif_callback, mdns_netif_ext_status_callback);
}

#endif /* LWIP_MDNS_RESPONDER */
