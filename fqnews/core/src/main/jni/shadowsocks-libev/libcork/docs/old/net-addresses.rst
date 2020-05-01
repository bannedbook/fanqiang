.. _net-addresses:

*****************
Network addresses
*****************

.. highlight:: c

::

  #include <libcork/core.h>


IP addresses
------------

libcork provides C types for storing IPv4 and IPv6 addresses, as well as
a union type for storing a generic IP address, regardless of whether
it's IPv4 or IPv6.  (This lets you distinguish between an IPv4 address
and the equivalent ``::ffff:0:0/96`` IPv4-mapped IPv6 address.)

.. type:: struct cork_ipv4
          struct cork_ipv6

   An IPv4 or IPv6 address.  The address is stored in memory exactly as
   it would be if sent over a network connection — i.e., in
   network-endian order, regardless of the endianness of the current
   host.  The types are also guaranteed to be exactly the size of an
   actual IPv4 or IPv6 address (without additional padding), so they can
   be embedded directly into ``struct`` types that represent binary
   disk/wire formats.

   The contents of these types should be considered opaque.  You should
   use the accessor functions defined below to interact with the IP
   address.

.. type:: struct cork_ip

   A single union type that can contain either an IPv4 or IPv6 address.
   This type contains a discriminator field, so you can't use it
   directly in a binary disk/wire format type.

   .. member:: unsigned int  version

      Either ``4`` or ``6``, indicating whether the current IP address
      is an IPv4 address or an IPv6 address.

   .. member:: struct cork_ipv4  ip.v4
               struct cork_ipv6  ip.v6

      Gives you access to the underlying :c:type:`cork_ipv4` or
      :c:type:`cork_ipv6` instance for the current address.  It's your
      responsibility to check the :c:member:`cork_ip.version` field and
      only access the union branch that corresponds to the current IP
      version.


.. function:: void cork_ipv4_copy(struct cork_ipv4 \*addr, const void \*src)
              void cork_ipv6_copy(struct cork_ipv6 \*addr, const void \*src)
              void cork_ip_from_ipv4(struct cork_ip \*addr, const void \*src)
              void cork_ip_from_ipv6(struct cork_ip \*addr, const void \*src)

   Initializes a :c:type:`cork_ipv4`, :c:type:`cork_ipv6`, or
   :c:type:`cork_ip` instance from an existing IP address somewhere in
   memory.  The existing address doesn't have to be an instance of the
   :c:type:`cork_ipv4` or :c:type:`cork_ipv6` types, but it does have to
   be a well-formed address.  (For IPv4, it must be 4 bytes long; for
   IPv6, 16 bytes long.  And in both cases, the address must already be
   in network-endian order, regardless of the host's endianness.)


.. function:: int cork_ipv4_init(struct cork_ipv4 \*addr, const char \*str)
              int cork_ipv6_init(struct cork_ipv6 \*addr, const char \*str)
              int cork_ip_init(struct cork_ip \*addr, const char \*str)

   Initializes a :c:type:`cork_ipv4`, :c:type:`cork_ipv6`, or
   :c:type:`cork_ip` instance from the string representation of an IP
   address.  *str* must point to a string containing a well-formed IP
   address.  (Dotted-quad for an IPv4, and colon-hex for IPv6.)
   Moreover, the version of the IP address in *str* must be compatible
   with the function that you call: it can't be an IPv6 address if you
   call ``cork_ipv4_init``, and it can't be an IPv4 address if you call
   ``cork_ipv6_init``.

   If *str* doesn't represent a valid address (of a compatible IP
   version), then we leave *addr* unchanged, fill in the current error
   condition with a :c:data:`CORK_NET_ADDRESS_PARSE_ERROR` error, and
   return ``-1``.


.. function:: bool cork_ipv4_equal(const struct cork_ipv4 \*addr1, const struct cork_ipv4 \*addr2)
              bool cork_ipv6_equal(const struct cork_ipv6 \*addr1, const struct cork_ipv6 \*addr2)
              bool cork_ip_equal(const struct cork_ip \*addr1, const struct cork_ip \*addr2)

   Checks two IP addresses for equality.


.. macro:: CORK_IPV4_STRING_LENGTH
           CORK_IPV6_STRING_LENGTH
           CORK_IP_STRING_LENGTH

   The maximum length of the string representation of an IPv4, IPv6, or
   generic IP address, including a ``NUL`` terminator.

.. function:: void cork_ipv4_to_raw_string(const struct cork_ipv4 \*addr, char \*dest)
              void cork_ipv6_to_raw_string(const struct cork_ipv6 \*addr, char \*dest)
              void cork_ip_to_raw_string(const struct cork_ip \*addr, char \*dest)

   Fills in *dest* with the string representation of an IPv4, IPv6, or
   generic IP address.  You are responsible for ensuring that *dest* is
   large enough to hold the string representation of any valid IP
   address of the given version.  The
   :c:macro:`CORK_IPV4_STRING_LENGTH`,
   :c:macro:`CORK_IPV6_STRING_LENGTH`, and
   :c:macro:`CORK_IP_STRING_LENGTH` macros can be helpful for this::

     char  buf[CORK_IPV4_STRING_LENGTH];
     struct cork_ipv4  addr;
     cork_ipv4_to_raw_string(&addr, buf);


.. function:: bool cork_ipv4_is_valid_network(const struct cork_ipv4 \*addr, unsigned int cidr_prefix)
              bool cork_ipv6_is_valid_network(const struct cork_ipv6 \*addr, unsigned int cidr_prefix)
              bool cork_ip_is_valid_network(const struct cork_ipv6 \*addr, unsigned int cidr_prefix)

    Checks an IP address for alignment with a CIDR block prefix. For example,
    10.1.2.4/24 is invalid, but 10.1.2.4/30 is valid.


.. macro:: CORK_NET_ADDRESS_ERROR
           CORK_NET_ADDRESS_PARSE_ERROR

   The error class and codes used for the :ref:`error conditions
   <errors>` described in this section.
