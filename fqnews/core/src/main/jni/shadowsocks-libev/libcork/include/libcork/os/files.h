/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_FILES_H
#define LIBCORK_CORE_FILES_H

#include <libcork/core/api.h>
#include <libcork/core/types.h>


/*-----------------------------------------------------------------------
 * Paths
 */

struct cork_path;

/* path can be relative or absolute */
CORK_API struct cork_path *
cork_path_new(const char *path);

CORK_API struct cork_path *
cork_path_clone(const struct cork_path *other);

CORK_API void
cork_path_free(struct cork_path *path);


CORK_API void
cork_path_set(struct cork_path *path, const char *content);

CORK_API const char *
cork_path_get(const struct cork_path *path);


CORK_API int
cork_path_set_cwd(struct cork_path *path);

CORK_API struct cork_path *
cork_path_cwd(void);


CORK_API int
cork_path_set_absolute(struct cork_path *path);

CORK_API struct cork_path *
cork_path_absolute(const struct cork_path *other);


CORK_API void
cork_path_append(struct cork_path *path, const char *more);

CORK_API void
cork_path_append_path(struct cork_path *path, const struct cork_path *more);

CORK_API struct cork_path *
cork_path_join(const struct cork_path *other, const char *more);

CORK_API struct cork_path *
cork_path_join_path(const struct cork_path *other,
                    const struct cork_path *more);


CORK_API void
cork_path_set_basename(struct cork_path *path);

CORK_API struct cork_path *
cork_path_basename(const struct cork_path *other);


CORK_API void
cork_path_set_dirname(struct cork_path *path);

CORK_API struct cork_path *
cork_path_dirname(const struct cork_path *other);


/*-----------------------------------------------------------------------
 * Lists of paths
 */

struct cork_path_list;

CORK_API struct cork_path_list *
cork_path_list_new_empty(void);

/* list must be a colon-separated list of paths */
CORK_API struct cork_path_list *
cork_path_list_new(const char *list);

CORK_API void
cork_path_list_free(struct cork_path_list *list);

CORK_API const char *
cork_path_list_to_string(const struct cork_path_list *list);

/* Takes control of path.  path must not already be in the list. */
CORK_API void
cork_path_list_add(struct cork_path_list *list, struct cork_path *path);

CORK_API size_t
cork_path_list_size(const struct cork_path_list *list);

/* The list still owns path; you must not free it or modify it. */
CORK_API const struct cork_path *
cork_path_list_get(const struct cork_path_list *list, size_t index);


/*-----------------------------------------------------------------------
 * Files
 */

#define CORK_FILE_RECURSIVE   0x0001
#define CORK_FILE_PERMISSIVE  0x0002

typedef unsigned int  cork_file_mode;

enum cork_file_type {
    CORK_FILE_MISSING = 0,
    CORK_FILE_REGULAR = 1,
    CORK_FILE_DIRECTORY = 2,
    CORK_FILE_SYMLINK = 3,
    CORK_FILE_UNKNOWN = 4
};

struct cork_file;

CORK_API struct cork_file *
cork_file_new(const char *path);

/* Takes control of path */
CORK_API struct cork_file *
cork_file_new_from_path(struct cork_path *path);

CORK_API void
cork_file_free(struct cork_file *file);

/* File owns the result; you should not free it */
CORK_API const struct cork_path *
cork_file_path(struct cork_file *file);

CORK_API int
cork_file_exists(struct cork_file *file, bool *exists);

CORK_API int
cork_file_type(struct cork_file *file, enum cork_file_type *type);


typedef int
(*cork_file_directory_iterator)(struct cork_file *child, const char *rel_name,
                                void *user_data);

CORK_API int
cork_file_iterate_directory(struct cork_file *file,
                            cork_file_directory_iterator iterator,
                            void *user_data);

/* If flags includes CORK_FILE_RECURSIVE, this creates parent directories,
 * if needed.  If flags doesn't include CORK_FILE_PERMISSIVE, then it's an error
 * if the directory already exists. */
CORK_API int
cork_file_mkdir(struct cork_file *file, cork_file_mode mode,
                unsigned int flags);

/* Removes a file or directory.  If file is a directory, and flags contains
 * CORK_FILE_RECURSIVE, then all of the directory's contents are removed, too.
 * Otherwise, the directory must already be empty. */
CORK_API int
cork_file_remove(struct cork_file *file, unsigned int flags);


CORK_API struct cork_file *
cork_path_list_find_file(const struct cork_path_list *list,
                         const char *rel_path);


/*-----------------------------------------------------------------------
 * Lists of files
 */

struct cork_file_list;

CORK_API struct cork_file_list *
cork_file_list_new_empty(void);

CORK_API struct cork_file_list *
cork_file_list_new(struct cork_path_list *path_list);

CORK_API void
cork_file_list_free(struct cork_file_list *list);

/* Takes control of file.  file must not already be in the list. */
CORK_API void
cork_file_list_add(struct cork_file_list *list, struct cork_file *file);

CORK_API size_t
cork_file_list_size(struct cork_file_list *list);

/* The list still owns file; you must not free it.  Editing the file updates the
 * entry in the list. */
CORK_API struct cork_file *
cork_file_list_get(struct cork_file_list *list, size_t index);


CORK_API struct cork_file_list *
cork_path_list_find_files(const struct cork_path_list *list,
                          const char *rel_path);


/*-----------------------------------------------------------------------
 * Walking a directory tree
 */

#define CORK_SKIP_DIRECTORY  1

struct cork_dir_walker {
    int
    (*enter_directory)(struct cork_dir_walker *walker, const char *full_path,
                       const char *rel_path, const char *base_name);

    int
    (*file)(struct cork_dir_walker *walker, const char *full_path,
            const char *rel_path, const char *base_name);

    int
    (*leave_directory)(struct cork_dir_walker *walker, const char *full_path,
                       const char *rel_path, const char *base_name);
};

#define cork_dir_walker_enter_directory(w, fp, rp, bn) \
    ((w)->enter_directory((w), (fp), (rp), (bn)))

#define cork_dir_walker_file(w, fp, rp, bn) \
    ((w)->file((w), (fp), (rp), (bn)))

#define cork_dir_walker_leave_directory(w, fp, rp, bn) \
    ((w)->leave_directory((w), (fp), (rp), (bn)))


CORK_API int
cork_walk_directory(const char *path, struct cork_dir_walker *walker);


/*-----------------------------------------------------------------------
 * Standard paths and path lists
 */

CORK_API struct cork_path *
cork_path_home(void);


CORK_API struct cork_path_list *
cork_path_config_paths(void);

CORK_API struct cork_path_list *
cork_path_data_paths(void);

CORK_API struct cork_path *
cork_path_user_cache_path(void);

CORK_API struct cork_path *
cork_path_user_runtime_path(void);


#endif /* LIBCORK_CORE_FILES_H */
