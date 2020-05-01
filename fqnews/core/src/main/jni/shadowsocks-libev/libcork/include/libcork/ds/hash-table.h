/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_DS_HASH_TABLE_H
#define LIBCORK_DS_HASH_TABLE_H

#include <libcork/core/api.h>
#include <libcork/core/callbacks.h>
#include <libcork/core/hash.h>
#include <libcork/core/mempool.h>
#include <libcork/core/types.h>
#include <libcork/ds/dllist.h>


/*-----------------------------------------------------------------------
 * Hash tables
 */

struct cork_hash_table_entry {
    cork_hash  hash;
    void  *key;
    void  *value;
};


struct cork_hash_table;

CORK_API struct cork_hash_table *
cork_hash_table_new(size_t initial_size, unsigned int flags);

CORK_API void
cork_hash_table_free(struct cork_hash_table *table);


CORK_API void
cork_hash_table_set_user_data(struct cork_hash_table *table,
                              void *user_data, cork_free_f free_user_data);

CORK_API void
cork_hash_table_set_equals(struct cork_hash_table *table, cork_equals_f equals);

CORK_API void
cork_hash_table_set_free_key(struct cork_hash_table *table, cork_free_f free);

CORK_API void
cork_hash_table_set_free_value(struct cork_hash_table *table, cork_free_f free);

CORK_API void
cork_hash_table_set_hash(struct cork_hash_table *table, cork_hash_f hash);


CORK_API void
cork_hash_table_clear(struct cork_hash_table *table);


CORK_API void
cork_hash_table_ensure_size(struct cork_hash_table *table,
                            size_t desired_count);

CORK_API size_t
cork_hash_table_size(const struct cork_hash_table *table);


CORK_API void *
cork_hash_table_get(const struct cork_hash_table *table, const void *key);

CORK_API void *
cork_hash_table_get_hash(const struct cork_hash_table *table,
                         cork_hash hash, const void *key);

CORK_API struct cork_hash_table_entry *
cork_hash_table_get_entry(const struct cork_hash_table *table,
                          const void *key);

CORK_API struct cork_hash_table_entry *
cork_hash_table_get_entry_hash(const struct cork_hash_table *table,
                               cork_hash hash, const void *key);

CORK_API struct cork_hash_table_entry *
cork_hash_table_get_or_create(struct cork_hash_table *table,
                              void *key, bool *is_new);

CORK_API struct cork_hash_table_entry *
cork_hash_table_get_or_create_hash(struct cork_hash_table *table,
                                   cork_hash hash, void *key, bool *is_new);

CORK_API void
cork_hash_table_put(struct cork_hash_table *table,
                    void *key, void *value,
                    bool *is_new, void **old_key, void **old_value);

CORK_API void
cork_hash_table_put_hash(struct cork_hash_table *table,
                         cork_hash hash, void *key, void *value,
                         bool *is_new, void **old_key, void **old_value);

CORK_API void
cork_hash_table_delete_entry(struct cork_hash_table *table,
                             struct cork_hash_table_entry *entry);

CORK_API bool
cork_hash_table_delete(struct cork_hash_table *table, const void *key,
                       void **deleted_key, void **deleted_value);

CORK_API bool
cork_hash_table_delete_hash(struct cork_hash_table *table,
                            cork_hash hash, const void *key,
                            void **deleted_key, void **deleted_value);


enum cork_hash_table_map_result {
    /* Abort the current @ref cork_hash_table_map operation. */
    CORK_HASH_TABLE_MAP_ABORT = 0,
    /* Continue on to the next entry in the hash table. */
    CORK_HASH_TABLE_MAP_CONTINUE = 1,
    /* Delete the entry that was just processed, and then continue on to
     * the next entry in the hash table. */
    CORK_HASH_TABLE_MAP_DELETE = 2
};

typedef enum cork_hash_table_map_result
(*cork_hash_table_map_f)(void *user_data, struct cork_hash_table_entry *entry);

CORK_API void
cork_hash_table_map(struct cork_hash_table *table, void *user_data,
                    cork_hash_table_map_f mapper);


struct cork_hash_table_iterator {
    struct cork_hash_table  *table;
    void  *priv;
};

CORK_API void
cork_hash_table_iterator_init(struct cork_hash_table *table,
                              struct cork_hash_table_iterator *iterator);

CORK_API struct cork_hash_table_entry *
cork_hash_table_iterator_next(struct cork_hash_table_iterator *iterator);


/*-----------------------------------------------------------------------
 * Built-in key types
 */

CORK_API struct cork_hash_table *
cork_string_hash_table_new(size_t initial_size, unsigned int flags);

CORK_API struct cork_hash_table *
cork_pointer_hash_table_new(size_t initial_size, unsigned int flags);


#endif /* LIBCORK_DS_HASH_TABLE_H */
