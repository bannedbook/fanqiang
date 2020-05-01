# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2015, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license details.
# ----------------------------------------------------------------------


#-----------------------------------------------------------------------
# Configuration options that control all of the below

set(ENABLE_SHARED YES CACHE BOOL "Whether to build a shared library")
set(ENABLE_SHARED_EXECUTABLES YES CACHE BOOL
    "Whether to link executables using shared libraries")
set(ENABLE_STATIC YES CACHE BOOL "Whether to build a static library")


#-----------------------------------------------------------------------
# Library, with options to build both shared and static versions

function(target_add_shared_libraries TARGET_NAME LIBRARIES LOCAL_LIBRARIES)
    foreach(lib ${LIBRARIES})
        string(REPLACE "-" "_" lib ${lib})
        string(TOUPPER ${lib} upperlib)
        target_link_libraries(
            ${TARGET_NAME}
            ${${upperlib}_LDFLAGS}
        )
    endforeach(lib)
    foreach(lib ${LOCAL_LIBRARIES})
        target_link_libraries(${TARGET_NAME} ${lib}-shared)
    endforeach(lib)
endfunction(target_add_shared_libraries)

function(target_add_static_libraries TARGET_NAME LIBRARIES LOCAL_LIBRARIES)
    foreach(lib ${LIBRARIES})
        string(REPLACE "-" "_" lib ${lib})
        string(TOUPPER ${lib} upperlib)
        target_link_libraries(
            ${TARGET_NAME}
            ${${upperlib}_STATIC_LDFLAGS}
        )
    endforeach(lib)
    foreach(lib ${LOCAL_LIBRARIES})
        target_link_libraries(${TARGET_NAME} ${lib}-static)
    endforeach(lib)
endfunction(target_add_static_libraries)

set_property(GLOBAL PROPERTY ALL_LOCAL_LIBRARIES "")

