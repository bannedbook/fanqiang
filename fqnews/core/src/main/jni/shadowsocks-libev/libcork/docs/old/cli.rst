.. _cli:

*********************
Command-line programs
*********************

.. highlight:: c

::

  #include <libcork/cli.h>

The functions in this section let you easily create complex command-line
applications that include subcommands, in the style of the ``git`` or ``svn``
programs.


Overview
========

If you're designing an application where you want to provide command-line access
to many different operations or use cases, the simplest solution is to create a
separate executable for each one.  This can clutter up the user's
``$PREFIX/bin`` directory, however, and can add complexity to your code base.
Many projects instead create a single “super-command” executable, which includes
within it all of the operations that you want to support.  You choose specific
operations by selecting a *subcommand* on the command line.

.. type:: struct cork_command

   An opaque type describing one of the subcommands in an executable.

So, for instance, if you were writing a library for manipulating sets of
objects, you could define several subcommands of a single ``set`` executable:

.. code-block:: none

    $ set add <filename> <element>
    $ set query <filename> <element>
    $ set remove <filename> <element>
    $ set union -o <output file> <file1> <file2>
    $ set print avro <filename>
    $ set print json <filename>

Each of these operations acts in exactly the same as if they were defined as
separate executables:

.. code-block:: none

    $ set-add <filename> <element>
    $ set-query <filename> <element>
    $ set-remove <filename> <element>
    $ set-union -o <output file> <file1> <file2>
    $ set-print-avro <filename>
    $ set-print-json <filename>

Note that you're not limited to one level of subcommands.  The ``set print``
subcommand, for instance, itself contains two subcommands: ``avro`` and
``json``.


Leaf commands
=============

A *leaf command* is a subcommand that represents one operation in your
executable.  In the example above, there are six leaf commands: ``set add``,
``set query``, ``set remove``, ``set union``, ``set print avro``, and ``set
print json``.

To define a leaf command, you use the following macro:

.. macro:: cork_leaf_command(const char \*name, const char \*short_description, const char \*usage, const char \*full_help, cork_option_parser parse_options, run)

   Returns :c:type:`cork_command` instance that defines a leaf command.  *name*
   is the name of the leaf command; this is the word that the user must type on
   the command-line to select this command.  (For ``set add``, this would be
   ``add``; for ``set print avro``, this would be ``avro``.)

   *short_description*, *usage*, and *full_help* should be static strings, and
   will be used to produce various forms of :ref:`help text <cli-help>` for the
   subcommand.  *short_description* should fit into one line; this will be used
   as the short description of this leaf command when we print out a list of all
   of the subcommands that are in the command set that this leaf belongs to.
   *usage* will be printed whenever we need to print out a usage synopsis.  This
   should describe the options and arguments to the leaf command; it will be
   printed after the full name of the subcommand.  (For instance, using the
   example above, the ``set add`` command's usage text would be ``<filename>
   <element>``.)  *full_help* should be a longer, multi-line string that
   describes the subcommand *in full detail*.  We will automatically preface the
   help text with the usage summary for the command.

   *parse_options* is a function that will be used to parse any command-line
   options that appear *after* the subcommand's name on the command line.  (See
   :ref:`below <cli-options>` for more details.)  This can be ``NULL`` if the
   subcommand does not have any options.

   *run* is the function that will be called to actually execute the command.
   Any options will have already been processed by the *parse_options* function;
   you should stash the option values into global or file-scope variables, and
   then use the contents of those variables in this function.  Your *run*
   function must be an instance of the :c:type:`cork_leaf_command_run` function
   type:

   .. type:: void (\*cork_leaf_command_run)(int argc, char \*\*argv)

      The *argc* and *argv* parameters will describe any values that appear on
      the command line after the name of the leaf command.  This will *not*
      include any options that were processed by the command's *parse_options*
      function.

