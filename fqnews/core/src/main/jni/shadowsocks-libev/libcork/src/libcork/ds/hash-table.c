/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>

#include "libcork/core/callbacks.h"
#include "libcork/core/hash.h"
#include "libcork/core/types.h"
#include "libcork/ds/dllist.h"
#include "libcork/ds/hash-table.h"
#include "libcork/helpers/errors.h"

#ifndef CORK_HASH_TABLE_DEBUG
#define CORK_HASH_TABLE_DEBUG 0
#endif

#if CORK_HASH_TABLE_DEBUG
#include <stdio.h>
#define DEBUG(...) \
    do { \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)
#else
#define DEBUG(...) /* nothing */
#endif


/*-----------------------------------------------------------------------
 * Hash tables
 */

struct cork_hash_table_entry_priv {
    struct cork_hash_table_entry  public;
    struct cork_dllist_item  in_bucket;
    struct cork_dllist_item  insertion_order;
};

struct cork_hash_table {
    struct cork_dllist  *bins;
    struct cork_dllist  insertion_order;
    size_t  bin_count;
    size_t  bin_mask;
    size_t  entry_count;
    void  *user_data;
    cork_free_f  free_user_data;
    cork_hash_f  hash;
    cork_equals_f  equals;
    cork_free_f  free_key;
    cork_free_f  free_value;
};

static cork_hash
cork_hash_table__default_hash(void *user_data, const void *key)
{
    return (cork_hash) (uintptr_t) key;
}

static bool
cork_hash_table__default_equals(void *user_data,
                                const void *key1, const void *key2)
{
    return key1 == key2;
}


/* The default initial number of bins to allocate in a new table. */
#define CORK_HASH_TABLE_DEFAULT_INITIAL_SIZE  8

/* The default number of entries per bin to allow before increasing the
 * number of bins. */
#define CORK_HASH_TABLE_MAX_DENSITY  5

/* Return a power-of-2 bin count that's at least as big as the given requested
 * size. */
static inline size_t
cork_hash_table_new_size(size_t desired_count)
{
    size_t  v = desired_count;
    size_t  r = 1;
    while (v >>= 1) {
        r <<= 1;
    }
    if (r != desired_count) {
        r <<= 1;
    }
    return r;
}

#define bin_index(table, hash)  ((hash) & (table)->bin_mask)

/* Allocates a new bins array in a hash table.  We overwrite the old
 * array, so make sure to stash it away somewhere safe first. */
static void
cork_hash_table_allocate_bins(struct cork_hash_table *table,
                              size_t desired_count)
{
    size_t  i;

    table->bin_count = cork_hash_table_new_size(desired_count);
    table->bin_mask = table->bin_count - 1;
    DEBUG("Allocate %zu bins", table->bin_count);
    table->bins = cork_calloc(table->bin_count, sizeof(struct cork_dllist));
    for (i = 0; i < table->bin_count; i++) {
        cork_dllist_init(&table->bins[i]);
    }
}


static struct cork_hash_table_entry_priv *
cork_hash_table_new_entry(struct cork_hash_table *table,
                          cork_hash hash, void *key, void *value)
{
    struct cork_hash_table_entry_priv  *entry =
        cork_new(struct cork_hash_table_entry_priv);
    cork_dllist_add(&table->insertion_order, &entry->insertion_order);
    entry->public.hash = hash;
    entry->public.key = key;
    entry->public.value = value;
    return entry;
}

static void
cork_hash_table_free_entry(struct cork_hash_table *table,
                           struct cork_hash_table_entry_priv *entry)
{
    if (table->free_key != NULL) {
        table->free_key(entry->public.key);
    }
    if (table->free_value != NULL) {
        table->free_value(entry->public.value);
    }
    cork_dllist_remove(&entry->insertion_order);
    cork_delete(struct cork_hash_table_entry_priv, entry);
}


