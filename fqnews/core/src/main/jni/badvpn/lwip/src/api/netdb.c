/**
 * @file
 * API functions for name resolving
 *
 * @defgroup netdbapi NETDB API
 * @ingroup socket
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
 * Author: Simon Goldschmidt
 *
 */

#include "lwip/netdb.h"

#if LWIP_DNS && LWIP_SOCKET

#include "lwip/err.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/ip_addr.h"
#include "lwip/api.h"
#include "lwip/dns.h"

#include <string.h> /* memset */
#include <stdlib.h> /* atoi */

/** helper struct for gethostbyname_r to access the char* buffer */
struct gethostbyname_r_helper {
  ip_addr_t *addr_list[2];
  ip_addr_t addr;
  char *aliases;
};

/** h_errno is exported in netdb.h for access by applications. */
#if LWIP_DNS_API_DECLARE_H_ERRNO
int h_errno;
#endif /* LWIP_DNS_API_DECLARE_H_ERRNO */

/** define "hostent" variables storage: 0 if we use a static (but unprotected)
 * set of variables for lwip_gethostbyname, 1 if we use a local storage */
#ifndef LWIP_DNS_API_HOSTENT_STORAGE
#define LWIP_DNS_API_HOSTENT_STORAGE 0
#endif

/** define "hostent" variables storage */
#if LWIP_DNS_API_HOSTENT_STORAGE
#define HOSTENT_STORAGE
#else
#define HOSTENT_STORAGE static
#endif /* LWIP_DNS_API_STATIC_HOSTENT */

/**
 * Returns an entry containing addresses of address family AF_INET
 * for the host with name name.
 * Due to dns_gethostbyname limitations, only one address is returned.
 *
 * @param name the hostname to resolve
 * @return an entry containing addresses of address family AF_INET
 *         for the host with name name
 */
struct hostent *
lwip_gethostbyname(const char *name)
{
  err_t err;
  ip_addr_t addr;

  /* buffer variables for lwip_gethostbyname() */
  HOSTENT_STORAGE struct hostent s_hostent;
  HOSTENT_STORAGE char *s_aliases;
  HOSTENT_STORAGE ip_addr_t s_hostent_addr;
  HOSTENT_STORAGE ip_addr_t *s_phostent_addr[2];
  HOSTENT_STORAGE char s_hostname[DNS_MAX_NAME_LENGTH + 1];

  /* query host IP address */
  err = netconn_gethostbyname(name, &addr);
  if (err != ERR_OK) {
    LWIP_DEBUGF(DNS_DEBUG, ("lwip_gethostbyname(%s) failed, err=%d\n", name, err));
    h_errno = HOST_NOT_FOUND;
    return NULL;
  }

  /* fill hostent */
  s_hostent_addr = addr;
  s_phostent_addr[0] = &s_hostent_addr;
  s_phostent_addr[1] = NULL;
  strncpy(s_hostname, name, DNS_MAX_NAME_LENGTH);
  s_hostname[DNS_MAX_NAME_LENGTH] = 0;
  s_hostent.h_name = s_hostname;
  s_aliases = NULL;
  s_hostent.h_aliases = &s_aliases;
  s_hostent.h_addrtype = AF_INET;
  s_hostent.h_length = sizeof(ip_addr_t);
  s_hostent.h_addr_list = (char **)&s_phostent_addr;

#if DNS_DEBUG
  /* dump hostent */
  LWIP_DEBUGF(DNS_DEBUG, ("hostent.h_name           == %s\n", s_hostent.h_name));
  LWIP_DEBUGF(DNS_DEBUG, ("hostent.h_aliases        == %p\n", (void *)s_hostent.h_aliases));
  /* h_aliases are always empty */
  LWIP_DEBUGF(DNS_DEBUG, ("hostent.h_addrtype       == %d\n", s_hostent.h_addrtype));
  LWIP_DEBUGF(DNS_DEBUG, ("hostent.h_length         == %d\n", s_hostent.h_length));
  LWIP_DEBUGF(DNS_DEBUG, ("hostent.h_addr_list      == %p\n", (void *)s_hostent.h_addr_list));
  if (s_hostent.h_addr_list != NULL) {
    u8_t idx;
    for (idx = 0; s_hostent.h_addr_list[idx]; idx++) {
      LWIP_DEBUGF(DNS_DEBUG, ("hostent.h_addr_list[%i]   == %p\n", idx, s_hostent.h_addr_list[idx]));
      LWIP_DEBUGF(DNS_DEBUG, ("hostent.h_addr_list[%i]-> == %s\n", idx, ipaddr_ntoa((ip_addr_t *)s_hostent.h_addr_list[idx])));
    }
  }
#endif /* DNS_DEBUG */

#if LWIP_DNS_API_HOSTENT_STORAGE
  /* this function should return the "per-thread" hostent after copy from s_hostent */
  return sys_thread_hostent(&s_hostent);
#else
  return &s_hostent;
#endif /* LWIP_DNS_API_HOSTENT_STORAGE */
}

