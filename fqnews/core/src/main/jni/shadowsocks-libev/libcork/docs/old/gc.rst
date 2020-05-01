.. _gc:

************************************
Reference-counted garbage collection
************************************

.. highlight:: c

::

  #include <libcork/core.h>

The functions in this section implement a reference counting garbage
collector.  The garbage collector handles reference cycles correctly.
It is **not** a conservative garbage collector — i.e., we don't assume
that every word inside an object might be a pointer.  Instead, each
garbage-collected object must provide a *recursion function* that knows
how to delve down into any child objects that it references.

The garbage collector is **not** thread-safe.  If your application is
multi-threaded, each thread will (automatically) have its own garbage
collection context.  There are two strategies that you can use when
using the garbage collector in a multi-threaded application:

* Have a single “master” thread be responsible for the lifecycle of
  every object.  This thread is the only one allowed to interact with
  the garbage collector.  **No** other threads are allowed to call any
  of the functions in this section, including the
  :c:func:`cork_gc_incref()` and :c:func:`cork_gc_decref()` functions.
  Other threads are allowed to access the objects that are managed by
  the garbage collector, but the master thread must ensure that all
  objects are live whenever another thread attempts to use them.  This
  will require some kind of thread-safe communication or synchronization
  between the master thread and the worker threads.

* Have a separate garbage collector per thread.  Each object is “owned”
  by a single thread, and the object is managed by that thread's garbage
  collector.  As with the first strategy, other threads can use any
  object, as long as the object's owner thread is able to guarantee that
  the object will be live for as long as it's needed.  (Eventually we'll
  also support migrating an object from one garbage collector to
  another, but that feature isn't currently implemented.)

The garbage collection implementation is based on the algorithm
described in §3 of [1]_.

.. [1] Bacon, DF and Rajan VT.  *Concurrent cycle collection in
   reference counted systems*.  Proc. ECOOP 2001.  LNCS 2072.


Creating a garbage collector
============================

.. function:: void cork_gc_init(void)

   Initalizes a garbage collection context for the current thread.
   Usually, you can allocate this on the stack of your ``main``
   function::

     int
     main(int argc, char ** argv)
     {
         cork_gc_init();

         // use the GC context

         // and free it when you're done
         cork_gc_done();
     }

   It's not required that you call this function at all; if you don't,
   we'll automatically initialize a garbage collection context for the
   current thread the first time you try to allocate a garbage-collected
   object.  You can call this function, though, if you want to have more
   control over when the initialization occurs.

.. function:: void cork_gc_done(void)

   Finalize the garbage collection context for the current thread.  All
   objects created in this thread will be freed when this function
   returns.

   You must call this function in each thread that allocates
   garbage-collected objects, just before that thread finishes.  (If
   your application is single-threaded, then you must call this function
   before the ``main`` function finishes.)  If you don't, you'll almost
   certainly get memory leaks.


Managing garbage-collected objects
==================================

A garbage collection context can't be used to manage arbitrary objects,
since each garbage-collected class must define some callback functions
for interacting with the garbage collector.  (The :ref:`next section
<new-gc-class>` contains more details.)

Each garbage-collected class will provide its own constructor function
for instantiating a new instance of that class.  There aren't any
explicit destructors for garbage-collected objects; instead you manage
“references” to the objects.  Each pointer to a garbage-collected object
is a reference, and each object maintains a count of the references to
itself.  A newly constructed object starts with a reference count of
``1``.  Whenever you save a pointer to a garbage-collected object, you
should increase the object's reference count.  When you're done with the
pointer, you decrease its reference count.  When the reference count
drops to ``0``, the garbage collector frees the object.

.. function:: void \*cork_gc_incref(void \*obj)

   Increments the reference count of an object *obj* that is managed by
   the current thread's garbage collector.  We always return *obj* as a
   result, which allows you to use the following idiom::

     struct my_obj * my_copy_of_obj = cork_gc_incref(obj);

.. function:: void cork_gc_decref(void \*obj)

   Decrements the reference count of an object *obj* that is managed by
   the current thread's garbage collector  If the reference count drops
   to ``0``, then the garbage collector will free the object.

   .. note::

      It's safe to call this function with a ``NULL`` *obj* pointer; in
      this case, the function acts as a no-op.

.. _borrow-ref:

Borrowing a reference
---------------------

