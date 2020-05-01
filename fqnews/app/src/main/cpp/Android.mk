LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libpolipo
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	atom.c auth.c chunk.c client.c config.c dirent_compat.c \
    	diskcache.c dns.c event.c forbidden.c fts_compat.c \
    	http.c http_parse.c io.c local.c log.c main.c md5.c \
    	mingw.c object.c parse_time.c server.c socks.c \
    	tunnel.c util.c polipocall.cpp

LOCAL_CFLAGS := \
	-DHAS_STDINT_H \
	-DLOCAL_ROOT="\"/sdcard/polipo/www\"" \
	-DDISK_CACHE_ROOT="\"/sdcard/polipo/cache/\"" \
	-Dmain=polipo_main

LOCAL_CFLAGS += \
	-Wno-unused-parameter -Wno-sign-compare

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)