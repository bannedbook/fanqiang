# - Check if _FILE_OFFSET_BITS macro needed for large files
# CHECK_FILE_OFFSET_BITS ()
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
# Copyright (c) 2009, Michihiro NAKAJIMA
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

#INCLUDE(CheckCSourceCompiles)

GET_FILENAME_COMPONENT(_selfdir_CheckFileOffsetBits
	 "${CMAKE_CURRENT_LIST_FILE}" PATH)

MACRO (CHECK_FILE_OFFSET_BITS)
  IF(NOT DEFINED _FILE_OFFSET_BITS)
    MESSAGE(STATUS "Cheking _FILE_OFFSET_BITS for large files")
    TRY_COMPILE(__WITHOUT_FILE_OFFSET_BITS_64
      ${CMAKE_CURRENT_BINARY_DIR}
      ${_selfdir_CheckFileOffsetBits}/CheckFileOffsetBits.c
      COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS})
    IF(NOT __WITHOUT_FILE_OFFSET_BITS_64)
      TRY_COMPILE(__WITH_FILE_OFFSET_BITS_64
        ${CMAKE_CURRENT_BINARY_DIR}
        ${_selfdir_CheckFileOffsetBits}/CheckFileOffsetBits.c
        COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_FILE_OFFSET_BITS=64)
    ENDIF(NOT __WITHOUT_FILE_OFFSET_BITS_64)

    IF(NOT __WITHOUT_FILE_OFFSET_BITS_64 AND __WITH_FILE_OFFSET_BITS_64)
      SET(_FILE_OFFSET_BITS 64 CACHE INTERNAL "_FILE_OFFSET_BITS macro needed for large files")
      MESSAGE(STATUS "Cheking _FILE_OFFSET_BITS for large files - needed")
    ELSE(NOT __WITHOUT_FILE_OFFSET_BITS_64 AND __WITH_FILE_OFFSET_BITS_64)
      SET(_FILE_OFFSET_BITS "" CACHE INTERNAL "_FILE_OFFSET_BITS macro needed for large files")
      MESSAGE(STATUS "Cheking _FILE_OFFSET_BITS for large files - not needed")
    ENDIF(NOT __WITHOUT_FILE_OFFSET_BITS_64 AND __WITH_FILE_OFFSET_BITS_64)
  ENDIF(NOT DEFINED _FILE_OFFSET_BITS)

ENDMACRO (CHECK_FILE_OFFSET_BITS)
