/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libcork/core.h"
#include "libcork/ds.h"
#include "libcork/os/subprocess.h"
#include "libcork/helpers/errors.h"
#include "libcork/helpers/mingw.h"

#ifdef __CYGWIN__
#include <cygwin/version.h>
#endif

#if defined(__APPLE__)
/* Apple doesn't provide access to the "environ" variable from a shared library.
 * There's a workaround function to grab the environ pointer described at [1].
 *
 * [1] http://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man7/environ.7.html
 */
#include <crt_externs.h>
#define environ  (*_NSGetEnviron())

#else
/* On all other POSIX platforms, we assume that environ is available in shared
 * libraries. */
extern char  **environ;

#endif


struct cork_env_var {
    const char  *name;
    const char  *value;
};

static struct cork_env_var *
cork_env_var_new(const char *name, const char *value)
{
    struct cork_env_var  *var = cork_new(struct cork_env_var);
    var->name = cork_strdup(name);
    var->value = cork_strdup(value);
    return var;
}

static void
cork_env_var_free(void *vvar)
{
    struct cork_env_var  *var = vvar;
    cork_strfree(var->name);
    cork_strfree(var->value);
    cork_delete(struct cork_env_var, var);
}


struct cork_env {
    struct cork_hash_table  *variables;
    struct cork_buffer  buffer;
};

struct cork_env *
cork_env_new(void)
{
    struct cork_env  *env = cork_new(struct cork_env);
    env->variables = cork_string_hash_table_new(0, 0);
    cork_hash_table_set_free_value(env->variables, cork_env_var_free);
    cork_buffer_init(&env->buffer);
    return env;
}

static void
cork_env_add_internal(struct cork_env *env, const char *name, const char *value)
{
    if (env == NULL) {
        setenv(name, value, true);
    } else {
        struct cork_env_var  *var = cork_env_var_new(name, value);
        void  *old_var;

        cork_hash_table_put
            (env->variables, (void *) var->name, var, NULL, NULL, &old_var);

        if (old_var != NULL) {
            cork_env_var_free(old_var);
        }
    }
}

struct cork_env *
cork_env_clone_current(void)
{
    char  **curr;
    struct cork_env  *env = cork_env_new();

    for (curr = environ; *curr != NULL; curr++) {
        const char  *entry = *curr;
        const char  *equal;

        equal = strchr(entry, '=');
        if (CORK_UNLIKELY(equal == NULL)) {
            /* This environment entry is malformed; skip it. */
            continue;
        }

        /* Make a copy of the name so that it's NUL-terminated rather than
         * equal-terminated. */
        cork_buffer_set(&env->buffer, entry, equal - entry);
        cork_env_add_internal(env, env->buffer.buf, equal + 1);
    }

    return env;
}


void
cork_env_free(struct cork_env *env)
{
    cork_hash_table_free(env->variables);
    cork_buffer_done(&env->buffer);
    cork_delete(struct cork_env, env);
}

const char *
cork_env_get(struct cork_env *env, const char *name)
{
    if (env == NULL) {
        return getenv(name);
    } else {
        struct cork_env_var  *var =
            cork_hash_table_get(env->variables, (void *) name);
        return (var == NULL)? NULL: var->value;
    }
}

void
cork_env_add(struct cork_env *env, const char *name, const char *value)
{
    cork_env_add_internal(env, name, value);
}

void
cork_env_add_vprintf(struct cork_env *env, const char *name,
                     const char *format, va_list args)
{
    cork_buffer_vprintf(&env->buffer, format, args);
    cork_env_add_internal(env, name, env->buffer.buf);
}

void
cork_env_add_printf(struct cork_env *env, const char *name,
                    const char *format, ...)
{
    va_list  args;
    va_start(args, format);
    cork_env_add_vprintf(env, name, format, args);
    va_end(args);
}

void
cork_env_remove(struct cork_env *env, const char *name)
{
    if (env == NULL) {
        unsetenv(name);
    } else {
        void  *old_var;
        cork_hash_table_delete(env->variables, (void *) name, NULL, &old_var);
        if (old_var != NULL) {
            cork_env_var_free(old_var);
        }
    }
}


static enum cork_hash_table_map_result
cork_env_set_vars(void *user_data, struct cork_hash_table_entry *entry)
{
    struct cork_env_var  *var = entry->value;
    setenv(var->name, var->value, false);
    return CORK_HASH_TABLE_MAP_CONTINUE;
}

#if (defined(__APPLE__) || (defined(BSD) && (BSD >= 199103))) && !defined(__GNU__) || (defined(__CYGWIN__) && CYGWIN_VERSION_API_MINOR < 326)
/* A handful of platforms [1] don't provide clearenv(), so we must implement our
 * own version that clears the environ array directly.
 *
 * [1] http://www.gnu.org/software/gnulib/manual/html_node/clearenv.html
 */
static void
clearenv(void)
{
    *environ = NULL;
}

#else
/* Otherwise assume that we have clearenv available. */
#endif

void
cork_env_replace_current(struct cork_env *env)
{
    clearenv();
    cork_hash_table_map(env->variables, NULL, cork_env_set_vars);
}
