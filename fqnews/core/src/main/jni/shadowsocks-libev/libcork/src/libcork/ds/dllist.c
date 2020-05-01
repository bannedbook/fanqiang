/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include "libcork/core/api.h"
#include "libcork/core/types.h"
#include "libcork/ds/dllist.h"


/* Include a linkable (but deprecated) version of this to maintain ABI
 * compatibility. */
#undef cork_dllist_init
CORK_API void
cork_dllist_init(struct cork_dllist *list)
{
    list->head.next = &list->head;
    list->head.prev = &list->head;
}


void
cork_dllist_map(struct cork_dllist *list,
                cork_dllist_map_func func, void *user_data)
{
    struct cork_dllist_item  *curr;
    struct cork_dllist_item  *next;
    cork_dllist_foreach_void(list, curr, next) {
        func(curr, user_data);
    }
}

int
cork_dllist_visit(struct cork_dllist *list, void *ud,
                  cork_dllist_visit_f *visit)
{
    struct cork_dllist_item  *curr;
    struct cork_dllist_item  *next;
    cork_dllist_foreach_void(list, curr, next) {
        int  rc = visit(ud, curr);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}


size_t
cork_dllist_size(const struct cork_dllist *list)
{
    size_t  size = 0;
    struct cork_dllist_item  *curr;
    struct cork_dllist_item  *next;
    cork_dllist_foreach_void(list, curr, next) {
        size++;
    }
    return size;
}
