find_program(XMLTO_EXECUTABLE NAMES xmlto)
find_program(ASCIIDOC_EXECUTABLE NAMES asciidoc asciidoc.py)

# Opt-in doc build option
if (NOT XMLTO_EXECUTABLE OR NOT ASCIIDOC_EXECUTABLE)
    option(WITH_DOC_MAN "Build manpage doc" OFF)
else ()
    option(WITH_DOC_MAN "Build manpage doc" ON)
endif ()

if (NOT ASCIIDOC_EXECUTABLE)
    option(WITH_DOC_HTML "Build html doc" OFF)
else ()
    option(WITH_DOC_HTML "Build html doc" ON)
endif ()

# NOTE For brew user, we have to setup this env var. see `brew info asciidoc'
set(XMLTO_ENV)
set(XMLTO_CATALOG_DIR_MACOS /usr/local/etc/xml/catalog)
if (EXISTS ${XMLTO_CATALOG_DIR_MACOS})
    set(XMLTO_ENV XML_CATALOG_FILES=${XMLTO_CATALOG_DIR_MACOS})
    message(STATUS "Detect xmlto catalog dir ${XMLTO_CATALOG_DIR_MACOS}")
endif ()

set(CMAKE_MANPAGE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/man)
set(CMAKE_HTML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/html)

set(DOC_DIR ${CMAKE_SOURCE_DIR}/doc)
set(XMLTO_OPTS -m ${DOC_DIR}/manpage-normal.xsl -m ${DOC_DIR}/manpage-bold-literal.xsl man)
set(ASCIIDOC_XML_OPTS -b docbook -d manpage -f ${DOC_DIR}/asciidoc.conf -aversion=${PROJECT_VERSION})
set(ASCIIDOC_HTML_OPTS -b html4 -d article -f ${DOC_DIR}/asciidoc.conf -aversion=${PROJECT_VERSION})


set(MAN_NAMES ss-local.1 ss-manager.1 ss-nat.1 ss-redir.1 ss-server.1 ss-tunnel.1 shadowsocks-libev.8)
set(MAN_FILES)
set(HTML_FILES)

foreach (manfile IN LISTS MAN_NAMES)
    string(REGEX REPLACE \\.. .xml xmlfile ${manfile})
    string(REGEX REPLACE \\.. .asciidoc docfile ${manfile})
    string(REGEX REPLACE \\.. .html htmlfile ${manfile})

    set(manfile ${CMAKE_MANPAGE_OUTPUT_DIRECTORY}/${manfile})
    set(htmlfile ${CMAKE_HTML_OUTPUT_DIRECTORY}/${htmlfile})
    set(docfile ${DOC_DIR}/${docfile})

    add_custom_command(OUTPUT ${manfile}
            COMMAND ${ASCIIDOC_EXECUTABLE} ${ASCIIDOC_XML_OPTS} -o ${xmlfile} ${docfile}
            COMMAND ${CMAKE_COMMAND} -E env ${XMLTO_ENV} ${XMLTO_EXECUTABLE} ${XMLTO_OPTS} ${xmlfile}
            # After we built the manpage, the xmlfile is nolongger needed
            COMMAND ${CMAKE_COMMAND} -E remove ${xmlfile}
            DEPENDS ${docfile}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/man
            COMMENT "Building manpage ${manfile}"
            VERBATIM)
    list(APPEND MAN_FILES ${manfile})

    add_custom_command(OUTPUT ${htmlfile}
            COMMAND ${ASCIIDOC_EXECUTABLE} ${ASCIIDOC_HTML_OPTS} -o ${htmlfile} ${docfile}
            DEPENDS ${docfile}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/html
            COMMENT "Building htmlfile ${htmlfile}"
            VERBATIM)
    list(APPEND HTML_FILES ${htmlfile})
endforeach ()

add_custom_target(doc-man ALL DEPENDS ${MAN_FILES})
add_custom_target(doc-html ALL DEPENDS ${HTML_FILES})


if (NOT WITH_DOC_MAN)
    set_target_properties(doc-man PROPERTIES EXCLUDE_FROM_ALL TRUE)
else ()
    install(DIRECTORY ${CMAKE_BINARY_DIR}/man/
            DESTINATION share/man/man1
            FILES_MATCHING PATTERN "*.1"
            )
    install(DIRECTORY ${CMAKE_BINARY_DIR}/man/
            DESTINATION share/man/man8
            FILES_MATCHING PATTERN "*.8"
            )
endif ()
if (NOT WITH_DOC_HTML)
    set_target_properties(doc-html PROPERTIES EXCLUDE_FROM_ALL TRUE)
else ()
    install(DIRECTORY ${CMAKE_BINARY_DIR}/html/
            DESTINATION share/doc/${PROJECT_NAME})
endif ()

# This is required for custom command
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/man)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/html)
