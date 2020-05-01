/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_ALLOCATOR_H
#define LIBCORK_CORE_ALLOCATOR_H

#include <assert.h>
#include <stdlib.h>

#include <libcork/core/api.h>
#include <libcork/core/attributes.h>
#include <libcork/core/callbacks.h>
#include <libcork/core/error.h>
#include <libcork/core/types.h>


/*-----------------------------------------------------------------------
 * Allocator interface
 */

struct cork_alloc;

typedef void *
(*cork_alloc_calloc_f)(const struct cork_alloc *alloc,
                       size_t count, size_t size);

typedef void *
(*cork_alloc_malloc_f)(const struct cork_alloc *alloc, size_t size);

/* Must not free `ptr` if allocation fails. */
typedef void *
(*cork_alloc_realloc_f)(const struct cork_alloc *alloc, void *ptr,
                        size_t old_size, size_t new_size);

typedef void
(*cork_alloc_free_f)(const struct cork_alloc *alloc, void *ptr, size_t size);

struct cork_alloc {
    const struct cork_alloc  *parent;
    void  *user_data;
    cork_free_f  free_user_data;
    cork_alloc_calloc_f  calloc;
    cork_alloc_malloc_f  malloc;
    cork_alloc_realloc_f  realloc;
    cork_alloc_calloc_f  xcalloc;
    cork_alloc_malloc_f  xmalloc;
    cork_alloc_realloc_f  xrealloc;
    cork_alloc_free_f  free;
};

/* NOT thread-safe; must be called before most other libcork functions.
 * Allocator will automatically be freed at process exit. */
CORK_API struct cork_alloc *
cork_alloc_new_alloc(const struct cork_alloc *parent);


CORK_API void
cork_alloc_set_user_data(struct cork_alloc *alloc,
                         void *user_data, cork_free_f free_user_data);

/* These variants must always return a valid pointer.  If allocation fails, they
 * should abort the process or transfer control in some other way to an error
 * handler or cleanup routine.
 *
 * If you only provide implementations of the `x` variants, we'll provide
 * default implementations of these that abort the process if a memory
 * allocation fails. */

CORK_API void
cork_alloc_set_calloc(struct cork_alloc *alloc, cork_alloc_calloc_f calloc);

CORK_API void
cork_alloc_set_malloc(struct cork_alloc *alloc, cork_alloc_malloc_f malloc);

CORK_API void
cork_alloc_set_realloc(struct cork_alloc *alloc, cork_alloc_realloc_f realloc);

/* These variants can return a NULL pointer if allocation fails. */

CORK_API void
cork_alloc_set_xcalloc(struct cork_alloc *alloc, cork_alloc_calloc_f xcalloc);

CORK_API void
cork_alloc_set_xmalloc(struct cork_alloc *alloc, cork_alloc_malloc_f xmalloc);

CORK_API void
cork_alloc_set_xrealloc(struct cork_alloc *alloc,
                        cork_alloc_realloc_f xrealloc);


CORK_API void
cork_alloc_set_free(struct cork_alloc *alloc, cork_alloc_free_f free);


