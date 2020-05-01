set(NSPRSUPPORT_SOURCES
    DummyPRFileDesc.c
    BSSLConnection.c
)
badvpn_add_library(nspr_support "system;flow;threadwork" "${NSPR_LIBRARIES};${NSS_LIBRARIES}" "${NSPRSUPPORT_SOURCES}")
