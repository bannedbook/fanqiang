/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_HELPERS_REFCOUNT_H
#define LIBCORK_HELPERS_REFCOUNT_H


#include <libcork/core/gc.h>
#include <libcork/core/types.h>


#define _free_(name) \
static void \
name##__free(void *obj)


#define _recurse_(name) \
static void \
name##__recurse(struct cork_gc *gc, void *obj, \
                cork_gc_recurser recurse, void *ud)


#define _gc_(name) \
static struct cork_gc_obj_iface  name##__gc = { \
    name##__free, name##__recurse \
};

#define _gc_no_free_(name) \
static struct cork_gc_obj_iface  name##__gc = { \
    NULL, name##__recurse \
};

#define _gc_no_recurse_(name) \
static struct cork_gc_obj_iface  name##__gc = { \
    name##__free, NULL \
};

#define _gc_leaf_(name) \
static struct cork_gc_obj_iface  name##__gc = { \
    NULL, NULL \
};


#endif /* LIBCORK_HELPERS_REFCOUNT_H */
