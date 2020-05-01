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
ipmap_init(struct ip_map *map, int default_value)
{
    /* The map starts empty, so every value assignment should yield the
     * default. */
    map->cache = ipset_node_cache_new();
    map->default_bdd = ipset_terminal_node_id(default_value);
    map->map_bdd = map->default_bdd;
}


struct ip_map *
ipmap_new(int default_value)
{
    struct ip_map  *result = cork_new(struct ip_map);
    ipmap_init(result, default_value);
    return result;
}


void
ipmap_done(struct ip_map *map)
{
    ipset_node_decref(map->cache, map->map_bdd);
    ipset_node_cache_free(map->cache);
}


void
ipmap_free(struct ip_map *map)
{
    ipmap_done(map);
    free(map);
}
