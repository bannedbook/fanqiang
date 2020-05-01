add_executable(gen_random_havege gen_random_havege.c)
target_link_libraries(gen_random_havege mbedtls)

add_executable(gen_random_ctr_drbg gen_random_ctr_drbg.c)
target_link_libraries(gen_random_ctr_drbg mbedtls)

add_executable(gen_entropy gen_entropy.c)
target_link_libraries(gen_entropy mbedtls)

install(TARGETS gen_random_havege gen_random_ctr_drbg gen_entropy
        DESTINATION "bin"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
