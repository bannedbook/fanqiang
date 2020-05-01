/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef IPSET_BITS_H
#define IPSET_BITS_H

#include <libcork/core.h>

/*-----------------------------------------------------------------------
 * Bit arrays
 */

/**
 * Extract the byte that contains a particular bit in an array.
 */
#define IPSET_BIT_GET_BYTE(array, i)            \
    (((uint8_t *) (array))[(i) / 8])

/**
 * Create a bit mask that extracts a particular bit from the byte that
 * it lives in.
 */
#define IPSET_BIT_ON_MASK(i)                    \
    (0x80 >> ((i) % 8))

/**
 * Create a bit mask that extracts everything except for a particular
 * bit from the byte that it lives in.
 */
#define IPSET_BIT_NEG_MASK(i)                   \
    (~IPSET_BIT_ON_MASK(i))

/**
 * Return whether a particular bit is set in a byte array.  Bits are
 * numbered from 0, in a big-endian order.
 */
#define IPSET_BIT_GET(array, i)                 \
    ((IPSET_BIT_GET_BYTE(array, i) &            \
      IPSET_BIT_ON_MASK(i)) != 0)

/**
 * Set (or unset) a particular bit is set in a byte array.  Bits are
 * numbered from 0, in a big-endian order.
 */
#define IPSET_BIT_SET(array, i, val)                           \
    (IPSET_BIT_GET_BYTE(array, i) =                            \
     (IPSET_BIT_GET_BYTE(array, i) & IPSET_BIT_NEG_MASK(i))    \
     | ((val)? IPSET_BIT_ON_MASK(i): 0))


#endif  /* IPSET_BITS_H */
