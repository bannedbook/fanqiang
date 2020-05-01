.. _hash-table:

***********
Hash tables
***********

.. highlight:: c

::

  #include <libcork/ds.h>

This section defines a hash table class.  Our hash table implementation
is based on the public domain hash table package written in the late
1980's by Peter Moore at UC Berkeley.

The keys and values of a libcork hash table are both represented by ``void *``
pointers.  You can also store integer keys or values, as long as you use the
:c:type:`intptr_t` or :c:type:`uintptr_t` integral types.  (These are the only
integer types guaranteed by the C99 standard to fit within the space used by a
``void *``.)  The keys of the hash table can be any arbitrary type; you must
provide two functions that control how key pointers are used to identify entries
in the table: the *hasher* (:c:type:`cork_hash_f`) and the *comparator*
(:c:type:`cork_equals_f`).  It's your responsibility to ensure that these two
functions are consistent with each other — i.e., if two keys are equal according
to your comparator, they must also map to the same hash value.  (The inverse
doesn't need to be true; it's fine for two keys to have the same hash value but
not be equal.)

.. type:: struct cork_hash_table

   A hash table.

.. function:: struct cork_hash_table \*cork_hash_table_new(size_t initial_size, unsigned int flags)

   Creates a new hash table instance.

   If you know roughly how many entries you're going to add to the hash
   table, you can pass this in as the *initial_size* parameter.  If you
   don't know how many entries there will be, you can use ``0`` for this
   parameter instead.

   You will most likely need to provide a hashing function and a comparison
   function for the new hash table (using :c:func:`cork_hash_table_set_hash` and
   :c:func:`cork_hash_table_set_equals`), which will be used to compare key
   values of the entries in the table.  If you do not provide your own
   functions, the default functions will compare key pointers as-is without
   interpreting what they point to.

   The *flags* field is currently unused, and should be ``0``.  In the future,
   this parameter will be used to let you customize the behavior of the hash
   table.


.. function:: void cork_hash_table_free(struct cork_hash_table \*table)

   Frees a hash table.  If you have provided a :c:func:`free_key
   <cork_hash_table_set_free_key>` or :c:func:`free_value
   <cork_hash_table_set_free_value>` callback for *table*, then we'll
   automatically free any remaining keys and/or values.


.. type:: struct cork_hash_table_entry

   The contents of an entry in a hash table.

   .. member:: void  \*key

      The key for this entry.  There won't be any other entries in the
      hash table with the same key, as determined by the comparator
      function that you provide.

   .. member:: void  \*value

      The value for this entry.  The entry's value is completely opaque
      to the hash table; we'll never need to compare or interrogate the
      values in the table.

   .. member:: cork_hash  hash

      The hash value for this entry's key.  This field is strictly
      read-only.


Callback functions
------------------

You can use the callback functions in this section to customize the behavior of
a hash table.

.. function:: void cork_hash_table_set_user_data(struct cork_hash_table \*table, void \*user_data, cork_free_f free_user_data)

   Lets you provide an opaque *user_data* pointer to each of the hash table's
   callbacks.  This lets you provide additional state, other than the hash table
   itself to those callbacks.  If *free_user_data* is not ``NULL``, then the
   hash table will take control of *user_data*, and will use the
   *free_user_data* function to free it when the hash table is destroyed.


Key management
~~~~~~~~~~~~~~

.. function:: void cork_hash_table_set_hash(struct cork_hash_table \*table, void \*user_data, cork_hash_f hash)

   The hash table will use the ``hash`` callback to calculate a hash value for
   each key.

   .. type:: cork_hash (\*cork_hash_f)(void \*user_data, const void \*key)

      .. note::

         It's important to use a hash function that has a uniform distribution
         of hash values for the set of values you expect to use as hash table
         keys.  In particular, you *should not* rely on there being a prime
         number of hash table bins to get the desired uniform distribution.  The
         :ref:`hash value functions <hash-values>` that we provide have uniform
         distribution (and are fast), and should be safe to use for most key
         types.

