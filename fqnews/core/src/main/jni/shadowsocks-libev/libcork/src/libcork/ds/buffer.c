/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "libcork/core/allocator.h"
#include "libcork/core/types.h"
#include "libcork/ds/buffer.h"
#include "libcork/ds/managed-buffer.h"
#include "libcork/ds/stream.h"
#include "libcork/helpers/errors.h"


void
cork_buffer_init(struct cork_buffer *buffer)
{
    buffer->buf = NULL;
    buffer->size = 0;
    buffer->allocated_size = 0;
}


struct cork_buffer *
cork_buffer_new(void)
{
    struct cork_buffer  *buffer = cork_new(struct cork_buffer);
    cork_buffer_init(buffer);
    return buffer;
}


void
cork_buffer_done(struct cork_buffer *buffer)
{
    if (buffer->buf != NULL) {
        cork_free(buffer->buf, buffer->allocated_size);
        buffer->buf = NULL;
    }
    buffer->size = 0;
    buffer->allocated_size = 0;
}


void
cork_buffer_free(struct cork_buffer *buffer)
{
    cork_buffer_done(buffer);
    cork_delete(struct cork_buffer, buffer);
}


bool
cork_buffer_equal(const struct cork_buffer *buffer1,
                  const struct cork_buffer *buffer2)
{
    if (buffer1 == buffer2) {
        return true;
    }

    if (buffer1->size != buffer2->size) {
        return false;
    }

    return (memcmp(buffer1->buf, buffer2->buf, buffer1->size) == 0);
}


static void
cork_buffer_ensure_size_int(struct cork_buffer *buffer, size_t desired_size)
{
    size_t  new_size;

    if (CORK_LIKELY(buffer->allocated_size >= desired_size)) {
        return;
    }

    /* Make sure we at least double the old size when reallocating. */
    new_size = buffer->allocated_size * 2;
    if (desired_size > new_size) {
        new_size = desired_size;
    }

    buffer->buf = cork_realloc(buffer->buf, buffer->allocated_size, new_size);
    buffer->allocated_size = new_size;
}

void
cork_buffer_ensure_size(struct cork_buffer *buffer, size_t desired_size)
{
    cork_buffer_ensure_size_int(buffer, desired_size);
}


void
cork_buffer_clear(struct cork_buffer *buffer)
{
    buffer->size = 0;
    if (buffer->buf != NULL) {
        ((char *) buffer->buf)[0] = '\0';
    }
}

void
cork_buffer_truncate(struct cork_buffer *buffer, size_t length)
{
    if (buffer->size > length) {
        buffer->size = length;
        if (length == 0) {
            if (buffer->buf != NULL) {
                ((char *) buffer->buf)[0] = '\0';
            }
        } else {
            ((char *) buffer->buf)[length] = '\0';
        }
    }
}


void
cork_buffer_set(struct cork_buffer *buffer, const void *src, size_t length)
{
    cork_buffer_ensure_size_int(buffer, length+1);
    memcpy(buffer->buf, src, length);
    ((char *) buffer->buf)[length] = '\0';
    buffer->size = length;
}


void
cork_buffer_append(struct cork_buffer *buffer, const void *src, size_t length)
{
    cork_buffer_ensure_size_int(buffer, buffer->size + length + 1);
    memcpy(buffer->buf + buffer->size, src, length);
    buffer->size += length;
    ((char *) buffer->buf)[buffer->size] = '\0';
}


void
cork_buffer_set_string(struct cork_buffer *buffer, const char *str)
{
    cork_buffer_set(buffer, str, strlen(str));
}


void
cork_buffer_append_string(struct cork_buffer *buffer, const char *str)
{
    cork_buffer_append(buffer, str, strlen(str));
}


void
cork_buffer_append_vprintf(struct cork_buffer *buffer, const char *format,
                           va_list args)
{
    size_t  format_size;
    va_list  args1;

    va_copy(args1, args);
    format_size = vsnprintf(buffer->buf + buffer->size,
                            buffer->allocated_size - buffer->size,
                            format, args1);
    va_end(args1);

    /* If the first call works, then set buffer->size and return. */
    if (format_size < (buffer->allocated_size - buffer->size)) {
        buffer->size += format_size;
        return;
    }

    /* If the first call fails, resize buffer and try again. */
    cork_buffer_ensure_size_int
        (buffer, buffer->allocated_size + format_size + 1);
    format_size = vsnprintf(buffer->buf + buffer->size,
                            buffer->allocated_size - buffer->size,
                            format, args);
    buffer->size += format_size;
}


