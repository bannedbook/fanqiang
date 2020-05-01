# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011-2015, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license details.
# ----------------------------------------------------------------------

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

#-----------------------------------------------------------------------
# Build the test cases

# For each test cases, we create two executables: one that links with a
# shared libcork, and the other that simulates embedding libcork into
# another project.  For the embedded version, we need to know which of
# the libcork submodules should be included when compiling the test
# case.  These are provided as additional parameters to the make_test
# macro.

macro(make_test test_name)
    if (ENABLE_SHARED OR ENABLE_SHARED_EXECUTABLES)
        add_executable(shared-${test_name} ${test_name}.c)
        target_link_libraries(shared-${test_name} ${CHECK_LIBRARIES}
            libcork-shared)
        add_test(shared-${test_name} shared-${test_name})
    endif (ENABLE_SHARED OR ENABLE_SHARED_EXECUTABLES)

    if (ENABLE_STATIC OR NOT ENABLE_SHARED_EXECUTABLES)
        add_executable(embedded-${test_name} ${test_name}.c)
        set_target_properties(embedded-${test_name} PROPERTIES
            COMPILE_DEFINITIONS CORK_EMBEDDED_TEST=1)
        target_link_libraries(embedded-${test_name} ${CHECK_LIBRARIES}
            libcork-static)
        add_test(embedded-${test_name} embedded-${test_name})
    endif (ENABLE_STATIC OR NOT ENABLE_SHARED_EXECUTABLES)
endmacro(make_test)

make_test(test-array)
make_test(test-bitset)
make_test(test-buffer)
make_test(test-core)
make_test(test-dllist)
make_test(test-files)
make_test(test-gc)
make_test(test-hash-table)
make_test(test-managed-buffer)
make_test(test-mempool)
make_test(test-ring-buffer)
make_test(test-slice)
make_test(test-subprocess)
make_test(test-threads)

#-----------------------------------------------------------------------
# Command-line tests

if(TILERA)
    configure_file(tile-test ${CMAKE_BINARY_DIR}/tile-test COPYONLY)
endif(TILERA)

find_package(PythonInterp)

if (PYTHON_EXECUTABLE)
    configure_file(ccram ${CMAKE_BINARY_DIR}/ccram COPYONLY)
    file(GLOB_RECURSE TESTS "${CMAKE_CURRENT_SOURCE_DIR}/*.t")
    foreach(TEST ${TESTS})
        get_filename_component(TEST_NAME "${TEST}" NAME_WE)
        add_test(
            ${TEST_NAME}
            ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/ccram
                --python ${PYTHON_EXECUTABLE}
                --root ${CMAKE_SOURCE_DIR}
                --tests ${TEST}
        )
    endforeach(TEST)
else (PYTHON_EXECUTABLE)
    message(WARNING "Unable to find Python; skipping cram tests.")
endif (PYTHON_EXECUTABLE)
