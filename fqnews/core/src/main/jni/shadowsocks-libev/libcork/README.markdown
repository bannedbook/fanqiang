# libcork

[![Build Status](https://img.shields.io/travis/redjack/libcork/develop.svg)](https://travis-ci.org/redjack/libcork)

So what is libcork, exactly?  It's a “simple, easily embeddable,
cross-platform C library”.  It falls roughly into the same category as
[glib](http://library.gnome.org/devel/glib/) or
[APR](http://apr.apache.org/) in the C world; the STL,
[POCO](http://pocoproject.org/), or [QtCore](http://qt.nokia.com/)
in the C++ world; or the standard libraries of any decent dynamic
language.

So if libcork has all of these comparables, why a new library?  Well,
none of the C++ options are really applicable here.  And none of the C
options work, because one of the main goals is to have the library be
highly modular, and useful in resource-constrained systems.  Once we
describe some of the design decisions that we've made in libcork, you'll
hopefully see how this fits into an interesting niche of its own.

## Using libcork

There are two primary ways to use libcork in your own software project:
as a _shared library_, or _embedded_.

When you use libcork as a shared library, you install it just like you
would any other C library.  We happen to use CMake as our build system,
so you follow the usual CMake recipe to install the library.  (See the
[INSTALL](INSTALL) file for details.)  All of the libcork code is
contained within a single shared library (called libcork.so,
libcork.dylib, or cork.dll, depending on the system).  We also install a
pkg-config file that makes it easy to add the appropriate compiler flags
in your own build scripts.  So, you use pkg-config to find libcork's
include and library files, link with libcork, and you're good to go.

The alternative is to embed libcork into your own software project's
directory structure.  In this case, your build scripts compile the
libcork source along with the rest of your code.  This has some
advantages for resource-constrained systems, since (assuming your
compiler and linker are any good), you only include the libcork routines
that you actually use.  And if your toolchain supports link-time
optimization, the libcork routines can be optimized into the rest of
your code.

Which should you use?  That's really up to you.  Linking against the
shared library adds a runtime dependency, but gives you the usual
benefits of shared libraries: the library in memory is shared across
each program that uses it; you can install a single bug-fix update and
all libcork programs automatically take advantage of the new release;
etc.  The embedding option is great if you really need to make your
library as small as possible, or if you don't want to add that runtime
dependency.

## Design decisions

Note that having libcork be **easily** embeddable has some ramifications
on the library's design.  In particular, we don't want to make any
assumptions about which build system you're embedding libcork into.  We
happen to use CMake, but you might be using autotools, waf, scons, or
any number of others.  Most cross-platform libraries follow the
autotools model of performing some checks at compile time (maybe during
a separate “configure” phase, maybe not) to choose the right API
implementation for the current platform.  Since we can't assume a build
system, we have to take a different approach, and do as many checks as
we can using the C preprocessor.  Any check that we can't make in the
preprocessor has to be driven by a C preprocessor macro definition,
which you (the libcork user) are responsible for checking for and
defining.  So we need to have as few of those as possible.
