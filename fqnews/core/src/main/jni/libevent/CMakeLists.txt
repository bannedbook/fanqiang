#
# Libevent CMake project
#
# Based on initial work by:
#    Alexey Ozeritsky
#
# Additional changes:
#   Brodie Thiesfield
#   Joakim Soderberg
#   Trond Norbye
#   Sergei Nikulov
#
#   Build example:
#
#       cd libevent
#       md build
#       cd build
#       cmake -G "Visual Studio 10" ..
#       start libevent.sln
#
if (WIN32)
    cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
else()
    cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release
        CACHE STRING "Set build type to Debug o Release (default Release)" FORCE)
endif()
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

# get rid of the extra default configurations
# what? why would you get id of other useful build types? - Ellzey
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Limited configurations" FORCE)

project(libevent C)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")
string(REGEX MATCH "SunOS" SOLARIS "${CMAKE_SYSTEM_NAME}")


include(CheckTypeSize)
include(CheckFunctionExistsEx)
include(CheckFileOffsetBits)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckVariableExists)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckCSourceCompiles)
include(CheckPrototypeDefinition)
include(CheckFunctionKeywords)
include(AddCompilerFlags)
include(VersionViaGit)

event_fuzzy_version_from_git()

set(EVENT_VERSION_MAJOR ${EVENT_GIT___VERSION_MAJOR})
set(EVENT_VERSION_MINOR ${EVENT_GIT___VERSION_MINOR})
set(EVENT_VERSION_PATCH ${EVENT_GIT___VERSION_PATCH})
set(EVENT_VERSION_STAGE ${EVENT_GIT___VERSION_STAGE})


set(EVENT_ABI_MAJOR ${EVENT_VERSION_MAJOR})
set(EVENT_ABI_MINOR ${EVENT_VERSION_MINOR})
set(EVENT_ABI_PATCH ${EVENT_VERSION_PATCH})

set(EVENT_ABI_LIBVERSION
    "${EVENT_ABI_MAJOR}.${EVENT_ABI_MINOR}.${EVENT_ABI_PATCH}")

set(EVENT_PACKAGE_VERSION
    "${EVENT_VERSION_MAJOR}.${EVENT_VERSION_MINOR}.${EVENT_VERSION_PATCH}")

set(EVENT_NUMERIC_VERSION 0x02010800)

# only a subset of names can be used, defaults to "beta"
set(EVENT_STAGE_NAME ${EVENT_VERSION_STAGE}
	CACHE STRING "set the stage name (beta|alpha|release)")

# a list that defines what can set for EVENT_STAGE_VERSION
set(EVENT__ALLOWED_STAGE_NAMES
    rc
    beta
    alpha
    release)

# attempt to find the EVENT__STAGE_VERSION in the allowed list
# of accepted stage names, the return value is stord in
# EVENT__STAGE_RET

list(FIND EVENT__ALLOWED_STAGE_NAMES
	${EVENT_STAGE_NAME}
	EVENT__STAGE_RET)

if (EVENT__STAGE_RET EQUAL "-1")
	set(EVENT_STAGE_NAME beta)
endif()

set(EVENT_VERSION
	"${EVENT_VERSION_MAJOR}.${EVENT_VERSION_MINOR}.${EVENT_VERSION_PATCH}-${EVENT_STAGE_NAME}")

option(EVENT__BUILD_SHARED_LIBRARIES
    "Define if libevent should be built with shared libraries instead of archives" OFF)

option(EVENT__DISABLE_DEBUG_MODE
    "Define if libevent should build without support for a debug mode" OFF)

option(EVENT__ENABLE_VERBOSE_DEBUG
    "Enables verbose debugging" OFF)

option(EVENT__DISABLE_MM_REPLACEMENT
    "Define if libevent should not allow replacing the mm functions" OFF)

option(EVENT__DISABLE_THREAD_SUPPORT
    "Define if libevent should not be compiled with thread support" OFF)

option(EVENT__DISABLE_OPENSSL
    "Define if libevent should build without support for OpenSSL encrpytion" OFF)

option(EVENT__DISABLE_BENCHMARK
    "Defines if libevent should build without the benchmark exectuables" OFF)

option(EVENT__DISABLE_TESTS
    "If tests should be compiled or not" OFF)

option(EVENT__DISABLE_REGRESS
    "Disable the regress tests" OFF)

option(EVENT__DISABLE_SAMPLES
    "Disable sample files" OFF)

option(EVENT__DISABLE_CLOCK_GETTIME
    "Do not use clock_gettime even if it is available" OFF)

option(EVENT__FORCE_KQUEUE_CHECK
    "When crosscompiling forces running a test program that verifies that Kqueue works with pipes. Note that this requires you to manually run the test program on the the cross compilation target to verify that it works. See cmake documentation for try_run for more details" OFF)

# TODO: Add --disable-largefile     omit support for large files
option(EVENT__COVERAGE
"Enable running gcov to get a test coverage report (only works with GCC/CLang). Make sure to enable -DCMAKE_BUILD_TYPE=Debug as well." OFF)

# Put the libaries and binaries that get built into directories at the
# top of the build tree rather than in hard-to-find leaf directories.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

if (EVENT__ENABLE_VERBOSE_DEBUG)
    add_definitions(-DUSE_DEBUG=1)
endif()

# Setup compiler flags for coverage.
if (EVENT__COVERAGE)
    if (NOT "${CMAKE_BUILD_TYPE_LOWER}" STREQUAL "debug")
        message(FATAL_ERROR "Coverage requires -DCMAKE_BUILD_TYPE=Debug")
    endif()

    message(STATUS "Setting coverage compiler flags")

    set(CMAKE_REQUIRED_LIBRARIES "--coverage")
    add_compiler_flags(-g -O0 --coverage)
    set(CMAKE_REQUIRED_LIBRARIES "")
endif()

# GCC specific options.
if (CMAKE_COMPILER_IS_GNUCC)
    option(EVENT__DISABLE_GCC_WARNINGS "Disable verbose warnings with GCC" OFF)
    option(EVENT__ENABLE_GCC_HARDENING "Enable compiler security checks" OFF)
    option(EVENT__ENABLE_GCC_FUNCTION_SECTIONS "Enable gcc function sections" OFF)
    option(EVENT__ENABLE_GCC_WARNINGS "Make all GCC warnings into errors" OFF)

    list(APPEND __FLAGS -Wall -Wswitch)

    if (EVENT__DISABLE_GCC_WARNINGS)
        list(APPEND __FLAGS -w)
    endif()

    if (EVENT__ENABLE_GCC_HARDENING)
        list(APPEND __FLAGS
             -fstack-protector-all
             -fwrapv
             -fPIE
             -Wstack-protector
             "--param ssp-buffer-size=1")

        add_definitions(-D_FORTIFY_SOURCE=2)
    endif()

    if (EVENT__ENABLE_GCC_FUNCTION_SECTIONS)
        list(APPEND __FLAGS -ffunction-sections)
        # TODO: Add --gc-sections support. We need some checks for NetBSD to ensure this works.
    endif()

    if (EVENT__ENABLE_GCC_WARNINGS)
        list(APPEND __FLAGS -Werror)
    endif()

    # We need to test for at least gcc 2.95 here, because older versions don't
    # have -fno-strict-aliasing
    list(APPEND __FLAGS -fno-strict-aliasing)

    add_compiler_flags(${__FLAGS})
endif()

if (APPLE)
    # Get rid of deprecated warnings for OpenSSL on OSX 10.7 and greater.
    add_compiler_flags(
        -Wno-error=deprecated-declarations
        -Qunused-arguments
    )
