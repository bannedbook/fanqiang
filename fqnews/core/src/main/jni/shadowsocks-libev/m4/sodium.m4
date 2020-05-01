dnl Check to find the libsodium headers/libraries

AC_DEFUN([ss_SODIUM],
[

    AC_ARG_WITH(sodium,
    AS_HELP_STRING([--with-sodium=DIR], [The Sodium crypto library base directory, or:]),
    [sodium="$withval"
     CFLAGS="$CFLAGS -I$withval/include"
     LDFLAGS="$LDFLAGS -L$withval/lib"]
    )

    AC_ARG_WITH(sodium-include,
    AS_HELP_STRING([--with-sodium-include=DIR], [The Sodium crypto library headers directory (without trailing /sodium)]),
    [sodium_include="$withval"
     CFLAGS="$CFLAGS -I$withval"]
    )

    AC_ARG_WITH(sodium-lib,
    AS_HELP_STRING([--with-sodium-lib=DIR], [The Sodium crypto library library directory]),
    [sodium_lib="$withval"
     LDFLAGS="$LDFLAGS -L$withval"]
    )

    AC_CHECK_LIB(sodium, sodium_init,
    [LIBS="-lsodium $LIBS"],
    [AC_MSG_ERROR([The Sodium crypto library libraries not found.])]
    )

    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
    #include <sodium.h>
    ], [
    #if SODIUM_LIBRARY_VERSION_MAJOR < 7 || SODIUM_LIBRARY_VERSION_MAJOR ==7 && SODIUM_LIBRARY_VERSION_MINOR < 6
    # error
    #endif
    ])],
    [AC_MSG_RESULT([checking for version of libsodium... yes])],
    [AC_MSG_ERROR([Wrong libsodium: version >= 1.0.4 required])])

])
