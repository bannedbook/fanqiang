.. _buffer:

************************
Resizable binary buffers
************************

.. highlight:: c

::

  #include <libcork/ds.h>

This section defines a resizable binary buffer type.  This class can
also be used to construct C strings, when you don't know the size of
the string in advance.

This class is not reference counted; we assume that there's a single
owner of the buffer.  The contents of a :c:type:`cork_buffer` are fully
mutable.  If you want to turn the buffer into something that's safe to
pass between threads, you can use the :c:func:`cork_buffer_to_slice()`
or :c:func:`cork_buffer_to_managed_buffer()` functions to create an
immutable managed wrapper around the buffer.

You can read the contents of the buffer by accessing the :c:member:`buf
<cork_buffer.buf>` and :c:member:`size <cork_buffer.size>` fields
directly.  However, to modify the contents of a buffer, you should use
the mutator functions described below, since they take care of
automatically resizing the underlying buffer when necessary.

.. note::

   This class always creates its own copy of any data added to the
   buffer; there aren't any methods for wrapping existing buffers
   without copying.  If you want to do that, you should use
   :ref:`managed-buffer` or :ref:`slice`.


.. type:: struct cork_buffer

   A resizable binary buffer.

   .. member:: void \*buf

      The current contents of the buffer.

   .. member:: size_t  size

      The current size of the buffer.


.. function:: void cork_buffer_init(struct cork_buffer \*buffer)
              struct cork_buffer CORK_BUFFER_INIT()

   Initialize a new buffer instance that you've allocated yourself
   (usually on the stack).  The ``CORK_BUFFER_INIT`` version can only be
   used as a static initializer.

   The preallocated ``cork_buffer`` instance that you provide doesn't
   include space for the content of the buffer; this will be allocated
   automatically as content is added.

.. function:: struct cork_buffer \*cork_buffer_new(void)

   Allocate and initialize a new buffer instance.

.. function:: void cork_buffer_done(struct cork_buffer \*buffer)

   Finalize a buffer, freeing any content that it contains.  This
   function should only be used for buffers that you allocated yourself,
   and initialized using :c:func:`cork_buffer_init()` or
   :c:func:`CORK_BUFFER_INIT()`.  You must **not** use this function to
   free a buffer allocated using :c:func:`cork_buffer_new()`.

.. function:: void cork_buffer_free(struct cork_buffer \*buffer)

   Finalize and deallocate a buffer, freeing any content that it
   contains.  This function should only be used for buffers allocated
   using :c:func:`cork_buffer_new()`.  You must **not** use this
   function to free a buffer initialized using
   :c:func:`cork_buffer_init()` or :c:func:`CORK_BUFFER_INIT()`.

.. function:: bool cork_buffer_equal(const struct cork_buffer \*buffer1, const struct cork_buffer \*buffer2)

   Compare two buffers for equality.

.. function:: void cork_buffer_ensure_size(struct cork_buffer \*buffer, size_t desired_size)

   Ensure that a buffer has allocated enough space to store at least
   *desired_size* bytes.  We won't shrink the size of the buffer's
   internal storage; if the buffer has already allocated at least
   *desired_size* bytes, the function acts as a no-op.

.. function:: uint8_t cork_buffer_byte(struct cork_buffer \*buffer, size_t index)
              char cork_buffer_char(struct cork_buffer \*buffer, size_t index)

   Return the byte or character at the given index in *buffer*.


Mutator functions
-----------------

Most of the mutator functions defined in this section come in two
variants: a ``_set`` function, which clears the buffer before adding new
content, and an ``_append`` function, which retains the old content,
adding the new content to the end of the buffer.

