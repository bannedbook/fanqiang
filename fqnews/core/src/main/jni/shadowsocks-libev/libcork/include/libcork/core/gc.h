/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_GC_REFCOUNT_H
#define LIBCORK_GC_REFCOUNT_H


#include <libcork/core/api.h>
#include <libcork/core/types.h>


struct cork_gc;

/* A callback for recursing through the children of a garbage-collected
 * object. */
typedef void
(*cork_gc_recurser)(struct cork_gc *gc, void *obj, void *ud);

typedef void
(*cork_gc_free_func)(void *obj);

typedef void
(*cork_gc_recurse_func)(struct cork_gc *gc, void *self,
                        cork_gc_recurser recurser, void *ud);

/* An interface that each garbage-collected object must implement. */
struct cork_gc_obj_iface {
    /* Perform additional cleanup; does *NOT* need to deallocate the
     * object itself, or release any child references */
    cork_gc_free_func  free;
    cork_gc_recurse_func  recurse;
};


CORK_API void
cork_gc_init(void);

CORK_API void
cork_gc_done(void);


CORK_API void *
cork_gc_alloc(size_t instance_size, struct cork_gc_obj_iface *iface);

#define cork_gc_new_iface(obj_type, iface) \
    ((obj_type *) \
     (cork_gc_alloc(sizeof(obj_type), (iface))))

#define cork_gc_new(struct_name) \
    (cork_gc_new_iface(struct struct_name, &struct_name##__gc))


CORK_API void *
cork_gc_incref(void *obj);

CORK_API void
cork_gc_decref(void *obj);


#endif /* LIBCORK_GC_REFCOUNT_H */
