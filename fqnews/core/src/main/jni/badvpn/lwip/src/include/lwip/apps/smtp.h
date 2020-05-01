#ifndef LWIP_HDR_APPS_SMTP_H
#define LWIP_HDR_APPS_SMTP_H

#include "lwip/apps/smtp_opts.h"
#include "lwip/err.h"
#include "lwip/prot/iana.h"

/** The default TCP port used for SMTP */
#define SMTP_DEFAULT_PORT         LWIP_IANA_PORT_SMTP
/** The default TCP port used for SMTPS */
#define SMTPS_DEFAULT_PORT        LWIP_IANA_PORT_SMTPS

/** Email successfully sent */
#define SMTP_RESULT_OK            0
/** Unknown error */
#define SMTP_RESULT_ERR_UNKNOWN   1
/** Connection to server failed */
#define SMTP_RESULT_ERR_CONNECT   2
/** Failed to resolve server hostname */
#define SMTP_RESULT_ERR_HOSTNAME  3
/** Connection unexpectedly closed by remote server */
#define SMTP_RESULT_ERR_CLOSED    4
/** Connection timed out (server didn't respond in time) */
#define SMTP_RESULT_ERR_TIMEOUT   5
/** Server responded with an unknown response code */
#define SMTP_RESULT_ERR_SVR_RESP  6
/** Out of resources locally */
#define SMTP_RESULT_ERR_MEM       7

/** Prototype of an smtp callback function
 *
 * @param arg argument specified when initiating the email
 * @param smtp_result result of the mail transfer (see defines SMTP_RESULT_*)
 * @param srv_err if aborted by the server, this contains the error code received
 * @param err an error returned by internal lwip functions, can help to specify
 *            the source of the error but must not necessarily be != ERR_OK
 */
typedef void (*smtp_result_fn)(void *arg, u8_t smtp_result, u16_t srv_err, err_t err);

/** This structure is used as argument for smtp_send_mail_int(),
 * which in turn can be used with tcpip_callback() to send mail
 * from interrupt context, e.g. like this:
 *    struct smtp_send_request *req; (to be filled)
 *    tcpip_try_callback(smtp_send_mail_int, (void*)req);
 *
 * For member description, see parameter description of smtp_send_mail().
 * When using with tcpip_callback, this structure has to stay allocated
 * (e.g. using mem_malloc/mem_free) until its 'callback_fn' is called.
 */
struct smtp_send_request {
  const char *from;
  const char* to;
  const char* subject;
  const char* body;
  smtp_result_fn callback_fn;
  void* callback_arg;
  /** If this is != 0, data is *not* copied into an extra buffer
   * but used from the pointers supplied in this struct.
   * This means less memory usage, but data must stay untouched until
   * the callback function is called. */
  u8_t static_data;
};


#if SMTP_BODYDH

#ifndef SMTP_BODYDH_BUFFER_SIZE
#define SMTP_BODYDH_BUFFER_SIZE 256
#endif /* SMTP_BODYDH_BUFFER_SIZE */

struct smtp_bodydh {
  u16_t state;
  u16_t length; /* Length of content in buffer */
  char buffer[SMTP_BODYDH_BUFFER_SIZE]; /* buffer for generated content */
#ifdef SMTP_BODYDH_USER_SIZE
  u8_t user[SMTP_BODYDH_USER_SIZE];
#endif /* SMTP_BODYDH_USER_SIZE */
};

enum bdh_retvals_e {
  BDH_DONE = 0,
  BDH_WORKING
};

/** Prototype of an smtp body callback function
 * It receives a struct smtp_bodydh, and a buffer to write data,
 * must return BDH_WORKING to be called again and BDH_DONE when
 * it has finished processing. This one tries to fill one TCP buffer with
 * data, your function will be repeatedly called until that happens; so if you
 * know you'll be taking too long to serve your request, pause once in a while
 * by writing length=0 to avoid hogging system resources
 *
 * @param arg argument specified when initiating the email
 * @param smtp_bodydh state handling + buffer structure
 */
typedef int (*smtp_bodycback_fn)(void *arg, struct smtp_bodydh *bodydh);

err_t smtp_send_mail_bodycback(const char *from, const char* to, const char* subject,
                     smtp_bodycback_fn bodycback_fn, smtp_result_fn callback_fn, void* callback_arg);

#endif /* SMTP_BODYDH */


err_t smtp_set_server_addr(const char* server);
void smtp_set_server_port(u16_t port);
#if LWIP_ALTCP && LWIP_ALTCP_TLS
struct altcp_tls_config;
void smtp_set_tls_config(struct altcp_tls_config *tls_config);
#endif
err_t smtp_set_auth(const char* username, const char* pass);
err_t smtp_send_mail(const char *from, const char* to, const char* subject, const char* body,
                     smtp_result_fn callback_fn, void* callback_arg);
err_t smtp_send_mail_static(const char *from, const char* to, const char* subject, const char* body,
                     smtp_result_fn callback_fn, void* callback_arg);
void smtp_send_mail_int(void *arg);
#ifdef LWIP_DEBUG
const char* smtp_result_str(u8_t smtp_result);
#endif

#endif /* LWIP_HDR_APPS_SMTP_H */
