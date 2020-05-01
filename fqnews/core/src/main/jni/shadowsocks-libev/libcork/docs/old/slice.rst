.. _slice:

*************
Binary slices
*************

.. highlight:: c

::

  #include <libcork/ds.h>

This section defines an interface for safely referring to the contents
of a binary buffer, without needing to know where the buffer came from.
In addition to accessing the contents of the underlying buffer, slices
support three operations:

* *Copying* initializes a new slice object to point at the same
  underlying buffer as the current slice.  Depending on how the
  underlying buffer is implemented, this doesn't necessarily involve
  actual copying; for instance, the :c:type:`cork_managed_buffer` type
  implements this operation by incrementing the reference count of the
  managed buffer.

* *Slicing* updates the current slice to point at a subset of its
  current contents.  This doesn't affect the underlying buffer.

* *Freeing* releases the resources used by the slice, possibly freeing
  the underlying buffer.

These operations are represented by the *slice interface*
(:c:type:`cork_slice_iface`).  To write a new slice implementation, you
just need to provide an instance of this interface type.


.. note::

   There's no generic constructor or initialization function for slices;
   instead, you'll create a slice from some other data structure, using
   a function specific to that data structure.  (An example is the
   :c:func:`cork_buffer_to_slice()` function, which initializes a slice
   from a :ref:`resizable buffer <buffer>`.)


.. type:: struct cork_slice

   A specific window into a portion of an underlying binary buffer.

   Instances of this class do not need to be (and almost never are)
   allocated on the heap; you can define an instance directly on the
   stack, or in some other kind of managed storage.

   .. note::

      It is **very important** that you ensure that
      :c:func:`cork_slice_finish()` is called whenever you are done with
      a slice — if you don't, there's a very good chance that the
      underlying buffer will never be freed.  Yes, yes, it's unfortunate
      that C doesn't have ``try``/``finally`` or RAII, but suck it up
      and make sure that :c:func:`cork_slice_finish()` gets called.

   .. member:: const void  \*buf

      The beginning of the sliced portion of the underlying buffer.

   .. member:: size_t  size

      The size of the sliced portion of the underlying buffer.

   .. member:: struct cork_slice_iface  \*iface

      The slice implementation of the underlying buffer.  For slice
      consumers, this field should be considered private.  For slice
      implementors, you should fill in this field with your slice
      interface.

   .. member:: void  \*user_data

      An opaque pointer used by the slice implementation.  For slice
      consumers, this field should be considered private.  For slice
      implementors, you can use this field to point at the underlying
      buffer (and/or any additional metadata that you need.)


.. function:: void cork_slice_clear(struct cork_slice \*slice)

   Clear a slice object.  This fills in a slice instance so that it's
   “empty”.  You should not try to call any of the slice methods on an
   empty slice, nor should you try to dereference the slice's
   :c:member:`buf <cork_slice.buf>` pointer.  An empty slice is
   equivalent to a ``NULL`` pointer.

.. function:: bool cork_slice_is_empty(struct cork_slice \*slice)

   Return whether a slice is empty.


.. function:: int cork_slice_copy(struct cork_slice \*dest, struct cork_slice \*src, size_t offset, size_t length)
              int cork_slice_copy_offset(struct cork_slice \*dest, struct cork_slice \*src, size_t offset)
              int cork_slice_copy_fast(struct cork_slice \*dest, struct cork_slice \*src, size_t offset, size_t length)
              int cork_slice_copy_offset_fast(struct cork_slice \*dest, struct cork_slice \*src, size_t offset)

   Initialize a new slice that refers to a subset of an existing slice.
   The *offset* and *length* parameters identify the subset.  (For the
   ``_copy_offset`` variant, the *length* is calculated automatically to
   include all of the original slice content starting from *offset*.)

   For the ``_fast`` variants, we **don't** verify that the *offset* and
   *length* parameters refer to a valid subset of the slice.  This is
   your responsibility.  For the non-\ ``_fast`` variants, we perform a
   bounds check for you, and return an error if the requested slice is
   invalid.

   Regardless of whether the new slice is valid, you **must** ensure
   that you call :c:func:`cork_slice_finish()` on *dest* when you are
   done with it.

.. function:: int cork_slice_light_copy(struct cork_slice \*dest, const struct cork_slice \*src, size_t offset, size_t length)
              int cork_slice_light_copy_offset(struct cork_slice \*dest, const struct cork_slice \*src, size_t offset)
              int cork_slice_light_copy_fast(struct cork_slice \*dest, const struct cork_slice \*src, size_t offset, size_t length)
              int cork_slice_light_copy_offset_fast(struct cork_slice \*dest, const struct cork_slice \*src, size_t offset)

   Initialize a new slice that refers to a subset of an existing slice.  By
   calling a ``_light_copy`` function instead of a ``_copy`` function, you are
   guaranteeing that *dest* will not outlive *src* — i.e., it is your
   responsibility to ensure that you call :c:func:`cork_slice_finish` on *dest*
   before you call it on *src*.  This guarantee lets slice implementations make
   a more light-weight copy of the slice: for instance, by not having to make a
   copy of the underlying buffer.

   The *offset* and *length* parameters identify the subset.  (For the
   ``_light_copy_offset`` variant, the *length* is calculated automatically to
   include all of the original slice content starting from *offset*.)

   For the ``_fast`` variants, we **don't** verify that the *offset* and
   *length* parameters refer to a valid subset of the slice.  This is
   your responsibility.  For the non-\ ``_fast`` variants, we perform a
   bounds check for you, and return an error if the requested slice is
   invalid.

   Regardless of whether the new slice is valid, you **must** ensure
   that you call :c:func:`cork_slice_finish()` on *dest* when you are
   done with it.

