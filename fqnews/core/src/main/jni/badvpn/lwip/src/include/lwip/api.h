/**
 * @file
 * netconn API (to be used from non-TCPIP threads)
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 */
#ifndef LWIP_HDR_API_H
#define LWIP_HDR_API_H

#include "lwip/opt.h"

#if LWIP_NETCONN || LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */
/* Note: Netconn API is always available when sockets are enabled -
 * sockets are implemented on top of them */

#include "lwip/arch.h"
#include "lwip/netbuf.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Throughout this file, IP addresses and port numbers are expected to be in
 * the same byte order as in the corresponding pcb.
 */

/* Flags for netconn_write (u8_t) */
#define NETCONN_NOFLAG      0x00
#define NETCONN_NOCOPY      0x00 /* Only for source code compatibility */
#define NETCONN_COPY        0x01
#define NETCONN_MORE        0x02
#define NETCONN_DONTBLOCK   0x04
#define NETCONN_NOAUTORCVD  0x08 /* prevent netconn_recv_data_tcp() from updating the tcp window - must be done manually via netconn_tcp_recvd() */

/* Flags for struct netconn.flags (u8_t) */
/** This netconn had an error, don't block on recvmbox/acceptmbox any more */
#define NETCONN_FLAG_MBOXCLOSED               0x01
/** Should this netconn avoid blocking? */
#define NETCONN_FLAG_NON_BLOCKING             0x02
/** Was the last connect action a non-blocking one? */
#define NETCONN_FLAG_IN_NONBLOCKING_CONNECT   0x04
/** If a nonblocking write has been rejected before, poll_tcp needs to
    check if the netconn is writable again */
#define NETCONN_FLAG_CHECK_WRITESPACE         0x10
#if LWIP_IPV6
/** If this flag is set then only IPv6 communication is allowed on the
    netconn. As per RFC#3493 this features defaults to OFF allowing
    dual-stack usage by default. */
#define NETCONN_FLAG_IPV6_V6ONLY              0x20
#endif /* LWIP_IPV6 */
#if LWIP_NETBUF_RECVINFO
/** Received packet info will be recorded for this netconn */
#define NETCONN_FLAG_PKTINFO                  0x40
#endif /* LWIP_NETBUF_RECVINFO */


/* Helpers to process several netconn_types by the same code */
#define NETCONNTYPE_GROUP(t)         ((t)&0xF0)
#define NETCONNTYPE_DATAGRAM(t)      ((t)&0xE0)
#if LWIP_IPV6
#define NETCONN_TYPE_IPV6            0x08
#define NETCONNTYPE_ISIPV6(t)        (((t)&NETCONN_TYPE_IPV6) != 0)
#define NETCONNTYPE_ISUDPLITE(t)     (((t)&0xF3) == NETCONN_UDPLITE)
#define NETCONNTYPE_ISUDPNOCHKSUM(t) (((t)&0xF3) == NETCONN_UDPNOCHKSUM)
#else /* LWIP_IPV6 */
#define NETCONNTYPE_ISIPV6(t)        (0)
#define NETCONNTYPE_ISUDPLITE(t)     ((t) == NETCONN_UDPLITE)
#define NETCONNTYPE_ISUDPNOCHKSUM(t) ((t) == NETCONN_UDPNOCHKSUM)
#endif /* LWIP_IPV6 */

/** @ingroup netconn_common
 * Protocol family and type of the netconn
 */
enum netconn_type {
  NETCONN_INVALID     = 0,
  /** TCP IPv4 */
  NETCONN_TCP         = 0x10,
#if LWIP_IPV6
  /** TCP IPv6 */
  NETCONN_TCP_IPV6    = NETCONN_TCP | NETCONN_TYPE_IPV6 /* 0x18 */,
#endif /* LWIP_IPV6 */
  /** UDP IPv4 */
  NETCONN_UDP         = 0x20,
  /** UDP IPv4 lite */
  NETCONN_UDPLITE     = 0x21,
  /** UDP IPv4 no checksum */
  NETCONN_UDPNOCHKSUM = 0x22,

#if LWIP_IPV6
  /** UDP IPv6 (dual-stack by default, unless you call @ref netconn_set_ipv6only) */
  NETCONN_UDP_IPV6         = NETCONN_UDP | NETCONN_TYPE_IPV6 /* 0x28 */,
  /** UDP IPv6 lite (dual-stack by default, unless you call @ref netconn_set_ipv6only) */
  NETCONN_UDPLITE_IPV6     = NETCONN_UDPLITE | NETCONN_TYPE_IPV6 /* 0x29 */,
  /** UDP IPv6 no checksum (dual-stack by default, unless you call @ref netconn_set_ipv6only) */
  NETCONN_UDPNOCHKSUM_IPV6 = NETCONN_UDPNOCHKSUM | NETCONN_TYPE_IPV6 /* 0x2a */,
#endif /* LWIP_IPV6 */