void
cork_buffer_vprintf(struct cork_buffer *buffer, const char *format,
                    va_list args)
{
    cork_buffer_clear(buffer);
    cork_buffer_append_vprintf(buffer, format, args);
}


void
cork_buffer_append_printf(struct cork_buffer *buffer, const char *format, ...)
{
    va_list  args;
    va_start(args, format);
    cork_buffer_append_vprintf(buffer, format, args);
    va_end(args);
}


void
cork_buffer_printf(struct cork_buffer *buffer, const char *format, ...)
{
    va_list  args;
    va_start(args, format);
    cork_buffer_vprintf(buffer, format, args);
    va_end(args);
}


void
cork_buffer_append_indent(struct cork_buffer *buffer, size_t indent)
{
    cork_buffer_ensure_size_int(buffer, buffer->size + indent + 1);
    memset(buffer->buf + buffer->size, ' ', indent);
    buffer->size += indent;
    ((char *) buffer->buf)[buffer->size] = '\0';
}

/* including space */
#define is_sprint(ch)  ((ch) >= 0x20 && (ch) <= 0x7e)

/* not including space */
#define is_print(ch)  ((ch) > 0x20 && (ch) <= 0x7e)

#define is_space(ch) \
    ((ch) == ' ' || \
     (ch) == '\f' || \
     (ch) == '\n' || \
     (ch) == '\r' || \
     (ch) == '\t' || \
     (ch) == '\v')

#define to_hex(nybble) \
    ((nybble) < 10? '0' + (nybble): 'a' - 10 + (nybble))

void
cork_buffer_append_c_string(struct cork_buffer *dest,
                            const char *chars, size_t length)
{
    size_t  i;
    cork_buffer_append(dest, "\"", 1);
    for (i = 0; i < length; i++) {
        char  ch = chars[i];
        switch (ch) {
            case '\"':
                cork_buffer_append_literal(dest, "\\\"");
                break;
            case '\\':
                cork_buffer_append_literal(dest, "\\\\");
                break;
            case '\f':
                cork_buffer_append_literal(dest, "\\f");
                break;
            case '\n':
                cork_buffer_append_literal(dest, "\\n");
                break;
            case '\r':
                cork_buffer_append_literal(dest, "\\r");
                break;
            case '\t':
                cork_buffer_append_literal(dest, "\\t");
                break;
            case '\v':
                cork_buffer_append_literal(dest, "\\v");
                break;
            default:
                if (is_sprint(ch)) {
                    cork_buffer_append(dest, &chars[i], 1);
                } else {
                    uint8_t  byte = ch;
                    cork_buffer_append_printf(dest, "\\x%02" PRIx8, byte);
                }
                break;
        }
    }
    cork_buffer_append(dest, "\"", 1);
}

void
cork_buffer_append_hex_dump(struct cork_buffer *dest, size_t indent,
                            const char *chars, size_t length)
{
    char  hex[3 * 16];
    char  print[16];
    char  *curr_hex = hex;
    char  *curr_print = print;
    size_t  i;
    size_t  column = 0;
    for (i = 0; i < length; i++) {
        char  ch = chars[i];
        uint8_t  u8 = ch;
        *curr_hex++ = to_hex(u8 >> 4);
        *curr_hex++ = to_hex(u8 & 0x0f);
        *curr_hex++ = ' ';
        *curr_print++ = is_sprint(ch)? ch: '.';
        if (column == 0 && i != 0) {
            cork_buffer_append_literal(dest, "\n");
            cork_buffer_append_indent(dest, indent);
            column++;
        } else if (column == 15) {
            cork_buffer_append_printf
                (dest, "%-48.*s", (int) (curr_hex - hex), hex);
            cork_buffer_append_literal(dest, " |");
            cork_buffer_append(dest, print, curr_print - print);
            cork_buffer_append_literal(dest, "|");
            curr_hex = hex;
            curr_print = print;
            column = 0;
        } else {
            column++;
        }
    }

    if (column > 0) {
        cork_buffer_append_printf(dest, "%-48.*s", (int) (curr_hex - hex), hex);
        cork_buffer_append_literal(dest, " |");
        cork_buffer_append(dest, print, curr_print - print);
        cork_buffer_append_literal(dest, "|");
    }
}

