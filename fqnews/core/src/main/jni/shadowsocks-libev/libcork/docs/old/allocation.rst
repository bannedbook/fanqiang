.. _allocation:

*****************
Memory allocation
*****************

.. highlight:: c

::

  #include <libcork/core.h>

One of the biggest hassles in writing C code is memory management.  libcork's
memory allocation API tries to simplify this task as much as possible.  This is
still C, so you still have to manage allocated memory manually — for instance,
by keeping careful track of which section of code "owns" any memory that you've
allocated from heap, and is therefore responsible for freeing it.  But we *can*
make it easier to handle memory allocation failures, and provide helper macros
for certain common allocation tasks.

There is another `important use case`_ that we also want to support: giving
application writers complete control over how the libraries they use allocate
and deallocate memory.  libcork :ref:`provides <libcork-allocators>` this
capability, giving you control over how, for instance, a hash table allocates
its internal buckets.  If you're writing a library that links with libcork as a
shared library, you'll get this behavior for free; if the application writer
customizes how libcork allocates memory, your library will pick up that
customization as well.  If you're embedding libcork, so that your library's
clients can't tell (or care) that you're using libcork, then you'll want to
expose your own similar customization interface.

.. _important use case: https://blog.mozilla.org/nnethercote/2013/11/08/libraries-should-permit-custom-allocators/


.. _allocation-api:

Allocating memory
=================

The simplest part of the API is the part responsible for actually allocating and
deallocating memory.  When using this part of the API, you don't have to worry
about customization at all; the functions described here will automatically "do
the right thing" based on how your library or application is configured.  The
biggest thing to worry about is how to handle memory allocation failures.  We
provide two strategies, "guaranteed" and "recoverable".

The most common use case is that running out of memory is a Really Bad Thing,
and there's nothing we can do to recover.  In this case, it doesn't make sense
to check for memory allocation failures throughout your code, since you can't
really do anything if it does happen.  The "guaranteed" family of functions
handles that error checking for you, and guarantees that if the allocation
function returns, it will return a valid piece of memory.  If the allocation
fails, the function will never return.  That allows you to right simple and safe
code like the following::

    struct my_type  *instance = cork_new(struct my_type);
    /* Just start using instance; don't worry about verifying that it's
     * not NULL! */

On the other hand, you might be writing some code that can gracefully handle a
memory allocation failure.  You might try to allocate a super-huge cache, for
instance; if you can't allocate the cache, your code will still work, it will
just be a bit slower.  In this case, you *want* to be able to detect memory
allocation failures, and handle them in whatever way is appropriate.  The
"recoverable" family of functions will return a ``NULL`` pointer if allocation
fails.

.. note::

   libcork itself uses the guaranteed functions for all internal memory
   allocation.


Guaranteed allocation
---------------------

The functions in this section are guaranteed to return a valid newly allocated
pointer.  If memory allocation fails, the functions will not return.

.. function:: void \*cork_malloc(size_t size)
              void \*cork_calloc(size_t count, size_t size)
              void \*cork_realloc(void \*ptr, size_t old_size, size_t new_size)
              type \*cork_new(TYPE type)

   The first three functions mimic the standard ``malloc``, ``calloc``, and
   ``realloc`` functions to allocate (or reallocate) some memory, with the added
   guarantee that they will always return a valid newly allocated pointer.
   ``cork_new`` is a convenience function for allocating an instance of a
   particular type; it is exactly equivalent to::

       cork_malloc(sizeof(type))

   Note that with ``cork_realloc``, unlike the standard ``realloc`` function,
   you must provide the old size of the memory region, in addition to the
   requested new size.

   Each allocation function has a corresponding deallocation function that you
   must use to free the memory when you are done with it: use
   :c:func:`cork_free` to free memory allocated using ``cork_malloc`` or
   ``cork_realloc``; use :c:func:`cork_cfree` to free memory allocated using
   ``cork_calloc``; and use :c:func:`cork_delete` to free memory allocated using
   ``cork_new``.

   .. note::

      Note that the possible memory leak in the standard ``realloc``
      function doesn't apply here, since we're going to abort the whole
      program if the reallocation fails.


Recoverable allocation
----------------------

The functions in this section will return a ``NULL`` pointer if any memory
allocation fails, allowing you to recover from the error condition, if possible.

