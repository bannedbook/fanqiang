/*****************************************************************************
* ppp.h - Network Point to Point Protocol header file.
*
* Copyright (c) 2003 by Marc Boucher, Services Informatiques (MBSI) inc.
* portions Copyright (c) 1997 Global Election Systems Inc.
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
* REVISION HISTORY
*
* 03-01-01 Marc Boucher <marc@mbsi.ca>
*   Ported to lwIP.
* 97-11-05 Guy Lancaster <glanca@gesn.com>, Global Election Systems Inc.
*   Original derived from BSD codes.
*****************************************************************************/
#ifndef LWIP_HDR_PPP_IMPL_H
#define LWIP_HDR_PPP_IMPL_H

#include "netif/ppp/ppp_opts.h"

#if PPP_SUPPORT /* don't build if not configured for use in lwipopts.h */

#ifdef PPP_INCLUDE_SETTINGS_HEADER
#include "ppp_settings.h"
#endif

#include <stdio.h> /* formats */
#include <stdarg.h>
#include <string.h>
#include <stdlib.h> /* strtol() */

#include "lwip/netif.h"
#include "lwip/def.h"
#include "lwip/timeouts.h"

#include "ppp.h"
#include "pppdebug.h"

/*
 * Memory used for control packets.
 *
 * PPP_CTRL_PBUF_MAX_SIZE is the amount of memory we allocate when we
 * cannot figure out how much we are going to use before filling the buffer.
 */
#if PPP_USE_PBUF_RAM
#define PPP_CTRL_PBUF_TYPE       PBUF_RAM
#define PPP_CTRL_PBUF_MAX_SIZE   512
#else /* PPP_USE_PBUF_RAM */
#define PPP_CTRL_PBUF_TYPE       PBUF_POOL
#define PPP_CTRL_PBUF_MAX_SIZE   PBUF_POOL_BUFSIZE
#endif /* PPP_USE_PBUF_RAM */

/*
 * The basic PPP frame.
 */
#define PPP_ADDRESS(p)	(((u_char *)(p))[0])
#define PPP_CONTROL(p)	(((u_char *)(p))[1])
#define PPP_PROTOCOL(p)	((((u_char *)(p))[2] << 8) + ((u_char *)(p))[3])

/*
 * Significant octet values.
 */
#define	PPP_ALLSTATIONS	0xff	/* All-Stations broadcast address */
#define	PPP_UI		0x03	/* Unnumbered Information */
#define	PPP_FLAG	0x7e	/* Flag Sequence */
#define	PPP_ESCAPE	0x7d	/* Asynchronous Control Escape */
#define	PPP_TRANS	0x20	/* Asynchronous transparency modifier */

/*
 * Protocol field values.
 */
#define PPP_IP		0x21	/* Internet Protocol */
#if 0 /* UNUSED */
#define PPP_AT		0x29	/* AppleTalk Protocol */
#define PPP_IPX		0x2b	/* IPX protocol */
#endif /* UNUSED */
#if VJ_SUPPORT
#define	PPP_VJC_COMP	0x2d	/* VJ compressed TCP */
#define	PPP_VJC_UNCOMP	0x2f	/* VJ uncompressed TCP */
#endif /* VJ_SUPPORT */
#if PPP_IPV6_SUPPORT
#define PPP_IPV6	0x57	/* Internet Protocol Version 6 */
#endif /* PPP_IPV6_SUPPORT */
#if CCP_SUPPORT
#define PPP_COMP	0xfd	/* compressed packet */
#endif /* CCP_SUPPORT */
#define PPP_IPCP	0x8021	/* IP Control Protocol */
#if 0 /* UNUSED */
#define PPP_ATCP	0x8029	/* AppleTalk Control Protocol */
#define PPP_IPXCP	0x802b	/* IPX Control Protocol */
#endif /* UNUSED */
#if PPP_IPV6_SUPPORT
#define PPP_IPV6CP	0x8057	/* IPv6 Control Protocol */
#endif /* PPP_IPV6_SUPPORT */
#if CCP_SUPPORT
#define PPP_CCP		0x80fd	/* Compression Control Protocol */
#endif /* CCP_SUPPORT */
#if ECP_SUPPORT
#define PPP_ECP		0x8053	/* Encryption Control Protocol */
#endif /* ECP_SUPPORT */
#define PPP_LCP		0xc021	/* Link Control Protocol */
#if PAP_SUPPORT
#define PPP_PAP		0xc023	/* Password Authentication Protocol */
#endif /* PAP_SUPPORT */
#if LQR_SUPPORT
#define PPP_LQR		0xc025	/* Link Quality Report protocol */
#endif /* LQR_SUPPORT */
#if CHAP_SUPPORT
#define PPP_CHAP	0xc223	/* Cryptographic Handshake Auth. Protocol */
#endif /* CHAP_SUPPORT */
#if CBCP_SUPPORT
#define PPP_CBCP	0xc029	/* Callback Control Protocol */
#endif /* CBCP_SUPPORT */
#if EAP_SUPPORT
#define PPP_EAP		0xc227	/* Extensible Authentication Protocol */
#endif /* EAP_SUPPORT */

