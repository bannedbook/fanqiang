/*
 * ipcp.c - PPP IP Control Protocol.
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
#if PPP_SUPPORT && PPP_IPV4_SUPPORT /* don't build if not configured for use in lwipopts.h */

/*
 * @todo:
 */

#if 0 /* UNUSED */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* UNUSED */

#include "netif/ppp/ppp_impl.h"

#include "netif/ppp/fsm.h"
#include "netif/ppp/ipcp.h"

#if 0 /* UNUSED */
/* global vars */
u32_t netmask = 0;		/* IP netmask to set on interface */
#endif /* UNUSED */

#if 0 /* UNUSED */
bool	disable_defaultip = 0;	/* Don't use hostname for default IP adrs */
#endif /* UNUSED */

#if 0 /* moved to ppp_settings */
bool	noremoteip = 0;		/* Let him have no IP address */
#endif /* moved to ppp_setting */

#if 0 /* UNUSED */
/* Hook for a plugin to know when IP protocol has come up */
void (*ip_up_hook) (void) = NULL;

/* Hook for a plugin to know when IP protocol has come down */
void (*ip_down_hook) (void) = NULL;

/* Hook for a plugin to choose the remote IP address */
void (*ip_choose_hook) (u32_t *) = NULL;
#endif /* UNUSED */

#if PPP_NOTIFY
/* Notifiers for when IPCP goes up and down */
struct notifier *ip_up_notifier = NULL;
struct notifier *ip_down_notifier = NULL;
#endif /* PPP_NOTIFY */

/* local vars */
#if 0 /* moved to ppp_pcb */
static int default_route_set[NUM_PPP];	/* Have set up a default route */
static int proxy_arp_set[NUM_PPP];	/* Have created proxy arp entry */
static int ipcp_is_up;			/* have called np_up() */
static int ipcp_is_open;		/* haven't called np_finished() */
static bool ask_for_local;		/* request our address from peer */
#endif /* moved to ppp_pcb */
#if 0 /* UNUSED */
static char vj_value[8];		/* string form of vj option value */
static char netmask_str[20];		/* string form of netmask value */
#endif /* UNUSED */

/*
 * Callbacks for fsm code.  (CI = Configuration Information)
 */
static void ipcp_resetci(fsm *f);	/* Reset our CI */
static int  ipcp_cilen(fsm *f);	        /* Return length of our CI */
static void ipcp_addci(fsm *f, u_char *ucp, int *lenp); /* Add our CI */
static int  ipcp_ackci(fsm *f, u_char *p, int len);	/* Peer ack'd our CI */
static int  ipcp_nakci(fsm *f, u_char *p, int len, int treat_as_reject);/* Peer nak'd our CI */
static int  ipcp_rejci(fsm *f, u_char *p, int len);	/* Peer rej'd our CI */
static int  ipcp_reqci(fsm *f, u_char *inp, int *len, int reject_if_disagree); /* Rcv CI */
static void ipcp_up(fsm *f);		/* We're UP */
static void ipcp_down(fsm *f);		/* We're DOWN */
static void ipcp_finished(fsm *f);	/* Don't need lower layer */

static const fsm_callbacks ipcp_callbacks = { /* IPCP callback routines */
    ipcp_resetci,		/* Reset our Configuration Information */
    ipcp_cilen,			/* Length of our Configuration Information */
    ipcp_addci,			/* Add our Configuration Information */
    ipcp_ackci,			/* ACK our Configuration Information */
    ipcp_nakci,			/* NAK our Configuration Information */
    ipcp_rejci,			/* Reject our Configuration Information */
    ipcp_reqci,			/* Request peer's Configuration Information */
    ipcp_up,			/* Called when fsm reaches OPENED state */
    ipcp_down,			/* Called when fsm leaves OPENED state */
    NULL,			/* Called when we want the lower layer up */
    ipcp_finished,		/* Called when we want the lower layer down */
    NULL,			/* Called when Protocol-Reject received */
    NULL,			/* Retransmission is necessary */
    NULL,			/* Called to handle protocol-specific codes */
    "IPCP"			/* String name of protocol */
};

/*
 * Command-line options.
 */
#if PPP_OPTIONS
static int setvjslots (char **);
static int setdnsaddr (char **);
static int setwinsaddr (char **);
static int setnetmask (char **);
int setipaddr (char *, char **, int);

static void printipaddr (option_t *, void (*)(void *, char *,...),void *);

static option_t ipcp_option_list[] = {
    { "noip", o_bool, &ipcp_protent.enabled_flag,
      "Disable IP and IPCP" },
    { "-ip", o_bool, &ipcp_protent.enabled_flag,
      "Disable IP and IPCP", OPT_ALIAS },

    { "novj", o_bool, &ipcp_wantoptions[0].neg_vj,
      "Disable VJ compression", OPT_A2CLR, &ipcp_allowoptions[0].neg_vj },
    { "-vj", o_bool, &ipcp_wantoptions[0].neg_vj,
      "Disable VJ compression", OPT_ALIAS | OPT_A2CLR,
      &ipcp_allowoptions[0].neg_vj },

    { "novjccomp", o_bool, &ipcp_wantoptions[0].cflag,
      "Disable VJ connection-ID compression", OPT_A2CLR,
      &ipcp_allowoptions[0].cflag },
    { "-vjccomp", o_bool, &ipcp_wantoptions[0].cflag,
      "Disable VJ connection-ID compression", OPT_ALIAS | OPT_A2CLR,
      &ipcp_allowoptions[0].cflag },

    { "vj-max-slots", o_special, (void *)setvjslots,
      "Set maximum VJ header slots",
      OPT_PRIO | OPT_A2STRVAL | OPT_STATIC, vj_value },

    { "ipcp-accept-local", o_bool, &ipcp_wantoptions[0].accept_local,
      "Accept peer's address for us", 1 },
    { "ipcp-accept-remote", o_bool, &ipcp_wantoptions[0].accept_remote,
      "Accept peer's address for it", 1 },

    { "ipparam", o_string, &ipparam,
      "Set ip script parameter", OPT_PRIO },

    { "noipdefault", o_bool, &disable_defaultip,
      "Don't use name for default IP adrs", 1 },

    { "ms-dns", 1, (void *)setdnsaddr,
      "DNS address for the peer's use" },
    { "ms-wins", 1, (void *)setwinsaddr,
      "Nameserver for SMB over TCP/IP for peer" },

    { "ipcp-restart", o_int, &ipcp_fsm[0].timeouttime,
      "Set timeout for IPCP", OPT_PRIO },
    { "ipcp-max-terminate", o_int, &ipcp_fsm[0].maxtermtransmits,
      "Set max #xmits for term-reqs", OPT_PRIO },
    { "ipcp-max-configure", o_int, &ipcp_fsm[0].maxconfreqtransmits,
      "Set max #xmits for conf-reqs", OPT_PRIO },
    { "ipcp-max-failure", o_int, &ipcp_fsm[0].maxnakloops,
      "Set max #conf-naks for IPCP", OPT_PRIO },

    { "defaultroute", o_bool, &ipcp_wantoptions[0].default_route,
      "Add default route", OPT_ENABLE|1, &ipcp_allowoptions[0].default_route },
    { "nodefaultroute", o_bool, &ipcp_allowoptions[0].default_route,
      "disable defaultroute option", OPT_A2CLR,
      &ipcp_wantoptions[0].default_route },
    { "-defaultroute", o_bool, &ipcp_allowoptions[0].default_route,
      "disable defaultroute option", OPT_ALIAS | OPT_A2CLR,
      &ipcp_wantoptions[0].default_route },

    { "replacedefaultroute", o_bool,
				&ipcp_wantoptions[0].replace_default_route,
      "Replace default route", 1
    },
    { "noreplacedefaultroute", o_bool,
				&ipcp_allowoptions[0].replace_default_route,
      "Never replace default route", OPT_A2COPY,
				&ipcp_wantoptions[0].replace_default_route },
    { "proxyarp", o_bool, &ipcp_wantoptions[0].proxy_arp,
      "Add proxy ARP entry", OPT_ENABLE|1, &ipcp_allowoptions[0].proxy_arp },
    { "noproxyarp", o_bool, &ipcp_allowoptions[0].proxy_arp,
      "disable proxyarp option", OPT_A2CLR,
      &ipcp_wantoptions[0].proxy_arp },
    { "-proxyarp", o_bool, &ipcp_allowoptions[0].proxy_arp,
      "disable proxyarp option", OPT_ALIAS | OPT_A2CLR,
      &ipcp_wantoptions[0].proxy_arp },

    { "usepeerdns", o_bool, &usepeerdns,
      "Ask peer for DNS address(es)", 1 },

    { "netmask", o_special, (void *)setnetmask,
      "set netmask", OPT_PRIO | OPT_A2STRVAL | OPT_STATIC, netmask_str },

    { "ipcp-no-addresses", o_bool, &ipcp_wantoptions[0].old_addrs,
      "Disable old-style IP-Addresses usage", OPT_A2CLR,
      &ipcp_allowoptions[0].old_addrs },
    { "ipcp-no-address", o_bool, &ipcp_wantoptions[0].neg_addr,
      "Disable IP-Address usage", OPT_A2CLR,
      &ipcp_allowoptions[0].neg_addr },

    { "noremoteip", o_bool, &noremoteip,
      "Allow peer to have no IP address", 1 },

    { "nosendip", o_bool, &ipcp_wantoptions[0].neg_addr,
      "Don't send our IP address to peer", OPT_A2CLR,
      &ipcp_wantoptions[0].old_addrs},

    { "IP addresses", o_wild, (void *) &setipaddr,
      "set local and remote IP addresses",
      OPT_NOARG | OPT_A2PRINTER, (void *) &printipaddr },

    { NULL }
};
#endif /* PPP_OPTIONS */

/*
 * Protocol entry points from main code.
 */