struct cork_hash_table *
cork_hash_table_new(size_t initial_size, unsigned int flags)
{
    struct cork_hash_table  *table = cork_new(struct cork_hash_table);
    table->entry_count = 0;
    table->user_data = NULL;
    table->free_user_data = NULL;
    table->hash = cork_hash_table__default_hash;
    table->equals = cork_hash_table__default_equals;
    table->free_key = NULL;
    table->free_value = NULL;
    cork_dllist_init(&table->insertion_order);
    if (initial_size < CORK_HASH_TABLE_DEFAULT_INITIAL_SIZE) {
        initial_size = CORK_HASH_TABLE_DEFAULT_INITIAL_SIZE;
    }
    cork_hash_table_allocate_bins(table, initial_size);
    return table;
}

void
cork_hash_table_clear(struct cork_hash_table *table)
{
    size_t  i;
    struct cork_dllist_item  *curr;
    struct cork_dllist_item  *next;

    DEBUG("(clear) Remove all entries");
    for (curr = cork_dllist_start(&table->insertion_order);
         !cork_dllist_is_end(&table->insertion_order, curr);
         curr = next) {
        struct cork_hash_table_entry_priv  *entry =
            cork_container_of
            (curr, struct cork_hash_table_entry_priv, insertion_order);
        next = curr->next;
        cork_hash_table_free_entry(table, entry);
    }
    cork_dllist_init(&table->insertion_order);

    DEBUG("(clear) Clear bins");
    for (i = 0; i < table->bin_count; i++) {
        DEBUG("  Bin %zu", i);
        cork_dllist_init(&table->bins[i]);
    }

    table->entry_count = 0;
}

void
cork_hash_table_free(struct cork_hash_table *table)
{
    cork_hash_table_clear(table);
    cork_cfree(table->bins, table->bin_count, sizeof(struct cork_dllist));
    cork_delete(struct cork_hash_table, table);
}

size_t
cork_hash_table_size(const struct cork_hash_table *table)
{
    return table->entry_count;
}

void
cork_hash_table_set_user_data(struct cork_hash_table *table,
                              void *user_data, cork_free_f free_user_data)
{
    table->user_data = user_data;
    table->free_user_data = free_user_data;
}

void
cork_hash_table_set_hash(struct cork_hash_table *table, cork_hash_f hash)
{
    table->hash = hash;
}

void
cork_hash_table_set_equals(struct cork_hash_table *table, cork_equals_f equals)
{
    table->equals = equals;
}

void
cork_hash_table_set_free_key(struct cork_hash_table *table, cork_free_f free)
{
    table->free_key = free;
}

void
cork_hash_table_set_free_value(struct cork_hash_table *table, cork_free_f free)
{
    table->free_value = free;
}


void
cork_hash_table_ensure_size(struct cork_hash_table *table, size_t desired_count)
{
    if (desired_count > table->bin_count) {
        struct cork_dllist  *old_bins = table->bins;
        size_t  old_bin_count = table->bin_count;

        cork_hash_table_allocate_bins(table, desired_count);

        if (old_bins != NULL) {
            size_t  i;
            for (i = 0; i < old_bin_count; i++) {
                struct cork_dllist  *bin = &old_bins[i];
                struct cork_dllist_item  *curr = cork_dllist_start(bin);
                while (!cork_dllist_is_end(bin, curr)) {
                    struct cork_hash_table_entry_priv  *entry =
                        cork_container_of
                        (curr, struct cork_hash_table_entry_priv, in_bucket);
                    struct cork_dllist_item  *next = curr->next;

                    size_t  bin_index = bin_index(table, entry->public.hash);
                    DEBUG("      Rehash %p from bin %zu to bin %zu",
                          entry, i, bin_index);
                    cork_dllist_add(&table->bins[bin_index], curr);

                    curr = next;
                }
            }

            cork_cfree(old_bins, old_bin_count, sizeof(struct cork_dllist));
        }
    }
}


static void
cork_hash_table_rehash(struct cork_hash_table *table)
{
    DEBUG("    Reached maximum density; rehash");
    cork_hash_table_ensure_size(table, table->bin_count + 1);
}


