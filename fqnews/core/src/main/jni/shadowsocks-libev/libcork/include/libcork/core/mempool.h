/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2015, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORK_MEMPOOL_H
#define LIBCORK_CORK_MEMPOOL_H


#include <libcork/config.h>
#include <libcork/core/api.h>
#include <libcork/core/attributes.h>
#include <libcork/core/callbacks.h>
#include <libcork/core/types.h>


#define CORK_MEMPOOL_DEFAULT_BLOCK_SIZE  4096


struct cork_mempool;


CORK_API struct cork_mempool *
cork_mempool_new_size_ex(size_t element_size, size_t block_size);

#define cork_mempool_new_size(element_size) \
    (cork_mempool_new_size_ex \
     ((element_size), CORK_MEMPOOL_DEFAULT_BLOCK_SIZE))

#define cork_mempool_new_ex(type, block_size) \
    (cork_mempool_new_size_ex(sizeof(type), (block_size)))

#define cork_mempool_new(type) \
    (cork_mempool_new_size(sizeof(type)))

CORK_API void
cork_mempool_free(struct cork_mempool *mp);


CORK_API void
cork_mempool_set_user_data(struct cork_mempool *mp,
                           void *user_data, cork_free_f free_user_data);

CORK_API void
cork_mempool_set_init_object(struct cork_mempool *mp, cork_init_f init_object);

CORK_API void
cork_mempool_set_done_object(struct cork_mempool *mp, cork_done_f done_object);

/* Deprecated; you should now use separate calls to cork_mempool_set_user_data,
 * cork_mempool_set_init_object, and cork_mempool_set_done_object. */
CORK_API void
cork_mempool_set_callbacks(struct cork_mempool *mp,
                           void *user_data, cork_free_f free_user_data,
                           cork_init_f init_object,
                           cork_done_f done_object);


CORK_API void *
cork_mempool_new_object(struct cork_mempool *mp);


CORK_API void
cork_mempool_free_object(struct cork_mempool *mp, void *ptr);


#endif /* LIBCORK_CORK_MEMPOOL_H */
