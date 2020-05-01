/**
 * @file
 * DNS - host name to IP address resolver.
 *
 * @defgroup dns DNS
 * @ingroup callbackstyle_api
 *
 * Implements a DNS host name to IP address resolver.
 *
 * The lwIP DNS resolver functions are used to lookup a host name and
 * map it to a numerical IP address. It maintains a list of resolved
 * hostnames that can be queried with the dns_lookup() function.
 * New hostnames can be resolved using the dns_query() function.
 *
 * The lwIP version of the resolver also adds a non-blocking version of
 * gethostbyname() that will work with a raw API application. This function
 * checks for an IP address string first and converts it if it is valid.
 * gethostbyname() then does a dns_lookup() to see if the name is
 * already in the table. If so, the IP is returned. If not, a query is
 * issued and the function returns with a ERR_INPROGRESS status. The app
 * using the dns client must then go into a waiting state.
 *
 * Once a hostname has been resolved (or found to be non-existent),
 * the resolver code calls a specified callback function (which
 * must be implemented by the module that uses the resolver).
 *
 * Multicast DNS queries are supported for names ending on ".local".
 * However, only "One-Shot Multicast DNS Queries" are supported (RFC 6762
 * chapter 5.1), this is not a fully compliant implementation of continuous
 * mDNS querying!
 *
 * All functions must be called from TCPIP thread.
 *
 * @see @ref netconn_common for thread-safe access.
 */

/*
 * Port to lwIP from uIP
 * by Jim Pettinato April 2007
 *
 * security fixes and more by Simon Goldschmidt
 *
 * uIP version Copyright (c) 2002-2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-----------------------------------------------------------------------------
 * RFC 1035 - Domain names - implementation and specification
 * RFC 2181 - Clarifications to the DNS Specification
 *----------------------------------------------------------------------------*/

/** @todo: define good default values (rfc compliance) */
/** @todo: improve answer parsing, more checkings... */
/** @todo: check RFC1035 - 7.3. Processing responses */
/** @todo: one-shot mDNS: dual-stack fallback to another IP version */

/*-----------------------------------------------------------------------------
 * Includes
 *----------------------------------------------------------------------------*/

#include "lwip/opt.h"

#if LWIP_DNS /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/udp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/dns.h"
#include "lwip/prot/dns.h"

#include <string.h>

/** Random generator function to create random TXIDs and source ports for queries */
#ifndef DNS_RAND_TXID
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_XID) != 0)
#define DNS_RAND_TXID LWIP_RAND
#else
static u16_t dns_txid;
#define DNS_RAND_TXID() (++dns_txid)
#endif
#endif

/** Limits the source port to be >= 1024 by default */
#ifndef DNS_PORT_ALLOWED
#define DNS_PORT_ALLOWED(port) ((port) >= 1024)
#endif

/** DNS maximum number of retries when asking for a name, before "timeout". */
#ifndef DNS_MAX_RETRIES
#define DNS_MAX_RETRIES           4
#endif

/** DNS resource record max. TTL (one week as default) */
#ifndef DNS_MAX_TTL
#define DNS_MAX_TTL               604800
#elif DNS_MAX_TTL > 0x7FFFFFFF
#error DNS_MAX_TTL must be a positive 32-bit value
#endif

#if DNS_TABLE_SIZE > 255
#error DNS_TABLE_SIZE must fit into an u8_t
#endif
#if DNS_MAX_SERVERS > 255
#error DNS_MAX_SERVERS must fit into an u8_t
#endif

/* The number of parallel requests (i.e. calls to dns_gethostbyname
 * that cannot be answered from the DNS table.
 * This is set to the table size by default.
 */
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING) != 0)
#ifndef DNS_MAX_REQUESTS
#define DNS_MAX_REQUESTS          DNS_TABLE_SIZE
#else
#if DNS_MAX_REQUESTS > 255
#error DNS_MAX_REQUESTS must fit into an u8_t
#endif
#endif
#else
/* In this configuration, both arrays have to have the same size and are used
 * like one entry (used/free) */
#define DNS_MAX_REQUESTS          DNS_TABLE_SIZE
#endif

/* The number of UDP source ports used in parallel */
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) != 0)
#ifndef DNS_MAX_SOURCE_PORTS
#define DNS_MAX_SOURCE_PORTS      DNS_MAX_REQUESTS
#else
#if DNS_MAX_SOURCE_PORTS > 255
#error DNS_MAX_SOURCE_PORTS must fit into an u8_t
#endif
#endif
#else
#ifdef DNS_MAX_SOURCE_PORTS
#undef DNS_MAX_SOURCE_PORTS
#endif
#define DNS_MAX_SOURCE_PORTS      1
#endif

#if LWIP_IPV4 && LWIP_IPV6
#define LWIP_DNS_ADDRTYPE_IS_IPV6(t) (((t) == LWIP_DNS_ADDRTYPE_IPV6_IPV4) || ((t) == LWIP_DNS_ADDRTYPE_IPV6))
#define LWIP_DNS_ADDRTYPE_MATCH_IP(t, ip) (IP_IS_V6_VAL(ip) ? LWIP_DNS_ADDRTYPE_IS_IPV6(t) : (!LWIP_DNS_ADDRTYPE_IS_IPV6(t)))
#define LWIP_DNS_ADDRTYPE_ARG(x) , x
#define LWIP_DNS_ADDRTYPE_ARG_OR_ZERO(x) x
#define LWIP_DNS_SET_ADDRTYPE(x, y) do { x = y; } while(0)
#else
#if LWIP_IPV6
#define LWIP_DNS_ADDRTYPE_IS_IPV6(t) 1
#else
#define LWIP_DNS_ADDRTYPE_IS_IPV6(t) 0
#endif
#define LWIP_DNS_ADDRTYPE_MATCH_IP(t, ip) 1
#define LWIP_DNS_ADDRTYPE_ARG(x)
#define LWIP_DNS_ADDRTYPE_ARG_OR_ZERO(x) 0
#define LWIP_DNS_SET_ADDRTYPE(x, y)
#endif /* LWIP_IPV4 && LWIP_IPV6 */

#if LWIP_DNS_SUPPORT_MDNS_QUERIES
#define LWIP_DNS_ISMDNS_ARG(x) , x
#else
#define LWIP_DNS_ISMDNS_ARG(x)
#endif

/** DNS query message structure.
    No packing needed: only used locally on the stack. */
struct dns_query {
  /* DNS query record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
  u16_t type;
  u16_t cls;
};
#define SIZEOF_DNS_QUERY 4

/** DNS answer message structure.
    No packing needed: only used locally on the stack. */
struct dns_answer {
  /* DNS answer record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
  u16_t type;
  u16_t cls;
  u32_t ttl;
  u16_t len;
};
#define SIZEOF_DNS_ANSWER 10
/* maximum allowed size for the struct due to non-packed */
#define SIZEOF_DNS_ANSWER_ASSERT 12

/* DNS table entry states */
typedef enum {
  DNS_STATE_UNUSED           = 0,
  DNS_STATE_NEW              = 1,
  DNS_STATE_ASKING           = 2,
  DNS_STATE_DONE             = 3
} dns_state_enum_t;

/** DNS table entry */
struct dns_table_entry {
  u32_t ttl;
  ip_addr_t ipaddr;
  u16_t txid;
  u8_t  state;
  u8_t  server_idx;
  u8_t  tmr;
  u8_t  retries;
  u8_t  seqno;
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) != 0)
  u8_t pcb_idx;
#endif
  char name[DNS_MAX_NAME_LENGTH];
#if LWIP_IPV4 && LWIP_IPV6
  u8_t reqaddrtype;
#endif /* LWIP_IPV4 && LWIP_IPV6 */
#if LWIP_DNS_SUPPORT_MDNS_QUERIES
  u8_t is_mdns;
#endif
};

