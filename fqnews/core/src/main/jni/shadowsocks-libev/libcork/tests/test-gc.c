/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "libcork/core/attributes.h"
#include "libcork/core/gc.h"
#include "libcork/core/types.h"
#include "libcork/helpers/gc.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Garbage collector
 */

struct tree {
    int  id;
    struct tree  *left;
    struct tree  *right;
};

_free_(tree) {
    /* nothing to do, I just want to test the macro */
}

_recurse_(tree) {
    struct tree  *self = obj;
    recurse(gc, self->left, ud);
    recurse(gc, self->right, ud);
}

/* test all the variations, even though we're only going to use "tree" */
_gc_(tree);

#define tree0__recurse  tree__recurse
CORK_ATTR_UNUSED static struct cork_gc_obj_iface  tree0__gc;
_gc_no_free_(tree0);

#define tree1__free  tree__free
CORK_ATTR_UNUSED static struct cork_gc_obj_iface  tree1__gc;
_gc_no_recurse_(tree1);

CORK_ATTR_UNUSED static struct cork_gc_obj_iface  tree2__gc;
_gc_leaf_(tree2);

struct tree *
tree_new(int id, struct tree *l, struct tree *r)
{
    struct tree  *self = cork_gc_new(tree);
    self->id = id;
    self->left = cork_gc_incref(l);
    self->right = cork_gc_incref(r);
    return self;
}

START_TEST(test_gc_acyclic_01)
{
    DESCRIBE_TEST;
    cork_gc_init();

    struct tree  *t1 = tree_new(0, NULL, NULL);
    struct tree  *t2 = tree_new(0, NULL, NULL);
    struct tree  *t0 = tree_new(0, t1, t2);

    cork_gc_decref(t1);
    cork_gc_decref(t2);
    cork_gc_decref(t0);

    cork_gc_done();
}
END_TEST

START_TEST(test_gc_cyclic_01)
{
    DESCRIBE_TEST;
    cork_gc_init();

    struct tree  *t1 = tree_new(0, NULL, NULL);
    struct tree  *t2 = tree_new(0, NULL, NULL);
    struct tree  *t0 = tree_new(0, t1, t2);

    t1->left = cork_gc_incref(t0);

    cork_gc_decref(t1);
    cork_gc_decref(t2);
    cork_gc_decref(t0);

    cork_gc_done();
}
END_TEST

START_TEST(test_gc_cyclic_02)
{
    DESCRIBE_TEST;
    cork_gc_init();

    struct tree  *t1 = tree_new(0, NULL, NULL);
    struct tree  *t2 = tree_new(0, NULL, NULL);
    struct tree  *t0 = tree_new(0, t1, t2);

    t1->left = cork_gc_incref(t0);
    t2->left = cork_gc_incref(t2);
    t2->right = cork_gc_incref(t0);

    cork_gc_decref(t1);
    cork_gc_decref(t2);
    cork_gc_decref(t0);

    cork_gc_done();
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("gc");

    TCase  *tc_gc = tcase_create("gc");
    tcase_add_test(tc_gc, test_gc_acyclic_01);
    tcase_add_test(tc_gc, test_gc_cyclic_01);
    tcase_add_test(tc_gc, test_gc_cyclic_02);
    suite_add_tcase(s, tc_gc);

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

