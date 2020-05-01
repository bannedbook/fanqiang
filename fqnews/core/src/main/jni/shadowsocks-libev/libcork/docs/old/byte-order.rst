.. _byte-order:

**********
Byte order
**********

.. highlight:: c

::

  #include <libcork/core.h>

This section contains definitions for determining the endianness of the
host system, and for byte-swapping integer values of various sizes.


Endianness detection
====================

.. macro:: CORK_LITTLE_ENDIAN
           CORK_BIG_ENDIAN
           CORK_HOST_ENDIANNESS
           CORK_OTHER_ENDIANNESS

   The ``CORK_HOST_ENDIANNESS`` macro can be used to determine the
   endianness of the host system.  It will be equal to either
   ``CORK_LITTLE_ENDIAN`` or ``CORK_BIG_ENDIAN``.  (The actual values
   don't matter; you should always compare against the predefined
   constants.)  The ``CORK_OTHER_endianness`` macro is defined to be the
   opposite endianness as ``CORK_HOST_ENDIANNESS``.  A common use case
   would be something like::

     #if CORK_HOST_endianness == CORK_LITTLE_ENDIAN
     /* do something to little-endian values */
     #else
     /* do something to big-endian values */
     #endif

.. macro:: CORK_HOST_ENDIANNESS_NAME
           CORK_OTHER_ENDIANNESS_NAME

   These macros give you a human-readable name of the host's endianness.
   You can use this in debugging messages.

   .. note:: You should *not* use these macros to detect the
      endianness of the system, since we might change their definitions
      at some point to support localization.  For that,
      use :macro:`CORK_LITTLE_ENDIAN` and :macro:`CORK_BIG_ENDIAN`.


Byte swapping
=============

Swapping arbitrary expressions
------------------------------

All of the macros in this section take in an rvalue (i.e., any arbitrary
expression) as a parameter.  The result of the swap is returned as the
value of the macro.

.. function:: uint16_t CORK_SWAP_UINT16(uint16_t value)
              uint32_t CORK_SWAP_UINT32(uint32_t value)
              uint64_t CORK_SWAP_UINT64(uint64_t value)

   These functions always perform a byte-swap, regardless of the
   endianness of the host system.

.. function:: uint16_t CORK_UINT16_BIG_TO_HOST(uint16_t value)
              uint32_t CORK_UINT32_BIG_TO_HOST(uint32_t value)
              uint64_t CORK_UINT64_BIG_TO_HOST(uint64_t value)

   These functions convert a big-endian (or network-endian) value into
   host endianness.  (I.e., they only perform a swap if the current host
   is little-endian.)

.. function:: uint16_t CORK_UINT16_HOST_TO_BIG(uint16_t value)
              uint32_t CORK_UINT32_HOST_TO_BIG(uint32_t value)
              uint64_t CORK_UINT64_HOST_TO_BIG(uint64_t value)

   These functions convert a host-endian value into big (or network)
   endianness.  (I.e., they only perform a swap if the current host is
   little-endian.)

.. function:: uint16_t CORK_UINT16_LITTLE_TO_HOST(uint16_t value)
              uint32_t CORK_UINT32_LITTLE_TO_HOST(uint32_t value)
              uint64_t CORK_UINT64_LITTLE_TO_HOST(uint64_t value)

   These functions convert a little-endian value into host endianness.
   (I.e., they only perform a swap if the current host is big-endian.)

.. function:: uint16_t CORK_UINT16_HOST_TO_LITTLE(uint16_t value)
              uint32_t CORK_UINT32_HOST_TO_LITTLE(uint32_t value)
              uint64_t CORK_UINT64_HOST_TO_LITTLE(uint64_t value)

   These functions convert a host-endian value into little endianness.
   (I.e., they only perform a swap if the current host is big-endian.)

Swapping values in place
------------------------

The macros in this section swap an integer *in place*, which means that
the original value is overwritten with the result of the swap.  To
support this, you must pass in an *lvalue* as the parameter to the
macro.  (Note that you don't pass in a *pointer* to the original value;
these operations are implemented as macros, and you just need to provide
a reference to the variable to be swapped.)

.. function:: void CORK_SWAP_UINT16_IN_PLACE(uint16_t &value)
              void CORK_SWAP_UINT32_IN_PLACE(uint32_t &value)
              void CORK_SWAP_UINT64_IN_PLACE(uint64_t &value)

   These functions always perform a byte-swap, regardless of the
   endianness of the host system.

.. function:: void CORK_UINT16_BIG_TO_HOST_IN_PLACE(uint16_t &value)
              void CORK_UINT32_BIG_TO_HOST_IN_PLACE(uint32_t &value)
              void CORK_UINT64_BIG_TO_HOST_IN_PLACE(uint64_t &value)

   These functions convert a big-endian (or network-endian) value into
   host endianness, and vice versa.  (I.e., they only perform a swap if
   the current host is little-endian.)

.. function:: void CORK_UINT16_HOST_TO_BIG_IN_PLACE(uint16_t &value)
              void CORK_UINT32_HOST_TO_BIG_IN_PLACE(uint32_t &value)
              void CORK_UINT64_HOST_TO_BIG_IN_PLACE(uint64_t &value)

   These functions convert a host-endian value into big (or network)
   endianness.  (I.e., they only perform a swap if the current host is
   little-endian.)

.. function:: void CORK_UINT16_LITTLE_TO_HOST_IN_PLACE(uint16_t &value)
              void CORK_UINT32_LITTLE_TO_HOST_IN_PLACE(uint32_t &value)
              void CORK_UINT64_LITTLE_TO_HOST_IN_PLACE(uint64_t &value)

   These functions convert a little-endian value into host endianness, and
   vice versa.  (I.e., they only perform a swap if the current host is
   big-endian.)

.. function:: void CORK_UINT16_HOST_TO_LITTLE_IN_PLACE(uint16_t &value)
              void CORK_UINT32_HOST_TO_LITTLE_IN_PLACE(uint32_t &value)
              void CORK_UINT64_HOST_TO_LITTLE_IN_PLACE(uint64_t &value)

   These functions convert a host-endian value into little endianness.
   (I.e., they only perform a swap if the current host is big-endian.)
