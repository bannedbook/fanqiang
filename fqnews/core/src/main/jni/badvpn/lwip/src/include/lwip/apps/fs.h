/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_HDR_APPS_FS_H
#define LWIP_HDR_APPS_FS_H

#include "httpd_opts.h"
#include "lwip/err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FS_READ_EOF     -1
#define FS_READ_DELAYED -2

#if HTTPD_PRECALCULATED_CHECKSUM
struct fsdata_chksum {
  u32_t offset;
  u16_t chksum;
  u16_t len;
};
#endif /* HTTPD_PRECALCULATED_CHECKSUM */

#define FS_FILE_FLAGS_HEADER_INCLUDED     0x01
#define FS_FILE_FLAGS_HEADER_PERSISTENT   0x02

/** Define FS_FILE_EXTENSION_T_DEFINED if you have typedef'ed to your private
 * pointer type (defaults to 'void' so the default usage is 'void*')
 */
#ifndef FS_FILE_EXTENSION_T_DEFINED
typedef void fs_file_extension;
#endif

struct fs_file {
  const char *data;
  int len;
  int index;
  /* pextension is free for implementations to hold private (extensional)
     arbitrary data, e.g. holding some file state or file system handle */
  fs_file_extension *pextension;
#if HTTPD_PRECALCULATED_CHECKSUM
  const struct fsdata_chksum *chksum;
  u16_t chksum_count;
#endif /* HTTPD_PRECALCULATED_CHECKSUM */
  u8_t flags;
#if LWIP_HTTPD_CUSTOM_FILES
  u8_t is_custom_file;
#endif /* LWIP_HTTPD_CUSTOM_FILES */
#if LWIP_HTTPD_FILE_STATE
  void *state;
#endif /* LWIP_HTTPD_FILE_STATE */
};

#if LWIP_HTTPD_FS_ASYNC_READ
typedef void (*fs_wait_cb)(void *arg);
#endif /* LWIP_HTTPD_FS_ASYNC_READ */

err_t fs_open(struct fs_file *file, const char *name);
void fs_close(struct fs_file *file);
#if LWIP_HTTPD_DYNAMIC_FILE_READ
#if LWIP_HTTPD_FS_ASYNC_READ
int fs_read_async(struct fs_file *file, char *buffer, int count, fs_wait_cb callback_fn, void *callback_arg);
#else /* LWIP_HTTPD_FS_ASYNC_READ */
int fs_read(struct fs_file *file, char *buffer, int count);
#endif /* LWIP_HTTPD_FS_ASYNC_READ */
#endif /* LWIP_HTTPD_DYNAMIC_FILE_READ */
#if LWIP_HTTPD_FS_ASYNC_READ
int fs_is_file_ready(struct fs_file *file, fs_wait_cb callback_fn, void *callback_arg);
#endif /* LWIP_HTTPD_FS_ASYNC_READ */
int fs_bytes_left(struct fs_file *file);

#if LWIP_HTTPD_FILE_STATE
/** This user-defined function is called when a file is opened. */
void *fs_state_init(struct fs_file *file, const char *name);
/** This user-defined function is called when a file is closed. */
void fs_state_free(struct fs_file *file, void *state);
#endif /* #if LWIP_HTTPD_FILE_STATE */

struct fsdata_file {
  const struct fsdata_file *next;
  const unsigned char *name;
  const unsigned char *data;
  int len;
  u8_t flags;
#if HTTPD_PRECALCULATED_CHECKSUM
  u16_t chksum_count;
  const struct fsdata_chksum *chksum;
#endif /* HTTPD_PRECALCULATED_CHECKSUM */
};

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_APPS_FS_H */
