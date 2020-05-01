if USE_SYSTEM_SHARED_LIB
SUBDIRS = src
else
SUBDIRS = libcork libipset libbloom src
endif

if ENABLE_DOCUMENTATION
SUBDIRS += doc
endif

ACLOCAL_AMFLAGS = -I m4

pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = shadowsocks-libev.pc

EXTRA_DIST = acl Changes completions debian docker rpm scripts README.md
EXTRA_DIST += libbloom
EXTRA_DIST += libcork/include libipset/include
EXTRA_DIST += libipset/src/libipset/map/inspection-template.c.in
EXTRA_DIST += libipset/src/libipset/set/inspection-template.c.in
