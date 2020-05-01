Installation instructions
=========================

The libcork library is written in ANSI C.  It uses cmake as its build
manager.


Prerequisite libraries
----------------------

To build libcork, you need the following libraries installed on your
system:

  * pkg-config
  * check (http://check.sourceforge.net)


Building from source
--------------------

The libcork library uses cmake as its build manager.  In most cases, you
should be able to build the source code using the following:

    $ mkdir build
    $ cd build
    $ cmake .. -DCMAKE_INSTALL_PREFIX=$PREFIX
    $ make
    $ make test
    $ make install

You might have to run the last command using sudo, if you need
administrative privileges to write to the $PREFIX directory.


Shared and static libraries
---------------------------

You can use the `ENABLE_SHARED` and `ENABLE_STATIC` cmake options to control
whether or not to install shared and static versions of libcork, respectively.
By default, both are installed.

You can use the `ENABLE_SHARED_EXECUTABLE` cmake option to control whether the
programs that we install link with libcork's shared library or static library.
(Note that this can override the value of `ENABLE_SHARED`; if you ask for the
programs to link with the shared library, then we have to install that shared
library for the programs to work properly.)  By default, we link with libcork
statically.

So, as an example, if you wanted to only build and install the shared library,
and to have our programs link with that shared library, you'd replace the cmake
command with the following:

    $ cmake .. \
        -DCMAKE_INSTALL_PREFIX=$PREFIX \
        -DENABLE_SHARED=YES \
        -DENABLE_STATIC=NO \
        -DENABLE_SHARED_EXECUTABLES=YES
