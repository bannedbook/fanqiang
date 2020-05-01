/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CONFIG_SOLARIS_H
#define LIBCORK_CONFIG_SOLARIS_H

/*-----------------------------------------------------------------------
 * Endianness
 */

#include <sys/isa_defs.h>

#if defined(_BIG_ENDIAN)
#define CORK_CONFIG_IS_BIG_ENDIAN      1
#define CORK_CONFIG_IS_LITTLE_ENDIAN   0
#elif defined(_LITTLE_ENDIAN)
#define CORK_CONFIG_IS_BIG_ENDIAN      0
#define CORK_CONFIG_IS_LITTLE_ENDIAN   1
#else
#error "Cannot determine system endianness"
#endif

#define CORK_HAVE_REALLOCF  0
#define CORK_HAVE_PTHREADS  1


#endif /* LIBCORK_CONFIG_SOLARIS_H */
