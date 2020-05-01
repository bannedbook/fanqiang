dnl Check to find the libcares headers/libraries

AC_DEFUN([ss_CARES],
[

    AC_ARG_WITH(cares,
    AS_HELP_STRING([--with-cares=DIR], [The c-ares library base directory, or:]),
    [cares="$withval"
     CFLAGS="$CFLAGS -I$withval/include"
     LDFLAGS="$LDFLAGS -L$withval/lib"]
    )

    AC_ARG_WITH(cares-include,
    AS_HELP_STRING([--with-cares-include=DIR], [The c-ares library headers directory (without trailing /cares)]),
    [cares_include="$withval"
     CFLAGS="$CFLAGS -I$withval"]
    )

    AC_ARG_WITH(cares-lib,
    AS_HELP_STRING([--with-cares-lib=DIR], [The c-ares library library directory]),
    [cares_lib="$withval"
     LDFLAGS="$LDFLAGS -L$withval"]
    )

    AC_CHECK_LIB(cares, ares_library_init,
    [LIBS="-lcares $LIBS"],
    [AC_MSG_ERROR([The c-ares library libraries not found.])]
    )

])