.. function:: void \*cork_xmalloc(size_t size)
              void \*cork_xcalloc(size_t count, size_t size)
              void \*cork_xrealloc(void \*ptr, size_t old_size, size_t new_size)
              void \*cork_xreallocf(void \*ptr, size_t old_size, size_t new_size)
              type \*cork_xnew(TYPE type)

   The first three functions mimic the standard ``malloc``, ``calloc``,
   ``realloc`` functions.  ``cork_xreallocf`` mimics the common ``reallocf``
   function from BSD.  These functions return ``NULL`` if the memory allocation
   fails.  (Note that unlike the standard functions, they do **not** set
   ``errno`` to ``ENOMEM``; the only indication you have of an error condition
   is a ``NULL`` return value.)

   Note that with ``cork_xrealloc`` and ``cork_xreallocf``, unlike the standard
   ``realloc`` function, you must provide the old size of the memory region, in
   addition to the requested new size.

   ``cork_xreallocf`` is more safe than the standard ``realloc`` function.  A
   common idiom when calling ``realloc`` is::

       void  *ptr = /* from somewhere */;
       /* UNSAFE!  Do not do this! */
       ptr = realloc(ptr, new_size);

   This is unsafe!  The ``realloc`` function returns a ``NULL`` pointer if the
   reallocation fails.  By assigning directly into *ptr*, you'll get a memory
   leak in these situations.  The ``cork_xreallocf`` function, on the other
   hand, will automatically free the existing pointer if the reallocation fails,
   eliminating the memory leak::

       void  *ptr = /* from somewhere */;
       /* This is safe.  Do this. */
       ptr = cork_xreallocf(ptr, new_size);
       /* Check whether ptr is NULL before using it! */

   Each allocation function has a corresponding deallocation function that you
   must use to free the memory when you are done with it: use
   :c:func:`cork_free` to free memory allocated using ``cork_xmalloc``,
   ``cork_xrealloc``, or ``cork_xreallocf``; use :c:func:`cork_cfree` to free
   memory allocated using ``cork_xcalloc``; and use :c:func:`cork_delete` to
   free memory allocated using ``cork_xnew``.


Deallocation
------------

Since this is C, you must free any memory region once you're done with it.
You must use one of the functions from this section to free any memory that you
created using any of the allocation functions described previously.

.. function:: void \*cork_free(void \*ptr, size_t size)
              void \*cork_cfree(void \*ptr, size_t count, size_t size)
              type \*cork_delete(void \*ptr, TYPE type)

   Frees a region of memory allocated by one of libcork's allocation functions.

   Note that unlike the standard ``free`` function, you must provide the size of
   the allocated region when it's freed, as well as when it's created.  Most of
   the time this isn't an issue, since you're either freeing a region whose size
   is known at compile time, or you're already keeping track of the size of a
   dynamically sized memory region for some other reason.

   You should use ``cork_free`` to free memory allocated using
   :c:func:`cork_malloc`, :c:func:`cork_realloc`, :c:func:`cork_xmalloc`,
   :c:func:`cork_xrealloc`, or :c:func:`cork_xreallocf`.  You should use
   ``cork_cfree`` to free memory allocated using :c:func:`cork_calloc` or
   :c:func:`cork_xcalloc`.  You should use ``cork_delete`` to free memory
   allocated using :c:func:`cork_new` or :c:func:`cork_xnew`.


Duplicating strings
-------------------

.. function:: const char \*cork_strdup(const char \*str)
              const char \*cork_strndup(const char \*str, size_t size)
              const char \*cork_xstrdup(const char \*str)
              const char \*cork_xstrndup(const char \*str, size_t size)

   These functions mimic the standard ``strdup`` function.  They create a copy
   of an existing C string, allocating exactly as much memory is needed to hold
   the copy.

   The ``strdup`` variants calculate the size of *str* using ``strlen``.  For
   the ``strndup`` variants, *str* does not need to be ``NUL``-terminated, and
   you must pass in its *size*.  (Note that is different than the standard
   ``strndup``, where *str* must be ``NUL``-terminated, and which copies **at
   most** *size* bytes.  Our version always copies **exactly** *size* bytes.)
   The result is guaranteed to be ``NUL``-terminated, even if the source *str*
   is not.

   You shouldn't modify the contents of the copied string.  You must use
   :c:func:`cork_strfree()` to free the string when you're done with it.  The
   ``x`` variant returns a ``NULL`` pointer if the allocation fails; the non-\
   ``x`` variant is guaranteed to return a valid pointer to a copied string.

.. function:: void cork_strfree(const char \*str)

   Frees *str*, which must have been created using
   :c:func:`cork_strdup()` or :c:func:`cork_xstrdup()`.


.. _libcork-allocators:

Customizing how libcork allocates
=================================

The ``cork_alloc`` type encapsulates a particular memory allocation scheme.  To
customize how libcork allocates memory, you create a new instance of this type,
and then use :c:func:`cork_set_allocator` to register it with libcork.

