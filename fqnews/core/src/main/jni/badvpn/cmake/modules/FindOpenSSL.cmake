# - Try to find the OpenSSL library
# Once done this will define
#
#  OpenSSL_FOUND - system has the OpenSSL library
#  OpenSSL_INCLUDE_DIRS - Include paths needed
#  OpenSSL_LIBRARY_DIRS - Linker paths needed
#  OpenSSL_LIBRARIES - Libraries needed

# Copyright (c) 2010, Ambroz Bizjak, <ambrop7@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindLibraryWithDebug)

if (OpenSSL_LIBRARIES)
   set(OpenSSL_FIND_QUIETLY TRUE)
endif ()

set(OpenSSL_FOUND FALSE)

if (WIN32)
    find_path(OpenSSL_FIND_INCLUDE_DIR openssl/ssl.h)

    if (OpenSSL_FIND_INCLUDE_DIR)
        # look for libraries built with GCC
        find_library(OpenSSL_FIND_LIBRARIES_SSL NAMES ssl)
        find_library(OpenSSL_FIND_LIBRARIES_CRYPTO NAMES crypto)

        if (OpenSSL_FIND_LIBRARIES_SSL AND OpenSSL_FIND_LIBRARIES_CRYPTO)
            set(OpenSSL_FOUND TRUE)
            set(OpenSSL_LIBRARY_DIRS "" CACHE STRING "OpenSSL library dirs")
            set(OpenSSL_LIBRARIES "${OpenSSL_FIND_LIBRARIES_SSL};${OpenSSL_FIND_LIBRARIES_CRYPTO}" CACHE STRING "OpenSSL libraries")
        else ()
            # look for libraries built with MSVC
            FIND_LIBRARY_WITH_DEBUG(OpenSSL_FIND_LIBRARIES_SSL WIN32_DEBUG_POSTFIX d NAMES ssl ssleay ssleay32 libssleay32 ssleay32MD)
            FIND_LIBRARY_WITH_DEBUG(OpenSSL_FIND_LIBRARIES_EAY WIN32_DEBUG_POSTFIX d NAMES eay libeay libeay32 libeay32MD)

            if (OpenSSL_FIND_LIBRARIES_SSL AND OpenSSL_FIND_LIBRARIES_EAY)
                set(OpenSSL_FOUND TRUE)
                set(OpenSSL_LIBRARY_DIRS "" CACHE STRING "OpenSSL library dirs")
                set(OpenSSL_LIBRARIES "${OpenSSL_FIND_LIBRARIES_SSL};${OpenSSL_FIND_LIBRARIES_EAY}" CACHE STRING "OpenSSL libraries")
            endif ()
        endif ()

        if (OpenSSL_FOUND)
            set(OpenSSL_INCLUDE_DIRS "${OpenSSL_FIND_INCLUDE_DIR}" CACHE STRING "OpenSSL include dirs")
        endif ()
    endif ()
else ()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OpenSSL_PC openssl)

    if (OpenSSL_PC_FOUND)
        set(OpenSSL_FOUND TRUE)
        set(OpenSSL_INCLUDE_DIRS "${OpenSSL_PC_INCLUDE_DIRS}" CACHE STRING "OpenSSL include dirs")
        set(OpenSSL_LIBRARY_DIRS "${OpenSSL_PC_LIBRARY_DIRS}" CACHE STRING "OpenSSL library dirs")
        set(OpenSSL_LIBRARIES "${OpenSSL_PC_LIBRARIES}" CACHE STRING "OpenSSL libraries")
    endif ()
endif ()

if (OpenSSL_FOUND)
    if (NOT OpenSSL_FIND_QUIETLY)
        MESSAGE(STATUS "Found OpenSSL: ${OpenSSL_INCLUDE_DIRS} ${OpenSSL_LIBRARY_DIRS} ${OpenSSL_LIBRARIES}")
    endif ()
else ()
    if (OpenSSL_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find OpenSSL")
    endif ()
endif ()

mark_as_advanced(OpenSSL_INCLUDE_DIRS OpenSSL_LIBRARY_DIRS OpenSSL_LIBRARIES)
