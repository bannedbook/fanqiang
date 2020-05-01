/*
 * upap.c - User/Password Authentication Protocol.
 *
 * Copyright (c) 1984-2000 Carnegie Mellon University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && PAP_SUPPORT  /* don't build if not configured for use in lwipopts.h */

/*
 * @todo:
 */

#if 0 /* UNUSED */
#include <stdio.h>
#include <string.h>
#endif /* UNUSED */

#include "netif/ppp/ppp_impl.h"

#include "netif/ppp/upap.h"

#if PPP_OPTIONS
/*
 * Command-line options.
 */
static option_t pap_option_list[] = {
    { "hide-password", o_bool, &hide_password,
      "Don't output passwords to log", OPT_PRIO | 1 },
    { "show-password", o_bool, &hide_password,
      "Show password string in debug log messages", OPT_PRIOSUB | 0 },

    { "pap-restart", o_int, &upap[0].us_timeouttime,
      "Set retransmit timeout for PAP", OPT_PRIO },
    { "pap-max-authreq", o_int, &upap[0].us_maxtransmits,
      "Set max number of transmissions for auth-reqs", OPT_PRIO },
    { "pap-timeout", o_int, &upap[0].us_reqtimeout,
      "Set time limit for peer PAP authentication", OPT_PRIO },

    { NULL }
};
#endif /* PPP_OPTIONS */

/*
 * Protocol entry points.
 */
static void upap_init(ppp_pcb *pcb);
static void upap_lowerup(ppp_pcb *pcb);
static void upap_lowerdown(ppp_pcb *pcb);
static void upap_input(ppp_pcb *pcb, u_char *inpacket, int l);
static void upap_protrej(ppp_pcb *pcb);
#if PRINTPKT_SUPPORT
static int upap_printpkt(const u_char *p, int plen, void (*printer) (void *, const char *, ...), void *arg);
#endif /* PRINTPKT_SUPPORT */

const struct protent pap_protent = {
    PPP_PAP,
    upap_init,
    upap_input,
    upap_protrej,
    upap_lowerup,
    upap_lowerdown,
    NULL,
    NULL,
#if PRINTPKT_SUPPORT
    upap_printpkt,
#endif /* PRINTPKT_SUPPORT */
#if PPP_DATAINPUT
    NULL,
#endif /* PPP_DATAINPUT */
#if PRINTPKT_SUPPORT
    "PAP",
    NULL,
#endif /* PRINTPKT_SUPPORT */
#if PPP_OPTIONS
    pap_option_list,
    NULL,
#endif /* PPP_OPTIONS */
#if DEMAND_SUPPORT
    NULL,
    NULL
#endif /* DEMAND_SUPPORT */
};

static void upap_timeout(void *arg);
#if PPP_SERVER
static void upap_reqtimeout(void *arg);
static void upap_rauthreq(ppp_pcb *pcb, u_char *inp, int id, int len);
#endif /* PPP_SERVER */
static void upap_rauthack(ppp_pcb *pcb, u_char *inp, int id, int len);
static void upap_rauthnak(ppp_pcb *pcb, u_char *inp, int id, int len);
static void upap_sauthreq(ppp_pcb *pcb);
#if PPP_SERVER
static void upap_sresp(ppp_pcb *pcb, u_char code, u_char id, const char *msg, int msglen);
#endif /* PPP_SERVER */


/*
 * upap_init - Initialize a UPAP unit.
 */
static void upap_init(ppp_pcb *pcb) {
    pcb->upap.us_user = NULL;
    pcb->upap.us_userlen = 0;
    pcb->upap.us_passwd = NULL;
    pcb->upap.us_passwdlen = 0;
    pcb->upap.us_clientstate = UPAPCS_INITIAL;
#if PPP_SERVER
    pcb->upap.us_serverstate = UPAPSS_INITIAL;
#endif /* PPP_SERVER */
    pcb->upap.us_id = 0;
}


/*
 * upap_authwithpeer - Authenticate us with our peer (start client).
 *
 * Set new state and send authenticate's.
 */
