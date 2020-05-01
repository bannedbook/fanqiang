add_executable(badvpn-ncd-request
    ncd-request.c
)
target_link_libraries(badvpn-ncd-request ncdrequest ncdvalgenerator ncdvalparser)

install(
    TARGETS badvpn-ncd-request
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
