.. _files:

*********************
Files and directories
*********************

.. highlight:: c

::

  #include <libcork/os.h>

The functions in this section let you interact with files and directories in the
local filesystem.


Paths
=====

We provide several functions for constructing and handling paths into the local
filesystem.

.. type:: struct cork_path

   Represents a path in the local filesystem.  The path can be relative or
   absolute.  The paths don't have to refer to existing files or directories.

.. function:: struct cork_path \*cork_path_new(const char \*path)
              struct cork_path \*cork_path_clone(const struct cork_path \*other)

   Construct a new path object from the given path string, or as a copy of
   another path object.

.. function:: void cork_path_free(struct cork_path \*path)

   Free a path object.

.. function:: const char \*cork_path_get(const struct cork_path \*path)

   Return the string content of a path.  This is not normalized in any way.  The
   result is guaranteed to be non-``NULL``, but may refer to an empty string.
   The return value belongs to the path object; you must not modify the contents
   of the string, nor should you try to free the underlying memory.

.. function:: struct cork_path \*cork_path_absolute(const struct cork_path \*other)
              int cork_path_make_absolute(struct cork_path \path)

   Convert a relative path into an absolute path.  The first variant constructs
   a new path object to hold the result; the second variant overwritesthe
   contents of *path*.

   If there is a problem obtaining the current working directory, these
   functions will return an error condition.

.. function:: struct cork_path \*cork_path_join(const struct cork_path \*path, const char \*more)
              struct cork_path \*cork_path_join_path(const struct cork_path \*path, const struct cork_path \*more)
              void \*cork_path_append(struct cork_path \path, const char \*more)
              void \*cork_path_append_path(struct cork_path \*path, const struct cork_path \*more)

   Concatenate two paths together.  The ``join`` variants create a new path
   object containing the concatenated results.  The ``append`` variants
   overwrite the contents of *path* with the concatenated results.


.. function:: struct cork_path \*cork_path_basename(const struct cork_path \*path)
              void \*cork_path_set_basename(struct cork_path \*path)

   Extract the base name of *path*.  This is the portion after the final
   trailing slash.  The first variant constructs a new path object to hold the
   result; the second variant overwritesthe contents of *path*.

   .. note::

      These functions return a different result than the standard
      ``basename(3)`` function.  We consider a trailing slash to be significant,
      whereas ``basename(3)`` does not::

          basename("a/b/c/") == "c"
          cork_path_basename("a/b/c/") == ""

.. function:: struct cork_path \*cork_path_dirname(const struct cork_path \*path)
              void \*cork_path_set_dirname(struct cork_path \*path)

   Extract the directory name of *path*.  This is the portion before the final
   trailing slash.  The first variant constructs a new path object to hold the
   result; the second variant overwritesthe contents of *path*.

   .. note::

      These functions return a different result than the standard ``dirname(3)``
      function.  We consider a trailing slash to be significant, whereas
      ``dirname(3)`` does not::

          dirname("a/b/c/") == "a/b"
          cork_path_dirname("a/b/c/") == "a/b/c"


Lists of paths
==============

.. type:: struct cork_path_list

   A list of paths in the local filesystem.

.. function:: struct cork_path_list \*cork_path_list_new_empty(void)
              struct cork_path_list \*cork_path_list_new(const char \*list)

   Create a new list of paths.  The first variant creates a list that is
   initially empty.  The second variant takes in a colon-separated list of paths
   as a single string, and adds each of those paths to the new list.

.. function:: void cork_path_list_free(struct cork_path_list \*list)

   Free a path list.

.. function:: void cork_path_list_add(struct cork_path_list \*list, struct cork_path \*path)

   Add *path* to *list*.  The list takes control of the path instance; you must
   not try to free *path* yourself.

.. function:: size_t cork_path_list_size(const struct cork_path_list \*list)

   Return the number of paths in *list*.

.. function:: const struct cork_path \*cork_path_list_get(const struct cork_path_list \*list, size_t index)

   Return the path in *list* at the given *index*.  The list still owns the path
   instance that's returned; you must not try to free it or modify its contents.

.. function:: const char \*cork_path_list_to_string(const struct cork_path_list \*list)

   Return a string containing all of the paths in *list* separated by colons.


.. function:: struct cork_file \*cork_path_list_find_file(const struct cork_path_list \*list, const char \*rel_path)
              struct cork_file_list \*cork_path_list_find_files(const struct cork_path_list \*list, const char \*rel_file)

   Search for a file in a list of paths.  *rel_path* gives the path of the
   sought-after file, relative to each of the directories in *list*.

   The first variant returns a :c:type:`cork_file` instance for the first match.
   In no file can be found, it returns ``NULL`` and sets an error condition.

   The second variant returns a :c:type:`cork_file_list` instance containing all
   of the matches.  In no file can be found, we return an empty list.  (Unlike
   the first variant, this is not considered an error.)