endif()

# Winsock.
if(WIN32)
    set(CMAKE_EXTRA_INCLUDE_FILES winsock2.h ws2tcpip.h)
    set(CMAKE_REQUIRED_LIBRARIES  ws2_32.lib)
    set(CMAKE_REQUIRED_DEFINITIONS -FIwinsock2.h -FIws2tcpip.h)
endif()
if (SOLARIS)
    set(CMAKE_REQUIRED_LIBRARIES socket nsl)
endif()

# Check if _GNU_SOURCE is available.
CHECK_SYMBOL_EXISTS(__GNU_LIBRARY__ "features.h" _GNU_SOURCE)

if (_GNU_SOURCE)
    add_definitions(-D_GNU_SOURCE)
endif()

CHECK_INCLUDE_FILE(sys/types.h EVENT__HAVE_SYS_TYPES_H)
if(EVENT__HAVE_SYS_TYPES_H)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES sys/types.h)
endif()

CHECK_INCLUDE_FILE(sys/socket.h EVENT__HAVE_SYS_SOCKET_H)
if(EVENT__HAVE_SYS_SOCKET_H)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
endif()

CHECK_INCLUDE_FILE(netinet/in.h EVENT__HAVE_NETINET_IN_H)
if(EVENT__HAVE_NETINET_IN_H)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES netinet/in.h)
endif()

CHECK_INCLUDE_FILE(netinet/in6.h EVENT__HAVE_NETINET_IN6_H)
if(EVENT__HAVE_NETINET_IN6_H)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES netinet/in6.h)
endif()