/*
 * The following struct gives the addresses of procedures to call
 * for a particular lower link level protocol.
 */
struct link_callbacks {
  /* Start a connection (e.g. Initiate discovery phase) */
  void (*connect) (ppp_pcb *pcb, void *ctx);
#if PPP_SERVER
  /* Listen for an incoming connection (Passive mode) */
  void (*listen) (ppp_pcb *pcb, void *ctx);
#endif /* PPP_SERVER */
  /* End a connection (i.e. initiate disconnect phase) */
  void (*disconnect) (ppp_pcb *pcb, void *ctx);
  /* Free lower protocol control block */
  err_t (*free) (ppp_pcb *pcb, void *ctx);
  /* Write a pbuf to a ppp link, only used from PPP functions to send PPP packets. */
  err_t (*write)(ppp_pcb *pcb, void *ctx, struct pbuf *p);
  /* Send a packet from lwIP core (IPv4 or IPv6) */
  err_t (*netif_output)(ppp_pcb *pcb, void *ctx, struct pbuf *p, u_short protocol);
  /* configure the transmit-side characteristics of the PPP interface */
  void (*send_config)(ppp_pcb *pcb, void *ctx, u32_t accm, int pcomp, int accomp);
  /* confire the receive-side characteristics of the PPP interface */
  void (*recv_config)(ppp_pcb *pcb, void *ctx, u32_t accm, int pcomp, int accomp);
};

/*
 * What to do with network protocol (NP) packets.
 */
enum NPmode {
    NPMODE_PASS,		/* pass the packet through */
    NPMODE_DROP,		/* silently drop the packet */
    NPMODE_ERROR,		/* return an error */
    NPMODE_QUEUE		/* save it up for later. */
};

/*
 * Statistics.
 */
#if PPP_STATS_SUPPORT
struct pppstat	{
    unsigned int ppp_ibytes;	/* bytes received */
    unsigned int ppp_ipackets;	/* packets received */
    unsigned int ppp_ierrors;	/* receive errors */
    unsigned int ppp_obytes;	/* bytes sent */
    unsigned int ppp_opackets;	/* packets sent */
    unsigned int ppp_oerrors;	/* transmit errors */
};

#if VJ_SUPPORT
struct vjstat {
    unsigned int vjs_packets;	/* outbound packets */
    unsigned int vjs_compressed; /* outbound compressed packets */
    unsigned int vjs_searches;	/* searches for connection state */
    unsigned int vjs_misses;	/* times couldn't find conn. state */
    unsigned int vjs_uncompressedin; /* inbound uncompressed packets */
    unsigned int vjs_compressedin; /* inbound compressed packets */
    unsigned int vjs_errorin;	/* inbound unknown type packets */
    unsigned int vjs_tossed;	/* inbound packets tossed because of error */
};
#endif /* VJ_SUPPORT */

struct ppp_stats {
    struct pppstat p;		/* basic PPP statistics */
#if VJ_SUPPORT
    struct vjstat vj;		/* VJ header compression statistics */
#endif /* VJ_SUPPORT */
};

#if CCP_SUPPORT
struct compstat {
    unsigned int unc_bytes;	/* total uncompressed bytes */
    unsigned int unc_packets;	/* total uncompressed packets */
    unsigned int comp_bytes;	/* compressed bytes */
    unsigned int comp_packets;	/* compressed packets */
    unsigned int inc_bytes;	/* incompressible bytes */
    unsigned int inc_packets;	/* incompressible packets */
    unsigned int ratio;		/* recent compression ratio << 8 */
};

