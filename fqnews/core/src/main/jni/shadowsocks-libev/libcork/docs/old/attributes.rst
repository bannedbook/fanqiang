.. _attributes:

*******************
Compiler attributes
*******************

.. highlight:: c

::

  #include <libcork/core.h>

The macros in this section define compiler-agnostic versions of several
common compiler attributes.


.. function:: CORK_LIKELY(expression)
              CORK_UNLIKELY(expression)

   Indicate that the given Boolean *expression* is likely to be ``true``
   or ``false``, respectively.  The compiler can sometimes use this
   information to generate more efficient code.


.. macro:: CORK_ATTR_CONST

   Declare a “constant” function.  The return value of a constant
   function can only depend on its parameters.  This is slightly more
   strict than a “pure” function (declared by
   :c:macro:`CORK_ATTR_PURE`); a constant function is not allowed to
   read from global variables, whereas a pure function is.

   .. note:: Note that the compiler won't verify that your function
      meets the requirements of a constant function.  Instead, this
      attribute notifies the compiler of your intentions, which allows
      the compiler to assume more about your function when optimizing
      code that calls it.

   ::

     int square(int x) CORK_ATTR_CONST;


.. macro:: CORK_ATTR_MALLOC

   Declare a function that returns a newly allocated pointer.  The
   compiler can use this information to generate more accurate aliasing
   information, since it can infer that the result of the function
   cannot alias any other existing pointer.

   ::

     void *custom_malloc(size_t size) CORK_ATTR_MALLOC;


.. macro:: CORK_ATTR_NOINLINE

   Declare that a function shouldn't be eligible for inlining.


.. macro:: CORK_ATTR_PRINTF(format_index, args_index)

   Declare a function that takes in ``printf``\ -like parameters.
   *format_index* is the index (starting from 1) of the parameter that
   contains the ``printf`` format string.  *args_index* is the index of
   the first parameter that contains the data to format.


.. macro:: CORK_ATTR_PURE

   Declare a “pure” function.  The return value of a pure function can
   only depend on its parameters, and on global variables.

   ::

     static int  _next_id;
     int get_next_id(void) CORK_ATTR_PURE;


.. macro:: CORK_ATTR_SENTINEL

   Declare a var-arg function whose last parameter must be a ``NULL``
   sentinel value.  When the compiler supports this attribute, it will
   check the actual parameters whenever this function is called, and
   ensure that the last parameter is a ``NULL``.


.. macro:: CORK_ATTR_UNUSED

   Declare a entity that might not be used.  This lets you keep
   ``-Wall`` activated in several cases where you're obligated to define
   something that you don't intend to use.

   ::

     CORK_ATTR_UNUSED static void
     unused_function(void)
     {
         CORK_ATTR_UNUSED int  unused_value;
     }


.. macro:: CORK_INITIALIZER(func_name)

   Declare a ``static`` function that will be automatically called at program
   startup.  If there are multiple initializer functions linked into a program,
   there is no guarantee about the order in which the functions will be called.

   ::

     #include <libcork/core.h>
     #include <libcork/ds.h>

     static cork_array(int)  array;

     CORK_INITIALIZER(init_array)
     {
        cork_array_init(&array);
     }
