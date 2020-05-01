set(FLOWEXTRA_SOURCES
    PacketPassInactivityMonitor.c
    KeepaliveIO.c
)
badvpn_add_library(flowextra "flow;system" "" "${FLOWEXTRA_SOURCES}")