static void ipcp_init(ppp_pcb *pcb);
static void ipcp_open(ppp_pcb *pcb);
static void ipcp_close(ppp_pcb *pcb, const char *reason);
static void ipcp_lowerup(ppp_pcb *pcb);
static void ipcp_lowerdown(ppp_pcb *pcb);
static void ipcp_input(ppp_pcb *pcb, u_char *p, int len);
static void ipcp_protrej(ppp_pcb *pcb);
#if PRINTPKT_SUPPORT
static int ipcp_printpkt(const u_char *p, int plen,
		void (*printer) (void *, const char *, ...), void *arg);
#endif /* PRINTPKT_SUPPORT */
#if PPP_OPTIONS
static void ip_check_options (void);
#endif /* PPP_OPTIONS */
#if DEMAND_SUPPORT
static int  ip_demand_conf (int);
static int  ip_active_pkt (u_char *, int);
#endif /* DEMAND_SUPPORT */
#if 0 /* UNUSED */
static void create_resolv (u32_t, u32_t);
#endif /* UNUSED */

const struct protent ipcp_protent = {
    PPP_IPCP,
    ipcp_init,
    ipcp_input,
    ipcp_protrej,
    ipcp_lowerup,
    ipcp_lowerdown,
    ipcp_open,
    ipcp_close,
#if PRINTPKT_SUPPORT
    ipcp_printpkt,
#endif /* PRINTPKT_SUPPORT */
#if PPP_DATAINPUT
    NULL,
#endif /* PPP_DATAINPUT */
#if PRINTPKT_SUPPORT
    "IPCP",
    "IP",
#endif /* PRINTPKT_SUPPORT */
#if PPP_OPTIONS
    ipcp_option_list,
    ip_check_options,
#endif /* PPP_OPTIONS */
#if DEMAND_SUPPORT
    ip_demand_conf,
    ip_active_pkt
#endif /* DEMAND_SUPPORT */
};

static void ipcp_clear_addrs(ppp_pcb *pcb, u32_t ouraddr, u32_t hisaddr, u8_t replacedefaultroute);

/*
 * Lengths of configuration options.
 */
#define CILEN_VOID	2
#define CILEN_COMPRESS	4	/* min length for compression protocol opt. */
#define CILEN_VJ	6	/* length for RFC1332 Van-Jacobson opt. */
#define CILEN_ADDR	6	/* new-style single address option */
#define CILEN_ADDRS	10	/* old-style dual address option */


#define CODENAME(x)	((x) == CONFACK ? "ACK" : \
			 (x) == CONFNAK ? "NAK" : "REJ")

#if 0 /* UNUSED, already defined by lwIP */
/*
 * Make a string representation of a network IP address.
 */
char *
ip_ntoa(ipaddr)
u32_t ipaddr;
{
    static char b[64];

    slprintf(b, sizeof(b), "%I", ipaddr);
    return b;
}
#endif /* UNUSED, already defined by lwIP */

/*
 * Option parsing.
 */
#if PPP_OPTIONS
/*
 * setvjslots - set maximum number of connection slots for VJ compression
 */
static int
setvjslots(argv)
    char **argv;
{
    int value;

    if (!int_option(*argv, &value))
	return 0;

    if (value < 2 || value > 16) {
	option_error("vj-max-slots value must be between 2 and 16");
	return 0;
    }
    ipcp_wantoptions [0].maxslotindex =
        ipcp_allowoptions[0].maxslotindex = value - 1;
    slprintf(vj_value, sizeof(vj_value), "%d", value);
    return 1;
}

/*
 * setdnsaddr - set the dns address(es)
 */
static int
setdnsaddr(argv)
    char **argv;
{
    u32_t dns;
    struct hostent *hp;

    dns = inet_addr(*argv);
    if (dns == (u32_t) -1) {
	if ((hp = gethostbyname(*argv)) == NULL) {
	    option_error("invalid address parameter '%s' for ms-dns option",
			 *argv);
	    return 0;
	}
	dns = *(u32_t *)hp->h_addr;
    }

    /* We take the last 2 values given, the 2nd-last as the primary
       and the last as the secondary.  If only one is given it
       becomes both primary and secondary. */
    if (ipcp_allowoptions[0].dnsaddr[1] == 0)
	ipcp_allowoptions[0].dnsaddr[0] = dns;
    else
	ipcp_allowoptions[0].dnsaddr[0] = ipcp_allowoptions[0].dnsaddr[1];

    /* always set the secondary address value. */
    ipcp_allowoptions[0].dnsaddr[1] = dns;

    return (1);
}

/*
 * setwinsaddr - set the wins address(es)
 * This is primrarly used with the Samba package under UNIX or for pointing
 * the caller to the existing WINS server on a Windows NT platform.
 */
static int
setwinsaddr(argv)
    char **argv;
{
    u32_t wins;
    struct hostent *hp;

    wins = inet_addr(*argv);
    if (wins == (u32_t) -1) {
	if ((hp = gethostbyname(*argv)) == NULL) {
	    option_error("invalid address parameter '%s' for ms-wins option",
			 *argv);
	    return 0;
	}
	wins = *(u32_t *)hp->h_addr;
    }

    /* We take the last 2 values given, the 2nd-last as the primary
       and the last as the secondary.  If only one is given it
       becomes both primary and secondary. */
    if (ipcp_allowoptions[0].winsaddr[1] == 0)
	ipcp_allowoptions[0].winsaddr[0] = wins;
    else
	ipcp_allowoptions[0].winsaddr[0] = ipcp_allowoptions[0].winsaddr[1];

    /* always set the secondary address value. */
    ipcp_allowoptions[0].winsaddr[1] = wins;

    return (1);
}

/*
 * setipaddr - Set the IP address
 * If doit is 0, the call is to check whether this option is
 * potentially an IP address specification.
 * Not static so that plugins can call it to set the addresses
 */
int
setipaddr(arg, argv, doit)
    char *arg;
    char **argv;
    int doit;
{
    struct hostent *hp;
    char *colon;
    u32_t local, remote;
    ipcp_options *wo = &ipcp_wantoptions[0];
    static int prio_local = 0, prio_remote = 0;

    /*
     * IP address pair separated by ":".
     */
    if ((colon = strchr(arg, ':')) == NULL)
	return 0;
    if (!doit)
	return 1;
  
    /*
     * If colon first character, then no local addr.
     */
    if (colon != arg && option_priority >= prio_local) {
	*colon = '\0';
	if ((local = inet_addr(arg)) == (u32_t) -1) {
	    if ((hp = gethostbyname(arg)) == NULL) {
		option_error("unknown host: %s", arg);
		return 0;
	    }
	    local = *(u32_t *)hp->h_addr;
	}
	if (bad_ip_adrs(local)) {
	    option_error("bad local IP address %s", ip_ntoa(local));
	    return 0;
	}
	if (local != 0)
	    wo->ouraddr = local;
	*colon = ':';
	prio_local = option_priority;
    }
  
    /*
     * If colon last character, then no remote addr.
     */
    if (*++colon != '\0' && option_priority >= prio_remote) {
	if ((remote = inet_addr(colon)) == (u32_t) -1) {
	    if ((hp = gethostbyname(colon)) == NULL) {
		option_error("unknown host: %s", colon);
		return 0;
	    }
	    remote = *(u32_t *)hp->h_addr;
	    if (remote_name[0] == 0)
		strlcpy(remote_name, colon, sizeof(remote_name));
	}
	if (bad_ip_adrs(remote)) {
	    option_error("bad remote IP address %s", ip_ntoa(remote));
	    return 0;
	}
	if (remote != 0)
	    wo->hisaddr = remote;
	prio_remote = option_priority;
    }

    return 1;
}

static void
printipaddr(opt, printer, arg)
    option_t *opt;
    void (*printer) (void *, char *, ...);
    void *arg;
{
	ipcp_options *wo = &ipcp_wantoptions[0];

	if (wo->ouraddr != 0)
		printer(arg, "%I", wo->ouraddr);
	printer(arg, ":");
	if (wo->hisaddr != 0)
		printer(arg, "%I", wo->hisaddr);
}

/*
 * setnetmask - set the netmask to be used on the interface.
 */
static int
setnetmask(argv)
    char **argv;
{
    u32_t mask;
    int n;
    char *p;

    /*
     * Unfortunately, if we use inet_addr, we can't tell whether
     * a result of all 1s is an error or a valid 255.255.255.255.
     */
    p = *argv;
    n = parse_dotted_ip(p, &mask);

    mask = lwip_htonl(mask);

    if (n == 0 || p[n] != 0 || (netmask & ~mask) != 0) {
	option_error("invalid netmask value '%s'", *argv);
	return 0;
    }

    netmask = mask;
    slprintf(netmask_str, sizeof(netmask_str), "%I", mask);

    return (1);
}

int
parse_dotted_ip(p, vp)
    char *p;
    u32_t *vp;
{
    int n;
    u32_t v, b;
    char *endp, *p0 = p;

    v = 0;
    for (n = 3;; --n) {
	b = strtoul(p, &endp, 0);
	if (endp == p)
	    return 0;
	if (b > 255) {
	    if (n < 3)
		return 0;
	    /* accept e.g. 0xffffff00 */
	    *vp = b;
	    return endp - p0;
	}
	v |= b << (n * 8);
	p = endp;
	if (n == 0)
	    break;
	if (*p != '.')
	    return 0;
	++p;
    }
    *vp = v;
    return p - p0;
}
#endif /* PPP_OPTIONS */

/*
 * ipcp_init - Initialize IPCP.
 */