void upap_authwithpeer(ppp_pcb *pcb, const char *user, const char *password) {

    if(!user || !password)
        return;

    /* Save the username and password we're given */
    pcb->upap.us_user = user;
    pcb->upap.us_userlen = LWIP_MIN(strlen(user), 0xff);
    pcb->upap.us_passwd = password;
    pcb->upap.us_passwdlen = LWIP_MIN(strlen(password), 0xff);
    pcb->upap.us_transmits = 0;

    /* Lower layer up yet? */
    if (pcb->upap.us_clientstate == UPAPCS_INITIAL ||
	pcb->upap.us_clientstate == UPAPCS_PENDING) {
	pcb->upap.us_clientstate = UPAPCS_PENDING;
	return;
    }

    upap_sauthreq(pcb);		/* Start protocol */
}

#if PPP_SERVER
/*
 * upap_authpeer - Authenticate our peer (start server).
 *
 * Set new state.
 */
void upap_authpeer(ppp_pcb *pcb) {

    /* Lower layer up yet? */
    if (pcb->upap.us_serverstate == UPAPSS_INITIAL ||
	pcb->upap.us_serverstate == UPAPSS_PENDING) {
	pcb->upap.us_serverstate = UPAPSS_PENDING;
	return;
    }

    pcb->upap.us_serverstate = UPAPSS_LISTEN;
    if (pcb->settings.pap_req_timeout > 0)
	TIMEOUT(upap_reqtimeout, pcb, pcb->settings.pap_req_timeout);
}
#endif /* PPP_SERVER */

/*
 * upap_timeout - Retransmission timer for sending auth-reqs expired.
 */
static void upap_timeout(void *arg) {
    ppp_pcb *pcb = (ppp_pcb*)arg;

    if (pcb->upap.us_clientstate != UPAPCS_AUTHREQ)
	return;

    if (pcb->upap.us_transmits >= pcb->settings.pap_max_transmits) {
	/* give up in disgust */
	ppp_error("No response to PAP authenticate-requests");
	pcb->upap.us_clientstate = UPAPCS_BADAUTH;
	auth_withpeer_fail(pcb, PPP_PAP);
	return;
    }

    upap_sauthreq(pcb);		/* Send Authenticate-Request */
}


#if PPP_SERVER
/*
 * upap_reqtimeout - Give up waiting for the peer to send an auth-req.
 */
static void upap_reqtimeout(void *arg) {
    ppp_pcb *pcb = (ppp_pcb*)arg;

    if (pcb->upap.us_serverstate != UPAPSS_LISTEN)
	return;			/* huh?? */

    auth_peer_fail(pcb, PPP_PAP);
    pcb->upap.us_serverstate = UPAPSS_BADAUTH;
}
#endif /* PPP_SERVER */


/*
 * upap_lowerup - The lower layer is up.
 *
 * Start authenticating if pending.
 */
static void upap_lowerup(ppp_pcb *pcb) {

    if (pcb->upap.us_clientstate == UPAPCS_INITIAL)
	pcb->upap.us_clientstate = UPAPCS_CLOSED;
    else if (pcb->upap.us_clientstate == UPAPCS_PENDING) {
	upap_sauthreq(pcb);	/* send an auth-request */
    }

#if PPP_SERVER
    if (pcb->upap.us_serverstate == UPAPSS_INITIAL)
	pcb->upap.us_serverstate = UPAPSS_CLOSED;
    else if (pcb->upap.us_serverstate == UPAPSS_PENDING) {
	pcb->upap.us_serverstate = UPAPSS_LISTEN;
	if (pcb->settings.pap_req_timeout > 0)
	    TIMEOUT(upap_reqtimeout, pcb, pcb->settings.pap_req_timeout);
    }
#endif /* PPP_SERVER */
}


/*
 * upap_lowerdown - The lower layer is down.
 *
 * Cancel all timeouts.
 */
static void upap_lowerdown(ppp_pcb *pcb) {

    if (pcb->upap.us_clientstate == UPAPCS_AUTHREQ)	/* Timeout pending? */
	UNTIMEOUT(upap_timeout, pcb);		/* Cancel timeout */
#if PPP_SERVER
    if (pcb->upap.us_serverstate == UPAPSS_LISTEN && pcb->settings.pap_req_timeout > 0)
	UNTIMEOUT(upap_reqtimeout, pcb);
#endif /* PPP_SERVER */

    pcb->upap.us_clientstate = UPAPCS_INITIAL;
#if PPP_SERVER
    pcb->upap.us_serverstate = UPAPSS_INITIAL;
#endif /* PPP_SERVER */
}


