set(THREADS_USE_PTHREADS_WIN32 true)
find_package(Threads)

set(libs
    mbedtls
)

set(targets
    dtls_client
    dtls_server
    ssl_client1
    ssl_client2
    ssl_server
    ssl_fork_server
    ssl_mail_client
    mini_client
)

if(USE_PKCS11_HELPER_LIBRARY)
    set(libs ${libs} pkcs11-helper)
endif(USE_PKCS11_HELPER_LIBRARY)

if(ENABLE_ZLIB_SUPPORT)
    set(libs ${libs} ${ZLIB_LIBRARIES})
endif(ENABLE_ZLIB_SUPPORT)

add_executable(dtls_client dtls_client.c)
target_link_libraries(dtls_client ${libs})

add_executable(dtls_server dtls_server.c)
target_link_libraries(dtls_server ${libs})

add_executable(ssl_client1 ssl_client1.c)
target_link_libraries(ssl_client1 ${libs})

add_executable(ssl_client2 ssl_client2.c)
target_sources(ssl_client2 PUBLIC query_config.c)
target_link_libraries(ssl_client2 ${libs})

add_executable(ssl_server ssl_server.c)
target_link_libraries(ssl_server ${libs})

add_executable(ssl_server2 ssl_server2.c)
target_sources(ssl_server2 PUBLIC query_config.c)
target_link_libraries(ssl_server2 ${libs})

add_executable(ssl_fork_server ssl_fork_server.c)
target_link_libraries(ssl_fork_server ${libs})

add_executable(ssl_mail_client ssl_mail_client.c)
target_link_libraries(ssl_mail_client ${libs})

add_executable(mini_client mini_client.c)
target_link_libraries(mini_client ${libs})

if(THREADS_FOUND)
    add_executable(ssl_pthread_server ssl_pthread_server.c)
    target_link_libraries(ssl_pthread_server ${libs} ${CMAKE_THREAD_LIBS_INIT})
    set(targets ${targets} ssl_pthread_server)
endif(THREADS_FOUND)

install(TARGETS ${targets}
        DESTINATION "bin"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