function(add_c_library __TARGET_NAME)
    set(options)
    set(one_args OUTPUT_NAME PKGCONFIG_NAME VERSION)
    set(multi_args LIBRARIES LOCAL_LIBRARIES SOURCES)
    cmake_parse_arguments(_ "${options}" "${one_args}" "${multi_args}" ${ARGN})

    if (__VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-dev)?$")
        set(__VERSION_CURRENT  "${CMAKE_MATCH_1}")
        set(__VERSION_REVISION "${CMAKE_MATCH_2}")
        set(__VERSION_AGE      "${CMAKE_MATCH_3}")
    else (__VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-dev)?$")
        message(FATAL_ERROR "Invalid library version number: ${__VERSION}")
    endif (__VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-dev)?$")

    math(EXPR __SOVERSION "${__VERSION_CURRENT} - ${__VERSION_AGE}")

    get_property(ALL_LOCAL_LIBRARIES GLOBAL PROPERTY ALL_LOCAL_LIBRARIES)
    list(APPEND ALL_LOCAL_LIBRARIES ${__TARGET_NAME})
    set_property(GLOBAL PROPERTY ALL_LOCAL_LIBRARIES "${ALL_LOCAL_LIBRARIES}")

    if (ENABLE_SHARED OR ENABLE_SHARED_EXECUTABLES)
        add_library(${__TARGET_NAME}-shared SHARED ${__SOURCES})
        set_target_properties(
            ${__TARGET_NAME}-shared PROPERTIES
            OUTPUT_NAME ${__OUTPUT_NAME}
            CLEAN_DIRECT_OUTPUT 1
            VERSION ${__VERSION}
            SOVERSION ${__SOVERSION}
        )

        if (CMAKE_VERSION VERSION_GREATER "2.8.11")
            target_include_directories(
                ${__TARGET_NAME}-shared PUBLIC
                ${CMAKE_SOURCE_DIR}/include
                ${CMAKE_BINARY_DIR}/include
            )
        else (CMAKE_VERSION VERSION_GREATER "2.8.11")
            include_directories(
                ${CMAKE_SOURCE_DIR}/include
                ${CMAKE_BINARY_DIR}/include
            )
        endif (CMAKE_VERSION VERSION_GREATER "2.8.11")

        target_add_shared_libraries(
            ${__TARGET_NAME}-shared
            "${__LIBRARIES}"
            "${__LOCAL_LIBRARIES}"
        )

        # We have to install the shared library if the user asked us to, or if
        # the user asked us to link our programs with the shared library.
        install(TARGETS ${__TARGET_NAME}-shared
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif (ENABLE_SHARED OR ENABLE_SHARED_EXECUTABLES)

    if (ENABLE_STATIC OR NOT ENABLE_SHARED_EXECUTABLES)
        add_library(${__TARGET_NAME}-static STATIC ${__SOURCES})
        set_target_properties(
            ${__TARGET_NAME}-static PROPERTIES
            OUTPUT_NAME ${__OUTPUT_NAME}
            CLEAN_DIRECT_OUTPUT 1
        )

        if (CMAKE_VERSION VERSION_GREATER "2.8.11")
            target_include_directories(
                ${__TARGET_NAME}-static PUBLIC
                ${CMAKE_SOURCE_DIR}/include
                ${CMAKE_BINARY_DIR}/include
            )
        else (CMAKE_VERSION VERSION_GREATER "2.8.11")
            include_directories(
                ${CMAKE_SOURCE_DIR}/include
                ${CMAKE_BINARY_DIR}/include
            )
        endif (CMAKE_VERSION VERSION_GREATER "2.8.11")

        target_add_static_libraries(
            ${__TARGET_NAME}-static
            "${__LIBRARIES}"
            "${__LOCAL_LIBRARIES}"
        )
    endif (ENABLE_STATIC OR NOT ENABLE_SHARED_EXECUTABLES)

    if (ENABLE_STATIC)
        # We DON'T have to install the static library if the user asked us to
        # link our programs statically.
        install(TARGETS ${__TARGET_NAME}-static
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif (ENABLE_STATIC)

    set(prefix ${CMAKE_INSTALL_PREFIX})
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${__PKGCONFIG_NAME}.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/${__PKGCONFIG_NAME}.pc
        @ONLY
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${__PKGCONFIG_NAME}.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
endfunction(add_c_library)


#-----------------------------------------------------------------------
# Executable

function(add_c_executable __TARGET_NAME)
    set(options SKIP_INSTALL)
    set(one_args OUTPUT_NAME)
    set(multi_args LIBRARIES LOCAL_LIBRARIES SOURCES)
    cmake_parse_arguments(_ "${options}" "${one_args}" "${multi_args}" ${ARGN})

    add_executable(${__TARGET_NAME} ${__SOURCES})

    if (CMAKE_VERSION VERSION_GREATER "2.8.11")
        target_include_directories(
            ${__TARGET_NAME} PUBLIC
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_BINARY_DIR}/include
        )
    else (CMAKE_VERSION VERSION_GREATER "2.8.11")
        include_directories(
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_BINARY_DIR}/include
        )
    endif (CMAKE_VERSION VERSION_GREATER "2.8.11")

    if (ENABLE_SHARED_EXECUTABLES)
        target_add_shared_libraries(
            ${__TARGET_NAME}
            "${__LIBRARIES}"
            "${__LOCAL_LIBRARIES}"
        )
    else (ENABLE_SHARED_EXECUTABLES)
        target_add_static_libraries(
            ${__TARGET_NAME}
            "${__LIBRARIES}"
            "${__LOCAL_LIBRARIES}"
        )
    endif (ENABLE_SHARED_EXECUTABLES)

    if (NOT __SKIP_INSTALL)
        install(TARGETS ${__TARGET_NAME} RUNTIME DESTINATION bin)
    endif (NOT __SKIP_INSTALL)
endfunction(add_c_executable)


#-----------------------------------------------------------------------
# Test case

pkgconfig_prereq(check OPTIONAL)

function(add_c_test TEST_NAME)
    get_property(ALL_LOCAL_LIBRARIES GLOBAL PROPERTY ALL_LOCAL_LIBRARIES)
    add_c_executable(
        ${TEST_NAME}
        SKIP_INSTALL
        OUTPUT_NAME ${TEST_NAME}
        SOURCES ${TEST_NAME}.c
        LIBRARIES check
        LOCAL_LIBRARIES ${ALL_LOCAL_LIBRARIES}
    )
    add_test(${TEST_NAME} ${TEST_NAME})
endfunction(add_c_test)
