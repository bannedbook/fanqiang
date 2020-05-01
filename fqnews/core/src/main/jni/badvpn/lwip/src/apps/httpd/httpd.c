/**
 * @file
 * LWIP HTTP server implementation
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
 *         Simon Goldschmidt
 *
 */

/**
 * @defgroup httpd HTTP server
 * @ingroup apps
 *
 * This httpd supports for a
 * rudimentary server-side-include facility which will replace tags of the form
 * <!--#tag--> in any file whose extension is .shtml, .shtm or .ssi with
 * strings provided by an include handler whose pointer is provided to the
 * module via function http_set_ssi_handler().
 * Additionally, a simple common
 * gateway interface (CGI) handling mechanism has been added to allow clients
 * to hook functions to particular request URIs.
 *
 * To enable SSI support, define label LWIP_HTTPD_SSI in lwipopts.h.
 * To enable CGI support, define label LWIP_HTTPD_CGI in lwipopts.h.
 *
 * By default, the server assumes that HTTP headers are already present in
 * each file stored in the file system.  By defining LWIP_HTTPD_DYNAMIC_HEADERS in
 * lwipopts.h, this behavior can be changed such that the server inserts the
 * headers automatically based on the extension of the file being served.  If
 * this mode is used, be careful to ensure that the file system image used
 * does not already contain the header information.
 *
 * File system images without headers can be created using the makefsfile
 * tool with the -h command line option.
 *
 *
 * Notes about valid SSI tags
 * --------------------------
 *
 * The following assumptions are made about tags used in SSI markers:
 *
 * 1. No tag may contain '-' or whitespace characters within the tag name.
 * 2. Whitespace is allowed between the tag leadin "<!--#" and the start of
 *    the tag name and between the tag name and the leadout string "-->".
 * 3. The maximum tag name length is LWIP_HTTPD_MAX_TAG_NAME_LEN, currently 8 characters.
 *
 * Notes on CGI usage
 * ------------------
 *
 * The simple CGI support offered here works with GET method requests only
 * and can handle up to 16 parameters encoded into the URI. The handler
 * function may not write directly to the HTTP output but must return a
 * filename that the HTTP server will send to the browser as a response to
 * the incoming CGI request.
 *
 *
 *
 * The list of supported file types is quite short, so if makefsdata complains
 * about an unknown extension, make sure to add it (and its doctype) to
 * the 'g_psHTTPHeaders' list.
 */
#include "lwip/init.h"
#include "lwip/apps/httpd.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/apps/fs.h"
#include "httpd_structs.h"
#include "lwip/def.h"

#include "lwip/altcp.h"
#include "lwip/altcp_tcp.h"
#if HTTPD_ENABLE_HTTPS
#include "lwip/altcp_tls.h"
#endif
#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif

#include <string.h> /* memset */
#include <stdlib.h> /* atoi */
#include <stdio.h>

#if LWIP_TCP && LWIP_CALLBACK_API

/** Minimum length for a valid HTTP/0.9 request: "GET /\r\n" -> 7 bytes */
#define MIN_REQ_LEN   7

#define CRLF "\r\n"
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
#define HTTP11_CONNECTIONKEEPALIVE  "Connection: keep-alive"
#define HTTP11_CONNECTIONKEEPALIVE2 "Connection: Keep-Alive"
#endif

#if LWIP_HTTPD_DYNAMIC_FILE_READ
#define HTTP_IS_DYNAMIC_FILE(hs) ((hs)->buf != NULL)
#else
#define HTTP_IS_DYNAMIC_FILE(hs) 0
#endif

/* This defines checks whether tcp_write has to copy data or not */

#ifndef HTTP_IS_DATA_VOLATILE
/** tcp_write does not have to copy data when sent from rom-file-system directly */
#define HTTP_IS_DATA_VOLATILE(hs)       (HTTP_IS_DYNAMIC_FILE(hs) ? TCP_WRITE_FLAG_COPY : 0)
#endif
/** Default: dynamic headers are sent from ROM (non-dynamic headers are handled like file data) */
#ifndef HTTP_IS_HDR_VOLATILE
#define HTTP_IS_HDR_VOLATILE(hs, ptr)   0
#endif

/* Return values for http_send_*() */
#define HTTP_DATA_TO_SEND_BREAK    2
#define HTTP_DATA_TO_SEND_CONTINUE 1
#define HTTP_NO_DATA_TO_SEND       0

typedef struct {
  const char *name;
  u8_t shtml;
} default_filename;

const default_filename g_psDefaultFilenames[] = {
  {"/index.shtml", 1 },
  {"/index.ssi",   1 },
  {"/index.shtm",  1 },
  {"/index.html",  0 },
  {"/index.htm",   0 }
};

#define NUM_DEFAULT_FILENAMES (sizeof(g_psDefaultFilenames) /   \
                               sizeof(default_filename))

#if LWIP_HTTPD_SUPPORT_REQUESTLIST
/** HTTP request is copied here from pbufs for simple parsing */
static char httpd_req_buf[LWIP_HTTPD_MAX_REQ_LENGTH + 1];
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */

#if LWIP_HTTPD_SUPPORT_POST
#if LWIP_HTTPD_POST_MAX_RESPONSE_URI_LEN > LWIP_HTTPD_MAX_REQUEST_URI_LEN
#define LWIP_HTTPD_URI_BUF_LEN LWIP_HTTPD_POST_MAX_RESPONSE_URI_LEN
#endif
#endif
#ifndef LWIP_HTTPD_URI_BUF_LEN
#define LWIP_HTTPD_URI_BUF_LEN LWIP_HTTPD_MAX_REQUEST_URI_LEN
#endif
#if LWIP_HTTPD_URI_BUF_LEN
/* Filename for response file to send when POST is finished or
 * search for default files when a directory is requested. */
static char http_uri_buf[LWIP_HTTPD_URI_BUF_LEN + 1];
#endif

#if LWIP_HTTPD_DYNAMIC_HEADERS
/* The number of individual strings that comprise the headers sent before each
 * requested file.
 */
#define NUM_FILE_HDR_STRINGS 5
#define HDR_STRINGS_IDX_HTTP_STATUS          0 /* e.g. "HTTP/1.0 200 OK\r\n" */
#define HDR_STRINGS_IDX_SERVER_NAME          1 /* e.g. "Server: "HTTPD_SERVER_AGENT"\r\n" */
#define HDR_STRINGS_IDX_CONTENT_LEN_KEPALIVE 2 /* e.g. "Content-Length: xy\r\n" and/or "Connection: keep-alive\r\n" */
#define HDR_STRINGS_IDX_CONTENT_LEN_NR       3 /* the byte count, when content-length is used */
#define HDR_STRINGS_IDX_CONTENT_TYPE         4 /* the content type (or default answer content type including default document) */

/* The dynamically generated Content-Length buffer needs space for CRLF + NULL */
#define LWIP_HTTPD_MAX_CONTENT_LEN_OFFSET 3
#ifndef LWIP_HTTPD_MAX_CONTENT_LEN_SIZE
/* The dynamically generated Content-Length buffer shall be able to work with
   ~953 MB (9 digits) */
#define LWIP_HTTPD_MAX_CONTENT_LEN_SIZE   (9 + LWIP_HTTPD_MAX_CONTENT_LEN_OFFSET)
#endif
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */

#if LWIP_HTTPD_SSI

#define HTTPD_LAST_TAG_PART 0xFFFF

enum tag_check_state {
  TAG_NONE,       /* Not processing an SSI tag */
  TAG_LEADIN,     /* Tag lead in "<!--#" being processed */
  TAG_FOUND,      /* Tag name being read, looking for lead-out start */
  TAG_LEADOUT,    /* Tag lead out "-->" being processed */
  TAG_SENDING     /* Sending tag replacement string */
};

struct http_ssi_state {
  const char *parsed;     /* Pointer to the first unparsed byte in buf. */
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
  const char *tag_started;/* Pointer to the first opening '<' of the tag. */
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG */
  const char *tag_end;    /* Pointer to char after the closing '>' of the tag. */
  u32_t parse_left; /* Number of unparsed bytes in buf. */
  u16_t tag_index;   /* Counter used by tag parsing state machine */
  u16_t tag_insert_len; /* Length of insert in string tag_insert */
#if LWIP_HTTPD_SSI_MULTIPART
  u16_t tag_part; /* Counter passed to and changed by tag insertion function to insert multiple times */
#endif /* LWIP_HTTPD_SSI_MULTIPART */
  u8_t tag_name_len; /* Length of the tag name in string tag_name */
  char tag_name[LWIP_HTTPD_MAX_TAG_NAME_LEN + 1]; /* Last tag name extracted */
  char tag_insert[LWIP_HTTPD_MAX_TAG_INSERT_LEN + 1]; /* Insert string for tag_name */
  enum tag_check_state tag_state; /* State of the tag processor */
};
#endif /* LWIP_HTTPD_SSI */

struct http_state {
#if LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED
  struct http_state *next;
#endif /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */
  struct fs_file file_handle;
  struct fs_file *handle;
  const char *file;       /* Pointer to first unsent byte in buf. */

  struct altcp_pcb *pcb;
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
  struct pbuf *req;
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */

#if LWIP_HTTPD_DYNAMIC_FILE_READ
  char *buf;        /* File read buffer. */
  int buf_len;      /* Size of file read buffer, buf. */
#endif /* LWIP_HTTPD_DYNAMIC_FILE_READ */
  u32_t left;       /* Number of unsent bytes in buf. */
  u8_t retries;
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
  u8_t keepalive;
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
#if LWIP_HTTPD_SSI
  struct http_ssi_state *ssi;
#endif /* LWIP_HTTPD_SSI */
#if LWIP_HTTPD_CGI
  char *params[LWIP_HTTPD_MAX_CGI_PARAMETERS]; /* Params extracted from the request URI */
  char *param_vals[LWIP_HTTPD_MAX_CGI_PARAMETERS]; /* Values for each extracted param */
#endif /* LWIP_HTTPD_CGI */
#if LWIP_HTTPD_DYNAMIC_HEADERS
  const char *hdrs[NUM_FILE_HDR_STRINGS]; /* HTTP headers to be sent. */
  char hdr_content_len[LWIP_HTTPD_MAX_CONTENT_LEN_SIZE];
  u16_t hdr_pos;     /* The position of the first unsent header byte in the
                        current string */
  u16_t hdr_index;   /* The index of the hdr string currently being sent. */
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */
#if LWIP_HTTPD_TIMING
  u32_t time_started;
#endif /* LWIP_HTTPD_TIMING */
#if LWIP_HTTPD_SUPPORT_POST
  u32_t post_content_len_left;
#if LWIP_HTTPD_POST_MANUAL_WND
  u32_t unrecved_bytes;
  u8_t no_auto_wnd;
  u8_t post_finished;
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
#endif /* LWIP_HTTPD_SUPPORT_POST*/
};

#if HTTPD_USE_MEM_POOL
LWIP_MEMPOOL_DECLARE(HTTPD_STATE,     MEMP_NUM_PARALLEL_HTTPD_CONNS,     sizeof(struct http_state),     "HTTPD_STATE")
#if LWIP_HTTPD_SSI
LWIP_MEMPOOL_DECLARE(HTTPD_SSI_STATE, MEMP_NUM_PARALLEL_HTTPD_SSI_CONNS, sizeof(struct http_ssi_state), "HTTPD_SSI_STATE")
#define HTTP_FREE_SSI_STATE(x)  LWIP_MEMPOOL_FREE(HTTPD_SSI_STATE, (x))
#define HTTP_ALLOC_SSI_STATE()  (struct http_ssi_state *)LWIP_MEMPOOL_ALLOC(HTTPD_SSI_STATE)
#endif /* LWIP_HTTPD_SSI */
#define HTTP_ALLOC_HTTP_STATE() (struct http_state *)LWIP_MEMPOOL_ALLOC(HTTPD_STATE)
#define HTTP_FREE_HTTP_STATE(x) LWIP_MEMPOOL_FREE(HTTPD_STATE, (x))
#else /* HTTPD_USE_MEM_POOL */
#define HTTP_ALLOC_HTTP_STATE() (struct http_state *)mem_malloc(sizeof(struct http_state))
#define HTTP_FREE_HTTP_STATE(x) mem_free(x)
#if LWIP_HTTPD_SSI
#define HTTP_ALLOC_SSI_STATE()  (struct http_ssi_state *)mem_malloc(sizeof(struct http_ssi_state))
#define HTTP_FREE_SSI_STATE(x)  mem_free(x)
#endif /* LWIP_HTTPD_SSI */
#endif /* HTTPD_USE_MEM_POOL */

