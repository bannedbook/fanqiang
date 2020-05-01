/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CONFIG_MINGW32_H
#define LIBCORK_CONFIG_MINGW32_H

#include <io.h>

/*-----------------------------------------------------------------------
 * Endianness
 */

/* Assume MinGW32 only works on x86 platform */ 

#define CORK_CONFIG_IS_BIG_ENDIAN      0
#define CORK_CONFIG_IS_LITTLE_ENDIAN   1

#define CORK_HAVE_REALLOCF  0
#define CORK_HAVE_PTHREADS  1

/*
 * File io stuff. Odd that this is not defined by MinGW.
 * Maybe there is an M$ish way to do it.
 */
#define F_SETFL    4
#define O_NONBLOCK 0x4000  /* non blocking I/O (POSIX style) */

#define F_GETFD 1
#define F_SETFD 2
#define FD_CLOEXEC 0x1

#define WNOHANG 1

/*
 * simple adaptors
 */

static inline int mingw_mkdir(const char *path, int mode)
{
        return mkdir(path);
}
#define mkdir mingw_mkdir


#endif /* LIBCORK_CONFIG_MINGW32_H */
