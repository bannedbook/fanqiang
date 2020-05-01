# redir need linux/* stuff
if (LINUX)
    option(WITH_SS_REDIR "Build ss-redir" ON)
else ()
    option(WITH_SS_REDIR "Build ss-redir" OFF)
endif ()


set(SS_SNI_SOURCE
        http.c
        tls.c
        )

set(SS_ACL_SOURCE
        acl.c
        rule.c
        )

set(SS_CRYPTO_SOURCE
        crypto.c
        aead.c
        stream.c
        base64.c
        )

set(SS_PLUGIN_SOURCE
        plugin.c
        )

set(SS_SHARED_SOURCES
        ppbloom.c
        utils.c
        jconf.c
        json.c
        netutils.c
        )

set(LIBSHADOWSOCKS_LIBEV_SOURCE
        ${SS_SHARED_SOURCES}
        udprelay.c
        cache.c
        local.c
        ${SS_CRYPTO_SOURCE}
        ${SS_PLUGIN_SOURCE}
        ${SS_SNI_SOURCE}
        ${SS_ACL_SOURCE}
        )

set(SS_LOCAL_SOURCE
        ${LIBSHADOWSOCKS_LIBEV_SOURCE}

        )

set(SS_TUNNEL_SOURCE
        ${SS_SHARED_SOURCES}
        udprelay.c
        cache.c
        tunnel.c
        ${SS_CRYPTO_SOURCE}
        ${SS_PLUGIN_SOURCE}
        )

set(SS_SERVER_SOURCE
        ${SS_SHARED_SOURCES}
        udprelay.c
        cache.c
        resolv.c
        server.c
        ${SS_CRYPTO_SOURCE}
        ${SS_PLUGIN_SOURCE}
        ${SS_SNI_SOURCE}
        ${SS_ACL_SOURCE}
        )

set(SS_MANAGER_SOURCE
        ${SS_SHARED_SOURCES}
        manager.c
        )

set(SS_REDIR_SOURCE
        ${SS_SHARED_SOURCES}
        udprelay.c
        cache.c
        redir.c
        ${SS_CRYPTO_SOURCE}
        ${SS_PLUGIN_SOURCE}
        )

if (CMAKE_SYSTEM_NAME STREQUAL Darwin)
find_path(LIBSODIUM_INCLUDE_DIR sodium.h
        PATHS
        $ENV{LIBSODIUM_INCLUDE_DIR}
        $ENV{LIBSODIUM_DIR}/include
        /usr/local/libsodium/include
        /opt/libsodium/include
        /usr/local/include
)
include_directories(${LIBSODIUM_INCLUDE_DIR})
endif ()

if (WITH_STATIC)
find_library(LIBSODIUM libsodium.a)
find_library(LIBMBEDTLS libmbedtls.a)
find_library(LIBMBEDCRYPTO libmbedcrypto.a)
find_library(LIBEV libev.a)
find_library(LIBUDNS libcares.a)
find_library(LIBPCRE libpcre.a)

# Dependencies we need for static and shared
list(APPEND DEPS
        m
        bloom
        ${LIBEV}
        ${LIBUDNS}
        ${LIBPCRE}
        ${LIBSODIUM}
        ${LIBMBEDTLS}
        ${LIBMBEDCRYPTO}
        )
endif ()

find_library(LIBSODIUM_SHARED sodium)
find_library(LIBMBEDTLS_SHARED mbedtls)
find_library(LIBMBEDCRYPTO_SHARED mbedcrypto)
find_library(LIBEV_SHARED ev)
find_library(LIBUDNS_SHARED cares)
find_library(LIBPCRE_SHARED pcre)

if (WITH_EMBEDDED_SRC)
list(APPEND DEPS_SHARED
        m
        bloom-shared
        cork-shared
        ipset-shared
        ${LIBEV_SHARED}
        ${LIBUDNS_SHARED}
        ${LIBPCRE_SHARED}
        ${LIBSODIUM_SHARED}
        ${LIBMBEDTLS_SHARED}
        ${LIBMBEDCRYPTO_SHARED}
        )
else ()
find_library(LIBBLOOM_SHARED bloom)
find_library(LIBCORK_SHARED cork)
find_library(LIBCORKIPSET_SHARED corkipset)
list(APPEND DEPS_SHARED
        m
        ${LIBBLOOM_SHARED}
        ${LIBCORK_SHARED}
        ${LIBCORKIPSET_SHARED}
        ${LIBEV_SHARED}
        ${LIBUDNS_SHARED}
        ${LIBPCRE_SHARED}
        ${LIBSODIUM_SHARED}
        ${LIBMBEDTLS_SHARED}
        ${LIBMBEDCRYPTO_SHARED}
        )
endif ()

find_package (Threads)

if (WITH_STATIC)
# ------------------------------------------------------------------
# Static
# By default we use normal name for static, all shared targets will add a `-shared' suffix
add_executable(ss-server ${SS_SERVER_SOURCE})
add_executable(ss-tunnel ${SS_TUNNEL_SOURCE})
add_executable(ss-manager ${SS_MANAGER_SOURCE})
add_executable(ss-local ${SS_LOCAL_SOURCE})
if (WITH_SS_REDIR)
    add_executable(ss-redir ${SS_REDIR_SOURCE})
