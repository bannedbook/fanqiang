.. _array:

****************
Resizable arrays
****************

.. highlight:: c

::

  #include <libcork/ds.h>

This section defines a resizable array class, similar to C++'s
``std::vector`` or Java's ``ArrayList`` classes.  Our arrays can store
any fixed-size element.  The arrays automatically resize themselves as
necessary to store the elements that you add.


.. type:: cork_array(element_type)

   A resizable array that contains elements of type *element_type*.

.. function:: void cork_array_init(cork_array(T) \*array)

   Initializes a new array.  You should allocate *array* yourself,
   presumably on the stack or directly within some other data type.  The
   array will start empty.

.. function:: void cork_array_done(cork_array(T) \*array)

   Finalizes an array, freeing any storage that was allocated to hold
   the arrays elements.

.. function:: size_t cork_array_size(cork_array(T) \*array)

   Returns the number of elements in *array*.

.. function:: bool cork_array_is_empty(cork_array(T) \*array)

   Returns whether *array* has any elements.

.. function:: void cork_array_void(cork_array(T) \*array)

   Removes all elements from *array*.

.. function:: T* cork_array_elements(cork_array(T) \*array)

   Returns a pointer to the underlying array of elements in *array*.  The
   elements are guaranteed to be contiguous, just like in a normal C array, but
   the particular pointer that is returned in **not** guaranteed to be
   consistent across function calls that modify the contents of the array.

.. function:: T cork_array_at(cork_array(T) \*array, size_t index)

   Returns the element in *array* at the given *index*.  Like accessing
   a normal C array, we don't do any bounds checking.  The result is a
   valid lvalue, so it can be directly assigned to::

     cork_array(int64_t)  array;
     cork_array_append(array, 5, err);
     cork_array_at(array, 0) = 12;

.. function:: void cork_array_append(cork_array(T) \*array, T element)

   Appends *element* to the end of *array*, reallocating the array's
   storage if necessary.  If you have an ``init`` or ``reset`` callback for
   *array*, it will be used to initialize the space that was allocated for the
   new element, and then *element* will be directly copied into that space
   (using ``memcpy`` or an equivalent).  If that is not the right copy behavior
   for the elements of *array*, then you should use
   :c:func:`cork_array_append_get` instead, and fill in the allocated element
   directly.

.. function:: T \*cork_array_append_get(cork_array(T) \*array)

   Appends a new element to the end of *array*, reallocating the array's storage
   if necessary, returning a pointer to the new element.

.. function:: int cork_array_ensure_size(cork_array(T) \*array, size_t desired_count)

   Ensures that *array* has enough allocated space to store *desired_count*
   elements, reallocating the array's storage if needed.  The actual size and
   existing contents of the array aren't changed.

.. function:: int cork_array_copy(cork_array(T) \*dest, cork_array(T) \*src, cork_copy_f \*copy, void \*user_data)

   Copy elements from *src* to *dest*.  If you provide a *copy* function, it
   will be called on each element to perform the copy.  If not, we'll use
   ``memcpy`` to bulk-copy the elements.

   If you've provided :ref:`callbacks <array-callbacks>` for *dest*, then those
   callbacks will be called appropriately.  We'll call the ``remove`` callback
   for any existing entries (will be overwritten by the copy).  We'll call
   ``init`` or ``reuse`` on each element entry before it's copied.

   .. type:: typedef int (\*cork_copy_f)(void \*user_data, void \*dest, const void \*src)

.. function:: size_t cork_array_element_size(cork_array(T) \*array)

   Returns the size of the elements that are stored in *array*.  You
   won't normally need to call this, since you can just use
   ``sizeof(T)``.


.. _array-callbacks:

Initializing and finalizing elements
------------------------------------

You can provide callback functions that will be used to automatically initialize
and finalize the elements of a resizable array.


.. function:: void cork_array_set_init(cork_array(T) \*array, cork_init_f init)
              void cork_array_set_done(cork_array(T) \*array, cork_done_f done)
              void cork_array_set_reuse(cork_array(T) \*array, cork_init_f reuse)
              void cork_array_set_remove(cork_array(T) \*array, cork_done_f remove)
              void cork_array_set_callback_data(cork_array(T) \*array, void \*user_data, cork_free_f free_user_data)

   Set one of the callback functions for *array*.  There are two pairs of
   callbacks: ``init`` and ``done``, and ``reuse`` and ``remove``.  Within each
   pair, one callback is used to initialize an element of the array, while the
   other is used to finalize it.

   The ``init`` callback is used to initialize an element when its array entry
   is used for the first time.  If you then shrink the array (via
   :c:func:`cork_array_clear`, for instance), and then append elements again,
   you will reuse array entries; in this case, the ``reset`` callback is used
   instead.  (Having separate ``init`` and ``reuse`` callbacks can be useful
   when the elements are complex objects with deep memory requirements.  If you
   use the ``init`` callback to allocate that memory, and use the ``reset``
   callback to "clear" it, then you can reduce some of the memory allocation
   overhead.)

   Similarly, the ``remove`` callback is used when an element is removed from
   the array, but the space that the element used isn't being reclaimed yet.
   The ``done`` callback, on the other hand, is used when the array entry is
   reclaimed and freed.

   All of the callbacks take in an additional *user_data* parameter, in addition
   to the array entries themselves.  You provide that parameter by calling the
   :c:func:`cork_array_set_callback_data` function.  If you pass in a
   *free_user_data* function, then we will use that function to free the
   *user_data* when the array itself is finalized.

   .. type:: typedef void (\*cork_init_f)(void \*user_data, void \*value)
             typedef void (\*cork_done_f)(void \*user_data, void \*value)
             typedef void (\*cork_free_f)(void \*value)
