/**
 * @file
 * HTTP server options list
 */

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
 * This version of the file has been modified by Texas Instruments to offer
 * simple server-side-include (SSI) and Common Gateway Interface (CGI)
 * capability.
 */

#ifndef LWIP_HDR_APPS_HTTPD_OPTS_H
#define LWIP_HDR_APPS_HTTPD_OPTS_H

#include "lwip/opt.h"
#include "lwip/prot/iana.h"

/**
 * @defgroup httpd_opts Options
 * @ingroup httpd
 * @{
 */

/** Set this to 1 to support CGI (old style) */
#if !defined LWIP_HTTPD_CGI || defined __DOXYGEN__
#define LWIP_HTTPD_CGI            0
#endif

/** Set this to 1 to support CGI (new style) */
#if !defined LWIP_HTTPD_CGI_SSI || defined __DOXYGEN__
#define LWIP_HTTPD_CGI_SSI        0
#endif

/** Set this to 1 to support SSI (Server-Side-Includes) */
#if !defined LWIP_HTTPD_SSI || defined __DOXYGEN__
#define LWIP_HTTPD_SSI            0
#endif

/** Set this to 1 to implement an SSI tag handler callback that gets a const char*
 * to the tag (instead of an index into a pre-registered array of known tags) */
#if !defined LWIP_HTTPD_SSI_RAW || defined __DOXYGEN__
#define LWIP_HTTPD_SSI_RAW        0
#endif

/** Set this to 1 to support HTTP POST */
#if !defined LWIP_HTTPD_SUPPORT_POST || defined __DOXYGEN__
#define LWIP_HTTPD_SUPPORT_POST   0
#endif

/* The maximum number of parameters that the CGI handler can be sent. */
#if !defined LWIP_HTTPD_MAX_CGI_PARAMETERS || defined __DOXYGEN__
#define LWIP_HTTPD_MAX_CGI_PARAMETERS 16
#endif

/** LWIP_HTTPD_SSI_MULTIPART==1: SSI handler function is called with 2 more
 * arguments indicating a counter for insert string that are too long to be
 * inserted at once: the SSI handler function must then set 'next_tag_part'
 * which will be passed back to it in the next call. */
#if !defined LWIP_HTTPD_SSI_MULTIPART || defined __DOXYGEN__
#define LWIP_HTTPD_SSI_MULTIPART    0
#endif

/* The maximum length of the string comprising the tag name */
#if !defined LWIP_HTTPD_MAX_TAG_NAME_LEN || defined __DOXYGEN__
#define LWIP_HTTPD_MAX_TAG_NAME_LEN 8
#endif

/* The maximum length of string that can be returned to replace any given tag */
#if !defined LWIP_HTTPD_MAX_TAG_INSERT_LEN || defined __DOXYGEN__
#define LWIP_HTTPD_MAX_TAG_INSERT_LEN 192
#endif

#if !defined LWIP_HTTPD_POST_MANUAL_WND || defined __DOXYGEN__
#define LWIP_HTTPD_POST_MANUAL_WND  0
#endif

/** This string is passed in the HTTP header as "Server: " */
#if !defined HTTPD_SERVER_AGENT || defined __DOXYGEN__
#define HTTPD_SERVER_AGENT "lwIP/" LWIP_VERSION_STRING " (http://savannah.nongnu.org/projects/lwip)"
#endif

/** Set this to 1 if you want to include code that creates HTTP headers
 * at runtime. Default is off: HTTP headers are then created statically
 * by the makefsdata tool. Static headers mean smaller code size, but
 * the (readonly) fsdata will grow a bit as every file includes the HTTP
 * header. */
#if !defined LWIP_HTTPD_DYNAMIC_HEADERS || defined __DOXYGEN__
#define LWIP_HTTPD_DYNAMIC_HEADERS 0
#endif

#if !defined HTTPD_DEBUG || defined __DOXYGEN__
#define HTTPD_DEBUG         LWIP_DBG_OFF
#endif

/** Set this to 1 to use a memp pool for allocating 
 * struct http_state instead of the heap.
 */
#if !defined HTTPD_USE_MEM_POOL || defined __DOXYGEN__
#define HTTPD_USE_MEM_POOL  0
#endif

/** The server port for HTTPD to use */
#if !defined HTTPD_SERVER_PORT || defined __DOXYGEN__
#define HTTPD_SERVER_PORT                   LWIP_IANA_PORT_HTTP
#endif

/** The https server port for HTTPD to use */
#if !defined HTTPD_SERVER_PORT_HTTPS || defined __DOXYGEN__
#define HTTPD_SERVER_PORT_HTTPS             LWIP_IANA_PORT_HTTPS
#endif

/** Enable https support? */
#if !defined HTTPD_ENABLE_HTTPS || defined __DOXYGEN__
#define HTTPD_ENABLE_HTTPS                  0
#endif

