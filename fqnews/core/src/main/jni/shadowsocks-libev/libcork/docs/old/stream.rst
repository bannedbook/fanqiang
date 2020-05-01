.. _stream:

*****************
Stream processing
*****************

.. highlight:: c

::

  #include <libcork/ds.h>


Stream producers
----------------

A *producer* of binary data should take in a pointer to a
:c:type:`cork_stream_consumer` instance.  Any data that is produced by the
stream is then sent into the consumer instance for processing.  Once the stream
has been exhausted (for instance, by reaching the end of a file), you signal
this to the consumer.  During both of these steps, the consumer is able to
signal error conditions; for instance, a stream consumer that parses a
particular file format might return an error condition if the stream of data is
malformed.  If possible, the stream producer can try to recover from the error
condition, but more often, the stream producer will simply pass the error back
up to its caller.

.. function:: int cork_stream_consumer_data(struct cork_stream_consumer \*consumer, const void \*buf, size_t size, bool is_first_chunk)

   Send the next chunk of data into a stream consumer.  You only have to ensure
   that *buf* is valid for the duration of this function call; the stream
   consumer is responsible for saving a copy of the data if it needs to be
   processed later.  In particular, this means that it's perfectly safe for
   *buf* to refer to a stack-allocated memory region.

.. function:: int cork_stream_consumer_eof(struct cork_stream_consumer \*consumer)

   Notify the stream consumer that the end of the stream has been reached.  The
   stream consumer might perform some final validation and error detection at
   this point.

.. function:: void cork_stream_consumer_free(struct cork_stream_consumer \*consumer)

   Finalize and deallocate a stream consumer.


Built-in stream producers
~~~~~~~~~~~~~~~~~~~~~~~~~

We provide several built-in stream producers:

.. function:: int cork_consume_fd(struct cork_stream_consumer \*consumer, int fd)
              int cork_consume_file(struct cork_stream_consumer \*consumer, FILE \*fp)
              int cork_consume_file_from_path(struct cork_stream_consumer \*consumer, const char \*path, int flags)

   Read in a file, passing its contents into the given stream consumer.  The
   ``_fd`` and ``_file`` variants consume a file that you've already opened; you
   are responsible for closing the file after its been consumed.  The
   ``_file_from_path`` variant will open the file for you, using the standard
   ``open(2)`` function with the given *flags*.  This variant will close the
   file before returning, regardless of whether the file was successfully
   consumed or not.


File stream producer example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As an example, we could implement the :c:func:`cork_consume_file` stream
producer as follows::

  #include <stdio.h>
  #include <libcork/core.h>
  #include <libcork/helpers/errors.h>
  #include <libcork/ds.h>

  #define BUFFER_SIZE  65536

  int
  cork_consume_file(struct cork_stream_consumer *consumer, FILE *fp)
  {
      char  buf[BUFFER_SIZE];
      size_t  bytes_read;
      bool  first = true;

      while ((bytes_read = fread(buf, 1, BUFFER_SIZE, fp)) > 0) {
          rii_check(cork_stream_consumer_data(consumer, buf, bytes_read, first));
          first = false;
      }

      if (feof(fp)) {
          return cork_stream_consumer_eof(consumer);
      } else {
          cork_system_error_set();
          return -1;
      }
  }

Note that this stream producer does not take care of opening or closing
the ``FILE`` object, nor does it take care of freeing the consumer.  (Our actual
implementation of :c:func:`cork_consume_file` also correctly handles ``EINTR``
errors, and so is a bit more complex.  But this example still works as an
illustration of how to pass data into a stream consumer.)


.. _stream-consumers:

Stream consumers
----------------

To consume data from a stream, you must create a type that implements the
:c:type:`cork_stream_consumer` interface.

.. type:: struct cork_stream_consumer

   An interface for consumer a stream of binary data.  The producer of
   the stream will call the :c:func:`cork_stream_consumer_data()`
   function repeatedly, once for each successive chunk of data in the
   stream.  Once the stream has been exhausted, the producer will call
   :c:func:`cork_stream_consumer_eof()` to signal the end of the stream.

   .. member:: int (\*data)(struct cork_stream_consumer \*consumer, const void \*buf, size_t size, bool is_first_chunk)

      Process the next chunk of data in the stream.  *buf* is only
      guaranteed to be valid for the duration of this function call.  If
      you need to access the contents of the slice later, you must save
      it somewhere yourself.

      If there is an error processing this chunk of data, you should
      return ``-1`` and fill in the current error condition using
      :c:func:`cork_error_set`.

   .. member:: int (\*eof)(struct cork_stream_consumer \*consumer)

      Handle the end of the stream.  This allows you to defer any final
      validation or error detection until all of the data has been
      processed.

      If there is an error detected at this point, you should return
      ``-1`` and fill in the current error condition using
      :c:func:`cork_error_set`.

   .. member:: void (\*free)(struct cork_stream_consumer \*consumer)

      Free the consumer object.


Built-in stream consumers
~~~~~~~~~~~~~~~~~~~~~~~~~

We provide several built-in stream consumers:

.. function:: struct cork_stream_consumer \*cork_fd_consumer_new(int fd)
              struct cork_stream_consumer \*cork_file_consumer_new(FILE \*fp)
              struct cork_stream_consumer \*cork_file_from_path_consumer_new(const char \*path, int flags)

   Create a stream consumer that appends any data that it receives to a file.
   The ``_fd`` and ``_file`` variants append to a file that you've already
   opened; you are responsible for closing the file after the consumer has
   finished processing data.  The ``_file_from_path`` variant will open the file
   for you, using the standard ``open(2)`` function with the given *flags*.
   This variant will close the file before returning, regardless of whether the
   stream consumer successfully processed the data or not.


File stream consumer example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As an example, we could implement a stream consumer for the
:c:func:`cork_file_consumer_new` function as follows::

  #include <stdio.h>
  #include <libcork/core.h>
  #include <libcork/helpers/errors.h>
  #include <libcork/ds.h>

  struct cork_file_consumer {
      /* cork_file_consumer implements the cork_stream_consumer interface */
      struct cork_stream_consumer  parent;
      /* the file to write the data into */
      FILE  *fp;
  };

  static int
  cork_file_consumer__data(struct cork_stream_consumer *vself,
                           const void *buf, size_t size, bool is_first)
  {
      struct file_consumer  *self =
          cork_container_of(vself, struct cork_file_consumer, parent);
      size_t  bytes_written = fwrite(buf, 1, size, self->fp);
      /* If there was an error writing to the file, then signal this to
       * the producer */
      if (bytes_written == size) {
          return 0;
      } else {
          cork_system_error_set();
          return -1;
      }
  }

  static int
  cork_file_consumer__eof(struct cork_stream_consumer *vself)
  {
      /* We don't close the file, so there's nothing special to do at
       * end-of-stream. */
      return 0;
  }

  static void
  cork_file_consumer__free(struct cork_stream_consumer *vself)
  {
      struct file_consumer  *self =
          cork_container_of(vself, struct cork_file_consumer, parent);
      free(self);
  }

  struct cork_stream_consumer *
  cork_file_consumer_new(FILE *fp)
  {
      struct cork_file_consumer  *self = cork_new(struct cork_file_consumer);
      self->parent.data = cork_file_consumer__data;
      self->parent.eof = cork_file_consumer__eof;
      self->parent.free = cork_file_consumer__free;
      self->fp = fp;
      return &self->parent;
  }

Note that this stream consumer does not take care of opening or closing the
``FILE`` object.
