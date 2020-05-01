.. _mempool:

************
Memory pools
************

.. highlight:: c

::

  #include <libcork/core.h>

The functions in this section let you define *memory pools*, which allow
you to reduce the overhead of allocating and freeing large numbers of
small objects.  Instead of generating a ``malloc`` call for each
individual object, the memory pool allocates a large *block* of memory,
and then subdivides this block of memory into objects of the desired
size.  The free objects in the memory pool are linked together in a
singly-linked list, which means that allocation and deallocation is
usually a (very small) constant-time operation.

.. note::

   Memory pools are *not* thread safe; if you have multiple threads
   allocating objects of the same type, they'll need separate memory
   threads.


Basic interface
---------------

.. type:: struct cork_mempool

   A memory pool.  All of the objects created by the memory pool will be
   the same size; this size is provided when you initialize the memory
   pool.

.. function:: struct cork_mempool \*cork_mempool_new_size(size_t element_size)
              struct cork_mempool \*cork_mempool_new(TYPE type)

   Allocate a new memory pool.  The size of the objects allocated by
   the memory pool is given either as an explicit *element_size*, or by
   giving the *type* of the objects.  The blocks allocated by the memory
   pool will be of a default size (currently 4Kb).

.. function:: struct cork_mempool \*cork_mempool_new_size_ex(size_t element_size, size_t block_size)
              struct cork_mempool \*cork_mempool_new_ex(TYPE type, size_t block_size)

   Allocate a new memory pool.  The size of the objects allocated by
   the memory pool is given either as an explicit *element_size*, or by
   giving the *type* of the objects.  The blocks allocated by the memory
   pool will be *block_size* bytes large.

.. function:: void cork_mempool_free(struct cork_mempool \*mp)

   Free a memory pool.  You **must** have already freed all of the
   objects allocated by the pool; if you haven't, then this function
   will cause the current process to abort.

.. function:: void \*cork_mempool_new_object(struct cork_mempool \*mp)

   Allocate a new object from the memory pool.

.. function:: void cork_mempool_free_object(struct cork_mempool \*mp, void \*ptr)

   Free an object that was allocated from the memory pool.



.. _mempool-lifecycle:

Initializing and finalizing objects
-----------------------------------

When you free an object that was allocated via a memory pool, the memory
for that object isn't actually freed immediately.  (That's kind of the
reason that you're using a memory pool in the first place.)  This means
that if your object contains any fields that are expensive to initialize
and finalize, it can make sense to postpone the finalization of those
fields until the memory for the object itself is actually freed.

As an example, let's say you have the following type that you're going
to allocate via a memory pool::

    struct my_data {
        struct cork_buffer  scratch_space;
        int  age;
    };

Our first attempt at a constructor and destructor would then be::

    static cork_mempool  *pool;
    pool = cork_mempool_new(struct my_data);

    struct my_data *
    my_data_new(void)
    {
        struct my_data  *self = cork_mempool_new_object(pool);
        if (self == NULL) {
            return NULL;
        }

        cork_buffer_init(&self->scratch_space);
        return self;
    }

    void
    my_data_free(struct my_data *self)
    {
        cork_buffer_done(&self->scratch_space);
        cork_mempool_free_object(pool, self);
    }

What's interesting about this example is that the ``scratch_space``
field, being a :c:type:`cork_buffer`, allocates some space internally to
hold whatever data we're building up in the buffer.  When we call
:c:func:`cork_buffer_done` in our destructor, that memory is returned to
the system.  Later on, when we allocate a new ``my_data``, the
:c:func:`cork_mempool_new_object` call in our constructor might get this same
physical instance back.  We'll then proceed to re-initialize the
``scratch_space`` buffer, which will then reallocate its internal buffer
space as we use the type.

Since we're using a memory pool to reuse the memory for the ``my_data``
instance, we might as well try to reuse the memory for the
``scratch_space`` field, as well.  To do this, you provide initialization and
finalization callbacks:

.. function:: void cork_mempool_set_user_data(struct cork_mempool \*mp, void \*user_data, cork_free_f free_user_data)
              void cork_mempool_set_init_object(struct cork_mempool \*mp, cork_init_f init_object)
              void cork_mempool_set_done_object(struct cork_mempool \*mp, cork_done_f done_object)

   Provide callback functions that will be used to initialize and finalize each
   object created by the memory pool.

So, instead of putting the initialization logic into our constructor, we
put it into the ``init_object`` function.  Similarly, the finalization
logic goes into ``done_object``, and not our destructor::

    static void
    my_data_init(void *user_data, void *vself)
    {
        struct my_data  *self = vself;
        cork_buffer_init(&self->scratch_space);
        return 0;
    }

    static void
    my_data_done(void *user_data, void *vself)
    {
        struct my_data  *self = vself;
        cork_buffer_done(&self->scratch_space);
    }

    static cork_mempool  *pool;
    pool = cork_mempool_new(pool, struct my_data);
    cork_mempool_set_init_object(pool, my_data_init);
    cork_mempool_set_done_object(pool, my_data_done);

    struct my_data *
    my_data_new(void)
    {
        return cork_mempool_new_object(pool);
    }

    void
    my_data_free(struct my_data *self)
    {
        cork_mempool_free_object(pool, self);
    }

In this implementation, the ``scratch_space`` buffer is initialized when
the memory for an instance is first allocated, and it's not finalized
until the memory for the instance is returned to the system.  (Which
basically means "when the memory pool itself is freed".)

A caveat with this approach is that we've no longer guaranteed that the
``scratch_space`` buffer is empty when ``my_data_new`` returns — if
we're reusing an existing object, then the contents of the "previous"
object's buffer will still be there.  We can either make sure that
consumers of ``my_data`` don't assume anything about the contents of
``scratch_space``, or better yet, we can *reset* the fields in our
constructor object::

    struct my_data *
    my_data_new(void)
    {
        struct my_data  *self = cork_mempool_new_object(pool);
        cork_buffer_clear(&self->scratch_space);
        return self;
    }

In this example, we can reset the buffer just by clearing it.  If
resetting is more involved, it can sometimes be better to leave the
instance in a "messy" state, and have your clients not make assumptions.
But if you do this, make sure to be clear about it in your
documentation.
