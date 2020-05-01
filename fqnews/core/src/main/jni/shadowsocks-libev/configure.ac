dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([shadowsocks-libev], [3.3.4], [max.c.lv@gmail.com])
AC_CONFIG_SRCDIR([src/crypto.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR(auto)
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([subdir-objects foreign -Wno-gnu -Werror])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_MAINTAINER_MODE
AM_DEP_TRACK

dnl Checks for lib
AC_DISABLE_STATIC
AC_DISABLE_SHARED
LT_INIT([dlopen])

dnl Check for pcre library
TS_CHECK_PCRE
if test "x${enable_pcre}" != "xyes"; then
  AC_MSG_ERROR([Cannot find pcre library. Configure --with-pcre=DIR])
fi

dnl Checks for using shared libraries from system
AC_ARG_ENABLE(
  [system-shared-lib],
  AS_HELP_STRING([--enable-system-shared-lib], [build against shared libraries when possible]),
  [
    case "${enableval}" in
      yes) enable_system_shared_lib=true ;;
      no) enable_system_shared_lib=false ;;
      *) AC_MSG_ERROR([bad value ${enableval} for --enable-system-shared-lib]) ;;
    esac], [enable_system_shared_lib=false])
AM_CONDITIONAL([USE_SYSTEM_SHARED_LIB], [test x$enable_system_shared_lib = xtrue])

AC_ARG_ENABLE([documentation],
  AS_HELP_STRING([--disable-documentation], [do not build documentation]),
  [disable_documentation=true],
  [disable_documentation=false])
AM_CONDITIONAL([ENABLE_DOCUMENTATION], [test x$disable_documentation = xfalse])

AM_COND_IF([ENABLE_DOCUMENTATION], [
  AC_PATH_PROG([ASCIIDOC], [asciidoc])
  test x"${ASCIIDOC}" != x || AC_MSG_ERROR([Cannot find `asciidoc` in PATH.])
  AC_PATH_PROG([XMLTO], [xmlto])
  test x"$XMLTO" != x || AC_MSG_ERROR([Cannot find `xmlto` in PATH.])
  AC_PATH_PROG([GZIP], [gzip], [gzip])
  AC_PATH_PROG([MV], [mv], [mv])
  AC_PROG_SED
])

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LANG_SOURCE

dnl Add library for mingw
case $host in
  *-mingw*)
    CFLAGS="$CFLAGS -mno-ms-bitfields"
    LIBS="$LIBS -lws2_32"
    ;;
  *-cygwin*)
    CFLAGS="$CFLAGS -mno-ms-bitfields"
    ;;
  *)
    ;;
esac

dnl Checks for TLS
AX_TLS([:], [:])

dnl Checks for crypto libraries
ss_MBEDTLS
ss_SODIUM
ss_CARES

dnl Checks for inet_ntop
ss_FUNC_INET_NTOP

dnl Checks for host.
AC_MSG_CHECKING(for what kind of host)
case $host in
  *-linux*)
    os_support=linux
    AC_MSG_RESULT(Linux)
    ;;
  *-mingw*)
    os_support=mingw
    AC_MSG_RESULT(MinGW)
    ;;
  *)
    AC_MSG_RESULT(transparent proxy does not support for $host)
    ;;
esac

dnl Checks for pthread
AX_PTHREAD([LIBS="$PTHREAD_LIBS $LIBS"
            CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
            CC="$PTHREAD_CC"], AC_MSG_ERROR(Can not find pthreads.  This is required.))

dnl Checks for stack protector
GGL_CHECK_STACK_PROTECTOR([has_stack_protector=yes], [has_stack_protector=no])
# XXX - disable -fstack-protector due to missing libssp_nonshared
case "$host_os" in
     *aix*)
	AC_MSG_NOTICE([-fstack-protector disabled on AIX])
	has_stack_protector=no
	;;
     *sunos*)
	AC_MSG_NOTICE([-fstack-protector disabled on SunOS])
	has_stack_protector=no
	;;
     *solaris*)
	AC_MSG_NOTICE([-fstack-protector disabled on Solaris])
	has_stack_protector=no
	;;
esac

AC_ARG_ENABLE(ssp,
[AS_HELP_STRING(--disable-ssp,Do not compile with -fstack-protector)],
[
  enable_ssp="no"
],
[
  enable_ssp="yes"
])

if test x$has_stack_protector = xyes && test x$enable_ssp = xyes; then
   CFLAGS="$CFLAGS -fstack-protector"
   AC_MSG_NOTICE([-fstack-protector enabled in CFLAGS])
fi

AM_CONDITIONAL(BUILD_REDIRECTOR, test "$os_support" = "linux")
AM_CONDITIONAL(BUILD_WINCOMPAT, test "$os_support" = "mingw")

