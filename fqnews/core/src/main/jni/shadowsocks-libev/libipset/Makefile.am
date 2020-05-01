noinst_LTLIBRARIES = libipset.la

bdd_src = src/libipset/bdd/assignments.c \
		  src/libipset/bdd/basics.c \
		  src/libipset/bdd/bdd-iterator.c \
		  src/libipset/bdd/expanded.c \
		  src/libipset/bdd/reachable.c \
		  src/libipset/bdd/read.c \
		  src/libipset/bdd/write.c 

map_src = src/libipset/map/allocation.c \
		  src/libipset/map/inspection.c \
		  src/libipset/map/ipv4_map.c \
		  src/libipset/map/ipv6_map.c \
		  src/libipset/map/storage.c

set_src = src/libipset/set/allocation.c \
		  src/libipset/set/inspection.c \
		  src/libipset/set/ipv4_set.c \
		  src/libipset/set/ipv6_set.c \
		  src/libipset/set/iterator.c \
		  src/libipset/set/storage.c

libipset_la_SOURCES = src/libipset/general.c ${bdd_src} ${map_src} ${set_src}
libipset_la_CFLAGS = -I$(top_srcdir)/libipset/include -I$(top_srcdir)/libcork/include

libipset_la_LDFLAGS = -static
