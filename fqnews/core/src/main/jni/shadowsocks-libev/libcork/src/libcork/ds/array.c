/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "libcork/core/types.h"
#include "libcork/ds/array.h"
#include "libcork/helpers/errors.h"

#ifndef CORK_ARRAY_DEBUG
#define CORK_ARRAY_DEBUG 0
#endif

#if CORK_ARRAY_DEBUG
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
 * Resizable arrays
 */

struct cork_array_priv {
    size_t  allocated_count;
    size_t  allocated_size;
    size_t  element_size;
    size_t  initialized_count;
    void  *user_data;
    cork_free_f  free_user_data;
    cork_init_f  init;
    cork_done_f  done;
    cork_init_f  reuse;
    cork_done_f  remove;
};

void
cork_raw_array_init(struct cork_raw_array *array, size_t element_size)
{
    array->items = NULL;
    array->size = 0;
    array->priv = cork_new(struct cork_array_priv);
    array->priv->allocated_count = 0;
    array->priv->allocated_size = 0;
    array->priv->element_size = element_size;
    array->priv->initialized_count = 0;
    array->priv->user_data = NULL;
    array->priv->free_user_data = NULL;
    array->priv->init = NULL;
    array->priv->done = NULL;
    array->priv->reuse = NULL;
    array->priv->remove = NULL;
}

void
cork_raw_array_done(struct cork_raw_array *array)
{
    if (array->priv->done != NULL) {
        size_t  i;
        char  *element = array->items;
        for (i = 0; i < array->priv->initialized_count; i++) {
            array->priv->done(array->priv->user_data, element);
            element += array->priv->element_size;
        }
    }
    if (array->items != NULL) {
        cork_free(array->items, array->priv->allocated_size);
    }
    cork_free_user_data(array->priv);
    cork_delete(struct cork_array_priv, array->priv);
}

void
cork_raw_array_set_callback_data(struct cork_raw_array *array,
                                 void *user_data, cork_free_f free_user_data)
{
    array->priv->user_data = user_data;
    array->priv->free_user_data = free_user_data;
}

void
cork_raw_array_set_init(struct cork_raw_array *array, cork_init_f init)
{
    array->priv->init = init;
}

void
cork_raw_array_set_done(struct cork_raw_array *array, cork_done_f done)
{
    array->priv->done = done;
}

void
cork_raw_array_set_reuse(struct cork_raw_array *array, cork_init_f reuse)
{
    array->priv->reuse = reuse;
}

void
cork_raw_array_set_remove(struct cork_raw_array *array, cork_done_f remove)
{
    array->priv->remove = remove;
}

size_t
cork_raw_array_element_size(const struct cork_raw_array *array)
{
    return array->priv->element_size;
}

void
cork_raw_array_clear(struct cork_raw_array *array)
{
    if (array->priv->remove != NULL) {
        size_t  i;
        char  *element = array->items;
        for (i = 0; i < array->priv->initialized_count; i++) {
            array->priv->remove(array->priv->user_data, element);
            element += array->priv->element_size;
        }
    }
    array->size = 0;
}

void *
cork_raw_array_elements(const struct cork_raw_array *array)
{
    return array->items;
}

void *
cork_raw_array_at(const struct cork_raw_array *array, size_t index)
{
    return ((char *) array->items) + (array->priv->element_size * index);
}

size_t
cork_raw_array_size(const struct cork_raw_array *array)
{
    return array->size;
}

bool
cork_raw_array_is_empty(const struct cork_raw_array *array)
{
    return (array->size == 0);
}

void
cork_raw_array_ensure_size(struct cork_raw_array *array, size_t desired_count)
{
    size_t  desired_size;

    DEBUG("--- Array %p: Ensure %zu %zu-byte elements",
          array, desired_count, array->priv->element_size);
    desired_size = desired_count * array->priv->element_size;

    if (desired_size > array->priv->allocated_size) {
        size_t  new_count = array->priv->allocated_count * 2;
        size_t  new_size = array->priv->allocated_size * 2;
        if (desired_size > new_size) {
            new_count = desired_count;
            new_size = desired_size;
        }

        DEBUG("--- Array %p: Reallocating %zu->%zu bytes",
              array, array->priv->allocated_size, new_size);
        array->items =
            cork_realloc(array->items, array->priv->allocated_size, new_size);

        array->priv->allocated_count = new_count;
        array->priv->allocated_size = new_size;
    }
}

void *
cork_raw_array_append(struct cork_raw_array *array)
{
    size_t  index;
    void  *element;
    index = array->size++;
    cork_raw_array_ensure_size(array, array->size);
    element = cork_raw_array_at(array, index);

    /* Call the init or reset callback, depending on whether this entry has been
     * initialized before. */

    /* Since we can currently only add elements by appending them one at a time,
     * then this entry is either already initialized, or is the first
     * uninitialized entry. */
    assert(index <= array->priv->initialized_count);

    if (index == array->priv->initialized_count) {
        /* This element has not been initialized yet. */
        array->priv->initialized_count++;
        if (array->priv->init != NULL) {
            array->priv->init(array->priv->user_data, element);
        }
    } else {
        /* This element has already been initialized. */
        if (array->priv->reuse != NULL) {
            array->priv->reuse(array->priv->user_data, element);
        }
    }

    return element;
}

