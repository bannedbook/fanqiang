cmake_minimum_required(VERSION 2.8)
project(BADVPN C)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

include(GNUInstallDirs)
include(TestBigEndian)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckTypeSize)

option(WITH_PLUGIN_LIBS "Build PIC versions of all libraries for use from plugins" OFF)

set(BUILD_COMPONENTS)

macro (build_switch name text default)
    if (BUILD_NOTHING_BY_DEFAULT)
        option(BUILD_${name} "${text}" OFF)
    else ()
        option(BUILD_${name} "${text}" "${default}")
    endif ()
    list(APPEND BUILD_COMPONENTS "${name}")
endmacro ()

# detect Emscripten
if (CMAKE_C_COMPILER MATCHES "/emcc$")
    set(EMSCRIPTEN ON)
else ()
    set(EMSCRIPTEN OFF)
endif ()

if (EMSCRIPTEN)
    set(ON_IF_NOT_EMSCRIPTEN OFF)
else ()
    set(ON_IF_NOT_EMSCRIPTEN ON)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT EMSCRIPTEN)
    set(ON_IF_LINUX ON)
else ()
    set(ON_IF_LINUX OFF)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR EMSCRIPTEN)
    set(ON_IF_LINUX_OR_EMSCRIPTEN ON)
else ()
    set(ON_IF_LINUX_OR_EMSCRIPTEN OFF)
endif ()

# define build defaults
build_switch(EXAMPLES "build example programs" ON)
build_switch(TESTS "build some other example programs" ON)
build_switch(SERVER "build badvpn-server" ${ON_IF_NOT_EMSCRIPTEN})
build_switch(CLIENT "build badvpn-client" ${ON_IF_NOT_EMSCRIPTEN})
build_switch(FLOODER "build badvpn-flooder" ${ON_IF_NOT_EMSCRIPTEN})
build_switch(TUN2SOCKS "build badvpn-tun2socks" ${ON_IF_NOT_EMSCRIPTEN})
build_switch(UDPGW "build badvpn-udpgw" ${ON_IF_NOT_EMSCRIPTEN})
build_switch(NCD "build badvpn-ncd" ${ON_IF_LINUX_OR_EMSCRIPTEN})
build_switch(DOSTEST "build dostest-server and dostest-attacker" OFF)

if (BUILD_NCD AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    message(FATAL_ERROR "NCD is only available on Linux")
endif ()

if (BUILD_CLIENT OR BUILD_SERVER)
    find_package(OpenSSL REQUIRED)
    set(LIBCRYPTO_INCLUDE_DIRS "${OpenSSL_INCLUDE_DIRS}")
    set(LIBCRYPTO_LIBRARY_DIRS "${OpenSSL_LIBRARY_DIRS}")
    set(LIBCRYPTO_LIBRARIES "${OpenSSL_LIBRARIES}")
endif ()

if (BUILD_SERVER OR BUILD_CLIENT OR BUILD_FLOODER)
    find_package(NSPR REQUIRED)
    find_package(NSS REQUIRED)
endif ()

# choose reactor
if (DEFINED BREACTOR_BACKEND)
    if (NOT (BREACTOR_BACKEND STREQUAL "badvpn" OR BREACTOR_BACKEND STREQUAL "glib"))
        message(FATAL_ERROR "unknown reactor backend specified")
    endif ()
else ()
    if (EMSCRIPTEN)
        set(BREACTOR_BACKEND "emscripten")
    else ()
        set(BREACTOR_BACKEND "badvpn")
    endif ()
endif ()

if (BREACTOR_BACKEND STREQUAL "badvpn")
    add_definitions(-DBADVPN_BREACTOR_BADVPN)
elseif (BREACTOR_BACKEND STREQUAL "glib")
    if (NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
        message(FATAL_ERROR "GLib reactor backend is only available on Linux")
    endif ()
    find_package(GLIB2 REQUIRED)
    add_definitions(-DBADVPN_BREACTOR_GLIB)
elseif (BREACTOR_BACKEND STREQUAL "emscripten")
    add_definitions(-DBADVPN_BREACTOR_EMSCRIPTEN)
endif ()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBCRYPTO_INCLUDE_DIRS}
    ${NSPR_INCLUDE_DIRS}
    ${NSS_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIR}
    lwip/custom
    lwip/src/include
    lwip/src/include/ipv4
    lwip/src/include/ipv6
)