CHECK_INCLUDE_FILE(unistd.h EVENT__HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(netdb.h EVENT__HAVE_NETDB_H)
CHECK_INCLUDE_FILE(dlfcn.h EVENT__HAVE_DLFCN_H)
CHECK_INCLUDE_FILE(arpa/inet.h EVENT__HAVE_ARPA_INET_H)
CHECK_INCLUDE_FILE(fcntl.h EVENT__HAVE_FCNTL_H)
if(EVENT__HAVE_FCNTL_H)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES fcntl.h)
endif()
CHECK_INCLUDE_FILE(inttypes.h EVENT__HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE(memory.h EVENT__HAVE_MEMORY_H)
CHECK_INCLUDE_FILE(poll.h EVENT__HAVE_POLL_H)
CHECK_INCLUDE_FILE(port.h EVENT__HAVE_PORT_H)
if(EVENT__HAVE_PORT_H)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES port.h)
endif()
CHECK_INCLUDE_FILE(signal.h EVENT__HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE(stdarg.h EVENT__HAVE_STDARG_H)
CHECK_INCLUDE_FILE(stddef.h EVENT__HAVE_STDDEF_H)
CHECK_INCLUDE_FILE(stdint.h EVENT__HAVE_STDINT_H)
CHECK_INCLUDE_FILE(stdlib.h EVENT__HAVE_STDLIB_H)
CHECK_INCLUDE_FILE(strings.h EVENT__HAVE_STRINGS_H)
CHECK_INCLUDE_FILE(string.h EVENT__HAVE_STRING_H)
CHECK_INCLUDE_FILE(sys/devpoll.h EVENT__HAVE_SYS_DEVPOLL_H)
CHECK_INCLUDE_FILE(sys/epoll.h EVENT__HAVE_SYS_EPOLL_H)
CHECK_INCLUDE_FILE(sys/eventfd.h EVENT__HAVE_SYS_EVENTFD_H)
CHECK_INCLUDE_FILE(sys/event.h EVENT__HAVE_SYS_EVENT_H)
CHECK_INCLUDE_FILE(sys/ioctl.h EVENT__HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILE(sys/mman.h EVENT__HAVE_SYS_MMAN_H)
CHECK_INCLUDE_FILE(sys/param.h EVENT__HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILE(sys/queue.h EVENT__HAVE_SYS_QUEUE_H)
CHECK_INCLUDE_FILE(sys/select.h EVENT__HAVE_SYS_SELECT_H)
CHECK_INCLUDE_FILE(sys/sendfile.h EVENT__HAVE_SYS_SENDFILE_H)
CHECK_INCLUDE_FILE(sys/stat.h EVENT__HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE(sys/time.h EVENT__HAVE_SYS_TIME_H)
if(EVENT__HAVE_SYS_TIME_H)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES sys/time.h)
endif()
CHECK_INCLUDE_FILE(sys/uio.h EVENT__HAVE_SYS_UIO_H)
CHECK_INCLUDE_FILES("sys/types.h;ifaddrs.h" EVENT__HAVE_IFADDRS_H)
CHECK_INCLUDE_FILE(mach/mach_time.h EVENT__HAVE_MACH_MACH_TIME_H)
CHECK_INCLUDE_FILE(netinet/tcp.h EVENT__HAVE_NETINET_TCP_H)
CHECK_INCLUDE_FILE(sys/wait.h EVENT__HAVE_SYS_WAIT_H)
CHECK_INCLUDE_FILE(sys/resource.h EVENT__HAVE_SYS_RESOURCE_H)
CHECK_INCLUDE_FILE(sys/sysctl.h EVENT__HAVE_SYS_SYSCTL_H)
CHECK_INCLUDE_FILE(sys/timerfd.h EVENT__HAVE_SYS_TIMERFD_H)
CHECK_INCLUDE_FILE(errno.h EVENT__HAVE_ERRNO_H)


CHECK_FUNCTION_EXISTS_EX(epoll_create EVENT__HAVE_EPOLL)
CHECK_FUNCTION_EXISTS_EX(epoll_ctl EVENT__HAVE_EPOLL_CTL)
CHECK_FUNCTION_EXISTS_EX(eventfd EVENT__HAVE_EVENTFD)
if(NOT EVENT__DISABLE_CLOCK_GETTIME)
    CHECK_FUNCTION_EXISTS_EX(clock_gettime EVENT__HAVE_CLOCK_GETTIME)
endif()
CHECK_FUNCTION_EXISTS_EX(fcntl EVENT__HAVE_FCNTL)
CHECK_FUNCTION_EXISTS_EX(getaddrinfo EVENT__HAVE_GETADDRINFO)
CHECK_FUNCTION_EXISTS_EX(getnameinfo EVENT__HAVE_GETNAMEINFO)
CHECK_FUNCTION_EXISTS_EX(gettimeofday EVENT__HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS_EX(getprotobynumber EVENT__HAVE_GETPROTOBYNUMBER)
CHECK_FUNCTION_EXISTS_EX(getservbyname EVENT__HAVE_GETSERVBYNAME)
CHECK_FUNCTION_EXISTS_EX(inet_ntop EVENT__HAVE_INET_NTOP)
CHECK_FUNCTION_EXISTS_EX(inet_pton EVENT__HAVE_INET_PTON)
CHECK_FUNCTION_EXISTS_EX(kqueue EVENT__HAVE_KQUEUE)
CHECK_FUNCTION_EXISTS_EX(mmap EVENT__HAVE_MMAP)
CHECK_FUNCTION_EXISTS_EX(pipe EVENT__HAVE_PIPE)
CHECK_FUNCTION_EXISTS_EX(pipe2 EVENT__HAVE_PIPE2)
CHECK_FUNCTION_EXISTS_EX(poll EVENT__HAVE_POLL)
CHECK_FUNCTION_EXISTS_EX(port_create EVENT__HAVE_PORT_CREATE)
CHECK_FUNCTION_EXISTS_EX(sendfile EVENT__HAVE_SENDFILE)
CHECK_FUNCTION_EXISTS_EX(sigaction EVENT__HAVE_SIGACTION)
CHECK_FUNCTION_EXISTS_EX(signal EVENT__HAVE_SIGNAL)
CHECK_FUNCTION_EXISTS_EX(splice EVENT__HAVE_SPLICE)
CHECK_FUNCTION_EXISTS_EX(strlcpy EVENT__HAVE_STRLCPY)
CHECK_FUNCTION_EXISTS_EX(strsep EVENT__HAVE_STRSEP)
CHECK_FUNCTION_EXISTS_EX(strtok_r EVENT__HAVE_STRTOK_R)
CHECK_FUNCTION_EXISTS_EX(strtoll EVENT__HAVE_STRTOLL)
CHECK_FUNCTION_EXISTS_EX(vasprintf EVENT__HAVE_VASPRINTF)
CHECK_FUNCTION_EXISTS_EX(sysctl EVENT__HAVE_SYSCTL)
CHECK_FUNCTION_EXISTS_EX(accept4 EVENT__HAVE_ACCEPT4)
CHECK_FUNCTION_EXISTS_EX(arc4random EVENT__HAVE_ARC4RANDOM)
CHECK_FUNCTION_EXISTS_EX(arc4random_buf EVENT__HAVE_ARC4RANDOM_BUF)
CHECK_FUNCTION_EXISTS_EX(epoll_create1 EVENT__HAVE_EPOLL_CREATE1)
CHECK_FUNCTION_EXISTS_EX(getegid EVENT__HAVE_GETEGID)
CHECK_FUNCTION_EXISTS_EX(geteuid EVENT__HAVE_GETEUID)
CHECK_FUNCTION_EXISTS_EX(getifaddrs EVENT__HAVE_GETIFADDRS)
CHECK_FUNCTION_EXISTS_EX(issetugid EVENT__HAVE_ISSETUGID)
CHECK_FUNCTION_EXISTS_EX(mach_absolute_time EVENT__HAVE_MACH_ABSOLUTE_TIME)
CHECK_FUNCTION_EXISTS_EX(nanosleep EVENT__HAVE_NANOSLEEP)
CHECK_FUNCTION_EXISTS_EX(usleep EVENT__HAVE_USLEEP)
CHECK_FUNCTION_EXISTS_EX(timeradd EVENT__HAVE_TIMERADD)
CHECK_FUNCTION_EXISTS_EX(timerclear EVENT__HAVE_TIMERCLEAR)
CHECK_FUNCTION_EXISTS_EX(timercmp EVENT__HAVE_TIMERCMP)
CHECK_FUNCTION_EXISTS_EX(timerfd_create HAVE_TIMERFD_CREATE)
CHECK_FUNCTION_EXISTS_EX(timerisset EVENT__HAVE_TIMERISSET)
CHECK_FUNCTION_EXISTS_EX(putenv EVENT__HAVE_PUTENV)
CHECK_FUNCTION_EXISTS_EX(setenv EVENT__HAVE_SETENV)
CHECK_FUNCTION_EXISTS_EX(setrlimit EVENT__HAVE_SETRLIMIT)
CHECK_FUNCTION_EXISTS_EX(umask EVENT__HAVE_UMASK)
CHECK_FUNCTION_EXISTS_EX(unsetenv EVENT__HAVE_UNSETENV)

# Get the gethostbyname_r prototype.
CHECK_FUNCTION_EXISTS_EX(gethostbyname_r EVENT__HAVE_GETHOSTBYNAME_R)

if(EVENT__HAVE_GETHOSTBYNAME_R)
    CHECK_PROTOTYPE_DEFINITION(gethostbyname_r
        "int gethostbyname_r(const char *name, struct hostent *hp, struct hostent_data *hdata)"
        "0"
        "netdb.h"
        EVENT__HAVE_GETHOSTBYNAME_R_3_ARG)

    CHECK_PROTOTYPE_DEFINITION(gethostbyname_r
        "struct hostent *gethostbyname_r(const char *name, struct hostent *hp, char *buf, size_t buflen, int *herr)"
        "NULL"
        "netdb.h"
        EVENT__HAVE_GETHOSTBYNAME_R_5_ARG)

    CHECK_PROTOTYPE_DEFINITION(gethostbyname_r
        "int gethostbyname_r(const char *name, struct hostent *hp, char *buf, size_t buflen, struct hostent **result, int *herr)"
        "0"
        "netdb.h"
        EVENT__HAVE_GETHOSTBYNAME_R_6_ARG)
endif()

if(HAVE_PORT_H AND HAVE_PORT_CREATE)
    set(EVENT__HAVE_EVENT_PORTS 1)
endif()

if(NOT WIN32)
    CHECK_FUNCTION_EXISTS_EX(select EVENT__HAVE_SELECT)
endif()

CHECK_TYPE_SIZE("uint8_t" EVENT__HAVE_UINT8_T)
CHECK_TYPE_SIZE("uint16_t" EVENT__HAVE_UINT16_T)
CHECK_TYPE_SIZE("uint32_t" EVENT__HAVE_UINT32_T)
CHECK_TYPE_SIZE("uint64_t" EVENT__HAVE_UINT64_T)
CHECK_TYPE_SIZE("short" EVENT__SIZEOF_SHORT BUILTIN_TYPES_ONLY)
CHECK_TYPE_SIZE("int" EVENT__SIZEOF_INT BUILTIN_TYPES_ONLY)
CHECK_TYPE_SIZE("unsigned" EVENT__SIZEOF_UNSIGNED BUILTIN_TYPES_ONLY)
CHECK_TYPE_SIZE("unsigned int" EVENT__SIZEOF_UNSIGNED_INT BUILTIN_TYPES_ONLY)
CHECK_TYPE_SIZE("long" EVENT__SIZEOF_LONG BUILTIN_TYPES_ONLY)
CHECK_TYPE_SIZE("long long" EVENT__SIZEOF_LONG_LONG BUILTIN_TYPES_ONLY)

if(WIN32)
    # These aren't available until Windows Vista.
    # But you can still link them. They just won't be found when running the exe.
    set(EVENT__HAVE_INET_NTOP 0)
    set(EVENT__HAVE_INET_PTON 0)
endif()

# Check for different inline keyword versions.
check_function_keywords("inline" "__inline" "__inline__")

if (HAVE_INLINE)
    set (EVENT__inline inline)
elseif (HAVE___INLINE)
    set(EVENT__inline __inline)
elseif(HAVE___INLINE__)
    set(EVENT__inline __inline__)
else()
    set(EVENT__inline)
endif()

CHECK_SYMBOL_EXISTS(TAILQ_FOREACH sys/queue.h EVENT__HAVE_TAILQFOREACH)
CHECK_SYMBOL_EXISTS(CTL_KERN sys/sysctl.h EVENT__HAVE_DECL_CTL_KERN)
CHECK_SYMBOL_EXISTS(KERN_ARND sys/sysctl.h EVENT__HAVE_DECL_KERN_ARND)
CHECK_SYMBOL_EXISTS(KERN_RANDOM sys/sysctl.h EVENT__HAVE_DECL_KERN_RANDOM)
CHECK_SYMBOL_EXISTS(RANDOM_UUID sys/sysctl.h EVENT__HAVE_DECL_RANDOM_UUID)
CHECK_SYMBOL_EXISTS(F_SETFD fcntl.h EVENT__HAVE_SETFD)

CHECK_TYPE_SIZE(fd_mask EVENT__HAVE_FD_MASK)

CHECK_TYPE_SIZE(size_t EVENT__SIZEOF_SIZEE_T)
if(NOT EVENT__SIZEOF_SIZE_T)
  set(EVENT__size_t "unsigned")
  set(EVENT__SIZEOF_SIZE_T ${EVENT__SIZEOF_UNSIGNED})
else()
    set(EVENT__size_t size_t)
endif()

CHECK_TYPE_SIZE("off_t" EVENT__SIZEOF_OFF_T LANGUAGE C)


# XXX we should functionalize these size and type sets. --elley

# Winssck.
if (_MSC_VER)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES BaseTsd.h)
endif()
CHECK_TYPE_SIZE("ssize_t" EVENT__SIZEOF_SSIZE_T_LOWER LANGUAGE C)
CHECK_TYPE_SIZE("SSIZE_T" EVENT__SIZEOF_SSIZE_T_UPPER LANGUAGE C)

if (EVENT__SIZEOF_SSIZE_T_LOWER)
    set(EVENT__ssize_t "ssize_t")
    set(EVENT__SIZEOF_SSIZE_T ${EVENT__SIZEOF_SSIZE_T_LOWER})
elseif (EVENT__SIZEOF_SSIZE_T_UPPER)
    set(EVENT__ssize_t "SSIZE_T")
    set(EVENT__SIZEOF_SSIZE_T ${EVENT__SIZEOF_SSIZE_T_UPPER})
else()
    set(EVENT__ssize_t "int")
    set(EVENT__SIZEOF_SSIZE_T ${EVENT__SIZEOF_INT})
endif()


CHECK_TYPE_SIZE(socklen_t EVENT__SIZEOF_SOCKLEN_T)
if(NOT EVENT__SIZEOF_SOCKLEN_T)
  set(EVENT__socklen_t "unsigned int")
  set(EVENT__SIZEOF_SOCKLEN_T ${EVENT__SIZEOF_UNSIGNED_INT})
else()
    set(EVENT__socklen_t "socklen_t")
endif()

CHECK_TYPE_SIZE(pid_t EVENT__SIZEOF_PID_T)
if(NOT EVENT__SIZEOF_PID_T)
  set(EVENT__pid_t "int")
  set(EVENT__SIZEOF_PID_T ${EVENT__SIZEOF_INT})
else()
	set(EVENT__pid_t "pid_t")
	set(EVENT__SIZEOF_PID_T EVENT__SIZEOF_PID_T)
endif()

if (NOT EVENT__DISABLE_THREAD_SUPPORT)
    CHECK_TYPE_SIZE(pthread_t EVENT__SIZEOF_PTHREAD_T)
endif()

if(EVENT__HAVE_CLOCK_GETTIME)
  set(EVENT__DNS_USE_CPU_CLOCK_FOR_ID 1)
endif()

# we're just getting lazy now.
CHECK_TYPE_SIZE("uintptr_t" EVENT__HAVE_UINTPTR_T)
CHECK_TYPE_SIZE("void *" EVENT__SIZEOF_VOID_P)

# Tests file offset bits.
# TODO: Add AIX test for if -D_LARGE_FILES is needed.

# XXX: Why is this here? we don't even use it. Well, we don't even use it
#      on top of that, why is it set in the config.h?! IT_MAKES_NO_SENSE
#      I'm commenting it out for now.
#      - ellzey

#CHECK_FILE_OFFSET_BITS()
#set(EVENT___FILE_OFFSET_BITS _FILE_OFFSET_BITS)

include(CheckWaitpidSupportWNOWAIT)

# Verify kqueue works with pipes.
if (EVENT__HAVE_KQUEUE)
    if (CMAKE_CROSSCOMPILING AND NOT EVENT__FORCE_KQUEUE_CHECK)
        message(WARNING "Cannot check if kqueue works with pipes when crosscompiling, use EVENT__FORCE_KQUEUE_CHECK to be sure (this requires manually running a test program on the cross compilation target)")
        set(EVENT__HAVE_WORKING_KQUEUE 1)
    else()
        message(STATUS "Checking if kqueue works with pipes...")
        include(CheckWorkingKqueue)
    endif()
endif()

CHECK_SYMBOL_EXISTS(_MINIX "stdio.h" EVENT___MINIX)
CHECK_SYMBOL_EXISTS(_POSIX_1_SOURCE "stdio.h" EVENT___POSIX_1_SOURCE)
CHECK_SYMBOL_EXISTS(_POSIX_SOURCE "stdio.h" EVENT___POSIX_SOURCE)

if(EVENT__HAVE_NETDB_H)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES netdb.h)
    CHECK_TYPE_SIZE("struct addrinfo" EVENT__HAVE_STRUCT_ADDRINFO)
elseif(WIN32)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES ws2tcpip.h)
    CHECK_TYPE_SIZE("struct addrinfo" EVENT__HAVE_STRUCT_ADDRINFO)
endif()

# Check for sockaddr structure sizes.
set(SOCKADDR_HEADERS)
if (WIN32)
    set(CMAKE_REQUIRED_DEFINITIONS "-DWIN32_LEAN_AND_MEAN")
    if (_MSC_VER LESS 1300)
        set(SOCKADDR_HEADERS winsock.h)
    else()
        set(SOCKADDR_HEADERS winsock2.h ws2tcpip.h)
    endif()
else()
    if (EVENT__HAVE_NETINET_IN_H)
        set(SOCKADDR_HEADERS ${SOCKADDR_HEADERS} netinet/in.h)
    endif()

    if (EVENT__HAVE_NETINET_IN6_H)
        set(SOCKADDR_HEADERS ${SOCKADDR_HEADERS} netinet/in6.h)
    endif()

    if (EVENT__HAVE_SYS_SOCKET_H)
        set(SOCKADDR_HEADERS ${SOCKADDR_HEADERS} sys/socket.h)
    endif()

    if (EVENT__HAVE_NETDB_H)
        set(SOCKADDR_HEADERS ${SOCKADDR_HEADERS} netdb.h)
    endif()
endif()

CHECK_TYPE_SIZE("struct in6_addr" EVENT__HAVE_STRUCT_IN6_ADDR)
if(EVENT__HAVE_STRUCT_IN6_ADDR)
    CHECK_STRUCT_HAS_MEMBER("struct in6_addr"
            s6_addr16 "${SOCKADDR_HEADERS}"
            EVENT__HAVE_STRUCT_IN6_ADDR_S6_ADDR16)

    CHECK_STRUCT_HAS_MEMBER("struct in6_addr"
            s6_addr32 "${SOCKADDR_HEADERS}"
            EVENT__HAVE_STRUCT_IN6_ADDR_S6_ADDR32)
endif()

CHECK_TYPE_SIZE("sa_family_t" EVENT__HAVE_SA_FAMILY_T)
CHECK_TYPE_SIZE("struct sockaddr_in6" EVENT__HAVE_STRUCT_SOCKADDR_IN6)

if(EVENT__HAVE_STRUCT_SOCKADDR_IN6)
    CHECK_STRUCT_HAS_MEMBER("struct sockaddr_in6"
            sin6_len "${SOCKADDR_HEADERS}"
            EVENT__HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN)

    CHECK_STRUCT_HAS_MEMBER("struct sockaddr_in6"
            sin_len "${SOCKADDR_HEADERS}"
            EVENT__HAVE_STRUCT_SOCKADDR_IN_SIN_LEN)
endif()

CHECK_TYPE_SIZE("struct sockaddr_storage" EVENT__HAVE_STRUCT_SOCKADDR_STORAGE)
if(EVENT__HAVE_STRUCT_SOCKADDR_STORAGE)
    CHECK_STRUCT_HAS_MEMBER("struct sockaddr_storage"
            ss_family "${SOCKADDR_HEADERS}"
            EVENT__HAVE_STRUCT_SOCKADDR_STORAGE_SS_FAMILY)

    CHECK_STRUCT_HAS_MEMBER("struct sockaddr_storage"
            __ss_family "${SOCKADDR_HEADERS}" EVENT__HAVE_STRUCT_SOCKADDR_STORAGE___SS_FAMILY)
endif()

# Group the source files.
set(HDR_PRIVATE
    bufferevent-internal.h
    changelist-internal.h
    defer-internal.h
    epolltable-internal.h
    evbuffer-internal.h
    event-internal.h
    evmap-internal.h
    evrpc-internal.h
    evsignal-internal.h
    evthread-internal.h
    ht-internal.h
    http-internal.h
    iocp-internal.h
    ipv6-internal.h
    log-internal.h
    minheap-internal.h
    mm-internal.h
    ratelim-internal.h
    strlcpy-internal.h
    util-internal.h
    evconfig-private.h
    compat/sys/queue.h)

set(HDR_COMPAT
    include/evdns.h
    include/evrpc.h
    include/event.h
    include/evhttp.h
    include/evutil.h)

set(HDR_PUBLIC
    include/event2/buffer.h
    include/event2/bufferevent.h
    include/event2/bufferevent_compat.h
    include/event2/bufferevent_struct.h
    include/event2/buffer_compat.h
    include/event2/dns.h
    include/event2/dns_compat.h
    include/event2/dns_struct.h
    include/event2/event.h
    include/event2/event_compat.h
    include/event2/event_struct.h
    include/event2/http.h
    include/event2/http_compat.h
    include/event2/http_struct.h
    include/event2/keyvalq_struct.h
    include/event2/listener.h
    include/event2/rpc.h
    include/event2/rpc_compat.h
    include/event2/rpc_struct.h
    include/event2/tag.h
    include/event2/tag_compat.h
    include/event2/thread.h
    include/event2/util.h
    include/event2/visibility.h
    ${PROJECT_BINARY_DIR}/include/event2/event-config.h)

set(SRC_CORE
    buffer.c
    bufferevent.c
    bufferevent_filter.c
    bufferevent_pair.c
    bufferevent_ratelim.c
    bufferevent_sock.c
    event.c
    evmap.c
    evthread.c
    evutil.c
    evutil_rand.c
    evutil_time.c
    listener.c
    log.c
    signal.c
    strlcpy.c)

if(EVENT__HAVE_SELECT)
    list(APPEND SRC_CORE select.c)
endif()

if(EVENT__HAVE_POLL)
    list(APPEND SRC_CORE poll.c)
endif()

if(EVENT__HAVE_KQUEUE)
    list(APPEND SRC_CORE kqueue.c)
endif()

if(EVENT__HAVE_DEVPOLL)
    list(APPEND SRC_CORE devpoll.c)
endif()

if(EVENT__HAVE_EPOLL)
    list(APPEND SRC_CORE epoll_sub.c epoll.c)
endif()

if(EVENT__HAVE_EVENT_PORTS)
    list(APPEND SRC_CORE evport.c)
endif()

if (NOT EVENT__DISABLE_OPENSSL)
    find_package(OpenSSL REQUIRED)

    set(EVENT__HAVE_OPENSSL 1)

    message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL lib: ${OPENSSL_LIBRARIES}")

    include_directories(${OPENSSL_INCLUDE_DIR})

    list(APPEND SRC_CORE bufferevent_openssl.c)
    list(APPEND HDR_PUBLIC include/event2/bufferevent_ssl.h)
    list(APPEND LIB_APPS ${OPENSSL_LIBRARIES})
endif()

if (NOT EVENT__DISABLE_THREAD_SUPPORT)
    if (WIN32)
        list(APPEND SRC_CORE evthread_win32.c)
    else()
        find_package(Threads REQUIRED)
        if (NOT CMAKE_USE_PTHREADS_INIT)
            message(FATAL_ERROR
                            "Failed to find Pthreads, set EVENT__DISABLE_THREAD_SUPPORT to disable")
        endif()

        set(EVENT__HAVE_PTHREADS 1)
        list(APPEND SRC_CORE evthread_pthread.c)
        list(APPEND LIB_APPS ${CMAKE_THREAD_LIBS_INIT})
    endif()
endif()

if (NOT EVENT__DISABLE_TESTS)
    # Zlib is only used for testing.
    find_package(ZLIB)

    if (ZLIB_LIBRARY AND ZLIB_INCLUDE_DIR)
        include_directories(${ZLIB_INCLUDE_DIRS})

        set(EVENT__HAVE_LIBZ 1)
        list(APPEND LIB_APPS ${ZLIB_LIBRARIES})
    endif()
endif()

set(SRC_EXTRA
    event_tagging.c
    http.c
    evdns.c
    evrpc.c)

add_definitions(-DHAVE_CONFIG_H)

# We use BEFORE here so we don't accidentally look in system directories
# first for some previous versions of the headers that are installed.
include_directories(BEFORE ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/compat
    ${PROJECT_SOURCE_DIR}/include)

if(WIN32)
    list(APPEND SRC_CORE
        buffer_iocp.c
        bufferevent_async.c
        event_iocp.c
        win32select.c)

    list(APPEND HDR_PRIVATE WIN32-Code/getopt.h)

    set(EVENT__DNS_USE_FTIME_FOR_ID 1)
    set(LIB_PLATFORM ws2_32)
    add_definitions(
            -D_CRT_SECURE_NO_WARNINGS
            -D_CRT_NONSTDC_NO_DEPRECATE)

    include_directories(./WIN32-Code)
endif()

if (UNIX)
    list(APPEND LIB_PLATFORM m)
endif()

if (SOLARIS)
    list(APPEND LIB_PLATFORM socket nsl)
endif()

source_group("Headers Private"  FILES ${HDR_PRIVATE})
source_group("Header Compat"    FILES ${HDR_COMPAT})
source_group("Headers Public"   FILES ${HDR_PUBLIC})
source_group("Source Core"      FILES ${SRC_CORE})
source_group("Source Extra"     FILES ${SRC_EXTRA})

# Generate the configure headers.
# (Place them in the build dir so we don't polute the source tree with generated files).
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}/include)

