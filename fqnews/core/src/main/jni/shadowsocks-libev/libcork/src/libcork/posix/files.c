/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifdef __GNU__
#define _GNU_SOURCE
#endif
#include <assert.h>
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
#include "libcork/ds/array.h"
#include "libcork/ds/buffer.h"
#include "libcork/helpers/errors.h"
#include "libcork/helpers/posix.h"
#include "libcork/helpers/mingw.h"
#include "libcork/os/files.h"
#include "libcork/os/subprocess.h"


#if !defined(CORK_DEBUG_FILES)
#define CORK_DEBUG_FILES  0
#endif

#if CORK_DEBUG_FILES
#include <stdio.h>
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...) /* no debug messages */
#endif


/*-----------------------------------------------------------------------
 * Paths
 */

struct cork_path {
    struct cork_buffer  given;
};

static struct cork_path *
cork_path_new_internal(const char *str, size_t length)
{
    struct cork_path  *path = cork_new(struct cork_path);
    cork_buffer_init(&path->given);
    if (length == 0) {
        cork_buffer_ensure_size(&path->given, 16);
        cork_buffer_set(&path->given, "", 0);
    } else {
        cork_buffer_set(&path->given, str, length);
    }
    return path;
}

struct cork_path *
cork_path_new(const char *source)
{
    return cork_path_new_internal(source, source == NULL? 0: strlen(source));
}

struct cork_path *
cork_path_clone(const struct cork_path *other)
{
    return cork_path_new_internal(other->given.buf, other->given.size);
}

void
cork_path_free(struct cork_path *path)
{
    cork_buffer_done(&path->given);
    cork_delete(struct cork_path, path);
}


void
cork_path_set(struct cork_path *path, const char *content)
{
    if (content == NULL) {
        cork_buffer_clear(&path->given);
    } else {
        cork_buffer_set_string(&path->given, content);
    }
}

const char *
cork_path_get(const struct cork_path *path)
{
    return path->given.buf;
}

#define cork_path_get(path) ((const char *) (path)->given.buf)
#define cork_path_size(path)  ((path)->given.size)
#define cork_path_truncate(path, size) \
    (cork_buffer_truncate(&(path)->given, (size)))


int
cork_path_set_cwd(struct cork_path *path)
{
#ifdef __GNU__
    char *dirname = get_current_dir_name();
    rip_check_posix(dirname);
    cork_buffer_set(&path->given, dirname, strlen(dirname));
    free(dirname);
#else
    cork_buffer_ensure_size(&path->given, PATH_MAX);
    rip_check_posix(getcwd(path->given.buf, PATH_MAX));
    path->given.size = strlen(path->given.buf);
#endif
    return 0;
}

struct cork_path *
cork_path_cwd(void)
{
    struct cork_path  *path = cork_path_new(NULL);
    ei_check(cork_path_set_cwd(path));
    return path;

error:
    cork_path_free(path);
    return NULL;
}


int
cork_path_set_absolute(struct cork_path *path)
{
    struct cork_buffer  buf;

    if (path->given.size > 0 &&
        cork_buffer_char(&path->given, 0) == '/') {
        /* The path is already absolute. */
        return 0;
    }

#ifdef __GNU__
    char *dirname;
    dirname = get_current_dir_name();
    ep_check_posix(dirname);
    cork_buffer_init(&buf);
    cork_buffer_set(&buf, dirname, strlen(dirname));
    free(dirname);
#else
    cork_buffer_init(&buf);
    cork_buffer_ensure_size(&buf, PATH_MAX);
    ep_check_posix(getcwd(buf.buf, PATH_MAX));
    buf.size = strlen(buf.buf);
#endif
    cork_buffer_append(&buf, "/", 1);
    cork_buffer_append_copy(&buf, &path->given);
    cork_buffer_done(&path->given);
    path->given = buf;
    return 0;

error:
    cork_buffer_done(&buf);
    return -1;
}

struct cork_path *
cork_path_absolute(const struct cork_path *other)
{
    struct cork_path  *path = cork_path_clone(other);
    ei_check(cork_path_set_absolute(path));
    return path;

error:
    cork_path_free(path);
    return NULL;
}


void
cork_path_append(struct cork_path *path, const char *more)
{
    if (more == NULL || more[0] == '\0') {
        return;
    }

    if (more[0] == '/') {
        /* If more starts with a "/", then it's absolute, and should replace
         * the contents of the current path. */
        cork_buffer_set_string(&path->given, more);
    } else {
        /* Otherwise, more is relative, and should be appended to the current
         * path.  If the current given path doesn't end in a "/", then we need
         * to add one to keep the path well-formed. */

        if (path->given.size > 0 &&
            cork_buffer_char(&path->given, path->given.size - 1) != '/') {
            cork_buffer_append(&path->given, "/", 1);
        }

        cork_buffer_append_string(&path->given, more);
    }
}