int
cork_raw_array_copy(struct cork_raw_array *dest,
                    const struct cork_raw_array *src,
                    cork_copy_f copy, void *user_data)
{
    size_t  i;
    size_t  reuse_count;
    char  *dest_element;

    DEBUG("--- Copying %zu elements (%zu bytes) from %p to %p",
          src->size, src->size * dest->priv->element_size, src, dest);
    assert(dest->priv->element_size == src->priv->element_size);
    cork_array_clear(dest);
    cork_array_ensure_size(dest, src->size);

    /* Initialize enough elements to hold the contents of src */
    reuse_count = dest->priv->initialized_count;
    if (src->size < reuse_count) {
        reuse_count = src->size;
    }

    dest_element = dest->items;
    if (dest->priv->reuse != NULL) {
        DEBUG("    Calling reuse on elements 0-%zu", reuse_count);
        for (i = 0; i < reuse_count; i++) {
            dest->priv->reuse(dest->priv->user_data, dest_element);
            dest_element += dest->priv->element_size;
        }
    } else {
        dest_element += reuse_count * dest->priv->element_size;
    }

    if (dest->priv->init != NULL) {
        DEBUG("    Calling init on elements %zu-%zu", reuse_count, src->size);
        for (i = reuse_count; i < src->size; i++) {
            dest->priv->init(dest->priv->user_data, dest_element);
            dest_element += dest->priv->element_size;
        }
    }

    if (src->size > dest->priv->initialized_count) {
        dest->priv->initialized_count = src->size;
    }

    /* If the caller provided a copy function, let it copy each element in turn.
     * Otherwise, bulk copy everything using memcpy. */
    if (copy == NULL) {
        memcpy(dest->items, src->items, src->size * dest->priv->element_size);
    } else {
        const char  *src_element = src->items;
        dest_element = dest->items;
        for (i = 0; i < src->size; i++) {
            rii_check(copy(user_data, dest_element, src_element));
            dest_element += dest->priv->element_size;
            src_element += dest->priv->element_size;
        }
    }

    dest->size = src->size;
    return 0;
}


/*-----------------------------------------------------------------------
 * Pointer arrays
 */

struct cork_pointer_array {
    cork_free_f  free;
};

static void
pointer__init(void *user_data, void *vvalue)
{
    void **value = vvalue;
    *value = NULL;
}

static void
pointer__done(void *user_data, void *vvalue)
{
    struct cork_pointer_array  *ptr_array = user_data;
    void **value = vvalue;
    if (*value != NULL) {
        ptr_array->free(*value);
    }
}

static void
pointer__remove(void *user_data, void *vvalue)
{
    struct cork_pointer_array  *ptr_array = user_data;
    void **value = vvalue;
    if (*value != NULL) {
        ptr_array->free(*value);
    }
    *value = NULL;
}

static void
pointer__free(void *user_data)
{
    struct cork_pointer_array  *ptr_array = user_data;
    cork_delete(struct cork_pointer_array, ptr_array);
}

void
cork_raw_pointer_array_init(struct cork_raw_array *array, cork_free_f free_ptr)
{
    struct cork_pointer_array  *ptr_array = cork_new(struct cork_pointer_array);
    ptr_array->free = free_ptr;
    cork_raw_array_init(array, sizeof(void *));
    cork_array_set_callback_data(array, ptr_array, pointer__free);
    cork_array_set_init(array, pointer__init);
    cork_array_set_done(array, pointer__done);
    cork_array_set_remove(array, pointer__remove);
}


/*-----------------------------------------------------------------------
 * String arrays
 */

void
cork_string_array_init(struct cork_string_array *array)
{
    cork_raw_pointer_array_init
        ((struct cork_raw_array *) array, (cork_free_f) cork_strfree);
}

void
cork_string_array_append(struct cork_string_array *array, const char *str)
{
    const char  *copy = cork_strdup(str);
    cork_array_append(array, copy);
}

static int
string__copy(void *user_data, void *vdest, const void *vsrc)
{
    const char  **dest = vdest;
    const char  **src = (const char **) vsrc;
    *dest = cork_strdup(*src);
    return 0;
}

void
cork_string_array_copy(struct cork_string_array *dest,
                       const struct cork_string_array *src)
{
    CORK_ATTR_UNUSED int  rc;
    rc = cork_array_copy(dest, src, string__copy, NULL);
    /* cork_array_copy can only fail if the copy callback fails, and ours
     * doesn't! */
    assert(rc == 0);
}
