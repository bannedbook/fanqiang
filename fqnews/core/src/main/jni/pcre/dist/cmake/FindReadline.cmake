# from http://websvn.kde.org/trunk/KDE/kdeedu/cmake/modules/FindReadline.cmake
# http://websvn.kde.org/trunk/KDE/kdeedu/cmake/modules/COPYING-CMAKE-SCRIPTS
# --> BSD licensed
#
# GNU Readline library finder
if(READLINE_INCLUDE_DIR AND READLINE_LIBRARY AND NCURSES_LIBRARY)
  set(READLINE_FOUND TRUE)
else(READLINE_INCLUDE_DIR AND READLINE_LIBRARY AND NCURSES_LIBRARY)
  FIND_PATH(READLINE_INCLUDE_DIR readline/readline.h
    /usr/include/readline
  )
  
# 2008-04-22 The next clause used to read like this:
#
#  FIND_LIBRARY(READLINE_LIBRARY NAMES readline)
#        FIND_LIBRARY(NCURSES_LIBRARY NAMES ncurses )
#        include(FindPackageHandleStandardArgs)
#        FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG NCURSES_LIBRARY READLINE_INCLUDE_DIR READLINE_LIBRARY )
#
# I was advised to modify it such that it will find an ncurses library if
# required, but not if one was explicitly given, that is, it allows the
# default to be overridden. PH 

  FIND_LIBRARY(READLINE_LIBRARY NAMES readline)
        include(FindPackageHandleStandardArgs)
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG READLINE_INCLUDE_DIR READLINE_LIBRARY )

  MARK_AS_ADVANCED(READLINE_INCLUDE_DIR READLINE_LIBRARY)
endif(READLINE_INCLUDE_DIR AND READLINE_LIBRARY AND NCURSES_LIBRARY)
