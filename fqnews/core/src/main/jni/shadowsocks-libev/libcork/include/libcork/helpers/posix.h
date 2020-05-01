/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_HELPERS_POSIX_H
#define LIBCORK_HELPERS_POSIX_H

/* This header is *not* automatically included when you include
 * libcork/core.h, since we define some macros that don't include a
 * cork_ or CORK_ prefix.  Don't want to pollute your namespace unless
 * you ask for it! */

#include <errno.h>

#include <libcork/core/allocator.h>
#include <libcork/core/attributes.h>
#include <libcork/core/error.h>


#if !defined(CORK_PRINT_ERRORS)
#define CORK_PRINT_ERRORS 0
#endif

#if !defined(CORK_PRINT_ERROR)
#if CORK_PRINT_ERRORS
#include <stdio.h>
#define CORK_PRINT_ERROR_(func, file, line) \
    fprintf(stderr, "---\nError in %s (%s:%u)\n  %s\n", \
            (func), (file), (unsigned int) (line), \
            cork_error_message());
#define CORK_PRINT_ERROR()  CORK_PRINT_ERROR_(__func__, __FILE__, __LINE__)
#else
#define CORK_PRINT_ERROR()  /* do nothing */
#endif
#endif


#define xi_check_posix(call, on_error) \
    do { \
        while (true) { \
            if ((call) == -1) { \
                if (errno == EINTR) { \
                    continue; \
                } else { \
                    cork_system_error_set(); \
                    CORK_PRINT_ERROR(); \
                    on_error; \
                } \
            } else { \
                break; \
            } \
        } \
    } while (0)

#define xp_check_posix(call, on_error) \
    do { \
        while (true) { \
            if ((call) == NULL) { \
                if (errno == EINTR) { \
                    continue; \
                } else { \
                    cork_system_error_set(); \
                    CORK_PRINT_ERROR(); \
                    on_error; \
                } \
            } else { \
                break; \
            } \
        } \
    } while (0)


#define ei_check_posix(call)  xi_check_posix(call, goto error)
#define rii_check_posix(call) xi_check_posix(call, return -1)
#define rpi_check_posix(call) xi_check_posix(call, return NULL)

#define ep_check_posix(call)  xp_check_posix(call, goto error)
#define rip_check_posix(call) xp_check_posix(call, return -1)
#define rpp_check_posix(call) xp_check_posix(call, return NULL)


#endif /* LIBCORK_HELPERS_POSIX_H */