/** Maximum retries before the connection is aborted/closed.
 * - number of times pcb->poll is called -> default is 4*500ms = 2s;
 * - reset when pcb->sent is called
 */
#if !defined HTTPD_MAX_RETRIES || defined __DOXYGEN__
#define HTTPD_MAX_RETRIES                   4
#endif

/** The poll delay is X*500ms */
#if !defined HTTPD_POLL_INTERVAL || defined __DOXYGEN__
#define HTTPD_POLL_INTERVAL                 4
#endif

/** Priority for tcp pcbs created by HTTPD (very low by default).
 *  Lower priorities get killed first when running out of memory.
 */
#if !defined HTTPD_TCP_PRIO || defined __DOXYGEN__
#define HTTPD_TCP_PRIO                      TCP_PRIO_MIN
#endif

/** Set this to 1 to enable timing each file sent */
#if !defined LWIP_HTTPD_TIMING || defined __DOXYGEN__
#define LWIP_HTTPD_TIMING                   0
#endif
/** Set this to 1 to enable timing each file sent */
#if !defined HTTPD_DEBUG_TIMING || defined __DOXYGEN__
#define HTTPD_DEBUG_TIMING                  LWIP_DBG_OFF
#endif

/** Set this to one to show error pages when parsing a request fails instead
    of simply closing the connection. */
#if !defined LWIP_HTTPD_SUPPORT_EXTSTATUS || defined __DOXYGEN__
#define LWIP_HTTPD_SUPPORT_EXTSTATUS        0
#endif

/** Set this to 0 to drop support for HTTP/0.9 clients (to save some bytes) */
#if !defined LWIP_HTTPD_SUPPORT_V09 || defined __DOXYGEN__
#define LWIP_HTTPD_SUPPORT_V09              1
#endif

/** Set this to 1 to enable HTTP/1.1 persistent connections.
 * ATTENTION: If the generated file system includes HTTP headers, these must
 * include the "Connection: keep-alive" header (pass argument "-11" to makefsdata).
 */
#if !defined LWIP_HTTPD_SUPPORT_11_KEEPALIVE || defined __DOXYGEN__
#define LWIP_HTTPD_SUPPORT_11_KEEPALIVE     0
#endif

/** Set this to 1 to support HTTP request coming in in multiple packets/pbufs */
#if !defined LWIP_HTTPD_SUPPORT_REQUESTLIST || defined __DOXYGEN__
#define LWIP_HTTPD_SUPPORT_REQUESTLIST      1
#endif

#if LWIP_HTTPD_SUPPORT_REQUESTLIST
/** Number of rx pbufs to enqueue to parse an incoming request (up to the first
    newline) */
#if !defined LWIP_HTTPD_REQ_QUEUELEN || defined __DOXYGEN__
#define LWIP_HTTPD_REQ_QUEUELEN             5
#endif

/** Number of (TCP payload-) bytes (in pbufs) to enqueue to parse and incoming
    request (up to the first double-newline) */
#if !defined LWIP_HTTPD_REQ_BUFSIZE || defined __DOXYGEN__
#define LWIP_HTTPD_REQ_BUFSIZE              LWIP_HTTPD_MAX_REQ_LENGTH
#endif

/** Defines the maximum length of a HTTP request line (up to the first CRLF,
    copied from pbuf into this a global buffer when pbuf- or packet-queues
    are received - otherwise the input pbuf is used directly) */
#if !defined LWIP_HTTPD_MAX_REQ_LENGTH || defined __DOXYGEN__
#define LWIP_HTTPD_MAX_REQ_LENGTH           LWIP_MIN(1023, (LWIP_HTTPD_REQ_QUEUELEN * PBUF_POOL_BUFSIZE))
#endif
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */

/** This is the size of a static buffer used when URIs end with '/'.
 * In this buffer, the directory requested is concatenated with all the
 * configured default file names.
 * Set to 0 to disable checking default filenames on non-root directories.
 */
#if !defined LWIP_HTTPD_MAX_REQUEST_URI_LEN || defined __DOXYGEN__
#define LWIP_HTTPD_MAX_REQUEST_URI_LEN      63
#endif

/** Maximum length of the filename to send as response to a POST request,
 * filled in by the application when a POST is finished.
 */
#if !defined LWIP_HTTPD_POST_MAX_RESPONSE_URI_LEN || defined __DOXYGEN__
#define LWIP_HTTPD_POST_MAX_RESPONSE_URI_LEN 63
#endif

/** Set this to 0 to not send the SSI tag (default is on, so the tag will
 * be sent in the HTML page */
#if !defined LWIP_HTTPD_SSI_INCLUDE_TAG || defined __DOXYGEN__
#define LWIP_HTTPD_SSI_INCLUDE_TAG           1
#endif