/**
 * Thread-safe variant of lwip_gethostbyname: instead of using a static
 * buffer, this function takes buffer and errno pointers as arguments
 * and uses these for the result.
 *
 * @param name the hostname to resolve
 * @param ret pre-allocated struct where to store the result
 * @param buf pre-allocated buffer where to store additional data
 * @param buflen the size of buf
 * @param result pointer to a hostent pointer that is set to ret on success
 *               and set to zero on error
 * @param h_errnop pointer to an int where to store errors (instead of modifying
 *                 the global h_errno)
 * @return 0 on success, non-zero on error, additional error information
 *         is stored in *h_errnop instead of h_errno to be thread-safe
 */
int
lwip_gethostbyname_r(const char *name, struct hostent *ret, char *buf,
                     size_t buflen, struct hostent **result, int *h_errnop)
{
  err_t err;
  struct gethostbyname_r_helper *h;
  char *hostname;
  size_t namelen;
  int lh_errno;

  if (h_errnop == NULL) {
    /* ensure h_errnop is never NULL */
    h_errnop = &lh_errno;
  }

  if (result == NULL) {
    /* not all arguments given */
    *h_errnop = EINVAL;
    return -1;
  }
  /* first thing to do: set *result to nothing */
  *result = NULL;
  if ((name == NULL) || (ret == NULL) || (buf == NULL)) {
    /* not all arguments given */
    *h_errnop = EINVAL;
    return -1;
  }

  namelen = strlen(name);
  if (buflen < (sizeof(struct gethostbyname_r_helper) + LWIP_MEM_ALIGN_BUFFER(namelen + 1))) {
    /* buf can't hold the data needed + a copy of name */
    *h_errnop = ERANGE;
    return -1;
  }

  h = (struct gethostbyname_r_helper *)LWIP_MEM_ALIGN(buf);
  hostname = ((char *)h) + sizeof(struct gethostbyname_r_helper);

  /* query host IP address */
  err = netconn_gethostbyname(name, &h->addr);
  if (err != ERR_OK) {
    LWIP_DEBUGF(DNS_DEBUG, ("lwip_gethostbyname(%s) failed, err=%d\n", name, err));
    *h_errnop = HOST_NOT_FOUND;
    return -1;
  }

  /* copy the hostname into buf */
  MEMCPY(hostname, name, namelen);
  hostname[namelen] = 0;

  /* fill hostent */
  h->addr_list[0] = &h->addr;
  h->addr_list[1] = NULL;
  h->aliases = NULL;
  ret->h_name = hostname;
  ret->h_aliases = &h->aliases;
  ret->h_addrtype = AF_INET;
  ret->h_length = sizeof(ip_addr_t);
  ret->h_addr_list = (char **)&h->addr_list;

  /* set result != NULL */
  *result = ret;

  /* return success */
  return 0;
}

/**
 * Frees one or more addrinfo structures returned by getaddrinfo(), along with
 * any additional storage associated with those structures. If the ai_next field
 * of the structure is not null, the entire list of structures is freed.
 *
 * @param ai struct addrinfo to free
 */
void
lwip_freeaddrinfo(struct addrinfo *ai)
{
  struct addrinfo *next;

  while (ai != NULL) {
    next = ai->ai_next;
    memp_free(MEMP_NETDB, ai);
    ai = next;
  }
}

/**
 * Translates the name of a service location (for example, a host name) and/or
 * a service name and returns a set of socket addresses and associated
 * information to be used in creating a socket with which to address the
 * specified service.
 * Memory for the result is allocated internally and must be freed by calling
 * lwip_freeaddrinfo()!
 *
 * Due to a limitation in dns_gethostbyname, only the first address of a
 * host is returned.
 * Also, service names are not supported (only port numbers)!
 *
 * @param nodename descriptive name or address string of the host
 *                 (may be NULL -> local address)
 * @param servname port number as string of NULL
 * @param hints structure containing input values that set socktype and protocol
 * @param res pointer to a pointer where to store the result (set to NULL on failure)
 * @return 0 on success, non-zero on failure
 *
 * @todo: implement AI_V4MAPPED, AI_ADDRCONFIG
 */
