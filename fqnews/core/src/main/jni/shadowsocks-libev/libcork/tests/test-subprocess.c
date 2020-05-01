/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "libcork/config.h"
#include "libcork/core.h"
#include "libcork/ds.h"
#include "libcork/os.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Verifying stream consumer
 */

struct verify_consumer {
    struct cork_stream_consumer  parent;
    struct cork_buffer  buf;
    const char  *name;
    const char  *expected;
};

static int
verify_consumer__data(struct cork_stream_consumer *vself,
                      const void *buf, size_t size, bool is_first)
{
    struct verify_consumer  *self =
        cork_container_of(vself, struct verify_consumer, parent);
    if (is_first) {
        cork_buffer_clear(&self->buf);
    }
    cork_buffer_append(&self->buf, buf, size);
    return 0;
}

static int
verify_consumer__eof(struct cork_stream_consumer *vself)
{
    struct verify_consumer  *self =
        cork_container_of(vself, struct verify_consumer, parent);
    const char  *actual = self->buf.buf;
    if (actual == NULL) {
        actual = "";
    }
    fail_unless(strcmp(actual, self->expected) == 0,
                "Unexpected %s: got\n%s\nexpected\n%s\n", self->name,
                actual, self->expected);
    return 0;
}

static void
verify_consumer__free(struct cork_stream_consumer *vself)
{
    struct verify_consumer  *self =
        cork_container_of(vself, struct verify_consumer, parent);
    cork_buffer_done(&self->buf);
    cork_strfree(self->name);
    cork_strfree(self->expected);
    cork_delete(struct verify_consumer, self);
}

struct cork_stream_consumer *
verify_consumer_new(const char *name, const char *expected)
{
    struct verify_consumer  *self = cork_new(struct verify_consumer);
    self->parent.data = verify_consumer__data;
    self->parent.eof = verify_consumer__eof;
    self->parent.free = verify_consumer__free;
    cork_buffer_init(&self->buf);
    self->name = cork_strdup(name);
    self->expected = cork_strdup(expected);
    return &self->parent;
}


/*-----------------------------------------------------------------------
 * Helpers
 */

struct env {
    const char  *name;
    const char  *value;
};

struct spec {
    char  *program;
    char * const  *params;
    struct env  *env;
    const char  *expected_stdout;
    const char  *expected_stderr;
    int  expected_exit_code;
    struct cork_stream_consumer  *verify_stdout;
    struct cork_stream_consumer  *verify_stderr;
    int  exit_code;
};

static struct cork_env *
test_env(struct env *env_spec)
{
    struct cork_env  *env;
    struct env  *curr;

    if (env_spec == NULL) {
        return NULL;
    }

    env = cork_env_new();
    for (curr = env_spec; curr->name != NULL; curr++) {
        cork_env_add_printf(env, curr->name, "%s", curr->value);
    }

    return env;
}

static void
test_subprocesses_(size_t spec_count, struct spec **specs)
{
    size_t  i;
    struct cork_subprocess_group  *group = cork_subprocess_group_new();

    for (i = 0; i < spec_count; i++) {
        struct spec  *spec = specs[i];
        struct cork_exec *exec;
        struct cork_env  *env = test_env(spec->env);
        struct cork_subprocess  *sub;
        spec->verify_stdout =
            verify_consumer_new("stdout", spec->expected_stdout);
        spec->verify_stderr =
            verify_consumer_new("stderr", spec->expected_stderr);
        fail_if_error(exec = cork_exec_new_with_param_array
                      (spec->program, spec->params));
        cork_exec_set_env(exec, env);
        fail_if_error(sub = cork_subprocess_new_exec
                      (exec, spec->verify_stdout, spec->verify_stderr,
                       &spec->exit_code));
        cork_subprocess_group_add(group, sub);
    }

    fail_if_error(cork_subprocess_group_start(group));
    fail_if_error(cork_subprocess_group_wait(group));

    for (i = 0; i < spec_count; i++) {
        struct spec  *spec = specs[i];
        fail_unless_equal("Exit codes", "%d",
                          spec->expected_exit_code, spec->exit_code);
        cork_stream_consumer_free(spec->verify_stdout);
        cork_stream_consumer_free(spec->verify_stderr);
    }

    cork_subprocess_group_free(group);
}

#define test_subprocesses(specs) \
    test_subprocesses_(sizeof(specs) / sizeof(specs[0]), specs)


/*-----------------------------------------------------------------------
 * Subprocesses
 */

static char  *echo_01_params[] = { "echo", "hello", "world", NULL };
static struct spec  echo_01 = {
    "echo", echo_01_params, NULL, "hello world\n", "", 0
};

static char  *echo_02_params[] = { "echo", "foo", "bar", "baz", NULL };
static struct spec  echo_02 = {
    "echo", echo_02_params, NULL, "foo bar baz\n", "", 0
};

static char  *echo_03_params[] = { "sh", "-c", "echo $CORK_TEST_VAR", NULL };
static struct env  echo_03_env[] = {
    { "CORK_TEST_VAR", "hello world" },
    { NULL }
};
static struct spec  echo_03 = {
    "sh", echo_03_params, echo_03_env, "hello world\n", "", 0
};

static char  *false_01_params[] = { "false", NULL };
static struct spec  false_01 = {
    "false", false_01_params, NULL, "", "", 1
};


START_TEST(test_subprocess_01)
{
    DESCRIBE_TEST;
    struct spec  *specs[] = { &echo_01 };
    test_subprocesses(specs);
}
END_TEST


START_TEST(test_subprocess_02)
{
    DESCRIBE_TEST;
    struct spec  *specs[] = { &echo_02 };
    test_subprocesses(specs);
}
END_TEST


START_TEST(test_subprocess_03)
{
    DESCRIBE_TEST;
    struct spec  *specs[] = { &echo_03 };
    test_subprocesses(specs);
}
END_TEST


START_TEST(test_subprocess_group_01)
{
    DESCRIBE_TEST;
    struct spec  *specs[] = { &echo_01, &echo_02 };
    test_subprocesses(specs);
}
END_TEST


START_TEST(test_subprocess_exit_code_01)
{
    DESCRIBE_TEST;
    struct spec  *specs[] = { &false_01 };
    test_subprocesses(specs);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("subprocess");

    TCase  *tc_subprocess = tcase_create("subprocess");
    tcase_add_test(tc_subprocess, test_subprocess_01);
    tcase_add_test(tc_subprocess, test_subprocess_02);
    tcase_add_test(tc_subprocess, test_subprocess_03);
    tcase_add_test(tc_subprocess, test_subprocess_group_01);
    tcase_add_test(tc_subprocess, test_subprocess_exit_code_01);
    suite_add_tcase(s, tc_subprocess);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    setup_allocator();
    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