/*
 * upap_protrej - Peer doesn't speak this protocol.
 *
 * This shouldn't happen.  In any case, pretend lower layer went down.
 */
static void upap_protrej(ppp_pcb *pcb) {

    if (pcb->upap.us_clientstate == UPAPCS_AUTHREQ) {
	ppp_error("PAP authentication failed due to protocol-reject");
	auth_withpeer_fail(pcb, PPP_PAP);
    }
#if PPP_SERVER
    if (pcb->upap.us_serverstate == UPAPSS_LISTEN) {
	ppp_error("PAP authentication of peer failed (protocol-reject)");
	auth_peer_fail(pcb, PPP_PAP);
    }
#endif /* PPP_SERVER */
    upap_lowerdown(pcb);
}


/*
 * upap_input - Input UPAP packet.
 */
static void upap_input(ppp_pcb *pcb, u_char *inpacket, int l) {
    u_char *inp;
    u_char code, id;
    int len;

    /*
     * Parse header (code, id and length).
     * If packet too short, drop it.
     */
    inp = inpacket;
    if (l < UPAP_HEADERLEN) {
	UPAPDEBUG(("pap_input: rcvd short header."));
	return;
    }
    GETCHAR(code, inp);
    GETCHAR(id, inp);
    GETSHORT(len, inp);
    if (len < UPAP_HEADERLEN) {
	UPAPDEBUG(("pap_input: rcvd illegal length."));
	return;
    }
    if (len > l) {
	UPAPDEBUG(("pap_input: rcvd short packet."));
	return;
    }
    len -= UPAP_HEADERLEN;

    /*
     * Action depends on code.
     */
    switch (code) {
    case UPAP_AUTHREQ:
#if PPP_SERVER
	upap_rauthreq(pcb, inp, id, len);
#endif /* PPP_SERVER */
	break;

    case UPAP_AUTHACK:
	upap_rauthack(pcb, inp, id, len);
	break;

    case UPAP_AUTHNAK:
	upap_rauthnak(pcb, inp, id, len);
	break;

    default:				/* XXX Need code reject */
	break;
    }
}

#if PPP_SERVER
/*
 * upap_rauth - Receive Authenticate.
 */
static void upap_rauthreq(ppp_pcb *pcb, u_char *inp, int id, int len) {
    u_char ruserlen, rpasswdlen;
    char *ruser;
    char *rpasswd;
    char rhostname[256];
    int retcode;
    const char *msg;
    int msglen;

    if (pcb->upap.us_serverstate < UPAPSS_LISTEN)
	return;

    /*
     * If we receive a duplicate authenticate-request, we are
     * supposed to return the same status as for the first request.
     */
    if (pcb->upap.us_serverstate == UPAPSS_OPEN) {
	upap_sresp(pcb, UPAP_AUTHACK, id, "", 0);	/* return auth-ack */
	return;
    }
    if (pcb->upap.us_serverstate == UPAPSS_BADAUTH) {
	upap_sresp(pcb, UPAP_AUTHNAK, id, "", 0);	/* return auth-nak */
	return;
    }

    /*
     * Parse user/passwd.
     */
    if (len < 1) {
	UPAPDEBUG(("pap_rauth: rcvd short packet."));
	return;
    }
    GETCHAR(ruserlen, inp);
    len -= sizeof (u_char) + ruserlen + sizeof (u_char);
    if (len < 0) {
	UPAPDEBUG(("pap_rauth: rcvd short packet."));
	return;
    }
    ruser = (char *) inp;
    INCPTR(ruserlen, inp);
    GETCHAR(rpasswdlen, inp);
    if (len < rpasswdlen) {
	UPAPDEBUG(("pap_rauth: rcvd short packet."));
	return;
    }

    rpasswd = (char *) inp;

    /*
     * Check the username and password given.
     */
    retcode = UPAP_AUTHNAK;
    if (auth_check_passwd(pcb, ruser, ruserlen, rpasswd, rpasswdlen, &msg, &msglen)) {
      retcode = UPAP_AUTHACK;
    }
    BZERO(rpasswd, rpasswdlen);

#if 0 /* UNUSED */
    /*
     * Check remote number authorization.  A plugin may have filled in
     * the remote number or added an allowed number, and rather than
     * return an authenticate failure, is leaving it for us to verify.
     */
    if (retcode == UPAP_AUTHACK) {
	if (!auth_number()) {
	    /* We do not want to leak info about the pap result. */
	    retcode = UPAP_AUTHNAK; /* XXX exit value will be "wrong" */
	    warn("calling number %q is not authorized", remote_number);
	}
    }

    msglen = strlen(msg);
    if (msglen > 255)
	msglen = 255;
#endif /* UNUSED */

    upap_sresp(pcb, retcode, id, msg, msglen);

    /* Null terminate and clean remote name. */
    ppp_slprintf(rhostname, sizeof(rhostname), "%.*v", ruserlen, ruser);

    if (retcode == UPAP_AUTHACK) {
	pcb->upap.us_serverstate = UPAPSS_OPEN;
	ppp_notice("PAP peer authentication succeeded for %q", rhostname);
	auth_peer_success(pcb, PPP_PAP, 0, ruser, ruserlen);
    } else {
	pcb->upap.us_serverstate = UPAPSS_BADAUTH;
	ppp_warn("PAP peer authentication failed for %q", rhostname);
	auth_peer_fail(pcb, PPP_PAP);
    }

    if (pcb->settings.pap_req_timeout > 0)
	UNTIMEOUT(upap_reqtimeout, pcb);
}
#endif /* PPP_SERVER */

