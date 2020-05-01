VERSION_INFO = 2:0:0

AM_CFLAGS = -g -O2 -Wall -Werror -Wno-deprecated-declarations -fno-strict-aliasing -std=gnu99 -D_GNU_SOURCE
AM_CFLAGS += $(PTHREAD_CFLAGS)
if !USE_SYSTEM_SHARED_LIB
AM_CFLAGS += -I$(top_srcdir)/libbloom
AM_CFLAGS += -I$(top_srcdir)/libipset/include
AM_CFLAGS += -I$(top_srcdir)/libcork/include
endif
AM_CFLAGS += $(LIBPCRE_CFLAGS)

SS_COMMON_LIBS = $(INET_NTOP_LIB) $(LIBPCRE_LIBS)
if !USE_SYSTEM_SHARED_LIB
SS_COMMON_LIBS += $(top_builddir)/libbloom/libbloom.la \
                  $(top_builddir)/libipset/libipset.la \
                  $(top_builddir)/libcork/libcork.la
else
SS_COMMON_LIBS += -lbloom -lcork -lcorkipset
endif
SS_COMMON_LIBS += -lev -lsodium -lm

bin_PROGRAMS = ss-local ss-tunnel ss-server
if !BUILD_WINCOMPAT
bin_PROGRAMS += ss-manager
endif

sni_src = http.c \
          tls.c

acl_src = rule.c \
          acl.c

crypto_src = crypto.c \
             aead.c \
             stream.c \
             ppbloom.c \
             base64.c

plugin_src = plugin.c

common_src = utils.c \
             jconf.c \
             json.c \
             udprelay.c \
             cache.c \
             netutils.c

if BUILD_WINCOMPAT
common_src += winsock.c
endif

ss_local_SOURCES = local.c \
                   $(common_src) \
                   $(crypto_src) \
                   $(plugin_src) \
                   $(sni_src) \
                   $(acl_src)

ss_tunnel_SOURCES = tunnel.c \
                    $(common_src) \
                    $(crypto_src) \
                    $(plugin_src)

ss_server_SOURCES = resolv.c \
                    server.c \
                    $(common_src) \
                    $(crypto_src) \
                    $(plugin_src) \
                    $(sni_src) \
                    ${acl_src}

ss_manager_SOURCES = utils.c \
                     jconf.c \
                     json.c \
                     netutils.c \
                     manager.c

ss_local_LDADD = $(SS_COMMON_LIBS)
ss_tunnel_LDADD = $(SS_COMMON_LIBS)
ss_server_LDADD = $(SS_COMMON_LIBS)
ss_manager_LDADD = $(SS_COMMON_LIBS)
ss_local_LDADD += -lcares
ss_tunnel_LDADD += -lcares
ss_server_LDADD += -lcares
ss_manager_LDADD += -lcares

ss_local_CFLAGS = $(AM_CFLAGS) -DMODULE_LOCAL
ss_tunnel_CFLAGS = $(AM_CFLAGS) -DMODULE_TUNNEL
ss_server_CFLAGS = $(AM_CFLAGS) -DMODULE_REMOTE
ss_manager_CFLAGS = $(AM_CFLAGS) -DMODULE_MANAGER

if BUILD_REDIRECTOR
bin_SCRIPTS = ss-nat
bin_PROGRAMS += ss-redir
ss_redir_SOURCES = utils.c \
                   jconf.c \
                   json.c \
                   netutils.c \
                   cache.c \
                   udprelay.c \
                   redir.c \
                   $(crypto_src) \
                   $(plugin_src)

ss_redir_CFLAGS = $(AM_CFLAGS) -DMODULE_REDIR
ss_redir_LDADD = $(SS_COMMON_LIBS)
ss_redir_LDADD += -lcares
endif

lib_LTLIBRARIES = libshadowsocks-libev.la
libshadowsocks_libev_la_SOURCES = $(ss_local_SOURCES)
libshadowsocks_libev_la_CFLAGS = $(ss_local_CFLAGS) -DLIB_ONLY
libshadowsocks_libev_la_LDFLAGS = -version-info $(VERSION_INFO)
libshadowsocks_libev_la_LIBADD = $(ss_local_LDADD)
include_HEADERS = shadowsocks.h

noinst_HEADERS = acl.h crypto.h stream.h aead.h json.h netutils.h redir.h server.h tls.h uthash.h \
                 cache.h http.h local.h plugin.h resolv.h tunnel.h utils.h base64.h ppbloom.h \
                 common.h jconf.h manager.h protocol.h rule.h socks5.h udprelay.h winsock.h
EXTRA_DIST = ss-nat