if (EVENT__BUILD_SHARED_LIBRARIES)
  set(EVENT__LIBRARY_TYPE SHARED)

  if ((CMAKE_COMPILER_IS_GNUCC) OR (${CMAKE_C_COMPILER_ID} STREQUAL "Clang"))
        add_compiler_flags(-fvisibility=hidden)
  elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "SunPro")
    add_compiler_flags(-xldscope=hidden)
  endif()

  set(EVENT__NEED_DLLIMPORT 1)
else (EVENT__BUILD_SHARED_LIBRARIES)
  set(EVENT__LIBRARY_TYPE STATIC)
endif (EVENT__BUILD_SHARED_LIBRARIES)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/event-config.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/event2/event-config.h
        NEWLINE_STYLE UNIX)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/evconfig-private.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/evconfig-private.h)

#
# Create the libraries.
#

# TODO: Add dynamic versions of the libraries as well.
add_library(event_core ${EVENT__LIBRARY_TYPE}
    ${HDR_PRIVATE}
    ${HDR_COMPAT}
    ${HDR_PUBLIC}
    ${SRC_CORE})

add_library(event_extra ${EVENT__LIBRARY_TYPE}
    ${HDR_PRIVATE}
    ${HDR_COMPAT}
    ${HDR_PUBLIC}
    ${SRC_CORE}
    ${SRC_EXTRA})

