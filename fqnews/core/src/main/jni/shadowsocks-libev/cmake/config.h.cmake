#ifndef _SHADOWSOCKS_CONFIG_H
#define _SHADOWSOCKS_CONFIG_H

/* Define if building universal (internal helper macro) */
#cmakedefine AC_APPLE_UNIVERSAL_BUILD

/* errno for incomplete non-blocking connect(2) */
#cmakedefine CONNECT_IN_PROGRESS @CONNECT_IN_PROGRESS@

/* Define to 1 if you have the <arpa/inet.h> header file. */
#cmakedefine HAVE_ARPA_INET_H 1

/* Define to 1 if you have the declaration of `inet_ntop', and to 0 if you
   don't. */
#cmakedefine HAVE_DECL_INET_NTOP 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <ev.h> header file. */
#cmakedefine HAVE_EV_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the `fork' function. */
#cmakedefine HAVE_FORK 1

/* Define to 1 if you have the `getpwnam_r' function. */
#cmakedefine HAVE_GETPWNAM_R 1

/* Define to 1 if you have the `inet_ntop' function. */
#cmakedefine HAVE_INET_NTOP 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Enable IPv6 support in libudns */
#cmakedefine HAVE_IPv6 1

/* Define to 1 if you have the <langinfo.h> header file. */
#cmakedefine HAVE_LANGINFO_H 1

/* Compiling with pcre support */
#cmakedefine HAVE_LIBPCRE 1

/* Define to 1 if you have the `socket' library (-lsocket). */
#cmakedefine HAVE_LIBSOCKET 1

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine HAVE_LIMITS_H 1

/* Define to 1 if you have the <linux/if.h> header file. */
#cmakedefine HAVE_LINUX_IF_H 1

/* Define to 1 if you have the <linux/tcp.h> header file. */
#cmakedefine HAVE_LINUX_TCP_H

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#cmakedefine HAVE_NETINET_TCP_H

/* Define to 1 if you have the <linux/netfilter_ipv4.h> header file. */
#cmakedefine HAVE_LINUX_NETFILTER_IPV4_H 1

/* Define to 1 if you have the <linux/netfilter_ipv6/ip6_tables.h> header
   file. */
#cmakedefine HAVE_LINUX_NETFILTER_IPV6_IP6_TABLES_H 1

/* Define to 1 if you have the <locale.h> header file. */
#cmakedefine HAVE_LOCALE_H 1

/* Define to 1 if you have the `malloc' function. */
#cmakedefine HAVE_MALLOC 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#cmakedefine HAVE_MEMSET 1

/* Define to 1 if you have the <netdb.h> header file. */
#cmakedefine HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#cmakedefine HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <net/if.h> header file. */
#cmakedefine HAVE_NET_IF_H 1

/* Define to 1 if you have the <pcre.h> header file. */
#cmakedefine HAVE_PCRE_H 1

/* Define to 1 if you have the <pcre/pcre.h> header file. */
#cmakedefine HAVE_PCRE_PCRE_H 1

/* Have PTHREAD_PRIO_INHERIT. */
#cmakedefine HAVE_PTHREAD_PRIO_INHERIT 1

/* Define to 1 if you have the `select' function. */
#cmakedefine HAVE_SELECT 1

/* Define to 1 if you have the `setresuid' function. */
#cmakedefine HAVE_SETRESUID 1

/* Define to 1 if you have the `setreuid' function. */
#cmakedefine HAVE_SETREUID 1

/* Define to 1 if you have the `setrlimit' function. */
#cmakedefine HAVE_SETRLIMIT 1

/* Define to 1 if you have the `socket' function. */
#cmakedefine HAVE_SOCKET 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the `strerror' function. */
#cmakedefine HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#cmakedefine HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#cmakedefine HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#cmakedefine HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#cmakedefine HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <udns.h> header file. */
#cmakedefine HAVE_UDNS_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if you have the `vfork' function. */
#cmakedefine HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
#cmakedefine HAVE_VFORK_H 1

/* Define to 1 if `fork' works. */
#cmakedefine HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#cmakedefine HAVE_WORKING_VFORK 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#cmakedefine LT_OBJDIR "@LT_OBJDIR@"

/* Define to 1 if assertions should be disabled. */
#cmakedefine NDEBUG 1

/* Name of package */
#define PACKAGE "@PROJECT_NAME@"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"

/* Define to the full name of this package. */
#define PACKAGE_NAME "@PROJECT_NAME@"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "@PROJECT_NAME@ @PROJECT_VERSION@"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "@PROJECT_NAME@"

/* Define to the home page for this package. */
#define PACKAGE_URL "@PACKAGE_URL@"

/* Define to the version of this package. */
#define PACKAGE_VERSION "@PROJECT_VERSION@"

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
#cmakedefine PTHREAD_CREATE_JOINABLE 1

/* Define as the return type of signal handlers (`int' or `void'). */
#cmakedefine RETSIGTYPE @RETSIGTYPE@

/* Define to the type of arg 1 for `select'. */
#cmakedefine SELECT_TYPE_ARG1 @SELECT_TYPE_ARG1@

/* Define to the type of args 2, 3 and 4 for `select'. */
#cmakedefine SELECT_TYPE_ARG234 @SELECT_TYPE_ARG234@

/* Define to the type of arg 5 for `select'. */
#cmakedefine SELECT_TYPE_ARG5 @SELECT_TYPE_ARG5@

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#cmakedefine TIME_WITH_SYS_TIME 1

/* If the compiler supports a TLS storage class define it to that here */
#cmakedefine TLS @TLS@

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
#cmakedefine _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
#cmakedefine _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
#cmakedefine _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
#cmakedefine _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
#cmakedefine __EXTENSIONS__ 1
#endif


/* Define if use system shared lib. */
#cmakedefine USE_SYSTEM_SHARED_LIB 1

/* Version number of package */
#define VERSION "@PROJECT_VERSION@"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#cmakedefine WORDS_BIGENDIAN 1
# endif
#endif

/* Define to 1 if on MINIX. */
#cmakedefine _MINIX 1

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
#cmakedefine _POSIX_1_SOURCE 1

/* Define to 1 if you need to in order for `stat' and other things to work. */
#cmakedefine _POSIX_SOURCE 1

/* Define for Solaris 2.5.1 so the uint8_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
#cmakedefine _UINT8_T 1

/* Define to empty if `const' does not conform to ANSI C. */
#cmakedefine const 1

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#cmakedefine inline 1
#endif

/* Define to `int' if <sys/types.h> does not define. */
#cmakedefine pid_t @pid_t@

/* Define to `unsigned int' if <sys/types.h> does not define. */
#cmakedefine size_t unsigned int

/* Define to `int' if <sys/types.h> does not define. */
#cmakedefine ssize_t int

/* Define to the type of an unsigned integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
#cmakedefine uint16_t @uint16_t@

/* Define to the type of an unsigned integer type of width exactly 8 bits if
   such a type exists and the standard includes do not define it. */
#cmakedefine uint8_t @uint8_t@

/* Define as `fork' if `vfork' does not work. */
#cmakedefine vfork

#endif
