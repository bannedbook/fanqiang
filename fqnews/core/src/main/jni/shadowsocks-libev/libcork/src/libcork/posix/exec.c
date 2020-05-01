/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <errno.h>
#include <unistd.h>

#include "libcork/core.h"
#include "libcork/ds.h"
#include "libcork/os/subprocess.h"
#include "libcork/helpers/errors.h"

#define ri_check_posix(call) \
    do { \
        while (true) { \
            if ((call) == -1) { \
                if (errno == EINTR) { \
                    continue; \
                } else { \
                    cork_system_error_set(); \
                    CORK_PRINT_ERROR(); \
                    return -1; \
                } \
            } else { \
                break; \
            } \
        } \
    } while (0)


struct cork_exec {
    const char  *program;
    struct cork_string_array  params;
    struct cork_env  *env;
    const char  *cwd;
    struct cork_buffer  description;
};

struct cork_exec *
cork_exec_new(const char *program)
{
    struct cork_exec  *exec = cork_new(struct cork_exec);
    exec->program = cork_strdup(program);
    cork_string_array_init(&exec->params);
    exec->env = NULL;
    exec->cwd = NULL;
    cork_buffer_init(&exec->description);
    cork_buffer_set_string(&exec->description, program);
    return exec;
}

struct cork_exec *
cork_exec_new_with_params(const char *program, ...)
{
    struct cork_exec  *exec;
    va_list  args;
    const char  *param;

    exec = cork_exec_new(program);
    cork_exec_add_param(exec, program);
    va_start(args, program);
    while ((param = va_arg(args, const char *)) != NULL) {
        cork_exec_add_param(exec, param);
    }
    return exec;
}

struct cork_exec *
cork_exec_new_with_param_array(const char *program, char * const *params)
{
    char * const  *curr;
    struct cork_exec  *exec = cork_exec_new(program);
    for (curr = params; *curr != NULL; curr++) {
        cork_exec_add_param(exec, *curr);
    }
    return exec;
}

void
cork_exec_free(struct cork_exec *exec)
{
    cork_strfree(exec->program);
    cork_array_done(&exec->params);
    if (exec->env != NULL) {
        cork_env_free(exec->env);
    }
    if (exec->cwd != NULL) {
        cork_strfree(exec->cwd);
    }
    cork_buffer_done(&exec->description);
    cork_delete(struct cork_exec, exec);
}

const char *
cork_exec_description(struct cork_exec *exec)
{
    return exec->description.buf;
}

const char *
cork_exec_program(struct cork_exec *exec)
{
    return exec->program;
}

size_t
cork_exec_param_count(struct cork_exec *exec)
{
    return cork_array_size(&exec->params);
}

const char *
cork_exec_param(struct cork_exec *exec, size_t index)
{
    return cork_array_at(&exec->params, index);
}

void
cork_exec_add_param(struct cork_exec *exec, const char *param)
{
    /* Don't add the first parameter to the description; that's a copy of the
     * program name, which we've already added. */
    if (!cork_array_is_empty(&exec->params)) {
        cork_buffer_append(&exec->description, " ", 1);
        cork_buffer_append_string(&exec->description, param);
    }
    cork_array_append(&exec->params, cork_strdup(param));
}

struct cork_env *
cork_exec_env(struct cork_exec *exec)
{
    return exec->env;
}

void
cork_exec_set_env(struct cork_exec *exec, struct cork_env *env)
{
    if (exec->env != NULL) {
        cork_env_free(exec->env);
    }
    exec->env = env;
}

const char *
cork_exec_cwd(struct cork_exec *exec)
{
    return exec->cwd;
}

void
cork_exec_set_cwd(struct cork_exec *exec, const char *directory)
{
    if (exec->cwd != NULL) {
        cork_strfree(exec->cwd);
    }
    exec->cwd = cork_strdup(directory);
}

int
cork_exec_run(struct cork_exec *exec)
{
    const char  **params;

    /* Make sure the parameter array is NULL-terminated. */
    cork_array_append(&exec->params, NULL);
    params = cork_array_elements(&exec->params);

    /* Fill in the requested environment */
    if (exec->env != NULL) {
        cork_env_replace_current(exec->env);
    }

    /* Change the working directory, if requested */
    if (exec->cwd != NULL) {
        ri_check_posix(chdir(exec->cwd));
    }

    /* Execute the new program */
    ri_check_posix(execvp(exec->program, (char * const *) params));

    /* This is unreachable */
    return 0;
}
