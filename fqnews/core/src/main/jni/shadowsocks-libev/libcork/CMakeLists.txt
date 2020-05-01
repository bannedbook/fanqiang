# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011-2015, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license details.
# ----------------------------------------------------------------------

cmake_minimum_required(VERSION 2.6)
set(PROJECT_NAME libcork)
set(RELEASE_DATE 2015-09-03)
project(${PROJECT_NAME})
enable_testing()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(ParseArguments)
find_package(Prereqs)
find_package(CTargets)

#-----------------------------------------------------------------------
# Retrieve the current version number

execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/version.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE VERSION_RESULT
    OUTPUT_VARIABLE VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(VERSION_RESULT)
    message(FATAL_ERROR
            "Cannot determine version number: " ${VERSION_RESULT})
endif(VERSION_RESULT)
# This causes an annoying extra prompt in ccmake.
# message("Current version: " ${VERSION})

string(REGEX REPLACE "-dev.*" "-dev" BASE_VERSION "${VERSION}")

if(BASE_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-dev)?$")
    set(VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(VERSION_MINOR "${CMAKE_MATCH_2}")
    set(VERSION_PATCH "${CMAKE_MATCH_3}")
else(BASE_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-dev)?$")
    message(FATAL_ERROR "Invalid version number: ${VERSION}")
endif(BASE_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-dev)?$")

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GIT_SHA1_RESULT
    OUTPUT_VARIABLE GIT_SHA1
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(GIT_SHA1_RESULT)
    message(FATAL_ERROR
            "Cannot determine git commit: " ${GIT_SHA1_RESULT})
endif(GIT_SHA1_RESULT)

#-----------------------------------------------------------------------
# Check for building on Tilera
# If the Tilera environment is installed, then $TILERA_ROOT is defined
# as the path to the active MDE.

if(DEFINED ENV{TILERA_ROOT})
    set(TILERA TRUE)
    set(TILERA_ROOT $ENV{TILERA_ROOT})
    message("-- Configuring for Tilera MDE ${TILERA_ROOT}")
    set(ENV{PKG_CONFIG_PATH}
        "${TILERA_ROOT}/tile/usr/lib/pkgconfig:${TILERA_ROOT}/tile/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}"
    )
    set(CMAKE_SYSTEM_NAME "Tilera")
    set(CMAKE_SYSTEM_PROCESSOR "tilegx")
    set(CMAKE_C_COMPILER "${TILERA_ROOT}/bin/tile-gcc")
    set(CMAKE_LINKER "${TILERA_ROOT}/bin/tile-ld")
    set(TILERA_MONITOR "${TILERA_ROOT}/bin/tile-monitor")
    #add_definitions(-std=gnu99)
    set(CMAKE_FIND_ROOT_PATH "${TILERA_ROOT}/tile")
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

#-----------------------------------------------------------------------
# Set some options

if(APPLE)
    if (NOT CMAKE_INSTALL_NAME_DIR)
        set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
    endif (NOT CMAKE_INSTALL_NAME_DIR)
endif(APPLE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(ENABLE_SHARED YES CACHE BOOL "Whether to build a shared library")
set(ENABLE_SHARED_EXECUTABLES NO CACHE BOOL
    "Whether to link executables using shared libraries")
set(ENABLE_STATIC YES CACHE BOOL "Whether to build a static library")

set(CMAKE_INSTALL_LIBDIR lib CACHE STRING
    "The base name of the installation directory for libraries")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_definitions(-Wall -Werror)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_definitions(-Wall -Werror)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Intel")
    add_definitions(-Wall -Werror)
endif(CMAKE_C_COMPILER_ID STREQUAL "GNU")

include(GNUInstallDirs)

#-----------------------------------------------------------------------
# Check for prerequisite libraries

find_package(Threads)
set(THREADS_LDFLAGS "${CMAKE_THREAD_LIBS_INIT}")
set(THREADS_STATIC_LDFLAGS "${CMAKE_THREAD_LIBS_INIT}")

#-----------------------------------------------------------------------
# Include our subdirectories

add_subdirectory(include)
add_subdirectory(share)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(docs/old)
