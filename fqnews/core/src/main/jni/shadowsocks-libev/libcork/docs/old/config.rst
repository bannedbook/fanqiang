.. _config:

*******************
Configuring libcork
*******************

.. highlight:: c

::

  #include <libcork/config.h>

Several libcork features have different implementations on different
platforms.  Since we want libcork to be easily embeddable into projects
with a wide range of build systems, we try to autodetect which
implementations to use, using only the C preprocessor and the predefined
macros that are available on the current system.

This module provides a layer of indirection, with all of the
preprocessor-based autodetection in one place.  This module's task is to
define a collection of libcork-specific configuration macros, which all
other libcork modules will use to select which implementation to use.

This design also lets you skip the autodetection, and provide values for
the configuration macros directly.  This is especially useful if you're
embedding libcork into another project, and already have a ``configure``
step in your build system that performs platform detection.  See
:c:macro:`CORK_CONFIG_SKIP_AUTODETECT` for details.

.. note::

   The autodetection logic is almost certainly incomplete.  If you need
   to port libcork to another platform, this is where an important chunk
   of edits will take place.  Patches are welcome!


.. _configuration-macros:

Configuration macros
====================

This section lists all of the macros that are defined by libcork's
autodetection logic.  Other libcork modules will use the values of these
macros to choose among the possible implementations.


.. macro:: CORK_CONFIG_VERSION_MAJOR
           CORK_CONFIG_VERSION_MINOR
           CORK_CONFIG_VERSION_PATCH

   The libcork library version, with each part of the version number separated
   out into separate macros.


.. macro:: CORK_CONFIG_VERSION_STRING

   The libcork library version, encoded as a single string.


.. macro:: CORK_CONFIG_REVISION

   The git SHA-1 commit identifier of the libcork version that you're using.


.. macro:: CORK_CONFIG_ARCH_X86
           CORK_CONFIG_ARCH_X64
           CORK_CONFIG_ARCH_PPC

   Exactly one of these macros should be defined to ``1`` to indicate
   the architecture of the current platform.  All of the other macros
   should be defined to ``0`` or left undefined.  The macros correspond
   to the following architectures:

   ============ ================================================
   Macro suffix Architecture
   ============ ================================================
   ``X86``      32-bit Intel (386 or greater)
   ``X64``      64-bit Intel/AMD (AMD64/EM64T, *not* IA-64)
   ``PPC``      32-bit PowerPC
   ============ ================================================


.. macro:: CORK_CONFIG_HAVE_GCC_ASM

   Whether the GCC `inline assembler`_ syntax is available.  (This
   doesn't imply that the compiler is specifically GCC.)  Should be
   defined to ``0`` or ``1``.

   .. _inline assembler: http://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html


.. macro:: CORK_CONFIG_HAVE_GCC_ATTRIBUTES

   Whether the GCC-style syntax for `compiler attributes`_ is available.
   (This doesn't imply that the compiler is specifically GCC.)  Should
   be defined to ``0`` or ``1``.

   .. _compiler attributes: http://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html


.. macro:: CORK_CONFIG_HAVE_GCC_ATOMICS

   Whether GCC-style `atomic intrinsics`_ are available.  (This doesn't
   imply that the compiler is specifically GCC.)  Should be defined to
   ``0`` or ``1``.

   .. _atomic intrinsics: http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html



.. macro:: CORK_CONFIG_HAVE_GCC_INT128

   Whether the GCC-style `128-bit integer`_ types (``__int128`` and ``unsigned
   __int128``) are available.  (This doesn't imply that the compiler is
   specifically GCC.)  Should be defined to ``0`` or ``1``.

   .. _128-bit integer: http://gcc.gnu.org/onlinedocs/gcc/_005f_005fint128.html


.. macro:: CORK_CONFIG_HAVE_GCC_MODE_ATTRIBUTE

   Whether GCC-style `machine modes`_ are available.  (This doesn't imply that
   the compiler is specifically GCC.)  Should be defined to ``0`` or ``1``.

   .. _machine modes: http://gcc.gnu.org/onlinedocs/gcc-4.8.1/gccint/Machine-Modes.html#Machine-Modes


.. macro:: CORK_CONFIG_HAVE_GCC_STATEMENT_EXPRS

   Whether GCC-style `statement expressions`_ are available.
   (This doesn't imply that the compiler is specifically GCC.)  Should
   be defined to ``0`` or ``1``.

   .. _statement expressions: http://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html


.. macro:: CORK_CONFIG_HAVE_REALLOCF

   Whether this platform defines a ``reallocf`` function in
   ``stdlib.h``.  ``reallocf`` is a BSD extension to the standard
   ``realloc`` function that frees the existing pointer if a
   reallocation fails.  If this function exists, we can use it to
   implement :func:`cork_realloc`.


.. macro:: CORK_CONFIG_IS_BIG_ENDIAN
           CORK_CONFIG_IS_LITTLE_ENDIAN

   Whether the current system is big-endian or little-endian.  Exactly
   one of these macros should be defined to ``1``; the other should be
   defined to ``0``.


.. _skipping-autodetection:

Skipping autodetection
======================


.. macro:: CORK_CONFIG_SKIP_AUTODETECT

   If you want to skip libcork's autodetection logic, then you are
   responsible for providing the appropriate values for all of the
   macros defined in :ref:`configuration-macros`.  To do this, have your
   build system define this macro, with a value of ``1``.  This will
   override the default value of ``0`` provided in the
   ``libcork/config/config.h`` header file.

   Then, create (or have your build system create) a
   ``libcork/config/custom.h`` header file.  You can place this file
   anywhere in your header search path.  We will load that file instead
   of libcork's autodetection logic.  Place the appropriate definitions
   for each of the configuration macros into this file.  If needed, you
   can generate this file as part of the ``configure`` step of your
   build system; the only requirement is that it's available once you
   start compiling the libcork source files.
