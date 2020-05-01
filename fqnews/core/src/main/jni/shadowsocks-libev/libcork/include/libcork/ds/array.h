/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_DS_ARRAY_H
#define LIBCORK_DS_ARRAY_H


#include <libcork/core/api.h>
#include <libcork/core/callbacks.h>
#include <libcork/core/types.h>


/*-----------------------------------------------------------------------
 * Resizable arrays
 */

struct cork_array_priv;

struct cork_raw_array {
    void  *items;
    size_t  size;
    struct cork_array_priv  *priv;
};

CORK_API void
cork_raw_array_init(struct cork_raw_array *array, size_t element_size);

CORK_API void
cork_raw_array_done(struct cork_raw_array *array);

CORK_API void
cork_raw_array_set_callback_data(struct cork_raw_array *array,
                                 void *user_data, cork_free_f free_user_data);

CORK_API void
cork_raw_array_set_init(struct cork_raw_array *array, cork_init_f init);

CORK_API void
cork_raw_array_set_done(struct cork_raw_array *array, cork_done_f done);

CORK_API void
cork_raw_array_set_reuse(struct cork_raw_array *array, cork_init_f reuse);

CORK_API void
cork_raw_array_set_remove(struct cork_raw_array *array, cork_done_f remove);

CORK_API size_t
cork_raw_array_element_size(const struct cork_raw_array *array);

CORK_API void
cork_raw_array_clear(struct cork_raw_array *array);

CORK_API void *
cork_raw_array_elements(const struct cork_raw_array *array);

CORK_API void *
cork_raw_array_at(const struct cork_raw_array *array, size_t index);

CORK_API size_t
cork_raw_array_size(const struct cork_raw_array *array);

CORK_API bool
cork_raw_array_is_empty(const struct cork_raw_array *array);

CORK_API void
cork_raw_array_ensure_size(struct cork_raw_array *array, size_t count);

CORK_API void *
cork_raw_array_append(struct cork_raw_array *array);

CORK_API int
cork_raw_array_copy(struct cork_raw_array *dest,
                    const struct cork_raw_array *src,
                    cork_copy_f copy, void *user_data);


/*-----------------------------------------------------------------------
 * Type-checked resizable arrays
 */

#define cork_array(T) \
    struct { \
        T  *items; \
        size_t  size; \
        struct cork_array_priv  *priv; \
    }

#define cork_array_element_size(arr)  (sizeof((arr)->items[0]))
#define cork_array_elements(arr)  ((arr)->items)
#define cork_array_at(arr, i)     ((arr)->items[(i)])
#define cork_array_size(arr)      ((arr)->size)
#define cork_array_is_empty(arr)  ((arr)->size == 0)
#define cork_array_to_raw(arr)    ((struct cork_raw_array *) (void *) (arr))

#define cork_array_init(arr) \
    (cork_raw_array_init(cork_array_to_raw(arr), cork_array_element_size(arr)))
#define cork_array_done(arr) \
    (cork_raw_array_done(cork_array_to_raw(arr)))

#define cork_array_set_callback_data(arr, ud, fud) \
    (cork_raw_array_set_callback_data(cork_array_to_raw(arr), (ud), (fud)))
#define cork_array_set_init(arr, i) \
    (cork_raw_array_set_init(cork_array_to_raw(arr), (i)))
#define cork_array_set_done(arr, d) \
    (cork_raw_array_set_done(cork_array_to_raw(arr), (d)))
#define cork_array_set_reuse(arr, r) \
    (cork_raw_array_set_reuse(cork_array_to_raw(arr), (r)))
#define cork_array_set_remove(arr, r) \
    (cork_raw_array_set_remove(cork_array_to_raw(arr), (r)))

#define cork_array_clear(arr) \
    (cork_raw_array_clear(cork_array_to_raw(arr)))
#define cork_array_copy(d, s, c, ud) \
    (cork_raw_array_copy(cork_array_to_raw(d), cork_array_to_raw(s), (c), (ud)))

#define cork_array_ensure_size(arr, count) \
    (cork_raw_array_ensure_size(cork_array_to_raw(arr), (count)))

#define cork_array_append(arr, element) \
    (cork_raw_array_append(cork_array_to_raw(arr)), \
     ((arr)->items[(arr)->size - 1] = (element), (void) 0))

#define cork_array_append_get(arr) \
    (cork_raw_array_append(cork_array_to_raw(arr)), \
     &(arr)->items[(arr)->size - 1])


/*-----------------------------------------------------------------------
 * Builtin array types
 */

CORK_API void
cork_raw_pointer_array_init(struct cork_raw_array *array, cork_free_f free);

#define cork_pointer_array_init(arr, f) \
    (cork_raw_pointer_array_init(cork_array_to_raw(arr), (f)))

struct cork_string_array {
    const char  **items;
    size_t  size;
    struct cork_array_priv  *priv;
};

CORK_API void
cork_string_array_init(struct cork_string_array *array);

CORK_API void
cork_string_array_append(struct cork_string_array *array, const char *str);

CORK_API void
cork_string_array_copy(struct cork_string_array *dest,
                       const struct cork_string_array *src);


#endif /* LIBCORK_DS_ARRAY_H */
