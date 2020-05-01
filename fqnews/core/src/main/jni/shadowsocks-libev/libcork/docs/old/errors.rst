.. _errors:

***************
Error reporting
***************

.. highlight:: c

::

  #include <libcork/core.h>

This section defines an API for reporting error conditions.  It's loosely
modeled on the POSIX ``errno`` mechanism.

The standard POSIX approach for reporting errors is to return an integer status
code, and to store error codes into the ``errno`` global variable.  This
approach has a couple of drawbacks.  The first is that you --- or really, your C
library --- has to ensure that ``errno`` is placed in thread-local storage, so
that separate threads have their own error condition variables.  The second, and
in our mind more important, is that the set of error codes is fixed and
platform-dependent.  It's difficult to add new error codes to represent
application-level error conditions.

The libcork error API is a way around this.  Like standard POSIX-conforming
functions, you return an integer status code from any function that might need
to report an error to its caller.  The status return code is simple: ``0``
indicates success, ``-1`` indicates failure.

When an error occurs, you can use the functions in this section to get more
information about the error: an *error code*, and human-readable string
description of the error.  The POSIX ``errno`` values, while hard to extend, are
perfectly well-defined for most platforms; therefore, any ``errno`` value
supported by your system's C library is a valid libcork error code.  To support
new application-specific error codes, an error code can also be the hash of some
string describing the error.  This “hash of a string” approach makes it easy to
define new error codes without needing any centralized mechanism for assigning
IDs to the various codes.  Moreover, it's very unlikely that a hashed error code
will conflict with some existing POSIX ``errno`` value, or with any other hashed
error codes.

.. note::

   We correctly maintain a separate error condition for each thread in
   the current process.  This is all hidden by the functions in this
   section; it's safe to call them from multiple threads simultaneously.


Calling a function that can return an error
-------------------------------------------

There are two basic forms for a function that can produce an error.  The
first is if the function returns a single pointer as its result::

  TYPE *
  my_function(/* parameters */);

The second is for any other function::

  int
  my_function(/* parameters */);

If an error occurs, the function will return either ``NULL`` or ``-1``,
depending on its return type.  Success will be indicated by a non-\
``NULL`` pointer or a ``0``.  (More complex return value schemes are
possible, if the function needs to signal more than a simple “success”
or “failure”; in that case, you'll need to check the function's
documentation for details.)

If you want to know specifics about the error, there are several
functions that you can use to interrogate the current error condition.

.. function:: bool cork_error_occurred(void)

   Returns whether an error has occurred.

.. function:: cork_error cork_error_code(void)

   Returns the error code of the current error condition.  If no error has
   occurred, the result will be :c:macro:`CORK_ERROR_NONE`.

.. function:: const char \*cork_error_message(void)

   Returns the human-readable string description the current error
   condition.  If no error occurred, the result of this function is
   undefined.

You can use the ``cork_error_prefix`` family of functions to add additional
context to the beginning of an error message.

.. function:: void cork_error_prefix_printf(const char \*format, ...)
              void cork_error_prefix_string(const char \*string)
              void cork_error_prefix_vprintf(const char \*format, va_list args)

   Prepends some additional text to the current error condition.

When you're done checking the current error condition, you clear it so
that later calls to :c:func:`cork_error_occurred` and friends don't
re-report this error.

.. function:: void cork_error_clear(void)

   Clears the current error condition.


Writing a function that can return an error
-------------------------------------------

When writing a function that might produce an error condition, your
function signature should follow one of the two standard patterns
described above::

  int
  my_function(/* parameters */);

  TYPE *
  my_function(/* parameters */);

You should return ``-1`` or ``NULL`` if an error occurs, and ``0`` or a
non-\ ``NULL`` pointer if it succeeds.  If ``NULL`` is a valid
“successful” result of the function, you should use the first form, and
define a ``TYPE **`` output parameter to return the actual pointer
value.  (If you're using the first form, you can use additional return
codes if there are other possible results besides a simple “success” and
“failure”.)

If your function results in an error, you need to fill in the current
error condition using the ``cork_error_set`` family of functions:

.. function:: void cork_error_set_printf(cork_error ecode, const char \*format, ...)
              void cork_error_set_string(cork_error ecode, const char \*string)
              void cork_error_set_vprintf(cork_error ecode, const char \*format, va_list args)

   Fills in the current error condition.  The error condition is defined
   by the error code *ecode*.  The human-readable description is constructed
   from *string*, or from *format* and any additional parameters, depending on
   which variant you use.

As an example, the :ref:`IP address <net-addresses>` parsing functions fill in
:c:macro:`CORK_PARSE_ERROR` error conditions when you try to parse a malformed
address::

  const char  *str = /* the string that's being parsed */;
  cork_error_set_printf
      (CORK_PARSE_ERROR, "Invalid IP address: %s", str);

If a particular kind of error can be raised in several places
throughout your code, it can be useful to define a helper function for
filling in the current error condition::

  static void
  cork_ip_address_parse_error(const char *version, const char *str)
  {
      cork_error_set_printf
          (CORK_PARSE_ERROR, "Invalid %s address: %s", version, str);
  }


