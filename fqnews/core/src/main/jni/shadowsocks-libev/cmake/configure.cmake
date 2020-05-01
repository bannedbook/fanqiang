
# -------------------------------------------------------------
# config.h

# If we generate config.h by automake
#include_directories(.)

# Use cmake to generate config.h
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)

# Define if building universal (internal helper macro)
# AC_APPLE_UNIVERSAL_BUILD
set(CONNECT_IN_PROGRESS "EINPROGRESS")
set(CONNECT_IN_PROGRESS "EINPROGRESS" CACHE STRING "")

if (CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(CMAKE_REQUIRED_INCLUDES "/usr/local/include" "/usr/include")
endif ()

check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(ev.h HAVE_EV_H)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_function_exists(fork HAVE_FORK)
check_function_exists(getpwnam_r HAVE_GETPWNAM_R)
check_function_exists(inet_ntop HAVE_INET_NTOP)
check_include_files(inttypes.h HAVE_INTTYPES_H)
set(HAVE_IPv6 1)
check_include_files(langinfo.h HAVE_LANGINFO_H)
set(HAVE_LIBPCRE 1)
check_library_exists(socket socket "" HAVE_LIBSOCKET)
check_include_files(limits.h HAVE_LIMITS_H)
check_include_files(linux/if.h HAVE_LINUX_IF_H)
check_include_files(linux/netfilter_ipv4.h HAVE_LINUX_NETFILTER_IPV4_H)
check_include_files(linux/netfilter_ipv6/ip6_tables.h HAVE_LINUX_NETFILTER_IPV6_IP6_TABLES_H)
check_include_files(locale.h HAVE_LOCALE_H)


check_function_exists(malloc HAVE_MALLOC)
check_include_files(memory.h HAVE_MEMORY_H)
check_function_exists(memset HAVE_MEMSET)

check_include_files(netdb.h HAVE_NETDB_H)
check_include_files(netinet/in.h HAVE_NETINET_IN_H)
check_include_files(netinet/tcp.h HAVE_NETINET_TCP_H)
check_include_files(linux/tcp.h HAVE_LINUX_TCP_H)
check_include_files(net/if.h HAVE_NET_IF_H)
check_include_files(pcre.h HAVE_PCRE_H)
check_include_files(pcre/pcre.h HAVE_PCRE_PCRE_H)
check_symbol_exists(PTHREAD_PRIO_INHERIT pthread.h HAVE_PTHREAD_PRIO_INHERIT)

check_function_exists(select HAVE_SELECT)
check_function_exists(setresuid HAVE_SETRESUID)
check_function_exists(setreuid HAVE_SETREUID)
check_function_exists(setrlimit HAVE_SETRLIMIT)
check_function_exists(socket HAVE_SOCKET)

check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdlib.h HAVE_STDLIB_H)

check_function_exists(strerror HAVE_STRERROR)

check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_files(sys/select.h HAVE_SYS_SELECT_H)
check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(sys/wait.h HAVE_SYS_WAIT_H)
check_include_files(ares.h HAVE_ARES_H)
check_include_files(unistd.h HAVE_UNISTD_H)

check_function_exists(fork HAVE_FORK)
check_function_exists(vfork HAVE_VFORK)
check_include_files(vfork.h HAVE_VFORK_H)
if (HAVE_VFORK)
    set(HAVE_WORKING_VFORK 1)
endif ()
if (HAVE_FORK)
    set(HAVE_WORKING_FORK 1)
endif ()


# Define to the sub-directory where libtool stores uninstalled libraries.
set(LT_OBJDIR ".libs/")
set(NDEBUG 1)
set(PACKAGE ${PROJECT_NAME})
set(PACKAGE_BUGREPORT max.c.lv@gmail.com)
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(PACKAGE_STRING "${PROJECT_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME ${PROJECT_NAME})
set(PACKAGE_URL "")

#message(${PACKAGE_NAME} - v${PACKAGE_VERSION} - v${PROJECT_VERSION})

# PTHREAD_CREATE_JOINABLE

# Define as the return type of signal handlers (`int' or `void').
set(RETSIGTYPE void)

# Define to the type of arg 1 for `select'.
set(SELECT_TYPE_ARG1 int)

# Define to the type of args 2, 3 and 4 for `select'.
set(SELECT_TYPE_ARG234 "(fd_set *)")

# Define to the type of arg 5 for `select'.
set(SELECT_TYPE_ARG5 "(struct timeval *)")

# Define to 1 if you have the ANSI C header files.
set(STDC_HEADERS 1)


check_include_files("sys/time.h;time.h" TIME_WITH_SYS_TIME)


# If the compiler supports a TLS storage class define it to that here
check_c_source_compiles("
        __thread int tls;
        int main(void) { return 0; }"
        HAVE_GCC_THREAD_LOCAL_STORAGE)
if (HAVE_GCC_THREAD_LOCAL_STORAGE)
    set(TLS __thread)
endif ()

set(_ALL_SOURCE 1)
set(_GNU_SOURCE 1)
set(_POSIX_PTHREAD_SEMANTICS 1)
set(_TANDEM_SOURCE 1)
set(__EXTENSIONS__ 1)
# USE_SYSTEM_SHARED_LIB
set(VERSION ${PACKAGE_VERSION})
# TODO WORDS_BIGENDIAN
# _MINIX
# _POSIX_1_SOURCE
# _POSIX_SOURCE
# _UINT8_T

# Define to empty if `const' does not conform to ANSI C.
# undef const

# Define to `__inline__' or `__inline' if that's what the C compiler
# calls it, or to nothing if 'inline' is not supported under any name.
#ifndef __cplusplus
#undef inline
#endif
# TODO Assume we got inline support
# https://cmake.org/Wiki/CMakeTestInline

# Define to `int' if <sys/types.h> does not define.
# undef pid_t
# Define to the type of an unsigned integer type of width exactly 16 bits if
# such a type exists and the standard includes do not define it.
# undef uint16_t
# Define to the type of an unsigned integer type of width exactly 8 bits if
# such a type exists and the standard includes do not define it.
# undef uint8_t
set(CMAKE_EXTRA_INCLUDE_FILES sys/types.h)
check_type_size(pid_t PID_T)
check_type_size(size_t SIZE_T)
check_type_size(ssize_t SSIZE_T)
set(CMAKE_EXTRA_INCLUDE_FILES)

check_type_size(uint16_t UINT16_T)
check_type_size(uint8_t UINT8_T)

## Inverse
if (NOT HAVE_PID_T)
    set(pid_t int)
endif ()
if (NOT HAVE_SIZE_T)
    set(size_t "unsigned int")
endif ()
if (NOT HAVE_SSIZE_T)
    set(ssize_t int)
endif ()

if (NOT HAVE_UINT8_T)
    set(uint8_t "unsigned char")
endif ()
if (NOT HAVE_UINT16_T)
    set(uint16_t "unsigned short")
endif ()


# Define as `fork' if `vfork' does not work.
if (NOT HAVE_WORKING_VFORK)
    set(vfork fork)
endif ()
