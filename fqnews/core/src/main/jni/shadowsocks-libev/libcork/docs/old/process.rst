.. _processes:

*********
Processes
*********

.. highlight:: c

::

  #include <libcork/os.h>

The functions in this section let you interact with the current running process.


Cleanup functions
~~~~~~~~~~~~~~~~~

Often you will need to perform some cleanup tasks whenever the current process
terminates normally.  The functions in this section let you do that.

.. function:: void cork_cleanup_at_exit(int priority, cork_cleanup_function function)
              void cork_cleanup_at_exit_named(const char \*name, int priority, cork_cleanup_function function)

   Register a *function* that should be called when the current process
   terminates.  When multiple functions are registered, the order in which they
   are called is determined by their *priority* values --- functions with lower
   priorities will be called first.  If any functions have the same priority
   value, there is no guarantee about the order in which they will be called.

   All cleanup functions must conform to the following signature:

   .. type:: void (\*cork_cleanup_function)(void)

   The ``_named`` variant lets you provide an explicit name for the cleanup
   function, which currently is only used when printing out debug messages.  The
   plain variant automatically detects the name of *function*, so that you don't
   have to provide it explicitly.


.. _env:

Environment variables
~~~~~~~~~~~~~~~~~~~~~

.. type:: struct cork_env

   A collection of environment variables that can be passed to subprocesses.


.. function:: struct cork_env \*cork_env_new(void)

   Create a new, empty collection of environment variables.

.. function:: struct cork_env \*cork_env_clone_current(void)

   Create a new :c:type:`cork_env` containing all of the environment variables
   in the current process's environment list.

.. function:: void cork_env_free(struct cork_env \*env)

   Free a collection of environment variables.


.. function:: const char \*cork_env_get(struct cork_env \*env, const char \*name)

   Return the value of the environment variable with the given *name*.  If there
   is no variable with that name, return ``NULL``.

   If *env* is ``NULL``, then the variable is retrieved from the current process
   environment; otherwise, it is retrieved from *env*.

.. function:: void cork_env_add(struct cork_env \*env, const char \*name, const char \*value)
              void cork_env_add_printf(struct cork_env \*env, const char \*name, const char \*format, ...)
              void cork_env_add_vprintf(struct cork_env \*env, const char \*name, const char \*format, va_list args)

   Add a new environment variable with the given *name* and *value*.  If there
   is already a variable with that name, it is overwritten.  We make a copy of
   both *name* and *variable*, so it is safe to pass in temporary or reusable
   strings for either.  The ``printf`` and ``vprintf`` variants construct the
   new variable's value from a ``printf``-like format string.

   If *env* is ``NULL``, then the new variable is added to the current process
   environment; otherwise, it is added to *env*.

.. function:: void cork_env_remove(struct cork_env \*env, const char \*name)

   Remove the environment variable with the given *name*, if it exists.  If
   there isn't any variable with that name, do nothing.

   If *env* is ``NULL``, then the variable is removed from the current process
   environment; otherwise, it is removed from *env*.


.. function:: void cork_env_replace_current(struct cork_env \*env)

   Replace the current process's environment list with the contents of *env*.


.. _exec:

Executing another program
~~~~~~~~~~~~~~~~~~~~~~~~~

.. type:: struct cork_exec

   A specification for executing another program.


.. function:: struct cork_exec \*cork_exec_new(const char \*program)
              struct cork_exec \*cork_exec_new_with_params(const char \*program, ...)
              struct cork_exec \*cork_exec_new_with_param_array(const char \*program, char \* const \*params)

   Create a new specification for executing *program*.  *program* must either be
   an absolute path to an executable on the local filesystem, or the name of an
   executable that should be found in the current ``PATH``.

   The first variant creates a specification that initially doesn't contain any
   parameters to pass into the new program.  The second variant allows you to
   pass in each argument as a separate parameter; you must ensure that you
   terminate the list of parameters with a ``NULL`` pointer.  The third variant
   allows you to pass in a ``NULL``-terminated array of strings to use as an
   initial parameter list.  For all three variants, you can add additional
   parameters before executing the new program via the :c:func:`cork_add_param`
   function.

   .. note::

      Most programs will expect the first parameter to be the name of the
      program being executed.  The :c:func:`cork_exec_new_with_params` function
      will automatically fill in this first parameter for you.  The other
      constructor functions do not; when using them, it is your responsibility
      to provide this parameter, just like any other parameters to pass into the
      program.

   This function does not actually execute the program; that is handled by the
   :c:func:`cork_exec_run` function.

.. function:: void cork_exec_free(struct cork_exec \*exec)

   Free an execution specification.  You normally won't need to call this
   function; normally you'll replace the current process with the new program
   (by calling :c:func:`cork_exec_run`), which means you won't have a chance to
   free the specification object.

.. function:: const char \*cork_exec_description(struct cork_exec \*exec)

   Return a string description of the program described by an execution
   specification.

.. function:: void cork_exec_add_param(struct cork_exec \*exec, const char \*param)

   Add a parameter to the parameter list that will be passed into the new
   program.

.. function:: void cork_exec_set_env(struct cork_exec \*exec, struct cork_env \*env)

   Provide a set of environment variables that will be passed into the new
   program.  The subprocess's environment will contain only those variables
   defined in *env*.  You can use the :c:func:`cork_env_clone_current` function
   to create a copy of the current process's environment, to use it as a base to
   add new variables or remove unsafe variables.  We will take control of *env*,
   so you must **not** call :c:func:`cork_env_free` to free the environment
   yourself.

   If you don't call this function for a specification object, the new
   program will use the same environment as the calling process.

.. function:: void cork_exec_set_cwd(struct cork_exec \*exec, const char \directory)

   Change the working directory that the new program will be called from.  If
   you don't call this function for a specification object, the new program will
   be executed in the same working directory as the calling process.


.. function:: const char \*cork_exec_program(struct cork_exec \*exec)
              size_t \*cork_exec_param_count(struct cork_exec \*exec)
              const char \*cork_exec_param(struct cork_exec \*exec, size_t index)
              struct cork_env \*cork_exec_env(struct cork_exec \*exec)
              const char \*cork_exec_cwd(struct cork_exec \*exec)

   Accessor functions that allow you to retrieve the contents of an execution
   specification.  The :c:func:`cork_exec_env` and :c:func:`cork_exec_cwd`
   functions might return ``NULL``, if there isn't an environment or working
   directory specified.


.. function:: int cork_exec_run(struct cork_exec \*exec)

   Execute the program specified by *exec*, replacing the current process.
   If we can successfully start the new program, this function will not return.
   If there are any errors starting the program, this function will return an
   error condition.
