/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libcork/core/allocator.h"
#include "libcork/core/attributes.h"
#include "libcork/core/error.h"
#include "libcork/core/types.h"
#include "libcork/os/process.h"


/*-----------------------------------------------------------------------
 * Allocator interface
 */

struct cork_alloc_priv {
    struct cork_alloc  public;
    struct cork_alloc_priv  *next;
};

static void *
cork_alloc__default_calloc(const struct cork_alloc *alloc,
                           size_t count, size_t size)
{
    void  *result = cork_alloc_xcalloc(alloc, count, size);
    if (CORK_UNLIKELY(result == NULL)) {
        abort();
    }
    return result;
}

static void *
cork_alloc__default_malloc(const struct cork_alloc *alloc, size_t size)
{
    void  *result = cork_alloc_xmalloc(alloc, size);
    if (CORK_UNLIKELY(result == NULL)) {
        abort();
    }
    return result;
}

static void *
cork_alloc__default_realloc(const struct cork_alloc *alloc, void *ptr,
                            size_t old_size, size_t new_size)
{
    void  *result = cork_alloc_xrealloc(alloc, ptr, old_size, new_size);
    if (CORK_UNLIKELY(result == NULL)) {
        abort();
    }
    return result;
}

static void *
cork_alloc__default_xcalloc(const struct cork_alloc *alloc,
                            size_t count, size_t size)
{
    void  *result;
    assert(count < (SIZE_MAX / size));
    result = cork_alloc_xmalloc(alloc, count * size);
    if (result != NULL) {
        memset(result, 0, count * size);
    }
    return result;
}

static void *
cork_alloc__default_xmalloc(const struct cork_alloc *alloc, size_t size)
{
    cork_abort("%s isn't defined", "cork_alloc:xmalloc");
}

static void *
cork_alloc__default_xrealloc(const struct cork_alloc *alloc, void *ptr,
                             size_t old_size, size_t new_size)
{
    void  *result = cork_alloc_xmalloc(alloc, new_size);
    if (CORK_LIKELY(result != NULL) && ptr != NULL) {
        size_t  min_size = (new_size < old_size)? new_size: old_size;
        memcpy(result, ptr, min_size);
        cork_alloc_free(alloc, ptr, old_size);
    }
    return result;
}

static void
cork_alloc__default_free(const struct cork_alloc *alloc, void *ptr, size_t size)
{
    cork_abort("%s isn't defined", "cork_alloc:free");
}

static bool  cleanup_registered = false;
static struct cork_alloc_priv  *all_allocs = NULL;

static void
cork_alloc_free_alloc(struct cork_alloc_priv *alloc)
{
    cork_free_user_data(&alloc->public);
    cork_alloc_delete(alloc->public.parent, struct cork_alloc_priv, alloc);
}

static void
cork_alloc_free_all(void)
{
    struct cork_alloc_priv  *curr;
    struct cork_alloc_priv  *next;
    for (curr = all_allocs; curr != NULL; curr = next) {
        next = curr->next;
        cork_alloc_free_alloc(curr);
    }
}

static void
cork_alloc_register_cleanup(void)
{
    if (CORK_UNLIKELY(!cleanup_registered)) {
        /* We don't use cork_cleanup because that requires the allocators to
         * have already been set up!  (atexit calls its functions in reverse
         * order, and this one will be registered before cork_cleanup's, which
         * makes it safe for cork_cleanup functions to still use the allocator,
         * since the allocator atexit function will be called last.) */
        atexit(cork_alloc_free_all);
        cleanup_registered = true;
    }
}