void
cork_buffer_append_multiline(struct cork_buffer *dest, size_t indent,
                             const char *chars, size_t length)
{
    size_t  i;
    for (i = 0; i < length; i++) {
        char  ch = chars[i];
        if (ch == '\n') {
            cork_buffer_append_literal(dest, "\n");
            cork_buffer_append_indent(dest, indent);
        } else {
            cork_buffer_append(dest, &chars[i], 1);
        }
    }
}

void
cork_buffer_append_binary(struct cork_buffer *dest, size_t indent,
                          const char *chars, size_t length)
{
    size_t  i;
    bool  newline = false;

    /* If there are any non-printable characters, print out a hex dump */
    for (i = 0; i < length; i++) {
        if (!is_print(chars[i]) && !is_space(chars[i])) {
            cork_buffer_append_literal(dest, "(hex)\n");
            cork_buffer_append_indent(dest, indent);
            cork_buffer_append_hex_dump(dest, indent, chars, length);
            return;
        } else if (chars[i] == '\n') {
            newline = true;
            /* Don't immediately use the multiline format, since there might be
             * a non-printable character later on that kicks us over to the hex
             * dump format. */
        }
    }

    if (newline) {
        cork_buffer_append_literal(dest, "(multiline)\n");
        cork_buffer_append_indent(dest, indent);
        cork_buffer_append_multiline(dest, indent, chars, length);
    } else {
        cork_buffer_append(dest, chars, length);
    }
}


struct cork_buffer__managed_buffer {
    struct cork_managed_buffer  parent;
    struct cork_buffer  *buffer;
};

static void
cork_buffer__managed_free(struct cork_managed_buffer *vself)
{
    struct cork_buffer__managed_buffer  *self =
        cork_container_of(vself, struct cork_buffer__managed_buffer, parent);
    cork_buffer_free(self->buffer);
    cork_delete(struct cork_buffer__managed_buffer, self);
}

static struct cork_managed_buffer_iface  CORK_BUFFER__MANAGED_BUFFER = {
    cork_buffer__managed_free
};

struct cork_managed_buffer *
cork_buffer_to_managed_buffer(struct cork_buffer *buffer)
{
    struct cork_buffer__managed_buffer  *self =
        cork_new(struct cork_buffer__managed_buffer);
    self->parent.buf = buffer->buf;
    self->parent.size = buffer->size;
    self->parent.ref_count = 1;
    self->parent.iface = &CORK_BUFFER__MANAGED_BUFFER;
    self->buffer = buffer;
    return &self->parent;
}


int
cork_buffer_to_slice(struct cork_buffer *buffer, struct cork_slice *slice)
{
    struct cork_managed_buffer  *managed =
        cork_buffer_to_managed_buffer(buffer);

    /* We don't have to check for NULL; cork_managed_buffer_slice_offset
     * will do that for us. */
    int  rc = cork_managed_buffer_slice_offset(slice, managed, 0);

    /* Before returning, drop our reference to the managed buffer.  If
     * the slicing succeeded, then there will be one remaining reference
     * in the slice.  If it didn't succeed, this will free the managed
     * buffer for us. */
    cork_managed_buffer_unref(managed);
    return rc;
}


struct cork_buffer__stream_consumer {
    struct cork_stream_consumer  consumer;
    struct cork_buffer  *buffer;
};

static int
cork_buffer_stream_consumer_data(struct cork_stream_consumer *consumer,
                                 const void *buf, size_t size,
                                 bool is_first_chunk)
{
    struct cork_buffer__stream_consumer  *bconsumer = cork_container_of
        (consumer, struct cork_buffer__stream_consumer, consumer);
    cork_buffer_append(bconsumer->buffer, buf, size);
    return 0;
}

static int
cork_buffer_stream_consumer_eof(struct cork_stream_consumer *consumer)
{
    return 0;
}

static void
cork_buffer_stream_consumer_free(struct cork_stream_consumer *consumer)
{
    struct cork_buffer__stream_consumer  *bconsumer =
        cork_container_of
        (consumer, struct cork_buffer__stream_consumer, consumer);
    cork_delete(struct cork_buffer__stream_consumer, bconsumer);
}

struct cork_stream_consumer *
cork_buffer_to_stream_consumer(struct cork_buffer *buffer)
{
    struct cork_buffer__stream_consumer  *bconsumer =
        cork_new(struct cork_buffer__stream_consumer);
    bconsumer->consumer.data = cork_buffer_stream_consumer_data;
    bconsumer->consumer.eof = cork_buffer_stream_consumer_eof;
    bconsumer->consumer.free = cork_buffer_stream_consumer_free;
    bconsumer->buffer = buffer;
    return &bconsumer->consumer;
}