# library exists for historical reasons; it contains the contents of
# both libevent_core and libevent_extra. You shouldn’t use it; it may
# go away in a future version of Libevent.
add_library(event ${EVENT__LIBRARY_TYPE}
    ${HDR_PRIVATE}
    ${HDR_COMPAT}
    ${HDR_PUBLIC}
    ${SRC_CORE}
    ${SRC_EXTRA})

if (EVENT__BUILD_SHARED_LIBRARIES)
    # Prepare static library to be linked to tests that need hidden symbols
    add_library(event_extra_static STATIC
        ${HDR_PRIVATE}
        ${HDR_COMPAT}
        ${HDR_PUBLIC}
        ${SRC_CORE}
        ${SRC_EXTRA})

    set(EVENT_EXTRA_FOR_TEST event_extra_static)

   target_link_libraries(event_core ${OPENSSL_LIBRARIES}
                                    ${CMAKE_THREAD_LIBS_INIT}
                                    ${LIB_PLATFORM})

   target_link_libraries(event ${OPENSSL_LIBRARIES}
                               ${CMAKE_THREAD_LIBS_INIT}
                               ${LIB_PLATFORM})

   target_link_libraries(event_extra ${OPENSSL_LIBRARIES}
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${LIB_PLATFORM})

  set_target_properties(event
        PROPERTIES SOVERSION
            ${EVENT_ABI_LIBVERSION})

  set_target_properties(event_core
        PROPERTIES SOVERSION
            ${EVENT_ABI_LIBVERSION})

  set_target_properties(event_extra
        PROPERTIES SOVERSION
            ${EVENT_ABI_LIBVERSION})