static void ipcp_init(ppp_pcb *pcb) {
    fsm *f = &pcb->ipcp_fsm;

    ipcp_options *wo = &pcb->ipcp_wantoptions;
    ipcp_options *ao = &pcb->ipcp_allowoptions;

    f->pcb = pcb;
    f->protocol = PPP_IPCP;
    f->callbacks = &ipcp_callbacks;
    fsm_init(f);

    /*
     * Some 3G modems use repeated IPCP NAKs as a way of stalling
     * until they can contact a server on the network, so we increase
     * the default number of NAKs we accept before we start treating
     * them as rejects.
     */
    f->maxnakloops = 100;

#if 0 /* Not necessary, everything is cleared in ppp_new() */
    memset(wo, 0, sizeof(*wo));
    memset(ao, 0, sizeof(*ao));
#endif /* 0 */

    wo->neg_addr = wo->old_addrs = 1;
#if VJ_SUPPORT
    wo->neg_vj = 1;
    wo->vj_protocol = IPCP_VJ_COMP;
    wo->maxslotindex = MAX_STATES - 1; /* really max index */
    wo->cflag = 1;
#endif /* VJ_SUPPORT */

#if 0 /* UNUSED */
    /* wanting default route by default */
    wo->default_route = 1;
#endif /* UNUSED */

    ao->neg_addr = ao->old_addrs = 1;
#if VJ_SUPPORT
    /* max slots and slot-id compression are currently hardwired in */
    /* ppp_if.c to 16 and 1, this needs to be changed (among other */
    /* things) gmc */

    ao->neg_vj = 1;
    ao->maxslotindex = MAX_STATES - 1;
    ao->cflag = 1;
#endif /* #if VJ_SUPPORT */

#if 0 /* UNUSED */
    /*
     * XXX These control whether the user may use the proxyarp
     * and defaultroute options.
     */
    ao->proxy_arp = 1;
    ao->default_route = 1;
#endif /* UNUSED */
}


/*
 * ipcp_open - IPCP is allowed to come up.
 */
static void ipcp_open(ppp_pcb *pcb) {
    fsm *f = &pcb->ipcp_fsm;
    fsm_open(f);
    pcb->ipcp_is_open = 1;
}


/*
 * ipcp_close - Take IPCP down.
 */
static void ipcp_close(ppp_pcb *pcb, const char *reason) {
    fsm *f = &pcb->ipcp_fsm;
    fsm_close(f, reason);
}


/*
 * ipcp_lowerup - The lower layer is up.
 */
static void ipcp_lowerup(ppp_pcb *pcb) {
    fsm *f = &pcb->ipcp_fsm;
    fsm_lowerup(f);
}


/*
 * ipcp_lowerdown - The lower layer is down.
 */
static void ipcp_lowerdown(ppp_pcb *pcb) {
    fsm *f = &pcb->ipcp_fsm;
    fsm_lowerdown(f);
}


/*
 * ipcp_input - Input IPCP packet.
 */
static void ipcp_input(ppp_pcb *pcb, u_char *p, int len) {
    fsm *f = &pcb->ipcp_fsm;
    fsm_input(f, p, len);
}


/*
 * ipcp_protrej - A Protocol-Reject was received for IPCP.
 *
 * Pretend the lower layer went down, so we shut up.
 */
static void ipcp_protrej(ppp_pcb *pcb) {
    fsm *f = &pcb->ipcp_fsm;
    fsm_lowerdown(f);
}


/*
 * ipcp_resetci - Reset our CI.
 * Called by fsm_sconfreq, Send Configure Request.
 */
static void ipcp_resetci(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ipcp_options *wo = &pcb->ipcp_wantoptions;
    ipcp_options *go = &pcb->ipcp_gotoptions;
    ipcp_options *ao = &pcb->ipcp_allowoptions;

    wo->req_addr = (wo->neg_addr || wo->old_addrs) &&
	(ao->neg_addr || ao->old_addrs);
    if (wo->ouraddr == 0)
	wo->accept_local = 1;
    if (wo->hisaddr == 0)
	wo->accept_remote = 1;
#if LWIP_DNS
    wo->req_dns1 = wo->req_dns2 = pcb->settings.usepeerdns;	/* Request DNS addresses from the peer */
#endif /* LWIP_DNS */
    *go = *wo;
    if (!pcb->ask_for_local)
	go->ouraddr = 0;
#if 0 /* UNUSED */
    if (ip_choose_hook) {
	ip_choose_hook(&wo->hisaddr);
	if (wo->hisaddr) {
	    wo->accept_remote = 0;
	}
    }
#endif /* UNUSED */
    BZERO(&pcb->ipcp_hisoptions, sizeof(ipcp_options));
}


/*
 * ipcp_cilen - Return length of our CI.
 * Called by fsm_sconfreq, Send Configure Request.
 */
static int ipcp_cilen(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ipcp_options *go = &pcb->ipcp_gotoptions;
#if VJ_SUPPORT
    ipcp_options *wo = &pcb->ipcp_wantoptions;
#endif /* VJ_SUPPORT */
    ipcp_options *ho = &pcb->ipcp_hisoptions;

#define LENCIADDRS(neg)		(neg ? CILEN_ADDRS : 0)
#if VJ_SUPPORT
#define LENCIVJ(neg, old)	(neg ? (old? CILEN_COMPRESS : CILEN_VJ) : 0)
#endif /* VJ_SUPPORT */
#define LENCIADDR(neg)		(neg ? CILEN_ADDR : 0)
#if LWIP_DNS
#define LENCIDNS(neg)		LENCIADDR(neg)
#endif /* LWIP_DNS */
#if 0 /* UNUSED - WINS */
#define LENCIWINS(neg)		LENCIADDR(neg)
#endif /* UNUSED - WINS */

    /*
     * First see if we want to change our options to the old
     * forms because we have received old forms from the peer.
     */
    if (go->neg_addr && go->old_addrs && !ho->neg_addr && ho->old_addrs)
	go->neg_addr = 0;

#if VJ_SUPPORT
    if (wo->neg_vj && !go->neg_vj && !go->old_vj) {
	/* try an older style of VJ negotiation */
	/* use the old style only if the peer did */
	if (ho->neg_vj && ho->old_vj) {
	    go->neg_vj = 1;
	    go->old_vj = 1;
	    go->vj_protocol = ho->vj_protocol;
	}
    }
#endif /* VJ_SUPPORT */

    return (LENCIADDRS(!go->neg_addr && go->old_addrs) +
#if VJ_SUPPORT
	    LENCIVJ(go->neg_vj, go->old_vj) +
#endif /* VJ_SUPPORT */
	    LENCIADDR(go->neg_addr) +
#if LWIP_DNS
	    LENCIDNS(go->req_dns1) +
	    LENCIDNS(go->req_dns2) +
#endif /* LWIP_DNS */
#if 0 /* UNUSED - WINS */
	    LENCIWINS(go->winsaddr[0]) +
	    LENCIWINS(go->winsaddr[1]) +
#endif /* UNUSED - WINS */
	    0);
}


/*
 * ipcp_addci - Add our desired CIs to a packet.
 * Called by fsm_sconfreq, Send Configure Request.
 */
static void ipcp_addci(fsm *f, u_char *ucp, int *lenp) {
    ppp_pcb *pcb = f->pcb;
    ipcp_options *go = &pcb->ipcp_gotoptions;
    int len = *lenp;

#define ADDCIADDRS(opt, neg, val1, val2) \
    if (neg) { \
	if (len >= CILEN_ADDRS) { \
	    u32_t l; \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(CILEN_ADDRS, ucp); \
	    l = lwip_ntohl(val1); \
	    PUTLONG(l, ucp); \
	    l = lwip_ntohl(val2); \
	    PUTLONG(l, ucp); \
	    len -= CILEN_ADDRS; \
	} else \
	    go->old_addrs = 0; \
    }

#if VJ_SUPPORT
#define ADDCIVJ(opt, neg, val, old, maxslotindex, cflag) \
    if (neg) { \
	int vjlen = old? CILEN_COMPRESS : CILEN_VJ; \
	if (len >= vjlen) { \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(vjlen, ucp); \
	    PUTSHORT(val, ucp); \
	    if (!old) { \
		PUTCHAR(maxslotindex, ucp); \
		PUTCHAR(cflag, ucp); \
	    } \
	    len -= vjlen; \
	} else \
	    neg = 0; \
    }
#endif /* VJ_SUPPORT */

#define ADDCIADDR(opt, neg, val) \
    if (neg) { \
	if (len >= CILEN_ADDR) { \
	    u32_t l; \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(CILEN_ADDR, ucp); \
	    l = lwip_ntohl(val); \
	    PUTLONG(l, ucp); \
	    len -= CILEN_ADDR; \
	} else \
	    neg = 0; \
    }

#if LWIP_DNS
#define ADDCIDNS(opt, neg, addr) \
    if (neg) { \
	if (len >= CILEN_ADDR) { \
	    u32_t l; \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(CILEN_ADDR, ucp); \
	    l = lwip_ntohl(addr); \
	    PUTLONG(l, ucp); \
	    len -= CILEN_ADDR; \
	} else \
	    neg = 0; \
    }
#endif /* LWIP_DNS */

#if 0 /* UNUSED - WINS */
#define ADDCIWINS(opt, addr) \
    if (addr) { \
	if (len >= CILEN_ADDR) { \
	    u32_t l; \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(CILEN_ADDR, ucp); \
	    l = lwip_ntohl(addr); \
	    PUTLONG(l, ucp); \
	    len -= CILEN_ADDR; \
	} else \
	    addr = 0; \
    }
#endif /* UNUSED - WINS */

    ADDCIADDRS(CI_ADDRS, !go->neg_addr && go->old_addrs, go->ouraddr,
	       go->hisaddr);

#if VJ_SUPPORT
    ADDCIVJ(CI_COMPRESSTYPE, go->neg_vj, go->vj_protocol, go->old_vj,
	    go->maxslotindex, go->cflag);
#endif /* VJ_SUPPORT */

    ADDCIADDR(CI_ADDR, go->neg_addr, go->ouraddr);

#if LWIP_DNS
    ADDCIDNS(CI_MS_DNS1, go->req_dns1, go->dnsaddr[0]);

    ADDCIDNS(CI_MS_DNS2, go->req_dns2, go->dnsaddr[1]);
#endif /* LWIP_DNS */

#if 0 /* UNUSED - WINS */
    ADDCIWINS(CI_MS_WINS1, go->winsaddr[0]);

    ADDCIWINS(CI_MS_WINS2, go->winsaddr[1]);
#endif /* UNUSED - WINS */
    
    *lenp -= len;
}


