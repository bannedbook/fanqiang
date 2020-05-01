# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2015, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license details.
# ----------------------------------------------------------------------


#-----------------------------------------------------------------------
# Configuration options that control all of the below

set(PKG_CONFIG_PATH CACHE STRING "pkg-config search path")
if (PKG_CONFIG_PATH)
    set(ENV{PKG_CONFIG_PATH} "${PKG_CONFIG_PATH}:$ENV{PKG_CONFIG_PATH}")
endif (PKG_CONFIG_PATH)


#-----------------------------------------------------------------------
# pkg-config prerequisites

find_package(PkgConfig)

function(pkgconfig_prereq DEP)
    set(options OPTIONAL)
    set(one_args)
    set(multi_args)
    cmake_parse_arguments(_ "${options}" "${one_args}" "${multi_args}" ${ARGN})

    string(REGEX REPLACE "[<>=].*" "" SHORT_NAME "${DEP}")
    string(REPLACE "-" "_" SHORT_NAME "${SHORT_NAME}")
    string(TOUPPER ${SHORT_NAME} UPPER_SHORT_NAME)
    string(TOLOWER ${SHORT_NAME} LOWER_SHORT_NAME)

    set(USE_CUSTOM_${UPPER_SHORT_NAME} NO CACHE BOOL
        "Whether you want to provide custom details for ${LOWER_SHORT_NAME}")

    if (NOT USE_CUSTOM_${UPPER_SHORT_NAME})
        set(PKG_CHECK_ARGS)
        if (NOT __OPTIONAL)
            list(APPEND PKG_CHECK_ARGS REQUIRED)
        endif (NOT __OPTIONAL)
        list(APPEND PKG_CHECK_ARGS ${DEP})

        pkg_check_modules(${UPPER_SHORT_NAME} ${PKG_CHECK_ARGS})
    endif (NOT USE_CUSTOM_${UPPER_SHORT_NAME})

    include_directories(${${UPPER_SHORT_NAME}_INCLUDE_DIRS})
    link_directories(${${UPPER_SHORT_NAME}_LIBRARY_DIRS})
endfunction(pkgconfig_prereq)


#-----------------------------------------------------------------------
# find_library prerequisites

function(library_prereq LIB_NAME)
    set(options OPTIONAL)
    set(one_args)
    set(multi_args)
    cmake_parse_arguments(_ "${options}" "${one_args}" "${multi_args}" ${ARGN})

    string(REPLACE "-" "_" SHORT_NAME "${LIB_NAME}")
    string(TOUPPER ${SHORT_NAME} UPPER_SHORT_NAME)
    string(TOLOWER ${SHORT_NAME} LOWER_SHORT_NAME)

    set(USE_CUSTOM_${UPPER_SHORT_NAME} NO CACHE BOOL
        "Whether you want to provide custom details for ${LOWER_SHORT_NAME}")

    if (USE_CUSTOM_${UPPER_SHORT_NAME})
        include_directories(${${UPPER_SHORT_NAME}_INCLUDE_DIRS})
        link_directories(${${UPPER_SHORT_NAME}_LIBRARY_DIRS})
    else (USE_CUSTOM_${UPPER_SHORT_NAME})
        find_library(${UPPER_SHORT_NAME}_LIBRARIES ${LIB_NAME})
    endif (USE_CUSTOM_${UPPER_SHORT_NAME})

endfunction(library_prereq)
