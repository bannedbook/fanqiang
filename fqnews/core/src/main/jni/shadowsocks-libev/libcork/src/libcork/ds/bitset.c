/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <string.h>

#include "libcork/core/allocator.h"
#include "libcork/core/api.h"
#include "libcork/core/types.h"
#include "libcork/ds/bitset.h"


static size_t
bytes_needed(size_t bit_count)
{
    /* We need one byte for every bit... */
    size_t  bytes_needed = bit_count / 8;
    /* Plus one extra for the leftovers that don't fit into a whole byte. */
    bytes_needed += ((bit_count % 8) > 0);
    return bytes_needed;
}

void
cork_bitset_init(struct cork_bitset *set, size_t bit_count)
{
    set->bit_count = bit_count;
    set->byte_count = bytes_needed(bit_count);
    set->bits = cork_malloc(set->byte_count);
    memset(set->bits, 0, set->byte_count);
}

struct cork_bitset *
cork_bitset_new(size_t bit_count)
{
    struct cork_bitset  *set = cork_new(struct cork_bitset);
    cork_bitset_init(set, bit_count);
    return set;
}

void
cork_bitset_done(struct cork_bitset *set)
{
    cork_free(set->bits, set->byte_count);
}

void
cork_bitset_free(struct cork_bitset *set)
{
    cork_bitset_done(set);
    cork_delete(struct cork_bitset, set);
}

void
cork_bitset_clear(struct cork_bitset *set)
{
    memset(set->bits, 0, set->byte_count);
}
