/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_DS_SLICE_H
#define LIBCORK_DS_SLICE_H

#include <libcork/core/api.h>
#include <libcork/core/types.h>


/*-----------------------------------------------------------------------
 * Error handling
 */

/* hash of "libcork/ds/slice.h" */
#define CORK_SLICE_ERROR  0x960ca750

enum cork_slice_error {
    /* Trying to slice a nonexistent subset of a buffer */
    CORK_SLICE_INVALID_SLICE
};


/*-----------------------------------------------------------------------
 * Slices
 */

struct cork_slice;

struct cork_slice_iface {
    /* Free the slice.  Can be NULL if you don't need to free any
     * underlying buffer. */
    void
    (*free)(struct cork_slice *self);

    /* Create a copy of a slice.  You can assume that offset and length
     * refer to a valid subset of the buffer. */
    int
    (*copy)(struct cork_slice *dest, const struct cork_slice *self,
            size_t offset, size_t length);

    /* Create a “light” copy of a slice.  A light copy is not allowed to exist
     * longer than the slice that it was copied from, which can sometimes let
     * you perform less work to produce the copy.  You can assume that offset
     * and length refer to a valid subset of the buffer. */
    int
    (*light_copy)(struct cork_slice *dest, const struct cork_slice *self,
                  size_t offset, size_t length);

    /* Update the current slice to point at a different subset.  You can
     * assume that offset and length refer to a valid subset of the
     * buffer.  Can be NULL if you don't need to do anything special to
     * the underlying buffer; in this case, we'll update the slice's buf
     * and size fields for you. */
    int
    (*slice)(struct cork_slice *self, size_t offset, size_t length);
};


struct cork_slice {
    /* The beginning of the sliced portion of the buffer. */
    const void  *buf;
    /* The length of the sliced portion of the buffer. */
    size_t  size;
    /* The slice implementation of the underlying buffer. */
    struct cork_slice_iface  *iface;
    /* An opaque pointer used by the slice implementation to refer to
     * the underlying buffer. */
    void  *user_data;
};


CORK_API void
cork_slice_clear(struct cork_slice *slice);

#define cork_slice_is_empty(slice)  ((slice)->buf == NULL)


CORK_API int
cork_slice_copy(struct cork_slice *dest, const struct cork_slice *slice,
                size_t offset, size_t length);

#define cork_slice_copy_fast(dest, slice, offset, length) \
    ((slice)->iface->copy((dest), (slice), (offset), (length)))

CORK_API int
cork_slice_copy_offset(struct cork_slice *dest, const struct cork_slice *slice,
                       size_t offset);

#define cork_slice_copy_offset_fast(dest, slice, offset) \
    ((slice)->iface->copy \
     ((dest), (slice), (offset), (slice)->size - (offset)))


CORK_API int
cork_slice_light_copy(struct cork_slice *dest, const struct cork_slice *slice,
                      size_t offset, size_t length);

#define cork_slice_light_copy_fast(dest, slice, offset, length) \
    ((slice)->iface->light_copy((dest), (slice), (offset), (length)))

CORK_API int
cork_slice_light_copy_offset(struct cork_slice *dest,
                             const struct cork_slice *slice, size_t offset);

#define cork_slice_light_copy_offset_fast(dest, slice, offset) \
    ((slice)->iface->light_copy \
     ((dest), (slice), (offset), (slice)->size - (offset)))


CORK_API int
cork_slice_slice(struct cork_slice *slice, size_t offset, size_t length);

#define cork_slice_slice_fast(_slice, offset, length) \
    ((_slice)->iface->slice == NULL? \
     ((_slice)->buf += (offset), (_slice)->size = (length), 0): \
     ((_slice)->iface->slice((_slice), (offset), (length))))

CORK_API int
cork_slice_slice_offset(struct cork_slice *slice, size_t offset);

#define cork_slice_slice_offset_fast(_slice, offset) \
    ((_slice)->iface->slice == NULL? \
     ((_slice)->buf += (offset), (_slice)->size -= (offset), 0): \
     ((_slice)->iface->slice \
      ((_slice), (offset), (_slice)->size - (offset))))


CORK_API void
cork_slice_finish(struct cork_slice *slice);

CORK_API bool
cork_slice_equal(const struct cork_slice *slice1,
                 const struct cork_slice *slice2);

CORK_API void
cork_slice_init_static(struct cork_slice *dest, const void *buf, size_t size);

CORK_API void
cork_slice_init_copy_once(struct cork_slice *dest, const void *buf,
                          size_t size);


#endif /* LIBCORK_DS_SLICE_H */