/** DNS request table entry: used when dns_gehostbyname cannot answer the
 * request from the DNS table */
struct dns_req_entry {
  /* pointer to callback on DNS query done */
  dns_found_callback found;
  /* argument passed to the callback function */
  void *arg;
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING) != 0)
  u8_t dns_table_idx;
#endif
#if LWIP_IPV4 && LWIP_IPV6
  u8_t reqaddrtype;
#endif /* LWIP_IPV4 && LWIP_IPV6 */
};

#if DNS_LOCAL_HOSTLIST

#if DNS_LOCAL_HOSTLIST_IS_DYNAMIC
/** Local host-list. For hostnames in this list, no
 *  external name resolution is performed */
static struct local_hostlist_entry *local_hostlist_dynamic;
#else /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */

/** Defining this allows the local_hostlist_static to be placed in a different
 * linker section (e.g. FLASH) */
#ifndef DNS_LOCAL_HOSTLIST_STORAGE_PRE
#define DNS_LOCAL_HOSTLIST_STORAGE_PRE static
#endif /* DNS_LOCAL_HOSTLIST_STORAGE_PRE */
/** Defining this allows the local_hostlist_static to be placed in a different
 * linker section (e.g. FLASH) */
#ifndef DNS_LOCAL_HOSTLIST_STORAGE_POST
#define DNS_LOCAL_HOSTLIST_STORAGE_POST
#endif /* DNS_LOCAL_HOSTLIST_STORAGE_POST */
DNS_LOCAL_HOSTLIST_STORAGE_PRE struct local_hostlist_entry local_hostlist_static[]
  DNS_LOCAL_HOSTLIST_STORAGE_POST = DNS_LOCAL_HOSTLIST_INIT;

#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */

static void dns_init_local(void);
static err_t dns_lookup_local(const char *hostname, ip_addr_t *addr LWIP_DNS_ADDRTYPE_ARG(u8_t dns_addrtype));
#endif /* DNS_LOCAL_HOSTLIST */


/* forward declarations */
static void dns_recv(void *s, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static void dns_check_entries(void);
static void dns_call_found(u8_t idx, ip_addr_t *addr);

/*-----------------------------------------------------------------------------
 * Globals
 *----------------------------------------------------------------------------*/

/* DNS variables */
static struct udp_pcb        *dns_pcbs[DNS_MAX_SOURCE_PORTS];
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) != 0)
static u8_t                   dns_last_pcb_idx;
#endif
static u8_t                   dns_seqno;
static struct dns_table_entry dns_table[DNS_TABLE_SIZE];
static struct dns_req_entry   dns_requests[DNS_MAX_REQUESTS];
static ip_addr_t              dns_servers[DNS_MAX_SERVERS];

#if LWIP_IPV4
const ip_addr_t dns_mquery_v4group = DNS_MQUERY_IPV4_GROUP_INIT;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
const ip_addr_t dns_mquery_v6group = DNS_MQUERY_IPV6_GROUP_INIT;
#endif /* LWIP_IPV6 */

/**
 * Initialize the resolver: set up the UDP pcb and configure the default server
 * (if DNS_SERVER_ADDRESS is set).
 */
void
dns_init(void)
{
#ifdef DNS_SERVER_ADDRESS
  /* initialize default DNS server address */
  ip_addr_t dnsserver;
  DNS_SERVER_ADDRESS(&dnsserver);
  dns_setserver(0, &dnsserver);
#endif /* DNS_SERVER_ADDRESS */

  LWIP_ASSERT("sanity check SIZEOF_DNS_QUERY",
              sizeof(struct dns_query) == SIZEOF_DNS_QUERY);
  LWIP_ASSERT("sanity check SIZEOF_DNS_ANSWER",
              sizeof(struct dns_answer) <= SIZEOF_DNS_ANSWER_ASSERT);

  LWIP_DEBUGF(DNS_DEBUG, ("dns_init: initializing\n"));

  /* if dns client not yet initialized... */
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) == 0)
  if (dns_pcbs[0] == NULL) {
    dns_pcbs[0] = udp_new_ip_type(IPADDR_TYPE_ANY);
    LWIP_ASSERT("dns_pcbs[0] != NULL", dns_pcbs[0] != NULL);

    /* initialize DNS table not needed (initialized to zero since it is a
     * global variable) */
    LWIP_ASSERT("For implicit initialization to work, DNS_STATE_UNUSED needs to be 0",
                DNS_STATE_UNUSED == 0);

    /* initialize DNS client */
    udp_bind(dns_pcbs[0], IP_ANY_TYPE, 0);
    udp_recv(dns_pcbs[0], dns_recv, NULL);
  }
#endif

#if DNS_LOCAL_HOSTLIST
  dns_init_local();
#endif
}

/**
 * @ingroup dns
 * Initialize one of the DNS servers.
 *
 * @param numdns the index of the DNS server to set must be < DNS_MAX_SERVERS
 * @param dnsserver IP address of the DNS server to set
 */
void
dns_setserver(u8_t numdns, const ip_addr_t *dnsserver)
{
  if (numdns < DNS_MAX_SERVERS) {
    if (dnsserver != NULL) {
      dns_servers[numdns] = (*dnsserver);
    } else {
      dns_servers[numdns] = *IP_ADDR_ANY;
    }
  }
}

/**
 * @ingroup dns
 * Obtain one of the currently configured DNS server.
 *
 * @param numdns the index of the DNS server
 * @return IP address of the indexed DNS server or "ip_addr_any" if the DNS
 *         server has not been configured.
 */
const ip_addr_t *
dns_getserver(u8_t numdns)
{
  if (numdns < DNS_MAX_SERVERS) {
    return &dns_servers[numdns];
  } else {
    return IP_ADDR_ANY;
  }
}

/**
 * The DNS resolver client timer - handle retries and timeouts and should
 * be called every DNS_TMR_INTERVAL milliseconds (every second by default).
 */
void
dns_tmr(void)
{
  LWIP_DEBUGF(DNS_DEBUG, ("dns_tmr: dns_check_entries\n"));
  dns_check_entries();
}

#if DNS_LOCAL_HOSTLIST
static void
dns_init_local(void)
{
#if DNS_LOCAL_HOSTLIST_IS_DYNAMIC && defined(DNS_LOCAL_HOSTLIST_INIT)
  size_t i;
  struct local_hostlist_entry *entry;
  /* Dynamic: copy entries from DNS_LOCAL_HOSTLIST_INIT to list */
  struct local_hostlist_entry local_hostlist_init[] = DNS_LOCAL_HOSTLIST_INIT;
  size_t namelen;
  for (i = 0; i < LWIP_ARRAYSIZE(local_hostlist_init); i++) {
    struct local_hostlist_entry *init_entry = &local_hostlist_init[i];
    LWIP_ASSERT("invalid host name (NULL)", init_entry->name != NULL);
    namelen = strlen(init_entry->name);
    LWIP_ASSERT("namelen <= DNS_LOCAL_HOSTLIST_MAX_NAMELEN", namelen <= DNS_LOCAL_HOSTLIST_MAX_NAMELEN);
    entry = (struct local_hostlist_entry *)memp_malloc(MEMP_LOCALHOSTLIST);
    LWIP_ASSERT("mem-error in dns_init_local", entry != NULL);
    if (entry != NULL) {
      char *entry_name = (char *)entry + sizeof(struct local_hostlist_entry);
      MEMCPY(entry_name, init_entry->name, namelen);
      entry_name[namelen] = 0;
      entry->name = entry_name;
      entry->addr = init_entry->addr;
      entry->next = local_hostlist_dynamic;
      local_hostlist_dynamic = entry;
    }
  }
#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC && defined(DNS_LOCAL_HOSTLIST_INIT) */
}

