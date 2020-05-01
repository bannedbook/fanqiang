set(libs
    mbedtls
)

if(USE_PKCS11_HELPER_LIBRARY)
    set(libs ${libs} pkcs11-helper)
endif(USE_PKCS11_HELPER_LIBRARY)

if(ENABLE_ZLIB_SUPPORT)
    set(libs ${libs} ${ZLIB_LIBRARIES})
endif(ENABLE_ZLIB_SUPPORT)

add_executable(selftest selftest.c)
target_link_libraries(selftest ${libs})

add_executable(benchmark benchmark.c)
target_link_libraries(benchmark ${libs})

if(TEST_CPP)
    add_executable(cpp_dummy_build cpp_dummy_build.cpp)
    target_link_libraries(cpp_dummy_build ${libs})
endif()

add_executable(ssl_cert_test ssl_cert_test.c)
target_link_libraries(ssl_cert_test ${libs})

add_executable(udp_proxy udp_proxy.c)
target_link_libraries(udp_proxy ${libs})

add_executable(zeroize zeroize.c)
target_link_libraries(zeroize ${libs})

add_executable(query_compile_time_config query_compile_time_config.c)
target_sources(query_compile_time_config PUBLIC ../ssl/query_config.c)
target_link_libraries(query_compile_time_config ${libs})

install(TARGETS selftest benchmark ssl_cert_test udp_proxy query_compile_time_config
        DESTINATION "bin"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
