.. _subprocesses:

************
Subprocesses
************

.. highlight:: c

::

  #include <libcork/os.h>

The functions in this section let you fork child processes, run arbitrary
commands in them, and collect any output that they produce.


Subprocess objects
~~~~~~~~~~~~~~~~~~

.. type:: struct cork_subprocess

   Represents a child process.  There are several functions for creating child
   processes, described below.


.. function:: void cork_subprocess_free(struct cork_subprocess \*sub)

   Free a subprocess.  The subprocess must not currently be executing.


Creating subprocesses
~~~~~~~~~~~~~~~~~~~~~

There are several functions that you can use to create and execute child
processes.

.. function:: struct cork_subprocess \*cork_subprocess_new(void \*user_data, cork_free_f free_user_data, cork_run_f run, struct cork_stream_consumer \*stdout, struct cork_stream_consumer \*stderr, int \*exit_code)
              struct cork_subprocess \*cork_subprocess_new_exec(struct cork_exec \*exec, struct cork_stream_consumer \*stdout, struct cork_stream_consumer \*stderr, int \*exit_code)

   Create a new subprocess specification.  The first variant will execute the
   given *run* function in the subprocess.  The second variant will execute a
   new program in the subprocess; the details of the program to execute are
   given by a :c:type:`cork_exec` specification object.

   For both of these functions, you can collect the data that the subprocess
   writes to its stdout and stderr streams by passing in :ref:`stream consumer
   <stream-consumers>` instances for the *stdout* and/or *stderr* parameters.
   If either (or both) of these parameters is ``NULL``, then the child process
   will inherit the corresponding output stream from the current process.
   (Usually, this means that the child's stdout or stderr will be interleaved
   with the parent's.)

   If you provide a non-``NULL`` pointer for the *exit_code* parameter, then we
   will fill in this pointer with the exit code of the subprocess when it
   finishes.  For :c:func:`cork_subprocess_new_exec`, the exit code is the value
   passed to the builtin ``exit`` function, or the value returned from the
   subprocess's ``main`` function.  For :c:func:`cork_subprocess_new`, the exit
   code is the value returned from the thread body's *run* function.


You can also create *groups* of subprocesses.  This lets you start up several
subprocesses at the same time, and wait for them all to finish.

.. type:: struct cork_subprocess_group

   A group of subprocesses that will all be executed simultaneously.

.. function:: struct cork_subprocess_group \*cork_subprocess_group_new(void)

   Create a new group of subprocesses.  The group will initially be empty.

.. function:: void cork_subprocess_group_free(struct cork_subprocess_group \*group)

   Free a subprocess group.  This frees all of the subprocesses in the group,
   too.  If you've started executing the subprocesses in the group, you **must
   not** call this function until they have finished executing.  (You can use
   the :c:func:`cork_subprocess_group_is_finished` function to see if the group
   is still executing, and the :c:func:`cork_subprocess_group_abort` to
   terminate the subprocesses before freeing the group.)

.. function:: void cork_subprocess_group_add(struct cork_subprocess_group \*group, struct cork_subprocess \*sub)

   Add the given subprocess to *group*.  The group takes control of the
   subprocess; you should not try to free it yourself.


Once you've created your subprocesses, you can start them executing:

.. function:: int cork_subprocess_start(struct cork_subprocess \*sub)
              int cork_subprocess_group_start(struct cork_subprocess_group \*group)

   Execute the given subprocess, or all of the subprocesses in *group*.  We
   immediately return once the processes have been started.  You can use the
   :c:func:`cork_subprocess_drain`, :c:func:`cork_subprocess_wait`,
   :c:func:`cork_subprocess_group_drain`, and
   :c:func:`cork_subprocess_group_wait` functions to wait for the subprocesses
   to complete.

   If there are any errors starting the subprocesses, we'll terminate any
   subprocesses that we were able to start, set an :ref:`error condition
   <errors>`, and return ``-1``.


Since we immediately return after starting the subprocesses, you must somehow
wait for them to finish.  There are two strategies for doing so.  If you don't
need to communicate with the subprocesses (by writing to their stdin streams or
sending them signals), the simplest strategy is to just wait for them to finish:

.. function:: int cork_subprocess_wait(struct cork_subprocess \*sub)
              int cork_subprocess_group_wait(struct cork_subprocess_group \*group)

   Wait until the given subprocess, or all of the subprocesses in *group*, have
   finished executing.  While waiting, we'll continue to read data from the
   subprocesses stdout and stderr streams as we can.

   If there are any errors reading from the subprocesses, we'll terminate all of
   the subprocesses that are still executing, set an :ref:`error condition
   <errors>`, and return ``-1``.  If the group has already finished, the
   function doesn't do anything.

As an example::

    struct cork_subprocess_group  *group = /* from somewhere */;
    /* Wait for the subprocesses to finish */
    if (cork_subprocess_group_wait(group) == -1) {
        /* An error occurred; handle it! */
    }

    /* At this point, we're guaranteed that the subprocesses have all been
     * terminated; either everything finished successfully, or the subprocesses
     * were terminated for us when an error was detected. */
    cork_subprocess_group_free(group);


If you do need to communicate with the subprocesses, then you need more control
over when we try to read from their stdout and stderr streams.  (The pipes that
connect the subprocesses to the parent process are fixed size, and so without
careful orchestration, you can easily get a deadlock.  Moreover, the right
pattern of reading and writing depends on the subprocesses that you're
executing, so it's not something that we can handle for you automatically.)

.. function:: struct cork_stream_consumer \*cork_subprocess_stdin(struct cork_subprocess \*sub)

   Return a :ref:`stream consumer <stream-consumers>` that lets you write data
   to the subprocess's stdin.  We do not buffer this data in any way; calling
   :c:func:`cork_stream_consumer_data` immediately tries to write the given data
   to the subprocess's stdin stream.  This can easily lead to deadlock if you do
   not manage the subprocess's particular orchestration correctly.

.. function:: bool cork_subprocess_is_finished(struct cork_subprocess \*sub)
              bool cork_subprocess_group_is_finished(struct cork_subprocess_group \*group)

   Return whether the given subprocess, or all of the subprocesses in *group*,
   have finished executing.

.. function:: int cork_subprocess_abort(struct cork_subprocess \*sub)
              int cork_subprocess_group_abort(struct cork_subprocess_group \*group)

   Immediately terminate the given subprocess, or all of the subprocesses in
   *group*.  This can be used to clean up if you detect an error condition and
   need to close the subprocesses early.  If the group has already finished, the
   function doesn't do anything.

.. function:: bool cork_subprocess_drain(struct cork_subprocess \*sub)
              bool cork_subprocess_group_drain(struct cork_subprocess_group \*group)

   Check the given subprocess, or all of the subprocesses in *group*, for any
   output on their stdout and stderr streams.  We'll read in as much data as we
   can from all of the subprocesses without blocking, and then return.  (Of
   course, we only do this for those subprocesses that you provided stdout or
   stderr consumers for.)

   This function lets you pass data into the subprocesses's stdin streams, or
   (**TODO: eventually**) send them signals, and handle any orchestration that's
   necessarily to ensure that the subprocesses don't deadlock.

   The return value indicates whether any "progress" was made.  We will return
   ``true`` if we were able to read any data from any of the subprocesses, or if
   we detected that any of the subprocesses exited.

   If there are any errors reading from the subprocesses, we'll terminate all of
   the subprocesses that are still executing, set an :ref:`error condition
   <errors>`, and return ``false``.  If the group has already finished, the
   function doesn't do anything.

To do this, you continue to "drain" the subprocesses whenever you're ready to
read from their stdout and stderr streams.  You repeat this in a loop, writing
to the stdin streams or sending signals as necessary, until all of the
subprocesses have finished::

    struct cork_subprocess_group  *group = /* from somewhere */;
    while (!cork_subprocess_group_is_finished(group)) {
        /* Drain the stdout and stderr streams */
        if (cork_subprocess_group_drain(group) == -1) {
            /* An error occurred; handle it! */
        } else {
            /* Write to the stdin streams or send signals */
        }
    }

    /* At this point, we're guaranteed that the subprocesses have all been
     * terminated; either everything finished successfully, or the subprocesses
     * were terminated for us when an error was detected. */
    cork_subprocess_group_free(group);