.. function:: void cork_hash_table_set_equals(struct cork_hash_table \*table, void \*user_data, cork_equals_f equals)

   The hash table will use the ``equals`` callback to compare keys.

   .. type:: bool (\*cork_equals_f)(void \*user_data, const void \*key1, const void \*key2)


Built-in key types
~~~~~~~~~~~~~~~~~~

We also provide a couple of specialized constructors for common key types, which
prevents you from having to duplicate common hashing and comparison functions.

.. function:: struct cork_hash_table \*cork_string_hash_table_new(size_t initial_size, unsigned int flags)

   Create a hash table whose keys will be C strings.

.. function:: struct cork_hash_table \*cork_pointer_hash_table_new(size_t initial_size, unsigned int flags)

   Create a hash table where keys should be compared using standard pointer
   equality.  (In other words, keys should only be considered equal if they
   point to the same physical object.)


Automatically freeing entries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. function:: void cork_hash_table_set_free_key(struct cork_hash_table \*table, void \*user_data, cork_free_f free_key)
              void cork_hash_table_set_free_value(struct cork_hash_table \*table, void \*user_data, cork_free_f free_value)

   If you provide ``free_key`` and/or ``free_value`` callbacks, then the hash
   table will take ownership of any keys and values that you add.  The hash
   table will use these callbacks to free each key and value when entries are
   explicitly deleted (via :c:func:`cork_hash_table_delete` or
   :c:func:`cork_hash_table_clear`), and when the hash table itself is
   destroyed.


Adding and retrieving entries
-----------------------------

There are several functions that can be used to add or retrieve entries
from a hash table.  Each one has slightly different semantics; you
should read through them all before deciding which one to use for a
particular use case.

.. note::

   Each of these functions comes in two variants.  The “normal” variant will use
   the hash table's :c:func:`hash <cork_hash_table_set_hash>` callback to
   calculate the hash value for the *key* parameter.  This is the normal way to
   interact with a hash table.

   When using the ``_hash`` variant, you must calculate the hash value for each
   key yourself, and pass in this hash value as an extra parameter.  The hash
   table's :c:func:`hash <cork_hash_table_set_hash>` callback is not invoked.
   This can be more efficient, if you've already calculated or cached the hash
   value.  It is your responsibility to make sure that the hash values you
   provide are consistent, just like when you write a :c:func:`hash
   <cork_hash_table_set_hash>` callback.

.. function:: void \*cork_hash_table_get(const struct cork_hash_table \*table, const void \*key)
              void \*cork_hash_table_get_hash(const struct cork_hash_table \*table, cork_hash hash, const void \*key)

   Retrieves the value in *table* with the given *key*.  We return
   ``NULL`` if there's no corresponding entry in the table.  This means
   that, using this function, you can't tell the difference between a
   missing entry, and an entry that's explicitly mapped to ``NULL``.  If
   you need to distinguish those cases, you should use
   :c:func:`cork_hash_table_get_entry()` instead.

.. function:: struct cork_hash_table_entry \*cork_hash_table_get_entry(const struct cork_hash_table \*table, const void \*key)
              struct cork_hash_table_entry \*cork_hash_table_get_entry_hash(const struct cork_hash_table \*table, cork_hash hash, const void \*key)

   Retrieves the entry in *table* with the given *key*.  We return
   ``NULL`` if there's no corresponding entry in the table.

   You are free to update the :c:member:`key
   <cork_hash_table_entry.key>` and :c:member:`value
   <cork_hash_table_entry.value>` fields of the entry.  However, you
   must ensure that any new key is considered “equal” to the old key,
   according to the hasher and comparator functions that you provided
   for this hash table.

