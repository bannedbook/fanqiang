/*
 * auth.c - PPP authentication and phase control.
 *
 * Copyright (c) 1993-2002 Paul Mackerras. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 3. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Paul Mackerras
 *     <paulus@samba.org>".
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Derived from main.c, which is:
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
#if PPP_SUPPORT /* don't build if not configured for use in lwipopts.h */

#if 0 /* UNUSED */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <utmp.h>
#include <fcntl.h>
#if defined(_PATH_LASTLOG) && defined(__linux__)
#include <lastlog.h>
#endif

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAS_SHADOW
#include <shadow.h>
#ifndef PW_PPP
#define PW_PPP PW_LOGIN
#endif
#endif

#include <time.h>
#endif /* UNUSED */

#include "netif/ppp/ppp_impl.h"

#include "netif/ppp/fsm.h"
#include "netif/ppp/lcp.h"
#if CCP_SUPPORT
#include "netif/ppp/ccp.h"
#endif /* CCP_SUPPORT */
#if ECP_SUPPORT
#include "netif/ppp/ecp.h"
#endif /* ECP_SUPPORT */
#include "netif/ppp/ipcp.h"
#if PAP_SUPPORT
#include "netif/ppp/upap.h"
#endif /* PAP_SUPPORT */
#if CHAP_SUPPORT
#include "netif/ppp/chap-new.h"
#endif /* CHAP_SUPPORT */
#if EAP_SUPPORT
#include "netif/ppp/eap.h"
#endif /* EAP_SUPPORT */
#if CBCP_SUPPORT
#include "netif/ppp/cbcp.h"
#endif

#if 0 /* UNUSED */
#include "session.h"
#endif /* UNUSED */

#if 0 /* UNUSED */
/* Bits in scan_authfile return value */
#define NONWILD_SERVER	1
#define NONWILD_CLIENT	2

#define ISWILD(word)	(word[0] == '*' && word[1] == 0)
#endif /* UNUSED */

#if 0 /* UNUSED */
/* List of addresses which the peer may use. */
static struct permitted_ip *addresses[NUM_PPP];

/* Wordlist giving addresses which the peer may use
   without authenticating itself. */
static struct wordlist *noauth_addrs;

/* Remote telephone number, if available */
char remote_number[MAXNAMELEN];

/* Wordlist giving remote telephone numbers which may connect. */
static struct wordlist *permitted_numbers;

/* Extra options to apply, from the secrets file entry for the peer. */
static struct wordlist *extra_options;
#endif /* UNUSED */

#if 0 /* UNUSED */
/* Set if we require authentication only because we have a default route. */
static bool default_auth;

/* Hook to enable a plugin to control the idle time limit */
int (*idle_time_hook) (struct ppp_idle *) = NULL;

/* Hook for a plugin to say whether we can possibly authenticate any peer */
int (*pap_check_hook) (void) = NULL;

/* Hook for a plugin to check the PAP user and password */
int (*pap_auth_hook) (char *user, char *passwd, char **msgp,
			  struct wordlist **paddrs,
			  struct wordlist **popts) = NULL;

/* Hook for a plugin to know about the PAP user logout */
void (*pap_logout_hook) (void) = NULL;

/* Hook for a plugin to get the PAP password for authenticating us */
int (*pap_passwd_hook) (char *user, char *passwd) = NULL;

/* Hook for a plugin to say if we can possibly authenticate a peer using CHAP */
int (*chap_check_hook) (void) = NULL;

/* Hook for a plugin to get the CHAP password for authenticating us */
int (*chap_passwd_hook) (char *user, char *passwd) = NULL;

/* Hook for a plugin to say whether it is OK if the peer
   refuses to authenticate. */
int (*null_auth_hook) (struct wordlist **paddrs,
			   struct wordlist **popts) = NULL;

int (*allowed_address_hook) (u32_t addr) = NULL;
#endif /* UNUSED */

#ifdef HAVE_MULTILINK
/* Hook for plugin to hear when an interface joins a multilink bundle */
void (*multilink_join_hook) (void) = NULL;
#endif

#if PPP_NOTIFY
/* A notifier for when the peer has authenticated itself,
   and we are proceeding to the network phase. */
struct notifier *auth_up_notifier = NULL;

/* A notifier for when the link goes down. */
struct notifier *link_down_notifier = NULL;
#endif /* PPP_NOTIFY */

/*
 * Option variables.
 */
#if 0 /* MOVED TO ppp_settings */
bool uselogin = 0;		/* Use /etc/passwd for checking PAP */
bool session_mgmt = 0;		/* Do session management (login records) */
bool cryptpap = 0;		/* Passwords in pap-secrets are encrypted */
bool refuse_pap = 0;		/* Don't wanna auth. ourselves with PAP */
bool refuse_chap = 0;		/* Don't wanna auth. ourselves with CHAP */
bool refuse_eap = 0;		/* Don't wanna auth. ourselves with EAP */
#if MSCHAP_SUPPORT
bool refuse_mschap = 0;		/* Don't wanna auth. ourselves with MS-CHAP */
bool refuse_mschap_v2 = 0;	/* Don't wanna auth. ourselves with MS-CHAPv2 */
#else /* MSCHAP_SUPPORT */
bool refuse_mschap = 1;		/* Don't wanna auth. ourselves with MS-CHAP */
bool refuse_mschap_v2 = 1;	/* Don't wanna auth. ourselves with MS-CHAPv2 */
#endif /* MSCHAP_SUPPORT */
bool usehostname = 0;		/* Use hostname for our_name */
bool auth_required = 0;		/* Always require authentication from peer */
bool allow_any_ip = 0;		/* Allow peer to use any IP address */
bool explicit_remote = 0;	/* User specified explicit remote name */
bool explicit_user = 0;		/* Set if "user" option supplied */
bool explicit_passwd = 0;	/* Set if "password" option supplied */
char remote_name[MAXNAMELEN];	/* Peer's name for authentication */
static char *uafname;		/* name of most recent +ua file */

extern char *crypt (const char *, const char *);
#endif /* UNUSED */
/* Prototypes for procedures local to this file. */

static void network_phase(ppp_pcb *pcb);
#if PPP_IDLETIMELIMIT
static void check_idle(void *arg);
#endif /* PPP_IDLETIMELIMIT */
#if PPP_MAXCONNECT
static void connect_time_expired(void *arg);
#endif /* PPP_MAXCONNECT */
#if 0 /* UNUSED */
static int  null_login (int);
/* static int  get_pap_passwd (char *); */
static int  have_pap_secret (int *);
static int  have_chap_secret (char *, char *, int, int *);
static int  have_srp_secret (char *client, char *server, int need_ip,
    int *lacks_ipp);
static int  ip_addr_check (u32_t, struct permitted_ip *);
static int  scan_authfile (FILE *, char *, char *, char *,
			       struct wordlist **, struct wordlist **,
			       char *, int);
static void free_wordlist (struct wordlist *);
static void set_allowed_addrs (int, struct wordlist *, struct wordlist *);
static int  some_ip_ok (struct wordlist *);
static int  setupapfile (char **);
static int  privgroup (char **);
static int  set_noauth_addr (char **);
static int  set_permitted_number (char **);
static void check_access (FILE *, char *);
static int  wordlist_count (struct wordlist *);
#endif /* UNUSED */

#ifdef MAXOCTETS
static void check_maxoctets (void *);
#endif

#if PPP_OPTIONS
/*
 * Authentication-related options.
 */
