# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2015, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license details.
# ----------------------------------------------------------------------


# CMake 2.8.4 and higher gives us cmake_parse_arguments out of the box.  For
# earlier versions (RHEL5!) we have to define it ourselves.  (The definition
# comes from <http://www.cmake.org/Wiki/CMakeMacroParseArguments>.)

if (CMAKE_VERSION VERSION_LESS "2.8.4")

MACRO(CMAKE_PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})
    SET(larg_names ${arg_names})
    LIST(FIND larg_names "${arg}" is_arg_name)
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name GREATER -1)
      SET(loption_names ${option_names})
      LIST(FIND loption_names "${arg}" is_option)
      IF (is_option GREATER -1)
          SET(${prefix}_${arg} TRUE)
      ELSE (is_option GREATER -1)
          SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option GREATER -1)
    ENDIF (is_arg_name GREATER -1)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(CMAKE_PARSE_ARGUMENTS)

else (CMAKE_VERSION VERSION_LESS "2.8.4")

    include(CMakeParseArguments)

endif (CMAKE_VERSION VERSION_LESS "2.8.4")
