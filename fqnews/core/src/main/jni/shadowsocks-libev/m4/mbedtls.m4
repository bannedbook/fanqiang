dnl Check to find the mbed TLS headers/libraries

AC_DEFUN([ss_MBEDTLS],
[

  AC_ARG_WITH(mbedtls,
    AS_HELP_STRING([--with-mbedtls=DIR], [mbed TLS base directory, or:]),
    [mbedtls="$withval"
     CFLAGS="$CFLAGS -I$withval/include"
     LDFLAGS="$LDFLAGS -L$withval/lib"]
  )

  AC_ARG_WITH(mbedtls-include,
    AS_HELP_STRING([--with-mbedtls-include=DIR], [mbed TLS headers directory (without trailing /mbedtls)]),
    [mbedtls_include="$withval"
     CFLAGS="$CFLAGS -I$withval"]
  )

  AC_ARG_WITH(mbedtls-lib,
    AS_HELP_STRING([--with-mbedtls-lib=DIR], [mbed TLS library directory]),
    [mbedtls_lib="$withval"
     LDFLAGS="$LDFLAGS -L$withval"]
  )

  AC_CHECK_LIB(mbedcrypto, mbedtls_cipher_setup,
    [LIBS="-lmbedcrypto $LIBS"],
    [AC_MSG_ERROR([mbed TLS libraries not found.])]
  )

  AC_MSG_CHECKING([whether mbedtls supports Cipher Feedback mode or not])
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
#include <mbedtls/config.h>
      ]],
      [[
#ifndef MBEDTLS_CIPHER_MODE_CFB
#error Cipher Feedback mode a.k.a CFB not supported by your mbed TLS.
#endif
      ]]
    )],
    [AC_MSG_RESULT([ok])],
    [AC_MSG_ERROR([MBEDTLS_CIPHER_MODE_CFB required])]
  )


  AC_MSG_CHECKING([whether mbedtls supports the ARC4 stream cipher or not])
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
#include <mbedtls/config.h>
      ]],
      [[
#ifndef MBEDTLS_ARC4_C
#error the ARC4 stream cipher not supported by your mbed TLS.
#endif
      ]]
    )],
    [AC_MSG_RESULT([ok])],
    [AC_MSG_WARN([We will continue without ARC4 stream cipher support, MBEDTLS_ARC4_C required])]
  )

  AC_MSG_CHECKING([whether mbedtls supports the Blowfish block cipher or not])
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
#include <mbedtls/config.h>
      ]],
      [[
#ifndef MBEDTLS_BLOWFISH_C
#error the Blowfish block cipher not supported by your mbed TLS.
#endif
      ]]
    )],
    [AC_MSG_RESULT([ok])],
    [AC_MSG_WARN([We will continue without Blowfish block cipher support, MBEDTLS_BLOWFISH_C required])]
  )

  AC_MSG_CHECKING([whether mbedtls supports the Camellia block cipher or not])
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
#include <mbedtls/config.h>
      ]],
      [[
#ifndef MBEDTLS_CAMELLIA_C
#error the Camellia block cipher not supported by your mbed TLS.
#endif
      ]]
    )],
    [AC_MSG_RESULT([ok])],
    [AC_MSG_WARN([We will continue without Camellia block cipher support, MBEDTLS_CAMELLIA_C required])]
  )
])