/**
 * @ingroup dns
 * Iterate the local host-list for a hostname.
 *
 * @param iterator_fn a function that is called for every entry in the local host-list
 * @param iterator_arg 3rd argument passed to iterator_fn
 * @return the number of entries in the local host-list
 */
size_t
dns_local_iterate(dns_found_callback iterator_fn, void *iterator_arg)
{
  size_t i;
#if DNS_LOCAL_HOSTLIST_IS_DYNAMIC
  struct local_hostlist_entry *entry = local_hostlist_dynamic;
  i = 0;
  while (entry != NULL) {
    if (iterator_fn != NULL) {
      iterator_fn(entry->name, &entry->addr, iterator_arg);
    }
    i++;
    entry = entry->next;
  }
#else /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */
  for (i = 0; i < LWIP_ARRAYSIZE(local_hostlist_static); i++) {
    if (iterator_fn != NULL) {
      iterator_fn(local_hostlist_static[i].name, &local_hostlist_static[i].addr, iterator_arg);
    }
  }
#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */
  return i;
}

/**
 * @ingroup dns
 * Scans the local host-list for a hostname.
 *
 * @param hostname Hostname to look for in the local host-list
 * @param addr the first IP address for the hostname in the local host-list or
 *         IPADDR_NONE if not found.
 * @param dns_addrtype - LWIP_DNS_ADDRTYPE_IPV4_IPV6: try to resolve IPv4 (ATTENTION: no fallback here!)
 *                     - LWIP_DNS_ADDRTYPE_IPV6_IPV4: try to resolve IPv6 (ATTENTION: no fallback here!)
 *                     - LWIP_DNS_ADDRTYPE_IPV4: try to resolve IPv4 only
 *                     - LWIP_DNS_ADDRTYPE_IPV6: try to resolve IPv6 only
 * @return ERR_OK if found, ERR_ARG if not found
 */
err_t
dns_local_lookup(const char *hostname, ip_addr_t *addr, u8_t dns_addrtype)
{
  LWIP_UNUSED_ARG(dns_addrtype);
  return dns_lookup_local(hostname, addr LWIP_DNS_ADDRTYPE_ARG(dns_addrtype));
}

/* Internal implementation for dns_local_lookup and dns_lookup */
static err_t
dns_lookup_local(const char *hostname, ip_addr_t *addr LWIP_DNS_ADDRTYPE_ARG(u8_t dns_addrtype))
{
#if DNS_LOCAL_HOSTLIST_IS_DYNAMIC
  struct local_hostlist_entry *entry = local_hostlist_dynamic;
  while (entry != NULL) {
    if ((lwip_stricmp(entry->name, hostname) == 0) &&
        LWIP_DNS_ADDRTYPE_MATCH_IP(dns_addrtype, entry->addr)) {
      if (addr) {
        ip_addr_copy(*addr, entry->addr);
      }
      return ERR_OK;
    }
    entry = entry->next;
  }
#else /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */
  size_t i;
  for (i = 0; i < LWIP_ARRAYSIZE(local_hostlist_static); i++) {
    if ((lwip_stricmp(local_hostlist_static[i].name, hostname) == 0) &&
        LWIP_DNS_ADDRTYPE_MATCH_IP(dns_addrtype, local_hostlist_static[i].addr)) {
      if (addr) {
        ip_addr_copy(*addr, local_hostlist_static[i].addr);
      }
      return ERR_OK;
    }
  }
#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */
  return ERR_ARG;
}

#if DNS_LOCAL_HOSTLIST_IS_DYNAMIC
/**
 * @ingroup dns
 * Remove all entries from the local host-list for a specific hostname
 * and/or IP address
 *
 * @param hostname hostname for which entries shall be removed from the local
 *                 host-list
 * @param addr address for which entries shall be removed from the local host-list
 * @return the number of removed entries
 */
int
dns_local_removehost(const char *hostname, const ip_addr_t *addr)
{
  int removed = 0;
  struct local_hostlist_entry *entry = local_hostlist_dynamic;
  struct local_hostlist_entry *last_entry = NULL;
  while (entry != NULL) {
    if (((hostname == NULL) || !lwip_stricmp(entry->name, hostname)) &&
        ((addr == NULL) || ip_addr_cmp(&entry->addr, addr))) {
      struct local_hostlist_entry *free_entry;
      if (last_entry != NULL) {
        last_entry->next = entry->next;
      } else {
        local_hostlist_dynamic = entry->next;
      }
      free_entry = entry;
      entry = entry->next;
      memp_free(MEMP_LOCALHOSTLIST, free_entry);
      removed++;
    } else {
      last_entry = entry;
      entry = entry->next;
    }
  }
  return removed;
}

/**
 * @ingroup dns
 * Add a hostname/IP address pair to the local host-list.
 * Duplicates are not checked.
 *
 * @param hostname hostname of the new entry
 * @param addr IP address of the new entry
 * @return ERR_OK if succeeded or ERR_MEM on memory error
 */
err_t
dns_local_addhost(const char *hostname, const ip_addr_t *addr)
{
  struct local_hostlist_entry *entry;
  size_t namelen;
  char *entry_name;
  LWIP_ASSERT("invalid host name (NULL)", hostname != NULL);
  namelen = strlen(hostname);
  LWIP_ASSERT("namelen <= DNS_LOCAL_HOSTLIST_MAX_NAMELEN", namelen <= DNS_LOCAL_HOSTLIST_MAX_NAMELEN);
  entry = (struct local_hostlist_entry *)memp_malloc(MEMP_LOCALHOSTLIST);
  if (entry == NULL) {
    return ERR_MEM;
  }
  entry_name = (char *)entry + sizeof(struct local_hostlist_entry);
  MEMCPY(entry_name, hostname, namelen);
  entry_name[namelen] = 0;
  entry->name = entry_name;
  ip_addr_copy(entry->addr, *addr);
  entry->next = local_hostlist_dynamic;
  local_hostlist_dynamic = entry;
  return ERR_OK;
}
#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC*/
#endif /* DNS_LOCAL_HOSTLIST */

/**
 * @ingroup dns
 * Look up a hostname in the array of known hostnames.
 *
 * @note This function only looks in the internal array of known
 * hostnames, it does not send out a query for the hostname if none
 * was found. The function dns_enqueue() can be used to send a query
 * for a hostname.
 *
 * @param name the hostname to look up
 * @param addr the hostname's IP address, as u32_t (instead of ip_addr_t to
 *         better check for failure: != IPADDR_NONE) or IPADDR_NONE if the hostname
 *         was not found in the cached dns_table.
 * @return ERR_OK if found, ERR_ARG if not found
 */
