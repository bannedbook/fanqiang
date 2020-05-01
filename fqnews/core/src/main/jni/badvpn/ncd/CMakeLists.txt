include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(NCD_ADDITIONAL_SOURCES)
set(NCD_ADDITIONAL_LIBS)

if (NOT EMSCRIPTEN)
    if (BADVPN_USE_LINUX_RFKILL)
        list(APPEND NCD_ADDITIONAL_SOURCES
            extra/NCDRfkillMonitor.c
            modules/net_backend_rfkill.c
        )
    endif ()

    if (BADVPN_USE_LINUX_INPUT)
        list(APPEND NCD_ADDITIONAL_SOURCES
            modules/sys_evdev.c
        )
    endif ()

    if (BADVPN_USE_INOTIFY)
        list(APPEND NCD_ADDITIONAL_SOURCES
            modules/sys_watch_directory.c
        )
    endif ()

    badvpn_add_library(ncdinterfacemonitor "base;system" "" extra/NCDInterfaceMonitor.c)
    
    badvpn_add_library(ncdrequest "base;system;ncdvalgenerator;ncdvalparser" "" extra/NCDRequestClient.c)
    
    list(APPEND NCD_ADDITIONAL_SOURCES
        extra/NCDIfConfig.c
        extra/build_cmdline.c
        extra/NCDBProcessOpts.c
        extra/NCDFastNames.c
        modules/command_template.c
        modules/event_template.c
        modules/regex_match.c
        modules/run.c
        modules/runonce.c
        modules/daemon.c
        modules/net_backend_waitdevice.c
        modules/net_backend_waitlink.c
        modules/net_backend_badvpn.c
        modules/net_backend_wpa_supplicant.c
        modules/net_up.c
        modules/net_dns.c
        modules/net_iptables.c
        modules/net_ipv4_addr.c
        modules/net_ipv4_route.c
        modules/net_ipv4_dhcp.c
        modules/net_ipv4_arp_probe.c
        modules/net_watch_interfaces.c
        modules/sys_watch_input.c
        modules/sys_watch_usb.c
        modules/sys_request_server.c
        modules/net_ipv6_wait_dynamic_addr.c
        modules/sys_request_client.c
        modules/reboot.c
        modules/net_ipv6_addr.c
        modules/net_ipv6_route.c
        modules/socket.c
        modules/sys_start_process.c
        modules/load_module.c
    )
    
    list(APPEND NCD_ADDITIONAL_LIBS
        dhcpclient arpprobe ncdinterfacemonitor ncdrequest udevmonitor badvpn_random dl
    )
endif ()

badvpn_add_library(ncdtokenizer "base" "" NCDConfigTokenizer.c)

badvpn_add_library(ncdstringindex "base" "" NCDStringIndex.c)

badvpn_add_library(ncdval "base;ncdstringindex" "" NCDVal.c)

badvpn_add_library(ncdvalgenerator "base;ncdval" "" NCDValGenerator.c)

badvpn_add_library(ncdvalparser "base;ncdval;ncdtokenizer;ncdvalcons" "" NCDValParser.c)

badvpn_add_library(ncdast "" "" NCDAst.c)

badvpn_add_library(ncdconfigparser "base;ncdtokenizer;ncdast" "" NCDConfigParser.c)

badvpn_add_library(ncdsugar "ncdast" "" NCDSugar.c)

badvpn_add_library(ncdvalcons "ncdval" "" NCDValCons.c)

badvpn_add_library(ncdbuildprogram "base;ncdast;ncdconfigparser" "" NCDBuildProgram.c)

badvpn_add_library(ncdobject "" "" NCDObject.c)

badvpn_add_library(ncdmodule "base;ncdobject;ncdstringindex;ncdval" "" NCDModule.c)

