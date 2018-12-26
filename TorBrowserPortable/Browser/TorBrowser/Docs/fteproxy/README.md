fteproxy
========

[![Build Status](https://travis-ci.org/kpdyer/fteproxy.svg?branch=master)](https://travis-ci.org/kpdyer/fteproxy)

* homepage: https://fteproxy.org
* source code: https://github.com/kpdyer/fteproxy
* publication: https://kpdyer.com/publications/ccs2013-fte.pdf

Overview
--------

fteproxy provides transport-layer protection to resist keyword filtering, censorship and discrimantory routing policies.
Its job is to relay datastreams, such as web browsing traffic, by encoding the stream into messages that satisfy a user-specified regular expression. 

fteproxy is powered by Format-Transforming Encryption [1] and was presented at CCS 2013.

[1] [Protocol Misidentification Made Easy with Format-Transforming Encryption](https://kpdyer.com/publications/ccs2013-fte.pdf), Kevin P. Dyer, Scott E. Coull, Thomas Ristenpart and Thomas Shrimpton

Quick Start
-----------

On Linux/OSX, use pip to install fteproxy.

```console
pip install fteproxy
```

On Windows, download pre-compiled binaries, located at: https://fteproxy.org/download

Dependencies
--------

Dependencies for building from source:
* Python 2.7.x: https://python.org/
* fte 0.0.x: https://pypi.python.org/pypi/fte
* pyptlib 0.0.x: https://gitweb.torproject.org/pluggable-transports/pyptlib.git
* obfsproxy 0.2.x: https://gitweb.torproject.org/pluggable-transports/obfsproxy.git
* Twisted 13.2.x: https://twistedmatrix.com/

Running
-------

For platform-specific examples of how to install dependencies see BUILDING.md.

There is nothing to build for fteproxy --- it is Python-only project. To run fteproxy, you need to do only the following.

```
git clone https://github.com/kpdyer/fteproxy.git
cd fteproxy
./bin/fteproxy
```

Documentation
-------------

See: https://fteproxy.org/documentation

Author
------

Please contact Kevin P. Dyer (kpdyer@gmail.com), if you have any questions.
