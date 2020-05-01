/*
 * acl.c - Manage the ACL (Access Control List)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>

#ifdef USE_SYSTEM_SHARED_LIB
#include <libcorkipset/ipset.h>
#else
#include <ipset/ipset.h>
#endif

#include "rule.h"
#include "netutils.h"
#include "utils.h"
#include "cache.h"
#include "acl.h"

static struct ip_set white_list_ipv4;
static struct ip_set white_list_ipv6;

static struct ip_set black_list_ipv4;
static struct ip_set black_list_ipv6;

static struct cork_dllist black_list_rules;
static struct cork_dllist white_list_rules;

static int acl_mode = BLACK_LIST;

static struct ip_set outbound_block_list_ipv4;
static struct ip_set outbound_block_list_ipv6;
static struct cork_dllist outbound_block_list_rules;

static void
parse_addr_cidr(const char *str, char *host, int *cidr)
{
    int ret = -1;
    char *pch;

    pch = strchr(str, '/');
    while (pch != NULL) {
        ret = pch - str;
        pch = strchr(pch + 1, '/');
    }
    if (ret == -1) {
        strcpy(host, str);
        *cidr = -1;
    } else {
        memcpy(host, str, ret);
        host[ret] = '\0';
        *cidr     = atoi(str + ret + 1);
    }
}

char *
trimwhitespace(char *str)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)   // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

int
init_acl(const char *path)
{
    if (path == NULL) {
        return -1;
    }

    // initialize ipset
    ipset_init_library();

    ipset_init(&white_list_ipv4);
    ipset_init(&white_list_ipv6);
    ipset_init(&black_list_ipv4);
    ipset_init(&black_list_ipv6);
    ipset_init(&outbound_block_list_ipv4);
    ipset_init(&outbound_block_list_ipv6);

    cork_dllist_init(&black_list_rules);
    cork_dllist_init(&white_list_rules);
    cork_dllist_init(&outbound_block_list_rules);

    struct ip_set *list_ipv4  = &black_list_ipv4;
    struct ip_set *list_ipv6  = &black_list_ipv6;
    struct cork_dllist *rules = &black_list_rules;

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        LOGE("Invalid acl path.");
        return -1;
    }

    char buf[MAX_HOSTNAME_LEN];

    while (!feof(f))
        if (fgets(buf, 256, f)) {
            // Discards the whole line if longer than 255 characters
            int long_line = 0;  // 1: Long  2: Error
            while ((strlen(buf) == 255) && (buf[254] != '\n')) {
                long_line = 1;
                LOGE("Discarding long ACL content: %s", buf);
                if (fgets(buf, 256, f) == NULL) {
                    long_line = 2;
                    break;
                }
            }
            if (long_line) {
                if (long_line == 1) {
                    LOGE("Discarding long ACL content: %s", buf);
                }
                continue;
            }

            // Trim the newline
            int len = strlen(buf);
            if (len > 0 && buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }

            char *comment = strchr(buf, '#');
            if (comment) {
                *comment = '\0';
            }

            char *line = trimwhitespace(buf);
            if (strlen(line) == 0) {
                continue;
            }

            if (strcmp(line, "[outbound_block_list]") == 0) {
                list_ipv4 = &outbound_block_list_ipv4;
                list_ipv6 = &outbound_block_list_ipv6;
                rules     = &outbound_block_list_rules;
                continue;
            } else if (strcmp(line, "[black_list]") == 0
                       || strcmp(line, "[bypass_list]") == 0) {
                list_ipv4 = &black_list_ipv4;
                list_ipv6 = &black_list_ipv6;
                rules     = &black_list_rules;
                continue;
            } else if (strcmp(line, "[white_list]") == 0
                       || strcmp(line, "[proxy_list]") == 0) {
                list_ipv4 = &white_list_ipv4;
                list_ipv6 = &white_list_ipv6;
                rules     = &white_list_rules;
                continue;
            } else if (strcmp(line, "[reject_all]") == 0
                       || strcmp(line, "[bypass_all]") == 0) {
                acl_mode = WHITE_LIST;
                continue;
            } else if (strcmp(line, "[accept_all]") == 0
                       || strcmp(line, "[proxy_all]") == 0) {
                acl_mode = BLACK_LIST;
                continue;
            }

            char host[MAX_HOSTNAME_LEN];
            int cidr;
            parse_addr_cidr(line, host, &cidr);

            struct cork_ip addr;
            int err = cork_ip_init(&addr, host);
            if (!err) {
                if (addr.version == 4) {
                    if (cidr >= 0) {
                        ipset_ipv4_add_network(list_ipv4, &(addr.ip.v4), cidr);
                    } else {
                        ipset_ipv4_add(list_ipv4, &(addr.ip.v4));
                    }
                } else if (addr.version == 6) {
                    if (cidr >= 0) {
                        ipset_ipv6_add_network(list_ipv6, &(addr.ip.v6), cidr);
                    } else {
                        ipset_ipv6_add(list_ipv6, &(addr.ip.v6));
                    }
                }
            } else {
                rule_t *rule = new_rule();
                accept_rule_arg(rule, line);
                init_rule(rule);
                add_rule(rules, rule);
            }
        }

    fclose(f);

    return 0;
}

void
free_rules(struct cork_dllist *rules)
{
    struct cork_dllist_item *iter;
    while ((iter = cork_dllist_head(rules)) != NULL) {
        rule_t *rule = cork_container_of(iter, rule_t, entries);
        remove_rule(rule);
    }
}

void
free_acl(void)
{
    ipset_done(&black_list_ipv4);
    ipset_done(&black_list_ipv6);
    ipset_done(&white_list_ipv4);
    ipset_done(&white_list_ipv6);

    free_rules(&black_list_rules);
    free_rules(&white_list_rules);
}

int
get_acl_mode(void)
{
    return acl_mode;
}

/*
 * Return 0,  if not match.
 * Return 1,  if match black list.
 * Return -1, if match white list.
 */