struct cork_hash_table_entry *
cork_hash_table_get_entry_hash(const struct cork_hash_table *table,
                               cork_hash hash, const void *key)
{
    size_t  bin_index;
    struct cork_dllist  *bin;
    struct cork_dllist_item  *curr;

    if (table->bin_count == 0) {
        DEBUG("(get) Empty table when searching for key %p "
              "(hash 0x%08" PRIx32 ")",
              key, hash);
        return NULL;
    }

    bin_index = bin_index(table, hash);
    DEBUG("(get) Search for key %p (hash 0x%08" PRIx32 ", bin %zu)",
          key, hash, bin_index);

    bin = &table->bins[bin_index];
    curr = cork_dllist_start(bin);
    while (!cork_dllist_is_end(bin, curr)) {
        struct cork_hash_table_entry_priv  *entry =
            cork_container_of
            (curr, struct cork_hash_table_entry_priv, in_bucket);

        DEBUG("  Check entry %p", entry);
        if (table->equals(table->user_data, key, entry->public.key)) {
            DEBUG("  Match");
            return &entry->public;
        }

        curr = curr->next;
    }

    DEBUG("  Entry not found");
    return NULL;
}

struct cork_hash_table_entry *
cork_hash_table_get_entry(const struct cork_hash_table *table, const void *key)
{
    cork_hash  hash = table->hash(table->user_data, key);
    return cork_hash_table_get_entry_hash(table, hash, key);
}

void *
cork_hash_table_get_hash(const struct cork_hash_table *table,
                         cork_hash hash, const void *key)
{
    struct cork_hash_table_entry  *entry =
        cork_hash_table_get_entry_hash(table, hash, key);
    if (entry == NULL) {
        return NULL;
    } else {
        DEBUG("  Extract value pointer %p", entry->value);
        return entry->value;
    }
}

void *
cork_hash_table_get(const struct cork_hash_table *table, const void *key)
{
    struct cork_hash_table_entry  *entry =
        cork_hash_table_get_entry(table, key);
    if (entry == NULL) {
        return NULL;
    } else {
        DEBUG("  Extract value pointer %p", entry->value);
        return entry->value;
    }
}


struct cork_hash_table_entry *
cork_hash_table_get_or_create_hash(struct cork_hash_table *table,
                                   cork_hash hash, void *key, bool *is_new)
{
    struct cork_hash_table_entry_priv  *entry;
    size_t  bin_index;

    if (table->bin_count > 0) {
        struct cork_dllist  *bin;
        struct cork_dllist_item  *curr;

        bin_index = bin_index(table, hash);
        DEBUG("(get_or_create) Search for key %p "
              "(hash 0x%08" PRIx32 ", bin %zu)",
              key, hash, bin_index);

        bin = &table->bins[bin_index];
        curr = cork_dllist_start(bin);
        while (!cork_dllist_is_end(bin, curr)) {
            struct cork_hash_table_entry_priv  *entry =
                cork_container_of
                (curr, struct cork_hash_table_entry_priv, in_bucket);

            DEBUG("  Check entry %p", entry);
            if (table->equals(table->user_data, key, entry->public.key)) {
                DEBUG("    Match");
                DEBUG("    Return value pointer %p", entry->public.value);
                *is_new = false;
                return &entry->public;
            }

            curr = curr->next;
        }

        /* create a new entry */
        DEBUG("  Entry not found");

        if ((table->entry_count / table->bin_count) >
            CORK_HASH_TABLE_MAX_DENSITY) {
            cork_hash_table_rehash(table);
            bin_index = bin_index(table, hash);
        }
    } else {
        DEBUG("(get_or_create) Search for key %p (hash 0x%08" PRIx32 ")",
              key, hash);
        DEBUG("  Empty table");
        cork_hash_table_rehash(table);
        bin_index = bin_index(table, hash);
    }

    DEBUG("    Allocate new entry");
    entry = cork_hash_table_new_entry(table, hash, key, NULL);
    DEBUG("    Created new entry %p", entry);

    DEBUG("    Add entry into bin %zu", bin_index);
    cork_dllist_add(&table->bins[bin_index], &entry->in_bucket);

    table->entry_count++;
    *is_new = true;
    return &entry->public;
}

struct cork_hash_table_entry *
cork_hash_table_get_or_create(struct cork_hash_table *table,
                              void *key, bool *is_new)
{
    cork_hash  hash = table->hash(table->user_data, key);
    return cork_hash_table_get_or_create_hash(table, hash, key, is_new);
}