option_t auth_options[] = {
    { "auth", o_bool, &auth_required,
      "Require authentication from peer", OPT_PRIO | 1 },
    { "noauth", o_bool, &auth_required,
      "Don't require peer to authenticate", OPT_PRIOSUB | OPT_PRIV,
      &allow_any_ip },
    { "require-pap", o_bool, &lcp_wantoptions[0].neg_upap,
      "Require PAP authentication from peer",
      OPT_PRIOSUB | 1, &auth_required },
    { "+pap", o_bool, &lcp_wantoptions[0].neg_upap,
      "Require PAP authentication from peer",
      OPT_ALIAS | OPT_PRIOSUB | 1, &auth_required },
    { "require-chap", o_bool, &auth_required,
      "Require CHAP authentication from peer",
      OPT_PRIOSUB | OPT_A2OR | MDTYPE_MD5,
      &lcp_wantoptions[0].chap_mdtype },
    { "+chap", o_bool, &auth_required,
      "Require CHAP authentication from peer",
      OPT_ALIAS | OPT_PRIOSUB | OPT_A2OR | MDTYPE_MD5,
      &lcp_wantoptions[0].chap_mdtype },
#if MSCHAP_SUPPORT
    { "require-mschap", o_bool, &auth_required,
      "Require MS-CHAP authentication from peer",
      OPT_PRIOSUB | OPT_A2OR | MDTYPE_MICROSOFT,
      &lcp_wantoptions[0].chap_mdtype },
    { "+mschap", o_bool, &auth_required,
      "Require MS-CHAP authentication from peer",
      OPT_ALIAS | OPT_PRIOSUB | OPT_A2OR | MDTYPE_MICROSOFT,
      &lcp_wantoptions[0].chap_mdtype },
    { "require-mschap-v2", o_bool, &auth_required,
      "Require MS-CHAPv2 authentication from peer",
      OPT_PRIOSUB | OPT_A2OR | MDTYPE_MICROSOFT_V2,
      &lcp_wantoptions[0].chap_mdtype },
    { "+mschap-v2", o_bool, &auth_required,
      "Require MS-CHAPv2 authentication from peer",
      OPT_ALIAS | OPT_PRIOSUB | OPT_A2OR | MDTYPE_MICROSOFT_V2,
      &lcp_wantoptions[0].chap_mdtype },
#endif /* MSCHAP_SUPPORT */
#if 0
    { "refuse-pap", o_bool, &refuse_pap,
      "Don't agree to auth to peer with PAP", 1 },
    { "-pap", o_bool, &refuse_pap,
      "Don't allow PAP authentication with peer", OPT_ALIAS | 1 },
    { "refuse-chap", o_bool, &refuse_chap,
      "Don't agree to auth to peer with CHAP",
      OPT_A2CLRB | MDTYPE_MD5,
      &lcp_allowoptions[0].chap_mdtype },
    { "-chap", o_bool, &refuse_chap,
      "Don't allow CHAP authentication with peer",
      OPT_ALIAS | OPT_A2CLRB | MDTYPE_MD5,
      &lcp_allowoptions[0].chap_mdtype },
#endif
#if MSCHAP_SUPPORT
#if 0
    { "refuse-mschap", o_bool, &refuse_mschap,
      "Don't agree to auth to peer with MS-CHAP",
      OPT_A2CLRB | MDTYPE_MICROSOFT,
      &lcp_allowoptions[0].chap_mdtype },
    { "-mschap", o_bool, &refuse_mschap,
      "Don't allow MS-CHAP authentication with peer",
      OPT_ALIAS | OPT_A2CLRB | MDTYPE_MICROSOFT,
      &lcp_allowoptions[0].chap_mdtype },
    { "refuse-mschap-v2", o_bool, &refuse_mschap_v2,
      "Don't agree to auth to peer with MS-CHAPv2",
      OPT_A2CLRB | MDTYPE_MICROSOFT_V2,
      &lcp_allowoptions[0].chap_mdtype },
    { "-mschap-v2", o_bool, &refuse_mschap_v2,
      "Don't allow MS-CHAPv2 authentication with peer",
      OPT_ALIAS | OPT_A2CLRB | MDTYPE_MICROSOFT_V2,
      &lcp_allowoptions[0].chap_mdtype },
#endif
#endif /* MSCHAP_SUPPORT*/
#if EAP_SUPPORT
    { "require-eap", o_bool, &lcp_wantoptions[0].neg_eap,
      "Require EAP authentication from peer", OPT_PRIOSUB | 1,
      &auth_required },
#if 0
    { "refuse-eap", o_bool, &refuse_eap,
      "Don't agree to authenticate to peer with EAP", 1 },
#endif
#endif /* EAP_SUPPORT */
    { "name", o_string, our_name,
      "Set local name for authentication",
      OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, MAXNAMELEN },

    { "+ua", o_special, (void *)setupapfile,
      "Get PAP user and password from file",
      OPT_PRIO | OPT_A2STRVAL, &uafname },

#if 0
    { "user", o_string, user,
      "Set name for auth with peer", OPT_PRIO | OPT_STATIC,
      &explicit_user, MAXNAMELEN },

    { "password", o_string, passwd,
      "Password for authenticating us to the peer",
      OPT_PRIO | OPT_STATIC | OPT_HIDE,
      &explicit_passwd, MAXSECRETLEN },
#endif

    { "usehostname", o_bool, &usehostname,
      "Must use hostname for authentication", 1 },

    { "remotename", o_string, remote_name,
      "Set remote name for authentication", OPT_PRIO | OPT_STATIC,
      &explicit_remote, MAXNAMELEN },

    { "login", o_bool, &uselogin,
      "Use system password database for PAP", OPT_A2COPY | 1 ,
      &session_mgmt },
    { "enable-session", o_bool, &session_mgmt,
      "Enable session accounting for remote peers", OPT_PRIV | 1 },

    { "papcrypt", o_bool, &cryptpap,
      "PAP passwords are encrypted", 1 },

    { "privgroup", o_special, (void *)privgroup,
      "Allow group members to use privileged options", OPT_PRIV | OPT_A2LIST },

    { "allow-ip", o_special, (void *)set_noauth_addr,
      "Set IP address(es) which can be used without authentication",
      OPT_PRIV | OPT_A2LIST },

    { "remotenumber", o_string, remote_number,
      "Set remote telephone number for authentication", OPT_PRIO | OPT_STATIC,
      NULL, MAXNAMELEN },

    { "allow-number", o_special, (void *)set_permitted_number,
      "Set telephone number(s) which are allowed to connect",
      OPT_PRIV | OPT_A2LIST },

    { NULL }
};
#endif /* PPP_OPTIONS */

#if 0 /* UNUSED */
/*
 * setupapfile - specifies UPAP info for authenticating with peer.
 */
static int
setupapfile(argv)
    char **argv;
{
    FILE *ufile;
    int l;
    uid_t euid;
    char u[MAXNAMELEN], p[MAXSECRETLEN];
    char *fname;

    lcp_allowoptions[0].neg_upap = 1;

    /* open user info file */
    fname = strdup(*argv);
    if (fname == NULL)
	novm("+ua file name");
    euid = geteuid();
    if (seteuid(getuid()) == -1) {
	option_error("unable to reset uid before opening %s: %m", fname);
	return 0;
    }
    ufile = fopen(fname, "r");
    if (seteuid(euid) == -1)
	fatal("unable to regain privileges: %m");
    if (ufile == NULL) {
	option_error("unable to open user login data file %s", fname);
	return 0;
    }
    check_access(ufile, fname);
    uafname = fname;

    /* get username */
    if (fgets(u, MAXNAMELEN - 1, ufile) == NULL
	|| fgets(p, MAXSECRETLEN - 1, ufile) == NULL) {
	fclose(ufile);
	option_error("unable to read user login data file %s", fname);
	return 0;
    }
    fclose(ufile);

    /* get rid of newlines */
    l = strlen(u);
    if (l > 0 && u[l-1] == '\n')
	u[l-1] = 0;
    l = strlen(p);
    if (l > 0 && p[l-1] == '\n')
	p[l-1] = 0;

    if (override_value("user", option_priority, fname)) {
	strlcpy(ppp_settings.user, u, sizeof(ppp_settings.user));
	explicit_user = 1;
    }
    if (override_value("passwd", option_priority, fname)) {
	strlcpy(ppp_settings.passwd, p, sizeof(ppp_settings.passwd));
	explicit_passwd = 1;
    }

    return (1);
}

/*
 * privgroup - allow members of the group to have privileged access.
 */
static int
privgroup(argv)
    char **argv;
{
    struct group *g;
    int i;

    g = getgrnam(*argv);
    if (g == 0) {
	option_error("group %s is unknown", *argv);
	return 0;
    }
    for (i = 0; i < ngroups; ++i) {
	if (groups[i] == g->gr_gid) {
	    privileged = 1;
	    break;
	}
    }
    return 1;
}


/*
 * set_noauth_addr - set address(es) that can be used without authentication.
 * Equivalent to specifying an entry like `"" * "" addr' in pap-secrets.
 */
static int
set_noauth_addr(argv)
    char **argv;
{
    char *addr = *argv;
    int l = strlen(addr) + 1;
    struct wordlist *wp;

    wp = (struct wordlist *) malloc(sizeof(struct wordlist) + l);
    if (wp == NULL)
	novm("allow-ip argument");
    wp->word = (char *) (wp + 1);
    wp->next = noauth_addrs;
    MEMCPY(wp->word, addr, l);
    noauth_addrs = wp;
    return 1;
}


/*
 * set_permitted_number - set remote telephone number(s) that may connect.
 */
static int
set_permitted_number(argv)
    char **argv;
{
    char *number = *argv;
    int l = strlen(number) + 1;
    struct wordlist *wp;

    wp = (struct wordlist *) malloc(sizeof(struct wordlist) + l);
    if (wp == NULL)
	novm("allow-number argument");
    wp->word = (char *) (wp + 1);
    wp->next = permitted_numbers;
    MEMCPY(wp->word, number, l);
    permitted_numbers = wp;
    return 1;
}
#endif

/*
 * An Open on LCP has requested a change from Dead to Establish phase.
 */
void link_required(ppp_pcb *pcb) {
    LWIP_UNUSED_ARG(pcb);
}

#if 0
/*
 * Bring the link up to the point of being able to do ppp.
 */
void start_link(unit)
    int unit;
{
    ppp_pcb *pcb = &ppp_pcb_list[unit];
    char *msg;

    status = EXIT_NEGOTIATION_FAILED;
    new_phase(pcb, PPP_PHASE_SERIALCONN);

    hungup = 0;
    devfd = the_channel->connect();
    msg = "Connect script failed";
    if (devfd < 0)
	goto fail;

    /* set up the serial device as a ppp interface */
    /*
     * N.B. we used to do tdb_writelock/tdb_writeunlock around this
     * (from establish_ppp to set_ifunit).  However, we won't be
     * doing the set_ifunit in multilink mode, which is the only time
     * we need the atomicity that the tdb_writelock/tdb_writeunlock
     * gives us.  Thus we don't need the tdb_writelock/tdb_writeunlock.
     */
    fd_ppp = the_channel->establish_ppp(devfd);
    msg = "ppp establishment failed";
    if (fd_ppp < 0) {
	status = EXIT_FATAL_ERROR;
	goto disconnect;
    }

    if (!demand && ifunit >= 0)
	set_ifunit(1);

    /*
     * Start opening the connection and wait for
     * incoming events (reply, timeout, etc.).
     */
    if (ifunit >= 0)
	ppp_notice("Connect: %s <--> %s", ifname, ppp_devnam);
    else
	ppp_notice("Starting negotiation on %s", ppp_devnam);
    add_fd(fd_ppp);

    new_phase(pcb, PPP_PHASE_ESTABLISH);

    lcp_lowerup(pcb);
    return;

 disconnect:
    new_phase(pcb, PPP_PHASE_DISCONNECT);
    if (the_channel->disconnect)
	the_channel->disconnect();

 fail:
    new_phase(pcb, PPP_PHASE_DEAD);
    if (the_channel->cleanup)
	(*the_channel->cleanup)();
}
#endif

