.. _multithreading:

**********************
Multithreading support
**********************

.. highlight:: c

::

  #include <libcork/threads.h>

libcork provides several functions for handling threads and writing
thread-aware code in a portable way.


.. _thread-ids:

Thread IDs
==========

.. type:: unsigned int cork_thread_id

   An identifier for a thread in the current process.  This is a portable type;
   it is not based on the "raw" thread ID used by the underlying thread
   implementation.  This type will always be equivalent to ``unsigned int``, on
   all platforms.  Furthermore, :c:data:`CORK_THREAD_NONE` will always refer to
   an instance of this type that we guarantee will not be used by any thread.

.. var:: cork_thread_id CORK_THREAD_NONE

   A :c:type:`cork_thread_id` value that will not be used as the ID of any
   thread.  You can use this value to represent "no thread" in any data
   structures you create.  Moreover, we guarantee that ``CORK_THREAD_NONE`` will
   have the value ``0``, which lets you zero-initialize a data structure
   containing a :c:type:`cork_thread_id`, and have its initial state
   automatically represent "no thread".

.. function:: cork_thread_id cork_current_thread_get_id(void)

   Returns the identifier of the currently executing thread.  This function
   works correctly for any thread in the current proces --- including the main
   thread, and threads that weren't created by :c:func:`cork_thread_new`.


.. _threads:

Creating threads
================

The functions in this section let you create and start new threads in the
current process.  Each libcork thread is named and has a unique :ref:`thread ID
<thread-ids>`.  Each thread also contains a ``run`` function, which defines the
code that should be executed within the new thread.

Every thread goes through the same lifecycle:

1) You create a new thread via :c:func:`cork_thread_new`.  At this point, the
   thread is ready to execute, but isn't automatically started.  If you
   encounter an error before you start the thread, you must use
   :c:func:`cork_thread_free` to free the thread object.

   When you create the thread, you give it a :c:type:`cork_run_f` function,
   which defines the code that will be executed in the new thread.  You also
   provide a ``user_data`` value, which it gives you a place to pass data into
   and out of the thread.

   .. note::

      Any data passed into and out of the thread via the body instance is not
      automatically synchronized or thread-safe.  You can safely pass in input
      data before calling :c:type:`cork_thread_new`, and retrieve output data
      after calling :c:type:`cork_thread_join`, all without requiring any extra
      synchronization effort.  While the thread is executing, however, you must
      implement your own synchronization or locking to access the contents of
      the body from some other thread.

2) You start the thread via :c:func:`cork_thread_start`.  You must ensure that
   you don't try to start a thread more than once.  Once you've started a
   thread, you no longer have responsibility for freeing it; you must ensure
   that you don't call :c:func:`cork_thread_free` on a thread that you've
   started.

3) Once you've started a thread, you wait for it to finish via
   :c:func:`cork_thread_join`.  Any thread can wait for any other thread to
   finish, although you are responsible for ensuring that your threads don't
   deadlock.  However, you can only join a particular thread once.




.. type:: struct cork_thread

   A thread within the current process.  This type is opaque; you must use the
   functions defined below to interact with the thread.


.. function:: struct cork_thread \*cork_thread_new(const char \*name, void \*user_data, cork_free_f free_user_data, cork_run_f run)

   Create a new thread with the given *name* that will execute *run*.  The
   thread does not start running immediately.

   When the thread is started, the *run* function will be called with
   *user_data* as its only parameter.  When the thread finishes (or if it is
   freed via :c:func:`cork_thread_free` before the thread is started), we'll use
   the *free_user_data* function to free the *user_data* value.  You can provide
   ``NULL`` if *user_data* shouldn't be freed, or if you want to free it
   yourself.

   .. note::

      If you provide a *free_user_data* function, it will be called as soon as
      the thread finished.  That means that if you use
      :c:func:`cork_thread_join` to wait for the thread to finish, the
      *user_data* value will no longer be valid when :c:func:`cork_thread_join`
      returns.  You must either copy any necessary data out into more a more
      persistent memory location before the thread finishes, or you should use a
      ``NULL`` *free_user_data* function and free the *user_data* memory
      yourself once you're sure the thread has finished.


.. function:: void cork_thread_free(struct cork_thread \*thread)

   Free *thread*.  You can only call this function if you haven't started the
   thread yet.  Once you start a thread, the thread is responsible for freeing
   itself when it finishes.

.. function:: struct cork_thread \*cork_current_thread_get(void)

   Return the :c:type:`cork_thread` instance for the current thread.  This
   function returns ``NULL`` when called from the main thread (i.e., the one
   created automatically when the process starts), or from a thread that wasn't
   created via :c:func:`cork_thread_new`.

.. function:: const char \* cork_thread_get_name(struct cork_thread \*thread)
              cork_thread_id cork_thread_get_id(struct cork_thread \*thread)

   Retrieve information about the given thread.

.. function:: int cork_thread_start(struct cork_thread \*thread)

   Start *thread*.  After calling this function, you must not try to free
   *thread* yourself; the thread will automatically free itself once it has
   finished executing and has been joined.

.. function:: int cork_thread_join(struct cork_thread \*thread)

   Wait for *thread* to finish executing.  If the thread's body's ``run``
   function an :ref:`error condition <errors>`, we will catch that error and
   return it ourselves.  The thread is automatically freed once it finishes
   executing.

   You cannot join a thread that has not been started, and once you've started a
   thread, you **must** join it exactly once.  (If you don't join it, there's no
   guarantee that it will be freed.)


.. _atomics:

Atomic operations
=================

We provide several platform-agnostic macros for implementing common
atomic operations.