While the strategy mentioned above implies that you should call
:c:func:`cork_gc_incref()` and :c:func:`cork_gc_decref()` for *every*
pointer to a garbage-collected object, you can sometimes get away
without bumping the reference count.  In particular, you can often
*borrow* an existing reference to an object, if you can guarantee that
the borrowed reference will exist for as long as you need access to the
object.  The most common example of this when you pass in a
garbage-collected object as the parameter to a function::

  int
  use_new_reference(struct my_obj *obj)
  {
      /* Here we're being pedantically correct, and incrementing obj's
       * reference count since we've got our own pointer to the object. */
      cork_gc_incref(obj);

      /* Do something useful with obj */

      /* And now that we're done with it, decrement the reference count. */
      cork_gc_decref(obj);
  }

  int
  borrowed_reference(struct my_obj *obj)
  {
      /* We can assume that the caller has a valid reference to obj, so
       * we're just going to borrow that reference. */

      /* Do something useful with obj */
  }

In this example, ``borrowed_reference`` doesn't need to update *obj*\ 's
reference count.  We assume that the caller has a valid reference to
*obj* when it makes the call to ``borrowed_reference``.  Moreover, we
know that the caller can't possibly release this reference (via
:c:func:`cork_gc_decref()`) until ``borrowed_reference`` returns.  Since
we can guarantee that the caller's reference to *obj* will exist for the
entire duration of ``borrowed_reference``, we don't need to protect it
with an ``incref``/``decref`` pair.

.. _steal-ref:

Stealing a reference
--------------------

Another common pattern is for a “parent” object to maintain a reference
to a “child” object.  (For example, a container class might maintain
references to all of the elements in the container, assuming that the
container and elements are all garbage-collected objects.)  When you
have a network of objects like this, the parent object's constructor
will usually take in a pointer to the child object as a parameter.  If
we strictly follow the basic referencing counting rules described above,
you'll end up with something like::

  struct child  *child = child_new();
  struct parent  *parent = parent_new(child);
  cork_gc_decref(child);

The ``child_new`` constructor gives us a reference to *child*.  The
``parent_new`` constructor then creates a new reference to *child*,
which will be stored somewhere in *parent*.  We no longer need our own
reference to *child*, so we immediately decrement its reference count.

This is a common enough occurrence that many constructor functions will
instead *steal* the reference passed in as a parameter.  This means that
the constructor takes control of the caller's reference.  This allows us
to rewrite the example as::

  struct parent  *parent = parent_new_stealing(child_new());

For functions that steal a reference, the caller **cannot** assume that
the object pointed to by the stolen reference exists when the function
returns.  (If there's an error in ``parent_new_stealing``, for instance,
it must release the stolen reference to *child* to prevent a memory
leak.)  If a function is going to steal a reference, but you also need
to use the object after the function call returns, then you need to
explicitly increment the reference count *before* calling the function::

  struct child  *child = child_new();
  struct parent  *parent = parent_new_stealing(cork_gc_incref(child));
  /* Do something with child. */
  /* And then release our reference when we're done. */
  cork_gc_decref(child);

.. note::

   It's important to point out that not every constructor will steal the
   references passed in as parameters.  Moreover, there are some
   constructors that steal references for some parameters but not for
   others.  It entirely depends on what the “normal” use case is for the
   constructor.  If you're almost always going to pass in a child object
   that was just created, and that will always be accessed via the
   parent, then the constructor will usually steal the reference.  If
   the child can be referenced by many parents, then the constructor
   will usually *not* steal the reference.  The documentation for each
   constructor function will explicitly state which references are
   stolen and which objects it creates new references for.


.. _new-gc-class:

Writing a new garbage-collected class
=====================================

When you are creating a new class that you want to be managed by a
garbage collector, there are two basic steps you need to follow:

* Implement a set of callback functions that allow the garbage collector
  to interact with objects of the new class.

* Use the garbage collector's allocation functions to allocate storage
  for instance of your class.

You won't need to write a public destructor function, since objects of
the new class will be destroyed automatically when the garbage collector
determines that they're no longer needed.

Garbage collector callback interface
------------------------------------

Each garbage-collected class must provide an implementation of the
“callback interface”:

.. type:: struct cork_gc_obj_iface

   .. member:: void (\*free)(void \*obj)

      This callback is called when a garbage-collected object is about
      to be freed.  You can perform any special cleanup steps in this
      callback.  You do **not** need to deallocate the object's storage,
      and you do **not** need to release any references that you old to
      other objects.  Both of these steps will be taken care of for you
      by the garbage collector.

      If your class doesn't need any additional finalization steps, this
      entry in the callback interface can be ``NULL``.

   .. member:: void (\*recurse)(struct cork_gc \*gc, void \*obj, cork_gc_recurser recurse, void \*ud)

      This callback is how you inform the garbage collector of your
      references to other garbage-collected objects.

      The garbage collector will call this function whenever it needs to
      traverse through a graph of object references.  Your
      implementation of this callback should just call *recurse* with
      each garbage-collected object that you hold a reference to.  You
      must pass in *gc* as the first parameter to each call to
      *recurse*, and *ud* as the third parameter.

      Note that it's fine to call *recurse* with a ``NULL`` object
      pointer, which makes it slightly easier to write implementations
      of this callback.

      If instances of your class can never contain references to other
      garbage-collected objects, this entry in the callback interface
      can be ``NULL``.

