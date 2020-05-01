.. _file-format:

File format reference
=====================

This section describes the current file format used to encode IP sets on disk.

Overview
--------

An IP set is stored internally using a `Binary Decision Diagram`_ (BDD).  This
structure is reflected in the on-disk format, too.  Taking an IPv4 address as an
example, we can create a Boolean variable for each of the 32 bits in the
address.  A set can then be thought of as a Boolean formula.  To see if a
particular IP address is in the set, you assign each variable a ``TRUE`` or
``FALSE`` value depending on whether that bit is set in the IP address.  You
then plug those variables into the formula: if the result is ``TRUE``, the
address is in the set; if it's ``FALSE``, it's not in the set.

.. _Binary Decision Diagram: http://en.wikipedia.org/wiki/Binary_decision_diagram)

To handle both IPv4 and IPv6 addresses, we use an initial variable (variable 0)
that indicates whether a particular address is IPv4 or IPv6; we use ``TRUE`` for
IPv4 and ``FALSE`` for IPv6.  IPv4 addresses then use variables 1-32 for each
bit in the address, while IPv6 addresses use variables 1-128.  (We can reuse the
variables like this since we've got variable 0 discriminating the two types of
address.)

A BDD encodes a Boolean formula as a binary search tree.  There are two kinds of
nodes: *terminal* nodes, which are the leaves of the tree, and *nonterminal*
nodes.  A terminal node is labeled with a value, representing the result of the
Boolean formula; for a set, this is either ``TRUE`` or ``FALSE``.  A nonterminal
node is labeled with a variable number, and has **low** and **high** connections
to other nodes.

To evaluate a BDD with a particular assignment of variables, you start at the
BDD's root node.  At each nonterminal, you look up the specified variable.  If
the variable is ``TRUE``, you follow the **high** pointer; if it's ``FALSE``,
you follow the **low** pointer.  When you reach a terminal node, you have the
result of the formula.

BDDs have certain restrictions placed on them for efficiency reasons.  They must
be *reduced*, which means that the **low** and **high** connections of a
nonterminal node must be different, and that you can't have two nonterminal
nodes with the same contents.  They must be *ordered*, which means that the
variable number of a nonterminal node must be less than the variable numbers of
its children.

All of this has ramifications on the storage format, since we're storing the BDD
structure of a set.  To store a BDD, we only need two basic low-level storage
elements: a *variable index*, and a *node ID*.  Since an IP set can store both
IPv4 and IPv6 addresses, we need 129 total variables: variable 0 to determine
which kind of address, and variables 1-128 to store the bits of the address.
Node IDs are references to the nodes in the BDD.

A terminal node has an ID ≥ 0.  We don't explicitly store terminal nodes in the
storage stream; instead, the node ID of a terminal node is the value associated
with that terminal node.  For sets, node 0 is the ``FALSE`` node, and node 1 is
the ``TRUE`` node.

A nonterminal node has an ID < 0.  Each nonterminal node has its own entry in
the storage stream.  The entries are stored as a flat list, starting with
nonterminal -1, followed by -2, etc.  Because of the tree structure of a BDD, we
can ensure that all references to a node appear after that node in the list.

Note that the BDD for an empty set has no nonterminal nodes — the formula always
evaluates to ``FALSE``, regardless of what values the variables have, so the BDD
is just the ``FALSE`` terminal node.  Similarly, the BDD for a set that contains
all IP addresses will just be the ``TRUE`` terminal.


On-disk syntax
--------------

With those preliminaries out of the way, we can define the actual on-disk
structure.  Note that all integers are big-endian.

Header
~~~~~~

All IP sets start with a 20-byte header.  This header starts with a six-byte
magic number, which corresponds to the ASCII string ``IP set``::

    +----+----+----+----+----+----+
    | 49 | 50 | 20 | 73 | 65 | 74 |
    +----+----+----+----+----+----+

Next comes a 16-bit version field that tells us which version of the IP set file
format is in use.  The current version is 1::

    +----+----+
    | 00 | 01 |
    +----+----+

Next comes a 64-bit length field.  This gives us the length of the *entire*
serialized IP set, including the magic number and other header fields.

::

    +----+----+----+----+----+----+----+----+
    |                 Length                |
    +----+----+----+----+----+----+----+----+

The last header field is a 32-bit integer giving the number of nonterminal nodes
in the set.

::

    +----+----+----+----+
    | Nonterminal count |
    +----+----+----+----+

Terminal node
~~~~~~~~~~~~~

As mentioned above, it's possible for there to be no nonterminals in the set's
BDD.  If there aren't any nonterminals, there must exactly one terminal in the
BDD.  This indicates that the set is empty (if it's the ``FALSE`` terminal), or
contains every IP address (if it's the ``TRUE`` terminal).  In this case, the
header is followed by a 32-bit integer that encodes the value of the BDD's
terminal node.

::

    +----+----+----+----+
    |  Terminal value   |
    +----+----+----+----+

Nonterminal nodes
~~~~~~~~~~~~~~~~~

If there are any nonterminal nodes in the BDD, the header is followed by a list
of *node structures*, one for each nonterminal.  Each node structure starts with
an 8-bit variable index::

    +----+
    | VI |
    +----+

Variable 0 determines whether a particular address is IPv4 or IPv6.  Variables
1-32 represent the bits of an IPv4 address; variables 1-128 represent the bits
of an IPv6 address.  (The bits of an address are numbered in big-endian byte
order, and from MSB to LSB within each byte.)

Next comes the **low** pointer and **high** pointer.  Each is encoded as a
32-bit node ID.  Node IDs ≥ 0 point to terminal nodes; in this case, the node ID
is the value of the terminal node.  Node IDs < 0 point to nonterminal nodes;
node -1 is the first node in the list, node -2 is second, etc.

::

    +----+----+----+----+
    |    Low pointer    |
    +----+----+----+----+
    |    High pointer   |
    +----+----+----+----+

We use a depth-first search when writing the nodes to disk.  This ensures any
nonterminal node reference points to a node earlier in the node list.
Therefore, when you read in an IP set, you can make a single pass through the
node list; whenever you encounter a node reference, you can assume that the node
it points to has already been read in.
