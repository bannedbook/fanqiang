/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_CALLBACKS_H
#define LIBCORK_CORE_CALLBACKS_H


#include <libcork/core/hash.h>


typedef int
(*cork_copy_f)(void *user_data, void *dest, const void *src);

typedef void
(*cork_done_f)(void *user_data, void *value);

typedef void
(*cork_free_f)(void *value);

typedef cork_hash
(*cork_hash_f)(void *user_data, const void *value);

typedef bool
(*cork_equals_f)(void *user_data, const void *value1, const void *value2);

typedef void
(*cork_init_f)(void *user_data, void *value);

#define cork_free_user_data(parent) \
    ((parent)->free_user_data == NULL? (void) 0: \
     (parent)->free_user_data((parent)->user_data))

typedef void *
(*cork_new_f)(void *user_data);

typedef int
(*cork_run_f)(void *user_data);


#endif /* LIBCORK_CORE_CALLBACKS_H */
