add_executable(badvpn-udpgw
    udpgw.c
)
target_link_libraries(badvpn-udpgw system flow flowextra)

install(
    TARGETS badvpn-udpgw
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