else ()
    add_executable(ss-redir EXCLUDE_FROM_ALL ${SS_REDIR_SOURCE})
endif ()
add_library(shadowsocks-libev STATIC ${LIBSHADOWSOCKS_LIBEV_SOURCE})


target_compile_definitions(ss-server PUBLIC -DMODULE_REMOTE)
target_compile_definitions(ss-tunnel PUBLIC -DMODULE_TUNNEL)
target_compile_definitions(ss-manager PUBLIC -DMODULE_MANAGER)
target_compile_definitions(ss-local PUBLIC -DMODULE_LOCAL)
target_compile_definitions(ss-redir PUBLIC -DMODULE_REDIR)
target_compile_definitions(shadowsocks-libev PUBLIC -DMODULE_LOCAL -DLIB_ONLY)

target_link_libraries(ss-server cork ipset ${DEPS})
target_link_libraries(ss-tunnel cork ${DEPS})
target_link_libraries(ss-manager m bloom cork ${LIBEV} ${LIBUDNS})
target_link_libraries(ss-local cork ipset ${DEPS})
target_link_libraries(ss-redir cork ipset ${DEPS})
target_link_libraries(shadowsocks-libev cork ipset ${DEPS})
endif ()

# ------------------------------------------------------------------
# Shared
# Shared target will have a `-shared' suffix
# All shared dependency also have `-shared' suffix
# For shared binary, we still use the same name as static, without `-shared', but will output to shared directory
add_executable(ss-server-shared ${SS_SERVER_SOURCE})
add_executable(ss-tunnel-shared ${SS_TUNNEL_SOURCE})
add_executable(ss-manager-shared ${SS_MANAGER_SOURCE})
add_executable(ss-local-shared ${SS_LOCAL_SOURCE})
if (WITH_SS_REDIR)
    add_executable(ss-redir-shared ${SS_REDIR_SOURCE})
else ()
    add_executable(ss-redir-shared EXCLUDE_FROM_ALL ${SS_REDIR_SOURCE})
endif ()
add_library(shadowsocks-libev-shared SHARED ${LIBSHADOWSOCKS_LIBEV_SOURCE})

target_compile_definitions(ss-server-shared PUBLIC -DMODULE_REMOTE)
target_compile_definitions(ss-tunnel-shared PUBLIC -DMODULE_TUNNEL)
target_compile_definitions(ss-manager-shared PUBLIC -DMODULE_MANAGER)
target_compile_definitions(ss-local-shared PUBLIC -DMODULE_LOCAL)
target_compile_definitions(ss-redir-shared PUBLIC -DMODULE_REDIR)
target_compile_definitions(shadowsocks-libev-shared PUBLIC -DMODULE_LOCAL)

target_link_libraries(ss-server-shared ${DEPS_SHARED})
target_link_libraries(ss-tunnel-shared ${DEPS_SHARED})
target_link_libraries(ss-manager-shared ${CMAKE_THREAD_LIBS_INIT} ${LIBEV_SHARED} ${LIBUDNS_SHARED} ${DEPS_SHARED})
target_link_libraries(ss-local-shared ${DEPS_SHARED})
target_link_libraries(ss-redir-shared ${DEPS_SHARED})
target_link_libraries(shadowsocks-libev-shared ${DEPS_SHARED})

set_target_properties(ss-server-shared PROPERTIES OUTPUT_NAME ss-server)
set_target_properties(ss-tunnel-shared PROPERTIES OUTPUT_NAME ss-tunnel)
set_target_properties(ss-manager-shared PROPERTIES OUTPUT_NAME ss-manager)
set_target_properties(ss-local-shared PROPERTIES OUTPUT_NAME ss-local)
set_target_properties(ss-redir-shared PROPERTIES OUTPUT_NAME ss-redir)

set_target_properties(ss-server-shared ss-tunnel-shared ss-manager-shared ss-local-shared ss-redir-shared
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_SHARED_OUTPUT_DIRECTORY}
        )

set_target_properties(shadowsocks-libev-shared PROPERTIES OUTPUT_NAME shadowsocks-libev)
target_compile_definitions(shadowsocks-libev-shared PUBLIC -DMODULE_LOCAL)
target_link_libraries(shadowsocks-libev-shared ${DEPS_SHARED})

# ------------------------------------------------------------------
# Misc
# Recommend to install shared by default
install(DIRECTORY ${RUNTIME_SHARED_OUTPUT_DIRECTORY}/
        USE_SOURCE_PERMISSIONS
        DESTINATION bin)

if (WITH_STATIC)
    install(TARGETS shadowsocks-libev
        ARCHIVE DESTINATION lib)
endif ()

install(TARGETS shadowsocks-libev-shared
        LIBRARY DESTINATION lib)

install(FILES shadowsocks.h DESTINATION include)


add_custom_target(distclean
        COMMAND ${CMAKE_COMMAND} -E echo WARNING: distclean target is not functional
        COMMAND ${CMAKE_COMMAND} -E echo Use 'git clean -fdx' instead
        VERBATIM
        )