else (EVENT__BUILD_SHARED_LIBRARIES)
    set(EVENT_EXTRA_FOR_TEST event_extra)
endif (EVENT__BUILD_SHARED_LIBRARIES)

#
# Samples.
#

if (NOT EVENT__DISABLE_SAMPLES)
    set(SAMPLES
        dns-example
        event-read-fifo
        hello-world
        signal-test
        http-server
        http-connect
        time-test)

    if (NOT EVENT__DISABLE_OPENSSL AND OPENSSL_LIBRARIES)
        set(CMAKE_REQUIRED_LIBRARIES ${OPENSSL_LIBRARIES})
        CHECK_FUNCTION_EXISTS_EX(ERR_remove_thread_state EVENT__HAVE_ERR_REMOVE_THREAD_STATE)
        set(CMAKE_REQUIRED_LIBRARIES "")

        # Special sample with more than one file.
        add_executable(https-client
            sample/https-client.c
            sample/openssl_hostname_validation.c
            sample/hostcheck.c)

        target_link_libraries(https-client
                    event_extra
                    ${LIB_APPS}
                    ${LIB_PLATFORM})

        add_dependencies(https-client event_extra)

        # Requires OpenSSL.
        list(APPEND SAMPLES le-proxy)
    endif()

    foreach(SAMPLE ${SAMPLES})
        add_executable(${SAMPLE}
                    sample/${SAMPLE}.c)

        target_link_libraries(${SAMPLE}
                    event_extra
                    ${LIB_APPS}
                    ${LIB_PLATFORM})

        add_dependencies(${SAMPLE} event_extra)
    endforeach()

    if (WIN32)
        target_sources(dns-example PUBLIC
                       WIN32-Code/getopt.c
                       WIN32-Code/getopt_long.c)
    endif()
endif()

if (NOT EVENT__DISABLE_BENCHMARK)
    foreach (BENCHMARK bench bench_cascade bench_http bench_httpclient)
        set(BENCH_SRC test/${BENCHMARK}.c)

        if (WIN32)
            list(APPEND BENCH_SRC
                            WIN32-Code/getopt.c
                            WIN32-Code/getopt_long.c)
        endif()

        add_executable(${BENCHMARK} ${BENCH_SRC})

        target_link_libraries(${BENCHMARK}
                    event_extra
                    ${LIB_PLATFORM})

        add_dependencies(${BENCHMARK} event_extra)
    endforeach()
endif()

