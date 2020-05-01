dnl Process this file with autoconf to produce a configure script.

dnl NOTE FOR MAINTAINERS: Do not use minor version numbers 08 or 09 because
dnl the leading zeros may cause them to be treated as invalid octal constants
dnl if a PCRE user writes code that uses PCRE_MINOR as a number. There is now
dnl a check further down that throws an error if 08 or 09 are used.

dnl The PCRE_PRERELEASE feature is for identifying release candidates. It might
dnl be defined as -RC2, for example. For real releases, it should be empty.

m4_define(pcre_major, [8])
m4_define(pcre_minor, [38])
m4_define(pcre_prerelease, [])
m4_define(pcre_date, [2015-11-23])

# NOTE: The CMakeLists.txt file searches for the above variables in the first
# 50 lines of this file. Please update that if the variables above are moved.

# Libtool shared library interface versions (current:revision:age)
m4_define(libpcre_version, [3:6:2])
m4_define(libpcre16_version, [2:6:2])
m4_define(libpcre32_version, [0:6:0])
m4_define(libpcreposix_version, [0:3:0])
m4_define(libpcrecpp_version, [0:1:0])

AC_PREREQ(2.57)
AC_INIT(PCRE, pcre_major.pcre_minor[]pcre_prerelease, , pcre)
AC_CONFIG_SRCDIR([pcre.h.in])
AM_INIT_AUTOMAKE([dist-bzip2 dist-zip])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS(config.h)

# This is a new thing required to stop a warning from automake 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# This was added at the suggestion of libtoolize (03-Jan-10)
AC_CONFIG_MACRO_DIR([m4])

# The default CFLAGS and CXXFLAGS in Autoconf are "-g -O2" for gcc and just
# "-g" for any other compiler. There doesn't seem to be a standard way of
# getting rid of the -g (which I don't think is needed for a production
# library). This fudge seems to achieve the necessary. First, we remember the
# externally set values of CFLAGS and CXXFLAGS. Then call the AC_PROG_CC and
# AC_PROG_CXX macros to find the compilers - if CFLAGS and CXXFLAGS are not
# set, they will be set to Autoconf's defaults. Afterwards, if the original
# values were not set, remove the -g from the Autoconf defaults.
# (PH 02-May-07)

remember_set_CFLAGS="$CFLAGS"
remember_set_CXXFLAGS="$CXXFLAGS"

AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O

if test "x$remember_set_CFLAGS" = "x"
then
  if test "$CFLAGS" = "-g -O2"
  then
    CFLAGS="-O2"
  elif test "$CFLAGS" = "-g"
  then
    CFLAGS=""
  fi
fi

if test "x$remember_set_CXXFLAGS" = "x"
then
  if test "$CXXFLAGS" = "-g -O2"
  then
    CXXFLAGS="-O2"
  elif test "$CXXFLAGS" = "-g"
  then
    CXXFLAGS=""
  fi
fi

# AC_PROG_CXX will return "g++" even if no c++ compiler is installed.
# Check for that case, and just disable c++ code if g++ doesn't run.
AC_LANG_PUSH(C++)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],, CXX=""; CXXCP=""; CXXFLAGS="")
AC_LANG_POP

# Check for a 64-bit integer type
AC_TYPE_INT64_T

AC_PROG_INSTALL
AC_LIBTOOL_WIN32_DLL
LT_INIT
AC_PROG_LN_S

# Check for GCC visibility feature

PCRE_VISIBILITY

# Versioning

PCRE_MAJOR="pcre_major"
PCRE_MINOR="pcre_minor"
PCRE_PRERELEASE="pcre_prerelease"
PCRE_DATE="pcre_date"

if test "$PCRE_MINOR" = "08" -o "$PCRE_MINOR" = "09"
then
  echo "***"
  echo "*** Minor version number $PCRE_MINOR must not be used. ***"
  echo "*** Use only 01 to 07 or 10 onwards, to avoid octal issues. ***"
  echo "***"
  exit 1
fi

AC_SUBST(PCRE_MAJOR)
AC_SUBST(PCRE_MINOR)
AC_SUBST(PCRE_PRERELEASE)
AC_SUBST(PCRE_DATE)

# Set a more sensible default value for $(htmldir).
if test "x$htmldir" = 'x${docdir}'
then
  htmldir='${docdir}/html'
fi

# Handle --disable-pcre8 (enabled by default)
AC_ARG_ENABLE(pcre8,
              AS_HELP_STRING([--disable-pcre8],
                             [disable 8 bit character support]),
              , enable_pcre8=unset)
AC_SUBST(enable_pcre8)

# Handle --enable-pcre16 (disabled by default)
AC_ARG_ENABLE(pcre16,
              AS_HELP_STRING([--enable-pcre16],
                             [enable 16 bit character support]),
              , enable_pcre16=unset)
AC_SUBST(enable_pcre16)

# Handle --enable-pcre32 (disabled by default)
AC_ARG_ENABLE(pcre32,
              AS_HELP_STRING([--enable-pcre32],
                             [enable 32 bit character support]),
              , enable_pcre32=unset)
AC_SUBST(enable_pcre32)

# Handle --disable-cpp. The substitution of enable_cpp is needed for use in
# pcre-config.
AC_ARG_ENABLE(cpp,
              AS_HELP_STRING([--disable-cpp],
                             [disable C++ support]),
              , enable_cpp=unset)
AC_SUBST(enable_cpp)

# Handle --enable-jit (disabled by default)
AC_ARG_ENABLE(jit,
              AS_HELP_STRING([--enable-jit],
                             [enable Just-In-Time compiling support]),
              , enable_jit=no)

# Handle --disable-pcregrep-jit (enabled by default)
AC_ARG_ENABLE(pcregrep-jit,
              AS_HELP_STRING([--disable-pcregrep-jit],
                             [disable JIT support in pcregrep]),
              , enable_pcregrep_jit=yes)

