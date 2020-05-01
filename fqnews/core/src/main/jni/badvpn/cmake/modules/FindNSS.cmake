# - Try to find the NSS library
# Once done this will define
#
#  NSS_FOUND - system has the NSS library
#  NSS_INCLUDE_DIRS - Include paths needed
#  NSS_LIBRARY_DIRS - Linker paths needed
#  NSS_LIBRARIES - Libraries needed

# Copyright (c) 2010, Ambroz Bizjak, <ambrop7@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindLibraryWithDebug)

if (NSS_LIBRARIES)
   set(NSS_FIND_QUIETLY TRUE)
endif ()

set(NSS_FOUND FALSE)

if (WIN32)
    find_path(NSS_FIND_INCLUDE_DIR nss/nss.h)

    FIND_LIBRARY_WITH_DEBUG(NSS_FIND_LIBRARIES_SSL WIN32_DEBUG_POSTFIX d NAMES ssl3)
    FIND_LIBRARY_WITH_DEBUG(NSS_FIND_LIBRARIES_SMIME WIN32_DEBUG_POSTFIX d NAMES smime3)
    FIND_LIBRARY_WITH_DEBUG(NSS_FIND_LIBRARIES_NSS WIN32_DEBUG_POSTFIX d NAMES nss3)

    if (NSS_FIND_INCLUDE_DIR AND NSS_FIND_LIBRARIES_SSL AND NSS_FIND_LIBRARIES_SMIME AND NSS_FIND_LIBRARIES_NSS)
        set(NSS_FOUND TRUE)
        set(NSS_INCLUDE_DIRS "${NSS_FIND_INCLUDE_DIR}" CACHE STRING "NSS include dirs")
        set(NSS_LIBRARY_DIRS "" CACHE STRING "NSS library dirs")
        set(NSS_LIBRARIES "${NSS_FIND_LIBRARIES_SSL};${NSS_FIND_LIBRARIES_SMIME};${NSS_FIND_LIBRARIES_NSS}" CACHE STRING "NSS libraries")
    endif ()
else ()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NSS_PC nss)

    if (NSS_PC_FOUND)
        set(NSS_FOUND TRUE)
        set(NSS_INCLUDE_DIRS "${NSS_PC_INCLUDE_DIRS}" CACHE STRING "NSS include dirs")
        set(NSS_LIBRARY_DIRS "${NSS_PC_LIBRARY_DIRS}" CACHE STRING "NSS library dirs")
        set(NSS_LIBRARIES "${NSS_PC_LIBRARIES}" CACHE STRING "NSS libraries")
    endif ()
endif ()

if (NSS_FOUND)
    if (NOT NSS_FIND_QUIETLY)
        MESSAGE(STATUS "Found NSS: ${NSS_INCLUDE_DIRS} ${NSS_LIBRARY_DIRS} ${NSS_LIBRARIES}")
    endif ()
else ()
    if (NSS_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find NSS")
    endif ()
endif ()

mark_as_advanced(NSS_INCLUDE_DIRS NSS_LIBRARY_DIRS NSS_LIBRARIES)
