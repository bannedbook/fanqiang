.. _visibility:

*****************
Symbol visibility
*****************

.. highlight:: c

::

  #include <libcork/core.h>

When writing a shared library, you should always be careful to explicitly mark
which functions and other symbols are part of the library's public API, and
which should only be used internally by the library.  There are a number of
benefits to doing this; there is a good summary on the `GCC wiki`_.  (Note that
even though that summary is on the GCC wiki, the notes apply equally well to
other compilers and platforms.)

.. _GCC wiki: http://gcc.gnu.org/wiki/Visibility

libcork provides several helper macros that make it easier to do this.  We use
these macros ourselves to define libcork's public API.


Defining a library's public API
-------------------------------

On some platforms (for instance, on Windows), you must declare each public
function and symbol differently depending on whether you're compiling the
library that *defines* the symbol, or a library or program that *uses* the
symbol.  The first is called an *export*, the second an *import*.  On other
platforms (for instance, GCC on Linux), the declaration of a public symbol is
the same regardless of whether you're exporting or importing the symbol.
libcork provides macros that let you explicitly declare a symbol as an export or
an import in a platform-independent way.

.. macro:: CORK_EXPORT
           CORK_IMPORT

   Explicitly declare that a symbol should be exported from the current shared
   library, or imported from some other shared library.

However, you will rarely need to use these macros directly.  Instead, when
writing a new shared library, you should declare a new preprocessor macro
(specific to the new library), which you'll use when declaring the library's
public API.  For instance, if you're creating a new library called
*libfoo*, you would declare a new preprocessor macro called ``FOO_API``::

    #if !defined(FOO_API)
    #define FOO_API  CORK_IMPORT
    #endif

This ensures that anyone who wants to *use* libfoo doesn't need to do anything
special; the ``FOO_API`` macro will default to importing the symbols from
libfoo's public API.

When *building* libfoo, you must set up your build system to define this
variable differently; since you need to *export* the symbols in this case, the
``FOO_API`` macro should be set to ``CORK_EXPORT``.  Each build system will have
a different way to do this.  In CMake, for instance, you'd add the following:

.. code-block:: cmake

    set_target_properties(libfoo PROPERTIES
        COMPILE_DEFINITIONS FOO_API=CORK_EXPORT
    )

Then, in all of your header files, you should use your new ``FOO_API`` macro
when declaring each function or symbol in the public API::

    FOO_API int
    foo_load_from_file(const char *name);

    FOO_API void
    foo_do_something_great(int flags);

    extern FOO_API  const char  *foo_name;


Local symbols
-------------

Normally, if you need a function to be local, and not be exported as part of the
library's public API, you can just declare it ``static``::

    static int
    file_local_function(void)
    {
        /* This function is not visible outside of this file. */
        return 0;
    }

This works great as long as the function is only needed within the current
source file.  Sometimes, though, you need to define a function that can be used
in other source files within the same library, but which shouldn't be visible
outside of the library.  To do this, you should define the function using the
:c:macro:`CORK_LOCAL` macro.

.. macro:: CORK_LOCAL

   Declare a symbol that should be visible in any source file within the current
   library, but not visible outside of the library.

As an example::

    CORK_LOCAL int
    library_local_function(void)
    {
        /* This function is visible in other files, but not outside of the
         * library. */
        return 0;
    }

Since you're going to use this function in multiple files, you'll want to
declare the function in a header file.  However, since the function is not part
of the public API, this should *not* be defined in a public header file (that
is, one that's installed along with the shared library).  Instead, you should
include a private header file that's only available in your library's source
code archive, and which should not be installed with the other public header
files.