  /** Raw connection IPv4 */
  NETCONN_RAW         = 0x40
#if LWIP_IPV6
  /** Raw connection IPv6 (dual-stack by default, unless you call @ref netconn_set_ipv6only) */
  , NETCONN_RAW_IPV6    = NETCONN_RAW | NETCONN_TYPE_IPV6 /* 0x48 */
#endif /* LWIP_IPV6 */
};

/** Current state of the netconn. Non-TCP netconns are always
 * in state NETCONN_NONE! */
enum netconn_state {
  NETCONN_NONE,
  NETCONN_WRITE,
  NETCONN_LISTEN,
  NETCONN_CONNECT,
  NETCONN_CLOSE
};

/** Used to inform the callback function about changes
 * 
 * Event explanation:
 * 
 * In the netconn implementation, there are three ways to block a client:
 * 
 * - accept mbox (sys_arch_mbox_fetch(&conn->acceptmbox, &accept_ptr, 0); in netconn_accept())
 * - receive mbox (sys_arch_mbox_fetch(&conn->recvmbox, &buf, 0); in netconn_recv_data())
 * - send queue is full (sys_arch_sem_wait(LWIP_API_MSG_SEM(msg), 0); in lwip_netconn_do_write())
 * 
 * The events have to be seen as events signaling the state of these mboxes/semaphores. For non-blocking
 * connections, you need to know in advance whether a call to a netconn function call would block or not,
 * and these events tell you about that.
 * 
 * RCVPLUS events say: Safe to perform a potentially blocking call call once more. 
 * They are counted in sockets - three RCVPLUS events for accept mbox means you are safe
 * to call netconn_accept 3 times without being blocked.
 * Same thing for receive mbox.
 * 
 * RCVMINUS events say: Your call to to a possibly blocking function is "acknowledged".
 * Socket implementation decrements the counter.
 * 
 * For TX, there is no need to count, its merely a flag. SENDPLUS means you may send something.
 * SENDPLUS occurs when enough data was delivered to peer so netconn_send() can be called again.
 * A SENDMINUS event occurs when the next call to a netconn_send() would be blocking.
 */
enum netconn_evt {
  NETCONN_EVT_RCVPLUS,
  NETCONN_EVT_RCVMINUS,
  NETCONN_EVT_SENDPLUS,
  NETCONN_EVT_SENDMINUS,
  NETCONN_EVT_ERROR
};

#if LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD)
/** Used for netconn_join_leave_group() */
enum netconn_igmp {
  NETCONN_JOIN,
  NETCONN_LEAVE
};
#endif /* LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD) */

#if LWIP_DNS
/* Used for netconn_gethostbyname_addrtype(), these should match the DNS_ADDRTYPE defines in dns.h */
#define NETCONN_DNS_DEFAULT   NETCONN_DNS_IPV4_IPV6
#define NETCONN_DNS_IPV4      0
#define NETCONN_DNS_IPV6      1
#define NETCONN_DNS_IPV4_IPV6 2 /* try to resolve IPv4 first, try IPv6 if IPv4 fails only */
#define NETCONN_DNS_IPV6_IPV4 3 /* try to resolve IPv6 first, try IPv4 if IPv6 fails only */
#endif /* LWIP_DNS */

/* forward-declare some structs to avoid to include their headers */
struct ip_pcb;
struct tcp_pcb;
struct udp_pcb;
struct raw_pcb;
struct netconn;
struct api_msg;

/** A callback prototype to inform about events for a netconn */
typedef void (* netconn_callback)(struct netconn *, enum netconn_evt, u16_t len);

