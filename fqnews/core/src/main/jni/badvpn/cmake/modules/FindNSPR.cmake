# - Try to find the NSPR library
# Once done this will define
#
#  NSPR_FOUND - system has the NSPR library
#  NSPR_INCLUDE_DIRS - Include paths needed
#  NSPR_LIBRARY_DIRS - Linker paths needed
#  NSPR_LIBRARIES - Libraries needed

# Copyright (c) 2010, Ambroz Bizjak, <ambrop7@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindLibraryWithDebug)

if (NSPR_LIBRARIES)
   set(NSPR_FIND_QUIETLY TRUE)
endif ()

set(NSPR_FOUND FALSE)

if (WIN32)
    find_path(NSPR_FIND_INCLUDE_DIR prerror.h)

    FIND_LIBRARY_WITH_DEBUG(NSPR_FIND_LIBRARIES_PLDS WIN32_DEBUG_POSTFIX d NAMES plds4 libplds4)
    FIND_LIBRARY_WITH_DEBUG(NSPR_FIND_LIBRARIES_PLC WIN32_DEBUG_POSTFIX d NAMES plc4 libplc4)
    FIND_LIBRARY_WITH_DEBUG(NSPR_FIND_LIBRARIES_NSPR WIN32_DEBUG_POSTFIX d NAMES nspr4 libnspr4)

    if (NSPR_FIND_INCLUDE_DIR AND NSPR_FIND_LIBRARIES_PLDS AND NSPR_FIND_LIBRARIES_PLC AND NSPR_FIND_LIBRARIES_NSPR)
        set(NSPR_FOUND TRUE)
        set(NSPR_INCLUDE_DIRS "${NSPR_FIND_INCLUDE_DIR}" CACHE STRING "NSPR include dirs")
        set(NSPR_LIBRARY_DIRS "" CACHE STRING "NSPR library dirs")
        set(NSPR_LIBRARIES "${NSPR_FIND_LIBRARIES_PLDS};${NSPR_FIND_LIBRARIES_PLC};${NSPR_FIND_LIBRARIES_NSPR}" CACHE STRING "NSPR libraries")
    endif ()
else ()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NSPR_PC nspr)

    if (NSPR_PC_FOUND)
        set(NSPR_FOUND TRUE)
        set(NSPR_INCLUDE_DIRS "${NSPR_PC_INCLUDE_DIRS}" CACHE STRING "NSPR include dirs")
        set(NSPR_LIBRARY_DIRS "${NSPR_PC_LIBRARY_DIRS}" CACHE STRING "NSPR library dirs")
        set(NSPR_LIBRARIES "${NSPR_PC_LIBRARIES}" CACHE STRING "NSPR libraries")
    endif ()
endif ()

if (NSPR_FOUND)
    if (NOT NSPR_FIND_QUIETLY)
        MESSAGE(STATUS "Found NSPR: ${NSPR_INCLUDE_DIRS} ${NSPR_LIBRARY_DIRS} ${NSPR_LIBRARIES}")
    endif ()
else ()
    if (NSPR_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find NSPR")
    endif ()
endif ()

mark_as_advanced(NSPR_INCLUDE_DIRS NSPR_LIBRARY_DIRS NSPR_LIBRARIES)
