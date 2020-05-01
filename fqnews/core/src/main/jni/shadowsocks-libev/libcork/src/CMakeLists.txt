# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011-2015, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license details.
# ----------------------------------------------------------------------

#-----------------------------------------------------------------------
# libcork

# Update the VERSION property below according to the following rules (taken from
# [1]):
#
# VERSION = current.revision.age
#
#   1. Start with a VERSION of `0.0.0` for each shared library.
#   2. Update VERSION only immediately before a public release of your software.
#      More frequent updates are unnecessary, and only guarantee that the
#      current interface number gets larger faster.
#   3. If the library source code has changed at all since the last update, then
#      increment `revision` (`c.r.a` becomes `c.r+1.a`).
#   4. If any interfaces have been added, removed, or changed since the last
#      update, increment `current`, and set `revision` to 0.
#   5. If any interfaces have been added since the last public release, then
#      increment `age`.
#   6. If any interfaces have been removed or changed since the last public
#      release, then set `age` to 0.
#
# Note that changing `current` means that you are releasing a new
# backwards-incompatible version of the library.  This has implications on
# packaging, so once an API has stabilized, this should be a rare occurrence.
#
# [1] http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info

add_c_library(
    libcork
    OUTPUT_NAME cork
    PKGCONFIG_NAME libcork
    VERSION 16.0.1
    SOURCES
        libcork/cli/commands.c
        libcork/core/allocator.c
        libcork/core/error.c
        libcork/core/gc.c
        libcork/core/hash.c
        libcork/core/ip-address.c
        libcork/core/mempool.c
        libcork/core/timestamp.c
        libcork/core/u128.c
        libcork/core/version.c
        libcork/ds/array.c
        libcork/ds/bitset.c
        libcork/ds/buffer.c
        libcork/ds/dllist.c
        libcork/ds/file-stream.c
        libcork/ds/hash-table.c
        libcork/ds/managed-buffer.c
        libcork/ds/ring-buffer.c
        libcork/ds/slice.c
        libcork/posix/directory-walker.c
        libcork/posix/env.c
        libcork/posix/exec.c
        libcork/posix/files.c
        libcork/posix/process.c
        libcork/posix/subprocess.c
        libcork/pthreads/thread.c
    LIBRARIES
        threads
)

if (ENABLE_SHARED OR ENABLE_SHARED_EXECUTABLES)
    set_target_properties(libcork-shared PROPERTIES
        COMPILE_DEFINITIONS CORK_API=CORK_EXPORT
    )
endif (ENABLE_SHARED OR ENABLE_SHARED_EXECUTABLES)

if (ENABLE_STATIC OR NOT ENABLE_SHARED_EXECUTABLES)
    set_target_properties(libcork-static PROPERTIES
        COMPILE_DEFINITIONS CORK_API=CORK_LOCAL
    )
endif (ENABLE_STATIC OR NOT ENABLE_SHARED_EXECUTABLES)


#-----------------------------------------------------------------------
# Utility commands

add_c_executable(
    cork-hash
    OUTPUT_NAME cork-hash
    SOURCES cork-hash/cork-hash.c
    LOCAL_LIBRARIES
        libcork
)

add_c_executable(
    cork-initializer
    SKIP_INSTALL
    OUTPUT_NAME cork-initializer
    SOURCES
        cork-initializer/init1.c
        cork-initializer/init2.c
        cork-initializer/main.c
    LOCAL_LIBRARIES
        libcork
)

add_c_executable(
    cork-test
    SKIP_INSTALL
    OUTPUT_NAME cork-test
    SOURCES cork-test/cork-test.c
    LOCAL_LIBRARIES
        libcork
)
