/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>

#include "libcork/core.h"
#include "libcork/ds.h"
#include "libcork/os/process.h"
#include "libcork/helpers/errors.h"


#if !defined(CORK_DEBUG_PROCESS)
#define CORK_DEBUG_PROCESS  0
#endif

#if CORK_DEBUG_PROCESS
#include <stdio.h>
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...) /* no debug messages */
#endif


struct cork_cleanup_entry {
    struct cork_dllist_item  item;
    int  priority;
    const char  *name;
    cork_cleanup_function  function;
};

static struct cork_cleanup_entry *
cork_cleanup_entry_new(const char *name, int priority,
                       cork_cleanup_function function)
{
    struct cork_cleanup_entry  *self = cork_new(struct cork_cleanup_entry);
    self->priority = priority;
    self->name = cork_strdup(name);
    self->function = function;
    return self;
}

static void
cork_cleanup_entry_free(struct cork_cleanup_entry *self)
{
    cork_strfree(self->name);
    cork_delete(struct cork_cleanup_entry, self);
}

static struct cork_dllist  cleanup_entries = CORK_DLLIST_INIT(cleanup_entries);
static bool  cleanup_registered = false;

static void
cork_cleanup_call_one(struct cork_dllist_item *item, void *user_data)
{
    struct cork_cleanup_entry  *entry =
        cork_container_of(item, struct cork_cleanup_entry, item);
    cork_cleanup_function  function = entry->function;
    DEBUG("Call cleanup function [%d] %s\n", entry->priority, entry->name);
    /* We need to free the entry before calling the entry's function, since one
     * of the functions that libcork registers frees the allocator instance that
     * we'd use to free the entry.  If we called the function first, the
     * allocator would be freed before we could use it to free the entry. */
    cork_cleanup_entry_free(entry);
    function();
}

static void
cork_cleanup_call_all(void)
{
    cork_dllist_map(&cleanup_entries, cork_cleanup_call_one, NULL);
}

static void
cork_cleanup_entry_add(struct cork_cleanup_entry *entry)
{
    struct cork_dllist_item  *curr;

    if (CORK_UNLIKELY(!cleanup_registered)) {
        atexit(cork_cleanup_call_all);
        cleanup_registered = true;
    }

    /* Linear search through the list of existing cleanup functions.  When we
     * find the first existing function with a higher priority, we've found
     * where to insert the new function. */
    for (curr = cork_dllist_start(&cleanup_entries);
         !cork_dllist_is_end(&cleanup_entries, curr); curr = curr->next) {
        struct cork_cleanup_entry  *existing =
            cork_container_of(curr, struct cork_cleanup_entry, item);
        if (existing->priority > entry->priority) {
            cork_dllist_add_before(&existing->item, &entry->item);
            return;
        }
    }

    /* If we fall through the loop, then the new function should be appended to
     * the end of the list. */
    cork_dllist_add(&cleanup_entries, &entry->item);
}


CORK_API void
cork_cleanup_at_exit_named(const char *name, int priority,
                           cork_cleanup_function function)
{
    struct cork_cleanup_entry  *entry =
        cork_cleanup_entry_new(name, priority, function);
    DEBUG("Register cleanup function [%d] %s\n", priority, name);
    cork_cleanup_entry_add(entry);
}