.. function:: struct cork_hash_table_entry \*cork_hash_table_get_or_create(struct cork_hash_table \*table, void \*key, bool \*is_new)
              struct cork_hash_table_entry \*cork_hash_table_get_or_create_hash(struct cork_hash_table \*table, cork_hash hash, void \*key, bool \*is_new)

   Retrieves the entry in *table* with the given *key*.  If there is no
   entry with the given key, it will be created.  (If we can't create
   the new entry, we'll return ``NULL``.)  We'll fill in the *is_new*
   output parameter to indicate whether the entry is new or not.

   If a new entry is created, its value will initially be ``NULL``, but
   you can update this as necessary.  You can also update the entry's
   key, though you must ensure that any new key is considered “equal” to
   the old key, according to the hasher and comparator functions that
   you provided for this hash table.  This is necessary, for instance,
   if the *key* parameter that we search for was allocated on the stack.
   We can't save this stack key into the hash table, since it will
   disapppear as soon as the calling function finishes.  Instead, you
   must create a new key on the heap, which can be saved into the entry.
   For efficiency, you'll only want to allocate this new heap-stored key
   if the entry is actually new, especially if there will be a lot
   successful lookups of existing keys.

.. function:: int cork_hash_table_put(struct cork_hash_table \*table, void \*key, void \*value, bool \*is_new, void \*\*old_key, void \*\*old_value)
              int cork_hash_table_put_hash(struct cork_hash_table \*table, cork_hash hash, void \*key, void \*value, bool \*is_new, void \*\*old_key, void \*\*old_value)

   Add an entry to a hash table.  If there is already an entry with the
   given key, we will overwrite its key and value with the *key* and
   *value* parameters.  If the *is_new* parameter is non-\ ``NULL``,
   we'll fill it in to indicate whether the entry is new or already
   existed in the table.  If the *old_key* and/or *old_value* parameters
   are non-\ ``NULL``, we'll fill them in with the existing key and
   value.  This can be used, for instance, to finalize an overwritten
   key or value object.

.. function:: void cork_hash_table_delete_entry(struct cork_hash_table \*table, struct cork_hash_table_entry \*entry)

   Removes *entry* from *table*.  You must ensure that *entry* refers to a
   valid, existing entry in the hash table.  This function can be more efficient
   than :c:func:`cork_hash_table_delete` if you've recently retrieved a hash
   table entry using :c:func:`cork_hash_table_get_or_create` or
   :c:func:`cork_hash_table_get_entry`, since we won't have to search for the
   entry again.

.. function:: bool cork_hash_table_delete(struct cork_hash_table \*table, const void \*key, void \*\*deleted_key, void \*\*deleted_value)
              bool cork_hash_table_delete_hash(struct cork_hash_table \*table, cork_hash hash, const void \*key, void \*\*deleted_key, void \*\*deleted_value)

   Removes the entry with the given *key* from *table*.  If there isn't
   any entry with the given key, we'll return ``false``.  If the
   *deleted_key* and/or *deleted_value* parameters are non-\ ``NULL``,
   we'll fill them in with the deleted key and value.  This can be used,
   for instance, to finalize the key or value object that was stored in
   the hash table entry.

   If you have provided a :c:func:`free_key <cork_hash_table_set_free_key>` or
   :c:func:`free_value <cork_hash_table_set_free_value>` callback for *table*,
   then we'll automatically free the key and/or value of the deleted entry.
   (This happens before ``cork_hash_table_delete`` returns, so you must not
   provide a *deleted_key* and/or *deleted_value* in this case.)


Other operations
----------------

.. function:: size_t cork_hash_table_size(const struct cork_hash_table \*table)

   Returns the number of entries in a hash table.

.. function:: void cork_hash_table_clear(struct cork_hash_table \*table)

   Removes all of the entries in a hash table, without finalizing the
   hash table itself.

   If you have provided a :c:func:`free_key <cork_hash_table_set_free_key>` or
   :c:func:`free_value <cork_hash_table_set_free_value>` callback for *table*,
   then we'll automatically free any remaining keys and/or values.

.. function:: int cork_hash_table_ensure_size(struct cork_hash_table \*table, size_t desired_count)

   Ensures that *table* has enough space to efficiently store a certain
   number of entries.  This can be used to reduce (or eliminate) the
   number of resizing operations needed to add a large number of entries
   to the table, when you know in advance roughly how many entries there
   will be.


Iterating through a hash table
------------------------------

There are two strategies you can use to access all of the entries in a
hash table: *mapping* and *iterating*.


Iteration order
~~~~~~~~~~~~~~~

Regardless of whether you use the mapping or iteration functions, we guarantee
that the collection of items will be processed in the same order that they were
added to the hash table.


Mapping
~~~~~~~

With mapping, you write a mapping function that will be applied to each entry in
the table.  (In this case, libcork controls the loop that steps through each
entry.)

.. function:: void cork_hash_table_map(struct cork_hash_table \*table, void \*user_data, cork_hash_table_map_f map)

   Applies the *map* function to each entry in a hash table.  The *map*
   function's :c:type:`cork_hash_table_map_result` return value can be used to
   influence the iteration.

   .. type:: enum cork_hash_table_map_result (\*cork_hash_table_map_f)(void \*user_data, struct cork_hash_table_entry \*entry)

      The function that will be applied to each entry in a hash table.  The
      function's return value can be used to influence the iteration:

      .. type:: enum cork_hash_table_map_result

         .. var:: CORK_HASH_TABLE_CONTINUE

            Continue the current :c:func:`cork_hash_table_map()` operation.  If
            there are any remaining elements, the next one will be passed into
            another call of the *map* function.

         .. var:: CORK_HASH_TABLE_ABORT

            Stop the current :c:func:`cork_hash_table_map()` operation.  No more
            entries will be processed after this one, even if there are
            remaining elements in the hash table.

         .. var:: CORK_HASH_TABLE_DELETE

            Continue the current :c:func:`cork_hash_table_map()` operation, but
            first delete the entry that was just processed.  If there are any
            remaining elements, the next one will be passed into another call of
            the *map* function.

For instance, you can manually calculate the number of entries in a hash
table as follows (assuming you didn't want to use the built-in
:c:func:`cork_hash_table_size()` function, of course)::

  static enum cork_hash_table_map_result
  count_entries(void *user_data, struct cork_hash_table_entry *entry)
  {
      size_t  *count = user_data;
      (*count)++;
      return CORK_HASH_TABLE_MAP_CONTINUE;
  }

  struct cork_hash_table  *table = /* from somewhere */;
  size_t  count = 0;
  cork_hash_table_map(table, &count, count_entries);
  /* the number of entries is now in count */


Iterating
~~~~~~~~~

The second strategy is to iterate through the entries yourself.  Since
the internal struture of the :c:type:`cork_hash_table` type is opaque
(and slightly more complex than a simple array), you have to use a
special “iterator” type to manage the manual iteration.  Note that
unlike when using a mapping function, it is **not** safe to delete
entries in a hash table as you manually iterate through them.

.. type:: struct cork_hash_table_iterator

   A helper type for manually iterating through the entries in a hash
   table.  All of the fields in this type are private.  You'll usually
   allocate this type on the stack.

.. function:: void cork_hash_table_iterator_init(struct cork_hash_table \*table, struct cork_hash_table_iterator \*iterator)

   Initializes a new iterator for the given hash table.

.. function:: struct cork_hash_table_entry \*cork_hash_table_iterator_next(struct cork_hash_table_iterator \*iterator)

   Returns the next entry in *iterator*\ 's hash table.  If you've
   already iterated through all of the entries in the table, we'll
   return ``NULL``.

With these functions, manually counting the hash table entries looks
like::

  struct cork_hash_table  *table = /* from somewhere */;
  struct cork_hash_table_iterator  iter;
  struct cork_hash_table_entry  *entry;
  size_t  count = 0;

  cork_hash_table_iterator_init(table, &iter);
  while ((entry = cork_hash_table_iterator_next(&iter)) != NULL) {
      count++;
  }
  /* the number of elements is now in count */