if (NOT EVENT__DISABLE_TESTS)
    #
    # Generate Regress tests.
    #
    if (NOT EVENT__DISABLE_REGRESS)

        # (We require python2 to generate the regress tests)
        foreach (PY python2.6 python2.7 python2)
            unset(FIND_PYTHON2 CACHE)
            find_program(FIND_PYTHON2 ${PY})
            if (FIND_PYTHON2)
                set(PYTHON_EXECUTABLE "${PY}")
                break()
            endif()
        endforeach()
        find_package(PythonInterp)

        if (PYTHONINTERP_FOUND AND PYTHON_VERSION_STRING VERSION_LESS "3.0.0")
            set(__FOUND_USABLE_PYTHON 1)
        endif()

        if (__FOUND_USABLE_PYTHON)
            message(STATUS "Generating regress tests...")

            add_definitions(-DTINYTEST_LOCAL)

            add_custom_command(
                OUTPUT
                    ${CMAKE_CURRENT_SOURCE_DIR}/test/regress.gen.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/test/regress.gen.h
                DEPENDS
                    event_rpcgen.py
                    test/regress.rpc
                COMMAND ${PYTHON_EXECUTABLE} ../event_rpcgen.py regress.rpc
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test)

            list(APPEND SRC_REGRESS
                test/regress.c
                test/regress.gen.c
                test/regress.gen.h
                test/regress_buffer.c
                test/regress_bufferevent.c
                test/regress_dns.c
                test/regress_et.c
                test/regress_finalize.c
                test/regress_http.c
                test/regress_listener.c
                test/regress_main.c
                test/regress_minheap.c
                test/regress_rpc.c
                test/regress_testutils.c
                test/regress_testutils.h
                test/regress_util.c
                test/tinytest.c
                ${SRC_CORE}
                ${SRC_EXTRA})

            if (WIN32)
                list(APPEND SRC_REGRESS test/regress_iocp.c)
                if (NOT EVENT__DISABLE_THREAD_SUPPORT)
                    list(APPEND SRC_REGRESS test/regress_thread.c)
                endif()
            endif()

            if (CMAKE_USE_PTHREADS_INIT)
                list(APPEND SRC_REGRESS test/regress_thread.c)
            endif()

            if (ZLIB_LIBRARY AND ZLIB_INCLUDE_DIR)
                list(APPEND SRC_REGRESS test/regress_zlib.c)
            endif()

            if (OPENSSL_LIBRARIES)
                list(APPEND SRC_REGRESS test/regress_ssl.c)
            endif()

            add_executable(regress ${SRC_REGRESS})

            # While building the test suite we don't want the visibility
            # header trying to "dllimport" the symbols on windows (it
            # generates a ton of warnings due to different link
            # attributes for all of the symbols)
            SET_TARGET_PROPERTIES(regress
                            PROPERTIES COMPILE_DEFINITIONS
              "EVENT_BUILDING_REGRESS_TEST=1")

            target_link_libraries(regress
                            ${LIB_APPS}
                            ${LIB_PLATFORM})
        else()
            message(WARNING "No suitable Python interpreter found, cannot generate regress tests!")
        endif()
    endif()

    #
    # Test programs.
    #
        # all of these, including the cmakelists.txt should be moved
        # into the dirctory 'tests' first.
        #
        # doing this, we can remove all the DISABLE_TESTS stuff, and simply
        # do something like:
        #
        # add_custom_targets(tests)
        # add_executable(... EXCLUDE_FROM_ALL ...c)
        # add_dependencis(tests testa testb testc)
        # add_test(....)
        #
        # then you can just run 'make tests' instead of them all
        # auto-compile|running
        # - ellzey
    set(TESTPROGS   test-changelist
                    test-eof
                    test-fdleak
                    test-init
                    test-time
                    test-weof)

    set(ALL_TESTPROGS
            ${TESTPROGS}
            test-dumpevents
            test-ratelim)

    # Create test program executables.
    foreach (TESTPROG ${ALL_TESTPROGS})
        add_executable(${TESTPROG}
                    test/${TESTPROG}.c)

        target_link_libraries(${TESTPROG}
                    ${EVENT_EXTRA_FOR_TEST}
                    ${LIB_PLATFORM})

        add_dependencies(${TESTPROG}
                    ${EVENT_EXTRA_FOR_TEST})
    endforeach()

    #
    # We run all tests with the different backends turned on one at a time.
    #

    # Add event backends based on system introspection result.
    set(BACKENDS "")

    if (EVENT__HAVE_EPOLL)
        list(APPEND BACKENDS EPOLL)
    endif()

    if (EVENT__HAVE_SELECT)
        list(APPEND BACKENDS SELECT)
    endif()

    if (EVENT__HAVE_POLL)
        list(APPEND BACKENDS POLL)
    endif()

    if (EVENT__HAVE_KQUEUE)
        list(APPEND BACKENDS KQUEUE)
    endif()

    if (EVENT__HAVE_EVENT_PORTS)
        list(APPEND BACKENDS EVPORT)
    endif()

    if (EVENT__HAVE_DEVPOLL)
        list(APPEND BACKENDS DEVPOLL)
    endif()

    if (WIN32)
        list(APPEND BACKENDS WIN32)
    endif()


    # Default environment variables turns off all event systems,
    # then we enable each one, one at a time when creating the tests.
    set(DEFAULT_TEST_ENV_VARS "EVENT_SHOW_METHOD=1;")
    foreach(BACKEND ${BACKENDS})
        set(BACKEND_ENV_VAR "EVENT_NO${BACKEND}=1")
        list(APPEND DEFAULT_TEST_ENV_VARS "${BACKEND_ENV_VAR}")
    endforeach()

    # Macro that creates the ctest test for a backend.
    macro(add_backend_test BACKEND_TEST_NAME ENV_VARS)
        set(TEST_NAMES "")

        foreach (TESTPROG ${TESTPROGS})
            set(TEST_NAME ${TESTPROG}__${BACKEND_TEST_NAME})

            add_test(${TEST_NAME}
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TESTPROG})

            list(APPEND TEST_NAMES ${TEST_NAME})

            set_tests_properties(${TEST_NAME}
                            PROPERTIES ENVIRONMENT "${ENV_VARS}")
        endforeach()

        # Dump events test.
        if (__FOUND_USABLE_PYTHON)
            set(TEST_NAME test-dumpevents__${BACKEND_TEST_NAME})

            add_test(${TEST_NAME}
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-dumpevents |
                            ${PYTHON_EXECUTABLE}
                            ${CMAKE_CURRENT_SOURCE_DIR}/test/check-dumpevents.py)

            set_tests_properties(${TEST_NAME}
                            PROPERTIES ENVIRONMENT "${ENV_VARS}")
        else()
            message(WARNING "test-dumpevents will be run without output check since python was not found!")
            set(TEST_NAME test-dumpevents__${BACKEND_TEST_NAME}_no_check)

            add_test(${TEST_NAME}
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-dumpevents)

            set_tests_properties(${TEST_NAME}
                            PROPERTIES ENVIRONMENT "${ENV_VARS}")
        endif()

        # Regress tests.
        if (NOT EVENT__DISABLE_REGRESS AND __FOUND_USABLE_PYTHON)
            set(TEST_NAME regress__${BACKEND_TEST_NAME})

            add_test(${TEST_NAME}
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/regress)

            set_tests_properties(${TEST_NAME}
                            PROPERTIES ENVIRONMENT "${ENV_VARS}")

            add_test(${TEST_NAME}_debug
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/regress)

            set_tests_properties(${TEST_NAME}_debug
                            PROPERTIES ENVIRONMENT "${ENV_VARS};EVENT_DEBUG_MODE=1")
        endif()
    endmacro()

    # Add the tests for each backend.
    foreach(BACKEND ${BACKENDS})
        # Enable this backend only.
        set(BACKEND_ENV_VARS ${DEFAULT_TEST_ENV_VARS})
        list(REMOVE_ITEM BACKEND_ENV_VARS EVENT_NO${BACKEND}=1)

        # Epoll has some extra settings.
        if (${BACKEND} STREQUAL "EPOLL")
            add_backend_test(timerfd_${BACKEND}
                            "${BACKEND_ENV_VARS};EVENT_PRECISE_TIMER=1")

            add_backend_test(changelist_${BACKEND}
                            "${BACKEND_ENV_VARS};EVENT_EPOLL_USE_CHANGELIST=yes")

            add_backend_test(timerfd_changelist_${BACKEND}
                            "${BACKEND_ENV_VARS};EVENT_EPOLL_USE_CHANGELIST=yes;EVENT_PRECISE_TIMER=1")
        else()
            add_backend_test(${BACKEND} "${BACKEND_ENV_VARS}")
        endif()
    endforeach()

    #
    # Rate limiter tests.
    #

    # Group limits, no connection limit.
        set(RL_BIN  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-ratelim)

    add_test(test-ratelim__group_lim
                ${RL_BIN}
                -g 30000
                -n 30
                -t 100
                --check-grouplimit 1000
                --check-stddev 100)

    # Connection limit, no group limit.
    add_test(test-ratelim__con_lim
                ${RL_BIN}
                -c 1000
                -n 30
                -t 100
                --check-connlimit 50
                --check-stddev 50)

    # Connection limit and group limit.
    add_test(test-ratelim__group_con_lim
                ${RL_BIN}
                -c 1000
                -g 30000
                -n 30
                -t 100
                --check-grouplimit 1000
                --check-connlimit 50
                --check-stddev 50)

    # Connection limit and group limit with independent drain.
    add_test(test-ratelim__group_con_lim_drain
                ${RL_BIN}
                -c 1000
                -g 35000
                -n 30
                -t 100
                -G 500
                --check-grouplimit 1000
                --check-connlimit 50
                --check-stddev 50)

    # Add a "make verify" target, same as for autoconf.
    # (Important! This will unset all EVENT_NO* environment variables.
    #  If they are set in the shell the tests are running using simply "ctest" or "make test" will fail)
    if (WIN32)
        # Windows doesn't have "unset". But you can use "set VAR=" instead.
        # We need to guard against the possibility taht EVENT_NOWIN32 is set, and all test failing
        # since no event backend being available.
        file(TO_NATIVE_PATH ${CMAKE_CTEST_COMMAND} WINDOWS_CTEST_COMMAND)

        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/verify_tests.bat
            "
            set EVENT_NOWIN32=
            \"${WINDOWS_CTEST_COMMAND}\"
            ")

        message(STATUS "${WINDOWS_CTEST_COMMAND}")

        file(COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/verify_tests.bat
            DESTINATION
                            ${CMAKE_CURRENT_BINARY_DIR}
            FILE_PERMISSIONS
                            OWNER_READ
                            OWNER_WRITE
                            OWNER_EXECUTE
                            GROUP_READ
                            GROUP_EXECUTE
                            WORLD_READ WORLD_EXECUTE)

        file(TO_NATIVE_PATH
                    "${CMAKE_CURRENT_BINARY_DIR}/verify_tests.bat" VERIFY_PATH)

        add_custom_target(verify COMMAND "${VERIFY_PATH}"
                        DEPENDS event ${ALL_TESTPROGS})
    else()
        # On some platforms doing exec(unset) as CMake does won't work, so make sure
        # we run the unset command in a shell instead.
        # First we write the script contents.
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/verify_tests.sh
            "
            #!/bin/bash
            unset EVENT_NOEPOLL; unset EVENT_NOPOLL; unset EVENT_NOSELECT; unset EVENT_NOWIN32; unset EVENT_NOEVPORT; unset EVENT_NOKQUEUE; unset EVENT_NODEVPOLL
            ${CMAKE_CTEST_COMMAND}
            ")

        # Then we copy the file (this allows us to set execute permission on it)
        file(COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/verify_tests.sh
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
            FILE_PERMISSIONS
                            OWNER_READ
                            OWNER_WRITE
                            OWNER_EXECUTE
                            GROUP_READ
                            GROUP_EXECUTE
                            WORLD_READ
                            WORLD_EXECUTE)

        # Create the target that runs the script.
        add_custom_target(verify
                    COMMAND
                        ${CMAKE_CURRENT_BINARY_DIR}/verify_tests.sh
          DEPENDS
                        event
                        ${ALL_TESTPROGS})
    endif()

    if (NOT EVENT__DISABLE_REGRESS AND __FOUND_USABLE_PYTHON)
        add_dependencies(verify regress)
    endif()

    if (EVENT__COVERAGE)
        include(CodeCoverage)

        setup_target_for_coverage(
            verify_coverage # Coverage target name "make verify_coverage"
            make            # Test runner.
            coverage        # Output directory.
            verify)         # Arguments passed to test runner. "make verify"
    endif()

    enable_testing()

    include(CTest)
