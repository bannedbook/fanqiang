/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2015, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CONFIG_CONFIG_H
#define LIBCORK_CONFIG_CONFIG_H


/* If you want to skip autodetection, define this to 1, and provide a
 * libcork/config/custom.h header file. */

#if !defined(CORK_CONFIG_SKIP_AUTODETECT)
#define CORK_CONFIG_SKIP_AUTODETECT  0
#endif


#if CORK_CONFIG_SKIP_AUTODETECT
/* The user has promised that they'll define everything themselves. */
#include <libcork/config/custom.h>

#else
/* Otherwise autodetect! */


/**** VERSION ****/

#include <libcork/config/version.h>


/**** ARCHITECTURES ****/

#include <libcork/config/arch.h>


/**** PLATFORMS ****/
#if (defined(__unix__) || defined(unix)) && !defined(USG)
/* We need this to test for BSD, but it's a good idea to have for
 * any brand of Unix.*/
#include <sys/param.h>
#endif

#if defined(__linux) || defined(__FreeBSD_kernel__) || defined(__GNU__) || defined(__CYGWIN__)
/* Do some Linux, kFreeBSD or GNU/Hurd specific autodetection. */
#include <libcork/config/linux.h>

#elif defined(__APPLE__) && defined(__MACH__)
/* Do some Mac OS X-specific autodetection. */
#include <libcork/config/macosx.h>

#elif defined(BSD) && (BSD >= 199103)
/* Do some BSD (4.3 code base or newer)specific autodetection. */
#include <libcork/config/bsd.h>

#elif defined(__MINGW32__)
/* Do some mingw32 autodetection. */
#include <libcork/config/mingw32.h>

#elif defined(__sun)
/* Do some Solaris autodetection. */
#include <libcork/config/solaris.h>

#endif  /* platforms */


/**** COMPILERS ****/

#if defined(__GNUC__)
/* Do some GCC-specific autodetection. */
#include <libcork/config/gcc.h>

#endif  /* compilers */


#endif  /* autodetect or not */


#endif /* LIBCORK_CONFIG_CONFIG_H */