/*
 * LCP has terminated the link; go to the Dead phase and take the
 * physical layer down.
 */
void link_terminated(ppp_pcb *pcb) {
    if (pcb->phase == PPP_PHASE_DEAD
#ifdef HAVE_MULTILINK
    || pcb->phase == PPP_PHASE_MASTER
#endif /* HAVE_MULTILINK */
    )
	return;
    new_phase(pcb, PPP_PHASE_DISCONNECT);

#if 0 /* UNUSED */
    if (pap_logout_hook) {
	pap_logout_hook();
    }
    session_end(devnam);
#endif /* UNUSED */

    if (!doing_multilink) {
	ppp_notice("Connection terminated.");
#if PPP_STATS_SUPPORT
	print_link_stats();
#endif /* PPP_STATS_SUPPORT */
    } else
	ppp_notice("Link terminated.");

    lcp_lowerdown(pcb);

    ppp_link_terminated(pcb);
#if 0
    /*
     * Delete pid files before disestablishing ppp.  Otherwise it
     * can happen that another pppd gets the same unit and then
     * we delete its pid file.
     */
    if (!doing_multilink && !demand)
	remove_pidfiles();

    /*
     * If we may want to bring the link up again, transfer
     * the ppp unit back to the loopback.  Set the
     * real serial device back to its normal mode of operation.
     */
    if (fd_ppp >= 0) {
	remove_fd(fd_ppp);
	clean_check();
	the_channel->disestablish_ppp(devfd);
	if (doing_multilink)
	    mp_exit_bundle();
	fd_ppp = -1;
    }
    if (!hungup)
	lcp_lowerdown(pcb);
    if (!doing_multilink && !demand)
	script_unsetenv("IFNAME");

    /*
     * Run disconnector script, if requested.
     * XXX we may not be able to do this if the line has hung up!
     */
    if (devfd >= 0 && the_channel->disconnect) {
	the_channel->disconnect();
	devfd = -1;
    }
    if (the_channel->cleanup)
	(*the_channel->cleanup)();

    if (doing_multilink && multilink_master) {
	if (!bundle_terminating)
	    new_phase(pcb, PPP_PHASE_MASTER);
	else
	    mp_bundle_terminated();
    } else
	new_phase(pcb, PPP_PHASE_DEAD);
#endif
}

/*
 * LCP has gone down; it will either die or try to re-establish.
 */
void link_down(ppp_pcb *pcb) {
#if PPP_NOTIFY
    notify(link_down_notifier, 0);
#endif /* PPP_NOTIFY */

    if (!doing_multilink) {
	upper_layers_down(pcb);
	if (pcb->phase != PPP_PHASE_DEAD
#ifdef HAVE_MULTILINK
	&& pcb->phase != PPP_PHASE_MASTER
#endif /* HAVE_MULTILINK */
	)
	    new_phase(pcb, PPP_PHASE_ESTABLISH);
    }
    /* XXX if doing_multilink, should do something to stop
       network-layer traffic on the link */
}

void upper_layers_down(ppp_pcb *pcb) {
    int i;
    const struct protent *protp;

    for (i = 0; (protp = protocols[i]) != NULL; ++i) {
        if (protp->protocol != PPP_LCP && protp->lowerdown != NULL)
	    (*protp->lowerdown)(pcb);
        if (protp->protocol < 0xC000 && protp->close != NULL)
	    (*protp->close)(pcb, "LCP down");
    }
    pcb->num_np_open = 0;
    pcb->num_np_up = 0;
}

/*
 * The link is established.
 * Proceed to the Dead, Authenticate or Network phase as appropriate.
 */
void link_established(ppp_pcb *pcb) {
#if PPP_AUTH_SUPPORT
    int auth;
#if PPP_SERVER
#if PAP_SUPPORT
    lcp_options *wo = &pcb->lcp_wantoptions;
#endif /* PAP_SUPPORT */
    lcp_options *go = &pcb->lcp_gotoptions;
#endif /* PPP_SERVER */
    lcp_options *ho = &pcb->lcp_hisoptions;
#endif /* PPP_AUTH_SUPPORT */
    int i;
    const struct protent *protp;

    /*
     * Tell higher-level protocols that LCP is up.
     */
    if (!doing_multilink) {
	for (i = 0; (protp = protocols[i]) != NULL; ++i)
	    if (protp->protocol != PPP_LCP
		&& protp->lowerup != NULL)
		(*protp->lowerup)(pcb);
    }

#if PPP_AUTH_SUPPORT
#if PPP_SERVER
#if PPP_ALLOWED_ADDRS
    if (!auth_required && noauth_addrs != NULL)
	set_allowed_addrs(unit, NULL, NULL);
#endif /* PPP_ALLOWED_ADDRS */

    if (pcb->settings.auth_required && !(0
#if PAP_SUPPORT
	|| go->neg_upap
#endif /* PAP_SUPPORT */
#if CHAP_SUPPORT
	|| go->neg_chap
#endif /* CHAP_SUPPORT */
#if EAP_SUPPORT
	|| go->neg_eap
#endif /* EAP_SUPPORT */
	)) {

#if PPP_ALLOWED_ADDRS
	/*
	 * We wanted the peer to authenticate itself, and it refused:
	 * if we have some address(es) it can use without auth, fine,
	 * otherwise treat it as though it authenticated with PAP using
	 * a username of "" and a password of "".  If that's not OK,
	 * boot it out.
	 */
	if (noauth_addrs != NULL) {
	    set_allowed_addrs(unit, NULL, NULL);
	} else
#endif /* PPP_ALLOWED_ADDRS */
	if (!pcb->settings.null_login
#if PAP_SUPPORT
	    || !wo->neg_upap
#endif /* PAP_SUPPORT */
	    ) {
	    ppp_warn("peer refused to authenticate: terminating link");
#if 0 /* UNUSED */
	    status = EXIT_PEER_AUTH_FAILED;
#endif /* UNUSED */
	    pcb->err_code = PPPERR_AUTHFAIL;
	    lcp_close(pcb, "peer refused to authenticate");
	    return;
	}
    }
#endif /* PPP_SERVER */

    new_phase(pcb, PPP_PHASE_AUTHENTICATE);
    auth = 0;
#if PPP_SERVER
#if EAP_SUPPORT
    if (go->neg_eap) {
	eap_authpeer(pcb, PPP_OUR_NAME);
	auth |= EAP_PEER;
    } else
#endif /* EAP_SUPPORT */
#if CHAP_SUPPORT
    if (go->neg_chap) {
	chap_auth_peer(pcb, PPP_OUR_NAME, CHAP_DIGEST(go->chap_mdtype));
	auth |= CHAP_PEER;
    } else
#endif /* CHAP_SUPPORT */
#if PAP_SUPPORT
    if (go->neg_upap) {
	upap_authpeer(pcb);
	auth |= PAP_PEER;
    } else
#endif /* PAP_SUPPORT */
    {}
#endif /* PPP_SERVER */

#if EAP_SUPPORT
    if (ho->neg_eap) {
	eap_authwithpeer(pcb, pcb->settings.user);
	auth |= EAP_WITHPEER;
    } else
#endif /* EAP_SUPPORT */
#if CHAP_SUPPORT
    if (ho->neg_chap) {
	chap_auth_with_peer(pcb, pcb->settings.user, CHAP_DIGEST(ho->chap_mdtype));
	auth |= CHAP_WITHPEER;
    } else
#endif /* CHAP_SUPPORT */
#if PAP_SUPPORT
    if (ho->neg_upap) {
	upap_authwithpeer(pcb, pcb->settings.user, pcb->settings.passwd);
	auth |= PAP_WITHPEER;
    } else
#endif /* PAP_SUPPORT */
    {}

    pcb->auth_pending = auth;
    pcb->auth_done = 0;

    if (!auth)
#endif /* PPP_AUTH_SUPPORT */
	network_phase(pcb);
}

/*
 * Proceed to the network phase.
 */
static void network_phase(ppp_pcb *pcb) {
#if CBCP_SUPPORT
    ppp_pcb *pcb = &ppp_pcb_list[unit];
#endif
#if 0 /* UNUSED */
    lcp_options *go = &lcp_gotoptions[unit];
#endif /* UNUSED */

#if 0 /* UNUSED */
    /* Log calling number. */
    if (*remote_number)
	ppp_notice("peer from calling number %q authorized", remote_number);
#endif /* UNUSED */

#if PPP_NOTIFY
    /*
     * If the peer had to authenticate, notify it now.
     */
    if (0
#if CHAP_SUPPORT
	|| go->neg_chap
#endif /* CHAP_SUPPORT */
#if PAP_SUPPORT
	|| go->neg_upap
#endif /* PAP_SUPPORT */
#if EAP_SUPPORT
	|| go->neg_eap
#endif /* EAP_SUPPORT */
	) {
	notify(auth_up_notifier, 0);
    }
#endif /* PPP_NOTIFY */

#if CBCP_SUPPORT
    /*
     * If we negotiated callback, do it now.
     */
    if (go->neg_cbcp) {
	new_phase(pcb, PPP_PHASE_CALLBACK);
	(*cbcp_protent.open)(pcb);
	return;
    }
#endif

#if PPP_OPTIONS
    /*
     * Process extra options from the secrets file
     */
    if (extra_options) {
	options_from_list(extra_options, 1);
	free_wordlist(extra_options);
	extra_options = 0;
    }
#endif /* PPP_OPTIONS */
    start_networks(pcb);
}