/*
 * ipcp_ackci - Ack our CIs.
 * Called by fsm_rconfack, Receive Configure ACK.
 *
 * Returns:
 *	0 - Ack was bad.
 *	1 - Ack was good.
 */
static int ipcp_ackci(fsm *f, u_char *p, int len) {
    ppp_pcb *pcb = f->pcb;
    ipcp_options *go = &pcb->ipcp_gotoptions;
    u_short cilen, citype;
    u32_t cilong;
#if VJ_SUPPORT
    u_short cishort;
    u_char cimaxslotindex, cicflag;
#endif /* VJ_SUPPORT */

    /*
     * CIs must be in exactly the same order that we sent...
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */

#define ACKCIADDRS(opt, neg, val1, val2) \
    if (neg) { \
	u32_t l; \
	if ((len -= CILEN_ADDRS) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_ADDRS || \
	    citype != opt) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	if (val1 != cilong) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	if (val2 != cilong) \
	    goto bad; \
    }

#if VJ_SUPPORT
#define ACKCIVJ(opt, neg, val, old, maxslotindex, cflag) \
    if (neg) { \
	int vjlen = old? CILEN_COMPRESS : CILEN_VJ; \
	if ((len -= vjlen) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != vjlen || \
	    citype != opt)  \
	    goto bad; \
	GETSHORT(cishort, p); \
	if (cishort != val) \
	    goto bad; \
	if (!old) { \
	    GETCHAR(cimaxslotindex, p); \
	    if (cimaxslotindex != maxslotindex) \
		goto bad; \
	    GETCHAR(cicflag, p); \
	    if (cicflag != cflag) \
		goto bad; \
	} \
    }
#endif /* VJ_SUPPORT */

#define ACKCIADDR(opt, neg, val) \
    if (neg) { \
	u32_t l; \
	if ((len -= CILEN_ADDR) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_ADDR || \
	    citype != opt) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	if (val != cilong) \
	    goto bad; \
    }

#if LWIP_DNS
#define ACKCIDNS(opt, neg, addr) \
    if (neg) { \
	u32_t l; \
	if ((len -= CILEN_ADDR) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_ADDR || citype != opt) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	if (addr != cilong) \
	    goto bad; \
    }
#endif /* LWIP_DNS */

#if 0 /* UNUSED - WINS */
#define ACKCIWINS(opt, addr) \
    if (addr) { \
	u32_t l; \
	if ((len -= CILEN_ADDR) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_ADDR || citype != opt) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	if (addr != cilong) \
	    goto bad; \
    }
#endif /* UNUSED - WINS */

    ACKCIADDRS(CI_ADDRS, !go->neg_addr && go->old_addrs, go->ouraddr,
	       go->hisaddr);

#if VJ_SUPPORT
    ACKCIVJ(CI_COMPRESSTYPE, go->neg_vj, go->vj_protocol, go->old_vj,
	    go->maxslotindex, go->cflag);
#endif /* VJ_SUPPORT */

    ACKCIADDR(CI_ADDR, go->neg_addr, go->ouraddr);

#if LWIP_DNS
    ACKCIDNS(CI_MS_DNS1, go->req_dns1, go->dnsaddr[0]);

    ACKCIDNS(CI_MS_DNS2, go->req_dns2, go->dnsaddr[1]);
#endif /* LWIP_DNS */

#if 0 /* UNUSED - WINS */
    ACKCIWINS(CI_MS_WINS1, go->winsaddr[0]);

    ACKCIWINS(CI_MS_WINS2, go->winsaddr[1]);
#endif /* UNUSED - WINS */

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
	goto bad;
    return (1);

bad:
    IPCPDEBUG(("ipcp_ackci: received bad Ack!"));
    return (0);
}

/*
 * ipcp_nakci - Peer has sent a NAK for some of our CIs.
 * This should not modify any state if the Nak is bad
 * or if IPCP is in the OPENED state.
 * Calback from fsm_rconfnakrej - Receive Configure-Nak or Configure-Reject.
 *
 * Returns:
 *	0 - Nak was bad.
 *	1 - Nak was good.
 */
static int ipcp_nakci(fsm *f, u_char *p, int len, int treat_as_reject) {
    ppp_pcb *pcb = f->pcb;
    ipcp_options *go = &pcb->ipcp_gotoptions;
    u_char citype, cilen, *next;
#if VJ_SUPPORT
    u_char cimaxslotindex, cicflag;
    u_short cishort;
#endif /* VJ_SUPPORT */
    u32_t ciaddr1, ciaddr2, l;
#if LWIP_DNS
    u32_t cidnsaddr;
#endif /* LWIP_DNS */
    ipcp_options no;		/* options we've seen Naks for */
    ipcp_options try_;		/* options to request next time */

    BZERO(&no, sizeof(no));
    try_ = *go;

    /*
     * Any Nak'd CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define NAKCIADDRS(opt, neg, code) \
    if ((neg) && \
	(cilen = p[1]) == CILEN_ADDRS && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	ciaddr1 = lwip_htonl(l); \
	GETLONG(l, p); \
	ciaddr2 = lwip_htonl(l); \
	no.old_addrs = 1; \
	code \
    }

#if VJ_SUPPORT
#define NAKCIVJ(opt, neg, code) \
    if (go->neg && \
	((cilen = p[1]) == CILEN_COMPRESS || cilen == CILEN_VJ) && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	no.neg = 1; \
        code \
    }
#endif /* VJ_SUPPORT */

#define NAKCIADDR(opt, neg, code) \
    if (go->neg && \
	(cilen = p[1]) == CILEN_ADDR && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	ciaddr1 = lwip_htonl(l); \
	no.neg = 1; \
	code \
    }

#if LWIP_DNS
#define NAKCIDNS(opt, neg, code) \
    if (go->neg && \
	((cilen = p[1]) == CILEN_ADDR) && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	cidnsaddr = lwip_htonl(l); \
	no.neg = 1; \
	code \
    }
#endif /* LWIP_DNS */

    /*
     * Accept the peer's idea of {our,his} address, if different
     * from our idea, only if the accept_{local,remote} flag is set.
     */
    NAKCIADDRS(CI_ADDRS, !go->neg_addr && go->old_addrs,
	       if (treat_as_reject) {
		   try_.old_addrs = 0;
	       } else {
		   if (go->accept_local && ciaddr1) {
		       /* take his idea of our address */
		       try_.ouraddr = ciaddr1;
		   }
		   if (go->accept_remote && ciaddr2) {
		       /* take his idea of his address */
		       try_.hisaddr = ciaddr2;
		   }
	       }
	);

#if VJ_SUPPORT
    /*
     * Accept the peer's value of maxslotindex provided that it
     * is less than what we asked for.  Turn off slot-ID compression
     * if the peer wants.  Send old-style compress-type option if
     * the peer wants.
     */
    NAKCIVJ(CI_COMPRESSTYPE, neg_vj,
	    if (treat_as_reject) {
		try_.neg_vj = 0;
	    } else if (cilen == CILEN_VJ) {
		GETCHAR(cimaxslotindex, p);
		GETCHAR(cicflag, p);
		if (cishort == IPCP_VJ_COMP) {
		    try_.old_vj = 0;
		    if (cimaxslotindex < go->maxslotindex)
			try_.maxslotindex = cimaxslotindex;
		    if (!cicflag)
			try_.cflag = 0;
		} else {
		    try_.neg_vj = 0;
		}
	    } else {
		if (cishort == IPCP_VJ_COMP || cishort == IPCP_VJ_COMP_OLD) {
		    try_.old_vj = 1;
		    try_.vj_protocol = cishort;
		} else {
		    try_.neg_vj = 0;
		}
	    }
	    );
#endif /* VJ_SUPPORT */

    NAKCIADDR(CI_ADDR, neg_addr,
	      if (treat_as_reject) {
		  try_.neg_addr = 0;
		  try_.old_addrs = 0;
	      } else if (go->accept_local && ciaddr1) {
		  /* take his idea of our address */
		  try_.ouraddr = ciaddr1;
	      }
	      );

#if LWIP_DNS
    NAKCIDNS(CI_MS_DNS1, req_dns1,
	     if (treat_as_reject) {
		 try_.req_dns1 = 0;
	     } else {
		 try_.dnsaddr[0] = cidnsaddr;
	     }
	     );

    NAKCIDNS(CI_MS_DNS2, req_dns2,
	     if (treat_as_reject) {
		 try_.req_dns2 = 0;
	     } else {
		 try_.dnsaddr[1] = cidnsaddr;
	     }
	     );
#endif /* #if LWIP_DNS */

    /*
     * There may be remaining CIs, if the peer is requesting negotiation
     * on an option that we didn't include in our request packet.
     * If they want to negotiate about IP addresses, we comply.
     * If they want us to ask for compression, we refuse.
     * If they want us to ask for ms-dns, we do that, since some
     * peers get huffy if we don't.
     */
    while (len >= CILEN_VOID) {
	GETCHAR(citype, p);
	GETCHAR(cilen, p);
	if ( cilen < CILEN_VOID || (len -= cilen) < 0 )
	    goto bad;
	next = p + cilen - 2;

	switch (citype) {
#if VJ_SUPPORT
	case CI_COMPRESSTYPE:
	    if (go->neg_vj || no.neg_vj ||
		(cilen != CILEN_VJ && cilen != CILEN_COMPRESS))
		goto bad;
	    no.neg_vj = 1;
	    break;
#endif /* VJ_SUPPORT */
	case CI_ADDRS:
	    if ((!go->neg_addr && go->old_addrs) || no.old_addrs
		|| cilen != CILEN_ADDRS)
		goto bad;
	    try_.neg_addr = 0;
	    GETLONG(l, p);
	    ciaddr1 = lwip_htonl(l);
	    if (ciaddr1 && go->accept_local)
		try_.ouraddr = ciaddr1;
	    GETLONG(l, p);
	    ciaddr2 = lwip_htonl(l);
	    if (ciaddr2 && go->accept_remote)
		try_.hisaddr = ciaddr2;
	    no.old_addrs = 1;
	    break;
	case CI_ADDR:
	    if (go->neg_addr || no.neg_addr || cilen != CILEN_ADDR)
		goto bad;
	    try_.old_addrs = 0;
	    GETLONG(l, p);
	    ciaddr1 = lwip_htonl(l);
	    if (ciaddr1 && go->accept_local)
		try_.ouraddr = ciaddr1;
	    if (try_.ouraddr != 0)
		try_.neg_addr = 1;
	    no.neg_addr = 1;
	    break;
#if LWIP_DNS
	case CI_MS_DNS1:
	    if (go->req_dns1 || no.req_dns1 || cilen != CILEN_ADDR)
		goto bad;
	    GETLONG(l, p);
	    try_.dnsaddr[0] = lwip_htonl(l);
	    try_.req_dns1 = 1;
	    no.req_dns1 = 1;
	    break;
	case CI_MS_DNS2:
	    if (go->req_dns2 || no.req_dns2 || cilen != CILEN_ADDR)
		goto bad;
	    GETLONG(l, p);
	    try_.dnsaddr[1] = lwip_htonl(l);
	    try_.req_dns2 = 1;
	    no.req_dns2 = 1;
	    break;
#endif /* LWIP_DNS */
#if 0 /* UNUSED - WINS */
	case CI_MS_WINS1:
	case CI_MS_WINS2:
	    if (cilen != CILEN_ADDR)
		goto bad;
	    GETLONG(l, p);
	    ciaddr1 = lwip_htonl(l);
	    if (ciaddr1)
		try_.winsaddr[citype == CI_MS_WINS2] = ciaddr1;
	    break;
#endif /* UNUSED - WINS */
	default:
	    break;
	}
	p = next;
    }

    /*
     * OK, the Nak is good.  Now we can update state.
     * If there are any remaining options, we ignore them.
     */
    if (f->state != PPP_FSM_OPENED)
	*go = try_;

    return 1;

bad:
    IPCPDEBUG(("ipcp_nakci: received bad Nak!"));
    return 0;
}


