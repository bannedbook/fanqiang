# visibility.m4 serial 4 (gettext-0.18.2)
dnl Copyright (C) 2005, 2008, 2010-2011 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

dnl Tests whether the compiler supports the command-line option
dnl -fvisibility=hidden and the function and variable attributes
dnl __attribute__((__visibility__("hidden"))) and
dnl __attribute__((__visibility__("default"))).
dnl Does *not* test for __visibility__("protected") - which has tricky
dnl semantics (see the 'vismain' test in glibc) and does not exist e.g. on
dnl MacOS X.
dnl Does *not* test for __visibility__("internal") - which has processor
dnl dependent semantics.
dnl Does *not* test for #pragma GCC visibility push(hidden) - which is
dnl "really only recommended for legacy code".
dnl Set the variable CFLAG_VISIBILITY.
dnl Defines and sets the variable HAVE_VISIBILITY.

dnl Modified to fit with PCRE build environment by Cristian Rodríguez.

AC_DEFUN([PCRE_VISIBILITY],
[
  AC_REQUIRE([AC_PROG_CC])
  VISIBILITY_CFLAGS=
  VISIBILITY_CXXFLAGS=
  HAVE_VISIBILITY=0
  if test -n "$GCC"; then
    dnl First, check whether -Werror can be added to the command line, or
    dnl whether it leads to an error because of some other option that the
    dnl user has put into $CC $CFLAGS $CPPFLAGS.
    AC_MSG_CHECKING([whether the -Werror option is usable])
    AC_CACHE_VAL([pcre_cv_cc_vis_werror], [
      pcre_save_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -Werror"
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[]], [[]])],
        [pcre_cv_cc_vis_werror=yes],
        [pcre_cv_cc_vis_werror=no])
      CFLAGS="$pcre_save_CFLAGS"])
    AC_MSG_RESULT([$pcre_cv_cc_vis_werror])
    dnl Now check whether visibility declarations are supported.
    AC_MSG_CHECKING([for simple visibility declarations])
    AC_CACHE_VAL([pcre_cv_cc_visibility], [
      pcre_save_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -fvisibility=hidden"
      dnl We use the option -Werror and a function dummyfunc, because on some
      dnl platforms (Cygwin 1.7) the use of -fvisibility triggers a warning
      dnl "visibility attribute not supported in this configuration; ignored"
      dnl at the first function definition in every compilation unit, and we
      dnl don't want to use the option in this case.
      if test $pcre_cv_cc_vis_werror = yes; then
        CFLAGS="$CFLAGS -Werror"
      fi
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
           [[extern __attribute__((__visibility__("hidden"))) int hiddenvar;
             extern __attribute__((__visibility__("default"))) int exportedvar;
             extern __attribute__((__visibility__("hidden"))) int hiddenfunc (void);
             extern __attribute__((__visibility__("default"))) int exportedfunc (void);
             void dummyfunc (void) {}
           ]],
           [[]])],
        [pcre_cv_cc_visibility=yes],
        [pcre_cv_cc_visibility=no])
      CFLAGS="$pcre_save_CFLAGS"])
    AC_MSG_RESULT([$pcre_cv_cc_visibility])
    if test $pcre_cv_cc_visibility = yes; then
      VISIBILITY_CFLAGS="-fvisibility=hidden"
      VISIBILITY_CXXFLAGS="-fvisibility=hidden -fvisibility-inlines-hidden"
      HAVE_VISIBILITY=1
      AC_DEFINE(PCRE_EXP_DECL, [extern __attribute__ ((visibility ("default")))], [to make a symbol visible])
      AC_DEFINE(PCRE_EXP_DEFN, [__attribute__ ((visibility ("default")))], [to make a symbol visible])
      AC_DEFINE(PCRE_EXP_DATA_DEFN, [__attribute__ ((visibility ("default")))], [to make a symbol visible])
      AC_DEFINE(PCREPOSIX_EXP_DECL, [extern __attribute__ ((visibility ("default")))], [to make a symbol visible])
      AC_DEFINE(PCREPOSIX_EXP_DEFN, [extern __attribute__ ((visibility ("default")))], [to make a symbol visible])
      AC_DEFINE(PCRECPP_EXP_DECL, [extern __attribute__ ((visibility ("default")))], [to make a symbol visible])
      AC_DEFINE(PCRECPP_EXP_DEFN, [__attribute__ ((visibility ("default")))], [to make a symbol visible])
    fi
  fi
  AC_SUBST([VISIBILITY_CFLAGS])
  AC_SUBST([VISIBILITY_CXXFLAGS])
  AC_SUBST([HAVE_VISIBILITY])
  AC_DEFINE_UNQUOTED([HAVE_VISIBILITY], [$HAVE_VISIBILITY],
    [Define to 1 if the compiler supports simple visibility declarations.])
])
