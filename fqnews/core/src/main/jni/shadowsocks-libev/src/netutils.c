/*
 * netutils.c - Network utilities
 *
 * Copyright (C) 2013 - 2019, Max Lv <max.c.lv@gmail.com>
 *
 * This file is part of the shadowsocks-libev.
 *
 * shadowsocks-libev is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * shadowsocks-libev is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shadowsocks-libev; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <math.h>

#include <libcork/core.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __MINGW32__
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#if defined(HAVE_SYS_IOCTL_H) && defined(HAVE_NET_IF_H) && defined(__linux__)
#include <net/if.h>
#include <sys/ioctl.h>
#define SET_INTERFACE
#endif

#include "netutils.h"
#include "utils.h"

#ifndef SO_REUSEPORT
#define SO_REUSEPORT 15
#endif

extern int verbose;

static const char valid_label_bytes[] =
    "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

int
set_reuseport(int socket)
{
    int opt = 1;
    return setsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
}

size_t
get_sockaddr_len(struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET) {
        return sizeof(struct sockaddr_in);
    } else if (addr->sa_family == AF_INET6) {
        return sizeof(struct sockaddr_in6);
    }
    return 0;
}

#ifdef SET_INTERFACE
int
setinterface(int socket_fd, const char *interface_name)
{
    struct ifreq interface;
    memset(&interface, 0, sizeof(struct ifreq));
    strncpy(interface.ifr_name, interface_name, IFNAMSIZ - 1);
    int res = setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &interface,
                         sizeof(struct ifreq));
    return res;
}

#endif

int
parse_local_addr(struct sockaddr_storage *storage_v4,
                 struct sockaddr_storage *storage_v6,
                 const char *host)
{
    if (host != NULL) {
        struct cork_ip ip;
        if (cork_ip_init(&ip, host) != -1) {
            if (ip.version == 4) {
                memset(storage_v4, 0, sizeof(struct sockaddr_storage));
                struct sockaddr_in *addr = (struct sockaddr_in *)storage_v4;
                inet_pton(AF_INET, host, &addr->sin_addr);
                addr->sin_family = AF_INET;
                LOGI("binding to outbound IPv4 addr: %s", host);
                return AF_INET;
            } else if (ip.version == 6) {
                memset(storage_v6, 0, sizeof(struct sockaddr_storage));
                struct sockaddr_in6 *addr = (struct sockaddr_in6 *)storage_v6;
                inet_pton(AF_INET6, host, &addr->sin6_addr);
                addr->sin6_family = AF_INET6;
                LOGI("binding to outbound IPv6 addr: %s", host);
                return AF_INET6;
            }
        }
    }
    return 0;
}

int
bind_to_addr(struct sockaddr_storage *storage,
             int socket_fd)
{
    if (storage->ss_family == AF_INET) {
        return bind(socket_fd, (struct sockaddr *)storage, sizeof(struct sockaddr_in));
    } else if (storage->ss_family == AF_INET6) {
        return bind(socket_fd, (struct sockaddr *)storage, sizeof(struct sockaddr_in6));
    }
    return -1;
}

ssize_t
get_sockaddr(char *host, char *port,
             struct sockaddr_storage *storage, int block,
             int ipv6first)
{
    struct cork_ip ip;
    if (cork_ip_init(&ip, host) != -1) {
        if (ip.version == 4) {
            struct sockaddr_in *addr = (struct sockaddr_in *)storage;
            addr->sin_family = AF_INET;
            inet_pton(AF_INET, host, &(addr->sin_addr));
            if (port != NULL) {
                addr->sin_port = htons(atoi(port));
            }
        } else if (ip.version == 6) {
            struct sockaddr_in6 *addr = (struct sockaddr_in6 *)storage;
            addr->sin6_family = AF_INET6;
            inet_pton(AF_INET6, host, &(addr->sin6_addr));
            if (port != NULL) {
                addr->sin6_port = htons(atoi(port));
            }
        }
        return 0;
    } else {
#ifdef __ANDROID__
        extern int vpn;
        assert(!vpn);   // protecting DNS packets isn't supported yet
#endif
        struct addrinfo hints;
        struct addrinfo *result, *rp;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family   = AF_UNSPEC;   /* Return IPv4 and IPv6 choices */
        hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */

        int err = getaddrinfo(host, port, &hints, &result);

        if (err != 0) {
            LOGE("getaddrinfo: %s", gai_strerror(err));
            return -1;
        }

        int prefer_af = ipv6first ? AF_INET6 : AF_INET;
        for (rp = result; rp != NULL; rp = rp->ai_next)
            if (rp->ai_family == prefer_af) {
                if (rp->ai_family == AF_INET)
                    memcpy(storage, rp->ai_addr, sizeof(struct sockaddr_in));
                else if (rp->ai_family == AF_INET6)
                    memcpy(storage, rp->ai_addr, sizeof(struct sockaddr_in6));
                break;
            }

        if (rp == NULL) {
            for (rp = result; rp != NULL; rp = rp->ai_next) {
                if (rp->ai_family == AF_INET)
                    memcpy(storage, rp->ai_addr, sizeof(struct sockaddr_in));
                else if (rp->ai_family == AF_INET6)
                    memcpy(storage, rp->ai_addr, sizeof(struct sockaddr_in6));
                break;
            }
        }

        if (rp == NULL) {
            LOGE("failed to resolve remote addr");
            return -1;
        }

        freeaddrinfo(result);
        return 0;
    }

    return -1;
}