static err_t
dns_lookup(const char *name, ip_addr_t *addr LWIP_DNS_ADDRTYPE_ARG(u8_t dns_addrtype))
{
  u8_t i;
#if DNS_LOCAL_HOSTLIST
  if (dns_lookup_local(name, addr LWIP_DNS_ADDRTYPE_ARG(dns_addrtype)) == ERR_OK) {
    return ERR_OK;
  }
#endif /* DNS_LOCAL_HOSTLIST */
#ifdef DNS_LOOKUP_LOCAL_EXTERN
  if (DNS_LOOKUP_LOCAL_EXTERN(name, addr, LWIP_DNS_ADDRTYPE_ARG_OR_ZERO(dns_addrtype)) == ERR_OK) {
    return ERR_OK;
  }
#endif /* DNS_LOOKUP_LOCAL_EXTERN */

  /* Walk through name list, return entry if found. If not, return NULL. */
  for (i = 0; i < DNS_TABLE_SIZE; ++i) {
    if ((dns_table[i].state == DNS_STATE_DONE) &&
        (lwip_strnicmp(name, dns_table[i].name, sizeof(dns_table[i].name)) == 0) &&
        LWIP_DNS_ADDRTYPE_MATCH_IP(dns_addrtype, dns_table[i].ipaddr)) {
      LWIP_DEBUGF(DNS_DEBUG, ("dns_lookup: \"%s\": found = ", name));
      ip_addr_debug_print_val(DNS_DEBUG, dns_table[i].ipaddr);
      LWIP_DEBUGF(DNS_DEBUG, ("\n"));
      if (addr) {
        ip_addr_copy(*addr, dns_table[i].ipaddr);
      }
      return ERR_OK;
    }
  }

  return ERR_ARG;
}

/**
 * Compare the "dotted" name "query" with the encoded name "response"
 * to make sure an answer from the DNS server matches the current dns_table
 * entry (otherwise, answers might arrive late for hostname not on the list
 * any more).
 *
 * @param query hostname (not encoded) from the dns_table
 * @param p pbuf containing the encoded hostname in the DNS response
 * @param start_offset offset into p where the name starts
 * @return 0xFFFF: names differ, other: names equal -> offset behind name
 */
static u16_t
dns_compare_name(const char *query, struct pbuf *p, u16_t start_offset)
{
  int n;
  u16_t response_offset = start_offset;

  do {
    n = pbuf_try_get_at(p, response_offset++);
    if ((n < 0) || (response_offset == 0)) {
      return 0xFFFF;
    }
    /** @see RFC 1035 - 4.1.4. Message compression */
    if ((n & 0xc0) == 0xc0) {
      /* Compressed name: cannot be equal since we don't send them */
      return 0xFFFF;
    } else {
      /* Not compressed name */
      while (n > 0) {
        int c = pbuf_try_get_at(p, response_offset);
        if (c < 0) {
          return 0xFFFF;
        }
        if ((*query) != (u8_t)c) {
          return 0xFFFF;
        }
        ++response_offset;
        if (response_offset == 0) {
          return 0xFFFF;
        }
        ++query;
        --n;
      }
      ++query;
    }
    n = pbuf_try_get_at(p, response_offset);
    if (n < 0) {
      return 0xFFFF;
    }
  } while (n != 0);

  if (response_offset == 0xFFFF) {
    return 0xFFFF;
  }
  return (u16_t)(response_offset + 1);
}

/**
 * Walk through a compact encoded DNS name and return the end of the name.
 *
 * @param p pbuf containing the name
 * @param query_idx start index into p pointing to encoded DNS name in the DNS server response
 * @return index to end of the name
 */
static u16_t
dns_skip_name(struct pbuf *p, u16_t query_idx)
{
  int n;
  u16_t offset = query_idx;

  do {
    n = pbuf_try_get_at(p, offset++);
    if ((n < 0) || (offset == 0)) {
      return 0xFFFF;
    }
    /** @see RFC 1035 - 4.1.4. Message compression */
    if ((n & 0xc0) == 0xc0) {
      /* Compressed name: since we only want to skip it (not check it), stop here */
      break;
    } else {
      /* Not compressed name */
      if (offset + n >= p->tot_len) {
        return 0xFFFF;
      }
      offset = (u16_t)(offset + n);
    }
    n = pbuf_try_get_at(p, offset);
    if (n < 0) {
      return 0xFFFF;
    }
  } while (n != 0);

  if (offset == 0xFFFF) {
    return 0xFFFF;
  }
  return (u16_t)(offset + 1);
}

/**
 * Send a DNS query packet.
 *
 * @param idx the DNS table entry index for which to send a request
 * @return ERR_OK if packet is sent; an err_t indicating the problem otherwise
 */
static err_t
dns_send(u8_t idx)
{
  err_t err;
  struct dns_hdr hdr;
  struct dns_query qry;
  struct pbuf *p;
  u16_t query_idx, copy_len;
  const char *hostname, *hostname_part;
  u8_t n;
  u8_t pcb_idx;
  struct dns_table_entry *entry = &dns_table[idx];

  LWIP_DEBUGF(DNS_DEBUG, ("dns_send: dns_servers[%"U16_F"] \"%s\": request\n",
                          (u16_t)(entry->server_idx), entry->name));
  LWIP_ASSERT("dns server out of array", entry->server_idx < DNS_MAX_SERVERS);
  if (ip_addr_isany_val(dns_servers[entry->server_idx])
#if LWIP_DNS_SUPPORT_MDNS_QUERIES
      && !entry->is_mdns
#endif
     ) {
    /* DNS server not valid anymore, e.g. PPP netif has been shut down */
    /* call specified callback function if provided */
    dns_call_found(idx, NULL);
    /* flush this entry */
    entry->state = DNS_STATE_UNUSED;
    return ERR_OK;
  }

  /* if here, we have either a new query or a retry on a previous query to process */
  p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)(SIZEOF_DNS_HDR + strlen(entry->name) + 2 +
                                         SIZEOF_DNS_QUERY), PBUF_RAM);
  if (p != NULL) {
    const ip_addr_t *dst;
    u16_t dst_port;
    /* fill dns header */
    memset(&hdr, 0, SIZEOF_DNS_HDR);
    hdr.id = lwip_htons(entry->txid);
    hdr.flags1 = DNS_FLAG1_RD;
    hdr.numquestions = PP_HTONS(1);
    pbuf_take(p, &hdr, SIZEOF_DNS_HDR);
    hostname = entry->name;
    --hostname;

    /* convert hostname into suitable query format. */
    query_idx = SIZEOF_DNS_HDR;
    do {
      ++hostname;
      hostname_part = hostname;
      for (n = 0; *hostname != '.' && *hostname != 0; ++hostname) {
        ++n;
      }
      copy_len = (u16_t)(hostname - hostname_part);
      if (query_idx + n + 1 > 0xFFFF) {
        /* u16_t overflow */
        goto overflow_return;
      }
      pbuf_put_at(p, query_idx, n);
      pbuf_take_at(p, hostname_part, copy_len, (u16_t)(query_idx + 1));
      query_idx = (u16_t)(query_idx + n + 1);
    } while (*hostname != 0);
    pbuf_put_at(p, query_idx, 0);
    query_idx++;

    /* fill dns query */
    if (LWIP_DNS_ADDRTYPE_IS_IPV6(entry->reqaddrtype)) {
      qry.type = PP_HTONS(DNS_RRTYPE_AAAA);
    } else {
      qry.type = PP_HTONS(DNS_RRTYPE_A);
    }
    qry.cls = PP_HTONS(DNS_RRCLASS_IN);
    pbuf_take_at(p, &qry, SIZEOF_DNS_QUERY, query_idx);

#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) != 0)
    pcb_idx = entry->pcb_idx;
#else
    pcb_idx = 0;
#endif
    /* send dns packet */
    LWIP_DEBUGF(DNS_DEBUG, ("sending DNS request ID %d for name \"%s\" to server %d\r\n",
                            entry->txid, entry->name, entry->server_idx));
#if LWIP_DNS_SUPPORT_MDNS_QUERIES
    if (entry->is_mdns) {
      dst_port = DNS_MQUERY_PORT;
#if LWIP_IPV6
      if (LWIP_DNS_ADDRTYPE_IS_IPV6(entry->reqaddrtype)) {
        dst = &dns_mquery_v6group;
      }
#endif
#if LWIP_IPV4 && LWIP_IPV6
      else
#endif
#if LWIP_IPV4
      {
        dst = &dns_mquery_v4group;
      }
#endif
    } else
#endif /* LWIP_DNS_SUPPORT_MDNS_QUERIES */
    {
      dst_port = DNS_SERVER_PORT;
      dst = &dns_servers[entry->server_idx];
    }
    err = udp_sendto(dns_pcbs[pcb_idx], p, dst, dst_port);

    /* free pbuf */
    pbuf_free(p);
  } else {
    err = ERR_MEM;
  }

  return err;
