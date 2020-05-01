.. _maps:

IP maps
=======

The functions in this section allow you to work with *IP maps*, which map IP
addresses to integer values.  IP maps work seamlessly with both IPv4 and IPv6
addresses.  Every IP address is mapped to some value; each IP map has a *default
value* that will be used for any addresses that aren't explicitly mapped to
something else.  (This is the map equivalent of an address not appearing in a
set; "removing" an address from a map is the same as setting it to the map's
default value.)

.. note::

   With this definition of a map, an IP set is just an IP map that is restricted
   to the values 0 and 1.  This is actually how sets are implemented internally.

.. note::

   Before using any of the IP map functions, you must initialize this library
   using :c:func:`ipset_init_library`.


Creating and freeing maps
-------------------------

.. type:: struct ip_map

   Maps IP addresses to integer values.  The fields of this ``struct`` are
   opaque; you should only use the public library functions to access the
   contents of the map.

There are two ways that you can work with IP maps.  The first is that you can
allocate the space for the :c:type:`ip_map` instance yourself --- for instance,
directly on the stack.  The second is to let the IP map library allocate the
space for you.  Your choice determines which of the following functions you will
use to create and free your IP maps.

.. function:: void ipmap_init(struct ip_map \*map, int default_value)
              struct ip_map \*ipmap_new(int default_value)

   Creates a new IP map.  The map will use *default_value* as the default value
   for any addresses that aren't explicitly mapped to something else.

   The ``init`` variant should be used if you've allocated space for the map
   yourself.  The ``new`` variant should be used if you want the library to
   allocate the space for you.  (The ``new`` variant never returns ``NULL``; it
   will abort the program if the allocation fails.) In both cases, the map
   starts off empty.

.. function:: void ipmap_done(struct ip_map \*map)
              void ipmap_free(struct ip_map \*map)

   Finalizes an IP map.  The ``done`` variant must be used if you created the
   map using :c:func:`ipmap_init`; the ``free`` variant must be used if you
   created the map using :c:func:`ipmap_new`.


Adding and removing elements
----------------------------

We provide a variety of functions for adding and removing addresses from an IP
map.

.. function:: void ipmap_ipv4_set(struct ip_map \*map, struct cork_ipv4 \*ip, int value)
              void ipmap_ipv6_set(struct ip_map \*map, struct cork_ipv6 \*ip, int value)
              void ipmap_ip_set(struct ip_map \*map, struct cork_ip \*ip, int value)

   Updates *map* so that *ip* is mapped to *value*.

.. function:: void ipmap_ipv4_set_network(struct ip_map \*map, struct cork_ipv4 \*ip, unsigned int cidr_prefix, int value)
              void ipmap_ipv6_set_network(struct ip_map \*map, struct cork_ipv6 \*ip, unsigned int cidr_prefix, int value)
              void ipmap_ip_set_network(struct ip_map \*map, struct cork_ip \*ip, unsigned int cidr_prefix, int value)

   Updates *map* so that all of the addresses in a CIDR network are mapped to
   *value*.  *ip* is one of the addresses in the CIDR network; *cidr_prefix* is
   the number of bits in the network portion of each IP address in the CIDR
   network, as defined in `RFC 4632`_.  *cidr_prefix* must be in the range 0-32
   (inclusive) if *ip* is an IPv4 address, and in the range 0-128 (inclusive) if
   it's an IPv6 address.

.. _RFC 4632: http://tools.ietf.org/html/rfc4632

.. note::

   In all of the ``_network`` functions, if you want to strictly adhere to RFC
   4632, *ip* can only have non-zero bits in its *cidr_prefix* uppermost bits.
   All of the lower-order bits (i.e., in the host portion of the IP address)
   must be map to 0.  We do not enforce this, however.


Querying a map
--------------

.. function:: int ipmap_get_ipv4(const struct ip_map \*map, struct cork_ipv4 \*ip)
              int ipmap_get_ipv6(const struct ip_map \*map, struct cork_ipv6 \*ip)
              int ipmap_get_ip(const struct ip_map \*map, struct cork_ip \*ip)

   Returns the value that *ip* is mapped to in *map*.

.. function:: bool ipmap_is_empty(const struct ip_map \*map)

   Returns whether *map* is empty.  A map is considered empty is every IP
   address is mapped to the default value.

.. function:: bool ipmap_is_equal(const struct ip_map \*map1, const struct ip_map \*map2)

   Returns whether every IP address is mapped to the same value in *map1* and
   *map2*.

.. function:: size_t ipmap_memory_size(const struct ip_map \*map)

   Returns the number of bytes of memory needed to store *map*.  Note that
   adding together the storage needed for each map you use doesn't necessarily
   give you the total memory requirements, since some storage can be shared
   between maps.


Storing maps in files
---------------------

The functions in this section allow you to store IP maps on disk, and reload
them into another program at a later time.  You don't have to know the details
of the file format to be able to use these functions; we guarantee that maps
written with previous versions of the library will be readable by later versions
of the library (but not vice versa).  And we guarantee that the file format is
platform-independent; maps written on any machine will be readable on any other
machine.

(That said, if you do want to know the details of the file format, that's
documented in :ref:`another section <file-format>`.)

.. function:: int ipmap_save(FILE \*stream, const struct ip_map \*map)

   Saves an IP map into *stream*.  You're responsible for opening *stream*
   before calling this function, and for closing *stream* afterwards.  If there
   are any errors writing the map, we return ``-1`` and fill in a libcork
   :ref:`error condition <libcork:errors>`.

.. function:: int ipmap_save_to_stream(struct cork_stream_consumer \*stream, const struct ip_map \*map)

   Saves an IP map into a libcork :ref:`stream consumer <libcork:stream>`.  If
   there are any errors writing the map, we return ``-1`` and fill in a libcork
   :ref:`error condition <libcork:errors>`.

.. function:: struct ip_map \*ipmap_load(FILE \*stream)

   Loads an IP map from *stream*.  You're responsible for opening *stream*
   before calling this function, and for closing *stream* afterwards.  If there
   are any errors reading the map, we return ``NULL`` and fill in a libcork
   :ref:`error condition <libcork:errors>`.  You must use :c:func:`ipmap_free`
   to free the map when you're done with it.
