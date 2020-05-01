set(UDEVMONITOR_SOURCES
    NCDUdevMonitorParser.c
    NCDUdevMonitor.c
    NCDUdevCache.c
    NCDUdevManager.c
)
badvpn_add_library(udevmonitor "system;flow;stringmap" "" "${UDEVMONITOR_SOURCES}")
