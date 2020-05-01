/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "libcork/core/types.h"
#include "libcork/ds/buffer.h"
#include "libcork/os/files.h"

#include "helpers.h"


static const char  *program_path;


/*-----------------------------------------------------------------------
 * Paths
 */

void
verify_path_content(struct cork_path *path, const char *expected)
{
    fail_if(cork_path_get(path) == NULL, "Path should not have NULL content");
    fail_unless_streq("Paths", expected, cork_path_get(path));
}

void
test_path(const char *p, const char *expected)
{
    struct cork_path  *path;
    struct cork_path  *cloned;
    struct cork_path  *set;

    fprintf(stderr, "path(\"%s\") ?= \"%s\"\n",
            (p == NULL)? "": p,
            expected);

    path = cork_path_new(p);
    verify_path_content(path, expected);

    cloned = cork_path_clone(path);
    verify_path_content(cloned, expected);
    cork_path_free(cloned);
    cork_path_free(path);

    set = cork_path_new(NULL);
    cork_path_set(set, p);
    verify_path_content(set, expected);
    cork_path_free(set);
}

START_TEST(test_path_01)
{
    DESCRIBE_TEST;
    test_path(NULL, "");
    test_path("a", "a");
    test_path("a/b", "a/b");
}
END_TEST


void
test_join(const char *p1, const char *p2, const char *expected)
{
    struct cork_path  *path1;
    struct cork_path  *path2;
    struct cork_path  *actual;

    fprintf(stderr, "join(\"%s\", \"%s\") ?= \"%s\"\n",
            (p1 == NULL)? "": p1,
            (p2 == NULL)? "": p2,
            expected);

    /* Try cork_path_join */
    path1 = cork_path_new(p1);
    actual = cork_path_join(path1, p2);
    verify_path_content(actual, expected);
    cork_path_free(path1);
    cork_path_free(actual);

    /* Try cork_path_join_path */
    path1 = cork_path_new(p1);
    path2 = cork_path_new(p2);
    actual = cork_path_join_path(path1, path2);
    verify_path_content(actual, expected);
    cork_path_free(path1);
    cork_path_free(path2);
    cork_path_free(actual);

    /* Try cork_path_append */
    actual = cork_path_new(p1);
    cork_path_append(actual, p2);
    verify_path_content(actual, expected);
    cork_path_free(actual);

    /* Try cork_path_append_path */
    actual = cork_path_new(p1);
    path2 = cork_path_new(p2);
    cork_path_append_path(actual, path2);
    verify_path_content(actual, expected);
    cork_path_free(path2);
    cork_path_free(actual);
}

START_TEST(test_path_join_01)
{
    DESCRIBE_TEST;
    test_join("a", "b",    "a/b");
    test_join("a/", "b",   "a/b");
    test_join("", "a/b",   "a/b");
    test_join("a/b", "",   "a/b");
    test_join(NULL, "a/b", "a/b");
    test_join("a/b", NULL, "a/b");
}
END_TEST

START_TEST(test_path_join_02)
{
    DESCRIBE_TEST;
    test_join("", "/b",   "/b");
    test_join(NULL, "/b", "/b");
    test_join("a", "/b",  "/b");
    test_join("a/", "/b", "/b");
}
END_TEST


void
test_basename(const char *p, const char *expected)
{
    struct cork_path  *path;
    struct cork_path  *actual;

    fprintf(stderr, "basename(\"%s\") ?= \"%s\"\n",
            (p == NULL)? "": p,
            expected);

    /* Try cork_path_basename */
    path = cork_path_new(p);
    actual = cork_path_basename(path);
    verify_path_content(actual, expected);
    cork_path_free(path);
    cork_path_free(actual);

    /* Try cork_path_set_basename */
    actual = cork_path_new(p);
    cork_path_set_basename(actual);
    verify_path_content(actual, expected);
    cork_path_free(actual);
}

START_TEST(test_path_basename_01)
{
    DESCRIBE_TEST;
    test_basename("", "");
    test_basename(NULL, "");
    test_basename("a", "a");
    test_basename("a/", "");
    test_basename("a/b", "b");
    test_basename("a/b/", "");
    test_basename("a/b/c", "c");
    test_basename("/a", "a");
    test_basename("/a/", "");
    test_basename("/a/b", "b");
    test_basename("/a/b/", "");
    test_basename("/a/b/c", "c");
}
END_TEST


void
test_dirname(const char *p, const char *expected)
{
    struct cork_path  *path;
    struct cork_path  *actual;

    fprintf(stderr, "dirname(\"%s\") ?= \"%s\"\n",
            (p == NULL)? "": p,
            expected);

    /* Try cork_path_dirname */
    path = cork_path_new(p);
    actual = cork_path_dirname(path);
    verify_path_content(actual, expected);
    cork_path_free(path);
    cork_path_free(actual);

    /* Try cork_path_set_dirname */
    actual = cork_path_new(p);
    cork_path_set_dirname(actual);
    verify_path_content(actual, expected);
    cork_path_free(actual);
}

START_TEST(test_path_dirname_01)
{
    DESCRIBE_TEST;
    test_dirname(NULL, "");
    test_dirname("", "");
    test_dirname("a", "");
    test_dirname("a/", "a");
    test_dirname("a/b", "a");
    test_dirname("a/b/", "a/b");
    test_dirname("a/b/c", "a/b");
    test_dirname("/", "/");
    test_dirname("/a", "/");
    test_dirname("/a/", "/a");
    test_dirname("/a/b", "/a");
    test_dirname("/a/b/", "/a/b");
    test_dirname("/a/b/c", "/a/b");
}
END_TEST


