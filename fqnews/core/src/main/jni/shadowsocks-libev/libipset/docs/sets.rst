.. _sets:

IP sets
=======

The functions in this section allow you to work with sets of IP addresses.  IP
sets work seamlessly with both IPv4 and IPv6 addresses.

.. note::

   Before using any of the IP set functions, you must initialize this library
   using the following function.

.. function:: int ipset_init_library(void)

   Initializes the IP set library.  This function **must** be called before any
   other function in the library.  It is safe to call this function multiple
   times, and from multiple threads.


Creating and freeing sets
-------------------------

.. type:: struct ip_set

   A set of IP addresses.  The fields of this ``struct`` are opaque; you should
   only use the public library functions to access the contents of the set.

There are two ways that you can work with IP sets.  The first is that you can
allocate the space for the :c:type:`ip_set` instance yourself --- for instance,
directly on the stack.  The second is to let the IP set library allocate the
space for you.  Your choice determines which of the following functions you will
use to create and free your IP sets.

.. function:: void ipset_init(struct ip_set \*set)
              struct ip_set \*ipset_new(void)

   Creates a new IP set.  The ``init`` variant should be used if you've
   allocated space for the set yourself.  The ``new`` variant should be used if
   you want the library to allocate the space for you.  (The ``new`` variant
   never returns ``NULL``; it will abort the program if the allocation fails.)
   In both cases, the set starts off empty.

.. function:: void ipset_done(struct ip_set \*set)
              void ipset_free(struct ip_set \*set)

   Finalizes an IP set.  The ``done`` variant must be used if you created the
   set using :c:func:`ipset_init`; the ``free`` variant must be used if you
   created the set using :c:func:`ipset_new`.


Adding and removing elements
----------------------------

We provide a variety of functions for adding and removing addresses from an IP
set.

.. function:: bool ipset_ipv4_add(struct ip_set \*set, struct cork_ipv4 \*ip)
              bool ipset_ipv6_add(struct ip_set \*set, struct cork_ipv6 \*ip)
              bool ipset_ip_add(struct ip_set \*set, struct cork_ip \*ip)

   Adds a single IP address to *set*.  If the IP address was not already in the
   set, we return ``true``; otherwise we return ``false``.  (In other words, we
   return whether the set changed as a result of this operation.)

.. function:: bool ipset_ipv4_remove(struct ip_set \*set, struct cork_ipv4 \*ip)
              bool ipset_ipv6_remove(struct ip_set \*set, struct cork_ipv6 \*ip)
              bool ipset_ip_remove(struct ip_set \*set, struct cork_ip \*ip)

   Removes a single IP removeress from *set*.  If the IP address was previously
   in the set, we return ``true``; otherwise we return ``false``.  (In other
   words, we return whether the set changed as a result of this operation.)

.. function:: bool ipset_ipv4_add_network(struct ip_set \*set, struct cork_ipv4 \*ip, unsigned int cidr_prefix)
              bool ipset_ipv6_add_network(struct ip_set \*set, struct cork_ipv6 \*ip, unsigned int cidr_prefix)
              bool ipset_ip_add_network(struct ip_set \*set, struct cork_ip \*ip, unsigned int cidr_prefix)

   Adds an entire CIDR network of IP addresses to *set*.  *ip* is one of the
   addresses in the set; *cidr_prefix* is the number of bits in the network
   portion of each IP address in the CIDR network, as defined in `RFC 4632`_.
   *cidr_prefix* must be in the range 0-32 (inclusive) if *ip* is an IPv4
   address, and in the range 0-128 (inclusive) if it's an IPv6 address.

   We return whether the set changed as a result of this operation; if we return
   ``true``, than at least one of the address in the CIDR network was not
   already present in *set*.  We cannot currently distinguish whether *all* of
   the addresses were missing (and therefore added).

.. function:: bool ipset_ipv4_remove_network(struct ip_set \*set, struct cork_ipv4 \*ip, unsigned int cidr_prefix)
              bool ipset_ipv6_remove_network(struct ip_set \*set, struct cork_ipv6 \*ip, unsigned int cidr_prefix)
              bool ipset_ip_remove_network(struct ip_set \*set, struct cork_ip \*ip, unsigned int cidr_prefix)

   Removes an entire CIDR network of IP addresses from *set*.  *ip* is one of
   the addresses in the set; *cidr_prefix* is the number of bits in the network
   portion of each IP address in the CIDR network, as defined in `RFC 4632`_.
   *cidr_prefix* must be in the range 0-32 (inclusive) if *ip* is an IPv4
   address, and in the range 0-128 (inclusive) if it's an IPv6 address.

   We return whether the set changed as a result of this operation; if we return
   ``true``, than at least one of the address in the CIDR network was present in
   *set*.  We cannot currently distinguish whether *all* of the addresses were
   present (and therefore removed).

.. _RFC 4632: http://tools.ietf.org/html/rfc4632

