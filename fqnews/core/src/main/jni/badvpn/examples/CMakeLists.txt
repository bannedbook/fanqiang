if (NOT EMSCRIPTEN)
    add_executable(btimer_example btimer_example.c)
    target_link_libraries(btimer_example system)
endif ()

if (BUILDING_PREDICATE)
    add_executable(predicate_test predicate_test.c)
    target_link_libraries(predicate_test predicate)
endif ()

if (NOT EMSCRIPTEN)
    add_executable(fairqueue_test fairqueue_test.c)
    target_link_libraries(fairqueue_test system flow)
endif ()

add_executable(indexedlist_test indexedlist_test.c)

if (BUILDING_SECURITY)
    add_executable(fairqueue_test2 fairqueue_test2.c)
    target_link_libraries(fairqueue_test2 system flow security)

    add_executable(bavl_test bavl_test.c)
    target_link_libraries(bavl_test security)

    add_executable(savl_test savl_test.c)
    target_link_libraries(savl_test security)

    add_executable(bencryption_bench bencryption_bench.c)
    target_link_libraries(bencryption_bench system security)
endif ()

if (BUILD_NCD)
    add_executable(ncd_tokenizer_test ncd_tokenizer_test.c)
    target_link_libraries(ncd_tokenizer_test ncdtokenizer)

    add_executable(ncd_parser_test ncd_parser_test.c)
    target_link_libraries(ncd_parser_test ncdconfigparser ncdvalgenerator ncdsugar)

    add_executable(ncd_value_parser_test ncd_value_parser_test.c)
    target_link_libraries(ncd_value_parser_test ncdvalparser ncdvalgenerator)

    if (NOT EMSCRIPTEN)
        add_executable(ncdinterfacemonitor_test ncdinterfacemonitor_test.c)
        target_link_libraries(ncdinterfacemonitor_test ncdinterfacemonitor)
    endif ()

    add_executable(ncdval_test ncdval_test.c)
    target_link_libraries(ncdval_test ncdval)
    
    add_executable(ncdvalcons_test ncdvalcons_test.c)
    target_link_libraries(ncdvalcons_test ncdvalcons ncdvalgenerator)
endif ()

if (BUILDING_UDEVMONITOR)
    add_executable(ncdudevmonitor_test ncdudevmonitor_test.c)
    target_link_libraries(ncdudevmonitor_test udevmonitor)

    add_executable(ncdudevmanager_test ncdudevmanager_test.c)
    target_link_libraries(ncdudevmanager_test udevmonitor)
endif ()

if (NOT WIN32 AND NOT EMSCRIPTEN)
    add_executable(bprocess_example bprocess_example.c)
    target_link_libraries(bprocess_example system)

    add_executable(stdin_input stdin_input.c)
    target_link_libraries(stdin_input system flow flowextra)
endif ()

if (BUILDING_DHCPCLIENT)
    add_executable(dhcpclient_test dhcpclient_test.c)
    target_link_libraries(dhcpclient_test dhcpclient)
endif ()

if (BUILDING_ARPPROBE)
    add_executable(arpprobe_test arpprobe_test.c)
    target_link_libraries(arpprobe_test arpprobe)
endif ()

add_executable(substring_test substring_test.c)

if (NOT WIN32)
    add_executable(ipaddr6_test ipaddr6_test.c)
    add_executable(parse_number_test parse_number_test.c)
endif ()

if (BUILDING_RANDOM)
    add_executable(brandom2_test brandom2_test.c)
    target_link_libraries(brandom2_test badvpn_random)
endif ()

add_executable(cavl_test cavl_test.c)

if (EMSCRIPTEN)
    add_executable(emscripten_test emscripten_test.c)
    target_link_libraries(emscripten_test system)
endif ()
