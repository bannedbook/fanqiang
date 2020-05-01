set(SERVERCONNECTION_SOURCES
    ServerConnection.c
    SCKeepaliveSource.c
)
badvpn_add_library(server_conection "system;flow;flowextra;nspr_support" "${NSPR_LIBRARIES};${NSS_LIBRARIES}" "${SERVERCONNECTION_SOURCES}")