struct ppp_comp_stats {
    struct compstat c;		/* packet compression statistics */
    struct compstat d;		/* packet decompression statistics */
};
#endif /* CCP_SUPPORT */

#endif /* PPP_STATS_SUPPORT */

#if PPP_IDLETIMELIMIT
/*
 * The following structure records the time in seconds since
 * the last NP packet was sent or received.
 */
struct ppp_idle {
    time_t xmit_idle;		/* time since last NP packet sent */
    time_t recv_idle;		/* time since last NP packet received */
};
#endif /* PPP_IDLETIMELIMIT */

/* values for epdisc.class */
#define EPD_NULL	0	/* null discriminator, no data */
#define EPD_LOCAL	1
#define EPD_IP		2
#define EPD_MAC		3
#define EPD_MAGIC	4
#define EPD_PHONENUM	5

/*
 * Global variables.
 */
#ifdef HAVE_MULTILINK
extern u8_t	multilink;	/* enable multilink operation */
extern u8_t	doing_multilink;
extern u8_t	multilink_master;
extern u8_t	bundle_eof;
extern u8_t	bundle_terminating;
#endif

#ifdef MAXOCTETS
extern unsigned int maxoctets;	     /* Maximum octetes per session (in bytes) */
extern int       maxoctets_dir;      /* Direction :
				      0 - in+out (default)
				      1 - in
				      2 - out
				      3 - max(in,out) */
extern int       maxoctets_timeout;  /* Timeout for check of octets limit */
#define PPP_OCTETS_DIRECTION_SUM        0
#define PPP_OCTETS_DIRECTION_IN         1
#define PPP_OCTETS_DIRECTION_OUT        2
#define PPP_OCTETS_DIRECTION_MAXOVERAL  3
/* same as previos, but little different on RADIUS side */
#define PPP_OCTETS_DIRECTION_MAXSESSION 4
#endif

/* Data input may be used by CCP and ECP, remove this entry
 * from struct protent to save some flash
 */
#define PPP_DATAINPUT 0

/*
 * The following struct gives the addresses of procedures to call
 * for a particular protocol.
 */
struct protent {
    u_short protocol;		/* PPP protocol number */
    /* Initialization procedure */
    void (*init) (ppp_pcb *pcb);
    /* Process a received packet */
    void (*input) (ppp_pcb *pcb, u_char *pkt, int len);
    /* Process a received protocol-reject */
    void (*protrej) (ppp_pcb *pcb);
    /* Lower layer has come up */
    void (*lowerup) (ppp_pcb *pcb);
    /* Lower layer has gone down */
    void (*lowerdown) (ppp_pcb *pcb);
    /* Open the protocol */
    void (*open) (ppp_pcb *pcb);
    /* Close the protocol */
    void (*close) (ppp_pcb *pcb, const char *reason);
#if PRINTPKT_SUPPORT
    /* Print a packet in readable form */
    int  (*printpkt) (const u_char *pkt, int len,
			  void (*printer) (void *, const char *, ...),
			  void *arg);
#endif /* PRINTPKT_SUPPORT */
#if PPP_DATAINPUT
    /* Process a received data packet */
    void (*datainput) (ppp_pcb *pcb, u_char *pkt, int len);
#endif /* PPP_DATAINPUT */
#if PRINTPKT_SUPPORT
    const char *name;		/* Text name of protocol */
    const char *data_name;	/* Text name of corresponding data protocol */
#endif /* PRINTPKT_SUPPORT */
#if PPP_OPTIONS
    option_t *options;		/* List of command-line options */
    /* Check requested options, assign defaults */
    void (*check_options) (void);
#endif /* PPP_OPTIONS */
#if DEMAND_SUPPORT
    /* Configure interface for demand-dial */
    int  (*demand_conf) (int unit);
    /* Say whether to bring up link for this pkt */
    int  (*active_pkt) (u_char *pkt, int len);
#endif /* DEMAND_SUPPORT */
};

/* Table of pointers to supported protocols */
extern const struct protent* const protocols[];


/* Values for auth_pending, auth_done */
#if PAP_SUPPORT
#define PAP_WITHPEER	0x1
#define PAP_PEER	0x2
#endif /* PAP_SUPPORT */
#if CHAP_SUPPORT
#define CHAP_WITHPEER	0x4
#define CHAP_PEER	0x8
#endif /* CHAP_SUPPORT */
#if EAP_SUPPORT
#define EAP_WITHPEER	0x10
#define EAP_PEER	0x20
#endif /* EAP_SUPPORT */