struct cork_path *
cork_path_join(const struct cork_path *other, const char *more)
{
    struct cork_path  *path = cork_path_clone(other);
    cork_path_append(path, more);
    return path;
}

void
cork_path_append_path(struct cork_path *path, const struct cork_path *more)
{
    cork_path_append(path, more->given.buf);
}

struct cork_path *
cork_path_join_path(const struct cork_path *other, const struct cork_path *more)
{
    struct cork_path  *path = cork_path_clone(other);
    cork_path_append_path(path, more);
    return path;
}


void
cork_path_set_basename(struct cork_path *path)
{
    char  *given = path->given.buf;
    const char  *last_slash = strrchr(given, '/');
    if (last_slash != NULL) {
        size_t  offset = last_slash - given;
        size_t  basename_length = path->given.size - offset - 1;
        memmove(given, last_slash + 1, basename_length);
        given[basename_length] = '\0';
        path->given.size = basename_length;
    }
}

struct cork_path *
cork_path_basename(const struct cork_path *other)
{
    struct cork_path  *path = cork_path_clone(other);
    cork_path_set_basename(path);
    return path;
}


void
cork_path_set_dirname(struct cork_path *path)
{
    const char  *given = path->given.buf;
    const char  *last_slash = strrchr(given, '/');
    if (last_slash == NULL) {
        cork_buffer_clear(&path->given);
    } else {
        size_t  offset = last_slash - given;
        if (offset == 0) {
            /* A special case for the immediate subdirectories of "/" */
            cork_buffer_truncate(&path->given, 1);
        } else {
            cork_buffer_truncate(&path->given, offset);
        }
    }
}

struct cork_path *
cork_path_dirname(const struct cork_path *other)
{
    struct cork_path  *path = cork_path_clone(other);
    cork_path_set_dirname(path);
    return path;
}


/*-----------------------------------------------------------------------
 * Lists of paths
 */

struct cork_path_list {
    cork_array(struct cork_path *)  array;
    struct cork_buffer  string;
};

struct cork_path_list *
cork_path_list_new_empty(void)
{
    struct cork_path_list  *list = cork_new(struct cork_path_list);
    cork_array_init(&list->array);
    cork_buffer_init(&list->string);
    return list;
}

void
cork_path_list_free(struct cork_path_list *list)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&list->array); i++) {
        struct cork_path  *path = cork_array_at(&list->array, i);
        cork_path_free(path);
    }
    cork_array_done(&list->array);
    cork_buffer_done(&list->string);
    cork_delete(struct cork_path_list, list);
}

const char *
cork_path_list_to_string(const struct cork_path_list *list)
{
    return list->string.buf;
}

void
cork_path_list_add(struct cork_path_list *list, struct cork_path *path)
{
    cork_array_append(&list->array, path);
    if (cork_array_size(&list->array) > 1) {
        cork_buffer_append(&list->string, ":", 1);
    }
    cork_buffer_append_string(&list->string, cork_path_get(path));
}

size_t
cork_path_list_size(const struct cork_path_list *list)
{
    return cork_array_size(&list->array);
}

const struct cork_path *
cork_path_list_get(const struct cork_path_list *list, size_t index)
{
    return cork_array_at(&list->array, index);
}

static void
cork_path_list_append_string(struct cork_path_list *list, const char *str)
{
    struct cork_path  *path;
    const char  *curr = str;
    const char  *next;

    while ((next = strchr(curr, ':')) != NULL) {
        size_t  size = next - curr;
        path = cork_path_new_internal(curr, size);
        cork_path_list_add(list, path);
        curr = next + 1;
    }

    path = cork_path_new(curr);
    cork_path_list_add(list, path);
}

struct cork_path_list *
cork_path_list_new(const char *str)
{
    struct cork_path_list  *list = cork_path_list_new_empty();
    cork_path_list_append_string(list, str);
    return list;
}


/*-----------------------------------------------------------------------
 * Files
 */

struct cork_file {
    struct cork_path  *path;
    struct stat  stat;
    enum cork_file_type  type;
    bool  has_stat;
};

static void
cork_file_init(struct cork_file *file, struct cork_path *path)
{
    file->path = path;
    file->has_stat = false;
}