void start_networks(ppp_pcb *pcb) {
#if CCP_SUPPORT || ECP_SUPPORT
    int i;
    const struct protent *protp;
#endif /* CCP_SUPPORT || ECP_SUPPORT */

    new_phase(pcb, PPP_PHASE_NETWORK);

#ifdef HAVE_MULTILINK
    if (multilink) {
	if (mp_join_bundle()) {
	    if (multilink_join_hook)
		(*multilink_join_hook)();
	    if (updetach && !nodetach)
		detach();
	    return;
	}
    }
#endif /* HAVE_MULTILINK */

#ifdef PPP_FILTER
    if (!demand)
	set_filters(&pass_filter, &active_filter);
#endif
#if CCP_SUPPORT || ECP_SUPPORT
    /* Start CCP and ECP */
    for (i = 0; (protp = protocols[i]) != NULL; ++i)
	if (
	    (0
#if ECP_SUPPORT
	    || protp->protocol == PPP_ECP
#endif /* ECP_SUPPORT */
#if CCP_SUPPORT
	    || protp->protocol == PPP_CCP
#endif /* CCP_SUPPORT */
	    )
	    && protp->open != NULL)
	    (*protp->open)(pcb);
#endif /* CCP_SUPPORT || ECP_SUPPORT */

    /*
     * Bring up other network protocols iff encryption is not required.
     */
    if (1
#if ECP_SUPPORT
        && !ecp_gotoptions[unit].required
#endif /* ECP_SUPPORT */
#if MPPE_SUPPORT
        && !pcb->ccp_gotoptions.mppe
#endif /* MPPE_SUPPORT */
        )
	continue_networks(pcb);
}

void continue_networks(ppp_pcb *pcb) {
    int i;
    const struct protent *protp;

    /*
     * Start the "real" network protocols.
     */
    for (i = 0; (protp = protocols[i]) != NULL; ++i)
	if (protp->protocol < 0xC000
#if CCP_SUPPORT
	    && protp->protocol != PPP_CCP
#endif /* CCP_SUPPORT */
#if ECP_SUPPORT
	    && protp->protocol != PPP_ECP
#endif /* ECP_SUPPORT */
	    && protp->open != NULL) {
	    (*protp->open)(pcb);
	    ++pcb->num_np_open;
	}

    if (pcb->num_np_open == 0)
	/* nothing to do */
	lcp_close(pcb, "No network protocols running");
}

#if PPP_AUTH_SUPPORT
#if PPP_SERVER
/*
 * auth_check_passwd - Check the user name and passwd against configuration.
 *
 * returns:
 *      0: Authentication failed.
 *      1: Authentication succeeded.
 * In either case, msg points to an appropriate message and msglen to the message len.
 */
int auth_check_passwd(ppp_pcb *pcb, char *auser, int userlen, char *apasswd, int passwdlen, const char **msg, int *msglen) {
  int secretuserlen;
  int secretpasswdlen;

  if (pcb->settings.user && pcb->settings.passwd) {
    secretuserlen = (int)strlen(pcb->settings.user);
    secretpasswdlen = (int)strlen(pcb->settings.passwd);
    if (secretuserlen == userlen
        && secretpasswdlen == passwdlen
        && !memcmp(auser, pcb->settings.user, userlen)
        && !memcmp(apasswd, pcb->settings.passwd, passwdlen) ) {
      *msg = "Login ok";
      *msglen = sizeof("Login ok")-1;
      return 1;
    }
  }

  *msg = "Login incorrect";
  *msglen = sizeof("Login incorrect")-1;
  return 0;
}

/*
 * The peer has failed to authenticate himself using `protocol'.
 */
void auth_peer_fail(ppp_pcb *pcb, int protocol) {
    LWIP_UNUSED_ARG(protocol);
    /*
     * Authentication failure: take the link down
     */
#if 0 /* UNUSED */
    status = EXIT_PEER_AUTH_FAILED;
#endif /* UNUSED */
    pcb->err_code = PPPERR_AUTHFAIL;
    lcp_close(pcb, "Authentication failed");
}

/*
 * The peer has been successfully authenticated using `protocol'.
 */
void auth_peer_success(ppp_pcb *pcb, int protocol, int prot_flavor, const char *name, int namelen) {
    int bit;
#ifndef HAVE_MULTILINK
    LWIP_UNUSED_ARG(name);
    LWIP_UNUSED_ARG(namelen);
#endif /* HAVE_MULTILINK */

    switch (protocol) {
#if CHAP_SUPPORT
    case PPP_CHAP:
	bit = CHAP_PEER;
	switch (prot_flavor) {
	case CHAP_MD5:
	    bit |= CHAP_MD5_PEER;
	    break;
#if MSCHAP_SUPPORT
	case CHAP_MICROSOFT:
	    bit |= CHAP_MS_PEER;
	    break;
	case CHAP_MICROSOFT_V2:
	    bit |= CHAP_MS2_PEER;
	    break;
#endif /* MSCHAP_SUPPORT */
	default:
	    break;
	}
	break;
#endif /* CHAP_SUPPORT */
#if PAP_SUPPORT
    case PPP_PAP:
	bit = PAP_PEER;
	break;
#endif /* PAP_SUPPORT */
#if EAP_SUPPORT
    case PPP_EAP:
	bit = EAP_PEER;
	break;
#endif /* EAP_SUPPORT */
    default:
	ppp_warn("auth_peer_success: unknown protocol %x", protocol);
	return;
    }

#ifdef HAVE_MULTILINK
    /*
     * Save the authenticated name of the peer for later.
     */
    if (namelen > (int)sizeof(pcb->peer_authname) - 1)
	namelen = (int)sizeof(pcb->peer_authname) - 1;
    MEMCPY(pcb->peer_authname, name, namelen);
    pcb->peer_authname[namelen] = 0;
#endif /* HAVE_MULTILINK */
#if 0 /* UNUSED */
    script_setenv("PEERNAME", , 0);
#endif /* UNUSED */

    /* Save the authentication method for later. */
    pcb->auth_done |= bit;

    /*
     * If there is no more authentication still to be done,
     * proceed to the network (or callback) phase.
     */
    if ((pcb->auth_pending &= ~bit) == 0)
        network_phase(pcb);
}
#endif /* PPP_SERVER */

/*
 * We have failed to authenticate ourselves to the peer using `protocol'.
 */
void auth_withpeer_fail(ppp_pcb *pcb, int protocol) {
    LWIP_UNUSED_ARG(protocol);
    /*
     * We've failed to authenticate ourselves to our peer.
     *
     * Some servers keep sending CHAP challenges, but there
     * is no point in persisting without any way to get updated
     * authentication secrets.
     *
     * He'll probably take the link down, and there's not much
     * we can do except wait for that.
     */
    pcb->err_code = PPPERR_AUTHFAIL;
    lcp_close(pcb, "Failed to authenticate ourselves to peer");
}

/*
 * We have successfully authenticated ourselves with the peer using `protocol'.
 */
void auth_withpeer_success(ppp_pcb *pcb, int protocol, int prot_flavor) {
    int bit;
    const char *prot = "";

    switch (protocol) {
#if CHAP_SUPPORT
    case PPP_CHAP:
	bit = CHAP_WITHPEER;
	prot = "CHAP";
	switch (prot_flavor) {
	case CHAP_MD5:
	    bit |= CHAP_MD5_WITHPEER;
	    break;
#if MSCHAP_SUPPORT
	case CHAP_MICROSOFT:
	    bit |= CHAP_MS_WITHPEER;
	    break;
	case CHAP_MICROSOFT_V2:
	    bit |= CHAP_MS2_WITHPEER;
	    break;
#endif /* MSCHAP_SUPPORT */
	default:
	    break;
	}
	break;
#endif /* CHAP_SUPPORT */
#if PAP_SUPPORT
    case PPP_PAP:
	bit = PAP_WITHPEER;
	prot = "PAP";
	break;
#endif /* PAP_SUPPORT */
#if EAP_SUPPORT
    case PPP_EAP:
	bit = EAP_WITHPEER;
	prot = "EAP";
	break;
#endif /* EAP_SUPPORT */
    default:
	ppp_warn("auth_withpeer_success: unknown protocol %x", protocol);
	bit = 0;
	/* no break */
    }

    ppp_notice("%s authentication succeeded", prot);

    /* Save the authentication method for later. */
    pcb->auth_done |= bit;

    /*
     * If there is no more authentication still being done,
     * proceed to the network (or callback) phase.
     */
    if ((pcb->auth_pending &= ~bit) == 0)
	network_phase(pcb);
}
#endif /* PPP_AUTH_SUPPORT */


/*
 * np_up - a network protocol has come up.
 */
void np_up(ppp_pcb *pcb, int proto) {
#if PPP_IDLETIMELIMIT
    int tlim;
#endif /* PPP_IDLETIMELIMIT */
    LWIP_UNUSED_ARG(proto);

    if (pcb->num_np_up == 0) {
	/*
	 * At this point we consider that the link has come up successfully.
	 */
	new_phase(pcb, PPP_PHASE_RUNNING);

#if PPP_IDLETIMELIMIT
#if 0 /* UNUSED */
	if (idle_time_hook != 0)
	    tlim = (*idle_time_hook)(NULL);
	else
#endif /* UNUSED */
	    tlim = pcb->settings.idle_time_limit;
	if (tlim > 0)
	    TIMEOUT(check_idle, (void*)pcb, tlim);
#endif /* PPP_IDLETIMELIMIT */

#if PPP_MAXCONNECT
	/*
	 * Set a timeout to close the connection once the maximum
	 * connect time has expired.
	 */
	if (pcb->settings.maxconnect > 0)
	    TIMEOUT(connect_time_expired, (void*)pcb, pcb->settings.maxconnect);
#endif /* PPP_MAXCONNECT */

#ifdef MAXOCTETS
	if (maxoctets > 0)
	    TIMEOUT(check_maxoctets, NULL, maxoctets_timeout);
#endif

#if 0 /* Unused */
	/*
	 * Detach now, if the updetach option was given.
	 */
	if (updetach && !nodetach)
	    detach();
#endif /* Unused */
    }
    ++pcb->num_np_up;
}

