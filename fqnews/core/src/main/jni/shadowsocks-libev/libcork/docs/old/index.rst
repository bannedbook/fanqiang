.. _index:

|project_name| documentation
============================

This is the documentation for |project_name| |release|, last updated
|today|.


Introduction
------------

So what is libcork, exactly?  It's a “simple, easily embeddable, cross-platform
C library”.  It falls roughly into the same category as glib_ or APR_ in the C
world; the STL, POCO_, or QtCore_ in the C++ world; or the standard libraries
of any decent dynamic language.

So if libcork has all of these comparables, why a new library?  Well, none of
the C++ options are really applicable here.  And none of the C options work,
because one of the main goals is to have the library be highly modular, and
useful in resource-constrained systems.  Once we describe some of the design
decisions that we've made in libcork, you'll hopefully see how this fits into
an interesting niche of its own.

.. _glib: http://library.gnome.org/devel/glib/
.. _APR: http://apr.apache.org/
.. _POCO: http://pocoproject.org/
.. _QtCore: http://qt.nokia.com/


Contents
--------

.. toctree::
   :maxdepth: 2

   config
   versions
   visibility
   basic-types
   byte-order
   attributes
   allocation
   errors
   gc
   mempool
   ds
   cli
   files
   process
   subprocess
   threads


Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`