/** A netconn descriptor */
struct netconn {
  /** type of the netconn (TCP, UDP or RAW) */
  enum netconn_type type;
  /** current state of the netconn */
  enum netconn_state state;
  /** the lwIP internal protocol control block */
  union {
    struct ip_pcb  *ip;
    struct tcp_pcb *tcp;
    struct udp_pcb *udp;
    struct raw_pcb *raw;
  } pcb;
  /** the last asynchronous unreported error this netconn had */
  err_t pending_err;
#if !LWIP_NETCONN_SEM_PER_THREAD
  /** sem that is used to synchronously execute functions in the core context */
  sys_sem_t op_completed;
#endif
  /** mbox where received packets are stored until they are fetched
      by the netconn application thread (can grow quite big) */
  sys_mbox_t recvmbox;
#if LWIP_TCP
  /** mbox where new connections are stored until processed
      by the application thread */
  sys_mbox_t acceptmbox;
#endif /* LWIP_TCP */
  /** only used for socket layer */
#if LWIP_SOCKET
  int socket;
#endif /* LWIP_SOCKET */
#if LWIP_SO_SNDTIMEO
  /** timeout to wait for sending data (which means enqueueing data for sending
      in internal buffers) in milliseconds */
  s32_t send_timeout;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVTIMEO
  /** timeout in milliseconds to wait for new data to be received
      (or connections to arrive for listening netconns) */
  u32_t recv_timeout;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
  /** maximum amount of bytes queued in recvmbox
      not used for TCP: adjust TCP_WND instead! */
  int recv_bufsize;
  /** number of bytes currently in recvmbox to be received,
      tested against recv_bufsize to limit bytes on recvmbox
      for UDP and RAW, used for FIONREAD */
  int recv_avail;
#endif /* LWIP_SO_RCVBUF */
#if LWIP_SO_LINGER
   /** values <0 mean linger is disabled, values > 0 are seconds to linger */
  s16_t linger;
#endif /* LWIP_SO_LINGER */
  /** flags holding more netconn-internal state, see NETCONN_FLAG_* defines */
  u8_t flags;
#if LWIP_TCP
  /** TCP: when data passed to netconn_write doesn't fit into the send buffer,
      this temporarily stores the message.
      Also used during connect and close. */
  struct api_msg *current_msg;
#endif /* LWIP_TCP */
  /** A callback function that is informed about events for this netconn */
  netconn_callback callback;
};

/** This vector type is passed to @ref netconn_write_vectors_partly to send
 * multiple buffers at once.
 * ATTENTION: This type has to directly map struct iovec since one is casted
 *            into the other!
 */
struct netvector {
  /** pointer to the application buffer that contains the data to send */
  const void *ptr;
  /** size of the application data to send */
  size_t len;
};

/** Register an Network connection event */
#define API_EVENT(c,e,l) if (c->callback) {         \
                           (*c->callback)(c, e, l); \
                         }

/* Network connection functions: */

/** @ingroup netconn_common
 * Create new netconn connection
 * @param t @ref netconn_type */
#define netconn_new(t)                  netconn_new_with_proto_and_callback(t, 0, NULL)
#define netconn_new_with_callback(t, c) netconn_new_with_proto_and_callback(t, 0, c)
struct netconn *netconn_new_with_proto_and_callback(enum netconn_type t, u8_t proto,
                                             netconn_callback callback);
err_t   netconn_delete(struct netconn *conn);
/** Get the type of a netconn (as enum netconn_type). */
#define netconn_type(conn) (conn->type)

err_t   netconn_getaddr(struct netconn *conn, ip_addr_t *addr,
                        u16_t *port, u8_t local);
/** @ingroup netconn_common */
#define netconn_peer(c,i,p) netconn_getaddr(c,i,p,0)
/** @ingroup netconn_common */
#define netconn_addr(c,i,p) netconn_getaddr(c,i,p,1)

err_t   netconn_bind(struct netconn *conn, const ip_addr_t *addr, u16_t port);
err_t   netconn_bind_if(struct netconn *conn, u8_t if_idx);
err_t   netconn_connect(struct netconn *conn, const ip_addr_t *addr, u16_t port);
err_t   netconn_disconnect (struct netconn *conn);
err_t   netconn_listen_with_backlog(struct netconn *conn, u8_t backlog);
/** @ingroup netconn_tcp */
#define netconn_listen(conn) netconn_listen_with_backlog(conn, TCP_DEFAULT_LISTEN_BACKLOG)
err_t   netconn_accept(struct netconn *conn, struct netconn **new_conn);
err_t   netconn_recv(struct netconn *conn, struct netbuf **new_buf);
err_t   netconn_recv_udp_raw_netbuf(struct netconn *conn, struct netbuf **new_buf);
err_t   netconn_recv_udp_raw_netbuf_flags(struct netconn *conn, struct netbuf **new_buf, u8_t apiflags);
err_t   netconn_recv_tcp_pbuf(struct netconn *conn, struct pbuf **new_buf);
err_t   netconn_recv_tcp_pbuf_flags(struct netconn *conn, struct pbuf **new_buf, u8_t apiflags);
err_t   netconn_tcp_recvd(struct netconn *conn, size_t len);
err_t   netconn_sendto(struct netconn *conn, struct netbuf *buf,
                             const ip_addr_t *addr, u16_t port);
err_t   netconn_send(struct netconn *conn, struct netbuf *buf);
err_t   netconn_write_partly(struct netconn *conn, const void *dataptr, size_t size,
                             u8_t apiflags, size_t *bytes_written);
err_t   netconn_write_vectors_partly(struct netconn *conn, struct netvector *vectors, u16_t vectorcnt,
                                     u8_t apiflags, size_t *bytes_written);
