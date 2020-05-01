.. _managed-buffer:

**********************
Managed binary buffers
**********************

.. highlight:: c

::

  #include <libcork/ds.h>

This section defines an interface for handling reference-counted binary
buffers.  The :c:type:`cork_managed_buffer` type wraps a buffer with a
simple reference count, and takes care of freeing the necessary
resources when the reference count drops to zero.  There should only be
a single :c:type:`cork_managed_buffer` instance for any given buffer,
regardless of how many threads or functions access that buffer.  Each
thread or function that uses the buffer does so via a
:c:type:`cork_slice` instance.  This type is meant to be allocated
directly on the stack (or in some other managed storage), and keeps a
pointer to the managed buffer instance that it slices.  As its name
implies, a slice can refer to a subset of the buffer.


.. type:: struct cork_managed_buffer

   A “managed buffer”, which wraps a buffer with a simple reference
   count.

   Managed buffer consumers should consider all of the fields of this
   class private.  Managed buffer implementors should fill in this
   fields when constructing a new ``cork_managed_buffer`` instance.

   .. member:: const void  \*buf

      The buffer that this instance manages.

   .. member:: size_t  size

      The size of :c:member:`buf`.

   .. member:: volatile int  ref_count

      A reference count for the buffer.  If this drops to ``0``, the
      buffer will be finalized.

   .. member:: struct cork_managed_buffer_iface  \*iface

      The managed buffer implementation for this instance.


.. function:: struct cork_managed_buffer \*cork_managed_buffer_ref(struct cork_managed_buffer \*buf)

   Atomically increase the reference count of a managed buffer.  This
   function is thread-safe.


.. function:: void cork_managed_buffer_unref(struct cork_managed_buffer \*buf)

   Atomically decrease the reference count of a managed buffer.  If the
   reference count falls to ``0``, the instance is freed.  This function
   is thread-safe.

.. function:: int cork_managed_buffer_slice(struct cork_slice \*dest, struct cork_managed_buffer \*buffer, size_t offset, size_t length)
              int cork_managed_buffer_slice_offset(struct cork_slice \*dest, struct cork_managed_buffer \*buffer, size_t offset)

   Initialize a new slice that refers to a subset of a managed buffer.
   The *offset* and *length* parameters identify the subset.  (For the
   ``_slice_offset`` variant, the *length* is calculated automatically
   to include all of the managed buffer content starting from *offset*.)
   If these parameters don't refer to a valid portion of the buffer, we
   return ``false``, and you must not try to deference the slice's
   :c:member:`buf <cork_slice.buf>` pointer.  If the slice is valid, we
   return ``true``.

   Regardless of whether the new slice is valid, you **must** ensure
   that you call :c:func:`cork_slice_finish()` when you are done with
   the slice.


Predefined managed buffer implementations
-----------------------------------------

.. function:: struct cork_managed_buffer \*cork_managed_buffer_new_copy(const void \*buf, size_t size)

   Make a copy of *buf*, and allocate a new managed buffer to manage
   this copy.  The copy will automatically be freed when the managed
   buffer's reference count drops to ``0``.


.. type:: void (\*cork_managed_buffer_freer)(void \*buf, size_t size)

   A finalization function for a managed buffer created by
   :c:func:`cork_managed_buffer_new()`.

.. function:: struct cork_managed_buffer \*cork_managed_buffer_new(const void \*buf, size_t size, cork_managed_buffer_freer free)

   Allocate a new managed buffer to manage an existing buffer (*buf*).
   The existing buffer is *not* copied; the new managed buffer instance
   takes control of it.  When the managed buffer's reference count drops
   to ``0``, it will call *free* to finalize *buf*.

   This is a helper function, and keeps you from having to write a
   complete custom managed buffer implementation when you don't need to
   store any additional state in the managed buffer object.

   .. note::

      The *free* function is *not* responsible for freeing the
      ``cork_managed_buffer`` instance itself.


Custom managed buffer implementations
-------------------------------------

.. type:: struct cork_managed_buffer_iface

   The interface of methods that managed buffer implementations must
   provide.

   .. member:: void (\*free)(struct cork_managed_buffer \*self)

      Free the contents of a managed buffer, and the
      ``cork_managed_buffer`` instance itself.
