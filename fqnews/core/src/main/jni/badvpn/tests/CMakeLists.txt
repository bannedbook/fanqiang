add_executable(chunkbuffer2_test chunkbuffer2_test.c)

add_executable(bproto_test bproto_test.c)

if (BUILDING_THREADWORK)
    add_executable(threadwork_test threadwork_test.c)
    target_link_libraries(threadwork_test threadwork)
endif ()