/** Set this to 1 to call tcp_abort when tcp_close fails with memory error.
 * This can be used to prevent consuming all memory in situations where the
 * HTTP server has low priority compared to other communication. */
#if !defined LWIP_HTTPD_ABORT_ON_CLOSE_MEM_ERROR || defined __DOXYGEN__
#define LWIP_HTTPD_ABORT_ON_CLOSE_MEM_ERROR  0
#endif

/** Set this to 1 to kill the oldest connection when running out of
 * memory for 'struct http_state' or 'struct http_ssi_state'.
 * ATTENTION: This puts all connections on a linked list, so may be kind of slow.
 */
#if !defined LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED || defined __DOXYGEN__
#define LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED 0
#endif

/** Set this to 1 to send URIs without extension without headers
 * (who uses this at all??) */
#if !defined LWIP_HTTPD_OMIT_HEADER_FOR_EXTENSIONLESS_URI || defined __DOXYGEN__
#define LWIP_HTTPD_OMIT_HEADER_FOR_EXTENSIONLESS_URI 0
#endif

/** Default: Tags are sent from struct http_state and are therefore volatile */
#if !defined HTTP_IS_TAG_VOLATILE || defined __DOXYGEN__
#define HTTP_IS_TAG_VOLATILE(ptr) TCP_WRITE_FLAG_COPY
#endif

/* By default, the httpd is limited to send 2*pcb->mss to keep resource usage low
   when http is not an important protocol in the device. */
#if !defined HTTPD_LIMIT_SENDING_TO_2MSS || defined __DOXYGEN__
#define HTTPD_LIMIT_SENDING_TO_2MSS 1
#endif

/* Define this to a function that returns the maximum amount of data to enqueue.
   The function have this signature: u16_t fn(struct altcp_pcb* pcb);
   The best place to define this is the hooks file (@see LWIP_HOOK_FILENAME) */
#if !defined HTTPD_MAX_WRITE_LEN || defined __DOXYGEN__
#if HTTPD_LIMIT_SENDING_TO_2MSS
#define HTTPD_MAX_WRITE_LEN(pcb)    ((u16_t)(2 * altcp_mss(pcb)))
#endif
#endif

/*------------------- FS OPTIONS -------------------*/

/** Set this to 1 and provide the functions:
 * - "int fs_open_custom(struct fs_file *file, const char *name)"
 *    Called first for every opened file to allow opening files
 *    that are not included in fsdata(_custom).c
 * - "void fs_close_custom(struct fs_file *file)"
 *    Called to free resources allocated by fs_open_custom().
 */
#if !defined LWIP_HTTPD_CUSTOM_FILES || defined __DOXYGEN__
#define LWIP_HTTPD_CUSTOM_FILES       0
#endif

/** Set this to 1 to support fs_read() to dynamically read file data.
 * Without this (default=off), only one-block files are supported,
 * and the contents must be ready after fs_open().
 */
#if !defined LWIP_HTTPD_DYNAMIC_FILE_READ || defined __DOXYGEN__
#define LWIP_HTTPD_DYNAMIC_FILE_READ  0
#endif

/** Set this to 1 to include an application state argument per file
 * that is opened. This allows to keep a state per connection/file.
 */
#if !defined LWIP_HTTPD_FILE_STATE || defined __DOXYGEN__
#define LWIP_HTTPD_FILE_STATE         0
#endif

/** HTTPD_PRECALCULATED_CHECKSUM==1: include precompiled checksums for
 * predefined (MSS-sized) chunks of the files to prevent having to calculate
 * the checksums at runtime. */
#if !defined HTTPD_PRECALCULATED_CHECKSUM || defined __DOXYGEN__
#define HTTPD_PRECALCULATED_CHECKSUM  0
#endif

/** LWIP_HTTPD_FS_ASYNC_READ==1: support asynchronous read operations
 * (fs_read_async returns FS_READ_DELAYED and calls a callback when finished).
 */
#if !defined LWIP_HTTPD_FS_ASYNC_READ || defined __DOXYGEN__
#define LWIP_HTTPD_FS_ASYNC_READ      0
#endif

/** Filename (including path) to use as FS data file */
#if !defined HTTPD_FSDATA_FILE || defined __DOXYGEN__
/* HTTPD_USE_CUSTOM_FSDATA: Compatibility with deprecated lwIP option */
#if defined(HTTPD_USE_CUSTOM_FSDATA) && (HTTPD_USE_CUSTOM_FSDATA != 0)
#define HTTPD_FSDATA_FILE "fsdata_custom.c"
#else
#define HTTPD_FSDATA_FILE "fsdata.c"
#endif
#endif

/**
 * @}
 */

#endif /* LWIP_HDR_APPS_HTTPD_OPTS_H */