.. function:: int cork_slice_slice(struct cork_slice \*slice, size_t offset, size_t length)
              int cork_slice_slice_offset(struct cork_slice \*slice, size_t offset)
              int cork_slice_slice_fast(struct cork_slice \*slice, size_t offset, size_t length)
              int cork_slice_slice_offset_fast(struct cork_slice \*slice, size_t offset)

   Update a slice to refer to a subset of its contents.  The *offset*
   and *length* parameters identify the subset.  (For the
   ``_slice_offset`` variant, the *length* is calculated automatically
   to include all of the original slice content starting from *offset*.)

   For the ``_fast`` variants, we **don't** verify that the *offset* and
   *length* parameters refer to a valid subset of the slice.  This is
   your responsibility.  For the non-\ ``_fast`` variants, we perform a
   bounds check for you, and return an error if the requested slice is
   invalid.

.. function:: void cork_slice_finish(struct cork_slice \*slice)

   Finalize a slice, freeing the underlying buffer if necessary.

.. function:: int cork_slice_equal(const struct cork_slice \*slice1, const struct cork_slice \*slice2)

   Compare the contents of two slices for equality.  (The *contents* of
   the slices are compared, not their pointers; this is the slice
   equivalent of ``memcmp``, not the ``==`` operator.)


Slice interface
---------------

.. type:: struct cork_slice_iface

   The interface of methods that slice implementations must provide.

   .. member:: void (\*free)(struct cork_slice \*self)

      Called when the slice should be freed.  If necessary, you should
      free the contents of the underlying buffer.  (If the buffer
      contents can be shared, it's up to you to keep track of when the
      contents are safe to be freed.)

      This function pointer can be ``NULL`` if you don't need to free
      any underlying buffer.

   .. member:: int (\*copy)(struct cork_slice \*dest, const struct cork_slice \*src, size_t offset, size_t length)
               int (\*light_copy)(struct cork_slice \*dest, const struct cork_slice \*src, size_t offset, size_t length)

      Create a copy of a slice.  You can assume that *offset* and
      *length* refer to a valid subset of *src*\ 's content.

      For the ``light_copy`` method, the caller guarantees that the new light
      copy (*dest*) will not outlive the original slice (*src*).  For some slice
      implementations, this lets you create a more light-weight copy — for
      instance, by not having to make an actualy copy of the underlying buffer.

   .. member:: int (\*slice)(struct cork_slice \*self, size_t offset, size_t length)

      Update *self* to point at a different subset of the underlying
      buffer.  You can assume that *offset* and *length* refer to a
      valid subset of the buffer.  (They will be relative to *self*\ 's
      existing slice, and not to the original buffer.)

      This function pointer can be ``NULL`` if you don't need to do
      anything special to the underlying buffer; in this case,
      :c:func:`cork_slice_slice()` and
      :c:func:`cork_slice_slice_offset()` will update the slice's *buf*
      and *size* fields for you.


Built-in slice implementations
------------------------------

Several libcork classes can be used to initialize a slice:

* :ref:`Managed buffers <managed-buffer>` via the
  :c:func:`cork_managed_buffer_slice` function

* :ref:`Resizable buffers <buffer>` via the
  :c:func:`cork_buffer_to_slice` function

You can also initialize a slice to point at an existing buffer:


.. function:: void cork_slice_init_static(struct cork_slice \*dest, const void \*buf, size_t size)

   Initializes *dest* to point at the given static buffer.  Since the
   buffer is static, and guaranteed to always exist, the slice's
   :c:member:`~cork_slice.copy` method doesn't copy the underlying data,
   it just creates a new pointer to the existing buffer.

   .. note::

      You can also use this function to refer to a non-static buffer,
      but then you take responsibility for ensuring that the underlying
      buffer exists for at least as long as the slice, and any copies
      made of the slice.

   As with all slices, you **must** ensure that you call
   :c:func:`cork_slice_finish` when you're done with the slice.


.. function:: void cork_slice_init_copy_once(struct cork_slice \*dest, const void \*buf, size_t size)

   Initializes *dest* to point at the given buffer.  If any copies are made of
   the slice, then we create a :ref:`managed copy <managed-buffer>` of the
   underlying buffer.  This means that you only have to ensure that *buf* exists
   for as long as the original *dest* slice is used.

   As with all slices, you **must** ensure that you call
   :c:func:`cork_slice_finish` when you're done with the slice.