static err_t http_close_conn(struct altcp_pcb *pcb, struct http_state *hs);
static err_t http_close_or_abort_conn(struct altcp_pcb *pcb, struct http_state *hs, u8_t abort_conn);
static err_t http_find_file(struct http_state *hs, const char *uri, int is_09);
static err_t http_init_file(struct http_state *hs, struct fs_file *file, int is_09, const char *uri, u8_t tag_check, char *params);
static err_t http_poll(void *arg, struct altcp_pcb *pcb);
static u8_t http_check_eof(struct altcp_pcb *pcb, struct http_state *hs);
#if LWIP_HTTPD_FS_ASYNC_READ
static void http_continue(void *connection);
#endif /* LWIP_HTTPD_FS_ASYNC_READ */

#if LWIP_HTTPD_SSI
/* SSI insert handler function pointer. */
tSSIHandler g_pfnSSIHandler;
#if !LWIP_HTTPD_SSI_RAW
int g_iNumTags;
const char **g_ppcTags;
#endif /* !LWIP_HTTPD_SSI_RAW */

#define LEN_TAG_LEAD_IN 5
const char *const g_pcTagLeadIn = "<!--#";

#define LEN_TAG_LEAD_OUT 3
const char *const g_pcTagLeadOut = "-->";
#endif /* LWIP_HTTPD_SSI */

#if LWIP_HTTPD_CGI
/* CGI handler information */
const tCGI *g_pCGIs;
int g_iNumCGIs;
int http_cgi_paramcount;
#define http_cgi_params     hs->params
#define http_cgi_param_vals hs->param_vals
#elif LWIP_HTTPD_CGI_SSI
char *http_cgi_params[LWIP_HTTPD_MAX_CGI_PARAMETERS]; /* Params extracted from the request URI */
char *http_cgi_param_vals[LWIP_HTTPD_MAX_CGI_PARAMETERS]; /* Values for each extracted param */
#endif /* LWIP_HTTPD_CGI */

#if LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED
/** global list of active HTTP connections, use to kill the oldest when
    running out of memory */
static struct http_state *http_connections;

static void
http_add_connection(struct http_state *hs)
{
  /* add the connection to the list */
  hs->next = http_connections;
  http_connections = hs;
}

static void
http_remove_connection(struct http_state *hs)
{
  /* take the connection off the list */
  if (http_connections) {
    if (http_connections == hs) {
      http_connections = hs->next;
    } else {
      struct http_state *last;
      for (last = http_connections; last->next != NULL; last = last->next) {
        if (last->next == hs) {
          last->next = hs->next;
          break;
        }
      }
    }
  }
}

static void
http_kill_oldest_connection(u8_t ssi_required)
{
  struct http_state *hs = http_connections;
  struct http_state *hs_free_next = NULL;
  while (hs && hs->next) {
#if LWIP_HTTPD_SSI
    if (ssi_required) {
      if (hs->next->ssi != NULL) {
        hs_free_next = hs;
      }
    } else
#else /* LWIP_HTTPD_SSI */
    LWIP_UNUSED_ARG(ssi_required);
#endif /* LWIP_HTTPD_SSI */
    {
      hs_free_next = hs;
    }
    LWIP_ASSERT("broken list", hs != hs->next);
    hs = hs->next;
  }
  if (hs_free_next != NULL) {
    LWIP_ASSERT("hs_free_next->next != NULL", hs_free_next->next != NULL);
    LWIP_ASSERT("hs_free_next->next->pcb != NULL", hs_free_next->next->pcb != NULL);
    /* send RST when killing a connection because of memory shortage */
    http_close_or_abort_conn(hs_free_next->next->pcb, hs_free_next->next, 1); /* this also unlinks the http_state from the list */
  }
}
#else /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */

#define http_add_connection(hs)
#define http_remove_connection(hs)

#endif /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */

#if LWIP_HTTPD_SSI
/** Allocate as struct http_ssi_state. */
static struct http_ssi_state *
http_ssi_state_alloc(void)
{
  struct http_ssi_state *ret = HTTP_ALLOC_SSI_STATE();
#if LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED
  if (ret == NULL) {
    http_kill_oldest_connection(1);
    ret = HTTP_ALLOC_SSI_STATE();
  }
#endif /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */
  if (ret != NULL) {
    memset(ret, 0, sizeof(struct http_ssi_state));
  }
  return ret;
}

/** Free a struct http_ssi_state. */
static void
http_ssi_state_free(struct http_ssi_state *ssi)
{
  if (ssi != NULL) {
    HTTP_FREE_SSI_STATE(ssi);
  }
}
#endif /* LWIP_HTTPD_SSI */

/** Initialize a struct http_state.
 */
static void
http_state_init(struct http_state *hs)
{
  /* Initialize the structure. */
  memset(hs, 0, sizeof(struct http_state));
#if LWIP_HTTPD_DYNAMIC_HEADERS
  /* Indicate that the headers are not yet valid */
  hs->hdr_index = NUM_FILE_HDR_STRINGS;
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */
}

/** Allocate a struct http_state. */
static struct http_state *
http_state_alloc(void)
{
  struct http_state *ret = HTTP_ALLOC_HTTP_STATE();
#if LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED
  if (ret == NULL) {
    http_kill_oldest_connection(0);
    ret = HTTP_ALLOC_HTTP_STATE();
  }
#endif /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */
  if (ret != NULL) {
    http_state_init(ret);
    http_add_connection(ret);
  }
  return ret;
}

/** Free a struct http_state.
 * Also frees the file data if dynamic.
 */
static void
http_state_eof(struct http_state *hs)
{
  if (hs->handle) {
#if LWIP_HTTPD_TIMING
    u32_t ms_needed = sys_now() - hs->time_started;
    u32_t needed = LWIP_MAX(1, (ms_needed / 100));
    LWIP_DEBUGF(HTTPD_DEBUG_TIMING, ("httpd: needed %"U32_F" ms to send file of %d bytes -> %"U32_F" bytes/sec\n",
                                     ms_needed, hs->handle->len, ((((u32_t)hs->handle->len) * 10) / needed)));
#endif /* LWIP_HTTPD_TIMING */
    fs_close(hs->handle);
    hs->handle = NULL;
  }
#if LWIP_HTTPD_DYNAMIC_FILE_READ
  if (hs->buf != NULL) {
    mem_free(hs->buf);
    hs->buf = NULL;
  }
#endif /* LWIP_HTTPD_DYNAMIC_FILE_READ */
#if LWIP_HTTPD_SSI
  if (hs->ssi) {
    http_ssi_state_free(hs->ssi);
    hs->ssi = NULL;
  }
#endif /* LWIP_HTTPD_SSI */
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
  if (hs->req) {
    pbuf_free(hs->req);
    hs->req = NULL;
  }
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
}

/** Free a struct http_state.
 * Also frees the file data if dynamic.
 */
static void
http_state_free(struct http_state *hs)
{
  if (hs != NULL) {
    http_state_eof(hs);
    http_remove_connection(hs);
    HTTP_FREE_HTTP_STATE(hs);
  }
}

/** Call tcp_write() in a loop trying smaller and smaller length
 *
 * @param pcb altcp_pcb to send
 * @param ptr Data to send
 * @param length Length of data to send (in/out: on return, contains the
 *        amount of data sent)
 * @param apiflags directly passed to tcp_write
 * @return the return value of tcp_write
 */
static err_t
http_write(struct altcp_pcb *pcb, const void *ptr, u16_t *length, u8_t apiflags)
{
  u16_t len, max_len;
  err_t err;
  LWIP_ASSERT("length != NULL", length != NULL);
  len = *length;
  if (len == 0) {
    return ERR_OK;
  }
  /* We cannot send more data than space available in the send buffer. */
  max_len = altcp_sndbuf(pcb);
  if (max_len < len) {
    len = max_len;
  }
#ifdef HTTPD_MAX_WRITE_LEN
  /* Additional limitation: e.g. don't enqueue more than 2*mss at once */
  max_len = HTTPD_MAX_WRITE_LEN(pcb);
  if (len > max_len) {
    len = max_len;
  }
#endif /* HTTPD_MAX_WRITE_LEN */
  do {
    LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Trying to send %d bytes\n", len));
    err = altcp_write(pcb, ptr, len, apiflags);
    if (err == ERR_MEM) {
      if ((altcp_sndbuf(pcb) == 0) ||
          (altcp_sndqueuelen(pcb) >= TCP_SND_QUEUELEN)) {
        /* no need to try smaller sizes */
        len = 1;
      } else {
        len /= 2;
      }
      LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE,
                  ("Send failed, trying less (%d bytes)\n", len));
    }
  } while ((err == ERR_MEM) && (len > 1));

  if (err == ERR_OK) {
    LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Sent %d bytes\n", len));
    *length = len;
  } else {
    LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Send failed with err %d (\"%s\")\n", err, lwip_strerr(err)));
    *length = 0;
  }

#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
  /* ensure nagle is normally enabled (only disabled for persistent connections
     when all data has been enqueued but the connection stays open for the next
     request */
  altcp_nagle_enable(pcb);
#endif

  return err;
}

/**
 * The connection shall be actively closed (using RST to close from fault states).
 * Reset the sent- and recv-callbacks.
 *
 * @param pcb the tcp pcb to reset callbacks
 * @param hs connection state to free
 */
static err_t
http_close_or_abort_conn(struct altcp_pcb *pcb, struct http_state *hs, u8_t abort_conn)
{
  err_t err;
  LWIP_DEBUGF(HTTPD_DEBUG, ("Closing connection %p\n", (void *)pcb));

#if LWIP_HTTPD_SUPPORT_POST
  if (hs != NULL) {
    if ((hs->post_content_len_left != 0)
#if LWIP_HTTPD_POST_MANUAL_WND
        || ((hs->no_auto_wnd != 0) && (hs->unrecved_bytes != 0))
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
       ) {
      /* make sure the post code knows that the connection is closed */
      http_uri_buf[0] = 0;
      httpd_post_finished(hs, http_uri_buf, LWIP_HTTPD_URI_BUF_LEN);
    }
  }
#endif /* LWIP_HTTPD_SUPPORT_POST*/


  altcp_arg(pcb, NULL);
  altcp_recv(pcb, NULL);
  altcp_err(pcb, NULL);
  altcp_poll(pcb, NULL, 0);
  altcp_sent(pcb, NULL);
  if (hs != NULL) {
    http_state_free(hs);
  }

  if (abort_conn) {
    altcp_abort(pcb);
    return ERR_OK;
  }
  err = altcp_close(pcb);
  if (err != ERR_OK) {
    LWIP_DEBUGF(HTTPD_DEBUG, ("Error %d closing %p\n", err, (void *)pcb));
    /* error closing, try again later in poll */
    altcp_poll(pcb, http_poll, HTTPD_POLL_INTERVAL);
  }
  return err;
}

/**
 * The connection shall be actively closed.
 * Reset the sent- and recv-callbacks.
 *
 * @param pcb the tcp pcb to reset callbacks
 * @param hs connection state to free
 */
static err_t
http_close_conn(struct altcp_pcb *pcb, struct http_state *hs)
{
  return http_close_or_abort_conn(pcb, hs, 0);
}

/** End of file: either close the connection (Connection: close) or
 * close the file (Connection: keep-alive)
 */
static void
http_eof(struct altcp_pcb *pcb, struct http_state *hs)
{
  /* HTTP/1.1 persistent connection? (Not supported for SSI) */
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
  if (hs->keepalive) {
    http_remove_connection(hs);

    http_state_eof(hs);
    http_state_init(hs);
    /* restore state: */
    hs->pcb = pcb;
    hs->keepalive = 1;
    http_add_connection(hs);
    /* ensure nagle doesn't interfere with sending all data as fast as possible: */
    altcp_nagle_disable(pcb);
  } else
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
  {
    http_close_conn(pcb, hs);
  }
}