/*
 * ipcp_rejci - Reject some of our CIs.
 * Callback from fsm_rconfnakrej.
 */
static int ipcp_rejci(fsm *f, u_char *p, int len) {
    ppp_pcb *pcb = f->pcb;
    ipcp_options *go = &pcb->ipcp_gotoptions;
    u_char cilen;
#if VJ_SUPPORT
    u_char cimaxslotindex, ciflag;
    u_short cishort;
#endif /* VJ_SUPPORT */
    u32_t cilong;
    ipcp_options try_;		/* options to request next time */

    try_ = *go;
    /*
     * Any Rejected CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define REJCIADDRS(opt, neg, val1, val2) \
    if ((neg) && \
	(cilen = p[1]) == CILEN_ADDRS && \
	len >= cilen && \
	p[0] == opt) { \
	u32_t l; \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	/* Check rejected value. */ \
	if (cilong != val1) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	/* Check rejected value. */ \
	if (cilong != val2) \
	    goto bad; \
	try_.old_addrs = 0; \
    }

#if VJ_SUPPORT
#define REJCIVJ(opt, neg, val, old, maxslot, cflag) \
    if (go->neg && \
	p[1] == (old? CILEN_COMPRESS : CILEN_VJ) && \
	len >= p[1] && \
	p[0] == opt) { \
	len -= p[1]; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	/* Check rejected value. */  \
	if (cishort != val) \
	    goto bad; \
	if (!old) { \
	   GETCHAR(cimaxslotindex, p); \
	   if (cimaxslotindex != maxslot) \
	     goto bad; \
	   GETCHAR(ciflag, p); \
	   if (ciflag != cflag) \
	     goto bad; \
        } \
	try_.neg = 0; \
     }
#endif /* VJ_SUPPORT */

#define REJCIADDR(opt, neg, val) \
    if (go->neg && \
	(cilen = p[1]) == CILEN_ADDR && \
	len >= cilen && \
	p[0] == opt) { \
	u32_t l; \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	/* Check rejected value. */ \
	if (cilong != val) \
	    goto bad; \
	try_.neg = 0; \
    }

#if LWIP_DNS
#define REJCIDNS(opt, neg, dnsaddr) \
    if (go->neg && \
	((cilen = p[1]) == CILEN_ADDR) && \
	len >= cilen && \
	p[0] == opt) { \
	u32_t l; \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	/* Check rejected value. */ \
	if (cilong != dnsaddr) \
	    goto bad; \
	try_.neg = 0; \
    }
#endif /* LWIP_DNS */

#if 0 /* UNUSED - WINS */
#define REJCIWINS(opt, addr) \
    if (addr && \
	((cilen = p[1]) == CILEN_ADDR) && \
	len >= cilen && \
	p[0] == opt) { \
	u32_t l; \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	cilong = lwip_htonl(l); \
	/* Check rejected value. */ \
	if (cilong != addr) \
	    goto bad; \
	try_.winsaddr[opt == CI_MS_WINS2] = 0; \
    }
#endif /* UNUSED - WINS */

    REJCIADDRS(CI_ADDRS, !go->neg_addr && go->old_addrs,
	       go->ouraddr, go->hisaddr);

#if VJ_SUPPORT
    REJCIVJ(CI_COMPRESSTYPE, neg_vj, go->vj_protocol, go->old_vj,
	    go->maxslotindex, go->cflag);
#endif /* VJ_SUPPORT */

    REJCIADDR(CI_ADDR, neg_addr, go->ouraddr);

#if LWIP_DNS
    REJCIDNS(CI_MS_DNS1, req_dns1, go->dnsaddr[0]);

    REJCIDNS(CI_MS_DNS2, req_dns2, go->dnsaddr[1]);
#endif /* LWIP_DNS */

#if 0 /* UNUSED - WINS */
    REJCIWINS(CI_MS_WINS1, go->winsaddr[0]);

    REJCIWINS(CI_MS_WINS2, go->winsaddr[1]);
#endif /* UNUSED - WINS */

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
	goto bad;
    /*
     * Now we can update state.
     */
    if (f->state != PPP_FSM_OPENED)
	*go = try_;
    return 1;

bad:
    IPCPDEBUG(("ipcp_rejci: received bad Reject!"));
    return 0;
}


/*
 * ipcp_reqci - Check the peer's requested CIs and send appropriate response.
 * Callback from fsm_rconfreq, Receive Configure Request
 *
 * Returns: CONFACK, CONFNAK or CONFREJ and input packet modified
 * appropriately.  If reject_if_disagree is non-zero, doesn't return
 * CONFNAK; returns CONFREJ if it can't return CONFACK.
 *
 * inp = Requested CIs
 * len = Length of requested CIs
 */