Standard paths
==============

.. function:: struct cork_path \*cork_path_home(void)

   Return a :c:type:`cork_path` that refers to the current user's home
   directory.  If we can't determine the current user's home directory, we set
   an error condition and return ``NULL``.

   On POSIX systems, this directory is determined by the ``HOME`` environment
   variable.

.. function:: struct cork_path_list \*cork_path_config_paths(void)

   Return a :c:type:`cork_path_list` that includes all of the standard
   directories that can be used to store configuration files.  This includes a
   user-specific directory that allows the user to override any global
   configuration files.

   On POSIX systems, these directories are defined XDG Base Directory
   Specification.

.. function:: struct cork_path_list \*cork_path_data_paths(void)

   Return a :c:type:`cork_path_list` that includes all of the standard
   directories that can be used to store application data files.  This includes
   a user-specific directory that allows the user to override any global data
   files.

   On POSIX systems, these directories are defined XDG Base Directory
   Specification.

.. function:: struct cork_path \*cork_path_user_cache_path(void)

   Return a :c:type:`cork_path` that refers to a directory that can be used to
   store cache files created on behalf of the current user.  This directory
   should only be used to store data that you can reproduce if needed.

   On POSIX systems, these directories are defined XDG Base Directory
   Specification.

.. function:: struct cork_path \*cork_path_user_runtime_path(void)

   Return a :c:type:`cork_path` that refers to a directory that can be used to
   store small runtime management files on behalf of the current user.

   On POSIX systems, these directories are defined XDG Base Directory
   Specification.


Files
=====

.. type:: struct cork_file

   Represents a file on the local filesystem.  The file in question does not
   necessarily have to exist; you can use :c:type:`cork_file` instances to refer
   to files that you have not yet created, for instance.

.. type:: typedef unsigned int  cork_file_mode

   Represents a Unix-style file permission set.


.. function:: struct cork_file \*cork_file_new(const char \*path)
              struct cork_file \*cork_file_new_from_path(struct cork_path \*path)

   Create a new :c:type:`cork_file` instance to represent the file with the
   given *path*.  The ``_from_path`` variant uses an existing
   :c:type:`cork_path` instance to specify the path.  The new file instance will
   take control of the :c:type`cork_path` instance, so you should not try to
   free it yourself.

.. function:: void cork_file_free(struct cork_file \*file)

   Free a file instance.

.. function:: const struct cork_path \*cork_file_path(struct cork_file \*file)

   Return the path of a file.  The :c:type:`cork_path` instance belongs to the
   file; you must not try to modify or free the path instance.

.. function:: int cork_file_exists(struct cork_file \*file, bool \*exists)

   Check whether a file exists in the filesystem, storing the result in
   *exists*.  The function returns an error condition if we are unable to
   determine whether the file exists --- for instance, because you do not have
   permission to look into one of the containing directories.

.. function:: int cork_file_type(struct cork_file \*file, enum cork_file_type \*type)

   Return what kind of file the given :c:type:`cork_file` instance refers to.
   The function returns an error condition if there is an error accessing the
   file --- for instance, because you do not have permission to look into one of
   the containing directories.

   If the function succeeds, it will fill in *type* with one of the following
   values:

   .. type:: enum cork_file_type

      .. member:: CORK_FILE_MISSING

         *file* does not exist.

      .. member:: CORK_FILE_REGULAR

         *file* is a regular file.

      .. member:: CORK_FILE_DIRECTORY

         *file* is a directory.

      .. member:: CORK_FILE_SYMLINK

         *file* is a symbolic link.

      .. member:: CORK_FILE_UNKNOWN

         We can access *file*, but we do not know what type of file it is.


.. function:: int cork_file_remove(struct cork_file \*file, unsigned int flags)

   Remove *file* from the filesystem.  *flags* must be the bitwise OR (``|``) of
   the following flags.  (Use ``0`` if you do not want any of the flags.)

   .. macro:: CORK_FILE_PERMISSIVE

      If this flag is given, then it is not considered an error if *file* does
      not exist.  If the flag is not given, then the function function returns
      an error if *file* doesn't exist.  (This mimics the standard ``rm -f``
      command.)

   .. macro:: CORK_FILE_RECURSIVE

      If this flag is given, and *file* refers to a directory, then the function
      will automatically remove the directory and all of its contents.  If the
      flag is not given, and *file* refers to a directory, then the directory
      must be empty for this function to succeed.  If *file* does not refer to a
      directory, this flag has no effect.  (This mimics the standard ``rmdir
      -r`` command.)