struct cork_file *
cork_file_new(const char *path)
{
    return cork_file_new_from_path(cork_path_new(path));
}

struct cork_file *
cork_file_new_from_path(struct cork_path *path)
{
    struct cork_file  *file = cork_new(struct cork_file);
    cork_file_init(file, path);
    return file;
}

static void
cork_file_reset(struct cork_file *file)
{
    file->has_stat = false;
}

static void
cork_file_done(struct cork_file *file)
{
    cork_path_free(file->path);
}

void
cork_file_free(struct cork_file *file)
{
    cork_file_done(file);
    cork_delete(struct cork_file, file);
}

const struct cork_path *
cork_file_path(struct cork_file *file)
{
    return file->path;
}

static int
cork_file_stat(struct cork_file *file)
{
    if (file->has_stat) {
        return 0;
    } else {
        int  rc;
        rc = stat(cork_path_get(file->path), &file->stat);

        if (rc == -1) {
            if (errno == ENOENT || errno == ENOTDIR) {
                file->type = CORK_FILE_MISSING;
                file->has_stat = true;
                return 0;
            } else {
                cork_system_error_set();
                return -1;
            }
        }

        if (S_ISREG(file->stat.st_mode)) {
            file->type = CORK_FILE_REGULAR;
        } else if (S_ISDIR(file->stat.st_mode)) {
            file->type = CORK_FILE_DIRECTORY;
        } else if (S_ISLNK(file->stat.st_mode)) {
            file->type = CORK_FILE_SYMLINK;
        } else {
            file->type = CORK_FILE_UNKNOWN;
        }

        file->has_stat = true;
        return 0;
    }
}

int
cork_file_exists(struct cork_file *file, bool *exists)
{
    rii_check(cork_file_stat(file));
    *exists = (file->type != CORK_FILE_MISSING);
    return 0;
}

int
cork_file_type(struct cork_file *file, enum cork_file_type *type)
{
    rii_check(cork_file_stat(file));
    *type = file->type;
    return 0;
}


struct cork_file *
cork_path_list_find_file(const struct cork_path_list *list,
                         const char *rel_path)
{
    size_t  i;
    size_t  count = cork_path_list_size(list);
    struct cork_file  *file;

    for (i = 0; i < count; i++) {
        const struct cork_path  *path = cork_path_list_get(list, i);
        struct cork_path  *joined = cork_path_join(path, rel_path);
        bool  exists;
        file = cork_file_new_from_path(joined);
        ei_check(cork_file_exists(file, &exists));
        if (exists) {
            return file;
        } else {
            cork_file_free(file);
        }
    }

    cork_error_set_printf
        (ENOENT, "%s not found in %s",
         rel_path, cork_path_list_to_string(list));
    return NULL;

error:
    cork_file_free(file);
    return NULL;
}


/*-----------------------------------------------------------------------
 * Directories
 */

int
cork_file_iterate_directory(struct cork_file *file,
                            cork_file_directory_iterator iterator,
                            void *user_data)
{
    DIR  *dir = NULL;
    struct dirent  *entry;
    size_t  dir_path_size;
    struct cork_path  *child_path;
    struct cork_file  child_file;

    rip_check_posix(dir = opendir(cork_path_get(file->path)));
    child_path = cork_path_clone(file->path);
    cork_file_init(&child_file, child_path);
    dir_path_size = cork_path_size(child_path);

    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip the "." and ".." entries */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        cork_path_append(child_path, entry->d_name);
        ei_check(cork_file_stat(&child_file));

        /* If the entry is a subdirectory, recurse into it. */
        ei_check(iterator(&child_file, entry->d_name, user_data));

        /* Remove this entry name from the path buffer. */
        cork_path_truncate(child_path, dir_path_size);
        cork_file_reset(&child_file);

        /* We have to reset errno to 0 because of the ambiguous way readdir uses
         * a return value of NULL.  Other functions may return normally yet set
         * errno to a non-zero value.  dlopen on Mac OS X is an ogreish example.
         * Since an error readdir is indicated by returning NULL and setting
         * errno to indicate the error, then we need to reset it to zero before
         * each call.  We shall assume, perhaps to our great misery, that
         * functions within this loop do proper error checking and act
         * accordingly. */
        errno = 0;
    }

    /* Check errno immediately after the while loop terminates */
    if (CORK_UNLIKELY(errno != 0)) {
        cork_system_error_set();
        goto error;
    }

    cork_file_done(&child_file);
    rii_check_posix(closedir(dir));
    return 0;