void
cork_hash_table_put_hash(struct cork_hash_table *table,
                         cork_hash hash, void *key, void *value,
                         bool *is_new, void **old_key, void **old_value)
{
    struct cork_hash_table_entry_priv  *entry;
    size_t  bin_index;

    if (table->bin_count > 0) {
        struct cork_dllist  *bin;
        struct cork_dllist_item  *curr;

        bin_index = bin_index(table, hash);
        DEBUG("(put) Search for key %p (hash 0x%08" PRIx32 ", bin %zu)",
              key, hash, bin_index);

        bin = &table->bins[bin_index];
        curr = cork_dllist_start(bin);
        while (!cork_dllist_is_end(bin, curr)) {
            struct cork_hash_table_entry_priv  *entry =
                cork_container_of
                (curr, struct cork_hash_table_entry_priv, in_bucket);

            DEBUG("  Check entry %p", entry);
            if (table->equals(table->user_data, key, entry->public.key)) {
                DEBUG("    Found existing entry; overwriting");
                DEBUG("    Return old key %p", entry->public.key);
                if (old_key != NULL) {
                    *old_key = entry->public.key;
                }
                DEBUG("    Return old value %p", entry->public.value);
                if (old_value != NULL) {
                    *old_value = entry->public.value;
                }
                DEBUG("    Copy key %p into entry", key);
                entry->public.key = key;
                DEBUG("    Copy value %p into entry", value);
                entry->public.value = value;
                if (is_new != NULL) {
                    *is_new = false;
                }
                return;
            }

            curr = curr->next;
        }

        /* create a new entry */
        DEBUG("  Entry not found");
        if ((table->entry_count / table->bin_count) >
            CORK_HASH_TABLE_MAX_DENSITY) {
            cork_hash_table_rehash(table);
            bin_index = bin_index(table, hash);
        }
    } else {
        DEBUG("(put) Search for key %p (hash 0x%08" PRIx32 ")",
              key, hash);
        DEBUG("  Empty table");
        cork_hash_table_rehash(table);
        bin_index = bin_index(table, hash);
    }

    DEBUG("    Allocate new entry");
    entry = cork_hash_table_new_entry(table, hash, key, value);
    DEBUG("    Created new entry %p", entry);

    DEBUG("    Add entry into bin %zu", bin_index);
    cork_dllist_add(&table->bins[bin_index], &entry->in_bucket);

    table->entry_count++;
    if (old_key != NULL) {
        *old_key = NULL;
    }
    if (old_value != NULL) {
        *old_value = NULL;
    }
    if (is_new != NULL) {
        *is_new = true;
    }
}

void
cork_hash_table_put(struct cork_hash_table *table,
                    void *key, void *value,
                    bool *is_new, void **old_key, void **old_value)
{
    cork_hash  hash = table->hash(table->user_data, key);
    cork_hash_table_put_hash
        (table, hash, key, value, is_new, old_key, old_value);
}


void
cork_hash_table_delete_entry(struct cork_hash_table *table,
                             struct cork_hash_table_entry *ventry)
{
    struct cork_hash_table_entry_priv  *entry =
        cork_container_of(ventry, struct cork_hash_table_entry_priv, public);
    cork_dllist_remove(&entry->in_bucket);
    table->entry_count--;
    cork_hash_table_free_entry(table, entry);
}