/** @ingroup netconn_tcp */
#define netconn_write(conn, dataptr, size, apiflags) \
          netconn_write_partly(conn, dataptr, size, apiflags, NULL)
err_t   netconn_close(struct netconn *conn);
err_t   netconn_shutdown(struct netconn *conn, u8_t shut_rx, u8_t shut_tx);

#if LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD)
err_t   netconn_join_leave_group(struct netconn *conn, const ip_addr_t *multiaddr,
                             const ip_addr_t *netif_addr, enum netconn_igmp join_or_leave);
err_t   netconn_join_leave_group_netif(struct netconn *conn, const ip_addr_t *multiaddr,
                             u8_t if_idx, enum netconn_igmp join_or_leave);
#endif /* LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD) */
#if LWIP_DNS
#if LWIP_IPV4 && LWIP_IPV6
err_t   netconn_gethostbyname_addrtype(const char *name, ip_addr_t *addr, u8_t dns_addrtype);
#define netconn_gethostbyname(name, addr) netconn_gethostbyname_addrtype(name, addr, NETCONN_DNS_DEFAULT)
#else /* LWIP_IPV4 && LWIP_IPV6 */
err_t   netconn_gethostbyname(const char *name, ip_addr_t *addr);
#define netconn_gethostbyname_addrtype(name, addr, dns_addrtype) netconn_gethostbyname(name, addr)
#endif /* LWIP_IPV4 && LWIP_IPV6 */
#endif /* LWIP_DNS */

err_t   netconn_err(struct netconn *conn);
#define netconn_recv_bufsize(conn)      ((conn)->recv_bufsize)

#define netconn_set_flags(conn, set_flags)     do { (conn)->flags = (u8_t)((conn)->flags |  (set_flags)); } while(0)
#define netconn_clear_flags(conn, clr_flags)   do { (conn)->flags = (u8_t)((conn)->flags & ~(clr_flags)); } while(0)
#define netconn_is_flag_set(conn, flag)        (((conn)->flags & (flag)) != 0)

/** Set the blocking status of netconn calls (@todo: write/send is missing) */
#define netconn_set_nonblocking(conn, val)  do { if(val) { \
  netconn_set_flags(conn, NETCONN_FLAG_NON_BLOCKING); \
} else { \
  netconn_clear_flags(conn, NETCONN_FLAG_NON_BLOCKING); }} while(0)
/** Get the blocking status of netconn calls (@todo: write/send is missing) */
#define netconn_is_nonblocking(conn)        (((conn)->flags & NETCONN_FLAG_NON_BLOCKING) != 0)

#if LWIP_IPV6
/** @ingroup netconn_common
 * TCP: Set the IPv6 ONLY status of netconn calls (see NETCONN_FLAG_IPV6_V6ONLY)
 */
#define netconn_set_ipv6only(conn, val)  do { if(val) { \
  netconn_set_flags(conn, NETCONN_FLAG_IPV6_V6ONLY); \
} else { \
  netconn_clear_flags(conn, NETCONN_FLAG_IPV6_V6ONLY); }} while(0)
/** @ingroup netconn_common
 * TCP: Get the IPv6 ONLY status of netconn calls (see NETCONN_FLAG_IPV6_V6ONLY)
 */
#define netconn_get_ipv6only(conn)        (((conn)->flags & NETCONN_FLAG_IPV6_V6ONLY) != 0)
#endif /* LWIP_IPV6 */

#if LWIP_SO_SNDTIMEO
/** Set the send timeout in milliseconds */
#define netconn_set_sendtimeout(conn, timeout)      ((conn)->send_timeout = (timeout))
/** Get the send timeout in milliseconds */
#define netconn_get_sendtimeout(conn)               ((conn)->send_timeout)
#endif /* LWIP_SO_SNDTIMEO */
#if LWIP_SO_RCVTIMEO
/** Set the receive timeout in milliseconds */
#define netconn_set_recvtimeout(conn, timeout)      ((conn)->recv_timeout = (timeout))
/** Get the receive timeout in milliseconds */
#define netconn_get_recvtimeout(conn)               ((conn)->recv_timeout)
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
/** Set the receive buffer in bytes */
#define netconn_set_recvbufsize(conn, recvbufsize)  ((conn)->recv_bufsize = (recvbufsize))
/** Get the receive buffer in bytes */
#define netconn_get_recvbufsize(conn)               ((conn)->recv_bufsize)
#endif /* LWIP_SO_RCVBUF*/

#if LWIP_NETCONN_SEM_PER_THREAD
void netconn_thread_init(void);
void netconn_thread_cleanup(void);
#else /* LWIP_NETCONN_SEM_PER_THREAD */
#define netconn_thread_init()
#define netconn_thread_cleanup()
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_NETCONN || LWIP_SOCKET */

#endif /* LWIP_HDR_API_H */
