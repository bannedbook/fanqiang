/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "libcork/core/attributes.h"
#include "libcork/core/error.h"
#include "libcork/core/types.h"
#include "libcork/ds/buffer.h"
#include "libcork/helpers/errors.h"
#include "libcork/helpers/posix.h"
#include "libcork/os/files.h"


static int
cork_walk_one_directory(struct cork_dir_walker *w, struct cork_buffer *path,
                        size_t root_path_size)
{
    DIR  *dir = NULL;
    struct dirent  *entry;
    size_t  dir_path_size;

    rip_check_posix(dir = opendir(path->buf));

    cork_buffer_append(path, "/", 1);
    dir_path_size = path->size;
    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        struct stat  info;

        /* Skip the "." and ".." entries */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Stat the directory entry */
        cork_buffer_append_string(path, entry->d_name);
        ei_check_posix(stat(path->buf, &info));

        /* If the entry is a subdirectory, recurse into it. */
        if (S_ISDIR(info.st_mode)) {
            int  rc = cork_dir_walker_enter_directory
                (w, path->buf, path->buf + root_path_size,
                 path->buf + dir_path_size);
            if (rc != CORK_SKIP_DIRECTORY) {
                ei_check(cork_walk_one_directory(w, path, root_path_size));
                ei_check(cork_dir_walker_leave_directory
                         (w, path->buf, path->buf + root_path_size,
                          path->buf + dir_path_size));
            }
        } else if (S_ISREG(info.st_mode)) {
            ei_check(cork_dir_walker_file
                     (w, path->buf, path->buf + root_path_size,
                      path->buf + dir_path_size));
        }

        /* Remove this entry name from the path buffer. */
        cork_buffer_truncate(path, dir_path_size);

        /* We have to reset errno to 0 because of the ambiguous way
         * readdir uses a return value of NULL.  Other functions may
         * return normally yet set errno to a non-zero value.  dlopen
         * on Mac OS X is an ogreish example.  Since an error readdir
         * is indicated by returning NULL and setting errno to indicate
         * the error, then we need to reset it to zero before each call.
         * We shall assume, perhaps to our great misery, that functions
         * within this loop do proper error checking and act accordingly.
         */
        errno = 0;
    }

    /* Check errno immediately after the while loop terminates */
    if (CORK_UNLIKELY(errno != 0)) {
        cork_system_error_set();
        goto error;
    }

    /* Remove the trailing '/' from the path buffer. */
    cork_buffer_truncate(path, dir_path_size - 1);
    rii_check_posix(closedir(dir));
    return 0;

error:
    if (dir != NULL) {
        rii_check_posix(closedir(dir));
    }
    return -1;
}

int
cork_walk_directory(const char *path, struct cork_dir_walker *w)
{
    int  rc;
    char  *p;
    struct cork_buffer  buf = CORK_BUFFER_INIT();

    /* Seed the buffer with the directory's path, ensuring that there's no
     * trailing '/' */
    cork_buffer_append_string(&buf, path);
    p = buf.buf;
    while (p[buf.size-1] == '/') {
        buf.size--;
        p[buf.size] = '\0';
    }
    rc = cork_walk_one_directory(w, &buf, buf.size + 1);
    cork_buffer_done(&buf);
    return rc;
}