.. note::

   In all of the ``_network`` functions, if you want to strictly adhere to RFC
   4632, *ip* can only have non-zero bits in its *cidr_prefix* uppermost bits.
   All of the lower-order bits (i.e., in the host portion of the IP address)
   must be set to 0.  We do not enforce this, however.


Querying a set
--------------

.. function:: bool ipset_contains_ipv4(const struct ip_set \*set, struct cork_ipv4 \*ip)
              bool ipset_contains_ipv6(const struct ip_set \*set, struct cork_ipv6 \*ip)
              bool ipset_contains_ip(const struct ip_set \*set, struct cork_ip \*ip)

   Returns whether *set* contains *ip*.

.. function:: bool ipset_is_empty(const struct ip_set \*set)

   Returns whether *set* is empty.

.. function:: bool ipset_is_equal(const struct ip_set \*set1, const struct ip_set \*set2)

   Returns whether *set1* and *set2* contain exactly the same addresses.

.. function:: size_t ipset_memory_size(const struct ip_set \*set)

   Returns the number of bytes of memory needed to store *set*.  Note that
   adding together the storage needed for each set you use doesn't necessarily
   give you the total memory requirements, since some storage can be shared
   between sets.


Iterating through a set
-----------------------

In addition to querying individual addresses, you can iterate through the entire
contents of an IP set.  There are two iterator functions; one that provides
every individual IP address, and one that collapses addresses into CIDR networks
as much as possible, and returns those networks.

.. note::

   You should not modify an IP set while you're actively iterating through its
   contents; if you do this, you'll get undefined behavior.


.. type:: struct ipset_iterator

   An iterator object that lets you query all of the addresses in an IP set.

   .. member:: struct cork_ip  addr

      If iterating through individual addresses, this contains the address that
      the iterator currently points at.  If iterating through CIDR networks,
      this is the representative address of the current network.

   .. member:: unsigned int  cidr_prefix

      If iterating through CIDR networks, this is the CIDR prefix of the current
      network.  If iterating through individual IP addresses, this will always
      be ``32`` or ``128``, depending on whether *addr* contains an IPv4 or IPv6
      address.

.. function:: struct ipset_iterator \*ipset_iterate(struct ip_set \*set, bool desired_value)
              struct ipset_iterator \*ipset_iterate_networks(struct ip_set \*set, bool desired_value)

   If *desired_value* is ``true``, then we return an iterator that will produce
   the IP addresses that are present in *set*.  If it's ``false``, then the
   iterator will produce the IP addresses that are *not* in *set*.

   The ``_networks`` variant will summarize the IP addresses into CIDR networks,
   to reduce the number of items that are reported by the iterator.  (This can
   be especially useful (necessary?) if your set contains any /8 or /16 IPv4
   networks, for instance; or even worse, a /64 IPv6 network.)

.. function:: void ipset_iterator_advance(struct ipset_iterator \*iterator)

   Advance *iterator* to the next IP address or network in its underlying set.

.. function:: void ipset_iterator_free(struct ipset_iterator \*iterator)

   Frees an IP set iterator.


Storing sets in files
---------------------

The functions in this section allow you to store IP sets on disk, and reload
them into another program at a later time.  You don't have to know the details
of the file format to be able to use these functions; we guarantee that sets
written with previous versions of the library will be readable by later versions
of the library (but not vice versa).  And we guarantee that the file format is
platform-independent; sets written on any machine will be readable on any other
machine.

(That said, if you do want to know the details of the file format, that's
documented in :ref:`another section <file-format>`.)

.. function:: int ipset_save(FILE \*stream, const struct ip_set \*set)

   Saves an IP set into *stream*.  You're responsible for opening *stream*
   before calling this function, and for closing *stream* afterwards.  If there
   are any errors writing the set, we return ``-1`` and fill in a libcork
   :ref:`error condition <libcork:errors>`.

.. function:: int ipset_save_to_stream(struct cork_stream_consumer \*stream, const struct ip_set \*set)

   Saves an IP set into a libcork :ref:`stream consumer <libcork:stream>`.  If
   there are any errors writing the set, we return ``-1`` and fill in a libcork
   :ref:`error condition <libcork:errors>`.

.. function:: struct ip_set \*ipset_load(FILE \*stream)

   Loads an IP set from *stream*.  You're responsible for opening *stream*
   before calling this function, and for closing *stream* afterwards.  If there
   are any errors reading the set, we return ``NULL`` and fill in a libcork
   :ref:`error condition <libcork:errors>`.  You must use :c:func:`ipset_free`
   to free the set when you're done with it.

.. function:: int ipset_save_dot(FILE \*stream, const struct ip_set \*set)

   Produces a GraphViz_ ``dot`` representation of the BDD graph used to store
   *set*, and writes this graph representation to *stream*.  You're responsible
   for opening *stream* before calling this function, and for closing *stream*
   afterwards.  If there are any errors writing the set, we return ``-1`` and
   fill in a libcork :ref:`error condition <libcork:errors>`.

   .. _GraphViz: http://www.graphviz.org/
