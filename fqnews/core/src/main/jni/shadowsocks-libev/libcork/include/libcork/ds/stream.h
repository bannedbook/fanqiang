/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_DS_STREAM_H
#define LIBCORK_DS_STREAM_H

#include <stdio.h>

#include <libcork/core/api.h>
#include <libcork/core/types.h>


struct cork_stream_consumer {
    int
    (*data)(struct cork_stream_consumer *consumer,
            const void *buf, size_t size, bool is_first_chunk);

    int
    (*eof)(struct cork_stream_consumer *consumer);

    void
    (*free)(struct cork_stream_consumer *consumer);
};


#define cork_stream_consumer_data(consumer, buf, size, is_first) \
    ((consumer)->data((consumer), (buf), (size), (is_first)))

#define cork_stream_consumer_eof(consumer) \
    ((consumer)->eof((consumer)))

#define cork_stream_consumer_free(consumer) \
    ((consumer)->free((consumer)))


CORK_API int
cork_consume_fd(struct cork_stream_consumer *consumer, int fd);

CORK_API int
cork_consume_file(struct cork_stream_consumer *consumer, FILE *fp);

CORK_API int
cork_consume_file_from_path(struct cork_stream_consumer *consumer,
                            const char *path, int flags);


CORK_API struct cork_stream_consumer *
cork_fd_consumer_new(int fd);

CORK_API struct cork_stream_consumer *
cork_file_consumer_new(FILE *fp);

CORK_API struct cork_stream_consumer *
cork_file_from_path_consumer_new(const char *path, int flags);


#endif /* LIBCORK_DS_STREAM_H */
