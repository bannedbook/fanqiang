# ipset

[![Build Status](https://img.shields.io/travis/redjack/ipset/develop.svg)](https://travis-ci.org/redjack/ipset)

The ipset library provides C data types for storing sets of IP
addresses, and maps of IP addresses to integers.  It supports both
IPv4 and IPv6 addresses.  It's implemented using [Binary Decision
Diagrams](http://en.wikipedia.org/wiki/Binary_decision_diagram)
(BDDs), which (we hypothesize) makes it space efficient for large
sets.

Please see the INSTALL file for installation instructions.
