#ifndef LWIP_HDR_APPS_SMTP_OPTS_H
#define LWIP_HDR_APPS_SMTP_OPTS_H

#include "lwip/opt.h"

#ifdef __cplusplus
extern "C" {
#endif
    
/**
 * @defgroup smtp_opts Options
 * @ingroup smtp
 * 
 * @{
 */
    
/** Set this to 1 to enable data handler callback on BODY */
#ifndef SMTP_BODYDH
#define SMTP_BODYDH             0
#endif

/** SMTP_DEBUG: Enable debugging for SNTP. */
#ifndef SMTP_DEBUG
#define SMTP_DEBUG              LWIP_DBG_OFF
#endif

/** Maximum length reserved for server name including terminating 0 byte */
#ifndef SMTP_MAX_SERVERNAME_LEN
#define SMTP_MAX_SERVERNAME_LEN 256
#endif

/** Maximum length reserved for username */
#ifndef SMTP_MAX_USERNAME_LEN
#define SMTP_MAX_USERNAME_LEN   32
#endif

/** Maximum length reserved for password */
#ifndef SMTP_MAX_PASS_LEN
#define SMTP_MAX_PASS_LEN       32
#endif

/** Set this to 0 if you know the authentication data will not change
 * during the smtp session, which saves some heap space. */
#ifndef SMTP_COPY_AUTHDATA
#define SMTP_COPY_AUTHDATA      1
#endif

/** Set this to 0 to save some code space if you know for sure that all data
 * passed to this module conforms to the requirements in the SMTP RFC.
 * WARNING: use this with care!
 */
#ifndef SMTP_CHECK_DATA
#define SMTP_CHECK_DATA         1
#endif

/** Set this to 1 to enable AUTH PLAIN support */
#ifndef SMTP_SUPPORT_AUTH_PLAIN
#define SMTP_SUPPORT_AUTH_PLAIN 1
#endif

/** Set this to 1 to enable AUTH LOGIN support */
#ifndef SMTP_SUPPORT_AUTH_LOGIN
#define SMTP_SUPPORT_AUTH_LOGIN 1
#endif

/* Memory allocation/deallocation can be overridden... */
#ifndef SMTP_STATE_MALLOC
#define SMTP_STATE_MALLOC(size)       mem_malloc(size)
#define SMTP_STATE_FREE(ptr)          mem_free(ptr)
#endif

/**
 * @}
 */
    
#ifdef __cplusplus
}
#endif

#endif /* SMTP_OPTS_H */