Error-checking macros
---------------------

There can be a lot of repetitive code when calling functions that return
error conditions.  We provide a collection of helper macros that make it
easier to write this code.

.. note::

   Unlike most libcork modules, these macros are **not** automatically
   defined when you include the ``libcork/core.h`` header file, since
   they don't include a ``cork_`` prefix.  Because of this, we don't
   want to pollute your namespace unless you ask for the macros.  To do
   so, you must explicitly include their header file::

     #include <libcork/helpers/errors.h>

Additional debugging output
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. macro:: CORK_PRINT_ERRORS

   If you define this macro to ``1`` before including
   :file:`libcork/helpers/errors.h`, then we'll output the current
   function name, file, and line number, along with the description of
   the error, to stderr whenever an error is detected by one of the
   macros described in this section.

Returning a default error code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you follow one of the standard function signature patterns described
above, then your function will either return an ``int`` or some pointer
type, and errors will be signalled by a return value of ``-1`` or
``NULL``.  If so, you can use the macros in this section to
automatically return the appropriate error return value if a nested
function call returns an error.

With these macros, you won't have a chance to inspect the error
condition when an error occurs, so you should pass in your own *err*
parameter when calling the nested function.

(The mnemonic for remembering these macro names is that they all start
with ``rXY_``.  The ``r`` indicates that they automatically “return”.
The second character indicates whether *your* function returns an
``int`` or a pointer.  The third character indicates whether the
function you're *calling* returns an ``int`` or a pointer.)

.. function:: void rie_check(call)

   Call a function whose return value isn't enough to check for an error, when
   your function returns an ``int``.  We'll use :c:func:`cork_error_occurred` to
   check for an error.  If the nested function call returns an error, we
   propagate that error on.

.. function:: void rii_check(call)

   Call a function that returns an ``int`` error indicator, when your
   function also returns an ``int``.  If the nested function call
   returns an error, we propagate that error on.

.. function:: void rip_check(call)

   Call a function that returns a pointer, when your function returns an
   ``int``.  If the nested function call returns an error, we propagate
   that error on.

.. function:: void rpe_check(call)

   Call a function whose return value isn't enough to check for an error, when
   your function returns a pointer.  We'll use :c:func:`cork_error_occurred` to
   check for an error.  If the nested function call returns an error, we
   propagate that error on.

.. function:: void rpi_check(call)

   Call a function that returns an ``int`` error indicator, when your
   function returns a pointer.  If the nested function call returns an
   error, we propagate that error on.

.. function:: void rpp_check(call)

   Call a function that returns a pointer, when your function also
   returns a pointer.  If the nested function call returns an error, we
   propagate that error on.

Returning a non-standard return value
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If your function doesn't have a standard signature, or it uses
additional return values besides ``0``, ``1``, ``NULL``, and valid
pointers, then you can use the macros in this section to return a custom
return value in case of an error.

With these macros, you won't have a chance to inspect the error
condition when an error occurs, so you should pass in your own *err*
parameter when calling the nested function.

(The mnemonic for remembering these macro names is that they all start
with ``xY_``.  The ``x`` doesn't standard for anything in particular.
The second character indicates whether the function you're *calling*
returns an ``int`` or a pointer.  We don't need separate macros for
*your* function's return type, since you provide a return value
explicitly.)

.. function:: void xe_check(retval, call)

   Call a function whose return value isn't enough to check for an error.  If
   the nested function call raises an error, we propagate that error on, and
   return *retval* from the current function.

.. function:: void xi_check(retval, call)

   Call a function that returns an ``int`` error indicator.  If the
   nested function call raises an error, we propagate that error on, and
   return *retval* from the current function.

.. function:: void xp_check(retval, call)

   Call a function that returns a pointer.  If the nested function call
   raises an error, we propagate that error on, and return *retval* from
   the current function.

Post-processing when an error occurs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you need to perform some post-processing when a nested function
returns an error, you can use the functions in this section.  They will
automatically jump to the current scope's ``error`` label whenever an
error occurs.

(The mnemonic for remembering these macro names is that they all start
with ``eY_``.  The ``e`` indicates that they'll jump to the ``error``
label.  The second character indicates whether the function you're
*calling* returns an ``int`` or a pointer.  We don't need separate
macros for *your* function's return type, since the macros won't
automatically return anything.)

.. function:: void ei_check(call)

   Call a function whose return value isn't enough to check for an error.  If
   the nested function call raises an error, we automatically jump to the
   current scope's ``error`` label.

.. function:: void ei_check(call)

   Call a function that returns an ``int`` error indicator.  If the
   nested function call raises an error, we automatically jump to the
   current scope's ``error`` label.

.. function:: void ep_check(call)

   Call a function that returns a pointer.  If the nested function call
   raises an error, we automatically jump to the current scope's
   ``error`` label.


Calling POSIX functions
~~~~~~~~~~~~~~~~~~~~~~~

The :c:func:`cork_system_error_set` function automatically translates a POSIX
error (specified in the standard ``errno`` variable) into a libcork error
condition (which will be reported by :c:func:`cork_error_occurred` and friends).
We also define several helper macros for calling a POSIX function and
automatically checking its result.

::

   #include <libcork/helpers/posix.h>

.. note::

   For all of these macros, the ``EINTR`` POSIX error is handled specially.
   This error indicates that a system call was interrupted by a signal, and that
   the call should be retried.  The macros do not translate ``EINTR`` errors
   into libcork errors; instead, they will retry the ``call`` until the
   statement succeeds or returns a non-``EINTR`` error.

.. function:: void rii_check_posix(call)

   Call a function that returns an ``int`` error indicator, when your function
   also returns an ``int``.  If the nested function call returns a POSIX error,
   we translate it into a libcork error and return a libcork error code.

.. function:: void rip_check_posix(call)

   Call a function that returns a pointer, when your function returns an
   ``int``.  If the nested function call returns a POSIX error, we translate it
   into a libcork error and return a libcork error code.

.. function:: void rpi_check_posix(call)

   Call a function that returns an ``int`` error indicator, when your function
   returns a pointer.  If the nested function call returns a POSIX error, we
   translate it into a libcork error and return a libcork error code.

.. function:: void rpp_check_posix(call)

   Call a function that returns a pointer, when your function also returns a
   pointer.  If the nested function call returns a POSIX error, we translate it
   into a libcork error and return a libcork error code.

.. function:: void ei_check_posix(call)

   Call a function that returns an ``int`` error indicator.  If the nested
   function call raises a POSIX error, we translate it into a libcork error and
   automatically jump to the current scope's ``error`` label.

.. function:: void ep_check_posix(call)

   Call a function that returns a pointer.  If the nested function call raises a
   POSIX error, we translate it into a libcork error and automatically jump to
   the current scope's ``error`` label.


Defining new error codes
------------------------

If none of the built-in error codes suffice for an error condition that you need
to report, you'll have to define our own.  As mentioned above, each libcork
error code is either a predefined POSIX ``errno`` value, or a hash some of
string identifying a custom error condition.

Typically, you will create a macro in one of your public header files, whose
value will be your new custom error code.  If this is the case, you can use the
macro name itself to create the hash value for the error code.  This is what we
do for the non-POSIX builtin errors; for instance, the value of the
:c:macro:`CORK_PARSE_ERROR` error code macro is the hash of the string
``CORK_PARSE_ERROR``.

Given this string, you can produce the error code's hash value using the
:ref:`cork-hash <cork-hash>` command that's installed with libcork::

  $ cork-hash CORK_PARSE_ERROR
  0x95dfd3c8

It's incredibly unlikely that the hash value for your new error code will
conflict with any other custom hash-based error codes, or with any predefined
POSIX ``errno`` values.

With your macro name and hash value ready, defining the new error code is
simple::

  #define CORK_PARSE_ERROR  0x95dfd3c8

You should also provide a helper macro that makes it easier to report new
instances of this error condition::

  #define cork_parse_error(...) \
      cork_error_set_printf(CORK_PARSE_ERROR, __VA_ARGS__)

.. type:: uint32_t  cork_error

   An identifier for a particular error condition.  This will either be a
   predefined POSIX ``errno`` value, or the hash of a unique string describing
   the error condition.

With your error class and code defined, you can fill in error instances
using :c:func:`cork_error_set_printf` and friends.


Builtin errors
--------------

In addition to all of the predefined POSIX ``errno`` values, we also provide
error codes for a handful of common error conditions.  You should feel free to
use these in your libraries and applications, instead of creating custom error
codes, if they apply.

.. macro:: CORK_ERROR_NONE

   A special error code that signals that no error occurred.

.. macro:: CORK_PARSE_ERROR

   The provided input violates the rules of the language grammar or file format
   (or anything else, really) that you're trying to parse.

   .. function:: void cork_parse_error(const char *format*, ...)

.. macro:: CORK_REDEFINED
           CORK_UNDEFINED

   Useful when you have a container type that must ensure that there is only one
   entry for any given key.

   .. function:: void cork_redefined(const char *format*, ...)
                 void cork_undefined(const char *format*, ...)

.. macro:: CORK_UNKNOWN_ERROR

   Some error occurred, but we don't have any other information about the error.

   .. function:: void cork_unknown_error(void)

      The error description will include the name of the current function.


We also provide some helper functions for setting these built-in errors:

.. function:: void cork_system_error_set(void)
              void cork_system_error_set_explicit(int err)

   Fills in the current libcork error condition with information from a POSIX
   ``errno`` value.  The human-readable description of the error will be
   obtained from the standard ``strerror`` function.  With the ``_explicit``
   variant, you provide the ``errno`` value directly; for the other variant, we
   get the error code from the C library's ``errno`` variable.


.. function:: void cork_abort(const char \*fmt, ...)

   Aborts the current program with an error message given by *fmt* and any
   additional parameters.

.. function:: void cork_unreachable(void)

   Aborts the current program with a message indicating that the code path
   should be unreachable.  This can be useful in the ``default`` clause of a
   ``switch`` statement if you can ensure that one of the non-``default``
   branches will always be selected.