#if LWIP_HTTPD_CGI || LWIP_HTTPD_CGI_SSI
/**
 * Extract URI parameters from the parameter-part of an URI in the form
 * "test.cgi?x=y" @todo: better explanation!
 * Pointers to the parameters are stored in hs->param_vals.
 *
 * @param hs http connection state
 * @param params pointer to the NULL-terminated parameter string from the URI
 * @return number of parameters extracted
 */
static int
extract_uri_parameters(struct http_state *hs, char *params)
{
  char *pair;
  char *equals;
  int loop;

  LWIP_UNUSED_ARG(hs);

  /* If we have no parameters at all, return immediately. */
  if (!params || (params[0] == '\0')) {
    return (0);
  }

  /* Get a pointer to our first parameter */
  pair = params;

  /* Parse up to LWIP_HTTPD_MAX_CGI_PARAMETERS from the passed string and ignore the
   * remainder (if any) */
  for (loop = 0; (loop < LWIP_HTTPD_MAX_CGI_PARAMETERS) && pair; loop++) {

    /* Save the name of the parameter */
    http_cgi_params[loop] = pair;

    /* Remember the start of this name=value pair */
    equals = pair;

    /* Find the start of the next name=value pair and replace the delimiter
     * with a 0 to terminate the previous pair string. */
    pair = strchr(pair, '&');
    if (pair) {
      *pair = '\0';
      pair++;
    } else {
      /* We didn't find a new parameter so find the end of the URI and
       * replace the space with a '\0' */
      pair = strchr(equals, ' ');
      if (pair) {
        *pair = '\0';
      }

      /* Revert to NULL so that we exit the loop as expected. */
      pair = NULL;
    }

    /* Now find the '=' in the previous pair, replace it with '\0' and save
     * the parameter value string. */
    equals = strchr(equals, '=');
    if (equals) {
      *equals = '\0';
      http_cgi_param_vals[loop] = equals + 1;
    } else {
      http_cgi_param_vals[loop] = NULL;
    }
  }

  return loop;
}
#endif /* LWIP_HTTPD_CGI || LWIP_HTTPD_CGI_SSI */

#if LWIP_HTTPD_SSI
/**
 * Insert a tag (found in an shtml in the form of "<!--#tagname-->" into the file.
 * The tag's name is stored in ssi->tag_name (NULL-terminated), the replacement
 * should be written to hs->tag_insert (up to a length of LWIP_HTTPD_MAX_TAG_INSERT_LEN).
 * The amount of data written is stored to ssi->tag_insert_len.
 *
 * @todo: return tag_insert_len - maybe it can be removed from struct http_state?
 *
 * @param hs http connection state
 */
static void
get_tag_insert(struct http_state *hs)
{
#if LWIP_HTTPD_SSI_RAW
  const char *tag;
#else /* LWIP_HTTPD_SSI_RAW */
  int tag;
#endif /* LWIP_HTTPD_SSI_RAW */
  size_t len;
  struct http_ssi_state *ssi;
#if LWIP_HTTPD_SSI_MULTIPART
  u16_t current_tag_part;
#endif /* LWIP_HTTPD_SSI_MULTIPART */

  LWIP_ASSERT("hs != NULL", hs != NULL);
  ssi = hs->ssi;
  LWIP_ASSERT("ssi != NULL", ssi != NULL);
#if LWIP_HTTPD_SSI_MULTIPART
  current_tag_part = ssi->tag_part;
  ssi->tag_part = HTTPD_LAST_TAG_PART;
#endif /* LWIP_HTTPD_SSI_MULTIPART */
#if LWIP_HTTPD_SSI_RAW
  tag = ssi->tag_name;
#endif

  if (g_pfnSSIHandler
#if !LWIP_HTTPD_SSI_RAW
      && g_ppcTags && g_iNumTags
#endif /* !LWIP_HTTPD_SSI_RAW */
     ) {

    /* Find this tag in the list we have been provided. */
#if LWIP_HTTPD_SSI_RAW
    {
#else /* LWIP_HTTPD_SSI_RAW */
    for (tag = 0; tag < g_iNumTags; tag++) {
      if (strcmp(ssi->tag_name, g_ppcTags[tag]) == 0)
#endif /* LWIP_HTTPD_SSI_RAW */
      {
        ssi->tag_insert_len = g_pfnSSIHandler(tag, ssi->tag_insert,
                                              LWIP_HTTPD_MAX_TAG_INSERT_LEN
#if LWIP_HTTPD_SSI_MULTIPART
                                              , current_tag_part, &ssi->tag_part
#endif /* LWIP_HTTPD_SSI_MULTIPART */
#if LWIP_HTTPD_FILE_STATE
                                              , (hs->handle ? hs->handle->state : NULL)
#endif /* LWIP_HTTPD_FILE_STATE */
                                             );
#if LWIP_HTTPD_SSI_RAW
        if (ssi->tag_insert_len != HTTPD_SSI_TAG_UNKNOWN)
#endif /* LWIP_HTTPD_SSI_RAW */
        {
          return;
        }
      }
    }
  }

  /* If we drop out, we were asked to serve a page which contains tags that
   * we don't have a handler for. Merely echo back the tags with an error
   * marker. */
#define UNKNOWN_TAG1_TEXT "<b>***UNKNOWN TAG "
#define UNKNOWN_TAG1_LEN  18
#define UNKNOWN_TAG2_TEXT "***</b>"
#define UNKNOWN_TAG2_LEN  7
  len = LWIP_MIN(sizeof(ssi->tag_name), LWIP_MIN(strlen(ssi->tag_name),
                 LWIP_HTTPD_MAX_TAG_INSERT_LEN - (UNKNOWN_TAG1_LEN + UNKNOWN_TAG2_LEN)));
  MEMCPY(ssi->tag_insert, UNKNOWN_TAG1_TEXT, UNKNOWN_TAG1_LEN);
  MEMCPY(&ssi->tag_insert[UNKNOWN_TAG1_LEN], ssi->tag_name, len);
  MEMCPY(&ssi->tag_insert[UNKNOWN_TAG1_LEN + len], UNKNOWN_TAG2_TEXT, UNKNOWN_TAG2_LEN);
  ssi->tag_insert[UNKNOWN_TAG1_LEN + len + UNKNOWN_TAG2_LEN] = 0;

  len = strlen(ssi->tag_insert);
  LWIP_ASSERT("len <= 0xffff", len <= 0xffff);
  ssi->tag_insert_len = (u16_t)len;
}
#endif /* LWIP_HTTPD_SSI */

#if LWIP_HTTPD_DYNAMIC_HEADERS
/**
 * Generate the relevant HTTP headers for the given filename and write
 * them into the supplied buffer.
 */
static void
get_http_headers(struct http_state *hs, const char *uri)
{
  size_t content_type;
  char *tmp;
  char *ext;
  char *vars;
  u8_t add_content_len;

  /* In all cases, the second header we send is the server identification
     so set it here. */
  hs->hdrs[HDR_STRINGS_IDX_SERVER_NAME] = g_psHTTPHeaderStrings[HTTP_HDR_SERVER];
  hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEPALIVE] = NULL;
  hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_NR] = NULL;

  /* Is this a normal file or the special case we use to send back the
     default "404: Page not found" response? */
  if (uri == NULL) {
    hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_NOT_FOUND];
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
    if (hs->keepalive) {
      hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = g_psHTTPHeaderStrings[DEFAULT_404_HTML_PERSISTENT];
    } else
#endif
    {
      hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = g_psHTTPHeaderStrings[DEFAULT_404_HTML];
    }

    /* Set up to send the first header string. */
    hs->hdr_index = 0;
    hs->hdr_pos = 0;
    return;
  }
  /* We are dealing with a particular filename. Look for one other
      special case.  We assume that any filename with "404" in it must be
      indicative of a 404 server error whereas all other files require
      the 200 OK header. */
  if (strstr(uri, "404")) {
    hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_NOT_FOUND];
  } else if (strstr(uri, "400")) {
    hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_BAD_REQUEST];
  } else if (strstr(uri, "501")) {
    hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_NOT_IMPL];
  } else {
    hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_OK];
  }

  /* Determine if the URI has any variables and, if so, temporarily remove
      them. */
  vars = strchr(uri, '?');
  if (vars) {
    *vars = '\0';
  }

  /* Get a pointer to the file extension.  We find this by looking for the
      last occurrence of "." in the filename passed. */
  ext = NULL;
  tmp = strchr(uri, '.');
  while (tmp) {
    ext = tmp + 1;
    tmp = strchr(ext, '.');
  }
  if (ext != NULL) {
    /* Now determine the content type and add the relevant header for that. */
    for (content_type = 0; content_type < NUM_HTTP_HEADERS; content_type++) {
      /* Have we found a matching extension? */
      if (!lwip_stricmp(g_psHTTPHeaders[content_type].extension, ext)) {
        break;
      }
    }
  } else {
    content_type = NUM_HTTP_HEADERS;
  }

  /* Reinstate the parameter marker if there was one in the original URI. */
  if (vars) {
    *vars = '?';
  }

#if LWIP_HTTPD_OMIT_HEADER_FOR_EXTENSIONLESS_URI
  /* Does the URL passed have any file extension?  If not, we assume it
     is a special-case URL used for control state notification and we do
     not send any HTTP headers with the response. */
  if (!ext) {
    /* Force the header index to a value indicating that all headers
       have already been sent. */
    hs->hdr_index = NUM_FILE_HDR_STRINGS;
    return;
  }
#endif /* LWIP_HTTPD_OMIT_HEADER_FOR_EXTENSIONLESS_URI */
  add_content_len = 1;
  /* Did we find a matching extension? */
  if (content_type < NUM_HTTP_HEADERS) {
    /* yes, store it */
    hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = g_psHTTPHeaders[content_type].content_type;
  } else if (!ext) {
    /* no, no extension found -> use binary transfer to prevent the browser adding '.txt' on save */
    hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = HTTP_HDR_APP;
  } else {
    /* No - use the default, plain text file type. */
    hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = HTTP_HDR_DEFAULT_TYPE;
  }
  /* Add content-length header? */
#if LWIP_HTTPD_SSI
  if (hs->ssi != NULL) {
    add_content_len = 0; /* @todo: get maximum file length from SSI */
  } else
#endif /* LWIP_HTTPD_SSI */
    if ((hs->handle == NULL) ||
        ((hs->handle->flags & (FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT)) == FS_FILE_FLAGS_HEADER_INCLUDED)) {
      add_content_len = 0;
    }
  if (add_content_len) {
    size_t len;
    lwip_itoa(hs->hdr_content_len, (size_t)LWIP_HTTPD_MAX_CONTENT_LEN_SIZE,
              hs->handle->len);
    len = strlen(hs->hdr_content_len);
    if (len <= LWIP_HTTPD_MAX_CONTENT_LEN_SIZE - LWIP_HTTPD_MAX_CONTENT_LEN_OFFSET) {
      SMEMCPY(&hs->hdr_content_len[len], CRLF, 3);
      hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_NR] = hs->hdr_content_len;
    } else {
      add_content_len = 0;
    }
  }
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
  if (add_content_len) {
    hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEPALIVE] = g_psHTTPHeaderStrings[HTTP_HDR_KEEPALIVE_LEN];
  } else {
    hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEPALIVE] = g_psHTTPHeaderStrings[HTTP_HDR_CONN_CLOSE];
  }
#else /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
  if (add_content_len) {
    hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEPALIVE] = g_psHTTPHeaderStrings[HTTP_HDR_CONTENT_LENGTH];
  }
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */

  /* Set up to send the first header string. */
  hs->hdr_index = 0;
  hs->hdr_pos = 0;
}

/** Sub-function of http_send(): send dynamic headers
 *
 * @returns: - HTTP_NO_DATA_TO_SEND: no new data has been enqueued
 *           - HTTP_DATA_TO_SEND_CONTINUE: continue with sending HTTP body
 *           - HTTP_DATA_TO_SEND_BREAK: data has been enqueued, headers pending,
 *                                      so don't send HTTP body yet
 */