error:
    cork_file_done(&child_file);
    rii_check_posix(closedir(dir));
    return -1;
}

static int
cork_file_mkdir_one(struct cork_file *file, cork_file_mode mode,
                    unsigned int flags)
{
    DEBUG("mkdir %s\n", cork_path_get(file->path));

    /* First check if the directory already exists. */
    rii_check(cork_file_stat(file));
    if (file->type == CORK_FILE_DIRECTORY) {
        DEBUG("  Already exists!\n");
        if (!(flags & CORK_FILE_PERMISSIVE)) {
            cork_system_error_set_explicit(EEXIST);
            return -1;
        } else {
            return 0;
        }
    } else if (file->type != CORK_FILE_MISSING) {
        DEBUG("  Exists and not a directory!\n");
        cork_system_error_set_explicit(EEXIST);
        return -1;
    }

    /* If the caller asked for a recursive mkdir, then make sure the parent
     * directory exists. */
    if (flags & CORK_FILE_RECURSIVE) {
        struct cork_path  *parent = cork_path_dirname(file->path);
        DEBUG("  Checking parent %s\n", cork_path_get(parent));
        if (parent->given.size == 0) {
            /* There is no parent; we're either at the filesystem root (for an
             * absolute path) or the current directory (for a relative one).
             * Either way, we can assume it already exists. */
            cork_path_free(parent);
        } else {
            int  rc;
            struct cork_file  parent_file;
            cork_file_init(&parent_file, parent);
            rc = cork_file_mkdir_one
                (&parent_file, mode, flags | CORK_FILE_PERMISSIVE);
            cork_file_done(&parent_file);
            rii_check(rc);
        }
    }

    /* Create the directory already! */
    DEBUG("  Creating %s\n", cork_path_get(file->path));
    rii_check_posix(mkdir(cork_path_get(file->path), mode));
    return 0;
}

int
cork_file_mkdir(struct cork_file *file, cork_file_mode mode,
                unsigned int flags)
{
    return cork_file_mkdir_one(file, mode, flags);
}

static int
cork_file_remove_iterator(struct cork_file *file, const char *rel_name,
                          void *user_data)
{
    unsigned int  *flags = user_data;
    return cork_file_remove(file, *flags);
}

int
cork_file_remove(struct cork_file *file, unsigned int flags)
{
    DEBUG("rm %s\n", cork_path_get(file->path));
    rii_check(cork_file_stat(file));

    if (file->type == CORK_FILE_MISSING) {
        if (flags & CORK_FILE_PERMISSIVE) {
            return 0;
        } else {
            cork_system_error_set_explicit(ENOENT);
            return -1;
        }
    } else if (file->type == CORK_FILE_DIRECTORY) {
        if (flags & CORK_FILE_RECURSIVE) {
            /* The user asked that we delete the contents of the directory
             * first. */
            rii_check(cork_file_iterate_directory
                      (file, cork_file_remove_iterator, &flags));
        }

        rii_check_posix(rmdir(cork_path_get(file->path)));
        return 0;
    } else {
        rii_check(unlink(cork_path_get(file->path)));
        return 0;
    }
}


/*-----------------------------------------------------------------------
 * Lists of files
 */

struct cork_file_list {
    cork_array(struct cork_file *)  array;
};

struct cork_file_list *
cork_file_list_new_empty(void)
{
    struct cork_file_list  *list = cork_new(struct cork_file_list);
    cork_array_init(&list->array);
    return list;
}

void
cork_file_list_free(struct cork_file_list *list)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&list->array); i++) {
        struct cork_file  *file = cork_array_at(&list->array, i);
        cork_file_free(file);
    }
    cork_array_done(&list->array);
    cork_delete(struct cork_file_list, list);
}

void
cork_file_list_add(struct cork_file_list *list, struct cork_file *file)
{
    cork_array_append(&list->array, file);
}

size_t
cork_file_list_size(struct cork_file_list *list)
{
    return cork_array_size(&list->array);
}

struct cork_file *
cork_file_list_get(struct cork_file_list *list, size_t index)
{
    return cork_array_at(&list->array, index);
}

struct cork_file_list *
cork_file_list_new(struct cork_path_list *path_list)
{
    struct cork_file_list  *list = cork_file_list_new_empty();
    size_t  count = cork_path_list_size(path_list);
    size_t  i;

    for (i = 0; i < count; i++) {
        const struct cork_path  *path = cork_path_list_get(path_list, i);
        struct cork_file  *file = cork_file_new(cork_path_get(path));
        cork_array_append(&list->array, file);
    }

    return list;
}


