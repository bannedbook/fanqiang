# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011-2014, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license details.
# ----------------------------------------------------------------------

find_program(
  SPHINX_EXECUTABLE
  NAMES sphinx-build
  HINTS ENV SPHINX_DIR
  PATH_SUFFIXES bin
  DOC "Sphinx documentation generator"
)

set(GENERATE_DOC TRUE)

if (TILERA)
    message(STATUS "Note: We don't build documentation on the Tilera platform")
    set(GENERATE_DOC FALSE)
endif (TILERA)

if (NOT SPHINX_EXECUTABLE)
  message(WARNING "Unable to find Sphinx documentation generator")
  set(GENERATE_DOC FALSE)
endif (NOT SPHINX_EXECUTABLE)

set(INTERSPHINX_OVERRIDES "")

macro(find_prereq_doc LIB_NAME)
  execute_process(
    COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=sphinxdir ${LIB_NAME}
    OUTPUT_VARIABLE LIB_SPHINX_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (LIB_SPHINX_DIR)
    message(STATUS "Using ${LIB_NAME} docs in ${LIB_SPHINX_DIR}")
    set(
      INTERSPHINX_OVERRIDES
      "${INTERSPHINX_OVERRIDES}\nintersphinx_mapping['${LIB_NAME}'] = ('${LIB_SPHINX_DIR}', None)"
    )
  endif (LIB_SPHINX_DIR)
endmacro(find_prereq_doc)

if (GENERATE_DOC)
  set(SPHINX_SRC_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/conf.py")
  set(SPHINX_CONF_FILE "${CMAKE_CURRENT_BINARY_DIR}/conf.py")
  set(SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")
  set(SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")

  # If your Sphinx documentation references the Sphinx documentation of
  # any of your prerequisite libraries, add some calls to
  # find_prereq_doc here:
  #
  # find_prereq_doc(libcork)

  set(
    VERSION_FOR_CONF_PY
    "\nrelease=\"${VERSION}\"\nversion=\"${BASE_VERSION}\""
  )
  configure_file(${SPHINX_SRC_CONF_FILE} ${SPHINX_CONF_FILE} @ONLY)
  file(COPY _static DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  file(COPY _templates DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  add_custom_target(old-doc ALL
    ${SPHINX_EXECUTABLE}
      -b html
      -d "${SPHINX_CACHE_DIR}"
      -c "${CMAKE_CURRENT_BINARY_DIR}"
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${SPHINX_HTML_DIR}"
    COMMENT "Building HTML documentation with Sphinx"
  )

  list(APPEND CLEAN_FILES "${SPHINX_CACHE_DIR}" "${SPHINX_HTML_DIR}")
  set_directory_properties(
    PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "${CLEAN_FILES}"
  )

  install(
    DIRECTORY "${SPHINX_HTML_DIR}"
    DESTINATION "share/doc/${PROJECT_NAME}"
  )
endif (GENERATE_DOC)
