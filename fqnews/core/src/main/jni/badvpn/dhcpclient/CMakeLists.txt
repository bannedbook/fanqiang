badvpn_add_library(dhcpclientcore "system;flow;flowextra;badvpn_random" "" BDHCPClientCore.c)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DHCPCLIENT_SOURCES
        BDHCPClient.c
        DHCPIpUdpEncoder.c
        DHCPIpUdpDecoder.c
    )
    badvpn_add_library(dhcpclient "system;flow;dhcpclientcore" "" "${DHCPCLIENT_SOURCES}")
endif ()