static u8_t
http_send_headers(struct altcp_pcb *pcb, struct http_state *hs)
{
  err_t err;
  u16_t len;
  u8_t data_to_send = HTTP_NO_DATA_TO_SEND;
  u16_t hdrlen, sendlen;

  /* How much data can we send? */
  len = altcp_sndbuf(pcb);
  sendlen = len;

  while (len && (hs->hdr_index < NUM_FILE_HDR_STRINGS) && sendlen) {
    const void *ptr;
    u16_t old_sendlen;
    u8_t apiflags;
    /* How much do we have to send from the current header? */
    hdrlen = (u16_t)strlen(hs->hdrs[hs->hdr_index]);

    /* How much of this can we send? */
    sendlen = (len < (hdrlen - hs->hdr_pos)) ? len : (hdrlen - hs->hdr_pos);

    /* Send this amount of data or as much as we can given memory
     * constraints. */
    ptr = (const void *)(hs->hdrs[hs->hdr_index] + hs->hdr_pos);
    old_sendlen = sendlen;
    apiflags = HTTP_IS_HDR_VOLATILE(hs, ptr);
    if (hs->hdr_index == HDR_STRINGS_IDX_CONTENT_LEN_NR) {
      /* content-length is always volatile */
      apiflags |= TCP_WRITE_FLAG_COPY;
    }
    if (hs->hdr_index < NUM_FILE_HDR_STRINGS - 1) {
      apiflags |= TCP_WRITE_FLAG_MORE;
    }
    err = http_write(pcb, ptr, &sendlen, apiflags);
    if ((err == ERR_OK) && (old_sendlen != sendlen)) {
      /* Remember that we added some more data to be transmitted. */
      data_to_send = HTTP_DATA_TO_SEND_CONTINUE;
    } else if (err != ERR_OK) {
      /* special case: http_write does not try to send 1 byte */
      sendlen = 0;
    }

    /* Fix up the header position for the next time round. */
    hs->hdr_pos += sendlen;
    len -= sendlen;

    /* Have we finished sending this string? */
    if (hs->hdr_pos == hdrlen) {
      /* Yes - move on to the next one */
      hs->hdr_index++;
      /* skip headers that are NULL (not all headers are required) */
      while ((hs->hdr_index < NUM_FILE_HDR_STRINGS) &&
             (hs->hdrs[hs->hdr_index] == NULL)) {
        hs->hdr_index++;
      }
      hs->hdr_pos = 0;
    }
  }

  if ((hs->hdr_index >= NUM_FILE_HDR_STRINGS) && (hs->file == NULL)) {
    /* When we are at the end of the headers, check for data to send
     * instead of waiting for ACK from remote side to continue
     * (which would happen when sending files from async read). */
    if (http_check_eof(pcb, hs)) {
      data_to_send = HTTP_DATA_TO_SEND_CONTINUE;
    }
  }
  /* If we get here and there are still header bytes to send, we send
   * the header information we just wrote immediately. If there are no
   * more headers to send, but we do have file data to send, drop through
   * to try to send some file data too. */
  if ((hs->hdr_index < NUM_FILE_HDR_STRINGS) || !hs->file) {
    LWIP_DEBUGF(HTTPD_DEBUG, ("tcp_output\n"));
    return HTTP_DATA_TO_SEND_BREAK;
  }
  return data_to_send;
}
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */

/** Sub-function of http_send(): end-of-file (or block) is reached,
 * either close the file or read the next block (if supported).
 *
 * @returns: 0 if the file is finished or no data has been read
 *           1 if the file is not finished and data has been read
 */
static u8_t
http_check_eof(struct altcp_pcb *pcb, struct http_state *hs)
{
  int bytes_left;
#if LWIP_HTTPD_DYNAMIC_FILE_READ
  int count;
#ifdef HTTPD_MAX_WRITE_LEN
  int max_write_len;
#endif /* HTTPD_MAX_WRITE_LEN */
#endif /* LWIP_HTTPD_DYNAMIC_FILE_READ */

  /* Do we have a valid file handle? */
  if (hs->handle == NULL) {
    /* No - close the connection. */
    http_eof(pcb, hs);
    return 0;
  }
  bytes_left = fs_bytes_left(hs->handle);
  if (bytes_left <= 0) {
    /* We reached the end of the file so this request is done. */
    LWIP_DEBUGF(HTTPD_DEBUG, ("End of file.\n"));
    http_eof(pcb, hs);
    return 0;
  }
#if LWIP_HTTPD_DYNAMIC_FILE_READ
  /* Do we already have a send buffer allocated? */
  if (hs->buf) {
    /* Yes - get the length of the buffer */
    count = LWIP_MIN(hs->buf_len, bytes_left);
  } else {
    /* We don't have a send buffer so allocate one now */
    count = altcp_sndbuf(pcb);
    if (bytes_left < count) {
      count = bytes_left;
    }
#ifdef HTTPD_MAX_WRITE_LEN
    /* Additional limitation: e.g. don't enqueue more than 2*mss at once */
    max_write_len = HTTPD_MAX_WRITE_LEN(pcb);
    if (count > max_write_len) {
      count = max_write_len;
    }
#endif /* HTTPD_MAX_WRITE_LEN */
    do {
      hs->buf = (char *)mem_malloc((mem_size_t)count);
      if (hs->buf != NULL) {
        hs->buf_len = count;
        break;
      }
      count = count / 2;
    } while (count > 100);

    /* Did we get a send buffer? If not, return immediately. */
    if (hs->buf == NULL) {
      LWIP_DEBUGF(HTTPD_DEBUG, ("No buff\n"));
      return 0;
    }
  }

  /* Read a block of data from the file. */
  LWIP_DEBUGF(HTTPD_DEBUG, ("Trying to read %d bytes.\n", count));

#if LWIP_HTTPD_FS_ASYNC_READ
  count = fs_read_async(hs->handle, hs->buf, count, http_continue, hs);
#else /* LWIP_HTTPD_FS_ASYNC_READ */
  count = fs_read(hs->handle, hs->buf, count);
#endif /* LWIP_HTTPD_FS_ASYNC_READ */
  if (count < 0) {
    if (count == FS_READ_DELAYED) {
      /* Delayed read, wait for FS to unblock us */
      return 0;
    }
    /* We reached the end of the file so this request is done.
     * @todo: close here for HTTP/1.1 when reading file fails */
    LWIP_DEBUGF(HTTPD_DEBUG, ("End of file.\n"));
    http_eof(pcb, hs);
    return 0;
  }

  /* Set up to send the block of data we just read */
  LWIP_DEBUGF(HTTPD_DEBUG, ("Read %d bytes.\n", count));
  hs->left = count;
  hs->file = hs->buf;
#if LWIP_HTTPD_SSI
  if (hs->ssi) {
    hs->ssi->parse_left = count;
    hs->ssi->parsed = hs->buf;
  }
#endif /* LWIP_HTTPD_SSI */
#else /* LWIP_HTTPD_DYNAMIC_FILE_READ */
  LWIP_ASSERT("SSI and DYNAMIC_HEADERS turned off but eof not reached", 0);
#endif /* LWIP_HTTPD_SSI || LWIP_HTTPD_DYNAMIC_HEADERS */
  return 1;
}

/** Sub-function of http_send(): This is the normal send-routine for non-ssi files
 *
 * @returns: - 1: data has been written (so call tcp_ouput)
 *           - 0: no data has been written (no need to call tcp_output)
 */
static u8_t
http_send_data_nonssi(struct altcp_pcb *pcb, struct http_state *hs)
{
  err_t err;
  u16_t len;
  u8_t data_to_send = 0;

  /* We are not processing an SHTML file so no tag checking is necessary.
   * Just send the data as we received it from the file. */
  len = (u16_t)LWIP_MIN(hs->left, 0xffff);

  err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
  if (err == ERR_OK) {
    data_to_send = 1;
    hs->file += len;
    hs->left -= len;
  }

  return data_to_send;
}

#if LWIP_HTTPD_SSI
/** Sub-function of http_send(): This is the send-routine for ssi files
 *
 * @returns: - 1: data has been written (so call tcp_ouput)
 *           - 0: no data has been written (no need to call tcp_output)
 */
