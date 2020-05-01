/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_DS_BUFFER_H
#define LIBCORK_DS_BUFFER_H


#include <stdarg.h>

#include <libcork/core/api.h>
#include <libcork/core/attributes.h>
#include <libcork/core/types.h>


struct cork_buffer {
    /* The current contents of the buffer. */
    void  *buf;
    /* The current size of the buffer. */
    size_t  size;
    /* The amount of space allocated for buf. */
    size_t  allocated_size;
};


CORK_API void
cork_buffer_init(struct cork_buffer *buffer);

#define CORK_BUFFER_INIT()  { NULL, 0, 0 }

CORK_API struct cork_buffer *
cork_buffer_new(void);

CORK_API void
cork_buffer_done(struct cork_buffer *buffer);

CORK_API void
cork_buffer_free(struct cork_buffer *buffer);


CORK_API bool
cork_buffer_equal(const struct cork_buffer *buffer1,
                  const struct cork_buffer *buffer2);


CORK_API void
cork_buffer_ensure_size(struct cork_buffer *buffer, size_t desired_size);


CORK_API void
cork_buffer_clear(struct cork_buffer *buffer);

CORK_API void
cork_buffer_truncate(struct cork_buffer *buffer, size_t length);

#define cork_buffer_byte(buffer, i)  (((const uint8_t *) (buffer)->buf)[(i)])
#define cork_buffer_char(buffer, i)  (((const char *) (buffer)->buf)[(i)])


/*-----------------------------------------------------------------------
 * A whole bunch of methods for adding data
 */

#define cork_buffer_copy(dest, src) \
    (cork_buffer_set((dest), (src)->buf, (src)->size))

CORK_API void
cork_buffer_set(struct cork_buffer *buffer, const void *src, size_t length);

#define cork_buffer_append_copy(dest, src) \
    (cork_buffer_append((dest), (src)->buf, (src)->size))

CORK_API void
cork_buffer_append(struct cork_buffer *buffer, const void *src, size_t length);


CORK_API void
cork_buffer_set_string(struct cork_buffer *buffer, const char *str);

CORK_API void
cork_buffer_append_string(struct cork_buffer *buffer, const char *str);

#define cork_buffer_set_literal(buffer, str) \
    (cork_buffer_set((buffer), (str), sizeof((str)) - 1))

#define cork_buffer_append_literal(buffer, str) \
    (cork_buffer_append((buffer), (str), sizeof((str)) - 1))


CORK_API void
cork_buffer_printf(struct cork_buffer *buffer, const char *format, ...)
    CORK_ATTR_PRINTF(2,3);

CORK_API void
cork_buffer_append_printf(struct cork_buffer *buffer, const char *format, ...)
    CORK_ATTR_PRINTF(2,3);

CORK_API void
cork_buffer_vprintf(struct cork_buffer *buffer, const char *format,
                    va_list args)
    CORK_ATTR_PRINTF(2,0);

CORK_API void
cork_buffer_append_vprintf(struct cork_buffer *buffer, const char *format,
                           va_list args)
    CORK_ATTR_PRINTF(2,0);


/*-----------------------------------------------------------------------
 * Some helpers for pretty-printing data
 */

CORK_API void
cork_buffer_append_indent(struct cork_buffer *buffer, size_t indent);

CORK_API void
cork_buffer_append_c_string(struct cork_buffer *buffer,
                            const char *src, size_t length);

CORK_API void
cork_buffer_append_hex_dump(struct cork_buffer *buffer, size_t indent,
                            const char *src, size_t length);

CORK_API void
cork_buffer_append_multiline(struct cork_buffer *buffer, size_t indent,
                             const char *src, size_t length);

CORK_API void
cork_buffer_append_binary(struct cork_buffer *buffer, size_t indent,
                          const char *src, size_t length);


/*-----------------------------------------------------------------------
 * Buffer's managed buffer/slice implementation
 */

#include <libcork/ds/managed-buffer.h>
#include <libcork/ds/slice.h>

CORK_API struct cork_managed_buffer *
cork_buffer_to_managed_buffer(struct cork_buffer *buffer);

CORK_API int
cork_buffer_to_slice(struct cork_buffer *buffer, struct cork_slice *slice);


/*-----------------------------------------------------------------------
 * Buffer's stream consumer implementation
 */

#include <libcork/ds/stream.h>

CORK_API struct cork_stream_consumer *
cork_buffer_to_stream_consumer(struct cork_buffer *buffer);


#endif /* LIBCORK_DS_BUFFER_H */