overflow_return:
  pbuf_free(p);
  return ERR_VAL;
}

#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) != 0)
static struct udp_pcb *
dns_alloc_random_port(void)
{
  err_t err;
  struct udp_pcb *pcb;

  pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (pcb == NULL) {
    /* out of memory, have to reuse an existing pcb */
    return NULL;
  }
  do {
    u16_t port = (u16_t)DNS_RAND_TXID();
    if (DNS_PORT_ALLOWED(port)) {
      err = udp_bind(pcb, IP_ANY_TYPE, port);
    } else {
      /* this port is not allowed, try again */
      err = ERR_USE;
    }
  } while (err == ERR_USE);
  if (err != ERR_OK) {
    udp_remove(pcb);
    return NULL;
  }
  udp_recv(pcb, dns_recv, NULL);
  return pcb;
}

/**
 * dns_alloc_pcb() - allocates a new pcb (or reuses an existing one) to be used
 * for sending a request
 *
 * @return an index into dns_pcbs
 */
static u8_t
dns_alloc_pcb(void)
{
  u8_t i;
  u8_t idx;

  for (i = 0; i < DNS_MAX_SOURCE_PORTS; i++) {
    if (dns_pcbs[i] == NULL) {
      break;
    }
  }
  if (i < DNS_MAX_SOURCE_PORTS) {
    dns_pcbs[i] = dns_alloc_random_port();
    if (dns_pcbs[i] != NULL) {
      /* succeeded */
      dns_last_pcb_idx = i;
      return i;
    }
  }
  /* if we come here, creating a new UDP pcb failed, so we have to use
     an already existing one (so overflow is no issue) */
  for (i = 0, idx = (u8_t)(dns_last_pcb_idx + 1); i < DNS_MAX_SOURCE_PORTS; i++, idx++) {
    if (idx >= DNS_MAX_SOURCE_PORTS) {
      idx = 0;
    }
    if (dns_pcbs[idx] != NULL) {
      dns_last_pcb_idx = idx;
      return idx;
    }
  }
  return DNS_MAX_SOURCE_PORTS;
}
#endif /* ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) != 0) */

/**
 * dns_call_found() - call the found callback and check if there are duplicate
 * entries for the given hostname. If there are any, their found callback will
 * be called and they will be removed.
 *
 * @param idx dns table index of the entry that is resolved or removed
 * @param addr IP address for the hostname (or NULL on error or memory shortage)
 */
static void
dns_call_found(u8_t idx, ip_addr_t *addr)
{
#if ((LWIP_DNS_SECURE & (LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING | LWIP_DNS_SECURE_RAND_SRC_PORT)) != 0)
  u8_t i;
#endif

#if LWIP_IPV4 && LWIP_IPV6
  if (addr != NULL) {
    /* check that address type matches the request and adapt the table entry */
    if (IP_IS_V6_VAL(*addr)) {
      LWIP_ASSERT("invalid response", LWIP_DNS_ADDRTYPE_IS_IPV6(dns_table[idx].reqaddrtype));
      dns_table[idx].reqaddrtype = LWIP_DNS_ADDRTYPE_IPV6;
    } else {
      LWIP_ASSERT("invalid response", !LWIP_DNS_ADDRTYPE_IS_IPV6(dns_table[idx].reqaddrtype));
      dns_table[idx].reqaddrtype = LWIP_DNS_ADDRTYPE_IPV4;
    }
  }
#endif /* LWIP_IPV4 && LWIP_IPV6 */

#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING) != 0)
  for (i = 0; i < DNS_MAX_REQUESTS; i++) {
    if (dns_requests[i].found && (dns_requests[i].dns_table_idx == idx)) {
      (*dns_requests[i].found)(dns_table[idx].name, addr, dns_requests[i].arg);
      /* flush this entry */
      dns_requests[i].found = NULL;
    }
  }
#else
  if (dns_requests[idx].found) {
    (*dns_requests[idx].found)(dns_table[idx].name, addr, dns_requests[idx].arg);
  }
  dns_requests[idx].found = NULL;
#endif
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) != 0)
  /* close the pcb used unless other request are using it */
  for (i = 0; i < DNS_MAX_REQUESTS; i++) {
    if (i == idx) {
      continue; /* only check other requests */
    }
    if (dns_table[i].state == DNS_STATE_ASKING) {
      if (dns_table[i].pcb_idx == dns_table[idx].pcb_idx) {
        /* another request is still using the same pcb */
        dns_table[idx].pcb_idx = DNS_MAX_SOURCE_PORTS;
        break;
      }
    }
  }
  if (dns_table[idx].pcb_idx < DNS_MAX_SOURCE_PORTS) {
    /* if we come here, the pcb is not used any more and can be removed */
    udp_remove(dns_pcbs[dns_table[idx].pcb_idx]);
    dns_pcbs[dns_table[idx].pcb_idx] = NULL;
    dns_table[idx].pcb_idx = DNS_MAX_SOURCE_PORTS;
  }
#endif
}

/* Create a query transmission ID that is unique for all outstanding queries */
static u16_t
dns_create_txid(void)
{
  u16_t txid;
  u8_t i;

again:
  txid = (u16_t)DNS_RAND_TXID();

  /* check whether the ID is unique */
  for (i = 0; i < DNS_TABLE_SIZE; i++) {
    if ((dns_table[i].state == DNS_STATE_ASKING) &&
        (dns_table[i].txid == txid)) {
      /* ID already used by another pending query */
      goto again;
    }
  }

  return txid;
}

/**
 * dns_check_entry() - see if entry has not yet been queried and, if so, sends out a query.
 * Check an entry in the dns_table:
 * - send out query for new entries
 * - retry old pending entries on timeout (also with different servers)
 * - remove completed entries from the table if their TTL has expired
 *
 * @param i index of the dns_table entry to check
 */