# Handle --enable-rebuild-chartables
AC_ARG_ENABLE(rebuild-chartables,
              AS_HELP_STRING([--enable-rebuild-chartables],
                             [rebuild character tables in current locale]),
              , enable_rebuild_chartables=no)

# Handle --enable-utf8 (disabled by default)
AC_ARG_ENABLE(utf8,
              AS_HELP_STRING([--enable-utf8],
                             [another name for --enable-utf. Kept only for compatibility reasons]),
              , enable_utf8=unset)

# Handle --enable-utf (disabled by default)
AC_ARG_ENABLE(utf,
              AS_HELP_STRING([--enable-utf],
                             [enable UTF-8/16/32 support (incompatible with --enable-ebcdic)]),
              , enable_utf=unset)

# Handle --enable-unicode-properties
AC_ARG_ENABLE(unicode-properties,
              AS_HELP_STRING([--enable-unicode-properties],
                             [enable Unicode properties support (implies --enable-utf)]),
              , enable_unicode_properties=no)

# Handle newline options
ac_pcre_newline=lf
AC_ARG_ENABLE(newline-is-cr,
              AS_HELP_STRING([--enable-newline-is-cr],
                             [use CR as newline character]),
              ac_pcre_newline=cr)
AC_ARG_ENABLE(newline-is-lf,
              AS_HELP_STRING([--enable-newline-is-lf],
                             [use LF as newline character (default)]),
              ac_pcre_newline=lf)
AC_ARG_ENABLE(newline-is-crlf,
              AS_HELP_STRING([--enable-newline-is-crlf],
                             [use CRLF as newline sequence]),
              ac_pcre_newline=crlf)
AC_ARG_ENABLE(newline-is-anycrlf,
              AS_HELP_STRING([--enable-newline-is-anycrlf],
                             [use CR, LF, or CRLF as newline sequence]),
              ac_pcre_newline=anycrlf)
AC_ARG_ENABLE(newline-is-any,
              AS_HELP_STRING([--enable-newline-is-any],
                             [use any valid Unicode newline sequence]),
              ac_pcre_newline=any)
enable_newline="$ac_pcre_newline"

# Handle --enable-bsr-anycrlf
AC_ARG_ENABLE(bsr-anycrlf,
              AS_HELP_STRING([--enable-bsr-anycrlf],
                             [\R matches only CR, LF, CRLF by default]),
              , enable_bsr_anycrlf=no)

# Handle --enable-ebcdic
AC_ARG_ENABLE(ebcdic,
              AS_HELP_STRING([--enable-ebcdic],
                             [assume EBCDIC coding rather than ASCII; incompatible with --enable-utf; use only in (uncommon) EBCDIC environments; it implies --enable-rebuild-chartables]),
              , enable_ebcdic=no)

# Handle --enable-ebcdic-nl25
AC_ARG_ENABLE(ebcdic-nl25,
              AS_HELP_STRING([--enable-ebcdic-nl25],
                             [set EBCDIC code for NL to 0x25 instead of 0x15; it implies --enable-ebcdic]),
              , enable_ebcdic_nl25=no)

