# - Check if the protoype we expect is correct.
# check_prototype_definition(FUNCTION PROTOTYPE RETURN HEADER VARIABLE)
#
#  FUNCTION - The name of the function (used to check if prototype exists)
#  PROTOTYPE- The prototype to check.
#  RETURN - The return value of the function.
#  HEADER - The header files required.
#  VARIABLE - The variable to store the result.
#
# Example:
#
# check_prototype_definition(getpwent_r
#     "struct passwd *getpwent_r(struct passwd *src, char *buf, int buflen)"
#     "NULL"
#     "unistd.h;pwd.h"
#     SOLARIS_GETPWENT_R)
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
#  CMAKE_REQUIRED_LIBRARIES = list of libraries to link


function(CHECK_PROTOTYPE_DEFINITION _FUNCTION _PROTOTYPE _RETURN _HEADER _VARIABLE)
    if (${_VARIABLE} MATCHES "^${_VARIABLE}$")
        set(CHECK_PROTOTYPE_DEFINITION_CONTENT "/* */\n")

        set(CHECK_PROTOTYPE_DEFINITION_FLAGS ${CMAKE_REQUIRED_FLAGS})
        if (CMAKE_REQUIRED_LIBRARIES)
          set(CHECK_PROTOTYPE_DEFINITION_LIBS
              "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
        else(CMAKE_REQUIRED_LIBRARIES)
            set(CHECK_PROTOTYPE_DEFINITION_LIBS)
        endif(CMAKE_REQUIRED_LIBRARIES)
        if (CMAKE_REQUIRED_INCLUDES)
            set(CMAKE_SYMBOL_EXISTS_INCLUDES
                "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}")
        else(CMAKE_REQUIRED_INCLUDES)
            set(CMAKE_SYMBOL_EXISTS_INCLUDES)
        endif(CMAKE_REQUIRED_INCLUDES)

        foreach(_FILE ${_HEADER})
          set(CHECK_PROTOTYPE_DEFINITION_HEADER
            "${CHECK_PROTOTYPE_DEFINITION_HEADER}#include <${_FILE}>\n")
        endforeach(_FILE)

        set(CHECK_PROTOTYPE_DEFINITION_SYMBOL ${_FUNCTION})
        set(CHECK_PROTOTYPE_DEFINITION_PROTO ${_PROTOTYPE})
        set(CHECK_PROTOTYPE_DEFINITION_RETURN ${_RETURN})

        configure_file("${PROJECT_SOURCE_DIR}/cmake/CheckPrototypeDefinition.c.in"
            "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckPrototypeDefinition.c" @ONLY)

        file(READ ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckPrototypeDefinition.c _SOURCE)

        try_compile(${_VARIABLE}
          ${CMAKE_BINARY_DIR}
          ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckPrototypeDefinition.c
          COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
          CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${CHECK_PROTOTYPE_DEFINITION_FLAGS}
          "${CHECK_PROTOTYPE_DEFINITION_LIBS}"
          "${CMAKE_SYMBOL_EXISTS_INCLUDES}"
          OUTPUT_VARIABLE OUTPUT)

        if (${_VARIABLE})
            set(${_VARIABLE} 1 CACHE INTERNAL "Have correct prototype for ${_FUNCTION}")
            message(STATUS "Checking prototype ${_FUNCTION} for ${_VARIABLE} - True")
            file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
                "Determining if the prototype ${_FUNCTION} exists for ${_VARIABLE} passed with the following output:\n"
                "${OUTPUT}\n\n")
        else (${_VARIABLE})
            message(STATUS "Checking prototype ${_FUNCTION} for ${_VARIABLE} - False")
            set(${_VARIABLE} 0 CACHE INTERNAL "Have correct prototype for ${_FUNCTION}")
            file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
                "Determining if the prototype ${_FUNCTION} exists for ${_VARIABLE} failed with the following output:\n"
                "${OUTPUT}\n\n${_SOURCE}\n\n")
        endif (${_VARIABLE})
    endif()
endfunction(CHECK_PROTOTYPE_DEFINITION)
