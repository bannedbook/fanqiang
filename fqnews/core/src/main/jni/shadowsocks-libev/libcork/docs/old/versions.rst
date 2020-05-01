.. _versions:

***************
Library version
***************

.. highlight:: c

::

  #include <libcork/core.h>

The macros and functions in this section let you determine the version of the
libcork library at compile-time and runtime, and make it easier for you to
define similar macros and functions in your own libraries.


libcork version
---------------

.. macro:: CORK_VERSION

   The libcork library version, encoded as a single number as follows::

       (major * 1000000) + (minor * 1000) + patch

   For instance, version 1.2.10 would be encoded as 1002010.

.. macro:: CORK_VERSION_MAJOR
           CORK_VERSION_MINOR
           CORK_VERSION_PATCH

   The libcork library version, with each part of the version number separated
   out into separate macros.


.. function:: const char \*cork_version_string(void)
              const char \*cork_revision_string(void)

   Return the libcork library version or revision as a string.  The *version* is
   the simple three-part version number (``major:minor:patch``).  The
   *revision* is an opaque string that identifies the specific revision in
   libcork's code history.  (Right now, this is a SHA-1 git commit identifier.)


.. tip:: Compile-time vs runtime

   There's an important difference between the :c:macro:`CORK_VERSION` macro and
   :c:func:`cork_version_string` function, even though they seemingly return the
   same information.

   The macro version be evaluated by the preprocessor, and so it will return the
   version that was available *when your code was compiled*.  If you later
   install a newer (but backwards-compatible) version of libcork, any code that
   called the macro will still have the original version, and not the new
   version.

   The function version, on the other hand, calculates the version information
   *at runtime*, when the function is actually called.  That means that the
   function result will always give you the current installed libcork version,
   even as newer versions are installed on the system.


Constructing version information
--------------------------------

If you're writing a library that uses libcork, it's a good idea to provide your
own version information, similar to how libcork does.


.. function:: CORK_MAKE_VERSION(major, minor, patch)

   Assemble a ``major.minor.patch`` version into a single number, using the same
   pattern as :c:macro:`CORK_VERSION`.