Addition
~~~~~~~~

.. function:: int cork_int_atomic_add(volatile int \*var, int delta)
              unsigned int cork_uint_atomic_add(volatile unsigned int \*var, unsigned int delta)
              size_t cork_size_atomic_add(volatile size_t \*var, size_t delta)

   Atomically add *delta* to the variable pointed to by *var*, returning
   the result of the addition.

.. function:: int cork_int_atomic_pre_add(volatile int \*var, int delta)
              unsigned int cork_uint_atomic_pre_add(volatile unsigned int \*var, unsigned int delta)
              size_t cork_size_atomic_pre_add(volatile size_t \*var, size_t delta)

   Atomically add *delta* to the variable pointed to by *var*, returning
   the value from before the addition.


Subtraction
~~~~~~~~~~~

.. function:: int cork_int_atomic_sub(volatile int \*var, int delta)
              unsigned int cork_uint_atomic_sub(volatile unsigned int \*var, unsigned int delta)
              size_t cork_size_atomic_sub(volatile size_t \*var, size_t delta)

   Atomically subtract *delta* from the variable pointed to by *var*,
   returning the result of the subtraction.

.. function:: int cork_int_atomic_pre_sub(volatile int \*var, int delta)
              unsigned int cork_uint_atomic_pre_sub(volatile unsigned int \*var, unsigned int delta)
              size_t cork_size_atomic_pre_sub(volatile size_t \*var, size_t delta)

   Atomically subtract *delta* from the variable pointed to by *var*,
   returning the value from before the subtraction.


Compare-and-swap
~~~~~~~~~~~~~~~~

.. function:: int cork_int_cas(volatile int_t \*var, int old_value, int new_value)
              unsigned int cork_uint_cas(volatile uint_t \*var, unsigned int old_value, unsigned int new_value)
              size_t cork_size_cas(volatile size_t \*var, size_t old_value, size_t new_value)
              TYPE \*cork_ptr_cas(TYPE \* volatile \*var, TYPE \*old_value, TYPE \*new_value)

   Atomically check whether the variable pointed to by *var* contains
   the value *old_value*, and if so, update it to contain the value
   *new_value*.  We return the value of *var* before the
   compare-and-swap.  (If this value is equal to *old_value*, then the
   compare-and-swap was successful.)


.. _once:

Executing something once
========================

The functions in this section let you ensure that a particular piece of
code is executed exactly once, even if multiple threads attempt the
execution at roughly the same time.

.. macro:: cork_once_barrier(name)

   Declares a barrier that can be used with the :c:func:`cork_once`
   macro.

.. macro:: cork_once(barrier, call)
           cork_once_recursive(barrier, call)

   Ensure that *call* (which can be an arbitrary statement) is executed
   exactly once, regardless of how many times control reaches the call
   to ``cork_once``.  If control reaches the ``cork_once`` call at
   roughly the same time in multiple threads, exactly one of them will
   be allowed to execute the code.  The call to ``cork_once`` won't
   return until *call* has been executed.

   If you have multiple calls to ``cork_once`` that use the same
   *barrier*, then exactly one *call* will succeed.  If the *call*
   statements are different in those ``cork_once`` invocations, then
   it's undefined which one gets executed.

   If the function that contains the ``cork_once`` call is recursive, then you
   should call the ``_recursive`` variant of the macro.  With the ``_recursive``
   variant, if the same thread tries to obtain the underlying lock multiple
   times, the second and later calls will silently succeed.  With the regular
   variant, you'll get a deadlock in this case.

These macros are usually used to initialize a static variable that will
be shared across multiple threads::

    static struct my_type  shared_value;

    static void
    expensive_initialization(void)
    {
        /* do something to initialize shared_value */
    }

    cork_once_barrier(shared_value_once);

    struct my_type *
    get_shared_value(void)
    {
        cork_once(shared_value_once, expensive_initialization());
        return &shared_value;
    }

Each thread can then call ``get_shared_value`` to retrieve a properly
initialized instance of ``struct my_type``.  Regardless of how many
threads call this function, and how often they call it, the value will
be initialized exactly once, and will be guaranteed to be initialized
before any thread tries to use it.


.. _tls:

Thread-local storage
====================

The macro in this section can be used to create thread-local storage in
a platform-agnostic manner.

.. macro:: cork_tls(TYPE type, SYMBOL name)

   Creates a static function called :samp:`{[name]}_get`, which will
   return a pointer to a thread-local instance of *type*.  This is a
   static function, so it won't be visible outside of the current
   compilation unit.

   When a particular thread's instance is created for the first time, it
   will be filled with ``0`` bytes.  If the actual type needs more
   complex initialization before it can be used, you can create a
   wrapper struct that contains a boolean indiciating whether that
   initialization has happened::

       struct wrapper {
           bool  initialized;
           struct real_type  val;
       };

       cork_tls(struct wrapper, wrapper);

       static struct real_type *
       real_type_get(void)
       {
           struct wrapper * wrapper = wrapper_get();
           struct real_type * real_val = &wrapper->val;
           if (CORK_UNLIKELY(!wrapper->initialized)) {
               expensive_initialization(real_val);
           }
           return real_val;
       }

   It's also not possible to provide a finalization function; if your
   thread-local variable acquires any resources or memory that needs to
   be freed when the thread finishes, you must make a “thread cleanup”
   function that you explicitly call at the end of each thread.

   .. note::

      On some platforms, the number of thread-local values that can be
      created by any given process is limited (i.e., on the order of 128
      or 256 values).  This means that you should limit the number of
      thread-local values you create, especially in a library.
