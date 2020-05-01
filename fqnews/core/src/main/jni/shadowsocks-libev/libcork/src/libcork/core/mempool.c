/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2015, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <stdlib.h>

#include "libcork/core/callbacks.h"
#include "libcork/core/mempool.h"
#include "libcork/core/types.h"
#include "libcork/helpers/errors.h"


#if !defined(CORK_DEBUG_MEMPOOL)
#define CORK_DEBUG_MEMPOOL  0
#endif

#if CORK_DEBUG_MEMPOOL
#include <stdio.h>
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...) /* no debug messages */
#endif



struct cork_mempool {
    size_t  element_size;
    size_t  block_size;
    struct cork_mempool_object  *free_list;
    /* The number of objects that have been given out by
     * cork_mempool_new but not returned via cork_mempool_free. */
    size_t  allocated_count;
    struct cork_mempool_block  *blocks;

    void  *user_data;
    cork_free_f  free_user_data;
    cork_init_f  init_object;
    cork_done_f  done_object;
};

struct cork_mempool_object {
    /* When this object is unclaimed, it will be in the cork_mempool
     * object's free_list using this pointer. */
    struct cork_mempool_object  *next_free;
};

struct cork_mempool_block {
    struct cork_mempool_block  *next_block;
};

#define cork_mempool_object_size(mp) \
    (sizeof(struct cork_mempool_object) + (mp)->element_size)

#define cork_mempool_get_header(obj) \
    (((struct cork_mempool_object *) (obj)) - 1)

#define cork_mempool_get_object(hdr) \
    ((void *) (((struct cork_mempool_object *) (hdr)) + 1))


struct cork_mempool *
cork_mempool_new_size_ex(size_t element_size, size_t block_size)
{
    struct cork_mempool  *mp = cork_new(struct cork_mempool);
    mp->element_size = element_size;
    mp->block_size = block_size;
    mp->free_list = NULL;
    mp->allocated_count = 0;
    mp->blocks = NULL;
    mp->user_data = NULL;
    mp->free_user_data = NULL;
    mp->init_object = NULL;
    mp->done_object = NULL;
    return mp;
}

void
cork_mempool_free(struct cork_mempool *mp)
{
    struct cork_mempool_block  *curr;
    assert(mp->allocated_count == 0);

    if (mp->done_object != NULL) {
        struct cork_mempool_object  *obj;
        for (obj = mp->free_list; obj != NULL; obj = obj->next_free) {
            mp->done_object
                (mp->user_data, cork_mempool_get_object(obj));
        }
    }

    for (curr = mp->blocks; curr != NULL; ) {
        struct cork_mempool_block  *next = curr->next_block;
        cork_free(curr, mp->block_size);
        /* Do this here instead of in the for statement to avoid
         * accessing the just-freed block. */
        curr = next;
    }

    cork_free_user_data(mp);
    cork_delete(struct cork_mempool, mp);
}


void
cork_mempool_set_user_data(struct cork_mempool *mp,
                           void *user_data, cork_free_f free_user_data)
{
    cork_free_user_data(mp);
    mp->user_data = user_data;
    mp->free_user_data = free_user_data;
}

void
cork_mempool_set_init_object(struct cork_mempool *mp, cork_init_f init_object)
{
    mp->init_object = init_object;
}

void
cork_mempool_set_done_object(struct cork_mempool *mp, cork_done_f done_object)
{
    mp->done_object = done_object;
}

void
cork_mempool_set_callbacks(struct cork_mempool *mp,
                           void *user_data, cork_free_f free_user_data,
                           cork_init_f init_object,
                           cork_done_f done_object)
{
    cork_mempool_set_user_data(mp, user_data, free_user_data);
    cork_mempool_set_init_object(mp, init_object);
    cork_mempool_set_done_object(mp, done_object);
}


/* If this function succeeds, then we guarantee that there will be at
 * least one object in mp->free_list. */
static void
cork_mempool_new_block(struct cork_mempool *mp)
{
    /* Allocate the new block and add it to mp's block list. */
    struct cork_mempool_block  *block;
    void  *vblock;
    DEBUG("Allocating new %zu-byte block\n", mp->block_size);
    block = cork_malloc(mp->block_size);
    block->next_block = mp->blocks;
    mp->blocks = block;
    vblock = block;

    /* Divide the block's memory region into a bunch of objects. */
    size_t  index = sizeof(struct cork_mempool_block);
    for (index = sizeof(struct cork_mempool_block);
         (index + cork_mempool_object_size(mp)) <= mp->block_size;
         index += cork_mempool_object_size(mp)) {
        struct cork_mempool_object  *obj = vblock + index;
        DEBUG("  New object at %p[%p]\n", cork_mempool_get_object(obj), obj);
        if (mp->init_object != NULL) {
            mp->init_object
                (mp->user_data, cork_mempool_get_object(obj));
        }
        obj->next_free = mp->free_list;
        mp->free_list = obj;
    }
}

void *
cork_mempool_new_object(struct cork_mempool *mp)
{
    struct cork_mempool_object  *obj;
    void  *ptr;

    if (CORK_UNLIKELY(mp->free_list == NULL)) {
        cork_mempool_new_block(mp);
    }

    obj = mp->free_list;
    mp->free_list = obj->next_free;
    mp->allocated_count++;
    ptr = cork_mempool_get_object(obj);
    return ptr;
}

void
cork_mempool_free_object(struct cork_mempool *mp, void *ptr)
{
    struct cork_mempool_object  *obj = cork_mempool_get_header(ptr);
    DEBUG("Returning %p[%p] to memory pool\n", ptr, obj);
    obj->next_free = mp->free_list;
    mp->free_list = obj;
    mp->allocated_count--;
}
