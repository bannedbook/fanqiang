add_executable(badvpn-client
    client.c
    StreamPeerIO.c
    DatagramPeerIO.c
    PasswordListener.c
    DataProto.c
    FrameDecider.c
    DPRelay.c
    DPReceive.c
    FragmentProtoDisassembler.c
    FragmentProtoAssembler.c
    SPProtoEncoder.c
    SPProtoDecoder.c
    DataProtoKeepaliveSource.c
    PeerChat.c
    SCOutmsgEncoder.c
    SimpleStreamBuffer.c
    SinglePacketSource.c
)
target_link_libraries(badvpn-client system flow flowextra tuntap server_conection security threadwork ${NSPR_LIBRARIES} ${NSS_LIBRARIES})

install(
    TARGETS badvpn-client
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    FILES badvpn-client.8
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man8
)
