/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_PROCESS_H
#define LIBCORK_CORE_PROCESS_H

#include <libcork/core/api.h>


typedef void
(*cork_cleanup_function)(void);

CORK_API void
cork_cleanup_at_exit_named(const char *name, int priority,
                           cork_cleanup_function function);

#define cork_cleanup_at_exit(priority, function) \
    cork_cleanup_at_exit_named(#function, priority, function)


#endif /* LIBCORK_CORE_PROCESS_H */