static void
dns_check_entry(u8_t i)
{
  err_t err;
  struct dns_table_entry *entry = &dns_table[i];

  LWIP_ASSERT("array index out of bounds", i < DNS_TABLE_SIZE);

  switch (entry->state) {
    case DNS_STATE_NEW:
      /* initialize new entry */
      entry->txid = dns_create_txid();
      entry->state = DNS_STATE_ASKING;
      entry->server_idx = 0;
      entry->tmr = 1;
      entry->retries = 0;

      /* send DNS packet for this entry */
      err = dns_send(i);
      if (err != ERR_OK) {
        LWIP_DEBUGF(DNS_DEBUG | LWIP_DBG_LEVEL_WARNING,
                    ("dns_send returned error: %s\n", lwip_strerr(err)));
      }
      break;
    case DNS_STATE_ASKING:
      if (--entry->tmr == 0) {
        if (++entry->retries == DNS_MAX_RETRIES) {
          if ((entry->server_idx + 1 < DNS_MAX_SERVERS) && !ip_addr_isany_val(dns_servers[entry->server_idx + 1])
#if LWIP_DNS_SUPPORT_MDNS_QUERIES
              && !entry->is_mdns
#endif /* LWIP_DNS_SUPPORT_MDNS_QUERIES */
             ) {
            /* change of server */
            entry->server_idx++;
            entry->tmr = 1;
            entry->retries = 0;
          } else {
            LWIP_DEBUGF(DNS_DEBUG, ("dns_check_entry: \"%s\": timeout\n", entry->name));
            /* call specified callback function if provided */
            dns_call_found(i, NULL);
            /* flush this entry */
            entry->state = DNS_STATE_UNUSED;
            break;
          }
        } else {
          /* wait longer for the next retry */
          entry->tmr = entry->retries;
        }

        /* send DNS packet for this entry */
        err = dns_send(i);
        if (err != ERR_OK) {
          LWIP_DEBUGF(DNS_DEBUG | LWIP_DBG_LEVEL_WARNING,
                      ("dns_send returned error: %s\n", lwip_strerr(err)));
        }
      }
      break;
    case DNS_STATE_DONE:
      /* if the time to live is nul */
      if ((entry->ttl == 0) || (--entry->ttl == 0)) {
        LWIP_DEBUGF(DNS_DEBUG, ("dns_check_entry: \"%s\": flush\n", entry->name));
        /* flush this entry, there cannot be any related pending entries in this state */
        entry->state = DNS_STATE_UNUSED;
      }
      break;
    case DNS_STATE_UNUSED:
      /* nothing to do */
      break;
    default:
      LWIP_ASSERT("unknown dns_table entry state:", 0);
      break;
  }
}

/**
 * Call dns_check_entry for each entry in dns_table - check all entries.
 */
static void
dns_check_entries(void)
{
  u8_t i;

  for (i = 0; i < DNS_TABLE_SIZE; ++i) {
    dns_check_entry(i);
  }
}

/**
 * Save TTL and call dns_call_found for correct response.
 */
static void
dns_correct_response(u8_t idx, u32_t ttl)
{
  struct dns_table_entry *entry = &dns_table[idx];

  entry->state = DNS_STATE_DONE;

  LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": response = ", entry->name));
  ip_addr_debug_print_val(DNS_DEBUG, entry->ipaddr);
  LWIP_DEBUGF(DNS_DEBUG, ("\n"));

  /* read the answer resource record's TTL, and maximize it if needed */
  entry->ttl = ttl;
  if (entry->ttl > DNS_MAX_TTL) {
    entry->ttl = DNS_MAX_TTL;
  }
  dns_call_found(idx, &entry->ipaddr);

  if (entry->ttl == 0) {
    /* RFC 883, page 29: "Zero values are
       interpreted to mean that the RR can only be used for the
       transaction in progress, and should not be cached."
       -> flush this entry now */
    /* entry reused during callback? */
    if (entry->state == DNS_STATE_DONE) {
      entry->state = DNS_STATE_UNUSED;
    }
  }
}
/**
 * Receive input function for DNS response packets arriving for the dns UDP pcb.
 */
static void
dns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  u8_t i;
  u16_t txid;
  u16_t res_idx;
  struct dns_hdr hdr;
  struct dns_answer ans;
  struct dns_query qry;
  u16_t nquestions, nanswers;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(port);

  /* is the dns message big enough ? */
  if (p->tot_len < (SIZEOF_DNS_HDR + SIZEOF_DNS_QUERY)) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: pbuf too small\n"));
    /* free pbuf and return */
    goto memerr;
  }

  /* copy dns payload inside static buffer for processing */
  if (pbuf_copy_partial(p, &hdr, SIZEOF_DNS_HDR, 0) == SIZEOF_DNS_HDR) {
    /* Match the ID in the DNS header with the name table. */
    txid = lwip_htons(hdr.id);
    for (i = 0; i < DNS_TABLE_SIZE; i++) {
      const struct dns_table_entry *entry = &dns_table[i];
      if ((entry->state == DNS_STATE_ASKING) &&
          (entry->txid == txid)) {

        /* We only care about the question(s) and the answers. The authrr
           and the extrarr are simply discarded. */
        nquestions = lwip_htons(hdr.numquestions);
        nanswers   = lwip_htons(hdr.numanswers);

        /* Check for correct response. */
        if ((hdr.flags1 & DNS_FLAG1_RESPONSE) == 0) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": not a response\n", entry->name));
          goto memerr; /* ignore this packet */
        }
        if (nquestions != 1) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": response not match to query\n", entry->name));
          goto memerr; /* ignore this packet */
        }

#if LWIP_DNS_SUPPORT_MDNS_QUERIES
        if (!entry->is_mdns)