Each mutator function will automatically append an extra ``NUL`` byte to
the end of whatever content is placed into the buffer.  However, this
``NUL`` byte will **not** be included in the :c:member:`size
<cork_buffer.size>` of the buffer.  This ensures that the contents of
any ``cork_buffer`` can be used as a ``NUL``\ -terminated C string
(assuming that there aren't any internal ``NUL``\ s), even if the buffer
is constructed from a data source that doesn't include ``NUL``
terminators.

.. function:: void cork_buffer_clear(struct cork_buffer \*buffer)

   Clear a buffer.  This does not free any storage that the buffer has
   allocated; this storage will be reused if you add contents back to the
   buffer.

.. function:: void cork_buffer_truncate(struct cork_buffer \*buffer, size_t length)

   Truncate a buffer so that contains no more than *length* bytes.  If the
   buffer is already shorter than this, it is not modified.

.. function:: void cork_buffer_copy(struct cork_buffer \*dest, const struct cork_buffer \*src)
              void cork_buffer_append_copy(struct cork_buffer \*dest, const struct cork_buffer \*src)

   Copy the contents of the *src* buffer into *dest*.  The ``_set`` variant
   clears the buffer first, while the ``_append`` variant adds *src* to whatever
   content is already there.

.. function:: void cork_buffer_set(struct cork_buffer \*buffer, const void \*src, size_t length)
              void cork_buffer_append(struct cork_buffer \*buffer, const void \*src, size_t length)

   Copy the contents of *src* into a buffer.  The ``_set`` variant
   clears the buffer first, while the ``_append`` variant adds *src* to
   whatever content is already there.

.. function:: void cork_buffer_set_string(struct cork_buffer \*buffer, const char \*str)
              void cork_buffer_append_string(struct cork_buffer \*buffer, const char \*str)
              void cork_buffer_set_literal(struct cork_buffer \*buffer, const char \*str)
              void cork_buffer_append_literal(struct cork_buffer \*buffer, const char \*str)

   Copy the contents of *str* (which must be a ``NUL``\ -terminated C
   string) into a buffer.  The ``_set`` variants clears the buffer first,
   while the ``_append`` variants adds *str* to whatever content is
   already there.  The ``_literal`` variants only work when *str* is a C string
   literal; we use the ``sizeof`` operator to determine the length of the string
   at compile time.  The ``_string`` variants work with any C string; we use the
   builtin ``strlen`` function to determine the length of the string.

.. function:: void cork_buffer_printf(struct cork_buffer \*buffer, const char \*format, ...)
              void cork_buffer_vprintf(struct cork_buffer \*buffer, const char \*format, va_list args)
              void cork_buffer_append_printf(struct cork_buffer \*buffer, const char \*format, ...)
              void cork_buffer_append_vprintf(struct cork_buffer \*buffer, const char \*format, va_list args)

   Format data according to a ``printf`` format string, placing the
   result into a buffer.  The ``_append`` variants add the formatted
   string to whatever content is already in the buffer; the non-\
   ``_append`` variants clear the buffer first.  The ``_printf``
   variants are vararg functions, and take in the format string's data
   as direct parameters.  The ``_vprintf`` variants can be used within
   another vararg function, and let you pass in the format string's data
   as a C99-standard ``va_list`` instance.


Pretty-printing
---------------

We also provide several helper functions for adding pretty-printed content to a
``cork_buffer``.

.. function:: void cork_buffer_append_indent(struct cork_buffer \*buffer, size_t indent)

   Append *indent* spaces to *buffer*.

.. function:: void cork_buffer_append_c_string(struct cork_buffer \*buffer, const char \*str, size_t length)

   Append the C string literal representation of *str* to *buffer*.  This will
   include opening and closing double quotes, and any non-printable characters
   will be escaped.  (We will use the standard letter-based escapes where
   possible, and fall back on ``"\xXX"`` hexadecimal escapes for other
   non-printable characters.)  The result is guaranteed to stay on a single
   line, since any embedded newlines will be converted to a ``\n`` escape
   sequence.

.. function:: void cork_buffer_append_hex_dump(struct cork_buffer \*buffer, size_t indent, const char \*str, size_t length)
              void cork_buffer_append_multiline(struct cork_buffer \*buffer, size_t indent, const char \*str, size_t length)
              void cork_buffer_append_binary(struct cork_buffer \*buffer, size_t indent, const char \*str, size_t length)

   Append a pretty-printed representation of *str* to *buffer*.  All of these
   functions can produce multiple lines of output.  All lines except for the
   first will be prefaced with *indent* space characters.  The final line will
   **not** have a trailing newline.

   The ``hex_dump`` variant will output a hex-dump representation of *str*.
   This will include the hexadecimal representation of each byte, and the actual
   character of any printable byte.

   The ``multiline`` variant appends the raw content of *str* to the buffer,
   without making any attempt to sanitize non-printable characters.  (That means
   you should only call this variant if you know that *str* contains only
   printable characters.)  If *str* itself spans multiple lines, then we'll
   insert indentation to make sure that we satisfy the indentation rules
   described above.

   The ``binary`` variant autodetects how to best render *str*.  If it contains
   any non-printable characters, then we'll use the ``hex_dump`` representation.
   If it spans multiple lines, we'll use the ``multiline`` representation.
   Otherwise, we'll append the content directly without any modification.


Other binary data structures
----------------------------

The ``cork_buffer`` class is the only binary data class that is mutable;
this comes at the cost of only being usable by a single owner thread or
function at a time.  Once you have constructed a binary string or
payload using a ``cork_buffer``, you can use the functions in this
section to produce a corresponding instance of one of libcork's
sharable, immutable binary data types.

.. function:: struct cork_managed_buffer \*cork_buffer_to_managed_buffer(struct cork_buffer \*buffer)

   Create a new :ref:`managed buffer <managed-buffer>` to manage the
   contents of a ``cork_buffer`` instance.  *buffer* must have been
   allocated on the heap (i.e., using :c:func:`cork_buffer_new()`, and
   not :c:func:`cork_buffer_init()`).  We take ownership of *buffer*,
   regardless of whether we're able to successfully create a new
   :c:type:`cork_managed_buffer` instance.  You must **not** try to free
   *buffer* yourself.

.. function:: int cork_buffer_to_slice(struct cork_buffer \*buffer, struct cork_slice \*slice)

   Initialize a new :ref:`slice <slice>` to manage the contents of
   *buffer*.  *buffer* must have been allocated on the heap (i.e., using
   :c:func:`cork_buffer_new()`, and not :c:func:`cork_buffer_init()`).
   We take ownership of *buffer*, regardless of whether we're able to
   successfully create a new :c:type:`cork_managed_buffer` instance.
   You must **not** try to free *buffer* yourself.

   The slice will point into the contents of a new :ref:`managed buffer
   <managed-buffer>` instance.  The managed buffer isn't returned
   directly, though you can create additional slices into it using the
   usual :c:type:`cork_slice` methods.

   Regardless of whether we can initialize the slice successfully, you
   **must** call :c:func:`cork_slice_finish()` on *slice* when you're
   done with the slice.

.. function:: struct cork_stream_consumer \*cork_buffer_to_stream_consumer(struct cork_buffer \*buffer)

   Create a new stream consumer that appends any received data into
   *buffer*.

   We do **not** take control of *buffer*.  You retain responsibility
   for freeing the buffer, and you must ensure that it remains allocated
   and valid for the entire lifetime of the stream consumer that we
   return.