.. type:: void (\*cork_gc_recurser)(struct cork_gc \*gc, void \*obj, void \*ud)

   An opaque callback provided by the garbage collector when it calls an
   object's :c:member:`~cork_gc_obj_iface.recurse` method.

.. type:: struct cork_gc

   An opaque type representing the current thread's garbage-collection
   context.  You'll only need to use this type when implementing a
   :c:member:`~cork_gc_obj_iface.recurse` function.


.. _gc-macros:

Helper macros
~~~~~~~~~~~~~

There are several macros declared in :file:`libcork/helpers/gc.h` that
make it easier to define the garbage-collection interface for a new
class.

.. note::

   Unlike most libcork modules, these macros are **not** automatically
   defined when you include the ``libcork/core.h`` header file, since
   they don't include a ``cork_`` prefix.  Because of this, we don't
   want to pollute your namespace unless you ask for the macros.  To do
   so, you must explicitly include their header file::

     #include <libcork/helpers/gc.h>

.. macro:: _free_(SYMBOL name)
           _recurse_(SYMBOL name)

   These macros declare the *free* and *recurse* methods for a new
   class.  The functions will be declared with exactly the signatures
   and parameter names shown in the entries for the
   :c:member:`~cork_gc_obj_iface.free` and
   :c:member:`~cork_gc_obj_iface.recurse` methods.

   You will almost certainly not need to refer to the method
   implementations directly, since you can use the :c:macro:`_gc_*_
   <_gc_>` macros below to declare the interface struct.  But if you do,
   they'll be called :samp:`{[name]}__free` and
   :samp:`{[name]}__recurse`.  (Note the double underscore.)

.. macro:: _gc_(SYMBOL name)
           _gc_no_free_(SYMBOL name)
           _gc_no_recurse_(SYMBOL name)
           _gc_leaf_(SYMBOL name)

   Define the garbage-collection interface struct for a new class.  If
   you defined both ``free`` and ``recurse`` methods, you should use the
   ``_gc_`` variant.  If you only defined one of the methods, you should
   use ``_gc_no_free_`` or ``_gc_no_recurse_``.  If you didn't define
   either method, you should use ``_gc_free_``.

   Like the method definitions, you probably won't need to refer to the
   interface struct directly, since you can use the
   :c:func:`cork_gc_new` macro to allocate new instances of the new
   class.  But if you do, it will be called :samp:`{[name]}__gc`.  (Note
   the double underscore.)


As an example, we can use these macros to define a new tree class::

    #include <libcork/helpers/gc.h>

    struct tree {
        const char  *name;
        struct tree  *left;
        struct tree  *right;
    };

    _free_(tree) {
        struct tree  *self = obj;
        cork_strfree(self->name);
    }

    _recurse_(tree) {
        struct tree  *self = obj;
        recurse(self->left, ud);
        recurse(self->right, ud);
    }

    _gc_(tree);


Allocating new garbage-collected objects
----------------------------------------

In your garbage-collected class's constructor, you must use one of the
following functions to allocate the object's storage.  (The garbage
collector hides some additional state in the object's memory region, so
you can't allocate the storage using ``malloc`` or :c:func:`cork_new()`
directly.)

.. function:: void \*cork_gc_alloc(size_t instance_size, struct cork_gc_obj_iface \*iface)

   Allocates a new garbage-collected object that is *instance_size*
   bytes large.  *iface* should be a pointer to a callback interface for
   the object.  If there are any problems allocating the new instance,
   the program will abort.

.. function:: type \*cork_gc_new_iface(TYPE type, struct cork_gc_obj_iface \*iface)

   Allocates a new garbage-collected instance of *type*.  The size of
   the memory region to allocate is calculated using the ``sizeof``
   operator, and the result will be automatically cast to ``type *``.
   *iface* should be a pointer to a callback interface for the object.
   If there are any problems allocating the new instance, the program
   will abort.

.. function:: struct name \*cork_gc_new(SYMBOL name)

   Allocates a new garbage-collected instance of :samp:`struct
   {[name]}`.  (Note that you don't pass in the ``struct`` part of the
   type name.) We assume that the garbage collection interface was
   created using one of the :c:macro:`_gc_*_ <_gc_>` macros, using the
   same *name* parameter.

Using these functions, could instantiate our example tree class as
follows::

    struct tree *
    tree_new(const char *name)
    {
        struct tree  *self = cork_gc_new(tree);
        self->name = cork_strdup(name);
        self->left = NULL;
        self->right = NULL;
        return self;
    }