static u8_t
http_send_data_ssi(struct altcp_pcb *pcb, struct http_state *hs)
{
  err_t err = ERR_OK;
  u16_t len;
  u8_t data_to_send = 0;

  struct http_ssi_state *ssi = hs->ssi;
  LWIP_ASSERT("ssi != NULL", ssi != NULL);
  /* We are processing an SHTML file so need to scan for tags and replace
   * them with insert strings. We need to be careful here since a tag may
   * straddle the boundary of two blocks read from the file and we may also
   * have to split the insert string between two tcp_write operations. */

  /* How much data could we send? */
  len = altcp_sndbuf(pcb);

  /* Do we have remaining data to send before parsing more? */
  if (ssi->parsed > hs->file) {
    len = (u16_t)LWIP_MIN(ssi->parsed - hs->file, 0xffff);

    err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
    if (err == ERR_OK) {
      data_to_send = 1;
      hs->file += len;
      hs->left -= len;
    }

    /* If the send buffer is full, return now. */
    if (altcp_sndbuf(pcb) == 0) {
      return data_to_send;
    }
  }

  LWIP_DEBUGF(HTTPD_DEBUG, ("State %d, %d left\n", ssi->tag_state, (int)ssi->parse_left));

  /* We have sent all the data that was already parsed so continue parsing
   * the buffer contents looking for SSI tags. */
  while (((ssi->tag_state == TAG_SENDING) || ssi->parse_left) && (err == ERR_OK)) {
    if (len == 0) {
      return data_to_send;
    }
    switch (ssi->tag_state) {
      case TAG_NONE:
        /* We are not currently processing an SSI tag so scan for the
         * start of the lead-in marker. */
        if (*ssi->parsed == g_pcTagLeadIn[0]) {
          /* We found what could be the lead-in for a new tag so change
           * state appropriately. */
          ssi->tag_state = TAG_LEADIN;
          ssi->tag_index = 1;
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
          ssi->tag_started = ssi->parsed;
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG */
        }

        /* Move on to the next character in the buffer */
        ssi->parse_left--;
        ssi->parsed++;
        break;

      case TAG_LEADIN:
        /* We are processing the lead-in marker, looking for the start of
         * the tag name. */

        /* Have we reached the end of the leadin? */
        if (ssi->tag_index == LEN_TAG_LEAD_IN) {
          ssi->tag_index = 0;
          ssi->tag_state = TAG_FOUND;
        } else {
          /* Have we found the next character we expect for the tag leadin? */
          if (*ssi->parsed == g_pcTagLeadIn[ssi->tag_index]) {
            /* Yes - move to the next one unless we have found the complete
             * leadin, in which case we start looking for the tag itself */
            ssi->tag_index++;
          } else {
            /* We found an unexpected character so this is not a tag. Move
             * back to idle state. */
            ssi->tag_state = TAG_NONE;
          }

          /* Move on to the next character in the buffer */
          ssi->parse_left--;
          ssi->parsed++;
        }
        break;

      case TAG_FOUND:
        /* We are reading the tag name, looking for the start of the
         * lead-out marker and removing any whitespace found. */

        /* Remove leading whitespace between the tag leading and the first
         * tag name character. */
        if ((ssi->tag_index == 0) && ((*ssi->parsed == ' ') ||
                                      (*ssi->parsed == '\t') || (*ssi->parsed == '\n') ||
                                      (*ssi->parsed == '\r'))) {
          /* Move on to the next character in the buffer */
          ssi->parse_left--;
          ssi->parsed++;
          break;
        }

        /* Have we found the end of the tag name? This is signalled by
         * us finding the first leadout character or whitespace */
        if ((*ssi->parsed == g_pcTagLeadOut[0]) ||
            (*ssi->parsed == ' ')  || (*ssi->parsed == '\t') ||
            (*ssi->parsed == '\n') || (*ssi->parsed == '\r')) {

          if (ssi->tag_index == 0) {
            /* We read a zero length tag so ignore it. */
            ssi->tag_state = TAG_NONE;
          } else {
            /* We read a non-empty tag so go ahead and look for the
             * leadout string. */
            ssi->tag_state = TAG_LEADOUT;
            LWIP_ASSERT("ssi->tag_index <= 0xff", ssi->tag_index <= 0xff);
            ssi->tag_name_len = (u8_t)ssi->tag_index;
            ssi->tag_name[ssi->tag_index] = '\0';
            if (*ssi->parsed == g_pcTagLeadOut[0]) {
              ssi->tag_index = 1;
            } else {
              ssi->tag_index = 0;
            }
          }
        } else {
          /* This character is part of the tag name so save it */
          if (ssi->tag_index < LWIP_HTTPD_MAX_TAG_NAME_LEN) {
            ssi->tag_name[ssi->tag_index++] = *ssi->parsed;
          } else {
            /* The tag was too long so ignore it. */
            ssi->tag_state = TAG_NONE;
          }
        }

        /* Move on to the next character in the buffer */
        ssi->parse_left--;
        ssi->parsed++;

        break;

      /* We are looking for the end of the lead-out marker. */
      case TAG_LEADOUT:
        /* Remove leading whitespace between the tag leading and the first
         * tag leadout character. */
        if ((ssi->tag_index == 0) && ((*ssi->parsed == ' ') ||
                                      (*ssi->parsed == '\t') || (*ssi->parsed == '\n') ||
                                      (*ssi->parsed == '\r'))) {
          /* Move on to the next character in the buffer */
          ssi->parse_left--;
          ssi->parsed++;
          break;
        }

        /* Have we found the next character we expect for the tag leadout? */
        if (*ssi->parsed == g_pcTagLeadOut[ssi->tag_index]) {
          /* Yes - move to the next one unless we have found the complete
           * leadout, in which case we need to call the client to process
           * the tag. */

          /* Move on to the next character in the buffer */
          ssi->parse_left--;
          ssi->parsed++;

          if (ssi->tag_index == (LEN_TAG_LEAD_OUT - 1)) {
            /* Call the client to ask for the insert string for the
             * tag we just found. */
#if LWIP_HTTPD_SSI_MULTIPART
            ssi->tag_part = 0; /* start with tag part 0 */
#endif /* LWIP_HTTPD_SSI_MULTIPART */
            get_tag_insert(hs);

            /* Next time through, we are going to be sending data
             * immediately, either the end of the block we start
             * sending here or the insert string. */
            ssi->tag_index = 0;
            ssi->tag_state = TAG_SENDING;
            ssi->tag_end = ssi->parsed;
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
            ssi->parsed = ssi->tag_started;
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG*/

            /* If there is any unsent data in the buffer prior to the
             * tag, we need to send it now. */
            if (ssi->tag_end > hs->file) {
              /* How much of the data can we send? */
#if LWIP_HTTPD_SSI_INCLUDE_TAG
              len = (u16_t)LWIP_MIN(ssi->tag_end - hs->file, 0xffff);
#else /* LWIP_HTTPD_SSI_INCLUDE_TAG*/
              /* we would include the tag in sending */
              len = (u16_t)LWIP_MIN(ssi->tag_started - hs->file, 0xffff);
#endif /* LWIP_HTTPD_SSI_INCLUDE_TAG*/

              err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
              if (err == ERR_OK) {
                data_to_send = 1;
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
                if (ssi->tag_started <= hs->file) {
                  /* pretend to have sent the tag, too */
                  len += (u16_t)(ssi->tag_end - ssi->tag_started);
                }
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG*/
                hs->file += len;
                hs->left -= len;
              }
            }
          } else {
            ssi->tag_index++;
          }
        } else {
          /* We found an unexpected character so this is not a tag. Move
           * back to idle state. */
          ssi->parse_left--;
          ssi->parsed++;
          ssi->tag_state = TAG_NONE;
        }
        break;

      /*
       * We have found a valid tag and are in the process of sending
       * data as a result of that discovery. We send either remaining data
       * from the file prior to the insert point or the insert string itself.
       */
      case TAG_SENDING:
        /* Do we have any remaining file data to send from the buffer prior
         * to the tag? */
        if (ssi->tag_end > hs->file) {
          /* How much of the data can we send? */
#if LWIP_HTTPD_SSI_INCLUDE_TAG
          len = (u16_t)LWIP_MIN(ssi->tag_end - hs->file, 0xffff);
#else /* LWIP_HTTPD_SSI_INCLUDE_TAG*/
          LWIP_ASSERT("hs->started >= hs->file", ssi->tag_started >= hs->file);
          /* we would include the tag in sending */
          len = (u16_t)LWIP_MIN(ssi->tag_started - hs->file, 0xffff);
#endif /* LWIP_HTTPD_SSI_INCLUDE_TAG*/
          if (len != 0) {
            err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
          } else {
            err = ERR_OK;
          }
          if (err == ERR_OK) {
            data_to_send = 1;
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
            if (ssi->tag_started <= hs->file) {
              /* pretend to have sent the tag, too */
              len += (u16_t)(ssi->tag_end - ssi->tag_started);
            }
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG*/
            hs->file += len;
            hs->left -= len;
          }
        } else {
#if LWIP_HTTPD_SSI_MULTIPART
          if (ssi->tag_index >= ssi->tag_insert_len) {
            /* Did the last SSIHandler have more to send? */
            if (ssi->tag_part != HTTPD_LAST_TAG_PART) {
              /* If so, call it again */
              ssi->tag_index = 0;
              get_tag_insert(hs);
            }
          }
#endif /* LWIP_HTTPD_SSI_MULTIPART */

          /* Do we still have insert data left to send? */
          if (ssi->tag_index < ssi->tag_insert_len) {
            /* We are sending the insert string itself. How much of the
             * insert can we send? */
            len = (ssi->tag_insert_len - ssi->tag_index);

            /* Note that we set the copy flag here since we only have a
             * single tag insert buffer per connection. If we don't do
             * this, insert corruption can occur if more than one insert
             * is processed before we call tcp_output. */
            err = http_write(pcb, &(ssi->tag_insert[ssi->tag_index]), &len,
                             HTTP_IS_TAG_VOLATILE(hs));
            if (err == ERR_OK) {
              data_to_send = 1;
              ssi->tag_index += len;
              /* Don't return here: keep on sending data */
            }
          } else {
#if LWIP_HTTPD_SSI_MULTIPART
            if (ssi->tag_part == HTTPD_LAST_TAG_PART)
#endif /* LWIP_HTTPD_SSI_MULTIPART */
            {
              /* We have sent all the insert data so go back to looking for
               * a new tag. */
              LWIP_DEBUGF(HTTPD_DEBUG, ("Everything sent.\n"));
              ssi->tag_index = 0;
              ssi->tag_state = TAG_NONE;
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
              ssi->parsed = ssi->tag_end;
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG*/
            }
          }
          break;
        default:
          break;
        }
    }
  }

  /* If we drop out of the end of the for loop, this implies we must have
   * file data to send so send it now. In TAG_SENDING state, we've already
   * handled this so skip the send if that's the case. */
  if ((ssi->tag_state != TAG_SENDING) && (ssi->parsed > hs->file)) {
#if LWIP_HTTPD_DYNAMIC_FILE_READ && !LWIP_HTTPD_SSI_INCLUDE_TAG
    if ((ssi->tag_state != TAG_NONE) && (ssi->tag_started > ssi->tag_end)) {
      /* If we found tag on the edge of the read buffer: just throw away the first part
         (we have copied/saved everything required for parsing on later). */
      len = (u16_t)(ssi->tag_started - hs->file);
      hs->left -= (ssi->parsed - ssi->tag_started);
      ssi->parsed = ssi->tag_started;
      ssi->tag_started = hs->buf;
    } else
#endif /* LWIP_HTTPD_DYNAMIC_FILE_READ && !LWIP_HTTPD_SSI_INCLUDE_TAG */
    {
      len = (u16_t)LWIP_MIN(ssi->parsed - hs->file, 0xffff);
    }

    err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
    if (err == ERR_OK) {
      data_to_send = 1;
      hs->file += len;
      hs->left -= len;
    }
  }
  return data_to_send;
}
#endif /* LWIP_HTTPD_SSI */

/**
 * Try to send more data on this pcb.
 *
 * @param pcb the pcb to send data
 * @param hs connection state
 */
static u8_t
http_send(struct altcp_pcb *pcb, struct http_state *hs)
{
  u8_t data_to_send = HTTP_NO_DATA_TO_SEND;

  LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_send: pcb=%p hs=%p left=%d\n", (void *)pcb,
              (void *)hs, hs != NULL ? (int)hs->left : 0));

#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
  if (hs->unrecved_bytes != 0) {
    return 0;
  }
#endif /* LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND */

  /* If we were passed a NULL state structure pointer, ignore the call. */
  if (hs == NULL) {
    return 0;
  }

#if LWIP_HTTPD_FS_ASYNC_READ
  /* Check if we are allowed to read from this file.
     (e.g. SSI might want to delay sending until data is available) */
  if (!fs_is_file_ready(hs->handle, http_continue, hs)) {
    return 0;
  }
#endif /* LWIP_HTTPD_FS_ASYNC_READ */

#if LWIP_HTTPD_DYNAMIC_HEADERS
  /* Do we have any more header data to send for this file? */
  if (hs->hdr_index < NUM_FILE_HDR_STRINGS) {
    data_to_send = http_send_headers(pcb, hs);
    if ((data_to_send != HTTP_DATA_TO_SEND_CONTINUE) &&
        (hs->hdr_index < NUM_FILE_HDR_STRINGS)) {
      return data_to_send;
    }
  }
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */

  /* Have we run out of file data to send? If so, we need to read the next
   * block from the file. */
  if (hs->left == 0) {
    if (!http_check_eof(pcb, hs)) {
      return 0;
    }
  }

#if LWIP_HTTPD_SSI
  if (hs->ssi) {
    data_to_send = http_send_data_ssi(pcb, hs);
  } else
#endif /* LWIP_HTTPD_SSI */
  {
    data_to_send = http_send_data_nonssi(pcb, hs);
  }

  if ((hs->left == 0) && (fs_bytes_left(hs->handle) <= 0)) {
    /* We reached the end of the file so this request is done.
     * This adds the FIN flag right into the last data segment. */
    LWIP_DEBUGF(HTTPD_DEBUG, ("End of file.\n"));
    http_eof(pcb, hs);
    return 0;
  }
  LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("send_data end.\n"));
  return data_to_send;
}

#if LWIP_HTTPD_SUPPORT_EXTSTATUS
/** Initialize a http connection with a file to send for an error message
 *
 * @param hs http connection state
 * @param error_nr HTTP error number
 * @return ERR_OK if file was found and hs has been initialized correctly
 *         another err_t otherwise
 */
static err_t
http_find_error_file(struct http_state *hs, u16_t error_nr)
{
  const char *uri1, *uri2, *uri3;
  err_t err;

  if (error_nr == 501) {
    uri1 = "/501.html";
    uri2 = "/501.htm";
    uri3 = "/501.shtml";
  } else {
    /* 400 (bad request is the default) */
    uri1 = "/400.html";
    uri2 = "/400.htm";
    uri3 = "/400.shtml";
  }
  err = fs_open(&hs->file_handle, uri1);
  if (err != ERR_OK) {
    err = fs_open(&hs->file_handle, uri2);
    if (err != ERR_OK) {
      err = fs_open(&hs->file_handle, uri3);
      if (err != ERR_OK) {
        LWIP_DEBUGF(HTTPD_DEBUG, ("Error page for error %"U16_F" not found\n",
                                  error_nr));
        return ERR_ARG;
      }
    }
  }
  return http_init_file(hs, &hs->file_handle, 0, NULL, 0, NULL);
}
#else /* LWIP_HTTPD_SUPPORT_EXTSTATUS */
#define http_find_error_file(hs, error_nr) ERR_ARG
#endif /* LWIP_HTTPD_SUPPORT_EXTSTATUS */

/**
 * Get the file struct for a 404 error page.
 * Tries some file names and returns NULL if none found.
 *
 * @param uri pointer that receives the actual file name URI
 * @return file struct for the error page or NULL no matching file was found
 */
static struct fs_file *
http_get_404_file(struct http_state *hs, const char **uri)
{
  err_t err;

  *uri = "/404.html";
  err = fs_open(&hs->file_handle, *uri);
  if (err != ERR_OK) {
    /* 404.html doesn't exist. Try 404.htm instead. */
    *uri = "/404.htm";
    err = fs_open(&hs->file_handle, *uri);
    if (err != ERR_OK) {
      /* 404.htm doesn't exist either. Try 404.shtml instead. */
      *uri = "/404.shtml";
      err = fs_open(&hs->file_handle, *uri);
      if (err != ERR_OK) {
        /* 404.htm doesn't exist either. Indicate to the caller that it should
         * send back a default 404 page.
         */
        *uri = NULL;
        return NULL;
      }
    }
  }

  return &hs->file_handle;
}