set(NCDINTERPRETER_SOURCES
    NCDInterpreter.c
    NCDModuleIndex.c
    NCDInterpProcess.c
    NCDInterpProg.c
    NCDEvaluator.c
    NCDMethodIndex.c
    extra/BEventLock.c
    extra/NCDBuf.c
    extra/value_utils.c
    extra/NCDRefString.c
    modules/var.c
    modules/list.c
    modules/depend.c
    modules/multidepend.c
    modules/dynamic_depend.c
    modules/concat.c
    modules/if.c
    modules/strcmp.c
    modules/logical.c
    modules/sleep.c
    modules/print.c
    modules/blocker.c
    modules/spawn.c
    modules/imperative.c
    modules/ref.c
    modules/index.c
    modules/alias.c
    modules/process_manager.c
    modules/ondemand.c
    modules/foreach.c
    modules/choose.c
    modules/from_string.c
    modules/to_string.c
    modules/value.c
    modules/try.c
    modules/exit.c
    modules/getargs.c
    modules/arithmetic.c
    modules/parse.c
    modules/valuemetic.c
    modules/file.c
    modules/netmask.c
    modules/implode.c
    modules/call2.c
    modules/assert.c
    modules/explode.c
    modules/net_ipv4_addr_in_network.c
    modules/net_ipv6_addr_in_network.c
    modules/timer.c
    modules/file_open.c
    modules/backtrack.c
    modules/depend_scope.c
    modules/substr.c
    modules/log.c
    modules/getenv.c
    modules/basic_functions.c
    modules/objref.c
    ${NCD_ADDITIONAL_SOURCES}
)
set(NCDINTERPRETER_LIBS
    base system flow flowextra ncdval ncdstringindex ncdvalgenerator ncdvalparser
    ncdconfigparser ncdsugar ncdobject ncdmodule ${NCD_ADDITIONAL_LIBS})
badvpn_add_library(ncdinterpreter "${NCDINTERPRETER_LIBS}" "" "${NCDINTERPRETER_SOURCES}")

if (BADVPN_USE_LINUX_INPUT)
    string(REPLACE " " ";" FLAGS_LIST "${CMAKE_C_FLAGS}")
    execute_process(COMMAND ${CMAKE_C_COMPILER} ${FLAGS_LIST} -E ${CMAKE_CURRENT_SOURCE_DIR}/include_linux_input.c
                    RESULT_VARIABLE LINUX_INPUT_PREPROCESS_RESULT
                    OUTPUT_VARIABLE LINUX_INPUT_PREPROCESS_OUTPUT)
    if (NOT LINUX_INPUT_PREPROCESS_RESULT EQUAL 0)
        message(FATAL_ERROR "failed to preprocess linux/input.h include")
    endif ()

    string(REGEX MATCH "\"(/[^\"]+/linux/input.h)\"" LINUX_INPUT_MATCH ${LINUX_INPUT_PREPROCESS_OUTPUT})
    if (NOT LINUX_INPUT_MATCH)
        message(FATAL_ERROR "failed to match preprocessor output for path of linux/input.h")
    endif ()
    set(LINUX_INPUT_H_PATH ${CMAKE_MATCH_1})
    
    string(REGEX MATCH "\"(/[^\"]+/linux/input-event-codes.h)\"" LINUX_INPUT_EVENT_CODES_MATCH ${LINUX_INPUT_PREPROCESS_OUTPUT})
    if (NOT LINUX_INPUT_EVENT_CODES_MATCH)
        set(LINUX_INPUT_EVENT_CODES_H_PATH)
    else ()
        set(LINUX_INPUT_EVENT_CODES_H_PATH ${CMAKE_MATCH_1})
    endif ()
    
    message(STATUS "Generating linux_input_names.h from ${LINUX_INPUT_H_PATH} ${LINUX_INPUT_EVENT_CODES_H_PATH}")

    execute_process(COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/parse_linux_input.sh 
                            ${CMAKE_CURRENT_BINARY_DIR}/linux_input_names.h
                            ${LINUX_INPUT_H_PATH} ${LINUX_INPUT_EVENT_CODES_H_PATH}
                    RESULT_VARIABLE LINUX_INPUT_PARSE_RESULT)
    if (NOT LINUX_INPUT_PARSE_RESULT EQUAL 0)
        message(FATAL_ERROR "failed to generate linux_input_names.h")
    endif ()
endif ()

if (NOT EMSCRIPTEN)
    add_executable(badvpn-ncd ncd.c)
    target_link_libraries(badvpn-ncd ncdinterpreter ncdbuildprogram)
    
    install(
        TARGETS badvpn-ncd
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif ()

if (EMSCRIPTEN)
    add_executable(emncd emncd.c)
    target_link_libraries(emncd ncdinterpreter)
    
    add_custom_command(
        OUTPUT emncd.bc
        DEPENDS emncd
        COMMAND cp emncd emncd.bc
    )
    
    add_custom_command(
        OUTPUT emncd.js
        DEPENDS emncd.bc
        COMMAND
        ${CMAKE_C_COMPILER} emncd.bc -o emncd.js -O2
        -s EXPORTED_FUNCTIONS=\"['_breactor_timer_cb','_main','_emncd_start','_emncd_stop']\"
    )
    
    add_custom_target(emncd_js ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/emncd.js)
endif ()
