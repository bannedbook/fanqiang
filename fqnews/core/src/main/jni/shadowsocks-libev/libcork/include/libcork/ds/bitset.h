/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_DS_BITS_H
#define LIBCORK_DS_BITS_H


#include <libcork/core/api.h>
#include <libcork/core/types.h>


/*-----------------------------------------------------------------------
 * Bit sets
 */

struct cork_bitset {
    uint8_t  *bits;
    size_t  bit_count;
    size_t  byte_count;
};

CORK_API struct cork_bitset *
cork_bitset_new(size_t bit_count);

CORK_API void
cork_bitset_init(struct cork_bitset *set, size_t bit_count);

CORK_API void
cork_bitset_free(struct cork_bitset *set);

CORK_API void
cork_bitset_done(struct cork_bitset *set);

CORK_API void
cork_bitset_clear(struct cork_bitset *set);

/* Extract the byte that contains a particular bit in an array. */
#define cork_bitset_byte_for_bit(set, i) \
    ((set)->bits[(i) / 8])

/* Create a bit mask that extracts a particular bit from the byte that it lives
 * in. */
#define cork_bitset_pos_mask_for_bit(i) \
    (0x80 >> ((i) % 8))

/* Create a bit mask that extracts everything except for a particular bit from
 * the byte that it lives in. */
#define cork_bitset_neg_mask_for_bit(i) \
    (~cork_bitset_pos_mask_for_bit(i))

/* Return whether a particular bit is set in a byte array.  Bits are numbered
 * from 0, in a big-endian order. */
#define cork_bitset_get(set, i) \
    ((cork_bitset_byte_for_bit(set, i) & cork_bitset_pos_mask_for_bit(i)) != 0)

/* Set (or unset) a particular bit is set in a byte array.  Bits are numbered
 * from 0, in a big-endian order. */
#define cork_bitset_set(set, i, val) \
    (cork_bitset_byte_for_bit(set, i) = \
     (cork_bitset_byte_for_bit(set, i) & cork_bitset_neg_mask_for_bit(i)) \
     | ((val)? cork_bitset_pos_mask_for_bit(i): 0))


#endif /* LIBCORK_DS_BITS_H */
