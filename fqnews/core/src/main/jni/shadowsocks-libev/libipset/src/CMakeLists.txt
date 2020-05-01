# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011-2013, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#-----------------------------------------------------------------------
# Build the library

set(LIBIPSET_SRC
    libipset/general.c
    libipset/bdd/assignments.c
    libipset/bdd/basics.c
    libipset/bdd/bdd-iterator.c
    libipset/bdd/expanded.c
    libipset/bdd/reachable.c
    libipset/bdd/read.c
    libipset/bdd/write.c
    libipset/map/allocation.c
    libipset/map/inspection.c
    libipset/map/ipv4_map.c
    libipset/map/ipv6_map.c
    libipset/map/storage.c
    libipset/set/allocation.c
    libipset/set/inspection.c
    libipset/set/ipv4_set.c
    libipset/set/ipv6_set.c
    libipset/set/iterator.c
    libipset/set/storage.c
)

# Update the VERSION and SOVERSION properties below according to the following
# rules (taken from [1]):
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
# SOVERSION should always equal `current`.
#
# Note that changing `current` means that you are releasing a new
# backwards-incompatible version of the library.  This has implications on
# packaging, so once an API has stabilized, these should be a rare occurrence.
#
# [1] http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info

add_library(libipset SHARED ${LIBIPSET_SRC})
set_target_properties(libipset PROPERTIES
    OUTPUT_NAME ipset
    VERSION 1.1.0
    SOVERSION 1)
target_link_libraries(libipset
    ${CORK_LIBRARIES}
)

install(TARGETS libipset DESTINATION ${CMAKE_INSTALL_LIBDIR})

#-----------------------------------------------------------------------
# Utility commands

set(IPSETBUILD_SRC
    ipsetbuild/ipsetbuild.c
)

add_executable(ipsetbuild ${IPSETBUILD_SRC})
target_link_libraries(ipsetbuild libipset)
install(TARGETS ipsetbuild DESTINATION bin)

set(IPSETCAT_SRC
    ipsetcat/ipsetcat.c
)

add_executable(ipsetcat ${IPSETCAT_SRC})
target_link_libraries(ipsetcat libipset)
install(TARGETS ipsetcat DESTINATION bin)

set(IPSETDOT_SRC
    ipsetdot/ipsetdot.c
)

add_executable(ipsetdot ${IPSETBUILD_SRC})
target_link_libraries(ipsetdot libipset)
install(TARGETS ipsetdot DESTINATION bin)

#-----------------------------------------------------------------------
# Generate the pkg-config file

set(prefix ${CMAKE_INSTALL_PREFIX})
configure_file(ipset.pc.in ${CMAKE_CURRENT_BINARY_DIR}/ipset.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ipset.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
