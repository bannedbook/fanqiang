set(BSYSTEM_ADDITIONAL_LIBS)
set(BSYSTEM_ADDITIONAL_SOURCES)

if (NOT EMSCRIPTEN)
    list(APPEND BSYSTEM_ADDITIONAL_SOURCES
        BSignal.c
        BNetwork.c
        BConnection_common.c
    )

    if (WIN32)
        list(APPEND BSYSTEM_ADDITIONAL_LIBS ws2_32 mswsock)
        list(APPEND BSYSTEM_ADDITIONAL_SOURCES
            BConnection_win.c
            BDatagram_win.c
        )
    endif ()

    if (NOT WIN32)
        list(APPEND BSYSTEM_ADDITIONAL_SOURCES
            BUnixSignal.c
            BConnection_unix.c
            BDatagram_unix.c
            BProcess.c
            BInputProcess.c
            BThreadSignal.c
            BLockReactor.c
        )
    endif ()
endif ()

if (BREACTOR_BACKEND STREQUAL "badvpn")
    list(APPEND BSYSTEM_ADDITIONAL_SOURCES BReactor_badvpn.c)
elseif (BREACTOR_BACKEND STREQUAL "glib")
    list(APPEND BSYSTEM_ADDITIONAL_SOURCES BReactor_glib.c)
    list(APPEND BSYSTEM_ADDITIONAL_LIBS ${GLIB2_LIBRARIES})
elseif (BREACTOR_BACKEND STREQUAL "emscripten")
    list(APPEND BSYSTEM_ADDITIONAL_SOURCES BReactor_emscripten.c)
endif ()

set(SYSTEM_SOURCES
    BTime.c
    ${BSYSTEM_ADDITIONAL_SOURCES}
)
badvpn_add_library(system "base;flow" "${BSYSTEM_ADDITIONAL_LIBS}" "${SYSTEM_SOURCES}")
