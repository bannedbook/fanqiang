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


void
ipset_init(struct ip_set *set)
{
    /* The set starts empty, so every value assignment should yield
     * false. */
    set->cache = ipset_node_cache_new();
    set->set_bdd = ipset_terminal_node_id(false);
}


struct ip_set *
ipset_new(void)
{
    struct ip_set  *result = cork_new(struct ip_set);
    ipset_init(result);
    return result;
}


void
ipset_done(struct ip_set *set)
{
    ipset_node_decref(set->cache, set->set_bdd);
    ipset_node_cache_free(set->cache);
}


void
ipset_free(struct ip_set *set)
{
    ipset_done(set);
    free(set);
}