static int ipcp_reqci(fsm *f, u_char *inp, int *len, int reject_if_disagree) {
    ppp_pcb *pcb = f->pcb;
    ipcp_options *wo = &pcb->ipcp_wantoptions;
    ipcp_options *ho = &pcb->ipcp_hisoptions;
    ipcp_options *ao = &pcb->ipcp_allowoptions;
    u_char *cip, *next;		/* Pointer to current and next CIs */
    u_short cilen, citype;	/* Parsed len, type */
#if VJ_SUPPORT
    u_short cishort;		/* Parsed short value */
#endif /* VJ_SUPPORT */
    u32_t tl, ciaddr1, ciaddr2;/* Parsed address values */
    int rc = CONFACK;		/* Final packet return code */
    int orc;			/* Individual option return code */
    u_char *p;			/* Pointer to next char to parse */
    u_char *ucp = inp;		/* Pointer to current output char */
    int l = *len;		/* Length left */
#if VJ_SUPPORT
    u_char maxslotindex, cflag;
#endif /* VJ_SUPPORT */
#if LWIP_DNS
    int d;
#endif /* LWIP_DNS */

    /*
     * Reset all his options.
     */
    BZERO(ho, sizeof(*ho));
    
    /*
     * Process all his options.
     */
    next = inp;
    while (l) {
	orc = CONFACK;			/* Assume success */
	cip = p = next;			/* Remember begining of CI */
	if (l < 2 ||			/* Not enough data for CI header or */
	    p[1] < 2 ||			/*  CI length too small or */
	    p[1] > l) {			/*  CI length too big? */
	    IPCPDEBUG(("ipcp_reqci: bad CI length!"));
	    orc = CONFREJ;		/* Reject bad CI */
	    cilen = l;			/* Reject till end of packet */
	    l = 0;			/* Don't loop again */
	    goto endswitch;
	}
	GETCHAR(citype, p);		/* Parse CI type */
	GETCHAR(cilen, p);		/* Parse CI length */
	l -= cilen;			/* Adjust remaining length */
	next += cilen;			/* Step to next CI */

	switch (citype) {		/* Check CI type */
	case CI_ADDRS:
	    if (!ao->old_addrs || ho->neg_addr ||
		cilen != CILEN_ADDRS) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }

	    /*
	     * If he has no address, or if we both have his address but
	     * disagree about it, then NAK it with our idea.
	     * In particular, if we don't know his address, but he does,
	     * then accept it.
	     */
	    GETLONG(tl, p);		/* Parse source address (his) */
	    ciaddr1 = lwip_htonl(tl);
	    if (ciaddr1 != wo->hisaddr
		&& (ciaddr1 == 0 || !wo->accept_remote)) {
		orc = CONFNAK;
		if (!reject_if_disagree) {
		    DECPTR(sizeof(u32_t), p);
		    tl = lwip_ntohl(wo->hisaddr);
		    PUTLONG(tl, p);
		}
	    } else if (ciaddr1 == 0 && wo->hisaddr == 0) {
		/*
		 * If neither we nor he knows his address, reject the option.
		 */
		orc = CONFREJ;
		wo->req_addr = 0;	/* don't NAK with 0.0.0.0 later */
		break;
	    }

	    /*
	     * If he doesn't know our address, or if we both have our address
	     * but disagree about it, then NAK it with our idea.
	     */
	    GETLONG(tl, p);		/* Parse desination address (ours) */
	    ciaddr2 = lwip_htonl(tl);
	    if (ciaddr2 != wo->ouraddr) {
		if (ciaddr2 == 0 || !wo->accept_local) {
		    orc = CONFNAK;
		    if (!reject_if_disagree) {
			DECPTR(sizeof(u32_t), p);
			tl = lwip_ntohl(wo->ouraddr);
			PUTLONG(tl, p);
		    }
		} else {
		    wo->ouraddr = ciaddr2;	/* accept peer's idea */
		}
	    }

	    ho->old_addrs = 1;
	    ho->hisaddr = ciaddr1;
	    ho->ouraddr = ciaddr2;
	    break;

	case CI_ADDR:
	    if (!ao->neg_addr || ho->old_addrs ||
		cilen != CILEN_ADDR) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }

	    /*
	     * If he has no address, or if we both have his address but
	     * disagree about it, then NAK it with our idea.
	     * In particular, if we don't know his address, but he does,
	     * then accept it.
	     */
	    GETLONG(tl, p);	/* Parse source address (his) */
	    ciaddr1 = lwip_htonl(tl);
	    if (ciaddr1 != wo->hisaddr
		&& (ciaddr1 == 0 || !wo->accept_remote)) {
		orc = CONFNAK;
		if (!reject_if_disagree) {
		    DECPTR(sizeof(u32_t), p);
		    tl = lwip_ntohl(wo->hisaddr);
		    PUTLONG(tl, p);
		}
	    } else if (ciaddr1 == 0 && wo->hisaddr == 0) {
		/*
		 * Don't ACK an address of 0.0.0.0 - reject it instead.
		 */
		orc = CONFREJ;
		wo->req_addr = 0;	/* don't NAK with 0.0.0.0 later */
		break;
	    }
	
	    ho->neg_addr = 1;
	    ho->hisaddr = ciaddr1;
	    break;

#if LWIP_DNS
	case CI_MS_DNS1:
	case CI_MS_DNS2:
	    /* Microsoft primary or secondary DNS request */
	    d = citype == CI_MS_DNS2;

	    /* If we do not have a DNS address then we cannot send it */
	    if (ao->dnsaddr[d] == 0 ||
		cilen != CILEN_ADDR) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }
	    GETLONG(tl, p);
	    if (lwip_htonl(tl) != ao->dnsaddr[d]) {
                DECPTR(sizeof(u32_t), p);
		tl = lwip_ntohl(ao->dnsaddr[d]);
		PUTLONG(tl, p);
		orc = CONFNAK;
            }
            break;
#endif /* LWIP_DNS */

#if 0 /* UNUSED - WINS */
	case CI_MS_WINS1:
	case CI_MS_WINS2:
	    /* Microsoft primary or secondary WINS request */
	    d = citype == CI_MS_WINS2;

	    /* If we do not have a DNS address then we cannot send it */
	    if (ao->winsaddr[d] == 0 ||
		cilen != CILEN_ADDR) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }
	    GETLONG(tl, p);
	    if (lwip_htonl(tl) != ao->winsaddr[d]) {
                DECPTR(sizeof(u32_t), p);
		tl = lwip_ntohl(ao->winsaddr[d]);
		PUTLONG(tl, p);
		orc = CONFNAK;
            }
            break;
#endif /* UNUSED - WINS */

#if VJ_SUPPORT
	case CI_COMPRESSTYPE:
	    if (!ao->neg_vj ||
		(cilen != CILEN_VJ && cilen != CILEN_COMPRESS)) {
		orc = CONFREJ;
		break;
	    }
	    GETSHORT(cishort, p);

	    if (!(cishort == IPCP_VJ_COMP ||
		  (cishort == IPCP_VJ_COMP_OLD && cilen == CILEN_COMPRESS))) {
		orc = CONFREJ;
		break;
	    }

	    ho->neg_vj = 1;
	    ho->vj_protocol = cishort;
	    if (cilen == CILEN_VJ) {
		GETCHAR(maxslotindex, p);
		if (maxslotindex > ao->maxslotindex) { 
		    orc = CONFNAK;
		    if (!reject_if_disagree){
			DECPTR(1, p);
			PUTCHAR(ao->maxslotindex, p);
		    }
		}
		GETCHAR(cflag, p);
		if (cflag && !ao->cflag) {
		    orc = CONFNAK;
		    if (!reject_if_disagree){
			DECPTR(1, p);
			PUTCHAR(wo->cflag, p);
		    }
		}
		ho->maxslotindex = maxslotindex;
		ho->cflag = cflag;
	    } else {
		ho->old_vj = 1;
		ho->maxslotindex = MAX_STATES - 1;
		ho->cflag = 1;
	    }
	    break;
#endif /* VJ_SUPPORT */

	default:
	    orc = CONFREJ;
	    break;
	}
endswitch:
	if (orc == CONFACK &&		/* Good CI */
	    rc != CONFACK)		/*  but prior CI wasnt? */
	    continue;			/* Don't send this one */

	if (orc == CONFNAK) {		/* Nak this CI? */
	    if (reject_if_disagree)	/* Getting fed up with sending NAKs? */
		orc = CONFREJ;		/* Get tough if so */
	    else {
		if (rc == CONFREJ)	/* Rejecting prior CI? */
		    continue;		/* Don't send this one */
		if (rc == CONFACK) {	/* Ack'd all prior CIs? */
		    rc = CONFNAK;	/* Not anymore... */
		    ucp = inp;		/* Backup */
		}
	    }
	}

	if (orc == CONFREJ &&		/* Reject this CI */
	    rc != CONFREJ) {		/*  but no prior ones? */
	    rc = CONFREJ;
	    ucp = inp;			/* Backup */
	}

	/* Need to move CI? */
	if (ucp != cip)
	    MEMCPY(ucp, cip, cilen);	/* Move it */

	/* Update output pointer */
	INCPTR(cilen, ucp);
    }

    /*
     * If we aren't rejecting this packet, and we want to negotiate
     * their address, and they didn't send their address, then we
     * send a NAK with a CI_ADDR option appended.  We assume the
     * input buffer is long enough that we can append the extra
     * option safely.
     */
    if (rc != CONFREJ && !ho->neg_addr && !ho->old_addrs &&
	wo->req_addr && !reject_if_disagree && !pcb->settings.noremoteip) {
	if (rc == CONFACK) {
	    rc = CONFNAK;
	    ucp = inp;			/* reset pointer */
	    wo->req_addr = 0;		/* don't ask again */
	}
	PUTCHAR(CI_ADDR, ucp);
	PUTCHAR(CILEN_ADDR, ucp);
	tl = lwip_ntohl(wo->hisaddr);
	PUTLONG(tl, ucp);
    }

    *len = ucp - inp;			/* Compute output length */
    IPCPDEBUG(("ipcp: returning Configure-%s", CODENAME(rc)));
    return (rc);			/* Return final code */
}


#if 0 /* UNUSED */
/*
 * ip_check_options - check that any IP-related options are OK,
 * and assign appropriate defaults.
 */
static void
ip_check_options()
{
    struct hostent *hp;
    u32_t local;
    ipcp_options *wo = &ipcp_wantoptions[0];

    /*
     * Default our local IP address based on our hostname.
     * If local IP address already given, don't bother.
     */
    if (wo->ouraddr == 0 && !disable_defaultip) {
	/*
	 * Look up our hostname (possibly with domain name appended)
	 * and take the first IP address as our local IP address.
	 * If there isn't an IP address for our hostname, too bad.
	 */
	wo->accept_local = 1;	/* don't insist on this default value */
	if ((hp = gethostbyname(hostname)) != NULL) {
	    local = *(u32_t *)hp->h_addr;
	    if (local != 0 && !bad_ip_adrs(local))
		wo->ouraddr = local;
	}
    }
    ask_for_local = wo->ouraddr != 0 || !disable_defaultip;
}
#endif /* UNUSED */

#if DEMAND_SUPPORT
/*
 * ip_demand_conf - configure the interface as though
 * IPCP were up, for use with dial-on-demand.
 */
static int
ip_demand_conf(u)
    int u;
{
    ppp_pcb *pcb = &ppp_pcb_list[u];
    ipcp_options *wo = &ipcp_wantoptions[u];

    if (wo->hisaddr == 0 && !pcb->settings.noremoteip) {
	/* make up an arbitrary address for the peer */
	wo->hisaddr = lwip_htonl(0x0a707070 + ifunit);
	wo->accept_remote = 1;
    }
    if (wo->ouraddr == 0) {
	/* make up an arbitrary address for us */
	wo->ouraddr = lwip_htonl(0x0a404040 + ifunit);
	wo->accept_local = 1;
	ask_for_local = 0;	/* don't tell the peer this address */
    }
    if (!sifaddr(pcb, wo->ouraddr, wo->hisaddr, get_mask(wo->ouraddr)))
	return 0;
    if (!sifup(pcb))
	return 0;
    if (!sifnpmode(pcb, PPP_IP, NPMODE_QUEUE))
	return 0;
#if 0 /* UNUSED */
    if (wo->default_route)
	if (sifdefaultroute(pcb, wo->ouraddr, wo->hisaddr,
		wo->replace_default_route))
	    default_route_set[u] = 1;
#endif /* UNUSED */
#if 0 /* UNUSED - PROXY ARP */
    if (wo->proxy_arp)
	if (sifproxyarp(pcb, wo->hisaddr))
	    proxy_arp_set[u] = 1;
#endif /* UNUSED - PROXY ARP */

    ppp_notice("local  IP address %I", wo->ouraddr);
    if (wo->hisaddr)
	ppp_notice("remote IP address %I", wo->hisaddr);

    return 1;
}
#endif /* DEMAND_SUPPORT */