/* Values for auth_done only */
#if CHAP_SUPPORT
#define CHAP_MD5_WITHPEER	0x40
#define CHAP_MD5_PEER		0x80
#if MSCHAP_SUPPORT
#define CHAP_MS_SHIFT		8	/* LSB position for MS auths */
#define CHAP_MS_WITHPEER	0x100
#define CHAP_MS_PEER		0x200
#define CHAP_MS2_WITHPEER	0x400
#define CHAP_MS2_PEER		0x800
#endif /* MSCHAP_SUPPORT */
#endif /* CHAP_SUPPORT */

/* Supported CHAP protocols */
#if CHAP_SUPPORT

#if MSCHAP_SUPPORT
#define CHAP_MDTYPE_SUPPORTED (MDTYPE_MICROSOFT_V2 | MDTYPE_MICROSOFT | MDTYPE_MD5)
#else /* MSCHAP_SUPPORT */
#define CHAP_MDTYPE_SUPPORTED (MDTYPE_MD5)
#endif /* MSCHAP_SUPPORT */

#else /* CHAP_SUPPORT */
#define CHAP_MDTYPE_SUPPORTED (MDTYPE_NONE)
#endif /* CHAP_SUPPORT */

#if PPP_STATS_SUPPORT
/*
 * PPP statistics structure
 */
struct pppd_stats {
    unsigned int	bytes_in;
    unsigned int	bytes_out;
    unsigned int	pkts_in;
    unsigned int	pkts_out;
};
#endif /* PPP_STATS_SUPPORT */


/*
 * PPP private functions
 */

 
/*
 * Functions called from lwIP core.
 */

/* initialize the PPP subsystem */
int ppp_init(void);

/*
 * Functions called from PPP link protocols.
 */

/* Create a new PPP control block */
ppp_pcb *ppp_new(struct netif *pppif, const struct link_callbacks *callbacks, void *link_ctx_cb,
                 ppp_link_status_cb_fn link_status_cb, void *ctx_cb);

/* Initiate LCP open request */
void ppp_start(ppp_pcb *pcb);

/* Called when link failed to setup */
void ppp_link_failed(ppp_pcb *pcb);

/* Called when link is normally down (i.e. it was asked to end) */
void ppp_link_end(ppp_pcb *pcb);

/* function called to process input packet */
void ppp_input(ppp_pcb *pcb, struct pbuf *pb);


/*
 * Functions called by PPP protocols.
 */

/* function called by all PPP subsystems to send packets */
err_t ppp_write(ppp_pcb *pcb, struct pbuf *p);

/* functions called by auth.c link_terminated() */
void ppp_link_terminated(ppp_pcb *pcb);

void new_phase(ppp_pcb *pcb, int p);

int ppp_send_config(ppp_pcb *pcb, int mtu, u32_t accm, int pcomp, int accomp);
int ppp_recv_config(ppp_pcb *pcb, int mru, u32_t accm, int pcomp, int accomp);

#if PPP_IPV4_SUPPORT
int sifaddr(ppp_pcb *pcb, u32_t our_adr, u32_t his_adr, u32_t netmask);
int cifaddr(ppp_pcb *pcb, u32_t our_adr, u32_t his_adr);
#if 0 /* UNUSED - PROXY ARP */
int sifproxyarp(ppp_pcb *pcb, u32_t his_adr);
int cifproxyarp(ppp_pcb *pcb, u32_t his_adr);
#endif /* UNUSED - PROXY ARP */
#if LWIP_DNS
int sdns(ppp_pcb *pcb, u32_t ns1, u32_t ns2);
int cdns(ppp_pcb *pcb, u32_t ns1, u32_t ns2);
#endif /* LWIP_DNS */
#if VJ_SUPPORT
int sifvjcomp(ppp_pcb *pcb, int vjcomp, int cidcomp, int maxcid);
#endif /* VJ_SUPPORT */
int sifup(ppp_pcb *pcb);
int sifdown (ppp_pcb *pcb);
u32_t get_mask(u32_t addr);
#endif /* PPP_IPV4_SUPPORT */

#if PPP_IPV6_SUPPORT
int sif6addr(ppp_pcb *pcb, eui64_t our_eui64, eui64_t his_eui64);
int cif6addr(ppp_pcb *pcb, eui64_t our_eui64, eui64_t his_eui64);
int sif6up(ppp_pcb *pcb);
int sif6down (ppp_pcb *pcb);
#endif /* PPP_IPV6_SUPPORT */