/* Low-level use of an allocator. */

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_alloc_calloc(const struct cork_alloc *alloc, size_t count, size_t size)
{
    return alloc->calloc(alloc, count, size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_alloc_malloc(const struct cork_alloc *alloc, size_t size)
{
    return alloc->malloc(alloc, size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_alloc_realloc(const struct cork_alloc *alloc, void *ptr,
                   size_t old_size, size_t new_size)
{
    return alloc->realloc(alloc, ptr, old_size, new_size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_alloc_xcalloc(const struct cork_alloc *alloc, size_t count, size_t size)
{
    return alloc->xcalloc(alloc, count, size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_alloc_xmalloc(const struct cork_alloc *alloc, size_t size)
{
    return alloc->xmalloc(alloc, size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_alloc_xrealloc(const struct cork_alloc *alloc, void *ptr,
                    size_t old_size, size_t new_size)
{
    return alloc->xrealloc(alloc, ptr, old_size, new_size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_alloc_xreallocf(const struct cork_alloc *alloc, void *ptr,
                     size_t old_size, size_t new_size)
{
    void  *result = alloc->xrealloc(alloc, ptr, old_size, new_size);
    if (result == NULL) {
        alloc->free(alloc, ptr, old_size);
        return NULL;
    } else {
        return result;
    }
}

CORK_ATTR_UNUSED
static void
cork_alloc_free(const struct cork_alloc *alloc, void *ptr, size_t size)
{
    return alloc->free(alloc, ptr, size);
}

CORK_ATTR_UNUSED
static void
cork_alloc_cfree(const struct cork_alloc *alloc, void *ptr,
                 size_t count, size_t size)
{
    assert(count < (SIZE_MAX / size));
    return alloc->free(alloc, ptr, count * size);
}

#define cork_alloc_new(alloc, type) \
    cork_alloc_malloc((alloc), sizeof(type))
#define cork_alloc_xnew(alloc, type) \
    cork_alloc_xmalloc((alloc), sizeof(type))
#define cork_alloc_delete(alloc, type, ptr) \
    cork_alloc_free((alloc), (ptr), sizeof(type))

/* string-related helper functions */

CORK_ATTR_MALLOC
CORK_API const char *
cork_alloc_strdup(const struct cork_alloc *alloc, const char *str);

CORK_ATTR_MALLOC
CORK_API const char *
cork_alloc_strndup(const struct cork_alloc *alloc,
                   const char *str, size_t size);

CORK_ATTR_MALLOC
CORK_API const char *
cork_alloc_xstrdup(const struct cork_alloc *alloc, const char *str);

CORK_ATTR_MALLOC
CORK_API const char *
cork_alloc_xstrndup(const struct cork_alloc *alloc,
                    const char *str, size_t size);

CORK_API void
cork_alloc_strfree(const struct cork_alloc *alloc, const char *str);


/*-----------------------------------------------------------------------
 * Using the allocator interface
 */

/* All of the functions that you use to actually allocate memory assume that
 * cork_current_allocator() returns the allocator instance that should be used.
 * Your easiest approach is to do nothing special; in that case, all of the
 * libcork memory allocation functions will transparently use the standard
 * malloc/free family of functions.
 *
 * If you're writing a library, and want to allow your library clients to
 * provide a separate custom memory allocator then the one they can already
 * override for libcork itself, you should declare a pair of functions for
 * getting and setting your library's current allocator (like libcork itself
 * does), and (only when compiling the source of your library) define
 * `cork_current_allocator` as a macro that aliases the getter function.  That
 * will cause the libcork memory allocation functions to use whichever allocator
 * your library user has provided.
 *
 * If you're writing an application, and want to provide a single allocator that
 * all libcork-using libraries will pick up, just call cork_set_allocator before
 * calling any other library functions.  Other libraries will use this as a
 * default and everything that uses libcork's memory allocation functions will
 * use your custom allocator. */


/* libcork's current allocator */

extern const struct cork_alloc  *cork_allocator;

/* We take control and will free when the process exits.  This is *NOT*
 * thread-safe; it's only safe to call before you've called *ANY* other libcork
 * function (or any function from any other library that uses libcork).  You can
 * only call this at most once. */
CORK_API void
cork_set_allocator(const struct cork_alloc *alloc);


/* The current allocator of whichever library is being compiled. */

#if !defined(cork_current_allocator)
#define cork_current_allocator()  (cork_allocator)
#endif


/* using an allocator */

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_calloc(size_t count, size_t size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_calloc(alloc, count, size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_malloc(size_t size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_malloc(alloc, size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_realloc(void *ptr, size_t old_size, size_t new_size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_realloc(alloc, ptr, old_size, new_size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_xcalloc(size_t count, size_t size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_xcalloc(alloc, count, size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_xmalloc(size_t size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_xmalloc(alloc, size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_xrealloc(void *ptr, size_t old_size, size_t new_size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_xrealloc(alloc, ptr, old_size, new_size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static void *
cork_xreallocf(void *ptr, size_t old_size, size_t new_size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_xreallocf(alloc, ptr, old_size, new_size);
}

CORK_ATTR_UNUSED
static void
cork_free(void *ptr, size_t size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    cork_alloc_free(alloc, ptr, size);
}

CORK_ATTR_UNUSED
static void
cork_cfree(void *ptr, size_t count, size_t size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    cork_alloc_cfree(alloc, ptr, count, size);
}

#define cork_new(type)          cork_malloc(sizeof(type))
#define cork_xnew(type)         cork_xmalloc(sizeof(type))
#define cork_delete(type, ptr)  cork_free((ptr), sizeof(type))


/* string-related helper functions */

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static const char *
cork_strdup(const char *str)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_strdup(alloc, str);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static const char *
cork_strndup(const char *str, size_t size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_strndup(alloc, str, size);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static const char *
cork_xstrdup(const char *str)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_xstrdup(alloc, str);
}

CORK_ATTR_MALLOC
CORK_ATTR_UNUSED
static const char *
cork_xstrndup(const char *str, size_t size)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_xstrndup(alloc, str, size);
}

CORK_ATTR_UNUSED
static void
cork_strfree(const char *str)
{
    const struct cork_alloc  *alloc = cork_current_allocator();
    return cork_alloc_strfree(alloc, str);
}


/*-----------------------------------------------------------------------
 * Debugging allocator
 */

/* An allocator that adds some additional debugging checks:
 *
 * - We verify that every "free" call (cork_free, cork_cfree, cork_delete,
 *   cork_realloc) is passed the "correct" size — i.e., the same size that was
 *   passed in to the correspond "new" call (cork_malloc, cork_calloc,
 *   cork_realloc, cork_new).
 */

struct cork_alloc *
cork_debug_alloc_new(const struct cork_alloc *parent);


#endif /* LIBCORK_CORE_ALLOCATOR_H */