/*
 * np_down - a network protocol has gone down.
 */
void np_down(ppp_pcb *pcb, int proto) {
    LWIP_UNUSED_ARG(proto);
    if (--pcb->num_np_up == 0) {
#if PPP_IDLETIMELIMIT
	UNTIMEOUT(check_idle, (void*)pcb);
#endif /* PPP_IDLETIMELIMIT */
#if PPP_MAXCONNECT
	UNTIMEOUT(connect_time_expired, NULL);
#endif /* PPP_MAXCONNECT */
#ifdef MAXOCTETS
	UNTIMEOUT(check_maxoctets, NULL);
#endif
	new_phase(pcb, PPP_PHASE_NETWORK);
    }
}

/*
 * np_finished - a network protocol has finished using the link.
 */
void np_finished(ppp_pcb *pcb, int proto) {
    LWIP_UNUSED_ARG(proto);
    if (--pcb->num_np_open <= 0) {
	/* no further use for the link: shut up shop. */
	lcp_close(pcb, "No network protocols running");
    }
}

#ifdef MAXOCTETS
static void
check_maxoctets(arg)
    void *arg;
{
#if PPP_STATS_SUPPORT
    unsigned int used;

    update_link_stats(ifunit);
    link_stats_valid=0;

    switch(maxoctets_dir) {
	case PPP_OCTETS_DIRECTION_IN:
	    used = link_stats.bytes_in;
	    break;
	case PPP_OCTETS_DIRECTION_OUT:
	    used = link_stats.bytes_out;
	    break;
	case PPP_OCTETS_DIRECTION_MAXOVERAL:
	case PPP_OCTETS_DIRECTION_MAXSESSION:
	    used = (link_stats.bytes_in > link_stats.bytes_out) ? link_stats.bytes_in : link_stats.bytes_out;
	    break;
	default:
	    used = link_stats.bytes_in+link_stats.bytes_out;
	    break;
    }
    if (used > maxoctets) {
	ppp_notice("Traffic limit reached. Limit: %u Used: %u", maxoctets, used);
	status = EXIT_TRAFFIC_LIMIT;
	lcp_close(pcb, "Traffic limit");
#if 0 /* UNUSED */
	need_holdoff = 0;
#endif /* UNUSED */
    } else {
        TIMEOUT(check_maxoctets, NULL, maxoctets_timeout);
    }
#endif /* PPP_STATS_SUPPORT */
}
#endif /* MAXOCTETS */

#if PPP_IDLETIMELIMIT
/*
 * check_idle - check whether the link has been idle for long
 * enough that we can shut it down.
 */
static void check_idle(void *arg) {
    ppp_pcb *pcb = (ppp_pcb*)arg;
    struct ppp_idle idle;
    time_t itime;
    int tlim;

    if (!get_idle_time(pcb, &idle))
	return;
#if 0 /* UNUSED */
    if (idle_time_hook != 0) {
	tlim = idle_time_hook(&idle);
    } else {
#endif /* UNUSED */
	itime = LWIP_MIN(idle.xmit_idle, idle.recv_idle);
	tlim = pcb->settings.idle_time_limit - itime;
#if 0 /* UNUSED */
    }
#endif /* UNUSED */
    if (tlim <= 0) {
	/* link is idle: shut it down. */
	ppp_notice("Terminating connection due to lack of activity.");
	pcb->err_code = PPPERR_IDLETIMEOUT;
	lcp_close(pcb, "Link inactive");
#if 0 /* UNUSED */
	need_holdoff = 0;
#endif /* UNUSED */
    } else {
	TIMEOUT(check_idle, (void*)pcb, tlim);
    }
}
#endif /* PPP_IDLETIMELIMIT */

#if PPP_MAXCONNECT
/*
 * connect_time_expired - log a message and close the connection.
 */
static void connect_time_expired(void *arg) {
    ppp_pcb *pcb = (ppp_pcb*)arg;
    ppp_info("Connect time expired");
    pcb->err_code = PPPERR_CONNECTTIME;
    lcp_close(pcb, "Connect time expired");	/* Close connection */
}
#endif /* PPP_MAXCONNECT */

#if PPP_OPTIONS
/*
 * auth_check_options - called to check authentication options.
 */
void
auth_check_options()
{
    lcp_options *wo = &lcp_wantoptions[0];
    int can_auth;
    int lacks_ip;

    /* Default our_name to hostname, and user to our_name */
    if (our_name[0] == 0 || usehostname)
	strlcpy(our_name, hostname, sizeof(our_name));
    /* If a blank username was explicitly given as an option, trust
       the user and don't use our_name */
    if (ppp_settings.user[0] == 0 && !explicit_user)
	strlcpy(ppp_settings.user, our_name, sizeof(ppp_settings.user));

    /*
     * If we have a default route, require the peer to authenticate
     * unless the noauth option was given or the real user is root.
     */
    if (!auth_required && !allow_any_ip && have_route_to(0) && !privileged) {
	auth_required = 1;
	default_auth = 1;
    }

#if CHAP_SUPPORT
    /* If we selected any CHAP flavors, we should probably negotiate it. :-) */
    if (wo->chap_mdtype)
	wo->neg_chap = 1;
#endif /* CHAP_SUPPORT */

    /* If authentication is required, ask peer for CHAP, PAP, or EAP. */
    if (auth_required) {
	allow_any_ip = 0;
	if (1
#if CHAP_SUPPORT
	    && !wo->neg_chap
#endif /* CHAP_SUPPORT */
#if PAP_SUPPORT
	    && !wo->neg_upap
#endif /* PAP_SUPPORT */
#if EAP_SUPPORT
	    && !wo->neg_eap
#endif /* EAP_SUPPORT */
	    ) {
#if CHAP_SUPPORT
	    wo->neg_chap = CHAP_MDTYPE_SUPPORTED != MDTYPE_NONE;
	    wo->chap_mdtype = CHAP_MDTYPE_SUPPORTED;
#endif /* CHAP_SUPPORT */
#if PAP_SUPPORT
	    wo->neg_upap = 1;
#endif /* PAP_SUPPORT */
#if EAP_SUPPORT
	    wo->neg_eap = 1;
#endif /* EAP_SUPPORT */
	}
    } else {
#if CHAP_SUPPORT
	wo->neg_chap = 0;
	wo->chap_mdtype = MDTYPE_NONE;
#endif /* CHAP_SUPPORT */
#if PAP_SUPPORT
	wo->neg_upap = 0;
#endif /* PAP_SUPPORT */
#if EAP_SUPPORT
	wo->neg_eap = 0;
#endif /* EAP_SUPPORT */
    }

    /*
     * Check whether we have appropriate secrets to use
     * to authenticate the peer.  Note that EAP can authenticate by way
     * of a CHAP-like exchanges as well as SRP.
     */
    lacks_ip = 0;
#if PAP_SUPPORT
    can_auth = wo->neg_upap && (uselogin || have_pap_secret(&lacks_ip));
#else
    can_auth = 0;
#endif /* PAP_SUPPORT */
    if (!can_auth && (0
#if CHAP_SUPPORT
	|| wo->neg_chap
#endif /* CHAP_SUPPORT */
#if EAP_SUPPORT
	|| wo->neg_eap
#endif /* EAP_SUPPORT */
	)) {
#if CHAP_SUPPORT
	can_auth = have_chap_secret((explicit_remote? remote_name: NULL),
				    our_name, 1, &lacks_ip);
#else
	can_auth = 0;
#endif
    }
    if (!can_auth
#if EAP_SUPPORT
	&& wo->neg_eap
#endif /* EAP_SUPPORT */
	) {
	can_auth = have_srp_secret((explicit_remote? remote_name: NULL),
				    our_name, 1, &lacks_ip);
    }

    if (auth_required && !can_auth && noauth_addrs == NULL) {
	if (default_auth) {
	    option_error(
"By default the remote system is required to authenticate itself");
	    option_error(
"(because this system has a default route to the internet)");
	} else if (explicit_remote)
	    option_error(
"The remote system (%s) is required to authenticate itself",
			 remote_name);
	else
	    option_error(
"The remote system is required to authenticate itself");
	option_error(
"but I couldn't find any suitable secret (password) for it to use to do so.");
	if (lacks_ip)
	    option_error(
"(None of the available passwords would let it use an IP address.)");

	exit(1);
    }

    /*
     * Early check for remote number authorization.
     */
    if (!auth_number()) {
	ppp_warn("calling number %q is not authorized", remote_number);
	exit(EXIT_CNID_AUTH_FAILED);
    }
}
#endif /* PPP_OPTIONS */

#if 0 /* UNUSED */
/*
 * auth_reset - called when LCP is starting negotiations to recheck
 * authentication options, i.e. whether we have appropriate secrets
 * to use for authenticating ourselves and/or the peer.
 */
