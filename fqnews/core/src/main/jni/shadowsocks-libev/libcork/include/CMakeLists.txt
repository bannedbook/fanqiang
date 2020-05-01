# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011-2015, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license details.
# ----------------------------------------------------------------------

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libcork/config/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/libcork/config/version.h
    ESCAPE_QUOTES @ONLY
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libcork/config/version.h
        DESTINATION include/libcork/config)