#if LWIP_HTTPD_SUPPORT_POST
static err_t
http_handle_post_finished(struct http_state *hs)
{
#if LWIP_HTTPD_POST_MANUAL_WND
  /* Prevent multiple calls to httpd_post_finished, since it might have already
     been called before from httpd_post_data_recved(). */
  if (hs->post_finished) {
    return ERR_OK;
  }
  hs->post_finished = 1;
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
  /* application error or POST finished */
  /* NULL-terminate the buffer */
  http_uri_buf[0] = 0;
  httpd_post_finished(hs, http_uri_buf, LWIP_HTTPD_URI_BUF_LEN);
  return http_find_file(hs, http_uri_buf, 0);
}

/** Pass received POST body data to the application and correctly handle
 * returning a response document or closing the connection.
 * ATTENTION: The application is responsible for the pbuf now, so don't free it!
 *
 * @param hs http connection state
 * @param p pbuf to pass to the application
 * @return ERR_OK if passed successfully, another err_t if the response file
 *         hasn't been found (after POST finished)
 */
static err_t
http_post_rxpbuf(struct http_state *hs, struct pbuf *p)
{
  err_t err;

  if (p != NULL) {
    /* adjust remaining Content-Length */
    if (hs->post_content_len_left < p->tot_len) {
      hs->post_content_len_left = 0;
    } else {
      hs->post_content_len_left -= p->tot_len;
    }
  }
#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
  /* prevent connection being closed if httpd_post_data_recved() is called nested */
  hs->unrecved_bytes++;
#endif
  err = httpd_post_receive_data(hs, p);
#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
  hs->unrecved_bytes--;
#endif
  if (err != ERR_OK) {
    /* Ignore remaining content in case of application error */
    hs->post_content_len_left = 0;
  }
  if (hs->post_content_len_left == 0) {
#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
    if (hs->unrecved_bytes != 0) {
      return ERR_OK;
    }
#endif /* LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND */
    /* application error or POST finished */
    return http_handle_post_finished(hs);
  }

  return ERR_OK;
}

/** Handle a post request. Called from http_parse_request when method 'POST'
 * is found.
 *
 * @param p The input pbuf (containing the POST header and body).
 * @param hs The http connection state.
 * @param data HTTP request (header and part of body) from input pbuf(s).
 * @param data_len Size of 'data'.
 * @param uri The HTTP URI parsed from input pbuf(s).
 * @param uri_end Pointer to the end of 'uri' (here, the rest of the HTTP
 *                header starts).
 * @return ERR_OK: POST correctly parsed and accepted by the application.
 *         ERR_INPROGRESS: POST not completely parsed (no error yet)
 *         another err_t: Error parsing POST or denied by the application
 */
static err_t
http_post_request(struct pbuf *inp, struct http_state *hs,
                  char *data, u16_t data_len, char *uri, char *uri_end)
{
  err_t err;
  /* search for end-of-header (first double-CRLF) */
  char *crlfcrlf = lwip_strnstr(uri_end + 1, CRLF CRLF, data_len - (uri_end + 1 - data));

  if (crlfcrlf != NULL) {
    /* search for "Content-Length: " */
#define HTTP_HDR_CONTENT_LEN                "Content-Length: "
#define HTTP_HDR_CONTENT_LEN_LEN            16
#define HTTP_HDR_CONTENT_LEN_DIGIT_MAX_LEN  10
    char *scontent_len = lwip_strnstr(uri_end + 1, HTTP_HDR_CONTENT_LEN, crlfcrlf - (uri_end + 1));
    if (scontent_len != NULL) {
      char *scontent_len_end = lwip_strnstr(scontent_len + HTTP_HDR_CONTENT_LEN_LEN, CRLF, HTTP_HDR_CONTENT_LEN_DIGIT_MAX_LEN);
      if (scontent_len_end != NULL) {
        int content_len;
        char *content_len_num = scontent_len + HTTP_HDR_CONTENT_LEN_LEN;
        content_len = atoi(content_len_num);
        if (content_len == 0) {
          /* if atoi returns 0 on error, fix this */
          if ((content_len_num[0] != '0') || (content_len_num[1] != '\r')) {
            content_len = -1;
          }
        }
        if (content_len >= 0) {
          /* adjust length of HTTP header passed to application */
          const char *hdr_start_after_uri = uri_end + 1;
          u16_t hdr_len = (u16_t)LWIP_MIN(data_len, crlfcrlf + 4 - data);
          u16_t hdr_data_len = (u16_t)LWIP_MIN(data_len, crlfcrlf + 4 - hdr_start_after_uri);
          u8_t post_auto_wnd = 1;
          http_uri_buf[0] = 0;
          /* trim http header */
          *crlfcrlf = 0;
          err = httpd_post_begin(hs, uri, hdr_start_after_uri, hdr_data_len, content_len,
                                 http_uri_buf, LWIP_HTTPD_URI_BUF_LEN, &post_auto_wnd);
          if (err == ERR_OK) {
            /* try to pass in data of the first pbuf(s) */
            struct pbuf *q = inp;
            u16_t start_offset = hdr_len;
#if LWIP_HTTPD_POST_MANUAL_WND
            hs->no_auto_wnd = !post_auto_wnd;
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
            /* set the Content-Length to be received for this POST */
            hs->post_content_len_left = (u32_t)content_len;

            /* get to the pbuf where the body starts */
            while ((q != NULL) && (q->len <= start_offset)) {
              start_offset -= q->len;
              q = q->next;
            }
            if (q != NULL) {
              /* hide the remaining HTTP header */
              pbuf_remove_header(q, start_offset);
#if LWIP_HTTPD_POST_MANUAL_WND
              if (!post_auto_wnd) {
                /* already tcp_recved() this data... */
                hs->unrecved_bytes = q->tot_len;
              }
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
              pbuf_ref(q);
              return http_post_rxpbuf(hs, q);
            } else if (hs->post_content_len_left == 0) {
              q = pbuf_alloc(PBUF_RAW, 0, PBUF_REF);
              return http_post_rxpbuf(hs, q);
            } else {
              return ERR_OK;
            }
          } else {
            /* return file passed from application */
            return http_find_file(hs, http_uri_buf, 0);
          }
        } else {
          LWIP_DEBUGF(HTTPD_DEBUG, ("POST received invalid Content-Length: %s\n",
                                    content_len_num));
          return ERR_ARG;
        }
      }
    }
    /* If we come here, headers are fully received (double-crlf), but Content-Length
       was not included. Since this is currently the only supported method, we have
       to fail in this case! */
    LWIP_DEBUGF(HTTPD_DEBUG, ("Error when parsing Content-Length\n"));
    return ERR_ARG;
  }
  /* if we come here, the POST is incomplete */
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
  return ERR_INPROGRESS;
#else /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
  return ERR_ARG;
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
}

#if LWIP_HTTPD_POST_MANUAL_WND
/** A POST implementation can call this function to update the TCP window.
 * This can be used to throttle data reception (e.g. when received data is
 * programmed to flash and data is received faster than programmed).
 *
 * @param connection A connection handle passed to httpd_post_begin for which
 *        httpd_post_finished has *NOT* been called yet!
 * @param recved_len Length of data received (for window update)
 */
void httpd_post_data_recved(void *connection, u16_t recved_len)
{
  struct http_state *hs = (struct http_state *)connection;
  if (hs != NULL) {
    if (hs->no_auto_wnd) {
      u16_t len = recved_len;
      if (hs->unrecved_bytes >= recved_len) {
        hs->unrecved_bytes -= recved_len;
      } else {
        LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("httpd_post_data_recved: recved_len too big\n"));
        len = (u16_t)hs->unrecved_bytes;
        hs->unrecved_bytes = 0;
      }
      if (hs->pcb != NULL) {
        if (len != 0) {
          altcp_recved(hs->pcb, len);
        }
        if ((hs->post_content_len_left == 0) && (hs->unrecved_bytes == 0)) {
          /* finished handling POST */
          http_handle_post_finished(hs);
          http_send(hs->pcb, hs);
        }
      }
    }
  }
}
#endif /* LWIP_HTTPD_POST_MANUAL_WND */

#endif /* LWIP_HTTPD_SUPPORT_POST */

#if LWIP_HTTPD_FS_ASYNC_READ
/** Try to send more data if file has been blocked before
 * This is a callback function passed to fs_read_async().
 */
static void
http_continue(void *connection)
{
  struct http_state *hs = (struct http_state *)connection;
  if (hs && (hs->pcb) && (hs->handle)) {
    LWIP_ASSERT("hs->pcb != NULL", hs->pcb != NULL);
    LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("httpd_continue: try to send more data\n"));
    if (http_send(hs->pcb, hs)) {
      /* If we wrote anything to be sent, go ahead and send it now. */
      LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("tcp_output\n"));
      altcp_output(hs->pcb);
    }
  }
}
#endif /* LWIP_HTTPD_FS_ASYNC_READ */

/**
 * When data has been received in the correct state, try to parse it
 * as a HTTP request.
 *
 * @param inp the received pbuf
 * @param hs the connection state
 * @param pcb the altcp_pcb which received this packet
 * @return ERR_OK if request was OK and hs has been initialized correctly
 *         ERR_INPROGRESS if request was OK so far but not fully received
 *         another err_t otherwise
 */
static err_t
http_parse_request(struct pbuf *inp, struct http_state *hs, struct altcp_pcb *pcb)
{
  char *data;
  char *crlf;
  u16_t data_len;
  struct pbuf *p = inp;
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
  u16_t clen;
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
#if LWIP_HTTPD_SUPPORT_POST
  err_t err;
#endif /* LWIP_HTTPD_SUPPORT_POST */

  LWIP_UNUSED_ARG(pcb); /* only used for post */
  LWIP_ASSERT("p != NULL", p != NULL);
  LWIP_ASSERT("hs != NULL", hs != NULL);

  if ((hs->handle != NULL) || (hs->file != NULL)) {
    LWIP_DEBUGF(HTTPD_DEBUG, ("Received data while sending a file\n"));
    /* already sending a file */
    /* @todo: abort? */
    return ERR_USE;
  }

#if LWIP_HTTPD_SUPPORT_REQUESTLIST

  LWIP_DEBUGF(HTTPD_DEBUG, ("Received %"U16_F" bytes\n", p->tot_len));

  /* first check allowed characters in this pbuf? */

  /* enqueue the pbuf */
  if (hs->req == NULL) {
    LWIP_DEBUGF(HTTPD_DEBUG, ("First pbuf\n"));
    hs->req = p;
  } else {
    LWIP_DEBUGF(HTTPD_DEBUG, ("pbuf enqueued\n"));
    pbuf_cat(hs->req, p);
  }
  /* increase pbuf ref counter as it is freed when we return but we want to
     keep it on the req list */
  pbuf_ref(p);

  if (hs->req->next != NULL) {
    data_len = LWIP_MIN(hs->req->tot_len, LWIP_HTTPD_MAX_REQ_LENGTH);
    pbuf_copy_partial(hs->req, httpd_req_buf, data_len, 0);
    data = httpd_req_buf;
  } else
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
  {
    data = (char *)p->payload;
    data_len = p->len;
    if (p->len != p->tot_len) {
      LWIP_DEBUGF(HTTPD_DEBUG, ("Warning: incomplete header due to chained pbufs\n"));
    }
  }

  /* received enough data for minimal request? */
  if (data_len >= MIN_REQ_LEN) {
    /* wait for CRLF before parsing anything */
    crlf = lwip_strnstr(data, CRLF, data_len);
    if (crlf != NULL) {
#if LWIP_HTTPD_SUPPORT_POST
      int is_post = 0;
#endif /* LWIP_HTTPD_SUPPORT_POST */
      int is_09 = 0;
      char *sp1, *sp2;
      u16_t left_len, uri_len;
      LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("CRLF received, parsing request\n"));
      /* parse method */
      if (!strncmp(data, "GET ", 4)) {
        sp1 = data + 3;
        /* received GET request */
        LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Received GET request\"\n"));
#if LWIP_HTTPD_SUPPORT_POST
      } else if (!strncmp(data, "POST ", 5)) {
        /* store request type */
        is_post = 1;
        sp1 = data + 4;
        /* received GET request */
        LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Received POST request\n"));
#endif /* LWIP_HTTPD_SUPPORT_POST */
      } else {
        /* null-terminate the METHOD (pbuf is freed anyway wen returning) */
        data[4] = 0;
        /* unsupported method! */
        LWIP_DEBUGF(HTTPD_DEBUG, ("Unsupported request method (not implemented): \"%s\"\n",
                                  data));
        return http_find_error_file(hs, 501);
      }
      /* if we come here, method is OK, parse URI */
      left_len = (u16_t)(data_len - ((sp1 + 1) - data));
      sp2 = lwip_strnstr(sp1 + 1, " ", left_len);