void
auth_reset(unit)
    int unit;
{
    lcp_options *go = &lcp_gotoptions[unit];
    lcp_options *ao = &lcp_allowoptions[unit];
    int hadchap;

    hadchap = -1;
    ao->neg_upap = !refuse_pap && (passwd[0] != 0 || get_pap_passwd(NULL));
    ao->neg_chap = (!refuse_chap || !refuse_mschap || !refuse_mschap_v2)
	&& (passwd[0] != 0 ||
	    (hadchap = have_chap_secret(user, (explicit_remote? remote_name:
					       NULL), 0, NULL)));
    ao->neg_eap = !refuse_eap && (
	passwd[0] != 0 ||
	(hadchap == 1 || (hadchap == -1 && have_chap_secret(user,
	    (explicit_remote? remote_name: NULL), 0, NULL))) ||
	have_srp_secret(user, (explicit_remote? remote_name: NULL), 0, NULL));

    hadchap = -1;
    if (go->neg_upap && !uselogin && !have_pap_secret(NULL))
	go->neg_upap = 0;
    if (go->neg_chap) {
	if (!(hadchap = have_chap_secret((explicit_remote? remote_name: NULL),
			      our_name, 1, NULL)))
	    go->neg_chap = 0;
    }
    if (go->neg_eap &&
	(hadchap == 0 || (hadchap == -1 &&
	    !have_chap_secret((explicit_remote? remote_name: NULL), our_name,
		1, NULL))) &&
	!have_srp_secret((explicit_remote? remote_name: NULL), our_name, 1,
	    NULL))
	go->neg_eap = 0;
}

/*
 * check_passwd - Check the user name and passwd against the PAP secrets
 * file.  If requested, also check against the system password database,
 * and login the user if OK.
 *
 * returns:
 *	UPAP_AUTHNAK: Authentication failed.
 *	UPAP_AUTHACK: Authentication succeeded.
 * In either case, msg points to an appropriate message.
 */
int
check_passwd(unit, auser, userlen, apasswd, passwdlen, msg)
    int unit;
    char *auser;
    int userlen;
    char *apasswd;
    int passwdlen;
    char **msg;
{
  return UPAP_AUTHNAK;
    int ret;
    char *filename;
    FILE *f;
    struct wordlist *addrs = NULL, *opts = NULL;
    char passwd[256], user[256];
    char secret[MAXWORDLEN];
    static int attempts = 0;

    /*
     * Make copies of apasswd and auser, then null-terminate them.
     * If there are unprintable characters in the password, make
     * them visible.
     */
    slprintf(ppp_settings.passwd, sizeof(ppp_settings.passwd), "%.*v", passwdlen, apasswd);
    slprintf(ppp_settings.user, sizeof(ppp_settings.user), "%.*v", userlen, auser);
    *msg = "";

    /*
     * Check if a plugin wants to handle this.
     */
    if (pap_auth_hook) {
	ret = (*pap_auth_hook)(ppp_settings.user, ppp_settings.passwd, msg, &addrs, &opts);
	if (ret >= 0) {
	    /* note: set_allowed_addrs() saves opts (but not addrs):
	       don't free it! */
	    if (ret)
		set_allowed_addrs(unit, addrs, opts);
	    else if (opts != 0)
		free_wordlist(opts);
	    if (addrs != 0)
		free_wordlist(addrs);
	    BZERO(ppp_settings.passwd, sizeof(ppp_settings.passwd));
	    return ret? UPAP_AUTHACK: UPAP_AUTHNAK;
	}
    }

    /*
     * Open the file of pap secrets and scan for a suitable secret
     * for authenticating this user.
     */
    filename = _PATH_UPAPFILE;
    addrs = opts = NULL;
    ret = UPAP_AUTHNAK;
    f = fopen(filename, "r");
    if (f == NULL) {
	ppp_error("Can't open PAP password file %s: %m", filename);

    } else {
	check_access(f, filename);
	if (scan_authfile(f, ppp_settings.user, our_name, secret, &addrs, &opts, filename, 0) < 0) {
	    ppp_warn("no PAP secret found for %s", user);
	} else {
	    /*
	     * If the secret is "@login", it means to check
	     * the password against the login database.
	     */
	    int login_secret = strcmp(secret, "@login") == 0;
	    ret = UPAP_AUTHACK;
	    if (uselogin || login_secret) {
		/* login option or secret is @login */
		if (session_full(ppp_settings.user, ppp_settings.passwd, devnam, msg) == 0) {
		    ret = UPAP_AUTHNAK;
		}
	    } else if (session_mgmt) {
		if (session_check(ppp_settings.user, NULL, devnam, NULL) == 0) {
		    ppp_warn("Peer %q failed PAP Session verification", user);
		    ret = UPAP_AUTHNAK;
		}
	    }
	    if (secret[0] != 0 && !login_secret) {
		/* password given in pap-secrets - must match */
		if ((cryptpap || strcmp(ppp_settings.passwd, secret) != 0)
		    && strcmp(crypt(ppp_settings.passwd, secret), secret) != 0)
		    ret = UPAP_AUTHNAK;
	    }
	}
	fclose(f);
    }

    if (ret == UPAP_AUTHNAK) {
        if (**msg == 0)
	    *msg = "Login incorrect";
	/*
	 * XXX can we ever get here more than once??
	 * Frustrate passwd stealer programs.
	 * Allow 10 tries, but start backing off after 3 (stolen from login).
	 * On 10'th, drop the connection.
	 */
	if (attempts++ >= 10) {
	    ppp_warn("%d LOGIN FAILURES ON %s, %s", attempts, devnam, user);
	    lcp_close(pcb, "login failed");
	}
	if (attempts > 3)
	    sleep((u_int) (attempts - 3) * 5);
	if (opts != NULL)
	    free_wordlist(opts);

    } else {
	attempts = 0;			/* Reset count */
	if (**msg == 0)
	    *msg = "Login ok";
	set_allowed_addrs(unit, addrs, opts);
    }

    if (addrs != NULL)
	free_wordlist(addrs);
    BZERO(ppp_settings.passwd, sizeof(ppp_settings.passwd));
    BZERO(secret, sizeof(secret));

    return ret;
}

/*
 * null_login - Check if a username of "" and a password of "" are
 * acceptable, and iff so, set the list of acceptable IP addresses
 * and return 1.
 */
static int
null_login(unit)
    int unit;
{
    char *filename;
    FILE *f;
    int i, ret;
    struct wordlist *addrs, *opts;
    char secret[MAXWORDLEN];

    /*
     * Check if a plugin wants to handle this.
     */
    ret = -1;
    if (null_auth_hook)
	ret = (*null_auth_hook)(&addrs, &opts);

    /*
     * Open the file of pap secrets and scan for a suitable secret.
     */
    if (ret <= 0) {
	filename = _PATH_UPAPFILE;
	addrs = NULL;
	f = fopen(filename, "r");
	if (f == NULL)
	    return 0;
	check_access(f, filename);

	i = scan_authfile(f, "", our_name, secret, &addrs, &opts, filename, 0);
	ret = i >= 0 && secret[0] == 0;
	BZERO(secret, sizeof(secret));
	fclose(f);
    }

    if (ret)
	set_allowed_addrs(unit, addrs, opts);
    else if (opts != 0)
	free_wordlist(opts);
    if (addrs != 0)
	free_wordlist(addrs);

    return ret;
}

/*
 * get_pap_passwd - get a password for authenticating ourselves with
 * our peer using PAP.  Returns 1 on success, 0 if no suitable password
 * could be found.
 * Assumes passwd points to MAXSECRETLEN bytes of space (if non-null).
 */
static int
get_pap_passwd(passwd)
    char *passwd;
{
    char *filename;
    FILE *f;
    int ret;
    char secret[MAXWORDLEN];

    /*
     * Check whether a plugin wants to supply this.
     */
    if (pap_passwd_hook) {
	ret = (*pap_passwd_hook)(ppp_settings,user, ppp_settings.passwd);
	if (ret >= 0)
	    return ret;
    }

    filename = _PATH_UPAPFILE;
    f = fopen(filename, "r");
    if (f == NULL)
	return 0;
    check_access(f, filename);
    ret = scan_authfile(f, user,
			(remote_name[0]? remote_name: NULL),
			secret, NULL, NULL, filename, 0);
    fclose(f);
    if (ret < 0)
	return 0;
    if (passwd != NULL)
	strlcpy(passwd, secret, MAXSECRETLEN);
    BZERO(secret, sizeof(secret));
    return 1;
}

/*
 * have_pap_secret - check whether we have a PAP file with any
 * secrets that we could possibly use for authenticating the peer.
 */
static int
have_pap_secret(lacks_ipp)
    int *lacks_ipp;
{
    FILE *f;
    int ret;
    char *filename;
    struct wordlist *addrs;

    /* let the plugin decide, if there is one */
    if (pap_check_hook) {
	ret = (*pap_check_hook)();
	if (ret >= 0)
	    return ret;
    }

    filename = _PATH_UPAPFILE;
    f = fopen(filename, "r");
    if (f == NULL)
	return 0;

    ret = scan_authfile(f, (explicit_remote? remote_name: NULL), our_name,
			NULL, &addrs, NULL, filename, 0);
    fclose(f);
    if (ret >= 0 && !some_ip_ok(addrs)) {
	if (lacks_ipp != 0)
	    *lacks_ipp = 1;
	ret = -1;
    }
    if (addrs != 0)
	free_wordlist(addrs);

    return ret >= 0;
}

/*
 * have_chap_secret - check whether we have a CHAP file with a
 * secret that we could possibly use for authenticating `client'
 * on `server'.  Either can be the null string, meaning we don't
 * know the identity yet.
 */
