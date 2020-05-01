/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "libcork/config.h"
#include "libcork/core/allocator.h"
#include "libcork/core/error.h"
#include "libcork/ds/buffer.h"
#include "libcork/os/process.h"
#include "libcork/threads/basics.h"


/*-----------------------------------------------------------------------
 * Life cycle
 */

struct cork_error {
    cork_error  code;
    struct cork_buffer  *message;
    struct cork_buffer  *other;
    struct cork_buffer  buf1;
    struct cork_buffer  buf2;
    struct cork_error  *next;
};

static struct cork_error *
cork_error_new(void)
{
    struct cork_error  *error = cork_new(struct cork_error);
    error->code = CORK_ERROR_NONE;
    cork_buffer_init(&error->buf1);
    cork_buffer_init(&error->buf2);
    error->message = &error->buf1;
    error->other = &error->buf2;
    return error;
}

static void
cork_error_free(struct cork_error *error)
{
    cork_buffer_done(&error->buf1);
    cork_buffer_done(&error->buf2);
    cork_delete(struct cork_error, error);
}


static struct cork_error * volatile  errors;

cork_once_barrier(cork_error_list);

static void
cork_error_list_done(void)
{
    struct cork_error  *curr;
    struct cork_error  *next;
    for (curr = errors; curr != NULL; curr = next) {
        next = curr->next;
        cork_error_free(curr);
    }
}

static void
cork_error_list_init(void)
{
    cork_cleanup_at_exit(0, cork_error_list_done);
}


cork_tls(struct cork_error *, cork_error_);

static struct cork_error *
cork_error_get(void)
{
    struct cork_error  **error_ptr = cork_error__get();
    if (CORK_UNLIKELY(*error_ptr == NULL)) {
        struct cork_error  *old_head;
        struct cork_error  *error = cork_error_new();
        cork_once(cork_error_list, cork_error_list_init());
        do {
            old_head = errors;
            error->next = old_head;
        } while (cork_ptr_cas(&errors, old_head, error) != old_head);
        *error_ptr = error;
        return error;
    } else {
        return *error_ptr;
    }
}


/*-----------------------------------------------------------------------
 * Public error API
 */

bool
cork_error_occurred(void)
{
    struct cork_error  *error = cork_error_get();
    return error->code != CORK_ERROR_NONE;
}

cork_error
cork_error_code(void)
{
    struct cork_error  *error = cork_error_get();
    return error->code;
}

const char *
cork_error_message(void)
{
    struct cork_error  *error = cork_error_get();
    return error->message->buf;
}

void
cork_error_clear(void)
{
    struct cork_error  *error = cork_error_get();
    error->code = CORK_ERROR_NONE;
    cork_buffer_clear(error->message);
}

void
cork_error_set_printf(cork_error code, const char *format, ...)
{
    va_list  args;
    struct cork_error  *error = cork_error_get();
    error->code = code;
    va_start(args, format);
    cork_buffer_vprintf(error->message, format, args);
    va_end(args);
}

void
cork_error_set_string(cork_error code, const char *str)
{
    struct cork_error  *error = cork_error_get();
    error->code = code;
    cork_buffer_set_string(error->message, str);
}

void
cork_error_set_vprintf(cork_error code, const char *format, va_list args)
{
    struct cork_error  *error = cork_error_get();
    error->code = code;
    cork_buffer_vprintf(error->message, format, args);
}

void
cork_error_prefix_printf(const char *format, ...)
{
    va_list  args;
    struct cork_error  *error = cork_error_get();
    struct cork_buffer  *temp;
    va_start(args, format);
    cork_buffer_vprintf(error->other, format, args);
    va_end(args);
    cork_buffer_append_copy(error->other, error->message);
    temp = error->other;
    error->other = error->message;
    error->message = temp;
}

void
cork_error_prefix_string(const char *str)
{
    struct cork_error  *error = cork_error_get();
    struct cork_buffer  *temp;
    cork_buffer_set_string(error->other, str);
    cork_buffer_append_copy(error->other, error->message);
    temp = error->other;
    error->other = error->message;
    error->message = temp;
}

void
cork_error_prefix_vprintf(const char *format, va_list args)
{
    struct cork_error  *error = cork_error_get();
    struct cork_buffer  *temp;
    cork_buffer_vprintf(error->other, format, args);
    cork_buffer_append_copy(error->other, error->message);
    temp = error->other;
    error->other = error->message;
    error->message = temp;
}


/*-----------------------------------------------------------------------
 * Deprecated
 */

void
cork_error_set(uint32_t error_class, unsigned int error_code,
               const char *format, ...)
{
    /* Create a fallback error code that's most likely not very useful. */
    va_list  args;
    va_start(args, format);
    cork_error_set_vprintf(error_class + error_code, format, args);
    va_end(args);
}

void
cork_error_prefix(const char *format, ...)
{
    va_list  args;
    va_start(args, format);
    cork_error_prefix_vprintf(format, args);
    va_end(args);
}


/*-----------------------------------------------------------------------
 * Built-in errors
 */

void
cork_system_error_set_explicit(int err)
{
    cork_error_set_string(err, strerror(err));
}

void
cork_system_error_set(void)
{
    cork_error_set_string(errno, strerror(errno));
}

void
cork_unknown_error_set_(const char *location)
{
    cork_error_set_printf(CORK_UNKNOWN_ERROR, "Unknown error in %s", location);
}