Directories
===========

Certain functions can only be applied to a :c:type:`cork_file` instance that
refers to a directory.


.. function:: int cork_file_mkdir(struct cork_file \*directory, cork_file_mode mode, unsigned int flags)

   Create a new directory in the filesystem, with permissions given by *mode*.
   *flags* must be the bitwise OR (``|``) of the following flags.  (Use ``0`` if
   you do not want any of the flags.)

   .. macro:: CORK_FILE_PERMISSIVE

      If this flag is given, then it is not considered an error if *directory*
      already exists.  If the flag is not given, then the function function
      returns an error if *directory* exists.  (This mimics part of the standard
      ``mkdir -p`` command.)

   .. macro:: CORK_FILE_RECURSIVE

      If this flag is given, then the function will ensure that all of the
      parent directories of *directory* exist, creating them if necessary.  Each
      directory created will have permissions given by *mode*.  (This mimics
      part of the standard ``mkdir -p`` command.)


.. function:: int cork_file_iterate_directory(struct cork_file \*directory, cork_file_directory_iterator iterator, void \*user_data)

   Call *iterator* for each file or subdirectory contained in *directory* (not
   including the directory's ``.`` and ``..`` entries).  This function does not
   recurse into any subdirectories; it only iterates through the immediate
   children of *directory*.

   If your iteration function returns a non-zero result, we will abort the
   iteration and return that value.  Otherwise, if each call to the iteration
   function returns ``0``, then we will return ``0`` as well.

   *iterator* must be an instance of the following function type:

   .. type:: typedef int (\*cork_file_directory_iterator)(struct cork_file \*child, const char \*rel_name, void \*user_data)

      Called for each child entry in *directory*.  *child* will be a file
      instance referring to the child entry.  *rel_name* gives the relative name
      of the child entry within its parent *directory*.


Lists of files
==============

.. type:: struct cork_file_list

   A list of files in the local filesystem.

.. function:: struct cork_file_list \*cork_file_list_new_empty(void)
              struct cork_file_list \*cork_file_list_new(struct cork_path_list \*path_list)

   Create a new list of files.  The first variant creates a list that is
   initially empty.  The second variant adds a new file instance for each of the
   paths in *path_list*.

.. function:: void cork_file_list_free(struct cork_file_list \*list)

   Free a file list.

.. function:: void cork_file_list_add(struct cork_file_list \*list, struct cork_file \*file)

   Add *file* to *list*.  The list takes control of the file instance; you must
   not try to free *file* yourself.

.. function:: size_t cork_file_list_size(const struct cork_file_list \*list)

   Return the number of files in *list*.

.. function:: struct cork_file \*cork_file_list_get(const struct cork_file_list \*list, size_t index)

   Return the file in *list* at the given *index*.  The list still owns the file
   instance that's returned; you must not try to free it.



Directory walking
=================

.. function:: int cork_walk_directory(const char \*path, struct cork_dir_walker \*walker)

   Walk through the contents of a directory.  *path* can be an absolute or
   relative path.  If it's relative, it will be interpreted relative to the
   current directory.  If *path* doesn't exist, or there are any problems
   reading the contents of the directory, we'll set an error condition and
   return ``-1``.

   To process the contents of the directory, you must provide a *walker* object,
   which contains several callback methods that we will call when files and
   subdirectories of *path* are encountered.  Each method should return ``0`` on
   success.  Unless otherwise noted, if we receive any other return result, we
   will abort the directory walk, and return that same result from the
   :c:func:`cork_walk_directory` call itself.

   In all of the following methods, *base_name* will be the base name of the
   entry within its immediate subdirectory.  *rel_path* will be the relative
   path of the entry within the *path* that you originally asked to walk
   through.  *full_path* will the full path to the entry, including *path*
   itself.

   .. type:: struct cork_dir_walker

      .. member:: int (\*file)(struct cork_dir_walker \*walker, const char \*full_path, const char \*rel_path, const char \*base_name)

         Called when a regular file is encountered.

      .. member:: int (\*enter_directory)(struct cork_dir_walker \*walker, const char \*full_path, const char \*rel_path, const char \*base_name)

         Called when a subdirectory of *path* of encountered.  If you don't want
         to recurse into this directory, return :c:data:`CORK_SKIP_DIRECTORY`.

         .. macro:: CORK_SKIP_DIRECTORY

      .. member:: int (\*leave_directory)(struct cork_dir_walker \*walker, const char \*full_path, const char \*rel_path, const char \*base_name)

         Called when a subdirectory has been fully processed.
