/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "libcork/ds/stream.h"
#include "libcork/helpers/errors.h"
#include "libcork/helpers/posix.h"

#define BUFFER_SIZE  4096


/*-----------------------------------------------------------------------
 * Producers
 */

int
cork_consume_fd(struct cork_stream_consumer *consumer, int fd)
{
    char  buf[BUFFER_SIZE];
    ssize_t  bytes_read;
    bool  first = true;

    while (true) {
        while ((bytes_read = read(fd, buf, BUFFER_SIZE)) > 0) {
            rii_check(cork_stream_consumer_data
                      (consumer, buf, bytes_read, first));
            first = false;
        }

        if (bytes_read == 0) {
            return cork_stream_consumer_eof(consumer);
        } else if (errno != EINTR) {
            cork_system_error_set();
            return -1;
        }
    }
}

int
cork_consume_file(struct cork_stream_consumer *consumer, FILE *fp)
{
    char  buf[BUFFER_SIZE];
    size_t  bytes_read;
    bool  first = true;

    while (true) {
        while ((bytes_read = fread(buf, 1, BUFFER_SIZE, fp)) > 0) {
            rii_check(cork_stream_consumer_data
                      (consumer, buf, bytes_read, first));
            first = false;
        }

        if (feof(fp)) {
            return cork_stream_consumer_eof(consumer);
        } else if (errno != EINTR) {
            cork_system_error_set();
            return -1;
        }
    }
}

int
cork_consume_file_from_path(struct cork_stream_consumer *consumer,
                            const char *path, int flags)
{
    int  fd;
    rii_check_posix(fd = open(path, flags));
    ei_check(cork_consume_fd(consumer, fd));
    rii_check_posix(close(fd));
    return 0;

error:
    rii_check_posix(close(fd));
    return -1;
}


/*-----------------------------------------------------------------------
 * Consumers
 */

struct cork_file_consumer {
    struct cork_stream_consumer  parent;
    FILE  *fp;
};

static int
cork_file_consumer__data(struct cork_stream_consumer *vself,
                         const void *buf, size_t size, bool is_first)
{
    struct cork_file_consumer  *self =
        cork_container_of(vself, struct cork_file_consumer, parent);
    size_t  bytes_written = fwrite(buf, 1, size, self->fp);
    /* If there was an error writing to the file, then signal this to
     * the producer */
    if (bytes_written == size) {
        return 0;
    } else {
        cork_system_error_set();
        return -1;
    }
}

static int
cork_file_consumer__eof(struct cork_stream_consumer *vself)
{
    /* We never close the file with this version of the consumer, so there's
     * nothing special to do at end-of-stream. */
    return 0;
}

static void
cork_file_consumer__free(struct cork_stream_consumer *vself)
{
    struct cork_file_consumer  *self =
        cork_container_of(vself, struct cork_file_consumer, parent);
    cork_delete(struct cork_file_consumer, self);
}

struct cork_stream_consumer *
cork_file_consumer_new(FILE *fp)
{
    struct cork_file_consumer  *self = cork_new(struct cork_file_consumer);
    self->parent.data = cork_file_consumer__data;
    self->parent.eof = cork_file_consumer__eof;
    self->parent.free = cork_file_consumer__free;
    self->fp = fp;
    return &self->parent;
}


struct cork_fd_consumer {
    struct cork_stream_consumer  parent;
    int  fd;
};

static int
cork_fd_consumer__data(struct cork_stream_consumer *vself,
                       const void *buf, size_t size, bool is_first)
{
    struct cork_fd_consumer  *self =
        cork_container_of(vself, struct cork_fd_consumer, parent);
    size_t  bytes_left = size;

    while (bytes_left > 0) {
        ssize_t  rc = write(self->fd, buf, bytes_left);
        if (rc == -1 && errno != EINTR) {
            cork_system_error_set();
            return -1;
        } else {
            bytes_left -= rc;
            buf += rc;
        }
    }

    return 0;
}

static int
cork_fd_consumer__eof_close(struct cork_stream_consumer *vself)
{
    int  rc;
    struct cork_fd_consumer  *self =
        cork_container_of(vself, struct cork_fd_consumer, parent);
    rii_check_posix(rc = close(self->fd));
    return 0;
}

static void
cork_fd_consumer__free(struct cork_stream_consumer *vself)
{
    struct cork_fd_consumer  *self =
        cork_container_of(vself, struct cork_fd_consumer, parent);
    cork_delete(struct cork_fd_consumer, self);
}

struct cork_stream_consumer *
cork_fd_consumer_new(int fd)
{
    struct cork_fd_consumer  *self = cork_new(struct cork_fd_consumer);
    self->parent.data = cork_fd_consumer__data;
    /* We don't want to close fd, so we reuse file_consumer's eof method */
    self->parent.eof = cork_file_consumer__eof;
    self->parent.free = cork_fd_consumer__free;
    self->fd = fd;
    return &self->parent;
}

struct cork_stream_consumer *
cork_file_from_path_consumer_new(const char *path, int flags)
{

    int  fd;
    struct cork_fd_consumer  *self;

    rpi_check_posix(fd = open(path, flags));
    self = cork_new(struct cork_fd_consumer);
    self->parent.data = cork_fd_consumer__data;
    self->parent.eof = cork_fd_consumer__eof_close;
    self->parent.free = cork_fd_consumer__free;
    self->fd = fd;
    return &self->parent;
}