void
test_relative_child(const char *p, const char *f, const char *expected)
{
    struct cork_path  *actual;

    fprintf(stderr, "relative_child(\"%s\", \"%s\") ?= \"%s\"\n",
            (p == NULL)? "": p,
            (f == NULL)? "": f,
            expected);

    actual = cork_path_new(p);
    cork_path_set_basename(actual);
    cork_path_append(actual, f);
    verify_path_content(actual, expected);
    cork_path_free(actual);
}

START_TEST(test_path_relative_child_01)
{
    DESCRIBE_TEST;
    test_relative_child(NULL, "a", "a");
    test_relative_child("", "a", "a");
    test_relative_child("a", "b", "a/b");
    test_relative_child("a/b", "c", "b/c");
}
END_TEST


void
test_set_absolute(const char *p, const char *expected)
{
    struct cork_path  *actual;

    fprintf(stderr, "set_absolute(\"%s\") ?= \"%s\"\n",
            (p == NULL)? "": p,
            expected);

    actual = cork_path_new(p);
    cork_path_set_absolute(actual);
    if ((p != NULL) && (p[0] == '/')) {
        /* If the first char in p is a '/', then we want to
         * test that already have an absolute path string. */
        verify_path_content(actual, expected);
    } else {
        /* Otherwise, we have to construct a new expected path
         * string using cork_path_cwd since the prepended $ROOT
         * path is unknown. */
        struct cork_path  *root_path;
        struct cork_buffer  *expected_path = cork_buffer_new();
        root_path = cork_path_cwd();
        cork_buffer_append_printf
            (expected_path, "%s/%s", cork_path_get(root_path), expected);
        verify_path_content(actual, (char *) expected_path->buf);
        cork_path_free(root_path);
        cork_buffer_free(expected_path);
    }
    cork_path_free(actual);
}

START_TEST(test_path_set_absolute_01)
{
    /* We test that an absolute path really is so. */
    DESCRIBE_TEST
    test_set_absolute("", "");
    test_set_absolute("/", "/");
    test_set_absolute("/a", "/a");
    test_set_absolute("/a/b", "/a/b");
    test_set_absolute("/a/b/", "/a/b/");
    test_set_absolute("c/d", "c/d");
    test_set_absolute("c/d/", "c/d/");
}
END_TEST


/*-----------------------------------------------------------------------
 * Lists of paths
 */

void
verify_path_list_content(struct cork_path_list *list, const char *expected)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    size_t  count = cork_path_list_size(list);
    size_t  i;

    for (i = 0; i < count; i++) {
        const struct cork_path  *path = cork_path_list_get(list, i);
        cork_buffer_append_string(&buf, cork_path_get(path));
        cork_buffer_append(&buf, "\n", 1);
    }

    fail_unless_streq("path lists", expected, buf.buf);
    cork_buffer_done(&buf);
}

void
test_path_list(const char *p, const char *expected)
{
    struct cork_path_list  *list;

    fprintf(stderr, "path_list(\"%s\")\n", p);

    list = cork_path_list_new(p);
    verify_path_list_content(list, expected);
    fail_unless_streq("path lists", p, cork_path_list_to_string(list));
    cork_path_list_free(list);
}

START_TEST(test_path_list_01)
{
    DESCRIBE_TEST;
    test_path_list("a", "a\n");
    test_path_list("a/b", "a/b\n");
    test_path_list(":a/b", "\na/b\n");
    test_path_list("a:a/b:", "a\na/b\n\n");
}
END_TEST


/*-----------------------------------------------------------------------
 * Files
 */

void
test_file_exists(const char *filename, bool expected)
{
    struct cork_path  *path;
    struct cork_file  *file;
    bool  actual;
    path = cork_path_new(program_path);
    cork_path_set_dirname(path);
    cork_path_append(path, filename);
    file = cork_file_new_from_path(path);
    fail_if_error(cork_file_exists(file, &actual));
    fail_unless(actual == expected, "File %s should%s exist",
                cork_path_get(path), expected? "": " not");
    cork_file_free(file);
}

START_TEST(test_file_exists_01)
{
    DESCRIBE_TEST;
    test_file_exists("embedded-test-files", true);
    test_file_exists("test-nonexistent", false);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("path");

    TCase  *tc_path = tcase_create("path");
    tcase_add_test(tc_path, test_path_01);
    tcase_add_test(tc_path, test_path_join_01);
    tcase_add_test(tc_path, test_path_join_02);
    tcase_add_test(tc_path, test_path_basename_01);
    tcase_add_test(tc_path, test_path_dirname_01);
    tcase_add_test(tc_path, test_path_relative_child_01);
    tcase_add_test(tc_path, test_path_set_absolute_01);
    suite_add_tcase(s, tc_path);

    TCase  *tc_path_list = tcase_create("path-list");
    tcase_add_test(tc_path_list, test_path_list_01);
    suite_add_tcase(s, tc_path_list);

    TCase  *tc_file = tcase_create("file");
    tcase_add_test(tc_file, test_file_exists_01);
    suite_add_tcase(s, tc_file);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    setup_allocator();
    assert(argc > 0);
    program_path = argv[0];
    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