struct cork_alloc *
cork_alloc_new_alloc(const struct cork_alloc *parent)
{
    struct cork_alloc_priv  *alloc =
        cork_alloc_new(parent, struct cork_alloc_priv);
    alloc->public.parent = parent;
    alloc->public.user_data = NULL;
    alloc->public.free_user_data = NULL;
    alloc->public.calloc = cork_alloc__default_calloc;
    alloc->public.malloc = cork_alloc__default_malloc;
    alloc->public.realloc = cork_alloc__default_realloc;
    alloc->public.xcalloc = cork_alloc__default_xcalloc;
    alloc->public.xmalloc = cork_alloc__default_xmalloc;
    alloc->public.xrealloc = cork_alloc__default_xrealloc;
    alloc->public.free = cork_alloc__default_free;

    cork_alloc_register_cleanup();
    alloc->next = all_allocs;
    all_allocs = alloc;

    return &alloc->public;
}


void
cork_alloc_set_user_data(struct cork_alloc *alloc,
                         void *user_data, cork_free_f free_user_data)
{
    cork_free_user_data(alloc);
    alloc->user_data = user_data;
    alloc->free_user_data = free_user_data;
}

void
cork_alloc_set_calloc(struct cork_alloc *alloc, cork_alloc_calloc_f calloc)
{
    alloc->calloc = calloc;
}

void
cork_alloc_set_malloc(struct cork_alloc *alloc, cork_alloc_malloc_f malloc)
{
    alloc->malloc = malloc;
}

void
cork_alloc_set_realloc(struct cork_alloc *alloc, cork_alloc_realloc_f realloc)
{
    alloc->realloc = realloc;
}

void
cork_alloc_set_xcalloc(struct cork_alloc *alloc, cork_alloc_calloc_f xcalloc)
{
    alloc->xcalloc = xcalloc;
}

void
cork_alloc_set_xmalloc(struct cork_alloc *alloc, cork_alloc_malloc_f xmalloc)
{
    alloc->xmalloc = xmalloc;
}

void
cork_alloc_set_xrealloc(struct cork_alloc *alloc,
                        cork_alloc_realloc_f xrealloc)
{
    alloc->xrealloc = xrealloc;
}

void
cork_alloc_set_free(struct cork_alloc *alloc, cork_alloc_free_f free)
{
    alloc->free = free;
}


/*-----------------------------------------------------------------------
 * Allocating strings
 */

static inline const char *
strndup_internal(const struct cork_alloc *alloc,
                 const char *str, size_t len)
{
    char  *dest;
    size_t  allocated_size = len + sizeof(size_t) + 1;
    size_t  *new_str = cork_alloc_malloc(alloc, allocated_size);
    *new_str = allocated_size;
    dest = (char *) (void *) (new_str + 1);
    memcpy(dest, str, len);
    dest[len] = '\0';
    return dest;
}

const char *
cork_alloc_strdup(const struct cork_alloc *alloc, const char *str)
{
    return strndup_internal(alloc, str, strlen(str));
}

const char *
cork_alloc_strndup(const struct cork_alloc *alloc,
                   const char *str, size_t size)
{
    return strndup_internal(alloc, str, size);
}

static inline const char *
xstrndup_internal(const struct cork_alloc *alloc,
                  const char *str, size_t len)
{
    size_t  allocated_size = len + sizeof(size_t) + 1;
    size_t  *new_str = cork_alloc_xmalloc(alloc, allocated_size);
    if (CORK_UNLIKELY(new_str == NULL)) {
        return NULL;
    } else {
        char  *dest;
        *new_str = allocated_size;
        dest = (char *) (void *) (new_str + 1);
        memcpy(dest, str, len);
        dest[len] = '\0';
        return dest;
    }
}

const char *
cork_alloc_xstrdup(const struct cork_alloc *alloc, const char *str)
{
    return xstrndup_internal(alloc, str, strlen(str));
}

const char *
cork_alloc_xstrndup(const struct cork_alloc *alloc,
                    const char *str, size_t size)
{
    return xstrndup_internal(alloc, str, size);
}

void
cork_alloc_strfree(const struct cork_alloc *alloc, const char *str)
{
    size_t  *base = ((size_t *) str) - 1;
    cork_alloc_free(alloc, base, *base);
}