#if LWIP_HTTPD_SUPPORT_V09
      if (sp2 == NULL) {
        /* HTTP 0.9: respond with correct protocol version */
        sp2 = lwip_strnstr(sp1 + 1, CRLF, left_len);
        is_09 = 1;
#if LWIP_HTTPD_SUPPORT_POST
        if (is_post) {
          /* HTTP/0.9 does not support POST */
          goto badrequest;
        }
#endif /* LWIP_HTTPD_SUPPORT_POST */
      }
#endif /* LWIP_HTTPD_SUPPORT_V09 */
      uri_len = (u16_t)(sp2 - (sp1 + 1));
      if ((sp2 != 0) && (sp2 > sp1)) {
        /* wait for CRLFCRLF (indicating end of HTTP headers) before parsing anything */
        if (lwip_strnstr(data, CRLF CRLF, data_len) != NULL) {
          char *uri = sp1 + 1;
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
          /* This is HTTP/1.0 compatible: for strict 1.1, a connection
             would always be persistent unless "close" was specified. */
          if (!is_09 && (lwip_strnstr(data, HTTP11_CONNECTIONKEEPALIVE, data_len) ||
                         lwip_strnstr(data, HTTP11_CONNECTIONKEEPALIVE2, data_len))) {
            hs->keepalive = 1;
          } else {
            hs->keepalive = 0;
          }
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
          /* null-terminate the METHOD (pbuf is freed anyway wen returning) */
          *sp1 = 0;
          uri[uri_len] = 0;
          LWIP_DEBUGF(HTTPD_DEBUG, ("Received \"%s\" request for URI: \"%s\"\n",
                                    data, uri));
#if LWIP_HTTPD_SUPPORT_POST
          if (is_post) {
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
            struct pbuf *q = hs->req;
#else /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
            struct pbuf *q = inp;
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
            err = http_post_request(q, hs, data, data_len, uri, sp2);
            if (err != ERR_OK) {
              /* restore header for next try */
              *sp1 = ' ';
              *sp2 = ' ';
              uri[uri_len] = ' ';
            }
            if (err == ERR_ARG) {
              goto badrequest;
            }
            return err;
          } else
#endif /* LWIP_HTTPD_SUPPORT_POST */
          {
            return http_find_file(hs, uri, is_09);
          }
        }
      } else {
        LWIP_DEBUGF(HTTPD_DEBUG, ("invalid URI\n"));
      }
    }
  }

#if LWIP_HTTPD_SUPPORT_REQUESTLIST
  clen = pbuf_clen(hs->req);
  if ((hs->req->tot_len <= LWIP_HTTPD_REQ_BUFSIZE) &&
      (clen <= LWIP_HTTPD_REQ_QUEUELEN)) {
    /* request not fully received (too short or CRLF is missing) */
    return ERR_INPROGRESS;
  } else
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
  {
#if LWIP_HTTPD_SUPPORT_POST
badrequest:
#endif /* LWIP_HTTPD_SUPPORT_POST */
    LWIP_DEBUGF(HTTPD_DEBUG, ("bad request\n"));
    /* could not parse request */
    return http_find_error_file(hs, 400);
  }
}

/** Try to find the file specified by uri and, if found, initialize hs
 * accordingly.
 *
 * @param hs the connection state
 * @param uri the HTTP header URI
 * @param is_09 1 if the request is HTTP/0.9 (no HTTP headers in response)
 * @return ERR_OK if file was found and hs has been initialized correctly
 *         another err_t otherwise
 */
static err_t
http_find_file(struct http_state *hs, const char *uri, int is_09)
{
  size_t loop;
  struct fs_file *file = NULL;
  char *params = NULL;
  err_t err;
#if LWIP_HTTPD_CGI
  int i;
#endif /* LWIP_HTTPD_CGI */
#if !LWIP_HTTPD_SSI
  const
#endif /* !LWIP_HTTPD_SSI */
  /* By default, assume we will not be processing server-side-includes tags */
  u8_t tag_check = 0;

  /* Have we been asked for the default file (in root or a directory) ? */
#if LWIP_HTTPD_MAX_REQUEST_URI_LEN
  size_t uri_len = strlen(uri);
  if ((uri_len > 0) && (uri[uri_len - 1] == '/') &&
      ((uri != http_uri_buf) || (uri_len == 1))) {
    size_t copy_len = LWIP_MIN(sizeof(http_uri_buf) - 1, uri_len - 1);
    if (copy_len > 0) {
      MEMCPY(http_uri_buf, uri, copy_len);
      http_uri_buf[copy_len] = 0;
    }
#else /* LWIP_HTTPD_MAX_REQUEST_URI_LEN */
  if ((uri[0] == '/') &&  (uri[1] == 0)) {
#endif /* LWIP_HTTPD_MAX_REQUEST_URI_LEN */
    /* Try each of the configured default filenames until we find one
       that exists. */
    for (loop = 0; loop < NUM_DEFAULT_FILENAMES; loop++) {
      const char *file_name;
#if LWIP_HTTPD_MAX_REQUEST_URI_LEN
      if (copy_len > 0) {
        size_t len_left = sizeof(http_uri_buf) - copy_len - 1;
        if (len_left > 0) {
          size_t name_len = strlen(g_psDefaultFilenames[loop].name);
          size_t name_copy_len = LWIP_MIN(len_left, name_len);
          MEMCPY(&http_uri_buf[copy_len], g_psDefaultFilenames[loop].name, name_copy_len);
        }
        file_name = http_uri_buf;
      } else
#endif /* LWIP_HTTPD_MAX_REQUEST_URI_LEN */
      {
        file_name = g_psDefaultFilenames[loop].name;
      }
      LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Looking for %s...\n", file_name));
      err = fs_open(&hs->file_handle, file_name);
      if (err == ERR_OK) {
        uri = file_name;
        file = &hs->file_handle;
        LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Opened.\n"));
#if LWIP_HTTPD_SSI
        tag_check = g_psDefaultFilenames[loop].shtml;
#endif /* LWIP_HTTPD_SSI */
        break;
      }
    }
  }
  if (file == NULL) {
    /* No - we've been asked for a specific file. */
    /* First, isolate the base URI (without any parameters) */
    params = (char *)strchr(uri, '?');
    if (params != NULL) {
      /* URI contains parameters. NULL-terminate the base URI */
      *params = '\0';
      params++;
    }

#if LWIP_HTTPD_CGI
    http_cgi_paramcount = -1;
    /* Does the base URI we have isolated correspond to a CGI handler? */
    if (g_iNumCGIs && g_pCGIs) {
      for (i = 0; i < g_iNumCGIs; i++) {
        if (strcmp(uri, g_pCGIs[i].pcCGIName) == 0) {
          /*
           * We found a CGI that handles this URI so extract the
           * parameters and call the handler.
           */
          http_cgi_paramcount = extract_uri_parameters(hs, params);
          uri = g_pCGIs[i].pfnCGIHandler(i, http_cgi_paramcount, hs->params,
                                         hs->param_vals);
          break;
        }
      }
    }
#endif /* LWIP_HTTPD_CGI */

    LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Opening %s\n", uri));

    err = fs_open(&hs->file_handle, uri);
    if (err == ERR_OK) {
      file = &hs->file_handle;
    } else {
      file = http_get_404_file(hs, &uri);
    }
#if LWIP_HTTPD_SSI
    if (file != NULL) {
      /* See if we have been asked for an shtml file and, if so,
         enable tag checking. */
      const char *ext = NULL, *sub;
      char *param = (char *)strstr(uri, "?");
      if (param != NULL) {
        /* separate uri from parameters for now, set back later */
        *param = 0;
      }
      sub = uri;
      ext = uri;
      for (sub = strstr(sub, "."); sub != NULL; sub = strstr(sub, ".")) {
        ext = sub;
        sub++;
      }
      tag_check = 0;
      for (loop = 0; loop < NUM_SHTML_EXTENSIONS; loop++) {
        if (!lwip_stricmp(ext, g_pcSSIExtensions[loop])) {
          tag_check = 1;
          break;
        }
      }
      if (param != NULL) {
        *param = '?';
      }
    }
#endif /* LWIP_HTTPD_SSI */
  }
  if (file == NULL) {
    /* None of the default filenames exist so send back a 404 page */
    file = http_get_404_file(hs, &uri);
  }
  return http_init_file(hs, file, is_09, uri, tag_check, params);
}

/** Initialize a http connection with a file to send (if found).
 * Called by http_find_file and http_find_error_file.
 *
 * @param hs http connection state
 * @param file file structure to send (or NULL if not found)
 * @param is_09 1 if the request is HTTP/0.9 (no HTTP headers in response)
 * @param uri the HTTP header URI
 * @param tag_check enable SSI tag checking
 * @param params != NULL if URI has parameters (separated by '?')
 * @return ERR_OK if file was found and hs has been initialized correctly
 *         another err_t otherwise
 */
static err_t
http_init_file(struct http_state *hs, struct fs_file *file, int is_09, const char *uri,
               u8_t tag_check, char *params)
{
  if (file != NULL) {
    /* file opened, initialise struct http_state */
#if LWIP_HTTPD_SSI
    if (tag_check) {
      struct http_ssi_state *ssi = http_ssi_state_alloc();
      if (ssi != NULL) {
        ssi->tag_index = 0;
        ssi->tag_state = TAG_NONE;
        ssi->parsed = file->data;
        ssi->parse_left = file->len;
        ssi->tag_end = file->data;
        hs->ssi = ssi;
      }
    }
#else /* LWIP_HTTPD_SSI */
    LWIP_UNUSED_ARG(tag_check);
#endif /* LWIP_HTTPD_SSI */
    hs->handle = file;
    hs->file = file->data;
    LWIP_ASSERT("File length must be positive!", (file->len >= 0));
#if LWIP_HTTPD_CUSTOM_FILES
    if (file->is_custom_file && (file->data == NULL)) {
      /* custom file, need to read data first (via fs_read_custom) */
      hs->left = 0;
    } else
#endif /* LWIP_HTTPD_CUSTOM_FILES */
    {
      hs->left = (u32_t)file->len;
    }
    hs->retries = 0;
#if LWIP_HTTPD_TIMING
    hs->time_started = sys_now();
#endif /* LWIP_HTTPD_TIMING */
#if !LWIP_HTTPD_DYNAMIC_HEADERS
    LWIP_ASSERT("HTTP headers not included in file system",
                (hs->handle->flags & FS_FILE_FLAGS_HEADER_INCLUDED) != 0);
#endif /* !LWIP_HTTPD_DYNAMIC_HEADERS */
#if LWIP_HTTPD_SUPPORT_V09
    if (is_09 && ((hs->handle->flags & FS_FILE_FLAGS_HEADER_INCLUDED) != 0)) {
      /* HTTP/0.9 responses are sent without HTTP header,
         search for the end of the header. */
      char *file_start = lwip_strnstr(hs->file, CRLF CRLF, hs->left);
      if (file_start != NULL) {
        int diff = file_start + 4 - hs->file;
        hs->file += diff;
        hs->left -= (u32_t)diff;
      }
    }
#endif /* LWIP_HTTPD_SUPPORT_V09*/
#if LWIP_HTTPD_CGI_SSI
    if (params != NULL) {
      /* URI contains parameters, call generic CGI handler */
      int count;
#if LWIP_HTTPD_CGI
      if (http_cgi_paramcount >= 0) {
        count = http_cgi_paramcount;
      } else
#endif
      {
        count = extract_uri_parameters(hs, params);
      }
      httpd_cgi_handler(uri, count, http_cgi_params, http_cgi_param_vals
#if defined(LWIP_HTTPD_FILE_STATE) && LWIP_HTTPD_FILE_STATE
                        , hs->handle->state
#endif /* LWIP_HTTPD_FILE_STATE */
                       );
    }
#else /* LWIP_HTTPD_CGI_SSI */
    LWIP_UNUSED_ARG(params);
#endif /* LWIP_HTTPD_CGI_SSI */
  } else {
    hs->handle = NULL;
    hs->file = NULL;
    hs->left = 0;
    hs->retries = 0;
  }
#if LWIP_HTTPD_DYNAMIC_HEADERS
  /* Determine the HTTP headers to send based on the file extension of
   * the requested URI. */
  if ((hs->handle == NULL) || ((hs->handle->flags & FS_FILE_FLAGS_HEADER_INCLUDED) == 0)) {
    get_http_headers(hs, uri);
  }
#else /* LWIP_HTTPD_DYNAMIC_HEADERS */
  LWIP_UNUSED_ARG(uri);
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
  if (hs->keepalive) {
#if LWIP_HTTPD_SSI
    if (hs->ssi != NULL) {
      hs->keepalive = 0;
    } else
#endif /* LWIP_HTTPD_SSI */
    {
      if ((hs->handle != NULL) &&
          ((hs->handle->flags & (FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT)) == FS_FILE_FLAGS_HEADER_INCLUDED)) {
        hs->keepalive = 0;
      }
    }
  }
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
  return ERR_OK;
}

/**
 * The pcb had an error and is already deallocated.
 * The argument might still be valid (if != NULL).
 */
static void
http_err(void *arg, err_t err)
{
  struct http_state *hs = (struct http_state *)arg;
  LWIP_UNUSED_ARG(err);

  LWIP_DEBUGF(HTTPD_DEBUG, ("http_err: %s", lwip_strerr(err)));

  if (hs != NULL) {
    http_state_free(hs);
  }
}

/**
 * Data has been sent and acknowledged by the remote host.
 * This means that more data can be sent.
 */
static err_t
http_sent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
  struct http_state *hs = (struct http_state *)arg;

  LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_sent %p\n", (void *)pcb));

  LWIP_UNUSED_ARG(len);

  if (hs == NULL) {
    return ERR_OK;
  }

  hs->retries = 0;

  http_send(pcb, hs);

  return ERR_OK;
}

