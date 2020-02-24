# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/bio.h>
"""

TYPES = """
typedef struct bio_st BIO;
typedef void bio_info_cb(BIO *, int, const char *, int, long, long);
struct bio_method_st {
    int type;
    const char *name;
    int (*bwrite)(BIO *, const char *, int);
    int (*bread)(BIO *, char *, int);
    int (*bputs)(BIO *, const char *);
    int (*bgets)(BIO *, char *, int);
    long (*ctrl)(BIO *, int, long, void *);
    int (*create)(BIO *);
    int (*destroy)(BIO *);
    long (*callback_ctrl)(BIO *, int, bio_info_cb *);
    ...;
};
typedef struct bio_method_st BIO_METHOD;
struct bio_st {
    BIO_METHOD *method;
    long (*callback)(struct bio_st *, int, const char *, int, long, long);
    char *cb_arg;
    int init;
    int shutdown;
    int flags;
    int retry_reason;
    int num;
    void *ptr;
    struct bio_st *next_bio;
    struct bio_st *prev_bio;
    int references;
    unsigned long num_read;
    unsigned long num_write;
    ...;
};
typedef ... BUF_MEM;

static const int BIO_TYPE_MEM;
static const int BIO_TYPE_FILE;
static const int BIO_TYPE_FD;
static const int BIO_TYPE_SOCKET;
static const int BIO_TYPE_CONNECT;
static const int BIO_TYPE_ACCEPT;
static const int BIO_TYPE_NULL;
static const int BIO_CLOSE;
static const int BIO_NOCLOSE;
static const int BIO_TYPE_SOURCE_SINK;
static const int BIO_CTRL_RESET;
static const int BIO_CTRL_EOF;
static const int BIO_CTRL_SET;
static const int BIO_CTRL_SET_CLOSE;
static const int BIO_CTRL_FLUSH;
static const int BIO_CTRL_DUP;
static const int BIO_CTRL_GET_CLOSE;
static const int BIO_CTRL_INFO;
static const int BIO_CTRL_GET;
static const int BIO_CTRL_PENDING;
static const int BIO_CTRL_WPENDING;
static const int BIO_C_FILE_SEEK;
static const int BIO_C_FILE_TELL;
static const int BIO_TYPE_NONE;
static const int BIO_TYPE_PROXY_CLIENT;
static const int BIO_TYPE_PROXY_SERVER;
static const int BIO_TYPE_NBIO_TEST;
static const int BIO_TYPE_BER;
static const int BIO_TYPE_BIO;
static const int BIO_TYPE_DESCRIPTOR;
static const int BIO_FLAGS_READ;
static const int BIO_FLAGS_WRITE;
static const int BIO_FLAGS_IO_SPECIAL;
static const int BIO_FLAGS_RWS;
static const int BIO_FLAGS_SHOULD_RETRY;
static const int BIO_TYPE_NULL_FILTER;
static const int BIO_TYPE_SSL;
static const int BIO_TYPE_MD;
static const int BIO_TYPE_BUFFER;
static const int BIO_TYPE_CIPHER;
static const int BIO_TYPE_BASE64;
static const int BIO_TYPE_FILTER;
"""

FUNCTIONS = """
BIO *BIO_new(BIO_METHOD *);
int BIO_set(BIO *, BIO_METHOD *);
int BIO_free(BIO *);
void BIO_vfree(BIO *);
void BIO_free_all(BIO *);
BIO *BIO_push(BIO *, BIO *);
BIO *BIO_pop(BIO *);
BIO *BIO_next(BIO *);
BIO *BIO_find_type(BIO *, int);
BIO_METHOD *BIO_s_mem(void);
BIO *BIO_new_mem_buf(void *, int);
BIO_METHOD *BIO_s_file(void);
BIO *BIO_new_file(const char *, const char *);
BIO *BIO_new_fp(FILE *, int);
BIO_METHOD *BIO_s_fd(void);
BIO *BIO_new_fd(int, int);
BIO_METHOD *BIO_s_socket(void);
BIO *BIO_new_socket(int, int);
BIO_METHOD *BIO_s_null(void);
long BIO_ctrl(BIO *, int, long, void *);
long BIO_callback_ctrl(
    BIO *,
    int,
    void (*)(struct bio_st *, int, const char *, int, long, long)
);
char *BIO_ptr_ctrl(BIO *, int, long);
long BIO_int_ctrl(BIO *, int, long, int);
size_t BIO_ctrl_pending(BIO *);
size_t BIO_ctrl_wpending(BIO *);
int BIO_read(BIO *, void *, int);
int BIO_gets(BIO *, char *, int);
int BIO_write(BIO *, const void *, int);
int BIO_puts(BIO *, const char *);
BIO_METHOD *BIO_f_null(void);
BIO_METHOD *BIO_f_buffer(void);
"""

MACROS = """
long BIO_set_fd(BIO *, long, int);
long BIO_get_fd(BIO *, char *);
long BIO_set_mem_eof_return(BIO *, int);
long BIO_get_mem_data(BIO *, char **);
long BIO_set_mem_buf(BIO *, BUF_MEM *, int);
long BIO_get_mem_ptr(BIO *, BUF_MEM **);
long BIO_set_fp(BIO *, FILE *, int);
long BIO_get_fp(BIO *, FILE **);
long BIO_read_filename(BIO *, char *);
long BIO_write_filename(BIO *, char *);
long BIO_append_filename(BIO *, char *);
long BIO_rw_filename(BIO *, char *);
int BIO_should_read(BIO *);
int BIO_should_write(BIO *);
int BIO_should_io_special(BIO *);
int BIO_retry_type(BIO *);
int BIO_should_retry(BIO *);
int BIO_reset(BIO *);
int BIO_seek(BIO *, int);
int BIO_tell(BIO *);
int BIO_flush(BIO *);
int BIO_eof(BIO *);
int BIO_set_close(BIO *,long);
int BIO_get_close(BIO *);
int BIO_pending(BIO *);
int BIO_wpending(BIO *);
int BIO_get_info_callback(BIO *, bio_info_cb **);
int BIO_set_info_callback(BIO *, bio_info_cb *);
long BIO_get_buffer_num_lines(BIO *);
long BIO_set_read_buffer_size(BIO *, long);
long BIO_set_write_buffer_size(BIO *, long);
long BIO_set_buffer_size(BIO *, long);
long BIO_set_buffer_read_data(BIO *, void *, long);

/* The following was a macro in 0.9.8e. Once we drop support for RHEL/CentOS 5
   we should move this back to FUNCTIONS. */
int BIO_method_type(const BIO *);
"""

CUSTOMIZATIONS = """
"""

CONDITIONAL_NAMES = {}
