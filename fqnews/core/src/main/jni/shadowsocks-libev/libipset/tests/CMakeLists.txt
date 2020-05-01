# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

include_directories(../include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)

#-----------------------------------------------------------------------
# Check for prerequisite libraries

find_package(PkgConfig)
find_package(Threads)

pkg_check_modules(CHECK REQUIRED check)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

#-----------------------------------------------------------------------
# Build the test cases

macro(make_test test_name)
    add_executable(${test_name} ${test_name}.c)
    target_link_libraries(${test_name} ${CHECK_LIBRARIES} libipset ${CMAKE_THREAD_LIBS_INIT})
    add_test(${test_name} ${test_name})
endmacro(make_test)

# Add calls to make_test for each .c test case in the tests/ directory.
make_test(test-assignment)
make_test(test-bdd)
make_test(test-ipmap)
make_test(test-ipset)
make_test(test-iterator)

#-----------------------------------------------------------------------
# Command-line tests

if(TILERA)
    configure_file(tile-test ${CMAKE_BINARY_DIR}/tile-test COPYONLY)
endif(TILERA)

find_program(
    CRAM_EXECUTABLE
    NAMES cram
    HINTS ENV CRAM_DIR
    PATH_SUFFIXES bin
    DOC "Cram test harness"
)

if (CRAM_EXECUTABLE)
    configure_file(ccram ${CMAKE_BINARY_DIR}/ccram COPYONLY)
    add_test(
        cram-tests
        ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/ccram
            --cram ${CRAM_EXECUTABLE}
            --tests ${CMAKE_CURRENT_SOURCE_DIR}
    )
else (CRAM_EXECUTABLE)
    message(WARNING "Unable to find cram test harness; skipping tests.")
endif (CRAM_EXECUTABLE)