/*
 * ipcp_up - IPCP has come UP.
 *
 * Configure the IP network interface appropriately and bring it up.
 */
static void ipcp_up(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    u32_t mask;
    ipcp_options *ho = &pcb->ipcp_hisoptions;
    ipcp_options *go = &pcb->ipcp_gotoptions;
    ipcp_options *wo = &pcb->ipcp_wantoptions;

    IPCPDEBUG(("ipcp: up"));

    /*
     * We must have a non-zero IP address for both ends of the link.
     */
    if (!ho->neg_addr && !ho->old_addrs)
	ho->hisaddr = wo->hisaddr;

    if (!(go->neg_addr || go->old_addrs) && (wo->neg_addr || wo->old_addrs)
	&& wo->ouraddr != 0) {
	ppp_error("Peer refused to agree to our IP address");
	ipcp_close(f->pcb, "Refused our IP address");
	return;
    }
    if (go->ouraddr == 0) {
	ppp_error("Could not determine local IP address");
	ipcp_close(f->pcb, "Could not determine local IP address");
	return;
    }
    if (ho->hisaddr == 0 && !pcb->settings.noremoteip) {
	ho->hisaddr = lwip_htonl(0x0a404040);
	ppp_warn("Could not determine remote IP address: defaulting to %I",
	     ho->hisaddr);
    }
#if 0 /* UNUSED */
    script_setenv("IPLOCAL", ip_ntoa(go->ouraddr), 0);
    if (ho->hisaddr != 0)
	script_setenv("IPREMOTE", ip_ntoa(ho->hisaddr), 1);
#endif /* UNUSED */

#if LWIP_DNS
    if (!go->req_dns1)
	    go->dnsaddr[0] = 0;
    if (!go->req_dns2)
	    go->dnsaddr[1] = 0;
#if 0 /* UNUSED */
    if (go->dnsaddr[0])
	script_setenv("DNS1", ip_ntoa(go->dnsaddr[0]), 0);
    if (go->dnsaddr[1])
	script_setenv("DNS2", ip_ntoa(go->dnsaddr[1]), 0);
#endif /* UNUSED */
    if (pcb->settings.usepeerdns && (go->dnsaddr[0] || go->dnsaddr[1])) {
	sdns(pcb, go->dnsaddr[0], go->dnsaddr[1]);
#if 0 /* UNUSED */
	script_setenv("USEPEERDNS", "1", 0);
	create_resolv(go->dnsaddr[0], go->dnsaddr[1]);
#endif /* UNUSED */
    }
#endif /* LWIP_DNS */

    /*
     * Check that the peer is allowed to use the IP address it wants.
     */
    if (ho->hisaddr != 0) {
	u32_t addr = lwip_ntohl(ho->hisaddr);
	if ((addr >> IP_CLASSA_NSHIFT) == IP_LOOPBACKNET
	    || IP_MULTICAST(addr) || IP_BADCLASS(addr)
	    /*
	     * For now, consider that PPP in server mode with peer required
	     * to authenticate must provide the peer IP address, reject any
	     * IP address wanted by peer different than the one we wanted.
	     */
#if PPP_SERVER && PPP_AUTH_SUPPORT
	    || (pcb->settings.auth_required && wo->hisaddr != ho->hisaddr)
#endif /* PPP_SERVER && PPP_AUTH_SUPPORT */
	    ) {
		ppp_error("Peer is not authorized to use remote address %I", ho->hisaddr);
		ipcp_close(pcb, "Unauthorized remote IP address");
		return;
	}
    }
#if 0 /* Unused */
    /* Upstream checking code */
    if (ho->hisaddr != 0 && !auth_ip_addr(f->unit, ho->hisaddr)) {
	ppp_error("Peer is not authorized to use remote address %I", ho->hisaddr);
	ipcp_close(f->unit, "Unauthorized remote IP address");
	return;
    }
#endif /* Unused */

#if VJ_SUPPORT
    /* set tcp compression */
    sifvjcomp(pcb, ho->neg_vj, ho->cflag, ho->maxslotindex);
#endif /* VJ_SUPPORT */

#if DEMAND_SUPPORT
    /*
     * If we are doing dial-on-demand, the interface is already
     * configured, so we put out any saved-up packets, then set the
     * interface to pass IP packets.
     */
    if (demand) {
	if (go->ouraddr != wo->ouraddr || ho->hisaddr != wo->hisaddr) {
	    ipcp_clear_addrs(f->unit, wo->ouraddr, wo->hisaddr,
				      wo->replace_default_route);
	    if (go->ouraddr != wo->ouraddr) {
		ppp_warn("Local IP address changed to %I", go->ouraddr);
		script_setenv("OLDIPLOCAL", ip_ntoa(wo->ouraddr), 0);
		wo->ouraddr = go->ouraddr;
	    } else
		script_unsetenv("OLDIPLOCAL");
	    if (ho->hisaddr != wo->hisaddr && wo->hisaddr != 0) {
		ppp_warn("Remote IP address changed to %I", ho->hisaddr);
		script_setenv("OLDIPREMOTE", ip_ntoa(wo->hisaddr), 0);
		wo->hisaddr = ho->hisaddr;
	    } else
		script_unsetenv("OLDIPREMOTE");

	    /* Set the interface to the new addresses */
	    mask = get_mask(go->ouraddr);
	    if (!sifaddr(pcb, go->ouraddr, ho->hisaddr, mask)) {
#if PPP_DEBUG
		ppp_warn("Interface configuration failed");
#endif /* PPP_DEBUG */
		ipcp_close(f->unit, "Interface configuration failed");
		return;
	    }

	    /* assign a default route through the interface if required */
	    if (ipcp_wantoptions[f->unit].default_route) 
		if (sifdefaultroute(pcb, go->ouraddr, ho->hisaddr,
			wo->replace_default_route))
		    default_route_set[f->unit] = 1;

#if 0 /* UNUSED - PROXY ARP */
	    /* Make a proxy ARP entry if requested. */
	    if (ho->hisaddr != 0 && ipcp_wantoptions[f->unit].proxy_arp)
		if (sifproxyarp(pcb, ho->hisaddr))
		    proxy_arp_set[f->unit] = 1;
#endif /* UNUSED - PROXY ARP */

	}
	demand_rexmit(PPP_IP,go->ouraddr);
	sifnpmode(pcb, PPP_IP, NPMODE_PASS);

    } else
#endif /* DEMAND_SUPPORT */
    {
	/*
	 * Set IP addresses and (if specified) netmask.
	 */
	mask = get_mask(go->ouraddr);

#if !(defined(SVR4) && (defined(SNI) || defined(__USLC__)))
	if (!sifaddr(pcb, go->ouraddr, ho->hisaddr, mask)) {
#if PPP_DEBUG
	    ppp_warn("Interface configuration failed");
#endif /* PPP_DEBUG */
	    ipcp_close(f->pcb, "Interface configuration failed");
	    return;
	}
#endif

	/* bring the interface up for IP */
	if (!sifup(pcb)) {
#if PPP_DEBUG
	    ppp_warn("Interface failed to come up");
#endif /* PPP_DEBUG */
	    ipcp_close(f->pcb, "Interface configuration failed");
	    return;
	}

#if (defined(SVR4) && (defined(SNI) || defined(__USLC__)))
	if (!sifaddr(pcb, go->ouraddr, ho->hisaddr, mask)) {
#if PPP_DEBUG
	    ppp_warn("Interface configuration failed");
#endif /* PPP_DEBUG */
	    ipcp_close(f->unit, "Interface configuration failed");
	    return;
	}
#endif
#if DEMAND_SUPPORT
	sifnpmode(pcb, PPP_IP, NPMODE_PASS);
#endif /* DEMAND_SUPPORT */

#if 0 /* UNUSED */
	/* assign a default route through the interface if required */
	if (wo->default_route)
	    if (sifdefaultroute(pcb, go->ouraddr, ho->hisaddr,
		    wo->replace_default_route))
		    pcb->default_route_set = 1;
#endif /* UNUSED */

#if 0 /* UNUSED - PROXY ARP */
	/* Make a proxy ARP entry if requested. */
	if (ho->hisaddr != 0 && wo->proxy_arp)
	    if (sifproxyarp(pcb, ho->hisaddr))
		pcb->proxy_arp_set = 1;
#endif /* UNUSED - PROXY ARP */

	wo->ouraddr = go->ouraddr;

	ppp_notice("local  IP address %I", go->ouraddr);
	if (ho->hisaddr != 0)
	    ppp_notice("remote IP address %I", ho->hisaddr);
#if LWIP_DNS
	if (go->dnsaddr[0])
	    ppp_notice("primary   DNS address %I", go->dnsaddr[0]);
	if (go->dnsaddr[1])
	    ppp_notice("secondary DNS address %I", go->dnsaddr[1]);
#endif /* LWIP_DNS */
    }

#if PPP_STATS_SUPPORT
    reset_link_stats(f->unit);
#endif /* PPP_STATS_SUPPORT */

    np_up(pcb, PPP_IP);
    pcb->ipcp_is_up = 1;

#if PPP_NOTIFY
    notify(ip_up_notifier, 0);
#endif /* PPP_NOTIFY */
#if 0 /* UNUSED */
    if (ip_up_hook)
	ip_up_hook();
#endif /* UNUSED */
}


