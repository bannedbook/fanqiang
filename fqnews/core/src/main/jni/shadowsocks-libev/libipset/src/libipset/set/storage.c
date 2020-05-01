/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libcork/core.h>

#include "ipset/bdd/nodes.h"
#include "ipset/errors.h"
#include "ipset/ipset.h"


static void
create_errno_error(FILE *stream)
{
    if (ferror(stream)) {
        cork_error_set(IPSET_ERROR, IPSET_IO_ERROR, "%s", strerror(errno));
    } else {
        cork_unknown_error();
    }
}

struct file_consumer {
    /* file_consumer is a subclass of cork_stream_consumer */
    struct cork_stream_consumer  parent;
    /* the file to write the data into */
    FILE  *fp;
};

static int
file_consumer_data(struct cork_stream_consumer *vself,
                   const void *buf, size_t size, bool is_first)
{
    struct file_consumer  *self =
        cork_container_of(vself, struct file_consumer, parent);
    size_t  bytes_written = fwrite(buf, 1, size, self->fp);
    /* If there was an error writing to the file, then signal this to
     * the producer */
    if (bytes_written == size) {
        return 0;
    } else {
        create_errno_error(self->fp);
        return -1;
    }
}

static int
file_consumer_eof(struct cork_stream_consumer *vself)
{
    /* We don't close the file, so there's nothing special to do at
     * end-of-stream. */
    return 0;
}

int
ipset_save_to_stream(struct cork_stream_consumer *stream,
                     const struct ip_set *set)
{
    return ipset_node_cache_save(stream, set->cache, set->set_bdd);
}

int
ipset_save(FILE *fp, const struct ip_set *set)
{
    struct file_consumer  stream = {
        { file_consumer_data, file_consumer_eof, NULL }, fp
    };
    return ipset_save_to_stream(&stream.parent, set);
}


int
ipset_save_dot(FILE *fp, const struct ip_set *set)
{
    struct file_consumer  stream = {
        { file_consumer_data, file_consumer_eof, NULL }, fp
    };
    return ipset_node_cache_save_dot(&stream.parent, set->cache, set->set_bdd);
}


struct ip_set *
ipset_load(FILE *stream)
{
    struct ip_set  *set;
    ipset_node_id  new_bdd;

    set = ipset_new();
    new_bdd = ipset_node_cache_load(stream, set->cache);
    if (cork_error_occurred()) {
        ipset_free(set);
        return NULL;
    }

    set->set_bdd = new_bdd;
    return set;
}