/*
 * upap_rauthack - Receive Authenticate-Ack.
 */
static void upap_rauthack(ppp_pcb *pcb, u_char *inp, int id, int len) {
    u_char msglen;
    char *msg;
    LWIP_UNUSED_ARG(id);

    if (pcb->upap.us_clientstate != UPAPCS_AUTHREQ) /* XXX */
	return;

    /*
     * Parse message.
     */
    if (len < 1) {
	UPAPDEBUG(("pap_rauthack: ignoring missing msg-length."));
    } else {
	GETCHAR(msglen, inp);
	if (msglen > 0) {
	    len -= sizeof (u_char);
	    if (len < msglen) {
		UPAPDEBUG(("pap_rauthack: rcvd short packet."));
		return;
	    }
	    msg = (char *) inp;
	    PRINTMSG(msg, msglen);
	}
    }

    pcb->upap.us_clientstate = UPAPCS_OPEN;

    auth_withpeer_success(pcb, PPP_PAP, 0);
}


/*
 * upap_rauthnak - Receive Authenticate-Nak.
 */
static void upap_rauthnak(ppp_pcb *pcb, u_char *inp, int id, int len) {
    u_char msglen;
    char *msg;
    LWIP_UNUSED_ARG(id);

    if (pcb->upap.us_clientstate != UPAPCS_AUTHREQ) /* XXX */
	return;

    /*
     * Parse message.
     */
    if (len < 1) {
	UPAPDEBUG(("pap_rauthnak: ignoring missing msg-length."));
    } else {
	GETCHAR(msglen, inp);
	if (msglen > 0) {
	    len -= sizeof (u_char);
	    if (len < msglen) {
		UPAPDEBUG(("pap_rauthnak: rcvd short packet."));
		return;
	    }
	    msg = (char *) inp;
	    PRINTMSG(msg, msglen);
	}
    }

    pcb->upap.us_clientstate = UPAPCS_BADAUTH;

    ppp_error("PAP authentication failed");
    auth_withpeer_fail(pcb, PPP_PAP);
}


/*
 * upap_sauthreq - Send an Authenticate-Request.
 */