link_directories(
    ${LIBCRYPTO_LIBRARY_DIRS}
    ${NSPR_LIBRARY_DIRS}
    ${NSS_LIBRARY_DIRS}
)

test_big_endian(BIG_ENDIAN)

check_type_size(int INT_SIZE)
if (NOT (INT_SIZE GREATER "3"))
    message(FATAL_ERROR "int must be at least 32 bits")
endif ()

check_type_size(size_t SIZE_SIZE)
if (NOT (SIZE_SIZE GREATER INT_SIZE OR SIZE_SIZE EQUAL INT_SIZE))
    message(FATAL_ERROR "size_t must be greater or equal than int")
endif ()

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS /wd4065 /wd4018 /wd4533 /wd4244 /wd4102 /wd4098 /wd4267 /wd4116)
    add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
else ()
    add_definitions(-std=gnu99 -Wall -Wno-unused-value -Wno-parentheses -Wno-switch -Wredundant-decls -Wshadow)

    if (NOT CMAKE_C_COMPILER_ID STREQUAL "PathScale")
        add_definitions(-Werror=implicit-function-declaration -Wno-switch-enum -Wno-unused-function
                        -Wstrict-aliasing)
    endif ()
    
    if (CMAKE_C_COMPILER_ID MATCHES "^Clang")
        add_definitions(-Wno-initializer-overrides -Wno-tautological-constant-out-of-range-compare)
    endif ()
endif ()

# platform-specific stuff
if (WIN32)
    add_definitions(-DBADVPN_USE_WINAPI -D_WIN32_WINNT=0x600 -DWIN32_LEAN_AND_MEAN)
    add_definitions(-DBADVPN_THREAD_SAFE=0)

    set(CMAKE_REQUIRED_DEFINITIONS "-D_WIN32_WINNT=0x600")
    check_symbol_exists(WSAID_WSASENDMSG "winsock2.h;mswsock.h" HAVE_MSW_1)
    check_symbol_exists(WSAID_WSARECVMSG "winsock2.h;mswsock.h" HAVE_MSW_2)
    check_symbol_exists(WSAID_ACCEPTEX "winsock2.h;mswsock.h" HAVE_MSW_3)
    check_symbol_exists(WSAID_GETACCEPTEXSOCKADDRS "winsock2.h;mswsock.h" HAVE_MSW_4)
    check_symbol_exists(WSAID_CONNECTEX "winsock2.h;mswsock.h" HAVE_MSW_5)
    set(CMAKE_REQUIRED_DEFINITIONS "")
    if (NOT (HAVE_MSW_1 AND HAVE_MSW_2 AND HAVE_MSW_3 AND HAVE_MSW_4 AND HAVE_MSW_5))
        add_definitions(-DBADVPN_USE_SHIPPED_MSWSOCK)
        check_type_size(WSAMSG HAVE_WSAMSG)
        if (NOT HAVE_WSAMSG)
            add_definitions(-DBADVPN_SHIPPED_MSWSOCK_DECLARE_WSAMSG)
        endif ()
    endif ()