dnl Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h inttypes.h arpa/inet.h fcntl.h langinfo.h locale.h linux/tcp.h netinet/tcp.h netdb.h netinet/in.h stdlib.h string.h strings.h unistd.h sys/ioctl.h linux/random.h])

dnl A special check required for <net/if.h> on Darwin. See
dnl http://www.gnu.org/software/autoconf/manual/html_node/Header-Portability.html.
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([net/if.h], [], [],
[
#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
])

case $host in
  *-mingw*)
    AC_DEFINE([CONNECT_IN_PROGRESS], [WSAEWOULDBLOCK], [errno for incomplete non-blocking connect(2)])
    AC_CHECK_HEADERS([winsock2.h ws2tcpip.h mswsock.h], [], [AC_MSG_ERROR([Missing MinGW headers])], [])
    ;;
  *-linux*)
    AC_DEFINE([CONNECT_IN_PROGRESS], [EINPROGRESS], [errno for incomplete non-blocking connect(2)])
    dnl Checks for netfilter headers
    AC_CHECK_HEADERS([linux/if.h linux/netfilter_ipv4.h linux/netfilter_ipv6/ip6_tables.h],
                     [], [AC_MSG_ERROR([Missing netfilter headers])],
    [[
    #if HAVE_LIMITS_H
    #include <limits.h>
    #endif
    /* Netfilter ip(6)tables v1.4.0 has broken headers */
    #if HAVE_NETINET_IN_H
    #include <netinet/in.h>
    #endif
    #if HAVE_LINUX_IF_H
    #include <linux/if.h>
    #endif
    #if HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
    #endif
    ]])
    ;;
  *)
    # These are POSIX-like systems using BSD-like sockets API.
    AC_DEFINE([CONNECT_IN_PROGRESS], [EINPROGRESS], [errno for incomplete non-blocking connect(2)])
    ;;
esac

AC_C_BIGENDIAN

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SSIZE_T

dnl Checks for header files.
AC_HEADER_ASSERT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_FUNC_FORK
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memset select setresuid setreuid strerror get_current_dir_name getpwnam_r setrlimit])

AC_CHECK_LIB(socket, connect)

dnl Checks for library functions.
AC_CHECK_FUNCS([malloc memset posix_memalign socket])

AC_ARG_WITH(ev,
  AS_HELP_STRING([--with-ev=DIR], [use a specific libev library]),
  [CFLAGS="$CFLAGS -I$withval/include"
   CPPFLAGS="$CPPFLAGS -I$withval/include"
   LDFLAGS="$LDFLAGS -L$withval/lib"]
)
AC_CHECK_HEADERS([ev.h libev/ev.h], [], [])
AC_CHECK_LIB([ev], [ev_loop_destroy], [LIBS="-lev $LIBS"], [AC_MSG_ERROR([Couldn't find libev. Try installing libev-dev@<:@el@:>@.])])

AC_CONFIG_FILES([shadowsocks-libev.pc
                 Makefile
                 doc/Makefile
                 src/Makefile])

AM_COND_IF([USE_SYSTEM_SHARED_LIB],
		   [AC_DEFINE([USE_SYSTEM_SHARED_LIB], [1], [Define if use system shared lib.])],
		   [AC_CONFIG_FILES([libbloom/Makefile libcork/Makefile libipset/Makefile])])

AC_ARG_ENABLE(connmarktos,
[AS_HELP_STRING(--enable-connmarktos, Enable saved connmark to IP TOS QoS feature)],
[ enable_connmarktos="$enableval" ])

if test x"$enable_connmarktos" = "xyes" ; then
	AC_MSG_NOTICE([Linux Netfilter Conntrack support requested by --enable-connmarktos: ${enable_connmarktos}])
	if test "x$enable_connmarktos" != "xno"; then
	    AC_SEARCH_LIBS([nfct_query], [netfilter_conntrack],,[
	        if test x"$enable_connmarktos" = "xyes"; then
	            AC_MSG_ERROR([--enable-connmarktos specified but libnetfilter-conntrack library not found])
	        fi
	        with_netfilter_conntrack=no], [-lnfnetlink])
	    AC_CHECK_HEADERS([libnetfilter_conntrack/libnetfilter_conntrack.h \
	        libnetfilter_conntrack/libnetfilter_conntrack_tcp.h],,[
	        if test x"$enable_connmarktos" = "xyes"; then
	            AC_MSG_ERROR([--enable-connmarktos specified but libnetfilter-conntrack headers not found])
	        fi
	        with_netfilter_conntrack=no])
	  # If nothing is broken; enable the libraries usage.
	  if test "x$with_netfilter_conntrack" != "xno"; then
		with_netfilter_conntrack=yes
		AC_DEFINE(USE_NFCONNTRACK_TOS, 1, [Enable support for QOS netfilter mark preservation])
	  fi
	fi
fi

AC_OUTPUT
