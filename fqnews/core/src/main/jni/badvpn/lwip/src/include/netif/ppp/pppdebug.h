/*****************************************************************************
* pppdebug.h - System debugging utilities.
*
* Copyright (c) 2003 by Marc Boucher, Services Informatiques (MBSI) inc.
* portions Copyright (c) 1998 Global Election Systems Inc.
* portions Copyright (c) 2001 by Cognizant Pty Ltd.
*
* The authors hereby grant permission to use, copy, modify, distribute,
* and license this software and its documentation for any purpose, provided
* that existing copyright notices are retained in all copies and that this
* notice and the following disclaimer are included verbatim in any 
* distributions. No written agreement, license, or royalty fee is required
* for any of the authorized uses.
*
* THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS *AS IS* AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
* IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
* REVISION HISTORY (please don't use tabs!)
*
* 03-01-01 Marc Boucher <marc@mbsi.ca>
*   Ported to lwIP.
* 98-07-29 Guy Lancaster <lancasterg@acm.org>, Global Election Systems Inc.
*   Original.
*
*****************************************************************************
*/

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT /* don't build if not configured for use in lwipopts.h */

#ifndef PPPDEBUG_H
#define PPPDEBUG_H

/* Trace levels. */
#define LOG_CRITICAL  (PPP_DEBUG | LWIP_DBG_LEVEL_SEVERE)
#define LOG_ERR       (PPP_DEBUG | LWIP_DBG_LEVEL_SEVERE)
#define LOG_NOTICE    (PPP_DEBUG | LWIP_DBG_LEVEL_WARNING)
#define LOG_WARNING   (PPP_DEBUG | LWIP_DBG_LEVEL_WARNING)
#define LOG_INFO      (PPP_DEBUG)
#define LOG_DETAIL    (PPP_DEBUG)
#define LOG_DEBUG     (PPP_DEBUG)

#if PPP_DEBUG

#define MAINDEBUG(a)    LWIP_DEBUGF(LWIP_DBG_LEVEL_WARNING, a)
#define SYSDEBUG(a)     LWIP_DEBUGF(LWIP_DBG_LEVEL_WARNING, a)
#define FSMDEBUG(a)     LWIP_DEBUGF(LWIP_DBG_LEVEL_WARNING, a)
#define LCPDEBUG(a)     LWIP_DEBUGF(LWIP_DBG_LEVEL_WARNING, a)
#define IPCPDEBUG(a)    LWIP_DEBUGF(LWIP_DBG_LEVEL_WARNING, a)
#define IPV6CPDEBUG(a)  LWIP_DEBUGF(LWIP_DBG_LEVEL_WARNING, a)
#define UPAPDEBUG(a)    LWIP_DEBUGF(LWIP_DBG_LEVEL_WARNING, a)
#define CHAPDEBUG(a)    LWIP_DEBUGF(LWIP_DBG_LEVEL_WARNING, a)
#define PPPDEBUG(a, b)  LWIP_DEBUGF(a, b)

#else /* PPP_DEBUG */

#define MAINDEBUG(a)
#define SYSDEBUG(a)
#define FSMDEBUG(a)
#define LCPDEBUG(a)
#define IPCPDEBUG(a)
#define IPV6CPDEBUG(a)
#define UPAPDEBUG(a)
#define CHAPDEBUG(a)
#define PPPDEBUG(a, b)

#endif /* PPP_DEBUG */

#endif /* PPPDEBUG_H */

#endif /* PPP_SUPPORT */