else ()
    set(BADVPN_THREADWORK_USE_PTHREAD 1)
    add_definitions(-DBADVPN_THREADWORK_USE_PTHREAD)
    add_definitions(-DBADVPN_THREAD_SAFE=1)

    link_libraries(rt pthread)

    if (EMSCRIPTEN)
        add_definitions(-DBADVPN_EMSCRIPTEN)
        add_definitions(-DBADVPN_NO_PROCESS -DBADVPN_NO_UDEV -DBADVPN_NO_RANDOM)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_definitions(-DBADVPN_LINUX)

        check_include_files(sys/signalfd.h HAVE_SYS_SIGNALFD_H)
        if (HAVE_SYS_SIGNALFD_H)
            add_definitions(-DBADVPN_USE_SIGNALFD)
        else ()
            add_definitions(-DBADVPN_USE_SELFPIPE)
        endif ()

        check_include_files(sys/epoll.h HAVE_SYS_EPOLL_H)
        if (HAVE_SYS_EPOLL_H)
            add_definitions(-DBADVPN_USE_EPOLL)
        else ()
            add_definitions(-DBADVPN_USE_POLL)
        endif ()

        check_include_files(linux/rfkill.h HAVE_LINUX_RFKILL_H)
        if (HAVE_LINUX_RFKILL_H)
            add_definitions(-DBADVPN_USE_LINUX_RFKILL)
            set(BADVPN_USE_LINUX_RFKILL 1)
        endif ()

        check_include_files(linux/input.h HAVE_LINUX_INPUT_H)
        if (HAVE_LINUX_INPUT_H)
            add_definitions(-DBADVPN_USE_LINUX_INPUT)
            set(BADVPN_USE_LINUX_INPUT 1)
        endif ()

        check_include_files(sys/inotify.h HAVE_SYS_INOTIFY_H)
        if (HAVE_SYS_INOTIFY_H)
            add_definitions(-DBADVPN_USE_INOTIFY)
            set(BADVPN_USE_INOTIFY 1)
        endif ()
    elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        add_definitions(-DBADVPN_FREEBSD)

        check_symbol_exists(kqueue "sys/types.h;sys/event.h;sys/time.h" HAVE_KQUEUE)
        if (NOT HAVE_KQUEUE)
            message(FATAL_ERROR "kqueue is required")
        endif ()
        add_definitions(-DBADVPN_USE_KEVENT)
    endif ()

    if (NOT DEFINED BADVPN_WITHOUT_CRYPTODEV)
        check_include_files(crypto/cryptodev.h HAVE_CRYPTO_CRYPTODEV_H)
        if (HAVE_CRYPTO_CRYPTODEV_H)
            add_definitions(-DBADVPN_USE_CRYPTODEV)
        elseif (DEFINED BADVPN_WITH_CRYPTODEV)
            message(FATAL_ERROR "crypto/cryptodev.h not found")
        endif ()
    endif ()
endif ()

# check for syslog
check_include_files(syslog.h HAVE_SYSLOG_H)
if (HAVE_SYSLOG_H)
    add_definitions(-DBADVPN_USE_SYSLOG)
endif ()

# add preprocessor definitions
if (BIG_ENDIAN)
    add_definitions(-DBADVPN_BIG_ENDIAN)
else ()
    add_definitions(-DBADVPN_LITTLE_ENDIAN)
endif ()

# install man pages
install(
    FILES badvpn.7
	DESTINATION ${CMAKE_INSTALL_MANDIR}/man7
)

# reset variables indicating whether we're building various libraries,
# and set them in the respective CMakeLists files. This is used to disable
# building examples and tests which require libraries that are not available.
set(BUILDING_SECURITY 0)
set(BUILDING_DHCPCLIENT 0)
set(BUILDING_ARPPROBE 0)
set(BUILDING_BKIO 0)
set(BUILDING_PREDICATE 0)
set(BUILDING_UDEVMONITOR 0)
set(BUILDING_THREADWORK 0)
set(BUILDING_RANDOM 0)

