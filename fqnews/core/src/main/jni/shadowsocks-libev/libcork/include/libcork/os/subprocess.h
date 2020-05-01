/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_OS_SUBPROCESS_H
#define LIBCORK_OS_SUBPROCESS_H

#include <stdarg.h>

#include <libcork/core/api.h>
#include <libcork/core/callbacks.h>
#include <libcork/core/types.h>
#include <libcork/ds/stream.h>
#include <libcork/threads/basics.h>


/*-----------------------------------------------------------------------
 * Environments
 */

struct cork_env;

CORK_API struct cork_env *
cork_env_new(void);

CORK_API struct cork_env *
cork_env_clone_current(void);

CORK_API void
cork_env_free(struct cork_env *env);


CORK_API void
cork_env_replace_current(struct cork_env *env);


/* For all of the following, if env is NULL, these functions access or update
 * the actual environment of the current process.  Otherwise, they act on the
 * given environment instance. */

CORK_API const char *
cork_env_get(struct cork_env *env, const char *name);

CORK_API void
cork_env_add(struct cork_env *env, const char *name, const char *value);

CORK_API void
cork_env_add_printf(struct cork_env *env, const char *name,
                    const char *format, ...)
    CORK_ATTR_PRINTF(3,4);

CORK_API void
cork_env_add_vprintf(struct cork_env *env, const char *name,
                     const char *format, va_list args)
    CORK_ATTR_PRINTF(3,0);

CORK_API void
cork_env_remove(struct cork_env *env, const char *name);


/*-----------------------------------------------------------------------
 * Executing another process
 */

struct cork_exec;

CORK_API struct cork_exec *
cork_exec_new(const char *program);

CORK_ATTR_SENTINEL
CORK_API struct cork_exec *
cork_exec_new_with_params(const char *program, ...);

CORK_API struct cork_exec *
cork_exec_new_with_param_array(const char *program, char * const *params);

CORK_API void
cork_exec_free(struct cork_exec *exec);

CORK_API const char *
cork_exec_description(struct cork_exec *exec);

CORK_API const char *
cork_exec_program(struct cork_exec *exec);

CORK_API size_t
cork_exec_param_count(struct cork_exec *exec);

CORK_API const char *
cork_exec_param(struct cork_exec *exec, size_t index);

CORK_API void
cork_exec_add_param(struct cork_exec *exec, const char *param);

/* Can return NULL */
CORK_API struct cork_env *
cork_exec_env(struct cork_exec *exec);

/* Takes control of env */
CORK_API void
cork_exec_set_env(struct cork_exec *exec, struct cork_env *env);

/* Can return NULL */
CORK_API const char *
cork_exec_cwd(struct cork_exec *exec);

CORK_API void
cork_exec_set_cwd(struct cork_exec *exec, const char *directory);

CORK_API int
cork_exec_run(struct cork_exec *exec);


/*-----------------------------------------------------------------------
 * Subprocesses
 */

struct cork_subprocess;

/* If env is NULL, we use the environment variables of the calling process. */

/* Takes control of body */
CORK_API struct cork_subprocess *
cork_subprocess_new(void *user_data, cork_free_f free_user_data,
                    cork_run_f run,
                    struct cork_stream_consumer *stdout_consumer,
                    struct cork_stream_consumer *stderr_consumer,
                    int *exit_code);

/* Takes control of exec */
CORK_API struct cork_subprocess *
cork_subprocess_new_exec(struct cork_exec *exec,
                         struct cork_stream_consumer *stdout_consumer,
                         struct cork_stream_consumer *stderr_consumer,
                         int *exit_code);

CORK_API void
cork_subprocess_free(struct cork_subprocess *sub);

CORK_API struct cork_stream_consumer *
cork_subprocess_stdin(struct cork_subprocess *sub);

CORK_API int
cork_subprocess_start(struct cork_subprocess *sub);

CORK_API bool
cork_subprocess_is_finished(struct cork_subprocess *sub);

CORK_API int
cork_subprocess_abort(struct cork_subprocess *sub);

CORK_API bool
cork_subprocess_drain(struct cork_subprocess *sub);

CORK_API int
cork_subprocess_wait(struct cork_subprocess *sub);


/*-----------------------------------------------------------------------
 * Groups of subprocesses
 */

struct cork_subprocess_group;

CORK_API struct cork_subprocess_group *
cork_subprocess_group_new(void);

CORK_API void
cork_subprocess_group_free(struct cork_subprocess_group *group);

/* Takes control of sub */
CORK_API void
cork_subprocess_group_add(struct cork_subprocess_group *group,
                          struct cork_subprocess *sub);

CORK_API int
cork_subprocess_group_start(struct cork_subprocess_group *group);

CORK_API bool
cork_subprocess_group_is_finished(struct cork_subprocess_group *group);

CORK_API int
cork_subprocess_group_abort(struct cork_subprocess_group *group);

CORK_API bool
cork_subprocess_group_drain(struct cork_subprocess_group *group);

CORK_API int
cork_subprocess_group_wait(struct cork_subprocess_group *group);


#endif /* LIBCORK_OS_SUBPROCESS_H */