#endif /* LWIP_DNS_SUPPORT_MDNS_QUERIES */
        {
          /* Check whether response comes from the same network address to which the
             question was sent. (RFC 5452) */
          if (!ip_addr_cmp(addr, &dns_servers[entry->server_idx])) {
            goto memerr; /* ignore this packet */
          }
        }

        /* Check if the name in the "question" part match with the name in the entry and
           skip it if equal. */
        res_idx = dns_compare_name(entry->name, p, SIZEOF_DNS_HDR);
        if (res_idx == 0xFFFF) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": response not match to query\n", entry->name));
          goto memerr; /* ignore this packet */
        }

        /* check if "question" part matches the request */
        if (pbuf_copy_partial(p, &qry, SIZEOF_DNS_QUERY, res_idx) != SIZEOF_DNS_QUERY) {
          goto memerr; /* ignore this packet */
        }
        if ((qry.cls != PP_HTONS(DNS_RRCLASS_IN)) ||
            (LWIP_DNS_ADDRTYPE_IS_IPV6(entry->reqaddrtype) && (qry.type != PP_HTONS(DNS_RRTYPE_AAAA))) ||
            (!LWIP_DNS_ADDRTYPE_IS_IPV6(entry->reqaddrtype) && (qry.type != PP_HTONS(DNS_RRTYPE_A)))) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": response not match to query\n", entry->name));
          goto memerr; /* ignore this packet */
        }
        /* skip the rest of the "question" part */
        if (res_idx + SIZEOF_DNS_QUERY > 0xFFFF) {
          goto memerr;
        }
        res_idx = (u16_t)(res_idx + SIZEOF_DNS_QUERY);

        /* Check for error. If so, call callback to inform. */
        if (hdr.flags2 & DNS_FLAG2_ERR_MASK) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": error in flags\n", entry->name));
        } else {
          while ((nanswers > 0) && (res_idx < p->tot_len)) {
            /* skip answer resource record's host name */
            res_idx = dns_skip_name(p, res_idx);
            if (res_idx == 0xFFFF) {
              goto memerr; /* ignore this packet */
            }

            /* Check for IP address type and Internet class. Others are discarded. */
            if (pbuf_copy_partial(p, &ans, SIZEOF_DNS_ANSWER, res_idx) != SIZEOF_DNS_ANSWER) {
              goto memerr; /* ignore this packet */
            }
            if (res_idx + SIZEOF_DNS_ANSWER > 0xFFFF) {
              goto memerr;
            }
            res_idx = (u16_t)(res_idx + SIZEOF_DNS_ANSWER);

            if (ans.cls == PP_HTONS(DNS_RRCLASS_IN)) {
#if LWIP_IPV4
              if ((ans.type == PP_HTONS(DNS_RRTYPE_A)) && (ans.len == PP_HTONS(sizeof(ip4_addr_t)))) {
#if LWIP_IPV4 && LWIP_IPV6
                if (!LWIP_DNS_ADDRTYPE_IS_IPV6(entry->reqaddrtype))
#endif /* LWIP_IPV4 && LWIP_IPV6 */
                {
                  ip4_addr_t ip4addr;
                  /* read the IP address after answer resource record's header */
                  if (pbuf_copy_partial(p, &ip4addr, sizeof(ip4_addr_t), res_idx) != sizeof(ip4_addr_t)) {
                    goto memerr; /* ignore this packet */
                  }
                  ip_addr_copy_from_ip4(dns_table[i].ipaddr, ip4addr);
                  pbuf_free(p);
                  /* handle correct response */
                  dns_correct_response(i, lwip_ntohl(ans.ttl));
                  return;
                }
              }
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
              if ((ans.type == PP_HTONS(DNS_RRTYPE_AAAA)) && (ans.len == PP_HTONS(sizeof(ip6_addr_p_t)))) {
#if LWIP_IPV4 && LWIP_IPV6
                if (LWIP_DNS_ADDRTYPE_IS_IPV6(entry->reqaddrtype))
#endif /* LWIP_IPV4 && LWIP_IPV6 */
                {
                  ip6_addr_p_t ip6addr;
                  /* read the IP address after answer resource record's header */
                  if (pbuf_copy_partial(p, &ip6addr, sizeof(ip6_addr_p_t), res_idx) != sizeof(ip6_addr_p_t)) {
                    goto memerr; /* ignore this packet */
                  }
                  /* @todo: scope ip6addr? Might be required for link-local addresses at least? */
                  ip_addr_copy_from_ip6_packed(dns_table[i].ipaddr, ip6addr);
                  pbuf_free(p);
                  /* handle correct response */
                  dns_correct_response(i, lwip_ntohl(ans.ttl));
                  return;
                }
              }
#endif /* LWIP_IPV6 */
            }
            /* skip this answer */
            if ((int)(res_idx + lwip_htons(ans.len)) > 0xFFFF) {
              goto memerr; /* ignore this packet */
            }
            res_idx = (u16_t)(res_idx + lwip_htons(ans.len));
            --nanswers;
          }
#if LWIP_IPV4 && LWIP_IPV6
          if ((entry->reqaddrtype == LWIP_DNS_ADDRTYPE_IPV4_IPV6) ||
              (entry->reqaddrtype == LWIP_DNS_ADDRTYPE_IPV6_IPV4)) {
            if (entry->reqaddrtype == LWIP_DNS_ADDRTYPE_IPV4_IPV6) {
              /* IPv4 failed, try IPv6 */
              dns_table[i].reqaddrtype = LWIP_DNS_ADDRTYPE_IPV6;
            } else {
              /* IPv6 failed, try IPv4 */
              dns_table[i].reqaddrtype = LWIP_DNS_ADDRTYPE_IPV4;
            }
            pbuf_free(p);
            dns_table[i].state = DNS_STATE_NEW;
            dns_check_entry(i);
            return;
          }
#endif /* LWIP_IPV4 && LWIP_IPV6 */
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": error in response\n", entry->name));
        }
        /* call callback to indicate error, clean up memory and return */
        pbuf_free(p);
        dns_call_found(i, NULL);
        dns_table[i].state = DNS_STATE_UNUSED;
        return;
      }
    }
  }

memerr:
  /* deallocate memory and return */
  pbuf_free(p);
  return;
}

/**
 * Queues a new hostname to resolve and sends out a DNS query for that hostname
 *
 * @param name the hostname that is to be queried
 * @param hostnamelen length of the hostname
 * @param found a callback function to be called on success, failure or timeout
 * @param callback_arg argument to pass to the callback function
 * @return err_t return code.
 */
static err_t
dns_enqueue(const char *name, size_t hostnamelen, dns_found_callback found,
            void *callback_arg LWIP_DNS_ADDRTYPE_ARG(u8_t dns_addrtype) LWIP_DNS_ISMDNS_ARG(u8_t is_mdns))
{
  u8_t i;
  u8_t lseq, lseqi;
  struct dns_table_entry *entry = NULL;
  size_t namelen;
  struct dns_req_entry *req;

#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING) != 0)
  u8_t r;
  /* check for duplicate entries */
  for (i = 0; i < DNS_TABLE_SIZE; i++) {
    if ((dns_table[i].state == DNS_STATE_ASKING) &&
        (lwip_strnicmp(name, dns_table[i].name, sizeof(dns_table[i].name)) == 0)) {
#if LWIP_IPV4 && LWIP_IPV6
      if (dns_table[i].reqaddrtype != dns_addrtype) {
        /* requested address types don't match
           this can lead to 2 concurrent requests, but mixing the address types
           for the same host should not be that common */
        continue;
      }
#endif /* LWIP_IPV4 && LWIP_IPV6 */
      /* this is a duplicate entry, find a free request entry */
      for (r = 0; r < DNS_MAX_REQUESTS; r++) {
        if (dns_requests[r].found == 0) {
          dns_requests[r].found = found;
          dns_requests[r].arg = callback_arg;
          dns_requests[r].dns_table_idx = i;
          LWIP_DNS_SET_ADDRTYPE(dns_requests[r].reqaddrtype, dns_addrtype);
          LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": duplicate request\n", name));
          return ERR_INPROGRESS;
        }
      }
    }
  }
  /* no duplicate entries found */
#endif

  /* search an unused entry, or the oldest one */
  lseq = 0;
  lseqi = DNS_TABLE_SIZE;
  for (i = 0; i < DNS_TABLE_SIZE; ++i) {
    entry = &dns_table[i];
    /* is it an unused entry ? */
    if (entry->state == DNS_STATE_UNUSED) {
      break;
    }
    /* check if this is the oldest completed entry */
    if (entry->state == DNS_STATE_DONE) {
      u8_t age = (u8_t)(dns_seqno - entry->seqno);
      if (age > lseq) {
        lseq = age;
        lseqi = i;
      }
    }
  }

  /* if we don't have found an unused entry, use the oldest completed one */
  if (i == DNS_TABLE_SIZE) {
    if ((lseqi >= DNS_TABLE_SIZE) || (dns_table[lseqi].state != DNS_STATE_DONE)) {
      /* no entry can be used now, table is full */
      LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": DNS entries table is full\n", name));
      return ERR_MEM;
    } else {
      /* use the oldest completed one */
      i = lseqi;
      entry = &dns_table[i];
    }
  }

#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING) != 0)
  /* find a free request entry */
  req = NULL;
  for (r = 0; r < DNS_MAX_REQUESTS; r++) {
    if (dns_requests[r].found == NULL) {
      req = &dns_requests[r];
      break;
    }
  }
  if (req == NULL) {
    /* no request entry can be used now, table is full */
    LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": DNS request entries table is full\n", name));
    return ERR_MEM;
  }
  req->dns_table_idx = i;