# Handle --disable-stack-for-recursion
AC_ARG_ENABLE(stack-for-recursion,
              AS_HELP_STRING([--disable-stack-for-recursion],
                             [don't use stack recursion when matching]),
              , enable_stack_for_recursion=yes)

# Handle --enable-pcregrep-libz
AC_ARG_ENABLE(pcregrep-libz,
              AS_HELP_STRING([--enable-pcregrep-libz],
                             [link pcregrep with libz to handle .gz files]),
              , enable_pcregrep_libz=no)

# Handle --enable-pcregrep-libbz2
AC_ARG_ENABLE(pcregrep-libbz2,
              AS_HELP_STRING([--enable-pcregrep-libbz2],
                             [link pcregrep with libbz2 to handle .bz2 files]),
              , enable_pcregrep_libbz2=no)

# Handle --with-pcregrep-bufsize=N
AC_ARG_WITH(pcregrep-bufsize,
              AS_HELP_STRING([--with-pcregrep-bufsize=N],
                             [pcregrep buffer size (default=20480, minimum=8192)]),
              , with_pcregrep_bufsize=20480)

# Handle --enable-pcretest-libedit
AC_ARG_ENABLE(pcretest-libedit,
              AS_HELP_STRING([--enable-pcretest-libedit],
                             [link pcretest with libedit]),
              , enable_pcretest_libedit=no)

# Handle --enable-pcretest-libreadline
AC_ARG_ENABLE(pcretest-libreadline,
              AS_HELP_STRING([--enable-pcretest-libreadline],
                             [link pcretest with libreadline]),
              , enable_pcretest_libreadline=no)

# Handle --with-posix-malloc-threshold=NBYTES
AC_ARG_WITH(posix-malloc-threshold,
            AS_HELP_STRING([--with-posix-malloc-threshold=NBYTES],
                           [threshold for POSIX malloc usage (default=10)]),
            , with_posix_malloc_threshold=10)

# Handle --with-link-size=N
AC_ARG_WITH(link-size,
            AS_HELP_STRING([--with-link-size=N],
                           [internal link size (2, 3, or 4 allowed; default=2)]),
            , with_link_size=2)

# Handle --with-parens-nest-limit=N
AC_ARG_WITH(parens-nest-limit,
            AS_HELP_STRING([--with-parens-nest-limit=N],
                           [nested parentheses limit (default=250)]),
            , with_parens_nest_limit=250)

# Handle --with-match-limit=N
AC_ARG_WITH(match-limit,
            AS_HELP_STRING([--with-match-limit=N],
                           [default limit on internal looping (default=10000000)]),
            , with_match_limit=10000000)

# Handle --with-match-limit_recursion=N
#
# Note: In config.h, the default is to define MATCH_LIMIT_RECURSION
# symbolically as MATCH_LIMIT, which in turn is defined to be some numeric
# value (e.g. 10000000). MATCH_LIMIT_RECURSION can otherwise be set to some
# different numeric value (or even the same numeric value as MATCH_LIMIT,
# though no longer defined in terms of the latter).
#
AC_ARG_WITH(match-limit-recursion,
            AS_HELP_STRING([--with-match-limit-recursion=N],
                           [default limit on internal recursion (default=MATCH_LIMIT)]),
            , with_match_limit_recursion=MATCH_LIMIT)

# Handle --enable-valgrind
AC_ARG_ENABLE(valgrind,
              AS_HELP_STRING([--enable-valgrind],
                             [valgrind support]),
              , enable_valgrind=no)

# Enable code coverage reports using gcov
AC_ARG_ENABLE(coverage,
              AS_HELP_STRING([--enable-coverage],
                             [enable code coverage reports using gcov]),
              , enable_coverage=no)

# Copy enable_utf8 value to enable_utf for compatibility reasons
if test "x$enable_utf8" != "xunset"
then
  if test "x$enable_utf" != "xunset"
  then
    AC_MSG_ERROR([--enable/disable-utf8 is kept only for compatibility reasons and its value is copied to --enable/disable-utf. Newer code must use --enable/disable-utf alone.])
  fi
  enable_utf=$enable_utf8
fi

# Set the default value for pcre8
if test "x$enable_pcre8" = "xunset"
then
  enable_pcre8=yes
fi

# Set the default value for pcre16
if test "x$enable_pcre16" = "xunset"
then
  enable_pcre16=no
fi

# Set the default value for pcre32
if test "x$enable_pcre32" = "xunset"
then
  enable_pcre32=no
fi

# Make sure enable_pcre8 or enable_pcre16 was set
if test "x$enable_pcre8$enable_pcre16$enable_pcre32" = "xnonono"
then
  AC_MSG_ERROR([At least one of 8, 16 or 32 bit pcre library must be enabled])
fi

# Make sure that if enable_unicode_properties was set, that UTF support is enabled.
if test "x$enable_unicode_properties" = "xyes"
then
  if test "x$enable_utf" = "xno"
  then
    AC_MSG_ERROR([support for Unicode properties requires UTF-8/16/32 support])
  fi
  enable_utf=yes
fi

# enable_utf is disabled by default.
if test "x$enable_utf" = "xunset"
then
  enable_utf=no
fi

# enable_cpp copies the value of enable_pcre8 by default
if test "x$enable_cpp" = "xunset"
then
  enable_cpp=$enable_pcre8
fi

# Make sure that if enable_cpp was set, that enable_pcre8 support is enabled
if test "x$enable_cpp" = "xyes"
then
  if test "x$enable_pcre8" = "xno"
  then
    AC_MSG_ERROR([C++ library requires pcre library with 8 bit characters])
  fi
fi

# Convert the newline identifier into the appropriate integer value. The first
# three are ASCII values 0x0a, 0x0d, and 0x0d0a, but if EBCDIC is enabled, they
# are changed below.

case "$enable_newline" in
  lf)      ac_pcre_newline_value=10   ;;
  cr)      ac_pcre_newline_value=13   ;;
  crlf)    ac_pcre_newline_value=3338 ;;
  anycrlf) ac_pcre_newline_value=-2   ;;
  any)     ac_pcre_newline_value=-1   ;;
  *)
  AC_MSG_ERROR([invalid argument \"$enable_newline\" to --enable-newline option])
  ;;
esac

# --enable-ebcdic-nl25 implies --enable-ebcdic
if test "x$enable_ebcdic_nl25" = "xyes"; then
  enable_ebcdic=yes
fi

# Make sure that if enable_ebcdic is set, rebuild_chartables is also enabled,
# and the newline value is adjusted appropriately (CR is still 13, but LF is
# 21 or 37). Also check that UTF support is not requested, because PCRE cannot
# handle EBCDIC and UTF in the same build. To do so it would need to use
# different character constants depending on the mode.
#
if test "x$enable_ebcdic" = "xyes"; then
  enable_rebuild_chartables=yes

  if test "x$enable_utf" = "xyes"; then
    AC_MSG_ERROR([support for EBCDIC and UTF-8/16/32 cannot be enabled at the same time])
  fi

  if test "x$enable_ebcdic_nl25" = "xno"; then
    case "$ac_pcre_newline_value" in
      10)   ac_pcre_newline_value=21 ;;
      3338) ac_pcre_newline_value=3349 ;;
    esac
  else
    case "$ac_pcre_newline_value" in
      10)   ac_pcre_newline_value=37 ;;
      3338) ac_pcre_newline_value=3365 ;;
    esac
  fi
fi