/*
 * ipcp_down - IPCP has gone DOWN.
 *
 * Take the IP network interface down, clear its addresses
 * and delete routes through it.
 */
static void ipcp_down(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ipcp_options *ho = &pcb->ipcp_hisoptions;
    ipcp_options *go = &pcb->ipcp_gotoptions;

    IPCPDEBUG(("ipcp: down"));
#if PPP_STATS_SUPPORT
    /* XXX a bit IPv4-centric here, we only need to get the stats
     * before the interface is marked down. */
    /* XXX more correct: we must get the stats before running the notifiers,
     * at least for the radius plugin */
    update_link_stats(f->unit);
#endif /* PPP_STATS_SUPPORT */
#if PPP_NOTIFY
    notify(ip_down_notifier, 0);
#endif /* PPP_NOTIFY */
#if 0 /* UNUSED */
    if (ip_down_hook)
	ip_down_hook();
#endif /* UNUSED */
    if (pcb->ipcp_is_up) {
	pcb->ipcp_is_up = 0;
	np_down(pcb, PPP_IP);
    }
#if VJ_SUPPORT
    sifvjcomp(pcb, 0, 0, 0);
#endif /* VJ_SUPPORT */

#if PPP_STATS_SUPPORT
    print_link_stats(); /* _after_ running the notifiers and ip_down_hook(),
			 * because print_link_stats() sets link_stats_valid
			 * to 0 (zero) */
#endif /* PPP_STATS_SUPPORT */

#if DEMAND_SUPPORT
    /*
     * If we are doing dial-on-demand, set the interface
     * to queue up outgoing packets (for now).
     */
    if (demand) {
	sifnpmode(pcb, PPP_IP, NPMODE_QUEUE);
    } else
#endif /* DEMAND_SUPPORT */
    {
#if DEMAND_SUPPORT
	sifnpmode(pcb, PPP_IP, NPMODE_DROP);
#endif /* DEMAND_SUPPORT */
	sifdown(pcb);
	ipcp_clear_addrs(pcb, go->ouraddr,
			 ho->hisaddr, 0);
#if LWIP_DNS
	cdns(pcb, go->dnsaddr[0], go->dnsaddr[1]);
#endif /* LWIP_DNS */
    }
}


/*
 * ipcp_clear_addrs() - clear the interface addresses, routes,
 * proxy arp entries, etc.
 */
static void ipcp_clear_addrs(ppp_pcb *pcb, u32_t ouraddr, u32_t hisaddr, u8_t replacedefaultroute) {
    LWIP_UNUSED_ARG(replacedefaultroute);

#if 0 /* UNUSED - PROXY ARP */
    if (pcb->proxy_arp_set) {
	cifproxyarp(pcb, hisaddr);
	pcb->proxy_arp_set = 0;
    }
#endif /* UNUSED - PROXY ARP */
#if 0 /* UNUSED */
    /* If replacedefaultroute, sifdefaultroute will be called soon
     * with replacedefaultroute set and that will overwrite the current
     * default route. This is the case only when doing demand, otherwise
     * during demand, this cifdefaultroute would restore the old default
     * route which is not what we want in this case. In the non-demand
     * case, we'll delete the default route and restore the old if there
     * is one saved by an sifdefaultroute with replacedefaultroute.
     */
    if (!replacedefaultroute && pcb->default_route_set) {
	cifdefaultroute(pcb, ouraddr, hisaddr);
	pcb->default_route_set = 0;
    }
#endif /* UNUSED */
    cifaddr(pcb, ouraddr, hisaddr);
}


/*
 * ipcp_finished - possibly shut down the lower layers.
 */
static void ipcp_finished(fsm *f) {
	ppp_pcb *pcb = f->pcb;
	if (pcb->ipcp_is_open) {
		pcb->ipcp_is_open = 0;
		np_finished(pcb, PPP_IP);
	}
}


#if 0 /* UNUSED */
/*
 * create_resolv - create the replacement resolv.conf file
 */
static void
create_resolv(peerdns1, peerdns2)
    u32_t peerdns1, peerdns2;
{

}
#endif /* UNUSED */

#if PRINTPKT_SUPPORT
/*
 * ipcp_printpkt - print the contents of an IPCP packet.
 */
static const char* const ipcp_codenames[] = {
    "ConfReq", "ConfAck", "ConfNak", "ConfRej",
    "TermReq", "TermAck", "CodeRej"
};

static int ipcp_printpkt(const u_char *p, int plen,
		void (*printer) (void *, const char *, ...), void *arg) {
    int code, id, len, olen;
    const u_char *pstart, *optend;
#if VJ_SUPPORT
    u_short cishort;
#endif /* VJ_SUPPORT */
    u32_t cilong;

    if (plen < HEADERLEN)
	return 0;
    pstart = p;
    GETCHAR(code, p);
    GETCHAR(id, p);
    GETSHORT(len, p);
    if (len < HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= (int)LWIP_ARRAYSIZE(ipcp_codenames))
	printer(arg, " %s", ipcp_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code);
    printer(arg, " id=0x%x", id);
    len -= HEADERLEN;
    switch (code) {
    case CONFREQ:
    case CONFACK:
    case CONFNAK:
    case CONFREJ:
	/* print option list */
	while (len >= 2) {
	    GETCHAR(code, p);
	    GETCHAR(olen, p);
	    p -= 2;
	    if (olen < 2 || olen > len) {
		break;
	    }
	    printer(arg, " <");
	    len -= olen;
	    optend = p + olen;
	    switch (code) {
	    case CI_ADDRS:
		if (olen == CILEN_ADDRS) {
		    p += 2;
		    GETLONG(cilong, p);
		    printer(arg, "addrs %I", lwip_htonl(cilong));
		    GETLONG(cilong, p);
		    printer(arg, " %I", lwip_htonl(cilong));
		}
		break;
#if VJ_SUPPORT
	    case CI_COMPRESSTYPE:
		if (olen >= CILEN_COMPRESS) {
		    p += 2;
		    GETSHORT(cishort, p);
		    printer(arg, "compress ");
		    switch (cishort) {
		    case IPCP_VJ_COMP:
			printer(arg, "VJ");
			break;
		    case IPCP_VJ_COMP_OLD:
			printer(arg, "old-VJ");
			break;
		    default:
			printer(arg, "0x%x", cishort);
		    }
		}
		break;
#endif /* VJ_SUPPORT */
	    case CI_ADDR:
		if (olen == CILEN_ADDR) {
		    p += 2;
		    GETLONG(cilong, p);
		    printer(arg, "addr %I", lwip_htonl(cilong));
		}
		break;
#if LWIP_DNS
	    case CI_MS_DNS1:
	    case CI_MS_DNS2:
	        p += 2;
		GETLONG(cilong, p);
		printer(arg, "ms-dns%d %I", (code == CI_MS_DNS1? 1: 2),
			htonl(cilong));
		break;
#endif /* LWIP_DNS */
#if 0 /* UNUSED - WINS */
	    case CI_MS_WINS1:
	    case CI_MS_WINS2:
	        p += 2;
		GETLONG(cilong, p);
		printer(arg, "ms-wins %I", lwip_htonl(cilong));
		break;
#endif /* UNUSED - WINS */
	    default:
		break;
	    }
	    while (p < optend) {
		GETCHAR(code, p);
		printer(arg, " %.2x", code);
	    }
	    printer(arg, ">");
	}
	break;

    case TERMACK:
    case TERMREQ:
	if (len > 0 && *p >= ' ' && *p < 0x7f) {
	    printer(arg, " ");
	    ppp_print_string(p, len, printer, arg);
	    p += len;
	    len = 0;
	}
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

#if DEMAND_SUPPORT
/*
 * ip_active_pkt - see if this IP packet is worth bringing the link up for.
 * We don't bring the link up for IP fragments or for TCP FIN packets
 * with no data.
 */
#define IP_HDRLEN	20	/* bytes */
#define IP_OFFMASK	0x1fff
#ifndef IPPROTO_TCP
#define IPPROTO_TCP	6
#endif
#define TCP_HDRLEN	20
#define TH_FIN		0x01

/*
 * We use these macros because the IP header may be at an odd address,
 * and some compilers might use word loads to get th_off or ip_hl.
 */

#define net_short(x)	(((x)[0] << 8) + (x)[1])
#define get_iphl(x)	(((unsigned char *)(x))[0] & 0xF)
#define get_ipoff(x)	net_short((unsigned char *)(x) + 6)
#define get_ipproto(x)	(((unsigned char *)(x))[9])
#define get_tcpoff(x)	(((unsigned char *)(x))[12] >> 4)
#define get_tcpflags(x)	(((unsigned char *)(x))[13])

static int
ip_active_pkt(pkt, len)
    u_char *pkt;
    int len;
{
    u_char *tcp;
    int hlen;

    len -= PPP_HDRLEN;
    pkt += PPP_HDRLEN;
    if (len < IP_HDRLEN)
	return 0;
    if ((get_ipoff(pkt) & IP_OFFMASK) != 0)
	return 0;
    if (get_ipproto(pkt) != IPPROTO_TCP)
	return 1;
    hlen = get_iphl(pkt) * 4;
    if (len < hlen + TCP_HDRLEN)
	return 0;
    tcp = pkt + hlen;
    if ((get_tcpflags(tcp) & TH_FIN) != 0 && len == hlen + get_tcpoff(tcp) * 4)
	return 0;
    return 1;
}
#endif /* DEMAND_SUPPORT */

#endif /* PPP_SUPPORT && PPP_IPV4_SUPPORT */
