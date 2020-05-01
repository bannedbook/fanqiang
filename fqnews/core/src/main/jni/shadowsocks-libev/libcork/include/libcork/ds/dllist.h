/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_DS_DLLIST_H
#define LIBCORK_DS_DLLIST_H

#include <libcork/core/api.h>
#include <libcork/core/types.h>


struct cork_dllist_item {
    /* A pointer to the next element in the list. */
    struct cork_dllist_item  *next;
    /* A pointer to the previous element in the list. */
    struct cork_dllist_item  *prev;
};


struct cork_dllist {
    /* The sentinel element for this list. */
    struct cork_dllist_item  head;
};

#define CORK_DLLIST_INIT(list)  { { &(list).head, &(list).head } }

#define cork_dllist_init(list) \
    do { \
        (list)->head.next = &(list)->head; \
        (list)->head.prev = &(list)->head; \
    } while (0)



/* DEPRECATED!  Use cork_dllist_foreach or cork_dllist_visit instead. */
typedef void
(*cork_dllist_map_func)(struct cork_dllist_item *element, void *user_data);

CORK_API void
cork_dllist_map(struct cork_dllist *list,
                cork_dllist_map_func func, void *user_data);


typedef int
cork_dllist_visit_f(void *ud, struct cork_dllist_item *element);

CORK_API int
cork_dllist_visit(struct cork_dllist *list, void *ud,
                  cork_dllist_visit_f *visit);


#define cork_dllist_foreach_void(list, curr, _next) \
    for ((curr) = cork_dllist_start((list)), (_next) = (curr)->next; \
         !cork_dllist_is_end((list), (curr)); \
         (curr) = (_next), (_next) = (curr)->next)

#define cork_dllist_foreach(list, curr, _next, etype, element, item_field) \
    for ((curr) = cork_dllist_start((list)), (_next) = (curr)->next, \
         (element) = cork_container_of((curr), etype, item_field); \
         !cork_dllist_is_end((list), (curr)); \
         (curr) = (_next), (_next) = (curr)->next, \
         (element) = cork_container_of((curr), etype, item_field))


CORK_API size_t
cork_dllist_size(const struct cork_dllist *list);


#define cork_dllist_add_after(pred, element) \
    do { \
        (element)->prev = (pred); \
        (element)->next = (pred)->next; \
        (pred)->next->prev = (element); \
        (pred)->next = (element); \
    } while (0)

#define cork_dllist_add_before(succ, element) \
    do { \
        (element)->next = (succ); \
        (element)->prev = (succ)->prev; \
        (succ)->prev->next = (element); \
        (succ)->prev = (element); \
    } while (0)

#define cork_dllist_add_to_head(list, element) \
    cork_dllist_add_after(&(list)->head, (element))

#define cork_dllist_add_to_tail(list, element) \
    cork_dllist_add_before(&(list)->head, (element))

#define cork_dllist_add  cork_dllist_add_to_tail


#define cork_dllist_add_list_to_head(dest, src) \
    do { \
        struct cork_dllist_item  *dest_start = cork_dllist_start(dest); \
        struct cork_dllist_item  *src_start = cork_dllist_start(src); \
        dest_start->prev = &(src)->head; \
        src_start->prev = &(dest)->head; \
        (src)->head.next = dest_start; \
        (dest)->head.next = src_start; \
        cork_dllist_remove(&(src)->head); \
        cork_dllist_init(src); \
    } while (0)

#define cork_dllist_add_list_to_tail(dest, src) \
    do { \
        struct cork_dllist_item  *dest_end = cork_dllist_end(dest); \
        struct cork_dllist_item  *src_end = cork_dllist_end(src); \
        dest_end->next = &(src)->head; \
        src_end->next = &(dest)->head; \
        (src)->head.prev = dest_end; \
        (dest)->head.prev = src_end; \
        cork_dllist_remove(&(src)->head); \
        cork_dllist_init(src); \
    } while (0)


#define cork_dllist_remove(element) \
    do { \
        (element)->prev->next = (element)->next; \
        (element)->next->prev = (element)->prev; \
    } while (0)


#define cork_dllist_is_empty(list) \
    (cork_dllist_is_end((list), cork_dllist_start((list))))


#define cork_dllist_head(list) \
    (((list)->head.next == &(list)->head)? NULL: (list)->head.next)
#define cork_dllist_tail(list) \
    (((list)->head.prev == &(list)->head)? NULL: (list)->head.prev)

#define cork_dllist_start(list) \
    ((list)->head.next)
#define cork_dllist_end(list) \
    ((list)->head.prev)

#define cork_dllist_is_start(list, element) \
    ((element) == &(list)->head)
#define cork_dllist_is_end(list, element) \
    ((element) == &(list)->head)


#endif /* LIBCORK_DS_DLLIST_H */