int
lwip_getaddrinfo(const char *nodename, const char *servname,
                 const struct addrinfo *hints, struct addrinfo **res)
{
  err_t err;
  ip_addr_t addr;
  struct addrinfo *ai;
  struct sockaddr_storage *sa = NULL;
  int port_nr = 0;
  size_t total_size;
  size_t namelen = 0;
  int ai_family;

  if (res == NULL) {
    return EAI_FAIL;
  }
  *res = NULL;
  if ((nodename == NULL) && (servname == NULL)) {
    return EAI_NONAME;
  }

  if (hints != NULL) {
    ai_family = hints->ai_family;
    if ((ai_family != AF_UNSPEC)
#if LWIP_IPV4
        && (ai_family != AF_INET)
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
        && (ai_family != AF_INET6)
#endif /* LWIP_IPV6 */
       ) {
      return EAI_FAMILY;
    }
  } else {
    ai_family = AF_UNSPEC;
  }

  if (servname != NULL) {
    /* service name specified: convert to port number
     * @todo?: currently, only ASCII integers (port numbers) are supported (AI_NUMERICSERV)! */
    port_nr = atoi(servname);
    if ((port_nr <= 0) || (port_nr > 0xffff)) {
      return EAI_SERVICE;
    }
  }

  if (nodename != NULL) {
    /* service location specified, try to resolve */
    if ((hints != NULL) && (hints->ai_flags & AI_NUMERICHOST)) {
      /* no DNS lookup, just parse for an address string */
      if (!ipaddr_aton(nodename, &addr)) {
        return EAI_NONAME;
      }
#if LWIP_IPV4 && LWIP_IPV6
      if ((IP_IS_V6_VAL(addr) && ai_family == AF_INET) ||
          (IP_IS_V4_VAL(addr) && ai_family == AF_INET6)) {
        return EAI_NONAME;
      }
#endif /* LWIP_IPV4 && LWIP_IPV6 */
    } else {
#if LWIP_IPV4 && LWIP_IPV6
      /* AF_UNSPEC: prefer IPv4 */
      u8_t type = NETCONN_DNS_IPV4_IPV6;
      if (ai_family == AF_INET) {
        type = NETCONN_DNS_IPV4;
      } else if (ai_family == AF_INET6) {
        type = NETCONN_DNS_IPV6;
      }
#endif /* LWIP_IPV4 && LWIP_IPV6 */
      err = netconn_gethostbyname_addrtype(nodename, &addr, type);
      if (err != ERR_OK) {
        return EAI_FAIL;
      }
    }
  } else {
    /* service location specified, use loopback address */
    if ((hints != NULL) && (hints->ai_flags & AI_PASSIVE)) {
      ip_addr_set_any_val(ai_family == AF_INET6, addr);
    } else {
      ip_addr_set_loopback_val(ai_family == AF_INET6, addr);
    }
  }

  total_size = sizeof(struct addrinfo) + sizeof(struct sockaddr_storage);
  if (nodename != NULL) {
    namelen = strlen(nodename);
    if (namelen > DNS_MAX_NAME_LENGTH) {
      /* invalid name length */
      return EAI_FAIL;
    }
    LWIP_ASSERT("namelen is too long", total_size + namelen + 1 > total_size);
    total_size += namelen + 1;
  }
  /* If this fails, please report to lwip-devel! :-) */
  LWIP_ASSERT("total_size <= NETDB_ELEM_SIZE: please report this!",
              total_size <= NETDB_ELEM_SIZE);
  ai = (struct addrinfo *)memp_malloc(MEMP_NETDB);
  if (ai == NULL) {
    return EAI_MEMORY;
  }
  memset(ai, 0, total_size);
  /* cast through void* to get rid of alignment warnings */
  sa = (struct sockaddr_storage *)(void *)((u8_t *)ai + sizeof(struct addrinfo));
  if (IP_IS_V6_VAL(addr)) {
#if LWIP_IPV6
    struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
    /* set up sockaddr */
    inet6_addr_from_ip6addr(&sa6->sin6_addr, ip_2_ip6(&addr));
    sa6->sin6_family = AF_INET6;
    sa6->sin6_len = sizeof(struct sockaddr_in6);
    sa6->sin6_port = lwip_htons((u16_t)port_nr);
    sa6->sin6_scope_id = ip6_addr_zone(ip_2_ip6(&addr));
    ai->ai_family = AF_INET6;
#endif /* LWIP_IPV6 */
  } else {
#if LWIP_IPV4
    struct sockaddr_in *sa4 = (struct sockaddr_in *)sa;
    /* set up sockaddr */
    inet_addr_from_ip4addr(&sa4->sin_addr, ip_2_ip4(&addr));
    sa4->sin_family = AF_INET;
    sa4->sin_len = sizeof(struct sockaddr_in);
    sa4->sin_port = lwip_htons((u16_t)port_nr);
    ai->ai_family = AF_INET;
#endif /* LWIP_IPV4 */
  }

  /* set up addrinfo */
  if (hints != NULL) {
    /* copy socktype & protocol from hints if specified */
    ai->ai_socktype = hints->ai_socktype;
    ai->ai_protocol = hints->ai_protocol;
  }
  if (nodename != NULL) {
    /* copy nodename to canonname if specified */
    ai->ai_canonname = ((char *)ai + sizeof(struct addrinfo) + sizeof(struct sockaddr_storage));
    MEMCPY(ai->ai_canonname, nodename, namelen);
    ai->ai_canonname[namelen] = 0;
  }
  ai->ai_addrlen = sizeof(struct sockaddr_storage);
  ai->ai_addr = (struct sockaddr *)sa;

  *res = ai;

  return 0;
}

#endif /* LWIP_DNS && LWIP_SOCKET */