/*-----------------------------------------------------------------------
 * stdlib allocator
 */

static void *
cork_stdlib_alloc__calloc(const struct cork_alloc *alloc,
                          size_t count, size_t size)
{
    void  *result = calloc(count, size);
    if (CORK_UNLIKELY(result == NULL)) {
        abort();
    }
    return result;
}

static void *
cork_stdlib_alloc__malloc(const struct cork_alloc *alloc, size_t size)
{
    void  *result = malloc(size);
    if (CORK_UNLIKELY(result == NULL)) {
        abort();
    }
    return result;
}

static void *
cork_stdlib_alloc__realloc(const struct cork_alloc *alloc, void *ptr,
                           size_t old_size, size_t new_size)
{
    /* Technically we don't really need to free `ptr` if the reallocation fails,
     * since we'll abort the process immediately after.  But my sense of
     * cleanliness makes me do it anyway. */

#if CORK_HAVE_REALLOCF
    void  *result = reallocf(ptr, new_size);
    if (result == NULL) {
        abort();
    }
    return result;
#else
    void  *result = realloc(ptr, new_size);
    if (result == NULL) {
        free(ptr);
        abort();
    }
    return result;
#endif
}

static void *
cork_stdlib_alloc__xcalloc(const struct cork_alloc *alloc,
                           size_t count, size_t size)
{
    return calloc(count, size);
}

static void *
cork_stdlib_alloc__xmalloc(const struct cork_alloc *alloc, size_t size)
{
    return malloc(size);
}

static void *
cork_stdlib_alloc__xrealloc(const struct cork_alloc *alloc, void *ptr,
                            size_t old_size, size_t new_size)
{
    return realloc(ptr, new_size);
}

static void
cork_stdlib_alloc__free(const struct cork_alloc *alloc, void *ptr, size_t size)
{
    free(ptr);
}


static const struct cork_alloc  default_allocator = {
    NULL,
    NULL,
    NULL,
    cork_stdlib_alloc__calloc,
    cork_stdlib_alloc__malloc,
    cork_stdlib_alloc__realloc,
    cork_stdlib_alloc__xcalloc,
    cork_stdlib_alloc__xmalloc,
    cork_stdlib_alloc__xrealloc,
    cork_stdlib_alloc__free
};


/*-----------------------------------------------------------------------
 * Customizing libcork's allocator
 */

const struct cork_alloc  *cork_allocator = &default_allocator;

void
cork_set_allocator(const struct cork_alloc *alloc)
{
    cork_allocator = alloc;
}


/*-----------------------------------------------------------------------
 * Debugging allocator
 */

static void *
cork_debug_alloc__xmalloc(const struct cork_alloc *alloc, size_t size)
{
    size_t  real_size = size + sizeof(size_t);
    size_t  *base = cork_alloc_xmalloc(alloc->parent, real_size);
    *base = size;
    return base + 1;
}

static void
cork_debug_alloc__free(const struct cork_alloc *alloc, void *ptr,
                       size_t expected_size)
{
    size_t  *base = ((size_t *) ptr) - 1;
    size_t  actual_size = *base;
    size_t  real_size = actual_size + sizeof(size_t);
    if (CORK_UNLIKELY(actual_size != expected_size)) {
        cork_abort
            ("Incorrect size when freeing pointer (got %zu, expected %zu)",
             expected_size, actual_size);
    }
    cork_alloc_free(alloc->parent, base, real_size);
}

struct cork_alloc *
cork_debug_alloc_new(const struct cork_alloc *parent)
{
    struct cork_alloc  *debug = cork_alloc_new_alloc(parent);
    cork_alloc_set_xmalloc(debug, cork_debug_alloc__xmalloc);
    cork_alloc_set_free(debug, cork_debug_alloc__free);
    return debug;
}
