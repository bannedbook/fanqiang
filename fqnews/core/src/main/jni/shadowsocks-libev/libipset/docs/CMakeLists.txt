# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011-2013, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

# Fill in this with the text that you want to include in the header and footer
# of each man page.

set(MAN_HEADER "ipset documentation")
set(MAN_FOOTER "ipset")

# Fill this in with any man pages that should be built from a pandoc source
# file.  For a man page called foo.5, there should be a pandoc source file
# called foo.5.pandoc.

set(MAN_PAGES
)

#-----------------------------------------------------------------------
# Everything below is boilerplate!

find_program(
    PANDOC_EXECUTABLE
    NAMES pandoc
    HINTS ENV PANDOC_DIR
    PATH_SUFFIXES bin
    DOC "Pandoc man page generator"
)

set(GENERATE_DOC TRUE CACHE BOOL
    "Whether to rebuild documentation")

if (NOT PANDOC_EXECUTABLE)
    message(WARNING "Unable to find pandoc documentation generator")
    set(GENERATE_DOC FALSE)
endif (NOT PANDOC_EXECUTABLE)


# Link man pages go in docs/links

macro(install_links section)
    file(GLOB links "links/*.${section}")
    if (links)
        install(
            FILES ${links}
            DESTINATION "share/man/man${section}"
        )
    endif (links)
endmacro(install_links section)

install_links(1)   # commands
install_links(3)   # library API
install_links(4)   # special files and drivers
install_links(5)   # file formats and conventions
install_links(7)   # miscellaneous
install_links(8)   # system commands


# Man pages with actual content go in docs

set(ALL_MANS)

macro(pandocify name)
    set(src  "${CMAKE_CURRENT_SOURCE_DIR}/${name}.pandoc")
    set(dest "${CMAKE_CURRENT_SOURCE_DIR}/${name}")
    get_filename_component(section "${name}" EXT)
    string(REPLACE "." "" section "${section}")

    # Only compile the markdown source into groff man pages if requested.
    if (GENERATE_DOC)
        add_custom_command(
            OUTPUT ${dest}
            COMMAND ${PANDOC_EXECUTABLE}
                -f markdown -t man -s --smart
                -V header="${MAN_HEADER}"
                -V footer="${MAN_FOOTER}"
                -V date=${RELEASE_DATE}
                -o ${dest} ${src}
            MAIN_DEPENDENCY ${src}
            COMMENT "Building ${name}"
        )
        list(APPEND ALL_MANS ${dest})
    endif (GENERATE_DOC)

    # We should always have an already-compiled copy of each man page in the
    # source tree, which we can install even if we didn't build fresh new
    # copies.
    install(
        FILES ${dest}
        DESTINATION "share/man/man${section}"
    )
endmacro(pandocify)

foreach(MAN_PAGE ${MAN_PAGES})
    pandocify(${MAN_PAGE})
endforeach(MAN_PAGE)

add_custom_target(doc ALL DEPENDS ${ALL_MANS})
