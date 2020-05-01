/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_TYPES_H
#define LIBCORK_CORE_TYPES_H

/* For now, we assume that the C99 integer types are available using the
 * standard headers. */

#include <limits.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/* Define preprocessor macros that contain the size of several built-in
 * types.  Again, we assume that we have the C99 definitions available. */

#if SHRT_MAX == INT8_MAX
#define CORK_SIZEOF_SHORT  1
#elif SHRT_MAX == INT16_MAX
#define CORK_SIZEOF_SHORT  2
#elif SHRT_MAX == INT32_MAX
#define CORK_SIZEOF_SHORT  4
#elif SHRT_MAX == INT64_MAX
#define CORK_SIZEOF_SHORT  8
#else
#error "Cannot determine size of short"
#endif

#if INT_MAX == INT8_MAX
#define CORK_SIZEOF_INT  1
#elif INT_MAX == INT16_MAX
#define CORK_SIZEOF_INT  2
#elif INT_MAX == INT32_MAX
#define CORK_SIZEOF_INT  4
#elif INT_MAX == INT64_MAX
#define CORK_SIZEOF_INT  8
#else
#error "Cannot determine size of int"
#endif

#if LONG_MAX == INT8_MAX
#define CORK_SIZEOF_LONG  1
#elif LONG_MAX == INT16_MAX
#define CORK_SIZEOF_LONG  2
#elif LONG_MAX == INT32_MAX
#define CORK_SIZEOF_LONG  4
#elif LONG_MAX == INT64_MAX
#define CORK_SIZEOF_LONG  8
#else
#error "Cannot determine size of long"
#endif

#if INTPTR_MAX == INT8_MAX
#define CORK_SIZEOF_POINTER  1
#elif INTPTR_MAX == INT16_MAX
#define CORK_SIZEOF_POINTER  2
#elif INTPTR_MAX == INT32_MAX
#define CORK_SIZEOF_POINTER  4
#elif INTPTR_MAX == INT64_MAX
#define CORK_SIZEOF_POINTER  8
#else
#error "Cannot determine size of void *"
#endif


/* Return a pointer to a @c struct, given a pointer to one of its
 * fields. */
#define cork_container_of(field, struct_type, field_name) \
    ((struct_type *) (- offsetof(struct_type, field_name) + \
                      (void *) (field)))

#endif /* LIBCORK_CORE_TYPES_H */