endif()

#
# Installation preparation.
#

# Allow the user to override installation directories.
set(EVENT_INSTALL_LIB_DIR       lib CACHE PATH "Installation directory for libraries")
set(EVENT_INSTALL_BIN_DIR       bin CACHE PATH "Installation directory for executables")
set(EVENT_INSTALL_INCLUDE_DIR   include CACHE PATH "Installation directory for header files")

if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR cmake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/cmake/libevent)
endif()

set(EVENT_INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

# Make sure the paths are absolute.
foreach(p LIB BIN INCLUDE CMAKE)
    set(var EVENT_INSTALL_${p}_DIR)
    if(NOT IS_ABSOLUTE "${${var}}")
        set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
    endif()
endforeach()

# Export targets (This is used for other CMake projects to easily find the libraries and include files).
export(TARGETS event event_extra event_core
        FILE "${PROJECT_BINARY_DIR}/LibeventTargets.cmake")
export(PACKAGE libevent)

# Generate the config file for the build-tree.
set(EVENT__INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/include")

set(LIBEVENT_INCLUDE_DIRS
    ${EVENT__INCLUDE_DIRS}
    CACHE PATH "Libevent include directories")

configure_file(${PROJECT_SOURCE_DIR}/cmake/LibeventConfigBuildTree.cmake.in
                ${PROJECT_BINARY_DIR}/LibeventConfig.cmake
                @ONLY)

# Generate the config file for the installation tree.
file(RELATIVE_PATH
    REL_INCLUDE_DIR
    "${EVENT_INSTALL_CMAKE_DIR}"
    "${EVENT_INSTALL_INCLUDE_DIR}") # Calculate the relative directory from the Cmake dir.

# Note the EVENT_CMAKE_DIR is defined in LibeventConfig.cmake.in,
# we escape it here so it's evaluated when it is included instead
# so that the include dirs are givenrelative to where the
# config file is located.
set(EVENT__INCLUDE_DIRS
    "\${EVENT_CMAKE_DIR}/${REL_INCLUDE_DIR}")

configure_file(${PROJECT_SOURCE_DIR}/cmake/LibeventConfig.cmake.in
                ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/LibeventConfig.cmake
                @ONLY)

# Generate version info for both build-tree and install-tree.
configure_file(${PROJECT_SOURCE_DIR}/cmake/LibeventConfigVersion.cmake.in
                ${PROJECT_BINARY_DIR}/LibeventConfigVersion.cmake
                @ONLY)

# Define the public headers.
set_target_properties(event event_core event_extra
    PROPERTIES PUBLIC_HEADER "${HDR_PUBLIC}")

#
# Install targets.
#
install(TARGETS event event_core event_extra
        EXPORT LibeventTargets
        RUNTIME DESTINATION "${EVENT_INSTALL_BIN_DIR}" COMPONENT bin
        LIBRARY DESTINATION "${EVENT_INSTALL_LIB_DIR}" COMPONENT lib
        ARCHIVE DESTINATION "${EVENT_INSTALL_LIB_DIR}" COMPONENT lib
        PUBLIC_HEADER DESTINATION "${EVENT_INSTALL_INCLUDE_DIR}/event2" COMPONENT dev)

# Install compat headers
install(FILES ${HDR_COMPAT}
    DESTINATION
            "${EVENT_INSTALL_INCLUDE_DIR}"
    COMPONENT dev)

# Install the configs.
install(FILES
    ${PROJECT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/LibeventConfig.cmake
    ${PROJECT_BINARY_DIR}/LibeventConfigVersion.cmake
    DESTINATION
            "${EVENT_INSTALL_CMAKE_DIR}"
        COMPONENT dev)

# Install exports for the install-tree.
install(EXPORT LibeventTargets
        DESTINATION
                    "${EVENT_INSTALL_CMAKE_DIR}"
                COMPONENT dev)

set(LIBEVENT_LIBRARIES
        event
        event_core
        event_extra
        CACHE STRING "Libevent libraries")

message(STATUS "")
message(STATUS "        ---( Libevent " ${EVENT_VERSION} " )---")
message(STATUS "")
message(STATUS "Available event backends: ${BACKENDS}")
message(STATUS "CMAKE_BINARY_DIR:         " ${CMAKE_BINARY_DIR})
message(STATUS "CMAKE_CURRENT_BINARY_DIR: " ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "CMAKE_SOURCE_DIR:         " ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "PROJECT_BINARY_DIR:       " ${PROJECT_BINARY_DIR})
message(STATUS "PROJECT_SOURCE_DIR:       " ${PROJECT_SOURCE_DIR})
message(STATUS "CMAKE_MODULE_PATH:        " ${CMAKE_MODULE_PATH})
message(STATUS "CMAKE_COMMAND:            " ${CMAKE_COMMAND})
message(STATUS "CMAKE_ROOT:               " ${CMAKE_ROOT} )
message(STATUS "CMAKE_SYSTEM:             " ${CMAKE_SYSTEM} )
message(STATUS "CMAKE_SYSTEM_NAME:        " ${CMAKE_SYSTEM_NAME} )
message(STATUS "CMAKE_SYSTEM_VERSION:     " ${CMAKE_SYSTEM_VERSION} )
message(STATUS "CMAKE_SYSTEM_PROCESSOR:   " ${CMAKE_SYSTEM_PROCESSOR} )
message(STATUS "CMAKE_SKIP_RPATH:         " ${CMAKE_SKIP_RPATH} )
message(STATUS "CMAKE_VERBOSE_MAKEFILE:   " ${CMAKE_VERBOSE_MAKEFILE} )
message(STATUS "CMAKE_C_FLAGS:            " ${CMAKE_C_FLAGS} )
message(STATUS "CMAKE_BUILD_TYPE:         " ${CMAKE_BUILD_TYPE} )
message(STATUS "CMAKE_C_COMPILER:         " ${CMAKE_C_COMPILER} )
message(STATUS "CMAKE_AR:                 " ${CMAKE_AR} )
message(STATUS "CMAKE_RANLIB:             " ${CMAKE_RANLIB} )
message(STATUS "")

