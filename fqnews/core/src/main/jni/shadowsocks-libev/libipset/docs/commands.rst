.. _commands:

Command-line utilities
======================

The IP set library comes with several command-line utilities for building and
interacting with IP sets.


ipsetbuild
----------

.. program:: ipsetbuild

The ``ipsetbuild`` command is used to an build binary IP set file from a text
file containing a list of IP addresses and CIDR networks.

::

    $ ipsetbuild [options] <input file>...

.. option:: --output <filename>, -o <filename>

   Writes the binary IP set file to *filename*.  If this option isn't given,
   then the binary set will be written to standard output.

.. option::  --loose-cidr, -l

   Be more lenient about the address portion of any CIDR network blocks found in
   the input file.  (This is described in more detail below.)

.. option:: --verbose, -v

   Show summary information about the IP set that's built, as well as progress
   information about the files being read and written.  If this option is not
   given, the only output will be any error, alert, or warning messages that
   occur.

.. option:: --quiet, -q

   Show only error message for malformed input. All warnings, alerts, and
   summary information about the IP set is suppressed.

.. option:: --help

   Display some help text and exit.

Each input file must contain one IP address or network per line.  Lines
beginning with a ``#`` are considered comments and are ignored.  Each IP address
must have one of the following formats::

    x.x.x.x
    x.x.x.x/cidr
    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx
    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/cidr

The first two are for IPv4 addresses and networks; the second two for IPv6
addresses and networks.  For IPv6 addresses, you can use the ``::`` shorthand
notation to collapse consecutive ``0`` portions.

If an address contains a ``/cidr`` suffix, then the entire CIDR network of
addresses will be added to the set.  You must ensure that the low- order bits of
the address are set to 0; if not, we'll raise an error.  (If you pass in the
:option:`--loose-cidr <ipsetbuild --loose-cidr>` option, we won't perform this
sanity check.)

You can also prefix any input line with an exclamation point (``!``).  This
causes the given address or network to be *removed* from the output set.  This
notation can be useful to define a set that contains most of the addresses in a
large CIDR block, except for addresses at certain "holes".

The order of the addresses and networks given to ipsetbuild does not matter.  If
a particular address is added to the set more than once, or removed from the set
more than once, whether on its own or via a CIDR network, then you will get a
warning message.  (You can silence these warnings with the :option:`--quiet
<ipsetbuild --quiet>` option.)  If an address is both added to and removed from
the set, then the removal takes precedence, regardless of where the relevant
lines appear in the input file.


ipsetcat
--------

.. program:: ipsetcat

The ``ipsetcat`` command is used to print out the (non-sorted) contents of a
binary IP set file.

::

    $ ipsetcat [options] <input file>

To read from stdin, use ``-`` as the filename.

.. option:: --output <filename>, -o <filename>

   Writes the contents of the binary IP set file to *filename*.  If this option
   isn't given, then the contents will be written to standard output.

.. option:: --networks, -n

   Where possible, group the IP addresses in the set into CIDR network blocks.
   For dense sets, this can greatly reduce the amount of output that's
   generated.

.. option:: --verbose, -v

   Show progress information about the files being read and written.  If this
   option is not given, the only output will be any error messages that occur.

.. option:: --help

   Display some help text and exit.

The output will contain one IP address or network per line.  If you give the
:option:`--networks <ipsetcat --networks>` option, then we will collapse
addresses into CIDR networks where possible.  CIDR network blocks will have one
of the following formats::

    x.x.x.x/cidr
    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/cidr

Individual IP addresses will have one of the following formats::

    x.x.x.x
    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx

Note that we never include a /32 or /128 suffix for individual addresses, even
if you've requested CIDR networks via the :option:`--networks <ipsetcat
--networks>` option.

.. note::

   Please note that the output is *unsorted*.  There are no guarantees made
   about the order of the IP addresses and networks in the output.


ipsetdot
--------

.. program:: ipsetdot

The ``ipsetdot`` command is used to create a GraphViz_ file showing the BDD
structure of an IP set.

.. _GraphViz: http://www.graphviz.org/

::

    $ ipsetdot [options] <input file>

To read from stdin, use ``-`` as the filename.

.. option:: --output <filename>, -o <filename>

   Writes the GraphViz representation of the binary IP set file to *filename*.
   If this option isn't given, then the contents will be written to standard
   output.

.. option:: --verbose, -v

   Show progress information about the files being read and written.  If this
   option is not given, the only output will be any error messages that occur.

.. option:: --help

   Display some help text and exit.

Internally, IP sets are represented by a binary-decision diagram (BDD).  The
``ipsetdot`` program can be used to produce a GraphViz file that describes the
internal BDD structure for an IP set.  The GraphViz representation can then be
passed in to GraphViz's ``dot`` program, for instance, to generate an image of
the BDD's graph structure.
