add_executable(badvpn-flooder flooder.c)
target_link_libraries(badvpn-flooder system flow server_conection ${NSPR_LIBRARIES} ${NSS_LIBRARIES})

install(
    TARGETS badvpn-flooder
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