static int
have_chap_secret(client, server, need_ip, lacks_ipp)
    char *client;
    char *server;
    int need_ip;
    int *lacks_ipp;
{
    FILE *f;
    int ret;
    char *filename;
    struct wordlist *addrs;

    if (chap_check_hook) {
	ret = (*chap_check_hook)();
	if (ret >= 0) {
	    return ret;
	}
    }

    filename = _PATH_CHAPFILE;
    f = fopen(filename, "r");
    if (f == NULL)
	return 0;

    if (client != NULL && client[0] == 0)
	client = NULL;
    else if (server != NULL && server[0] == 0)
	server = NULL;

    ret = scan_authfile(f, client, server, NULL, &addrs, NULL, filename, 0);
    fclose(f);
    if (ret >= 0 && need_ip && !some_ip_ok(addrs)) {
	if (lacks_ipp != 0)
	    *lacks_ipp = 1;
	ret = -1;
    }
    if (addrs != 0)
	free_wordlist(addrs);

    return ret >= 0;
}

/*
 * have_srp_secret - check whether we have a SRP file with a
 * secret that we could possibly use for authenticating `client'
 * on `server'.  Either can be the null string, meaning we don't
 * know the identity yet.
 */
static int
have_srp_secret(client, server, need_ip, lacks_ipp)
    char *client;
    char *server;
    int need_ip;
    int *lacks_ipp;
{
    FILE *f;
    int ret;
    char *filename;
    struct wordlist *addrs;

    filename = _PATH_SRPFILE;
    f = fopen(filename, "r");
    if (f == NULL)
	return 0;

    if (client != NULL && client[0] == 0)
	client = NULL;
    else if (server != NULL && server[0] == 0)
	server = NULL;

    ret = scan_authfile(f, client, server, NULL, &addrs, NULL, filename, 0);
    fclose(f);
    if (ret >= 0 && need_ip && !some_ip_ok(addrs)) {
	if (lacks_ipp != 0)
	    *lacks_ipp = 1;
	ret = -1;
    }
    if (addrs != 0)
	free_wordlist(addrs);

    return ret >= 0;
}
#endif /* UNUSED */

#if PPP_AUTH_SUPPORT
/*
 * get_secret - open the CHAP secret file and return the secret
 * for authenticating the given client on the given server.
 * (We could be either client or server).
 */
int get_secret(ppp_pcb *pcb, const char *client, const char *server, char *secret, int *secret_len, int am_server) {
  int len;
  LWIP_UNUSED_ARG(server);
  LWIP_UNUSED_ARG(am_server);

  if (!client || !client[0] || !pcb->settings.user || !pcb->settings.passwd || strcmp(client, pcb->settings.user)) {
    return 0;
  }

  len = (int)strlen(pcb->settings.passwd);
  if (len > MAXSECRETLEN) {
    ppp_error("Secret for %s on %s is too long", client, server);
    len = MAXSECRETLEN;
  }

  MEMCPY(secret, pcb->settings.passwd, len);
  *secret_len = len;
  return 1;

#if 0 /* UNUSED */
    FILE *f;
    int ret, len;
    char *filename;
    struct wordlist *addrs, *opts;
    char secbuf[MAXWORDLEN];
    struct wordlist *addrs;
    addrs = NULL;

    if (!am_server && ppp_settings.passwd[0] != 0) {
	strlcpy(secbuf, ppp_settings.passwd, sizeof(secbuf));
    } else if (!am_server && chap_passwd_hook) {
	if ( (*chap_passwd_hook)(client, secbuf) < 0) {
	    ppp_error("Unable to obtain CHAP password for %s on %s from plugin",
		  client, server);
	    return 0;
	}
    } else {
	filename = _PATH_CHAPFILE;
	addrs = NULL;
	secbuf[0] = 0;

	f = fopen(filename, "r");
	if (f == NULL) {
	    ppp_error("Can't open chap secret file %s: %m", filename);
	    return 0;
	}
	check_access(f, filename);

	ret = scan_authfile(f, client, server, secbuf, &addrs, &opts, filename, 0);
	fclose(f);
	if (ret < 0)
	    return 0;

	if (am_server)
	    set_allowed_addrs(unit, addrs, opts);
	else if (opts != 0)
	    free_wordlist(opts);
	if (addrs != 0)
	    free_wordlist(addrs);
    }

    len = strlen(secbuf);
    if (len > MAXSECRETLEN) {
	ppp_error("Secret for %s on %s is too long", client, server);
	len = MAXSECRETLEN;
    }
    MEMCPY(secret, secbuf, len);
    BZERO(secbuf, sizeof(secbuf));
    *secret_len = len;

    return 1;
#endif /* UNUSED */
}
#endif /* PPP_AUTH_SUPPORT */


#if 0 /* UNUSED */
/*
 * get_srp_secret - open the SRP secret file and return the secret
 * for authenticating the given client on the given server.
 * (We could be either client or server).
 */
int
get_srp_secret(unit, client, server, secret, am_server)
    int unit;
    char *client;
    char *server;
    char *secret;
    int am_server;
{
    FILE *fp;
    int ret;
    char *filename;
    struct wordlist *addrs, *opts;

    if (!am_server && ppp_settings.passwd[0] != '\0') {
	strlcpy(secret, ppp_settings.passwd, MAXWORDLEN);
    } else {
	filename = _PATH_SRPFILE;
	addrs = NULL;

	fp = fopen(filename, "r");
	if (fp == NULL) {
	    ppp_error("Can't open srp secret file %s: %m", filename);
	    return 0;
	}
	check_access(fp, filename);

	secret[0] = '\0';
	ret = scan_authfile(fp, client, server, secret, &addrs, &opts,
	    filename, am_server);
	fclose(fp);
	if (ret < 0)
	    return 0;

	if (am_server)
	    set_allowed_addrs(unit, addrs, opts);
	else if (opts != NULL)
	    free_wordlist(opts);
	if (addrs != NULL)
	    free_wordlist(addrs);
    }

    return 1;
}

/*
 * set_allowed_addrs() - set the list of allowed addresses.
 * Also looks for `--' indicating options to apply for this peer
 * and leaves the following words in extra_options.
 */
static void
set_allowed_addrs(unit, addrs, opts)
    int unit;
    struct wordlist *addrs;
    struct wordlist *opts;
{
    int n;
    struct wordlist *ap, **plink;
    struct permitted_ip *ip;
    char *ptr_word, *ptr_mask;
    struct hostent *hp;
    struct netent *np;
    u32_t a, mask, ah, offset;
    struct ipcp_options *wo = &ipcp_wantoptions[unit];
    u32_t suggested_ip = 0;

    if (addresses[unit] != NULL)
	free(addresses[unit]);
    addresses[unit] = NULL;
    if (extra_options != NULL)
	free_wordlist(extra_options);
    extra_options = opts;

    /*
     * Count the number of IP addresses given.
     */
    n = wordlist_count(addrs) + wordlist_count(noauth_addrs);
    if (n == 0)
	return;
    ip = (struct permitted_ip *) malloc((n + 1) * sizeof(struct permitted_ip));
    if (ip == 0)
	return;

    /* temporarily append the noauth_addrs list to addrs */
    for (plink = &addrs; *plink != NULL; plink = &(*plink)->next)
	;
    *plink = noauth_addrs;

    n = 0;
    for (ap = addrs; ap != NULL; ap = ap->next) {
	/* "-" means no addresses authorized, "*" means any address allowed */
	ptr_word = ap->word;
	if (strcmp(ptr_word, "-") == 0)
	    break;
	if (strcmp(ptr_word, "*") == 0) {
	    ip[n].permit = 1;
	    ip[n].base = ip[n].mask = 0;
	    ++n;
	    break;
	}

	ip[n].permit = 1;
	if (*ptr_word == '!') {
	    ip[n].permit = 0;
	    ++ptr_word;
	}

	mask = ~ (u32_t) 0;
	offset = 0;
	ptr_mask = strchr (ptr_word, '/');
	if (ptr_mask != NULL) {
	    int bit_count;
	    char *endp;

	    bit_count = (int) strtol (ptr_mask+1, &endp, 10);
	    if (bit_count <= 0 || bit_count > 32) {
		ppp_warn("invalid address length %v in auth. address list",
		     ptr_mask+1);
		continue;
	    }
	    bit_count = 32 - bit_count;	/* # bits in host part */
	    if (*endp == '+') {
		offset = ifunit + 1;
		++endp;
	    }
	    if (*endp != 0) {
		ppp_warn("invalid address length syntax: %v", ptr_mask+1);
		continue;
	    }
	    *ptr_mask = '\0';
	    mask <<= bit_count;
	}

	hp = gethostbyname(ptr_word);
	if (hp != NULL && hp->h_addrtype == AF_INET) {
	    a = *(u32_t *)hp->h_addr;
	} else {
	    np = getnetbyname (ptr_word);
	    if (np != NULL && np->n_addrtype == AF_INET) {
		a = lwip_htonl ((u32_t)np->n_net);
		if (ptr_mask == NULL) {
		    /* calculate appropriate mask for net */
		    ah = lwip_ntohl(a);
		    if (IN_CLASSA(ah))
			mask = IN_CLASSA_NET;
		    else if (IN_CLASSB(ah))
			mask = IN_CLASSB_NET;
		    else if (IN_CLASSC(ah))
			mask = IN_CLASSC_NET;
		}
	    } else {
		a = inet_addr (ptr_word);
	    }
	}

	if (ptr_mask != NULL)
	    *ptr_mask = '/';

	if (a == (u32_t)-1L) {
	    ppp_warn("unknown host %s in auth. address list", ap->word);
	    continue;
	}
	if (offset != 0) {
	    if (offset >= ~mask) {
		ppp_warn("interface unit %d too large for subnet %v",
		     ifunit, ptr_word);
		continue;
	    }
	    a = lwip_htonl((lwip_ntohl(a) & mask) + offset);
	    mask = ~(u32_t)0;
	}
	ip[n].mask = lwip_htonl(mask);
	ip[n].base = a & ip[n].mask;
	++n;
	if (~mask == 0 && suggested_ip == 0)
	    suggested_ip = a;
    }
    *plink = NULL;