#if DEMAND_SUPPORT
int sifnpmode(ppp_pcb *pcb, int proto, enum NPmode mode);
#endif /* DEMAND_SUPPORt */

void netif_set_mtu(ppp_pcb *pcb, int mtu);
int netif_get_mtu(ppp_pcb *pcb);

#if CCP_SUPPORT
#if 0 /* unused */
int ccp_test(ppp_pcb *pcb, u_char *opt_ptr, int opt_len, int for_transmit);
#endif /* unused */
void ccp_set(ppp_pcb *pcb, u8_t isopen, u8_t isup, u8_t receive_method, u8_t transmit_method);
void ccp_reset_comp(ppp_pcb *pcb);
void ccp_reset_decomp(ppp_pcb *pcb);
#if 0 /* unused */
int ccp_fatal_error(ppp_pcb *pcb);
#endif /* unused */
#endif /* CCP_SUPPORT */

#if PPP_IDLETIMELIMIT
int get_idle_time(ppp_pcb *pcb, struct ppp_idle *ip);
#endif /* PPP_IDLETIMELIMIT */

#if DEMAND_SUPPORT
int get_loop_output(void);
#endif /* DEMAND_SUPPORT */

/* Optional protocol names list, to make our messages a little more informative. */
#if PPP_PROTOCOLNAME
const char * protocol_name(int proto);
#endif /* PPP_PROTOCOLNAME  */

/* Optional stats support, to get some statistics on the PPP interface */
#if PPP_STATS_SUPPORT
void print_link_stats(void); /* Print stats, if available */
void reset_link_stats(int u); /* Reset (init) stats when link goes up */
void update_link_stats(int u); /* Get stats at link termination */
#endif /* PPP_STATS_SUPPORT */



/*
 * Inline versions of get/put char/short/long.
 * Pointer is advanced; we assume that both arguments
 * are lvalues and will already be in registers.
 * cp MUST be u_char *.
 */
#define GETCHAR(c, cp) { \
	(c) = *(cp)++; \
}
#define PUTCHAR(c, cp) { \
	*(cp)++ = (u_char) (c); \
}
#define GETSHORT(s, cp) { \
	(s) = *(cp)++ << 8; \
	(s) |= *(cp)++; \
}
#define PUTSHORT(s, cp) { \
	*(cp)++ = (u_char) ((s) >> 8); \
	*(cp)++ = (u_char) (s); \
}
#define GETLONG(l, cp) { \
	(l) = *(cp)++ << 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; \
}
#define PUTLONG(l, cp) { \
	*(cp)++ = (u_char) ((l) >> 24); \
	*(cp)++ = (u_char) ((l) >> 16); \
	*(cp)++ = (u_char) ((l) >> 8); \
	*(cp)++ = (u_char) (l); \
}

#define INCPTR(n, cp)	((cp) += (n))
#define DECPTR(n, cp)	((cp) -= (n))

/*
 * System dependent definitions for user-level 4.3BSD UNIX implementation.
 */
#define TIMEOUT(f, a, t)        do { sys_untimeout((f), (a)); sys_timeout((t)*1000, (f), (a)); } while(0)
#define TIMEOUTMS(f, a, t)      do { sys_untimeout((f), (a)); sys_timeout((t), (f), (a)); } while(0)
#define UNTIMEOUT(f, a)         sys_untimeout((f), (a))

#define BZERO(s, n)		memset(s, 0, n)
#define	BCMP(s1, s2, l)		memcmp(s1, s2, l)

#define PRINTMSG(m, l)		{ ppp_info("Remote message: %0.*v", l, m); }

/*
 * MAKEHEADER - Add Header fields to a packet.
 */
#define MAKEHEADER(p, t) { \
    PUTCHAR(PPP_ALLSTATIONS, p); \
    PUTCHAR(PPP_UI, p); \
    PUTSHORT(t, p); }

