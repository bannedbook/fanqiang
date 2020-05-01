.. _bits:

********
Bit sets
********

.. highlight:: c

::

  #include <libcork/ds.h>

This sections defines a type for storing an array of bits.  This data structure
is most often used to implement a set of integers.  It is particularly good when
you expect your sets to be *dense*.  You should not use a bitset if the number
of possibly elements is outrageously large, however, since that would cause your
bitset to exhaust the available memory.

.. type:: struct cork_bitset

   An array of bits.  You should not allocate any instances of this type
   yourself; use :c:func:`cork_bitset_new` instead.

   .. member:: size_t bit_count

      The number of bits that are included in this array.  (Each bit can be on
      or off; this does not give you the number of bits that are *on*, it gives
      you the number of bits in total, on or off.)


.. function:: void cork_bitset_init(struct cork_bitset \*set)

   Initialize a new bitset instance that you've allocated yourself
   (usually on the stack).  All bits will be initialized to ``0``.

.. function:: struct cork_bitset \*cork_bitset_new(size_t bit_count)

   Create a new bitset with enough space to store the given number of bits.
   All bits will be initialized to ``0``.

.. function:: void cork_bitset_done(struct cork_bitset \*set)

   Finalize a bitset, freeing any set content that it contains.  This
   function should only be used for bitsets that you allocated yourself,
   and initialized using :c:func:`cork_bitset_init()`.  You must **not** use
   this function to free a bitset allocated using :c:func:`cork_bitset_new()`.

.. function:: void cork_bitset_free(struct cork_bitset \*set)

   Finalize and deallocate a bitset, freeing any set content that it
   contains.  This function should only be used for bitsets allocated
   using :c:func:`cork_bitset_new()`.  You must **not** use this
   function to free a bitset initialized using :c:func:`cork_bitset_init()`.

.. function:: bool cork_bitset_get(struct cork_bitset \*set, size_t index)

   Return whether the given bit is on or off in *set*.  It is your
   responsibility to ensure that *index* is within the valid range for *set*.

.. function:: void cork_bitset_set(struct cork_bitset \*set, size_t index, bool value)

   Turn the given bit on or off in *set*.  It is your responsibility to ensure
   that *index* is within the valid range for *set*.

.. function:: void cork_bitset_clear(struct cork_bitset \*set)

   Turn off of the bits in *set*.
