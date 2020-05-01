# FIND_PACKAGE_HANDLE_STANDARD_ARGS(NAME (DEFAULT_MSG|"Custom failure message") VAR1 ... )
#    This macro is intended to be used in FindXXX.cmake modules files.
#    It handles the REQUIRED and QUIET argument to FIND_PACKAGE() and
#    it also sets the <UPPERCASED_NAME>_FOUND variable.
#    The package is found if all variables listed are TRUE.
#    Example:
#
#    FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibXml2 DEFAULT_MSG LIBXML2_LIBRARIES LIBXML2_INCLUDE_DIR)
#
#    LibXml2 is considered to be found, if both LIBXML2_LIBRARIES and 
#    LIBXML2_INCLUDE_DIR are valid. Then also LIBXML2_FOUND is set to TRUE.
#    If it is not found and REQUIRED was used, it fails with FATAL_ERROR, 
#    independent whether QUIET was used or not.
#    If it is found, the location is reported using the VAR1 argument, so 
#    here a message "Found LibXml2: /usr/lib/libxml2.so" will be printed out.
#    If the second argument is DEFAULT_MSG, the message in the failure case will 
#    be "Could NOT find LibXml2", if you don't like this message you can specify
#    your own custom failure message there.

MACRO(FIND_PACKAGE_HANDLE_STANDARD_ARGS _NAME _FAIL_MSG _VAR1 )

  IF("${_FAIL_MSG}" STREQUAL "DEFAULT_MSG")
    IF (${_NAME}_FIND_REQUIRED)
      SET(_FAIL_MESSAGE "Could not find REQUIRED package ${_NAME}")
    ELSE (${_NAME}_FIND_REQUIRED)
      SET(_FAIL_MESSAGE "Could not find OPTIONAL package ${_NAME}")
    ENDIF (${_NAME}_FIND_REQUIRED)
  ELSE("${_FAIL_MSG}" STREQUAL "DEFAULT_MSG")
    SET(_FAIL_MESSAGE "${_FAIL_MSG}")
  ENDIF("${_FAIL_MSG}" STREQUAL "DEFAULT_MSG")

  STRING(TOUPPER ${_NAME} _NAME_UPPER)

  SET(${_NAME_UPPER}_FOUND TRUE)
  IF(NOT ${_VAR1})
    SET(${_NAME_UPPER}_FOUND FALSE)
  ENDIF(NOT ${_VAR1})

  FOREACH(_CURRENT_VAR ${ARGN})
    IF(NOT ${_CURRENT_VAR})
      SET(${_NAME_UPPER}_FOUND FALSE)
    ENDIF(NOT ${_CURRENT_VAR})
  ENDFOREACH(_CURRENT_VAR)

  IF (${_NAME_UPPER}_FOUND)
    IF (NOT ${_NAME}_FIND_QUIETLY)
        MESSAGE(STATUS "Found ${_NAME}: ${${_VAR1}}")
    ENDIF (NOT ${_NAME}_FIND_QUIETLY)
  ELSE (${_NAME_UPPER}_FOUND)
    IF (${_NAME}_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "${_FAIL_MESSAGE}")
    ELSE (${_NAME}_FIND_REQUIRED)
      IF (NOT ${_NAME}_FIND_QUIETLY)
        MESSAGE(STATUS "${_FAIL_MESSAGE}")
      ENDIF (NOT ${_NAME}_FIND_QUIETLY)
    ENDIF (${_NAME}_FIND_REQUIRED)
  ENDIF (${_NAME_UPPER}_FOUND)
ENDMACRO(FIND_PACKAGE_HANDLE_STANDARD_ARGS)
