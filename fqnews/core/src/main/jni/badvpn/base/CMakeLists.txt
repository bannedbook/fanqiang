set(BASE_ADDITIONAL_SOURCES)

if (HAVE_SYSLOG_H)
    list(APPEND BASE_ADDITIONAL_SOURCES BLog_syslog.c)
endif ()

set(BASE_SOURCES
    DebugObject.c
    BLog.c
    BPending.c
    ${BASE_ADDITIONAL_SOURCES}
)
badvpn_add_library(base "" "" "${BASE_SOURCES}")