#else
  /* in this configuration, the entry index is the same as the request index */
  req = &dns_requests[i];
#endif

  /* use this entry */
  LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": use DNS entry %"U16_F"\n", name, (u16_t)(i)));

  /* fill the entry */
  entry->state = DNS_STATE_NEW;
  entry->seqno = dns_seqno;
  LWIP_DNS_SET_ADDRTYPE(entry->reqaddrtype, dns_addrtype);
  LWIP_DNS_SET_ADDRTYPE(req->reqaddrtype, dns_addrtype);
  req->found = found;
  req->arg   = callback_arg;
  namelen = LWIP_MIN(hostnamelen, DNS_MAX_NAME_LENGTH - 1);
  MEMCPY(entry->name, name, namelen);
  entry->name[namelen] = 0;

#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) != 0)
  entry->pcb_idx = dns_alloc_pcb();
  if (entry->pcb_idx >= DNS_MAX_SOURCE_PORTS) {
    /* failed to get a UDP pcb */
    LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": failed to allocate a pcb\n", name));
    entry->state = DNS_STATE_UNUSED;
    req->found = NULL;
    return ERR_MEM;
  }
  LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": use DNS pcb %"U16_F"\n", name, (u16_t)(entry->pcb_idx)));
#endif

#if LWIP_DNS_SUPPORT_MDNS_QUERIES
  entry->is_mdns = is_mdns;
#endif

  dns_seqno++;

  /* force to send query without waiting timer */
  dns_check_entry(i);

  /* dns query is enqueued */
  return ERR_INPROGRESS;
}

/**
 * @ingroup dns
 * Resolve a hostname (string) into an IP address.
 * NON-BLOCKING callback version for use with raw API!!!
 *
 * Returns immediately with one of err_t return codes:
 * - ERR_OK if hostname is a valid IP address string or the host
 *   name is already in the local names table.
 * - ERR_INPROGRESS enqueue a request to be sent to the DNS server
 *   for resolution if no errors are present.
 * - ERR_ARG: dns client not initialized or invalid hostname
 *
 * @param hostname the hostname that is to be queried
 * @param addr pointer to a ip_addr_t where to store the address if it is already
 *             cached in the dns_table (only valid if ERR_OK is returned!)
 * @param found a callback function to be called on success, failure or timeout (only if
 *              ERR_INPROGRESS is returned!)
 * @param callback_arg argument to pass to the callback function
 * @return a err_t return code.
 */
err_t
dns_gethostbyname(const char *hostname, ip_addr_t *addr, dns_found_callback found,
                  void *callback_arg)
{
  return dns_gethostbyname_addrtype(hostname, addr, found, callback_arg, LWIP_DNS_ADDRTYPE_DEFAULT);
}

/**
 * @ingroup dns
 * Like dns_gethostbyname, but returned address type can be controlled:
 * @param hostname the hostname that is to be queried
 * @param addr pointer to a ip_addr_t where to store the address if it is already
 *             cached in the dns_table (only valid if ERR_OK is returned!)
 * @param found a callback function to be called on success, failure or timeout (only if
 *              ERR_INPROGRESS is returned!)
 * @param callback_arg argument to pass to the callback function
 * @param dns_addrtype - LWIP_DNS_ADDRTYPE_IPV4_IPV6: try to resolve IPv4 first, try IPv6 if IPv4 fails only
 *                     - LWIP_DNS_ADDRTYPE_IPV6_IPV4: try to resolve IPv6 first, try IPv4 if IPv6 fails only
 *                     - LWIP_DNS_ADDRTYPE_IPV4: try to resolve IPv4 only
 *                     - LWIP_DNS_ADDRTYPE_IPV6: try to resolve IPv6 only
 */
err_t
dns_gethostbyname_addrtype(const char *hostname, ip_addr_t *addr, dns_found_callback found,
                           void *callback_arg, u8_t dns_addrtype)
{
  size_t hostnamelen;
#if LWIP_DNS_SUPPORT_MDNS_QUERIES
  u8_t is_mdns;
#endif
  /* not initialized or no valid server yet, or invalid addr pointer
   * or invalid hostname or invalid hostname length */
  if ((addr == NULL) ||
      (!hostname) || (!hostname[0])) {
    return ERR_ARG;
  }
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_RAND_SRC_PORT) == 0)
  if (dns_pcbs[0] == NULL) {
    return ERR_ARG;
  }
#endif
  hostnamelen = strlen(hostname);
  if (hostnamelen >= DNS_MAX_NAME_LENGTH) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_gethostbyname: name too long to resolve"));
    return ERR_ARG;
  }


#if LWIP_HAVE_LOOPIF
  if (strcmp(hostname, "localhost") == 0) {
    ip_addr_set_loopback(LWIP_DNS_ADDRTYPE_IS_IPV6(dns_addrtype), addr);
    return ERR_OK;
  }
#endif /* LWIP_HAVE_LOOPIF */

  /* host name already in octet notation? set ip addr and return ERR_OK */
  if (ipaddr_aton(hostname, addr)) {
#if LWIP_IPV4 && LWIP_IPV6
    if ((IP_IS_V6(addr) && (dns_addrtype != LWIP_DNS_ADDRTYPE_IPV4)) ||
        (IP_IS_V4(addr) && (dns_addrtype != LWIP_DNS_ADDRTYPE_IPV6)))
#endif /* LWIP_IPV4 && LWIP_IPV6 */
    {
      return ERR_OK;
    }
  }
  /* already have this address cached? */
  if (dns_lookup(hostname, addr LWIP_DNS_ADDRTYPE_ARG(dns_addrtype)) == ERR_OK) {
    return ERR_OK;
  }
#if LWIP_IPV4 && LWIP_IPV6
  if ((dns_addrtype == LWIP_DNS_ADDRTYPE_IPV4_IPV6) || (dns_addrtype == LWIP_DNS_ADDRTYPE_IPV6_IPV4)) {
    /* fallback to 2nd IP type and try again to lookup */
    u8_t fallback;
    if (dns_addrtype == LWIP_DNS_ADDRTYPE_IPV4_IPV6) {
      fallback = LWIP_DNS_ADDRTYPE_IPV6;
    } else {
      fallback = LWIP_DNS_ADDRTYPE_IPV4;
    }
    if (dns_lookup(hostname, addr LWIP_DNS_ADDRTYPE_ARG(fallback)) == ERR_OK) {
      return ERR_OK;
    }
  }
#else /* LWIP_IPV4 && LWIP_IPV6 */
  LWIP_UNUSED_ARG(dns_addrtype);
#endif /* LWIP_IPV4 && LWIP_IPV6 */

#if LWIP_DNS_SUPPORT_MDNS_QUERIES
  if (strstr(hostname, ".local") == &hostname[hostnamelen] - 6) {
    is_mdns = 1;
  } else {
    is_mdns = 0;
  }

  if (!is_mdns)
#endif /* LWIP_DNS_SUPPORT_MDNS_QUERIES */
  {
    /* prevent calling found callback if no server is set, return error instead */
    if (ip_addr_isany_val(dns_servers[0])) {
      return ERR_VAL;
    }
  }

  /* queue query with specified callback */
  return dns_enqueue(hostname, hostnamelen, found, callback_arg LWIP_DNS_ADDRTYPE_ARG(dns_addrtype)
                     LWIP_DNS_ISMDNS_ARG(is_mdns));
}

#endif /* LWIP_DNS */
