/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <libcork/core.h>

#include "ipset/bdd/nodes.h"
#include "ipset/ipset.h"

bool
ipset_is_empty(const struct ip_set *set)
{
    /* Since BDDs are unique, the only empty set is the “false” BDD. */
    return (set->set_bdd == ipset_terminal_node_id(false));
}

bool
ipset_is_equal(const struct ip_set *set1, const struct ip_set *set2)
{
    return ipset_node_cache_nodes_equal
        (set1->cache, set1->set_bdd, set2->cache, set2->set_bdd);
}

size_t
ipset_memory_size(const struct ip_set *set)
{
    return ipset_node_memory_size(set->cache, set->set_bdd);
}


bool
ipset_ip_add(struct ip_set *set, struct cork_ip *addr)
{
    if (addr->version == 4) {
        return ipset_ipv4_add(set, &addr->ip.v4);
    } else {
        return ipset_ipv6_add(set, &addr->ip.v6);
    }
}


bool
ipset_ip_add_network(struct ip_set *set, struct cork_ip *addr,
                     unsigned int cidr_prefix)
{
    if (addr->version == 4) {
        return ipset_ipv4_add_network(set, &addr->ip.v4, cidr_prefix);
    } else {
        return ipset_ipv6_add_network(set, &addr->ip.v6, cidr_prefix);
    }
}


bool
ipset_ip_remove(struct ip_set *set, struct cork_ip *addr)
{
    if (addr->version == 4) {
        return ipset_ipv4_remove(set, &addr->ip.v4);
    } else {
        return ipset_ipv6_remove(set, &addr->ip.v6);
    }
}


bool
ipset_ip_remove_network(struct ip_set *set, struct cork_ip *addr,
                        unsigned int cidr_prefix)
{
    if (addr->version == 4) {
        return ipset_ipv4_remove_network(set, &addr->ip.v4, cidr_prefix);
    } else {
        return ipset_ipv6_remove_network(set, &addr->ip.v6, cidr_prefix);
    }
}


bool
ipset_contains_ip(const struct ip_set *set, struct cork_ip *addr)
{
    if (addr->version == 4) {
        return ipset_contains_ipv4(set, &addr->ip.v4);
    } else {
        return ipset_contains_ipv6(set, &addr->ip.v6);
    }
}
