prefix=@prefix@
exec_prefix=${prefix}/@CMAKE_INSTALL_BINDIR@
libdir=${exec_prefix}/@CMAKE_INSTALL_FULL_LIBDIR@
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@
sharedir=${prefix}/@CMAKE_INSTALL_DATAROOTDIR@
mandir=${prefix}/@CMAKE_INSTALL_MANDIR@

Name: @PROJECT_NAME@
Description: @PROJECT_DESC@
URL: @PROJECT_URL@
Version: @PROJECT_VERSION@
Requires:
Cflags: -I${includedir}
Libs: -L${libdir} -lshadowsocks-libev -lcrypto
