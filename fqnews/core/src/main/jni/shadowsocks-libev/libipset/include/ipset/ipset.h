/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef IPSET_IPSET_H
#define IPSET_IPSET_H

#include <stdio.h>

#include <libcork/core.h>
#include <libcork/ds.h>

#include <ipset/bdd/nodes.h>


struct ip_set {
    struct ipset_node_cache  *cache;
    ipset_node_id  set_bdd;
};


struct ip_map {
    struct ipset_node_cache  *cache;
    ipset_node_id  map_bdd;
    ipset_node_id  default_bdd;
};


/*---------------------------------------------------------------------
 * General functions
 */

int
ipset_init_library(void);


/*---------------------------------------------------------------------
 * IP set functions
 */

void
ipset_init(struct ip_set *set);

void
ipset_done(struct ip_set *set);

struct ip_set *
ipset_new(void);

void
ipset_free(struct ip_set *set);

bool
ipset_is_empty(const struct ip_set *set);

bool
ipset_is_equal(const struct ip_set *set1, const struct ip_set *set2);

size_t
ipset_memory_size(const struct ip_set *set);

int
ipset_save(FILE *stream, const struct ip_set *set);

int
ipset_save_to_stream(struct cork_stream_consumer *stream,
                     const struct ip_set *set);

int
ipset_save_dot(FILE *stream, const struct ip_set *set);

struct ip_set *
ipset_load(FILE *stream);

bool
ipset_ipv4_add(struct ip_set *set, struct cork_ipv4 *elem);

bool
ipset_ipv4_add_network(struct ip_set *set, struct cork_ipv4 *elem,
                       unsigned int cidr_prefix);

bool
ipset_ipv4_remove(struct ip_set *set, struct cork_ipv4 *elem);

bool
ipset_ipv4_remove_network(struct ip_set *set, struct cork_ipv4 *elem,
                          unsigned int cidr_prefix);

bool
ipset_contains_ipv4(const struct ip_set *set, struct cork_ipv4 *elem);

bool
ipset_ipv6_add(struct ip_set *set, struct cork_ipv6 *elem);

bool
ipset_ipv6_add_network(struct ip_set *set, struct cork_ipv6 *elem,
                       unsigned int cidr_prefix);

bool
ipset_ipv6_remove(struct ip_set *set, struct cork_ipv6 *elem);

bool
ipset_ipv6_remove_network(struct ip_set *set, struct cork_ipv6 *elem,
                          unsigned int cidr_prefix);

bool
ipset_contains_ipv6(const struct ip_set *set, struct cork_ipv6 *elem);

bool
ipset_ip_add(struct ip_set *set, struct cork_ip *addr);

bool
ipset_ip_add_network(struct ip_set *set, struct cork_ip *addr,
                     unsigned int cidr_prefix);

bool
ipset_ip_remove(struct ip_set *set, struct cork_ip *addr);

bool
ipset_ip_remove_network(struct ip_set *set, struct cork_ip *addr,
                        unsigned int cidr_prefix);

bool
ipset_contains_ip(const struct ip_set *set, struct cork_ip *elem);


/* An internal state type used by the ipset_iterator_multiple_expansion_state
 * field. */
enum ipset_iterator_state {
    IPSET_ITERATOR_NORMAL = 0,
    IPSET_ITERATOR_MULTIPLE_IPV4,
    IPSET_ITERATOR_MULTIPLE_IPV6
};


/* An iterator that returns all of the IP addresses that have a given value in
 * an IP set or map. */
struct ipset_iterator {
    /* The address of the current IP network in the iterator. */
    struct cork_ip  addr;

    /* The netmask of the current IP network in the iterator, given as a
     * CIDR prefix.  For a single IP address, this will be 32 or 128. */
    unsigned int  cidr_prefix;

    /* Whether the current assignment needs to be expanded a second
     * time.
     *
     * We have to expand IPv4 and IPv6 assignments separately, since the
     * set of variables to turn into address bits is different.
     * Unfortunately, a BDD assignment can contain both IPv4 and IPv6
     * addresses, if variable 0 is EITHER.  (This is trivially true for
     * the empty set, for instance.)  In this case, we have to
     * explicitly set variable 0 to TRUE, expand it as IPv4, and then
     * set it to FALSE, and expand it as IPv6.  This variable tells us
     * whether we're in an assignment that needs to be expanded twice,
     * and if so, which expansion we're currently in.
     */
    enum ipset_iterator_state  multiple_expansion_state;

    /* An iterator for retrieving each assignment in the set's BDD. */
    struct ipset_bdd_iterator  *bdd_iterator;

    /* An iterator for expanding each assignment into individual IP
     * addresses. */
    struct ipset_expanded_assignment  *assignment_iterator;

    /* Whether there are any more IP addresses in this iterator. */
    bool  finished;

    /* The desired value for each IP address. */
    bool  desired_value;

    /* Whether to summarize the contents of the IP set as networks,
     * where possible. */
    bool  summarize;
};


struct ipset_iterator *
ipset_iterate(struct ip_set *set, bool desired_value);

struct ipset_iterator *
ipset_iterate_networks(struct ip_set *set, bool desired_value);

void
ipset_iterator_free(struct ipset_iterator *iterator);

void
ipset_iterator_advance(struct ipset_iterator *iterator);


/*---------------------------------------------------------------------
 * IP map functions
 */

void
ipmap_init(struct ip_map *map, int default_value);

void
ipmap_done(struct ip_map *map);

struct ip_map *
ipmap_new(int default_value);

void
ipmap_free(struct ip_map *map);

bool
ipmap_is_empty(const struct ip_map *map);

bool
ipmap_is_equal(const struct ip_map *map1, const struct ip_map *map2);

size_t
ipmap_memory_size(const struct ip_map *map);

int
ipmap_save(FILE *stream, const struct ip_map *map);

int
ipmap_save_to_stream(struct cork_stream_consumer *stream,
                     const struct ip_map *map);

struct ip_map *
ipmap_load(FILE *stream);

void
ipmap_ipv4_set(struct ip_map *map, struct cork_ipv4 *elem, int value);

void
ipmap_ipv4_set_network(struct ip_map *map, struct cork_ipv4 *elem,
                       unsigned int cidr_prefix, int value);

int
ipmap_ipv4_get(struct ip_map *map, struct cork_ipv4 *elem);

void
ipmap_ipv6_set(struct ip_map *map, struct cork_ipv6 *elem, int value);

void
ipmap_ipv6_set_network(struct ip_map *map, struct cork_ipv6 *elem,
                       unsigned int cidr_prefix, int value);

int
ipmap_ipv6_get(struct ip_map *map, struct cork_ipv6 *elem);

void
ipmap_ip_set(struct ip_map *map, struct cork_ip *addr, int value);

void
ipmap_ip_set_network(struct ip_map *map, struct cork_ip *addr,
                     unsigned int cidr_prefix, int value);

int
ipmap_ip_get(struct ip_map *map, struct cork_ip *addr);


#endif  /* IPSET_IPSET_H */