int
acl_match_host(const char *host)
{
    struct cork_ip addr;
    int ret = 0;
    int err = cork_ip_init(&addr, host);

    if (err) {
        int host_len = strlen(host);
        if (lookup_rule(&black_list_rules, host, host_len) != NULL)
            ret = 1;
        else if (lookup_rule(&white_list_rules, host, host_len) != NULL)
            ret = -1;
        return ret;
    }

    if (addr.version == 4) {
        if (ipset_contains_ipv4(&black_list_ipv4, &(addr.ip.v4)))
            ret = 1;
        else if (ipset_contains_ipv4(&white_list_ipv4, &(addr.ip.v4)))
            ret = -1;
    } else if (addr.version == 6) {
        if (ipset_contains_ipv6(&black_list_ipv6, &(addr.ip.v6)))
            ret = 1;
        else if (ipset_contains_ipv6(&white_list_ipv6, &(addr.ip.v6)))
            ret = -1;
    }

    return ret;
}

int
acl_add_ip(const char *ip)
{
    struct cork_ip addr;
    int err = cork_ip_init(&addr, ip);
    if (err) {
        return -1;
    }

    if (addr.version == 4) {
        ipset_ipv4_add(&black_list_ipv4, &(addr.ip.v4));
    } else if (addr.version == 6) {
        ipset_ipv6_add(&black_list_ipv6, &(addr.ip.v6));
    }

    return 0;
}

int
acl_remove_ip(const char *ip)
{
    struct cork_ip addr;
    int err = cork_ip_init(&addr, ip);
    if (err) {
        return -1;
    }

    if (addr.version == 4) {
        ipset_ipv4_remove(&black_list_ipv4, &(addr.ip.v4));
    } else if (addr.version == 6) {
        ipset_ipv6_remove(&black_list_ipv6, &(addr.ip.v6));
    }

    return 0;
}

/*
 * Return 0,  if not match.
 * Return 1,  if match black list.
 */
int
outbound_block_match_host(const char *host)
{
    struct cork_ip addr;
    int ret = 0;
    int err = cork_ip_init(&addr, host);

    if (err) {
        int host_len = strlen(host);
        if (lookup_rule(&outbound_block_list_rules, host, host_len) != NULL)
            ret = 1;
        return ret;
    }

    if (addr.version == 4) {
        if (ipset_contains_ipv4(&outbound_block_list_ipv4, &(addr.ip.v4)))
            ret = 1;
    } else if (addr.version == 6) {
        if (ipset_contains_ipv6(&outbound_block_list_ipv6, &(addr.ip.v6)))
            ret = 1;
    }

    return ret;
}
