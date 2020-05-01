/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_ATTRIBUTES_H
#define LIBCORK_CORE_ATTRIBUTES_H

#include <libcork/config.h>


/*
 * Declare a “const” function.
 *
 * A const function is one whose return value depends only on its
 * parameters.  This is slightly more strict than a “pure” function; a
 * const function is not allowed to read from global variables, whereas
 * a pure function is.
 *
 *   int square(int x) CORK_ATTR_CONST;
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES
#define CORK_ATTR_CONST  __attribute__((const))
#else
#define CORK_ATTR_CONST
#endif


/*
 * Declare a “pure” function.
 *
 * A pure function is one whose return value depends only on its
 * parameters, and global variables.
 *
 *   int square(int x) CORK_ATTR_PURE;
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES
#define CORK_ATTR_PURE  __attribute__((pure))
#else
#define CORK_ATTR_PURE
#endif


/*
 * Declare that a function returns a newly allocated pointer.
 *
 * The compiler can use this information to generate more accurate
 * aliasing information, since it can infer that the result of the
 * function cannot alias any other existing pointer.
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES
#define CORK_ATTR_MALLOC  __attribute__((malloc))
#else
#define CORK_ATTR_MALLOC
#endif


/*
 * Declare that a function shouldn't be inlined.
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES
#define CORK_ATTR_NOINLINE  __attribute__((noinline))
#else
#define CORK_ATTR_NOINLINE
#endif


/*
 * Declare an entity that isn't used.
 *
 * This lets you keep -Wall activated in several cases where you're
 * obligated to define something that you don't intend to use.
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES
#define CORK_ATTR_UNUSED  __attribute__((unused))
#else
#define CORK_ATTR_UNUSED
#endif


/*
 * Declare a function that takes in printf-like parameters.
 *
 * When the compiler supports this attribute, it will check the format
 * string, and the following arguments, to make sure that they match.
 * format_index and args_index are 1-based.
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES
#define CORK_ATTR_PRINTF(format_index, args_index) \
    __attribute__((format(printf, format_index, args_index)))
#else
#define CORK_ATTR_PRINTF(format_index, args_index)
#endif


/*
 * Declare a var-arg function whose last parameter must be a NULL
 * sentinel value.
 *
 * When the compiler supports this attribute, it will check the actual
 * parameters whenever this function is called, and ensure that the last
 * parameter is a @c NULL.
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES
#define CORK_ATTR_SENTINEL  __attribute__((sentinel))
#else
#define CORK_ATTR_SENTINEL
#endif


/*
 * Declare that a boolean expression is likely to be true or false.
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES
#define CORK_LIKELY(expr)  __builtin_expect((expr), 1)
#define CORK_UNLIKELY(expr)  __builtin_expect((expr), 0)
#else
#define CORK_LIKELY(expr)  (expr)
#define CORK_UNLIKELY(expr)  (expr)
#endif

/*
 * Declare that a function is part of the current library's public API, or that
 * it's internal to the current library.
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES && !defined(__CYGWIN__) && !defined(__MINGW32__)
#define CORK_EXPORT  __attribute__((visibility("default")))
#define CORK_IMPORT  __attribute__((visibility("default")))
#define CORK_LOCAL   __attribute__((visibility("hidden")))
#else
#define CORK_EXPORT
#define CORK_IMPORT
#define CORK_LOCAL
#endif


/*
 * Declare a static function that should automatically be called at program
 * startup.
 */

/* TODO: When we implement a full Windows port, [1] describes how best to
 * implement an initialization function under Visual Studio.
 *
 * [1] http://stackoverflow.com/questions/1113409/attribute-constructor-equivalent-in-vc
 */

#if CORK_CONFIG_HAVE_GCC_ATTRIBUTES
#define CORK_INITIALIZER(name) \
__attribute__((constructor)) \
static void \
name(void)
#else
#error "Don't know how to implement initialization functions of this platform"
#endif


#endif /* LIBCORK_CORE_ATTRIBUTES_H */