As an example, we could define the ``set add`` command as follows::

    static void
    set_add_run(int argc, char **argv);

    #define SET_ADD_SHORT  "Adds an element to a set"
    #define SET_ADD_USAGE  "<filename> <element>"
    #define SET_ADD_FULL \
        "Loads in a set from <filename>, and adds <element> to the set.  The\n" \
        "new set will be written back out to <filename>.\n"

    static struct cork_command  set_add =
        cork_leaf_command("add", SET_ADD_SHORT, SET_ADD_USAGE, SET_ADD_FULL,
                          NULL, set_add_run);

    static void
    set_add_run(int argc, char **argv)
    {
        /* Verify that the user gave both required options... */
        if (argc < 1) {
            cork_command_show_help(&set_add, "Missing set filename.");
            exit(EXIT_FAILURE);
        }
        if (argc < 2) {
            cork_command_show_help(&set_add, "Missing element to add.");
            exit(EXIT_FAILURE);
        }

        /* ...and no others. */
        if (argc > 2) {
            cork_command_show_help(&set_add, "Too many values on command line.");
            exit(EXIT_FAILURE);
        }

        /* At this point, <filename> will be in argv[0], <element> will be in
         * argv[1]. */

        /* Do what needs to be done */
        exit(EXIT_SUCCESS);
    }

There are a few interesting points to make.  First, note that we use
preprocessor macros to define all of the help text for the command.  Also, note
that *each* line (including the last) of the full help text needs to have a
trailing newline included in the string literal.

Lastly, note that we still have to perform some final validation of the command
line arguments given by the user.  If the user hasn't satisfied the subcommand's
requirements, we use the :c:func:`cork_command_show_help` function to print out
a nice error message (including a usage summary of the subcommand), and then we
halt the executable using the standard ``exit`` function.


Command sets
============

A *command set* is a collection of subcommands.  Every executable will have at
least one command set, for the root executable itself.  It's also possible to
have nested command sets.  In our example above, ``set`` and ``set print`` are
both command sets.

To define a command set, you use the following macro:

.. macro:: cork_command_set(const char \*name, const char \*short_description, cork_option_parser parse_options, struct cork_command \*\*subcommands)

   Returns :c:type:`cork_command` instance that defines a command set.  *name*
   is the name of the command set; this is the word that the user must type on
   the command-line to select this set of commands.  If the user only specifies
   the name of the command set, then we'll print out a list of this set's
   subcommands, along with their short descriptions.  (For instance, running
   ``set`` on its own would describe the ``set add``, ``set query``, ``set
   remove``, ``set union``, and ``set print`` subcommands.  Running ``set
   print`` on its own would describe the ``set print avro`` and ``set print
   json`` commands.)

   *short_description*, should be a static strings, and will be used to produce
   various forms of :ref:`help text <cli-help>` for the command set.
   *short_description* should fit into one line; this will be used as the short
   description of this command when we print out a list of all of the
   subcommands that are in the command set that this command belongs to.

   *parse_options* is a function that will be used to parse any command-line
   options that appear *after* the command set's name on the command line, but
   *before* the name of one of the set's subcommands.  (See :ref:`below
   <cli-options>` for more details.)  This can be ``NULL`` if the command set
   does not have any options.

   *subcommands* should be an array of :c:type:`cork_command` pointers.  The
   array **must** have a ``NULL`` pointer as its last element.  The order of the
   subcommands in the array will effect the order that the commands are listed
   in the command set's help text.

As an example, we could define the ``set print`` command set as follows::

    /* Assuming set_print_avro and set_print_json were already defined
     * previously, using cork_leaf_command: */
    struct cork_command  set_print_avro = cork_leaf_command(...);
    struct cork_command  set_print_json = cork_leaf_command(...);

    /* "set print" command set */
    static struct cork_command  *set_print_subcommands[] = {
        &set_print_avro,
        &set_print_json,
        NULL
    };

    #define SET_PRINT_SHORT \
        "Print out the contents of a set in a variety of formats"

    static struct cork_command  set_print =
        cork_command_set("print", SET_PRINT_SHORT, NULL, &set_print_subcommands);

You must define your executable's top level of subcommands as a command set as
well.  For instance, we could define the ``set`` command set as follows::

    static struct cork_command  *root_subcommands[] = {
        &set_add,
        &set_query,
        &set_remove,
        &set_union,
        &set_print,
        NULL
    };

    static struct cork_command  root =
        cork_command_set("set", NULL, NULL, &root_subcommands);

Note that we don't need to provide a short description for the root command,
since it doesn't belong to any command sets.


Running the commands
====================

Once you've defined all of your subcommands, your executable's ``main`` function
is trivial::

    int
    main(int argc, char **argv)
    {
        return cork_command_main(&root, argc, argv);
    }

.. function:: int cork_command_main(struct cork_command \*root, int argc, char \*\*argv)

   Runs a subcommand, as defined by the command-line arguments given by *argc*
   and *argv*.  *root* should define the root command set for the executable.


.. _cli-help:

Help text
=========

The command-line programs created with this framework automatically support
generating several flavors of help text for its subcommands.  You don't need to
do anything special, except for ensuring that the actual help text that you
provide to the :c:macro:`cork_leaf_command` and :c:macro:`cork_command_set`
macros defined is intelligble and useful.

Your executable will automatically include a ``help`` command in every command
set, as well as ``--help`` and ``-h`` options in every command set and leaf
command.  So all of the following would print out the help text for the ``set
add`` command:

.. code-block:: none

    $ set help add
    $ set add --help
    $ set add -h

And all of the following would print out the list of ``set print`` subcommands:

.. code-block:: none

    $ set help print
    $ set print --help
    $ set print -h

You can also print out the help text for a command explicitly by calling the
following function:

.. function:: void cork_command_show_help(struct cork_command \*command, const char \*message)

    Prints out help text for *command*.  (If it's a leaf command, this is the
    full help text.  If it's a command set, it's a list of the set's
    subcommands.)  We will preface the help text with *message* if it's
    non-``NULL``.  (The message should not include a trailing newline.)


.. _cli-options:

Option parsing
==============

Leaf commands and command sets both let you provide a function that parse
command-line options for the given command.  We don't prescribe any particular
option parsing library, you just need to conform to the interface described in
this section.  (Note that the standard ``getopt`` and ``getopt_long`` functions
can easily be used in an option parsing function.)

.. type:: int (\*cork_option_parser)(int argc, char \*\*argv)

   Should parse any command-line options that can appear at this point in the
   executable's command line.  (The options must appear immediately after the
   name of the command that this function belongs to.  See below for several
   examples.)

   Your function must look for and process any options that appear at the
   beginning of *argv*.  If there are any errors processing the options, you
   should print out an error message (most likely via
   :c:func:`cork_command_show_help`) and exit the program, using the standard
   ``exit`` function, with an exit code of ``EXIT_FAILURE``.

   If there aren't any errors processing the options, you should return the
   number of *argv* elements that were consumed while processing the options.
   We will use this return value to update *argc* and *argv* beforing continuing
   with subcommand selection and argument processing.  (Note that ``getopt``'s
   ``optind`` variable is exactly what you need for the return value.)

As mentioned above, different option parsing functions are used to parse options
from a particular point in the command line.  Given the following command:

.. code-block:: none

    $ set --opt1 print --opt2 avro --opt3 --opt4=foo <filename>

The ``--opt1`` option would be parsed by the ``set`` command's parser.  The
``--opt2`` option would be parsed by the ``set print`` command's parser.  The
``--opt3`` and ``-opt4=foo`` options would be parsed by the ``set print avro``
command's parser.  And the ``<filename>`` argument would be parsed by the ``set
print avro`` command's *run* function.