/* Procedures exported from auth.c */
void link_required(ppp_pcb *pcb);     /* we are starting to use the link */
void link_terminated(ppp_pcb *pcb);   /* we are finished with the link */
void link_down(ppp_pcb *pcb);	      /* the LCP layer has left the Opened state */
void upper_layers_down(ppp_pcb *pcb); /* take all NCPs down */
void link_established(ppp_pcb *pcb);  /* the link is up; authenticate now */
void start_networks(ppp_pcb *pcb);    /* start all the network control protos */
void continue_networks(ppp_pcb *pcb); /* start network [ip, etc] control protos */
#if PPP_AUTH_SUPPORT
#if PPP_SERVER
int auth_check_passwd(ppp_pcb *pcb, char *auser, int userlen, char *apasswd, int passwdlen, const char **msg, int *msglen);
                                /* check the user name and passwd against configuration */
void auth_peer_fail(ppp_pcb *pcb, int protocol);
				/* peer failed to authenticate itself */
void auth_peer_success(ppp_pcb *pcb, int protocol, int prot_flavor, const char *name, int namelen);
				/* peer successfully authenticated itself */
#endif /* PPP_SERVER */
void auth_withpeer_fail(ppp_pcb *pcb, int protocol);
				/* we failed to authenticate ourselves */
void auth_withpeer_success(ppp_pcb *pcb, int protocol, int prot_flavor);
				/* we successfully authenticated ourselves */
#endif /* PPP_AUTH_SUPPORT */
void np_up(ppp_pcb *pcb, int proto);    /* a network protocol has come up */
void np_down(ppp_pcb *pcb, int proto);  /* a network protocol has gone down */
void np_finished(ppp_pcb *pcb, int proto); /* a network protocol no longer needs link */
#if PPP_AUTH_SUPPORT
int get_secret(ppp_pcb *pcb, const char *client, const char *server, char *secret, int *secret_len, int am_server);
				/* get "secret" for chap */
#endif /* PPP_AUTH_SUPPORT */

/* Procedures exported from ipcp.c */
/* int parse_dotted_ip (char *, u32_t *); */

/* Procedures exported from demand.c */
#if DEMAND_SUPPORT
void demand_conf (void);	/* config interface(s) for demand-dial */
void demand_block (void);	/* set all NPs to queue up packets */
void demand_unblock (void); /* set all NPs to pass packets */
void demand_discard (void); /* set all NPs to discard packets */
void demand_rexmit (int, u32_t); /* retransmit saved frames for an NP*/
int  loop_chars (unsigned char *, int); /* process chars from loopback */
int  loop_frame (unsigned char *, int); /* should we bring link up? */
#endif /* DEMAND_SUPPORT */

/* Procedures exported from multilink.c */
#ifdef HAVE_MULTILINK
void mp_check_options (void); /* Check multilink-related options */
int  mp_join_bundle (void);  /* join our link to an appropriate bundle */
void mp_exit_bundle (void);  /* have disconnected our link from bundle */
void mp_bundle_terminated (void);
char *epdisc_to_str (struct epdisc *); /* string from endpoint discrim. */
int  str_to_epdisc (struct epdisc *, char *); /* endpt disc. from str */
#else
#define mp_bundle_terminated()	/* nothing */
#define mp_exit_bundle()	/* nothing */
#define doing_multilink		0
#define multilink_master	0
#endif

/* Procedures exported from utils.c. */
void ppp_print_string(const u_char *p, int len, void (*printer) (void *, const char *, ...), void *arg);   /* Format a string for output */
int ppp_slprintf(char *buf, int buflen, const char *fmt, ...);            /* sprintf++ */
int ppp_vslprintf(char *buf, int buflen, const char *fmt, va_list args);  /* vsprintf++ */
size_t ppp_strlcpy(char *dest, const char *src, size_t len);        /* safe strcpy */
size_t ppp_strlcat(char *dest, const char *src, size_t len);        /* safe strncpy */
void ppp_dbglog(const char *fmt, ...);    /* log a debug message */
void ppp_info(const char *fmt, ...);      /* log an informational message */
void ppp_notice(const char *fmt, ...);    /* log a notice-level message */
void ppp_warn(const char *fmt, ...);      /* log a warning message */
void ppp_error(const char *fmt, ...);     /* log an error message */
void ppp_fatal(const char *fmt, ...);     /* log an error message and die(1) */
#if PRINTPKT_SUPPORT
void ppp_dump_packet(ppp_pcb *pcb, const char *tag, unsigned char *p, int len);
                                /* dump packet to debug log if interesting */
#endif /* PRINTPKT_SUPPORT */


#endif /* PPP_SUPPORT */
#endif /* LWIP_HDR_PPP_IMPL_H */