bool
cork_hash_table_delete_hash(struct cork_hash_table *table,
                            cork_hash hash, const void *key,
                            void **deleted_key, void **deleted_value)
{
    size_t  bin_index;
    struct cork_dllist  *bin;
    struct cork_dllist_item  *curr;

    if (table->bin_count == 0) {
        DEBUG("(delete) Empty table when searching for key %p "
              "(hash 0x%08" PRIx32 ")",
              key, hash);
        return false;
    }

    bin_index = bin_index(table, hash);
    DEBUG("(delete) Search for key %p (hash 0x%08" PRIx32 ", bin %zu)",
          key, hash, bin_index);

    bin = &table->bins[bin_index];
    curr = cork_dllist_start(bin);
    while (!cork_dllist_is_end(bin, curr)) {
        struct cork_hash_table_entry_priv  *entry =
            cork_container_of
            (curr, struct cork_hash_table_entry_priv, in_bucket);

        DEBUG("  Check entry %p", entry);
        if (table->equals(table->user_data, key, entry->public.key)) {
            DEBUG("    Match");
            if (deleted_key != NULL) {
                *deleted_key = entry->public.key;
            }
            if (deleted_value != NULL) {
                *deleted_value = entry->public.value;
            }

            DEBUG("    Remove entry from hash bin %zu", bin_index);
            cork_dllist_remove(curr);
            table->entry_count--;

            DEBUG("    Free entry %p", entry);
            cork_hash_table_free_entry(table, entry);
            return true;
        }

        curr = curr->next;
    }

    DEBUG("  Entry not found");
    return false;
}

bool
cork_hash_table_delete(struct cork_hash_table *table, const void *key,
                       void **deleted_key, void **deleted_value)
{
    cork_hash  hash = table->hash(table->user_data, key);
    return cork_hash_table_delete_hash
        (table, hash, key, deleted_key, deleted_value);
}


void
cork_hash_table_map(struct cork_hash_table *table, void *user_data,
                    cork_hash_table_map_f map)
{
    struct cork_dllist_item  *curr;
    DEBUG("Map across hash table");

    curr = cork_dllist_start(&table->insertion_order);
    while (!cork_dllist_is_end(&table->insertion_order, curr)) {
        struct cork_hash_table_entry_priv  *entry =
            cork_container_of
            (curr, struct cork_hash_table_entry_priv, insertion_order);
        struct cork_dllist_item  *next = curr->next;
        enum cork_hash_table_map_result  result;

        DEBUG("    Apply function to entry %p", entry);
        result = map(user_data, &entry->public);

        if (result == CORK_HASH_TABLE_MAP_ABORT) {
            return;
        } else if (result == CORK_HASH_TABLE_MAP_DELETE) {
            DEBUG("      Delete requested");
            cork_dllist_remove(curr);
            cork_dllist_remove(&entry->in_bucket);
            table->entry_count--;
            cork_hash_table_free_entry(table, entry);
        }

        curr = next;
    }
}


void
cork_hash_table_iterator_init(struct cork_hash_table *table,
                              struct cork_hash_table_iterator *iterator)
{
    DEBUG("Iterate through hash table");
    iterator->table = table;
    iterator->priv = cork_dllist_start(&table->insertion_order);
}


struct cork_hash_table_entry *
cork_hash_table_iterator_next(struct cork_hash_table_iterator *iterator)
{
    struct cork_hash_table  *table = iterator->table;
    struct cork_dllist_item  *curr = iterator->priv;
    struct cork_hash_table_entry_priv  *entry;

    if (cork_dllist_is_end(&table->insertion_order, curr)) {
        return NULL;
    }

    entry = cork_container_of
        (curr, struct cork_hash_table_entry_priv, insertion_order);
    DEBUG("    Return entry %p", entry);
    iterator->priv = curr->next;
    return &entry->public;
}


/*-----------------------------------------------------------------------
 * Built-in key types
 */

static cork_hash
string_hash(void *user_data, const void *vk)
{
    const char  *k = vk;
    size_t  len = strlen(k);
    return cork_hash_buffer(0, k, len);
}

static bool
string_equals(void *user_data, const void *vk1, const void *vk2)
{
    const char  *k1 = vk1;
    const char  *k2 = vk2;
    return strcmp(k1, k2) == 0;
}

struct cork_hash_table *
cork_string_hash_table_new(size_t initial_size, unsigned int flags)
{
    struct cork_hash_table  *table = cork_hash_table_new(initial_size, flags);
    cork_hash_table_set_hash(table, string_hash);
    cork_hash_table_set_equals(table, string_equals);
    return table;
}

struct cork_hash_table *
cork_pointer_hash_table_new(size_t initial_size, unsigned int flags)
{
    return cork_hash_table_new(initial_size, flags);
}