struct cork_file_list *
cork_path_list_find_files(const struct cork_path_list *path_list,
                          const char *rel_path)
{
    size_t  i;
    size_t  count = cork_path_list_size(path_list);
    struct cork_file_list  *list = cork_file_list_new_empty();
    struct cork_file  *file;

    for (i = 0; i < count; i++) {
        const struct cork_path  *path = cork_path_list_get(path_list, i);
        struct cork_path  *joined = cork_path_join(path, rel_path);
        bool  exists;
        file = cork_file_new_from_path(joined);
        ei_check(cork_file_exists(file, &exists));
        if (exists) {
            cork_file_list_add(list, file);
        } else {
            cork_file_free(file);
        }
    }

    return list;

error:
    cork_file_list_free(list);
    cork_file_free(file);
    return NULL;
}


/*-----------------------------------------------------------------------
 * Standard paths and path lists
 */

#define empty_string(str)  ((str) == NULL || (str)[0] == '\0')

struct cork_path *
cork_path_home(void)
{
    const char  *path = cork_env_get(NULL, "HOME");
    if (empty_string(path)) {
        cork_undefined("Cannot determine home directory");
        return NULL;
    } else {
        return cork_path_new(path);
    }
}


struct cork_path_list *
cork_path_config_paths(void)
{
    struct cork_path_list  *list = cork_path_list_new_empty();
    const char  *var;
    struct cork_path  *path;

    /* The first entry should be the user's configuration directory.  This is
     * specified by $XDG_CONFIG_HOME, with $HOME/.config as the default. */
    var = cork_env_get(NULL, "XDG_CONFIG_HOME");
    if (empty_string(var)) {
        ep_check(path = cork_path_home());
        cork_path_append(path, ".config");
        cork_path_list_add(list, path);
    } else {
        path = cork_path_new(var);
        cork_path_list_add(list, path);
    }

    /* The remaining entries should be the system-wide configuration
     * directories.  These are specified by $XDG_CONFIG_DIRS, with /etc/xdg as
     * the default. */
    var = cork_env_get(NULL, "XDG_CONFIG_DIRS");
    if (empty_string(var)) {
        path = cork_path_new("/etc/xdg");
        cork_path_list_add(list, path);
    } else {
        cork_path_list_append_string(list, var);
    }

    return list;

error:
    cork_path_list_free(list);
    return NULL;
}

struct cork_path_list *
cork_path_data_paths(void)
{
    struct cork_path_list  *list = cork_path_list_new_empty();
    const char  *var;
    struct cork_path  *path;

    /* The first entry should be the user's data directory.  This is specified
     * by $XDG_DATA_HOME, with $HOME/.local/share as the default. */
    var = cork_env_get(NULL, "XDG_DATA_HOME");
    if (empty_string(var)) {
        ep_check(path = cork_path_home());
        cork_path_append(path, ".local/share");
        cork_path_list_add(list, path);
    } else {
        path = cork_path_new(var);
        cork_path_list_add(list, path);
    }

    /* The remaining entries should be the system-wide configuration
     * directories.  These are specified by $XDG_DATA_DIRS, with
     * /usr/local/share:/usr/share as the the default. */
    var = cork_env_get(NULL, "XDG_DATA_DIRS");
    if (empty_string(var)) {
        path = cork_path_new("/usr/local/share");
        cork_path_list_add(list, path);
        path = cork_path_new("/usr/share");
        cork_path_list_add(list, path);
    } else {
        cork_path_list_append_string(list, var);
    }

    return list;

error:
    cork_path_list_free(list);
    return NULL;
}

struct cork_path *
cork_path_user_cache_path(void)
{
    const char  *var;
    struct cork_path  *path;

    /* The user's cache directory is specified by $XDG_CACHE_HOME, with
     * $HOME/.cache as the default. */
    var = cork_env_get(NULL, "XDG_CACHE_HOME");
    if (empty_string(var)) {
        rpp_check(path = cork_path_home());
        cork_path_append(path, ".cache");
        return path;
    } else {
        return cork_path_new(var);
    }
}

struct cork_path *
cork_path_user_runtime_path(void)
{
    const char  *var;

    /* The user's cache directory is specified by $XDG_RUNTIME_DIR, with
     * no default given by the spec. */
    var = cork_env_get(NULL, "XDG_RUNTIME_DIR");
    if (empty_string(var)) {
        cork_undefined("Cannot determine user-specific runtime directory");
        return NULL;
    } else {
        return cork_path_new(var);
    }
}