.. function:: void cork_set_allocator(const struct cork_alloc \*alloc)

   Override which :ref:`allocator instance <allocators>` libcork will use to
   create and free memory.  We will take control of *alloc*; you must not free
   it yourself after passing it to this function.

   You can only call this function at most once.  This function is **not**
   thread-safe; it's only safe to call before you've called **any** other
   libcork function (or any function from any other library that uses libcork.
   (The only exceptions are libcork functions that take in a
   :c:type:`cork_alloc` parameter or return a :c:type:`cork_alloc` result; these
   functions are safe to call before calling ``cork_set_allocator``.)

.. var:: const struct cork_alloc \*cork_allocator

   The current :ref:`allocator instance <allocators>` that libcork will use to
   create and free memory.


.. _allocators:

Writing a custom allocator
--------------------------

.. type:: struct cork_alloc

   The ``cork_alloc`` type contains several methods for performing different
   allocation and deallocation operations.

   You are only required to provide implementations of ``xmalloc`` and ``free``;
   we can provide default implementations of all of the other methods in terms
   of those two.  You can provide optimized versions of the other methods, if
   appropriate.


.. function:: struct cork_alloc \*cork_alloc_new_alloc(const struct cork_alloc \*parent)

   ``cork_alloc_new`` creates a new allocator instance.  The new instance will
   itself be allocated using *parent*.  You must provide implementations of at
   least the ``xmalloc`` and ``free`` methods.  You can also override our
   default implementations of any of the other methods.

   This function is **not** thread-safe; it's only safe to call before you've
   called **any** other libcork function (or any function from any other library
   that uses libcork.  (The only exceptions are libcork functions that take in a
   :c:type:`cork_alloc` parameter or return a :c:type:`cork_alloc` result; these
   functions are safe to call before calling ``cork_set_allocator``.)

   The new allocator instance will automatically be freed when the process
   exits.  If you registered a *user_data* pointer for your allocation methods
   (via :c:func:`cork_alloc_set_user_data`), it will be freed using the
   *free_user_data* method you provided.  If you create more than one
   ``cork_alloc`` instance in the process, they will be freed in the reverse
   order that they were created.

   .. note::

      In your allocator implementation, you cannot assume that the rest of the
      libcork allocation framework has been set up yet.  So if your allocator
      needs to allocate, you must not use the usual :c:func:`cork_malloc` family
      of functions; instead you should use the ``cork_alloc_malloc`` variants to
      explicitly allocate memory using your new allocator's *parent*.


.. function:: void cork_alloc_set_user_data(struct cork_alloc \*alloc, void \*user_data, cork_free_f free_user_data)

   Provide a *user_data* pointer, which will be passed unmodified to each
   allocation method that you register.  You can also provide an optional
   *free_user_data* method, which we will use to free the *user_data* instance
   when the allocator itself is freed.


.. function:: void cork_alloc_set_calloc(struct cork_alloc \*alloc, cork_alloc_calloc_f calloc)
              void cork_alloc_set_xcalloc(struct cork_alloc \*alloc, cork_alloc_calloc_f calloc)

   .. type:: void \*(\*cork_alloc_calloc_f)(const struct cork_alloc \*alloc, size_t count, size_t size)

      These methods are used to implement the :c:func:`cork_calloc` and
      :c:func:`cork_xcalloc` functions.  Your must allocate and return ``count *
      size`` bytes of memory.  You must ensure that every byte in this region is
      initialized to ``0``.  The ``calloc`` variant must always return a valid
      pointer; if memory allocation fails, it must not return.  The ``xcalloc``
      variant should return ``NULL`` if allocation fails.


.. function:: void cork_alloc_set_malloc(struct cork_alloc \*alloc, cork_alloc_malloc_f malloc)
              void cork_alloc_set_xmalloc(struct cork_alloc \*alloc, cork_alloc_malloc_f malloc)

   .. type:: void \*(\*cork_alloc_malloc_f)(const struct cork_alloc \*alloc, size_t size)

      These methods are used to implement the :c:func:`cork_malloc` and
      :c:func:`cork_xmalloc` functions.  You must allocate and return *size*
      bytes of memory.  The ``malloc`` variant must always return a valid
      pointer; if memory allocation fails, it must not return.  The ``xmalloc``
      variant should return ``NULL`` if allocation fails.


.. function:: void cork_alloc_set_realloc(struct cork_alloc \*alloc, cork_alloc_realloc_f realloc)
              void cork_alloc_set_xrealloc(struct cork_alloc \*alloc, cork_alloc_realloc_f realloc)

   .. type:: void \*(\*cork_alloc_realloc_f)(const struct cork_alloc \*alloc, void \*ptr, size_t old_size, size_t new_size)

      These methods are used to implement the :c:func:`cork_realloc`,
      :c:func:`cork_xrealloc`, and :c:func:`cork_xreallocf` functions.  You
      must reallocate *ptr* to contain *new_size* bytes of memory and return the
      reallocated pointer.  *old_size* will be the previously allocated size of
      *ptr*.  The ``realloc`` variant must always return a valid pointer; if
      memory reallocation fails, it must not return.  The ``xrealloc`` variant
      should return ``NULL`` if reallocation fails.


.. function:: void cork_alloc_set_free(struct cork_alloc \*alloc, cork_alloc_free_f free)

   .. type:: void \*(\*cork_alloc_free_f)(const struct cork_alloc \*alloc, void \*ptr, size_t size)

      These methods are used to implement the :c:func:`cork_free`,
      :c:func:`cork_cfree`, and :c:func:`cork_delete` functions.  You must
      deallocate *ptr*.  *size* will be the allocated size of *ptr*.