static void upap_sauthreq(ppp_pcb *pcb) {
    struct pbuf *p;
    u_char *outp;
    int outlen;

    outlen = UPAP_HEADERLEN + 2 * sizeof (u_char) +
	pcb->upap.us_userlen + pcb->upap.us_passwdlen;
    p = pbuf_alloc(PBUF_RAW, (u16_t)(PPP_HDRLEN +outlen), PPP_CTRL_PBUF_TYPE);
    if(NULL == p)
        return;
    if(p->tot_len != p->len) {
        pbuf_free(p);
        return;
    }

    outp = (u_char*)p->payload;
    MAKEHEADER(outp, PPP_PAP);

    PUTCHAR(UPAP_AUTHREQ, outp);
    PUTCHAR(++pcb->upap.us_id, outp);
    PUTSHORT(outlen, outp);
    PUTCHAR(pcb->upap.us_userlen, outp);
    MEMCPY(outp, pcb->upap.us_user, pcb->upap.us_userlen);
    INCPTR(pcb->upap.us_userlen, outp);
    PUTCHAR(pcb->upap.us_passwdlen, outp);
    MEMCPY(outp, pcb->upap.us_passwd, pcb->upap.us_passwdlen);

    ppp_write(pcb, p);

    TIMEOUT(upap_timeout, pcb, pcb->settings.pap_timeout_time);
    ++pcb->upap.us_transmits;
    pcb->upap.us_clientstate = UPAPCS_AUTHREQ;
}

#if PPP_SERVER
/*
 * upap_sresp - Send a response (ack or nak).
 */
static void upap_sresp(ppp_pcb *pcb, u_char code, u_char id, const char *msg, int msglen) {
    struct pbuf *p;
    u_char *outp;
    int outlen;

    outlen = UPAP_HEADERLEN + sizeof (u_char) + msglen;
    p = pbuf_alloc(PBUF_RAW, (u16_t)(PPP_HDRLEN +outlen), PPP_CTRL_PBUF_TYPE);
    if(NULL == p)
        return;
    if(p->tot_len != p->len) {
        pbuf_free(p);
        return;
    }

    outp = (u_char*)p->payload;
    MAKEHEADER(outp, PPP_PAP);

    PUTCHAR(code, outp);
    PUTCHAR(id, outp);
    PUTSHORT(outlen, outp);
    PUTCHAR(msglen, outp);
    MEMCPY(outp, msg, msglen);

    ppp_write(pcb, p);
}
#endif /* PPP_SERVER */

#if PRINTPKT_SUPPORT
/*
 * upap_printpkt - print the contents of a PAP packet.
 */
static const char* const upap_codenames[] = {
    "AuthReq", "AuthAck", "AuthNak"
};

static int upap_printpkt(const u_char *p, int plen, void (*printer) (void *, const char *, ...), void *arg) {
    int code, id, len;
    int mlen, ulen, wlen;
    const u_char *user, *pwd, *msg;
    const u_char *pstart;

    if (plen < UPAP_HEADERLEN)
	return 0;
    pstart = p;
    GETCHAR(code, p);
    GETCHAR(id, p);
    GETSHORT(len, p);
    if (len < UPAP_HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= (int)LWIP_ARRAYSIZE(upap_codenames))
	printer(arg, " %s", upap_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code);
    printer(arg, " id=0x%x", id);
    len -= UPAP_HEADERLEN;
    switch (code) {
    case UPAP_AUTHREQ:
	if (len < 1)
	    break;
	ulen = p[0];
	if (len < ulen + 2)
	    break;
	wlen = p[ulen + 1];
	if (len < ulen + wlen + 2)
	    break;
	user = (const u_char *) (p + 1);
	pwd = (const u_char *) (p + ulen + 2);
	p += ulen + wlen + 2;
	len -= ulen + wlen + 2;
	printer(arg, " user=");
	ppp_print_string(user, ulen, printer, arg);
	printer(arg, " password=");
/* FIXME: require ppp_pcb struct as printpkt() argument */
#if 0
	if (!pcb->settings.hide_password)
#endif
	    ppp_print_string(pwd, wlen, printer, arg);
#if 0
	else
	    printer(arg, "<hidden>");
#endif
	break;
    case UPAP_AUTHACK:
    case UPAP_AUTHNAK:
	if (len < 1)
	    break;
	mlen = p[0];
	if (len < mlen + 1)
	    break;
	msg = (const u_char *) (p + 1);
	p += mlen + 1;
	len -= mlen + 1;
	printer(arg, " ");
	ppp_print_string(msg, mlen, printer, arg);
	break;
    default:
	break;
    }

    /* print the rest of the bytes in the packet */
    for (; len > 0; --len) {
	GETCHAR(code, p);
	printer(arg, " %.2x", code);
    }

    return p - pstart;
}
#endif /* PRINTPKT_SUPPORT */

#endif /* PPP_SUPPORT && PAP_SUPPORT */
