.. _basic-types:

***********
Basic types
***********

.. highlight:: c

::

  #include <libcork/core.h>

The types in this section ensure that the C99 integer types are
available, regardless of platform.  We also define some preprocessor
macros that give the size of the non-fixed-size standard types.  In
addition, libcork defines some useful low-level types:

.. toctree::
   :maxdepth: 1

   int128
   net-addresses
   timestamps
   hash-values
   unique-ids

Integral types
==============

.. type:: bool

   A boolean.  Where possible, we simply include ``<stdbool.h>`` to get
   this type.  It might be ``typedef``\ ed to ``int``\ .  We also make
   sure that the following constants are defined:

   .. var:: bool false
            bool true

.. type:: int8_t
          uint8_t
          int16_t
          uint16_t
          int32_t
          uint32_t
          int64_t
          uint64_t

   Signed and unsigned, fixed-size integral types.

.. type:: intptr_t
          uintptr_t

   Signed and unsigned integers that are guaranteed to be big enough to
   hold a type-cast ``void *``\ .

.. type:: size_t

   An unsigned integer big enough to hold the size of a memory object,
   or a maximal array index.

.. type:: ptrdiff_t

   A signed integer big enough to hold the difference between two
   pointers.

Size macros
===========

.. macro:: CORK_SIZEOF_SHORT
           CORK_SIZEOF_INT
           CORK_SIZEOF_LONG
           CORK_SIZEOF_POINTER

   The size (in bytes) of the ``short``, ``int``, ``long``, and ``void
   *`` types, respectively.


.. _embedded-struct:

Embedded ``struct``\ s
======================

Quite often a callback function or API will take in a pointer to a
particular ``struct``, with the expectation that you can embed that
``struct`` into some other type for extension purposes.  Kind of a
bastardized subclassing mechanism for C code.  The doubly-linked list
module is a perfect example; you're meant to embed
:c:type:`cork_dllist_item` within the linked list element type.  You can
use the following macro to obtain the pointer to the containing
(“subclass”) ``struct``, when given a pointer to the contained
(“superclass”) ``struct``:

.. function:: struct_type \*cork_container_of(field_type \*field, TYPE struct_type, FIELD field_name)

   The *struct_type* parameter must be the name of a ``struct`` type,
   *field_name* must be the name of some field within that
   ``struct``, and *field* must be a pointer to an instance of that
   field.  The macro returns a pointer to the containing ``struct``.
   So, given the following definitions::

     struct superclass {
         int  a;
     };

     struct subclass {
         int  b;
         struct superclass  parent;
     };

     struct subclass  instance;

   then the following identity holds::

     cork_container_of(&instance.parent, struct subclass, parent) == &instance

   .. note:: When the superclass ``struct`` appears as the first element
      of the subclass ``struct``, you can obtain the same effect using a
      simple type-cast.  However, the ``cork_container_of`` macro is
      more robust, since it also works when the superclass ``struct``
      appears later on in the subclass ``struct``.