    ip[n].permit = 0;		/* make the last entry forbid all addresses */
    ip[n].base = 0;		/* to terminate the list */
    ip[n].mask = 0;

    addresses[unit] = ip;

    /*
     * If the address given for the peer isn't authorized, or if
     * the user hasn't given one, AND there is an authorized address
     * which is a single host, then use that if we find one.
     */
    if (suggested_ip != 0
	&& (wo->hisaddr == 0 || !auth_ip_addr(unit, wo->hisaddr))) {
	wo->hisaddr = suggested_ip;
	/*
	 * Do we insist on this address?  No, if there are other
	 * addresses authorized than the suggested one.
	 */
	if (n > 1)
	    wo->accept_remote = 1;
    }
}

/*
 * auth_ip_addr - check whether the peer is authorized to use
 * a given IP address.  Returns 1 if authorized, 0 otherwise.
 */
int
auth_ip_addr(unit, addr)
    int unit;
    u32_t addr;
{
    int ok;

    /* don't allow loopback or multicast address */
    if (bad_ip_adrs(addr))
	return 0;

    if (allowed_address_hook) {
	ok = allowed_address_hook(addr);
	if (ok >= 0) return ok;
    }

    if (addresses[unit] != NULL) {
	ok = ip_addr_check(addr, addresses[unit]);
	if (ok >= 0)
	    return ok;
    }

    if (auth_required)
	return 0;		/* no addresses authorized */
    return allow_any_ip || privileged || !have_route_to(addr);
}

static int
ip_addr_check(addr, addrs)
    u32_t addr;
    struct permitted_ip *addrs;
{
    for (; ; ++addrs)
	if ((addr & addrs->mask) == addrs->base)
	    return addrs->permit;
}

/*
 * bad_ip_adrs - return 1 if the IP address is one we don't want
 * to use, such as an address in the loopback net or a multicast address.
 * addr is in network byte order.
 */
int
bad_ip_adrs(addr)
    u32_t addr;
{
    addr = lwip_ntohl(addr);
    return (addr >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET
	|| IN_MULTICAST(addr) || IN_BADCLASS(addr);
}

/*
 * some_ip_ok - check a wordlist to see if it authorizes any
 * IP address(es).
 */
static int
some_ip_ok(addrs)
    struct wordlist *addrs;
{
    for (; addrs != 0; addrs = addrs->next) {
	if (addrs->word[0] == '-')
	    break;
	if (addrs->word[0] != '!')
	    return 1;		/* some IP address is allowed */
    }
    return 0;
}

/*
 * auth_number - check whether the remote number is allowed to connect.
 * Returns 1 if authorized, 0 otherwise.
 */
int
auth_number()
{
    struct wordlist *wp = permitted_numbers;
    int l;

    /* Allow all if no authorization list. */
    if (!wp)
	return 1;

    /* Allow if we have a match in the authorization list. */
    while (wp) {
	/* trailing '*' wildcard */
	l = strlen(wp->word);
	if ((wp->word)[l - 1] == '*')
	    l--;
	if (!strncasecmp(wp->word, remote_number, l))
	    return 1;
	wp = wp->next;
    }

    return 0;
}

/*
 * check_access - complain if a secret file has too-liberal permissions.
 */
static void
check_access(f, filename)
    FILE *f;
    char *filename;
{
    struct stat sbuf;

    if (fstat(fileno(f), &sbuf) < 0) {
	ppp_warn("cannot stat secret file %s: %m", filename);
    } else if ((sbuf.st_mode & (S_IRWXG | S_IRWXO)) != 0) {
	ppp_warn("Warning - secret file %s has world and/or group access",
	     filename);
    }
}

/*
 * scan_authfile - Scan an authorization file for a secret suitable
 * for authenticating `client' on `server'.  The return value is -1
 * if no secret is found, otherwise >= 0.  The return value has
 * NONWILD_CLIENT set if the secret didn't have "*" for the client, and
 * NONWILD_SERVER set if the secret didn't have "*" for the server.
 * Any following words on the line up to a "--" (i.e. address authorization
 * info) are placed in a wordlist and returned in *addrs.  Any
 * following words (extra options) are placed in a wordlist and
 * returned in *opts.
 * We assume secret is NULL or points to MAXWORDLEN bytes of space.
 * Flags are non-zero if we need two colons in the secret in order to
 * match.
 */
static int
scan_authfile(f, client, server, secret, addrs, opts, filename, flags)
    FILE *f;
    char *client;
    char *server;
    char *secret;
    struct wordlist **addrs;
    struct wordlist **opts;
    char *filename;
    int flags;
{
    int newline, xxx;
    int got_flag, best_flag;
    FILE *sf;
    struct wordlist *ap, *addr_list, *alist, **app;
    char word[MAXWORDLEN];
    char atfile[MAXWORDLEN];
    char lsecret[MAXWORDLEN];
    char *cp;

    if (addrs != NULL)
	*addrs = NULL;
    if (opts != NULL)
	*opts = NULL;
    addr_list = NULL;
    if (!getword(f, word, &newline, filename))
	return -1;		/* file is empty??? */
    newline = 1;
    best_flag = -1;
    for (;;) {
	/*
	 * Skip until we find a word at the start of a line.
	 */
	while (!newline && getword(f, word, &newline, filename))
	    ;
	if (!newline)
	    break;		/* got to end of file */

	/*
	 * Got a client - check if it's a match or a wildcard.
	 */
	got_flag = 0;
	if (client != NULL && strcmp(word, client) != 0 && !ISWILD(word)) {
	    newline = 0;
	    continue;
	}
	if (!ISWILD(word))
	    got_flag = NONWILD_CLIENT;

	/*
	 * Now get a server and check if it matches.
	 */
	if (!getword(f, word, &newline, filename))
	    break;
	if (newline)
	    continue;
	if (!ISWILD(word)) {
	    if (server != NULL && strcmp(word, server) != 0)
		continue;
	    got_flag |= NONWILD_SERVER;
	}

	/*
	 * Got some sort of a match - see if it's better than what
	 * we have already.
	 */
	if (got_flag <= best_flag)
	    continue;

	/*
	 * Get the secret.
	 */
	if (!getword(f, word, &newline, filename))
	    break;
	if (newline)
	    continue;

	/*
	 * SRP-SHA1 authenticator should never be reading secrets from
	 * a file.  (Authenticatee may, though.)
	 */
	if (flags && ((cp = strchr(word, ':')) == NULL ||
	    strchr(cp + 1, ':') == NULL))
	    continue;

	if (secret != NULL) {
	    /*
	     * Special syntax: @/pathname means read secret from file.
	     */
	    if (word[0] == '@' && word[1] == '/') {
		strlcpy(atfile, word+1, sizeof(atfile));
		if ((sf = fopen(atfile, "r")) == NULL) {
		    ppp_warn("can't open indirect secret file %s", atfile);
		    continue;
		}
		check_access(sf, atfile);
		if (!getword(sf, word, &xxx, atfile)) {
		    ppp_warn("no secret in indirect secret file %s", atfile);
		    fclose(sf);
		    continue;
		}
		fclose(sf);
	    }
	    strlcpy(lsecret, word, sizeof(lsecret));
	}

	/*
	 * Now read address authorization info and make a wordlist.
	 */
	app = &alist;
	for (;;) {
	    if (!getword(f, word, &newline, filename) || newline)
		break;
	    ap = (struct wordlist *)
		    malloc(sizeof(struct wordlist) + strlen(word) + 1);
	    if (ap == NULL)
		novm("authorized addresses");
	    ap->word = (char *) (ap + 1);
	    strcpy(ap->word, word);
	    *app = ap;
	    app = &ap->next;
	}
	*app = NULL;

	/*
	 * This is the best so far; remember it.
	 */
	best_flag = got_flag;
	if (addr_list)
	    free_wordlist(addr_list);
	addr_list = alist;
	if (secret != NULL)
	    strlcpy(secret, lsecret, MAXWORDLEN);

	if (!newline)
	    break;
    }

    /* scan for a -- word indicating the start of options */
    for (app = &addr_list; (ap = *app) != NULL; app = &ap->next)
	if (strcmp(ap->word, "--") == 0)
	    break;
    /* ap = start of options */
    if (ap != NULL) {
	ap = ap->next;		/* first option */
	free(*app);			/* free the "--" word */
	*app = NULL;		/* terminate addr list */
    }
    if (opts != NULL)
	*opts = ap;
    else if (ap != NULL)
	free_wordlist(ap);
    if (addrs != NULL)
	*addrs = addr_list;
    else if (addr_list != NULL)
	free_wordlist(addr_list);

    return best_flag;
}

/*
 * wordlist_count - return the number of items in a wordlist
 */
static int
wordlist_count(wp)
    struct wordlist *wp;
{
    int n;

    for (n = 0; wp != NULL; wp = wp->next)
	++n;
    return n;
}

/*
 * free_wordlist - release memory allocated for a wordlist.
 */
static void
free_wordlist(wp)
    struct wordlist *wp;
{
    struct wordlist *next;

    while (wp != NULL) {
	next = wp->next;
	free(wp);
	wp = next;
    }
}
#endif /* UNUSED */

#endif /* PPP_SUPPORT */