# Used to register an internal library.
# This will also add a library with the -plugin suffix, which is useful
# for use by dynamic libraries (e.g. NCD modules):
# - If BUILD_SHARED_LIBS is off (default), the libraries ${LIB_NAME} and ${LIB_NAME}-plugin
#   are built separately. Both are static libraries but the -plugin variant is build as position
#   independent code, so it can be (statically) linked into dynamic libraries.
# - If BUILD_SHARED_LIBS is on, only ${LIB_NAME} is built, as a shared library.
#   The ${LIB_NAME}-plugin target is set up as an alias to ${LIB_NAME}.
function(badvpn_add_library LIB_NAME LINK_BADVPN_LIBS LINK_SYS_LIBS LIB_SOURCES)
    set(BADVPN_LIBS_EXEC)
    set(BADVPN_LIBS_PLUGIN)
    foreach(LIB ${LINK_BADVPN_LIBS})
        list(APPEND BADVPN_LIBS_EXEC "${LIB}")
        list(APPEND BADVPN_LIBS_PLUGIN "${LIB}-plugin")
    endforeach()

    add_library("${LIB_NAME}" ${LIB_SOURCES})
    target_link_libraries("${LIB_NAME}" ${BADVPN_LIBS_EXEC} ${LINK_SYS_LIBS})
    set_target_properties("${LIB_NAME}" PROPERTIES OUTPUT_NAME "badvpn-${LIB_NAME}")

    if(WITH_PLUGIN_LIBS)
        if(BUILD_SHARED_LIBS)
            add_library("${LIB_NAME}-plugin" ALIAS "${LIB_NAME}")
        else()
            add_library("${LIB_NAME}-plugin" STATIC ${LIB_SOURCES})
            target_link_libraries("${LIB_NAME}-plugin" ${BADVPN_LIBS_PLUGIN} ${LINK_SYS_LIBS})
            set_target_properties("${LIB_NAME}-plugin" PROPERTIES OUTPUT_NAME "badvpn-${LIB_NAME}-plugin")
            set_target_properties("${LIB_NAME}-plugin" PROPERTIES POSITION_INDEPENDENT_CODE YES)
            set_target_properties("${LIB_NAME}-plugin" PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -DBADVPN_PLUGIN")
        endif()
    endif()
endfunction()

# internal libraries
add_subdirectory(base)
add_subdirectory(system)
add_subdirectory(flow)
add_subdirectory(flowextra)
if (OpenSSL_FOUND)
    set(BUILDING_SECURITY 1)
    add_subdirectory(security)
endif ()
if (NSS_FOUND)
    add_subdirectory(nspr_support)
endif ()
if (BUILD_CLIENT OR BUILDING_SECURITY)
    set(BUILDING_THREADWORK 1)
    add_subdirectory(threadwork)
endif ()
if (BUILD_CLIENT OR BUILD_TUN2SOCKS)
    add_subdirectory(tuntap)
endif ()
if (BUILD_SERVER)
    set(BUILDING_PREDICATE 1)
    add_subdirectory(predicate)
endif ()
if (BUILD_CLIENT OR BUILD_FLOODER)
    add_subdirectory(server_connection)
endif ()
if (BUILD_NCD AND NOT EMSCRIPTEN)
    set(BUILDING_DHCPCLIENT 1)
    set(BUILDING_ARPPROBE 1)
    set(BUILDING_UDEVMONITOR 1)
    set(BUILDING_RANDOM 1)
    add_subdirectory(stringmap)
    add_subdirectory(udevmonitor)
    add_subdirectory(dhcpclient)
    add_subdirectory(arpprobe)
    add_subdirectory(random)
endif ()
if (BUILD_TUN2SOCKS)
    add_subdirectory(socksclient)
    add_subdirectory(udpgw_client)
    add_subdirectory(lwip)
endif ()

# example programs
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

# tests
if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()

# server
if (BUILD_SERVER)
    add_subdirectory(server)
endif ()

# client
if (BUILD_CLIENT)
    add_subdirectory(client)
endif ()

# flooder
if (BUILD_FLOODER)
    add_subdirectory(flooder)
endif ()

# tun2socks
if (BUILD_TUN2SOCKS)
    add_subdirectory(tun2socks)
endif ()

# udpgw
if (BUILD_UDPGW)
    add_subdirectory(udpgw)
endif ()

# ncd
if (BUILD_NCD)
    add_subdirectory(ncd)
    if (NOT EMSCRIPTEN)
        add_subdirectory(ncd-request)
    endif ()
endif ()

# dostest
if (BUILD_DOSTEST)
    add_subdirectory(dostest)
endif ()

message(STATUS "Building components:")

# print what we're building and what not
foreach (name ${BUILD_COMPONENTS})
    # to lower name
    string(TOLOWER "${name}" name_withspaces)

    # append spaces to name
    #while (TRUE)
    #    string(LENGTH "${name_withspaces}" length)
    #    if (NOT (length LESS 12))
    #        break()
    #    endif ()
    #    set(name_withspaces "${name_withspaces} ")
    #endwhile ()
    
    # determine if we're building
    if (BUILD_${name})
        set(building "yes")
    else ()
        set(building "no")
    endif ()
    
    message(STATUS "    ${name_withspaces} ${building}")
endforeach ()