/**
 * The poll function is called every 2nd second.
 * If there has been no data sent (which resets the retries) in 8 seconds, close.
 * If the last portion of a file has not been sent in 2 seconds, close.
 *
 * This could be increased, but we don't want to waste resources for bad connections.
 */
static err_t
http_poll(void *arg, struct altcp_pcb *pcb)
{
  struct http_state *hs = (struct http_state *)arg;
  LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_poll: pcb=%p hs=%p pcb_state=%s\n",
              (void *)pcb, (void *)hs, tcp_debug_state_str(altcp_dbg_get_tcp_state(pcb))));

  if (hs == NULL) {
    err_t closed;
    /* arg is null, close. */
    LWIP_DEBUGF(HTTPD_DEBUG, ("http_poll: arg is NULL, close\n"));
    closed = http_close_conn(pcb, NULL);
    LWIP_UNUSED_ARG(closed);
#if LWIP_HTTPD_ABORT_ON_CLOSE_MEM_ERROR
    if (closed == ERR_MEM) {
      altcp_abort(pcb);
      return ERR_ABRT;
    }
#endif /* LWIP_HTTPD_ABORT_ON_CLOSE_MEM_ERROR */
    return ERR_OK;
  } else {
    hs->retries++;
    if (hs->retries == HTTPD_MAX_RETRIES) {
      LWIP_DEBUGF(HTTPD_DEBUG, ("http_poll: too many retries, close\n"));
      http_close_conn(pcb, hs);
      return ERR_OK;
    }

    /* If this connection has a file open, try to send some more data. If
     * it has not yet received a GET request, don't do this since it will
     * cause the connection to close immediately. */
    if (hs && (hs->handle)) {
      LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_poll: try to send more data\n"));
      if (http_send(pcb, hs)) {
        /* If we wrote anything to be sent, go ahead and send it now. */
        LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("tcp_output\n"));
        altcp_output(pcb);
      }
    }
  }

  return ERR_OK;
}

/**
 * Data has been received on this pcb.
 * For HTTP 1.0, this should normally only happen once (if the request fits in one packet).
 */
static err_t
http_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
  struct http_state *hs = (struct http_state *)arg;
  LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_recv: pcb=%p pbuf=%p err=%s\n", (void *)pcb,
              (void *)p, lwip_strerr(err)));

  if ((err != ERR_OK) || (p == NULL) || (hs == NULL)) {
    /* error or closed by other side? */
    if (p != NULL) {
      /* Inform TCP that we have taken the data. */
      altcp_recved(pcb, p->tot_len);
      pbuf_free(p);
    }
    if (hs == NULL) {
      /* this should not happen, only to be robust */
      LWIP_DEBUGF(HTTPD_DEBUG, ("Error, http_recv: hs is NULL, close\n"));
    }
    http_close_conn(pcb, hs);
    return ERR_OK;
  }

#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
  if (hs->no_auto_wnd) {
    hs->unrecved_bytes += p->tot_len;
  } else
#endif /* LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND */
  {
    /* Inform TCP that we have taken the data. */
    altcp_recved(pcb, p->tot_len);
  }

#if LWIP_HTTPD_SUPPORT_POST
  if (hs->post_content_len_left > 0) {
    /* reset idle counter when POST data is received */
    hs->retries = 0;
    /* this is data for a POST, pass the complete pbuf to the application */
    http_post_rxpbuf(hs, p);
    /* pbuf is passed to the application, don't free it! */
    if (hs->post_content_len_left == 0) {
      /* all data received, send response or close connection */
      http_send(pcb, hs);
    }
    return ERR_OK;
  } else
#endif /* LWIP_HTTPD_SUPPORT_POST */
  {
    if (hs->handle == NULL) {
      err_t parsed = http_parse_request(p, hs, pcb);
      LWIP_ASSERT("http_parse_request: unexpected return value", parsed == ERR_OK
                  || parsed == ERR_INPROGRESS || parsed == ERR_ARG || parsed == ERR_USE);
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
      if (parsed != ERR_INPROGRESS) {
        /* request fully parsed or error */
        if (hs->req != NULL) {
          pbuf_free(hs->req);
          hs->req = NULL;
        }
      }
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
      pbuf_free(p);
      if (parsed == ERR_OK) {
#if LWIP_HTTPD_SUPPORT_POST
        if (hs->post_content_len_left == 0)
#endif /* LWIP_HTTPD_SUPPORT_POST */
        {
          LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_recv: data %p len %"S32_F"\n", (const void *)hs->file, hs->left));
          http_send(pcb, hs);
        }
      } else if (parsed == ERR_ARG) {
        /* @todo: close on ERR_USE? */
        http_close_conn(pcb, hs);
      }
    } else {
      LWIP_DEBUGF(HTTPD_DEBUG, ("http_recv: already sending data\n"));
      /* already sending but still receiving data, we might want to RST here? */
      pbuf_free(p);
    }
  }
  return ERR_OK;
}

/**
 * A new incoming connection has been accepted.
 */
static err_t
http_accept(void *arg, struct altcp_pcb *pcb, err_t err)
{
  struct http_state *hs;
  LWIP_UNUSED_ARG(err);
  LWIP_UNUSED_ARG(arg);
  LWIP_DEBUGF(HTTPD_DEBUG, ("http_accept %p / %p\n", (void *)pcb, arg));

  if ((err != ERR_OK) || (pcb == NULL)) {
    return ERR_VAL;
  }

  /* Set priority */
  altcp_setprio(pcb, HTTPD_TCP_PRIO);

  /* Allocate memory for the structure that holds the state of the
     connection - initialized by that function. */
  hs = http_state_alloc();
  if (hs == NULL) {
    LWIP_DEBUGF(HTTPD_DEBUG, ("http_accept: Out of memory, RST\n"));
    return ERR_MEM;
  }
  hs->pcb = pcb;

  /* Tell TCP that this is the structure we wish to be passed for our
     callbacks. */
  altcp_arg(pcb, hs);

  /* Set up the various callback functions */
  altcp_recv(pcb, http_recv);
  altcp_err(pcb, http_err);
  altcp_poll(pcb, http_poll, HTTPD_POLL_INTERVAL);
  altcp_sent(pcb, http_sent);

  return ERR_OK;
}

static void
httpd_init_pcb(struct altcp_pcb *pcb, u16_t port)
{
  err_t err;

  if (pcb) {
    altcp_setprio(pcb, HTTPD_TCP_PRIO);
    /* set SOF_REUSEADDR here to explicitly bind httpd to multiple interfaces */
    err = altcp_bind(pcb, IP_ANY_TYPE, port);
    LWIP_UNUSED_ARG(err); /* in case of LWIP_NOASSERT */
    LWIP_ASSERT("httpd_init: tcp_bind failed", err == ERR_OK);
    pcb = altcp_listen(pcb);
    LWIP_ASSERT("httpd_init: tcp_listen failed", pcb != NULL);
    altcp_accept(pcb, http_accept);
  }
}

/**
 * @ingroup httpd
 * Initialize the httpd: set up a listening PCB and bind it to the defined port
 */
void
httpd_init(void)
{
  struct altcp_pcb *pcb;

#if HTTPD_USE_MEM_POOL
  LWIP_MEMPOOL_INIT(HTTPD_STATE);
#if LWIP_HTTPD_SSI
  LWIP_MEMPOOL_INIT(HTTPD_SSI_STATE);
#endif
#endif
  LWIP_DEBUGF(HTTPD_DEBUG, ("httpd_init\n"));

  pcb = altcp_tcp_new_ip_type(IPADDR_TYPE_ANY);
  LWIP_ASSERT("httpd_init: tcp_new failed", pcb != NULL);
  httpd_init_pcb(pcb, HTTPD_SERVER_PORT);
}

#if HTTPD_ENABLE_HTTPS
/**
 * @ingroup httpd
 * Initialize the httpd: set up a listening PCB and bind it to the defined port.
 * Also set up TLS connection handling (HTTPS).
 */
void
httpd_inits(struct altcp_tls_config *conf)
{
#if LWIP_ALTCP_TLS
  struct altcp_pcb *pcb_tls;
  struct altcp_pcb *pcb_tcp = altcp_tcp_new_ip_type(IPADDR_TYPE_ANY);
  LWIP_ASSERT("httpd_init: tcp_new failed", pcb_tcp != NULL);
  pcb_tls = altcp_tls_new(conf, pcb_tcp);
  LWIP_ASSERT("httpd_init: altcp_tls_new failed", pcb_tls != NULL);
  httpd_init_pcb(pcb_tls, HTTPD_SERVER_PORT_HTTPS);
#else /* LWIP_ALTCP_TLS */
  LWIP_UNUSED_ARG(conf);
#endif /* LWIP_ALTCP_TLS */
}
#endif /* HTTPD_ENABLE_HTTPS */

#if LWIP_HTTPD_SSI
/**
 * Set the SSI handler function.
 *
 * @param ssi_handler the SSI handler function
 * @param tags an array of SSI tag strings to search for in SSI-enabled files
 * @param num_tags number of tags in the 'tags' array
 */
void
http_set_ssi_handler(tSSIHandler ssi_handler, const char **tags, int num_tags)
{
  LWIP_DEBUGF(HTTPD_DEBUG, ("http_set_ssi_handler\n"));

  LWIP_ASSERT("no ssi_handler given", ssi_handler != NULL);
  g_pfnSSIHandler = ssi_handler;

#if LWIP_HTTPD_SSI_RAW
  LWIP_UNUSED_ARG(tags);
  LWIP_UNUSED_ARG(num_tags);
#else /* LWIP_HTTPD_SSI_RAW */
  LWIP_ASSERT("no tags given", tags != NULL);
  LWIP_ASSERT("invalid number of tags", num_tags > 0);

  g_ppcTags = tags;
  g_iNumTags = num_tags;
#endif /* !LWIP_HTTPD_SSI_RAW */
}
#endif /* LWIP_HTTPD_SSI */

#if LWIP_HTTPD_CGI
/**
 * Set an array of CGI filenames/handler functions
 *
 * @param cgis an array of CGI filenames/handler functions
 * @param num_handlers number of elements in the 'cgis' array
 */
void
http_set_cgi_handlers(const tCGI *cgis, int num_handlers)
{
  LWIP_ASSERT("no cgis given", cgis != NULL);
  LWIP_ASSERT("invalid number of handlers", num_handlers > 0);

  g_pCGIs = cgis;
  g_iNumCGIs = num_handlers;
}
#endif /* LWIP_HTTPD_CGI */

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