# Check argument to --with-link-size
case "$with_link_size" in
  2|3|4) ;;
  *)
  AC_MSG_ERROR([invalid argument \"$with_link_size\" to --with-link-size option])
  ;;
esac

AH_TOP([
/* PCRE is written in Standard C, but there are a few non-standard things it
can cope with, allowing it to run on SunOS4 and other "close to standard"
systems.

In environments that support the GNU autotools, config.h.in is converted into
config.h by the "configure" script. In environments that use CMake,
config-cmake.in is converted into config.h. If you are going to build PCRE "by
hand" without using "configure" or CMake, you should copy the distributed
config.h.generic to config.h, and edit the macro definitions to be the way you
need them. You must then add -DHAVE_CONFIG_H to all of your compile commands,
so that config.h is included at the start of every source.

Alternatively, you can avoid editing by using -D on the compiler command line
to set the macro values. In this case, you do not have to set -DHAVE_CONFIG_H,
but if you do, default values will be taken from config.h for non-boolean
macros that are not defined on the command line.

Boolean macros such as HAVE_STDLIB_H and SUPPORT_PCRE8 should either be defined
(conventionally to 1) for TRUE, and not defined at all for FALSE. All such
macros are listed as a commented #undef in config.h.generic. Macros such as
MATCH_LIMIT, whose actual value is relevant, have defaults defined, but are
surrounded by #ifndef/#endif lines so that the value can be overridden by -D.

PCRE uses memmove() if HAVE_MEMMOVE is defined; otherwise it uses bcopy() if
HAVE_BCOPY is defined. If your system has neither bcopy() nor memmove(), make
sure both macros are undefined; an emulation function will then be used. */])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h sys/types.h sys/stat.h dirent.h)
AC_CHECK_HEADERS([windows.h], [HAVE_WINDOWS_H=1])

# The files below are C++ header files.
pcre_have_type_traits="0"
pcre_have_bits_type_traits="0"

if test "x$enable_cpp" = "xyes" -a -z "$CXX"; then
   AC_MSG_ERROR([You need a C++ compiler for C++ support.])
fi

if test "x$enable_cpp" = "xyes" -a -n "$CXX"
then
AC_LANG_PUSH(C++)

# Older versions of pcre defined pcrecpp::no_arg, but in new versions
# it's called pcrecpp::RE::no_arg.  For backwards ABI compatibility,
# we want to make one an alias for the other.  Different systems do
# this in different ways.  Some systems, for instance, can do it via
# a linker flag: -alias (for os x 10.5) or -i (for os x <=10.4).
OLD_LDFLAGS="$LDFLAGS"
for flag in "-alias,__ZN7pcrecpp2RE6no_argE,__ZN7pcrecpp6no_argE" \
            "-i__ZN7pcrecpp6no_argE:__ZN7pcrecpp2RE6no_argE"; do
  AC_MSG_CHECKING([for alias support in the linker])
  LDFLAGS="$OLD_LDFLAGS -Wl,$flag"
  # We try to run the linker with this new ld flag.  If the link fails,
  # we give up and remove the new flag from LDFLAGS.
  AC_LINK_IFELSE([AC_LANG_PROGRAM([namespace pcrecpp {
                                    class RE { static int no_arg; };
                                    int RE::no_arg;
                                  }],
                                 [])],
                 [AC_MSG_RESULT([yes]);
                  EXTRA_LIBPCRECPP_LDFLAGS="$EXTRA_LIBPCRECPP_LDFLAGS -Wl,$flag";
                  break;],
                 AC_MSG_RESULT([no]))
done
LDFLAGS="$OLD_LDFLAGS"

# We could be more clever here, given we're doing AC_SUBST with this
# (eg set a var to be the name of the include file we want). But we're not
# so it's easy to change back to 'regular' autoconf vars if we needed to.
AC_CHECK_HEADERS(string, [pcre_have_cpp_headers="1"],
                         [pcre_have_cpp_headers="0"])
AC_CHECK_HEADERS(bits/type_traits.h, [pcre_have_bits_type_traits="1"],
                                     [pcre_have_bits_type_traits="0"])
AC_CHECK_HEADERS(type_traits.h, [pcre_have_type_traits="1"],
                                [pcre_have_type_traits="0"])

# (This isn't c++-specific, but is only used in pcrecpp.cc, so try this
# in a c++ context.  This matters becuase strtoimax is C99 and may not
# be supported by the C++ compiler.)
# Figure out how to create a longlong from a string: strtoll and
# equiv.  It's not enough to call AC_CHECK_FUNCS: hpux has a
# strtoll, for instance, but it only takes 2 args instead of 3!
# We have to call AH_TEMPLATE since AC_DEFINE_UNQUOTED below is complex.
AH_TEMPLATE(HAVE_STRTOQ, [Define to 1 if you have `strtoq'.])
AH_TEMPLATE(HAVE_STRTOLL, [Define to 1 if you have `strtoll'.])
AH_TEMPLATE(HAVE__STRTOI64, [Define to 1 if you have `_strtoi64'.])
AH_TEMPLATE(HAVE_STRTOIMAX, [Define to 1 if you have `strtoimax'.])
have_strto_fn=0
for fn in strtoq strtoll _strtoi64 strtoimax; do
  AC_MSG_CHECKING([for $fn])
  if test "$fn" = strtoimax; then
    include=stdint.h
  else
    include=stdlib.h
  fi
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <$include>],
                                    [char* e; return $fn("100", &e, 10)])],
                    [AC_MSG_RESULT(yes)
                     AC_DEFINE_UNQUOTED(HAVE_`echo $fn | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ`, 1,
                                        [Define to 1 if you have `$fn'.])
                     have_strto_fn=1
                     break],
                    [AC_MSG_RESULT(no)])
done

if test "$have_strto_fn" = 1; then
  AC_CHECK_TYPES([long long],
                 [pcre_have_long_long="1"],
                 [pcre_have_long_long="0"])
  AC_CHECK_TYPES([unsigned long long],
                 [pcre_have_ulong_long="1"],
                 [pcre_have_ulong_long="0"])
else
  pcre_have_long_long="0"
  pcre_have_ulong_long="0"
fi
AC_SUBST(pcre_have_long_long)
AC_SUBST(pcre_have_ulong_long)

AC_LANG_POP
fi
# Using AC_SUBST eliminates the need to include config.h in a public .h file
AC_SUBST(pcre_have_type_traits)
AC_SUBST(pcre_have_bits_type_traits)

# Conditional compilation
AM_CONDITIONAL(WITH_PCRE8, test "x$enable_pcre8" = "xyes")
AM_CONDITIONAL(WITH_PCRE16, test "x$enable_pcre16" = "xyes")
AM_CONDITIONAL(WITH_PCRE32, test "x$enable_pcre32" = "xyes")
AM_CONDITIONAL(WITH_PCRE_CPP, test "x$enable_cpp" = "xyes")
AM_CONDITIONAL(WITH_REBUILD_CHARTABLES, test "x$enable_rebuild_chartables" = "xyes")
AM_CONDITIONAL(WITH_JIT, test "x$enable_jit" = "xyes")
AM_CONDITIONAL(WITH_UTF, test "x$enable_utf" = "xyes")
AM_CONDITIONAL(WITH_VALGRIND, test "x$enable_valgrind" = "xyes")

# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.

AC_CHECK_FUNCS(bcopy memmove strerror)

# Check for the availability of libz (aka zlib)

AC_CHECK_HEADERS([zlib.h], [HAVE_ZLIB_H=1])
AC_CHECK_LIB([z], [gzopen], [HAVE_LIBZ=1])

# Check for the availability of libbz2. Originally we just used AC_CHECK_LIB,
# as for libz. However, this had the following problem, diagnosed and fixed by
# a user:
#
#   - libbz2 uses the Pascal calling convention (WINAPI) for the functions
#     under Win32.
#   - The standard autoconf AC_CHECK_LIB fails to include "bzlib.h",
#     therefore missing the function definition.
#   - The compiler thus generates a "C" signature for the test function.
#   - The linker fails to find the "C" function.
#   - PCRE fails to configure if asked to do so against libbz2.
#
# Solution:
#
#   - Replace the AC_CHECK_LIB test with a custom test.

AC_CHECK_HEADERS([bzlib.h], [HAVE_BZLIB_H=1])
# Original test
# AC_CHECK_LIB([bz2], [BZ2_bzopen], [HAVE_LIBBZ2=1])
#
# Custom test follows

AC_MSG_CHECKING([for libbz2])
OLD_LIBS="$LIBS"
LIBS="$LIBS -lbz2"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_BZLIB_H
#include <bzlib.h>
#endif]],
[[return (int)BZ2_bzopen("conftest", "rb");]])],
[AC_MSG_RESULT([yes]);HAVE_LIBBZ2=1; break;],
AC_MSG_RESULT([no]))
LIBS="$OLD_LIBS"

# Check for the availabiity of libreadline

if test "$enable_pcretest_libreadline" = "yes"; then
 AC_CHECK_HEADERS([readline/readline.h], [HAVE_READLINE_H=1])
 AC_CHECK_HEADERS([readline/history.h], [HAVE_HISTORY_H=1])
 AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-lreadline"],
   [unset ac_cv_lib_readline_readline;
    AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-ltinfo"],
     [unset ac_cv_lib_readline_readline;
      AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-lcurses"],
       [unset ac_cv_lib_readline_readline;
        AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-lncurses"],
         [unset ac_cv_lib_readline_readline;
	  AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-lncursesw"],
           [unset ac_cv_lib_readline_readline;
	    AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-ltermcap"],
             [LIBREADLINE=""],
             [-ltermcap])],
           [-lncursesw])],
         [-lncurses])],
       [-lcurses])],
     [-ltinfo])])
 AC_SUBST(LIBREADLINE)
 if test -n "$LIBREADLINE"; then
   if test "$LIBREADLINE" != "-lreadline"; then
     echo "-lreadline needs $LIBREADLINE"
     LIBREADLINE="-lreadline $LIBREADLINE"
   fi
 fi
fi


# Check for the availability of libedit. Different distributions put its
# headers in different places. Try to cover the most common ones.

if test "$enable_pcretest_libedit" = "yes"; then
  AC_CHECK_HEADERS([editline/readline.h], [HAVE_EDITLINE_READLINE_H=1],
    [AC_CHECK_HEADERS([edit/readline/readline.h], [HAVE_READLINE_READLINE_H=1],
      [AC_CHECK_HEADERS([readline/readline.h], [HAVE_READLINE_READLINE_H=1])])])
  AC_CHECK_LIB([edit], [readline], [LIBEDIT="-ledit"])
fi

# This facilitates -ansi builds under Linux
dnl AC_DEFINE([_GNU_SOURCE], [], [Enable GNU extensions in glibc])

PCRE_STATIC_CFLAG=""
if test "x$enable_shared" = "xno" ; then
  AC_DEFINE([PCRE_STATIC], [1], [
    Define to any value if linking statically (TODO: make nice with Libtool)])
  PCRE_STATIC_CFLAG="-DPCRE_STATIC"
fi
AC_SUBST(PCRE_STATIC_CFLAG)

# Here is where pcre specific defines are handled

if test "$enable_pcre8" = "yes"; then
  AC_DEFINE([SUPPORT_PCRE8], [], [
    Define to any value to enable the 8 bit PCRE library.])
fi

if test "$enable_pcre16" = "yes"; then
  AC_DEFINE([SUPPORT_PCRE16], [], [
    Define to any value to enable the 16 bit PCRE library.])
fi

if test "$enable_pcre32" = "yes"; then
  AC_DEFINE([SUPPORT_PCRE32], [], [
    Define to any value to enable the 32 bit PCRE library.])
fi

# Unless running under Windows, JIT support requires pthreads.

if test "$enable_jit" = "yes"; then
  if test "$HAVE_WINDOWS_H" != "1"; then
    AX_PTHREAD([], [AC_MSG_ERROR([JIT support requires pthreads])])
    CC="$PTHREAD_CC"
    CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
    LIBS="$PTHREAD_LIBS $LIBS"
  fi
  AC_DEFINE([SUPPORT_JIT], [], [
    Define to any value to enable support for Just-In-Time compiling.])
else
  enable_pcregrep_jit="no"
fi

if test "$enable_pcregrep_jit" = "yes"; then
  AC_DEFINE([SUPPORT_PCREGREP_JIT], [], [
    Define to any value to enable JIT support in pcregrep.])
fi

if test "$enable_utf" = "yes"; then
  AC_DEFINE([SUPPORT_UTF], [], [
    Define to any value to enable support for the UTF-8/16/32 Unicode encoding.
    This will work even in an EBCDIC environment, but it is incompatible
    with the EBCDIC macro. That is, PCRE can support *either* EBCDIC
    code *or* ASCII/UTF-8/16/32, but not both at once.])
fi

if test "$enable_unicode_properties" = "yes"; then
  AC_DEFINE([SUPPORT_UCP], [], [
    Define to any value to enable support for Unicode properties.])
fi

if test "$enable_stack_for_recursion" = "no"; then
  AC_DEFINE([NO_RECURSE], [], [
    PCRE uses recursive function calls to handle backtracking while
    matching. This can sometimes be a problem on systems that have
    stacks of limited size. Define NO_RECURSE to any value to get a
    version that doesn't use recursion in the match() function; instead
    it creates its own stack by steam using pcre_recurse_malloc() to obtain
    memory from the heap. For more detail, see the comments and other stuff
    just above the match() function.])
fi

if test "$enable_pcregrep_libz" = "yes"; then
  AC_DEFINE([SUPPORT_LIBZ], [], [
    Define to any value to allow pcregrep to be linked with libz, so that it is
    able to handle .gz files.])
fi

if test "$enable_pcregrep_libbz2" = "yes"; then
  AC_DEFINE([SUPPORT_LIBBZ2], [], [
    Define to any value to allow pcregrep to be linked with libbz2, so that it
    is able to handle .bz2 files.])
fi

if test $with_pcregrep_bufsize -lt 8192 ; then
  AC_MSG_WARN([$with_pcregrep_bufsize is too small for --with-pcregrep-bufsize; using 8192])
  with_pcregrep_bufsize="8192"
else
  if test $? -gt 1 ; then
  AC_MSG_ERROR([Bad value for  --with-pcregrep-bufsize])
  fi
fi

AC_DEFINE_UNQUOTED([PCREGREP_BUFSIZE], [$with_pcregrep_bufsize], [
  The value of PCREGREP_BUFSIZE determines the size of buffer used by pcregrep
  to hold parts of the file it is searching. This is also the minimum value.
  The actual amount of memory used by pcregrep is three times this number,
  because it allows for the buffering of "before" and "after" lines.])

if test "$enable_pcretest_libedit" = "yes"; then
  AC_DEFINE([SUPPORT_LIBEDIT], [], [
    Define to any value to allow pcretest to be linked with libedit.])
  LIBREADLINE="$LIBEDIT"
elif test "$enable_pcretest_libreadline" = "yes"; then
  AC_DEFINE([SUPPORT_LIBREADLINE], [], [
    Define to any value to allow pcretest to be linked with libreadline.])
fi

AC_DEFINE_UNQUOTED([NEWLINE], [$ac_pcre_newline_value], [
  The value of NEWLINE determines the default newline character sequence. PCRE
  client programs can override this by selecting other values at run time. In
  ASCII environments, the value can be 10 (LF), 13 (CR), or 3338 (CRLF); in
  EBCDIC environments the value can be 21 or 37 (LF), 13 (CR), or 3349 or 3365
  (CRLF) because there are two alternative codepoints (0x15 and 0x25) that are
  used as the NL line terminator that is equivalent to ASCII LF. In both ASCII
  and EBCDIC environments the value can also be -1 (ANY), or -2 (ANYCRLF).])

if test "$enable_bsr_anycrlf" = "yes"; then
  AC_DEFINE([BSR_ANYCRLF], [], [
    By default, the \R escape sequence matches any Unicode line ending
    character or sequence of characters. If BSR_ANYCRLF is defined (to any
    value), this is changed so that backslash-R matches only CR, LF, or CRLF.
    The build-time default can be overridden by the user of PCRE at runtime.])
fi

AC_DEFINE_UNQUOTED([LINK_SIZE], [$with_link_size], [
  The value of LINK_SIZE determines the number of bytes used to store
  links as offsets within the compiled regex. The default is 2, which
  allows for compiled patterns up to 64K long. This covers the vast
  majority of cases. However, PCRE can also be compiled to use 3 or 4
  bytes instead. This allows for longer patterns in extreme cases.])

AC_DEFINE_UNQUOTED([POSIX_MALLOC_THRESHOLD], [$with_posix_malloc_threshold], [
  When calling PCRE via the POSIX interface, additional working storage
  is required for holding the pointers to capturing substrings because
  PCRE requires three integers per substring, whereas the POSIX
  interface provides only two. If the number of expected substrings is
  small, the wrapper function uses space on the stack, because this is
  faster than using malloc() for each call. The threshold above which
  the stack is no longer used is defined by POSIX_MALLOC_THRESHOLD.])

AC_DEFINE_UNQUOTED([PARENS_NEST_LIMIT], [$with_parens_nest_limit], [
  The value of PARENS_NEST_LIMIT specifies the maximum depth of nested
  parentheses (of any kind) in a pattern. This limits the amount of system
  stack that is used while compiling a pattern.])

AC_DEFINE_UNQUOTED([MATCH_LIMIT], [$with_match_limit], [
  The value of MATCH_LIMIT determines the default number of times the
  internal match() function can be called during a single execution of
  pcre_exec(). There is a runtime interface for setting a different
  limit. The limit exists in order to catch runaway regular
  expressions that take for ever to determine that they do not match.
  The default is set very large so that it does not accidentally catch
  legitimate cases.])

AC_DEFINE_UNQUOTED([MATCH_LIMIT_RECURSION], [$with_match_limit_recursion], [
  The above limit applies to all calls of match(), whether or not they
  increase the recursion depth. In some environments it is desirable
  to limit the depth of recursive calls of match() more strictly, in
  order to restrict the maximum amount of stack (or heap, if
  NO_RECURSE is defined) that is used. The value of
  MATCH_LIMIT_RECURSION applies only to recursive calls of match(). To
  have any useful effect, it must be less than the value of
  MATCH_LIMIT. The default is to use the same value as MATCH_LIMIT.
  There is a runtime method for setting a different limit.])

AC_DEFINE([MAX_NAME_SIZE], [32], [
  This limit is parameterized just in case anybody ever wants to
  change it. Care must be taken if it is increased, because it guards
  against integer overflow caused by enormously large patterns.])

AC_DEFINE([MAX_NAME_COUNT], [10000], [
  This limit is parameterized just in case anybody ever wants to
  change it. Care must be taken if it is increased, because it guards
  against integer overflow caused by enormously large patterns.])

AH_VERBATIM([PCRE_EXP_DEFN], [
/* If you are compiling for a system other than a Unix-like system or
   Win32, and it needs some magic to be inserted before the definition
   of a function that is exported by the library, define this macro to
   contain the relevant magic. If you do not define this macro, a suitable
    __declspec value is used for Windows systems; in other environments
   "extern" is used for a C compiler and "extern C" for a C++ compiler.
   This macro apears at the start of every exported function that is part
   of the external API. It does not appear on functions that are "external"
   in the C sense, but which are internal to the library. */
#undef PCRE_EXP_DEFN])

if test "$enable_ebcdic" = "yes"; then
  AC_DEFINE_UNQUOTED([EBCDIC], [], [
    If you are compiling for a system that uses EBCDIC instead of ASCII
    character codes, define this macro to any value. You must also edit the
    NEWLINE macro below to set a suitable EBCDIC newline, commonly 21 (0x15).
    On systems that can use "configure" or CMake to set EBCDIC, NEWLINE is
    automatically adjusted. When EBCDIC is set, PCRE assumes that all input
    strings are in EBCDIC. If you do not define this macro, PCRE will assume
    input strings are ASCII or UTF-8/16/32 Unicode. It is not possible to build
    a version of PCRE that supports both EBCDIC and UTF-8/16/32.])
fi

if test "$enable_ebcdic_nl25" = "yes"; then
  AC_DEFINE_UNQUOTED([EBCDIC_NL25], [], [
    In an EBCDIC environment, define this macro to any value to arrange for
    the NL character to be 0x25 instead of the default 0x15. NL plays the role
    that LF does in an ASCII/Unicode environment. The value must also be set in
    the NEWLINE macro below. On systems that can use "configure" or CMake to
    set EBCDIC_NL25, the adjustment of NEWLINE is automatic.])
fi

if test "$enable_valgrind" = "yes"; then
  AC_DEFINE_UNQUOTED([SUPPORT_VALGRIND], [], [
     Define to any value for valgrind support to find invalid memory reads.])
fi

# Platform specific issues
NO_UNDEFINED=
EXPORT_ALL_SYMBOLS=
case $host_os in
  cygwin* | mingw* )
    if test X"$enable_shared" = Xyes; then
      NO_UNDEFINED="-no-undefined"
      EXPORT_ALL_SYMBOLS="-Wl,--export-all-symbols"
    fi
    ;;
esac

# The extra LDFLAGS for each particular library
# (Note: The libpcre*_version bits are m4 variables, assigned above)

EXTRA_LIBPCRE_LDFLAGS="$EXTRA_LIBPCRE_LDFLAGS \
                       $NO_UNDEFINED -version-info libpcre_version"

EXTRA_LIBPCRE16_LDFLAGS="$EXTRA_LIBPCRE16_LDFLAGS \
                       $NO_UNDEFINED -version-info libpcre16_version"

EXTRA_LIBPCRE32_LDFLAGS="$EXTRA_LIBPCRE32_LDFLAGS \
                       $NO_UNDEFINED -version-info libpcre32_version"

EXTRA_LIBPCREPOSIX_LDFLAGS="$EXTRA_LIBPCREPOSIX_LDFLAGS \
                            $NO_UNDEFINED -version-info libpcreposix_version"

EXTRA_LIBPCRECPP_LDFLAGS="$EXTRA_LIBPCRECPP_LDFLAGS \
                          $NO_UNDEFINED -version-info libpcrecpp_version \
                          $EXPORT_ALL_SYMBOLS"

AC_SUBST(EXTRA_LIBPCRE_LDFLAGS)
AC_SUBST(EXTRA_LIBPCRE16_LDFLAGS)
AC_SUBST(EXTRA_LIBPCRE32_LDFLAGS)
AC_SUBST(EXTRA_LIBPCREPOSIX_LDFLAGS)
AC_SUBST(EXTRA_LIBPCRECPP_LDFLAGS)

# When we run 'make distcheck', use these arguments. Turning off compiler
# optimization makes it run faster.
DISTCHECK_CONFIGURE_FLAGS="CFLAGS='' CXXFLAGS='' --enable-pcre16 --enable-pcre32 --enable-jit --enable-cpp --enable-unicode-properties"
AC_SUBST(DISTCHECK_CONFIGURE_FLAGS)

# Check that, if --enable-pcregrep-libz or --enable-pcregrep-libbz2 is
# specified, the relevant library is available.

if test "$enable_pcregrep_libz" = "yes"; then
  if test "$HAVE_ZLIB_H" != "1"; then
    echo "** Cannot --enable-pcregrep-libz because zlib.h was not found"
    exit 1
  fi
  if test "$HAVE_LIBZ" != "1"; then
    echo "** Cannot --enable-pcregrep-libz because libz was not found"
    exit 1
  fi
  LIBZ="-lz"
fi
AC_SUBST(LIBZ)

if test "$enable_pcregrep_libbz2" = "yes"; then
  if test "$HAVE_BZLIB_H" != "1"; then
    echo "** Cannot --enable-pcregrep-libbz2 because bzlib.h was not found"
    exit 1
  fi
  if test "$HAVE_LIBBZ2" != "1"; then
    echo "** Cannot --enable-pcregrep-libbz2 because libbz2 was not found"
    exit 1
  fi
  LIBBZ2="-lbz2"
fi
AC_SUBST(LIBBZ2)

# Similarly for --enable-pcretest-readline

if test "$enable_pcretest_libedit" = "yes"; then
  if test "$enable_pcretest_libreadline" = "yes"; then
    echo "** Cannot use both --enable-pcretest-libedit and --enable-pcretest-readline"
    exit 1
  fi
  if test "$HAVE_EDITLINE_READLINE_H" != "1" -a \
          "$HAVE_READLINE_READLINE_H" != "1"; then
    echo "** Cannot --enable-pcretest-libedit because neither editline/readline.h"
    echo "** nor readline/readline.h was found."
    exit 1
  fi
  if test -z "$LIBEDIT"; then
    echo "** Cannot --enable-pcretest-libedit because libedit library was not found."
    exit 1
  fi
fi

if test "$enable_pcretest_libreadline" = "yes"; then
  if test "$HAVE_READLINE_H" != "1"; then
    echo "** Cannot --enable-pcretest-readline because readline/readline.h was not found."
    exit 1
  fi
  if test "$HAVE_HISTORY_H" != "1"; then
    echo "** Cannot --enable-pcretest-readline because readline/history.h was not found."
    exit 1
  fi
  if test -z "$LIBREADLINE"; then
    echo "** Cannot --enable-pcretest-readline because readline library was not found."
    exit 1
  fi
fi

# Handle valgrind support

if test "$enable_valgrind" = "yes"; then
  m4_ifdef([PKG_CHECK_MODULES],
           [PKG_CHECK_MODULES([VALGRIND],[valgrind])],
           [AC_MSG_ERROR([pkg-config not supported])])
fi

# Handle code coverage reporting support
if test "$enable_coverage" = "yes"; then
  if test "x$GCC" != "xyes"; then
    AC_MSG_ERROR([Code coverage reports can only be generated when using GCC])
  fi

  # ccache is incompatible with gcov
  AC_PATH_PROG([SHTOOL],[shtool],[false])
  case `$SHTOOL path $CC` in
    *ccache*) cc_ccache=yes;;
    *) cc_ccache=no;;
  esac

  if test "$cc_ccache" = "yes"; then
    if test -z "$CCACHE_DISABLE" -o "$CCACHE_DISABLE" != "1"; then
      AC_MSG_ERROR([must export CCACHE_DISABLE=1 to disable ccache for code coverage])
    fi
  fi

  AC_ARG_VAR([LCOV],[the ltp lcov program])
  AC_PATH_PROG([LCOV],[lcov],[false])
  if test "x$LCOV" = "xfalse"; then
    AC_MSG_ERROR([lcov not found])
  fi

  AC_ARG_VAR([GENHTML],[the ltp genhtml program])
  AC_PATH_PROG([GENHTML],[genhtml],[false])
  if test "x$GENHTML" = "xfalse"; then
    AC_MSG_ERROR([genhtml not found])
  fi

  # Set flags needed for gcov
  GCOV_CFLAGS="-O0 -ggdb3 -fprofile-arcs -ftest-coverage"
  GCOV_CXXFLAGS="-O0 -ggdb3 -fprofile-arcs -ftest-coverage"
  GCOV_LIBS="-lgcov"
  AC_SUBST([GCOV_CFLAGS])
  AC_SUBST([GCOV_CXXFLAGS])
  AC_SUBST([GCOV_LIBS])
fi # enable_coverage

AM_CONDITIONAL([WITH_GCOV],[test "x$enable_coverage" = "xyes"])

# Produce these files, in addition to config.h.
AC_CONFIG_FILES(
	Makefile
	libpcre.pc
	libpcre16.pc
	libpcre32.pc
	libpcreposix.pc
	libpcrecpp.pc
	pcre-config
	pcre.h
	pcre_stringpiece.h
	pcrecpparg.h
)

# Make the generated script files executable.
AC_CONFIG_COMMANDS([script-chmod], [chmod a+x pcre-config])

# Make sure that pcre_chartables.c is removed in case the method for
# creating it was changed by reconfiguration.
AC_CONFIG_COMMANDS([delete-old-chartables], [rm -f pcre_chartables.c])

AC_OUTPUT

# Print out a nice little message after configure is run displaying the
# chosen options.

ebcdic_nl_code=n/a
if test "$enable_ebcdic_nl25" = "yes"; then
  ebcdic_nl_code=0x25
elif test "$enable_ebcdic" = "yes"; then
  ebcdic_nl_code=0x15
fi

cat <<EOF

$PACKAGE-$VERSION configuration summary:

    Install prefix .................. : ${prefix}
    C preprocessor .................. : ${CPP}
    C compiler ...................... : ${CC}
    C++ preprocessor ................ : ${CXXCPP}
    C++ compiler .................... : ${CXX}
    Linker .......................... : ${LD}
    C preprocessor flags ............ : ${CPPFLAGS}
    C compiler flags ................ : ${CFLAGS} ${VISIBILITY_CFLAGS}
    C++ compiler flags .............. : ${CXXFLAGS} ${VISIBILITY_CXXFLAGS}
    Linker flags .................... : ${LDFLAGS}
    Extra libraries ................. : ${LIBS}

    Build 8 bit pcre library ........ : ${enable_pcre8}
    Build 16 bit pcre library ....... : ${enable_pcre16}
    Build 32 bit pcre library ....... : ${enable_pcre32}
    Build C++ library ............... : ${enable_cpp}
    Enable JIT compiling support .... : ${enable_jit}
    Enable UTF-8/16/32 support ...... : ${enable_utf}
    Unicode properties .............. : ${enable_unicode_properties}
    Newline char/sequence ........... : ${enable_newline}
    \R matches only ANYCRLF ......... : ${enable_bsr_anycrlf}
    EBCDIC coding ................... : ${enable_ebcdic}
    EBCDIC code for NL .............. : ${ebcdic_nl_code}
    Rebuild char tables ............. : ${enable_rebuild_chartables}
    Use stack recursion ............. : ${enable_stack_for_recursion}
    POSIX mem threshold ............. : ${with_posix_malloc_threshold}
    Internal link size .............. : ${with_link_size}
    Nested parentheses limit ........ : ${with_parens_nest_limit}
    Match limit ..................... : ${with_match_limit}
    Match limit recursion ........... : ${with_match_limit_recursion}
    Build shared libs ............... : ${enable_shared}
    Build static libs ............... : ${enable_static}
    Use JIT in pcregrep ............. : ${enable_pcregrep_jit}
    Buffer size for pcregrep ........ : ${with_pcregrep_bufsize}
    Link pcregrep with libz ......... : ${enable_pcregrep_libz}
    Link pcregrep with libbz2 ....... : ${enable_pcregrep_libbz2}
    Link pcretest with libedit ...... : ${enable_pcretest_libedit}
    Link pcretest with libreadline .. : ${enable_pcretest_libreadline}
    Valgrind support ................ : ${enable_valgrind}
    Code coverage ................... : ${enable_coverage}

EOF

dnl end configure.ac