int
sockaddr_cmp(struct sockaddr_storage *addr1,
             struct sockaddr_storage *addr2, socklen_t len)
{
    struct sockaddr_in *p1_in   = (struct sockaddr_in *)addr1;
    struct sockaddr_in *p2_in   = (struct sockaddr_in *)addr2;
    struct sockaddr_in6 *p1_in6 = (struct sockaddr_in6 *)addr1;
    struct sockaddr_in6 *p2_in6 = (struct sockaddr_in6 *)addr2;
    if (p1_in->sin_family < p2_in->sin_family)
        return -1;
    if (p1_in->sin_family > p2_in->sin_family)
        return 1;
    /* compare ip4 */
    if (p1_in->sin_family == AF_INET) {
        /* just order it, ntohs not required */
        if (p1_in->sin_port < p2_in->sin_port)
            return -1;
        if (p1_in->sin_port > p2_in->sin_port)
            return 1;
        return memcmp(&p1_in->sin_addr, &p2_in->sin_addr, INET_SIZE);
    } else if (p1_in6->sin6_family == AF_INET6) {
        /* just order it, ntohs not required */
        if (p1_in6->sin6_port < p2_in6->sin6_port)
            return -1;
        if (p1_in6->sin6_port > p2_in6->sin6_port)
            return 1;
        return memcmp(&p1_in6->sin6_addr, &p2_in6->sin6_addr,
                      INET6_SIZE);
    } else {
        /* eek unknown type, perform this comparison for sanity. */
        return memcmp(addr1, addr2, len);
    }
}

int
sockaddr_cmp_addr(struct sockaddr_storage *addr1,
                  struct sockaddr_storage *addr2, socklen_t len)
{
    struct sockaddr_in *p1_in   = (struct sockaddr_in *)addr1;
    struct sockaddr_in *p2_in   = (struct sockaddr_in *)addr2;
    struct sockaddr_in6 *p1_in6 = (struct sockaddr_in6 *)addr1;
    struct sockaddr_in6 *p2_in6 = (struct sockaddr_in6 *)addr2;
    if (p1_in->sin_family < p2_in->sin_family)
        return -1;
    if (p1_in->sin_family > p2_in->sin_family)
        return 1;
    if (verbose) {
        LOGI("sockaddr_cmp_addr: sin_family equal? %d", p1_in->sin_family == p2_in->sin_family);
    }
    /* compare ip4 */
    if (p1_in->sin_family == AF_INET) {
        return memcmp(&p1_in->sin_addr, &p2_in->sin_addr, INET_SIZE);
    } else if (p1_in6->sin6_family == AF_INET6) {
        return memcmp(&p1_in6->sin6_addr, &p2_in6->sin6_addr,
                      INET6_SIZE);
    } else {
        /* eek unknown type, perform this comparison for sanity. */
        return memcmp(addr1, addr2, len);
    }
}

int
validate_hostname(const char *hostname, const int hostname_len)
{
    if (hostname == NULL)
        return 0;

    if (hostname_len < 1 || hostname_len > 255)
        return 0;

    if (hostname[0] == '.')
        return 0;

    const char *label = hostname;
    while (label < hostname + hostname_len) {
        size_t label_len = hostname_len - (label - hostname);
        char *next_dot   = strchr(label, '.');
        if (next_dot != NULL)
            label_len = next_dot - label;

        if (label + label_len > hostname + hostname_len)
            return 0;

        if (label_len > 63 || label_len < 1)
            return 0;

        if (label[0] == '-' || label[label_len - 1] == '-')
            return 0;

        if (strspn(label, valid_label_bytes) < label_len)
            return 0;

        label += label_len + 1;
    }

    return 1;
}

int
is_ipv6only(ss_addr_t *servers, size_t server_num, int ipv6first)
{
    int i;
    for (i = 0; i < server_num; i++) {
        struct sockaddr_storage storage;
        memset(&storage, 0, sizeof(struct sockaddr_storage));
        if (get_sockaddr(servers[i].host, servers[i].port, &storage, 1, ipv6first) == -1) {
            FATAL("failed to resolve the provided hostname");
        }
        if (storage.ss_family != AF_INET6) {
            return 0;
        }
    }
    return 1;
}
