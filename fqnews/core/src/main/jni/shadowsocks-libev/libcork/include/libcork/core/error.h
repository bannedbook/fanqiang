/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_ERROR_H
#define LIBCORK_CORE_ERROR_H

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <libcork/core/api.h>
#include <libcork/core/attributes.h>
#include <libcork/core/types.h>


/* Should be a hash of a string representing the error code. */
typedef uint32_t  cork_error;

/* An error code that represents “no error”. */
#define CORK_ERROR_NONE  ((cork_error) 0)

CORK_API bool
cork_error_occurred(void);

CORK_API cork_error
cork_error_code(void);

CORK_API const char *
cork_error_message(void);


CORK_API void
cork_error_clear(void);

CORK_API void
cork_error_set_printf(cork_error code, const char *format, ...)
    CORK_ATTR_PRINTF(2,3);

CORK_API void
cork_error_set_string(cork_error code, const char *str);

CORK_API void
cork_error_set_vprintf(cork_error code, const char *format, va_list args)
    CORK_ATTR_PRINTF(2,0);

CORK_API void
cork_error_prefix_printf(const char *format, ...)
    CORK_ATTR_PRINTF(1,2);

CORK_API void
cork_error_prefix_string(const char *str);

CORK_API void
cork_error_prefix_vprintf(const char *format, va_list arg)
    CORK_ATTR_PRINTF(1,0);


/* deprecated */
CORK_API void
cork_error_set(uint32_t error_class, unsigned int error_code,
               const char *format, ...)
    CORK_ATTR_PRINTF(3,4);

/* deprecated */
CORK_API void
cork_error_prefix(const char *format, ...)
    CORK_ATTR_PRINTF(1,2);


/*-----------------------------------------------------------------------
 * Built-in errors
 */

#define CORK_PARSE_ERROR               0x95dfd3c8
#define CORK_REDEFINED                 0x171629cb
#define CORK_UNDEFINED                 0xedc3d7d9
#define CORK_UNKNOWN_ERROR             0x8cb0880d

#define cork_parse_error(...) \
    cork_error_set_printf(CORK_PARSE_ERROR, __VA_ARGS__)
#define cork_redefined(...) \
    cork_error_set_printf(CORK_REDEFINED, __VA_ARGS__)
#define cork_undefined(...) \
    cork_error_set_printf(CORK_UNDEFINED, __VA_ARGS__)

CORK_API void
cork_system_error_set(void);

CORK_API void
cork_system_error_set_explicit(int err);

CORK_API void
cork_unknown_error_set_(const char *location);

#define cork_unknown_error() \
    cork_unknown_error_set_(__func__)


/*-----------------------------------------------------------------------
 * Abort on failure
 */

#define cork_abort_(func, file, line, fmt, ...) \
    do { \
        fprintf(stderr, fmt "\n  in %s (%s:%u)\n", \
                __VA_ARGS__, (func), (file), (unsigned int) (line)); \
        abort(); \
    } while (0)

#define cork_abort(fmt, ...) \
    cork_abort_(__func__, __FILE__, __LINE__, fmt, __VA_ARGS__)

CORK_ATTR_UNUSED
static void *
cork_abort_if_null_(void *ptr, const char *msg, const char *func,
                    const char *file, unsigned int line)
{
    if (CORK_UNLIKELY(ptr == NULL)) {
        cork_abort_(func, file, line, "%s", msg);
    } else {
        return ptr;
    }
}

#define cork_abort_if_null(ptr, msg) \
    (cork_abort_if_null_(ptr, msg, __func__, __FILE__, __LINE__))

#define cork_unreachable() \
    cork_abort("%s", "Code should not be reachable")


#endif /* LIBCORK_CORE_ERROR_H */
