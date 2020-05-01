.. _unique-ids:

******************
Unique identifiers
******************

.. highlight:: c

::

  #include <libcork/core.h>


The functions in this section let you define compile-time unique identifiers.
These identifiers are simple C variables, and each one is guaranteed to be
unique within the context of a single execution of your program.  They are *not*
appropriate for use as external identifiers --- for instance, for serializing
into long-term storage or sending via a communications channel to another
process.


.. type:: cork_uid

   A unique identifier.


.. macro:: cork_uid  CORK_UID_NONE

   A unique identifier value that means "no identifier".  This is guaranteed to
   be distinct from all other unique identifiers.  It is invalid to call
   :c:func:`cork_uid_hash`, :c:func:`cork_uid_id`, or :c:func:`cork_uid_name` on
   this identifier.


.. macro:: cork_uid_define(SYMBOL id)
           cork_uid_define_named(SYMBOL id, const char \*name)

   You use the :c:func:`cork_uid_define` macro to define a new unique identifier
   with the given C identifier *id*.  The ``_define`` variant also uses *id* as
   the identifier's human-readable name; the ``_define_named`` variant let's you
   provide a separate human-readable name.  Within the context of a single
   execution of this program, this identifier is guaranteed to be distinct from
   any other identifier, regardless of which library the identifiers are defined
   in.

   In the same compilation unit, you can then use the C identifier *id* to
   retrieve the :c:type:`cork_uid` instance for this identifier.

   .. note::

      The unique identifier objects are declared ``static``, so they are only
      directly visible (using the C identifier *id*) in the same compilation
      unit as the :c:func:`cork_uid_define` call that created the identifier.
      The resulting :c:type:`cork_uid` value, however, can be passed around the
      rest of your code however you want.


.. function:: bool cork_uid_equal(const cork_uid id1, const cork_uid id2)

   Return whether two :c:type:`cork_uid` values refer to the same unique
   identifier.


.. function:: cork_hash cork_uid_hash(const cork_uid id)

   Return a :ref:`hash value <hash-values>` for the given identifier.


.. function:: const char \*cork_uid_name(const cork_uid id)

   Return the name of the given unique identifier.


Example
=======

::

    #include <stdio.h>
    #include <libcork/core.h>

    cork_uid_define(test_id);

    int
    main(void)
    {
        cork_uid  id = test_id;
        printf("Identifier %p has name %s\n", id, cork_uid_name(id));
        return 0;
    }
