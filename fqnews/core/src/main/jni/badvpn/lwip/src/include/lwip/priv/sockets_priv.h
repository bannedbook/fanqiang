/**
 * @file
 * Sockets API internal implementations (do not use in application code)
 */

/*
 * Copyright (c) 2017 Joel Cunningham, Garmin International, Inc. <joel.cunningham@garmin.com>
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
 * Author: Joel Cunningham <joel.cunningham@me.com>
 *
 */
#ifndef LWIP_HDR_SOCKETS_PRIV_H
#define LWIP_HDR_SOCKETS_PRIV_H

#include "lwip/opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_SOCKETS MEMP_NUM_NETCONN

/** This is overridable for the rare case where more than 255 threads
 * select on the same socket...
 */
#ifndef SELWAIT_T
#define SELWAIT_T u8_t
#endif

union lwip_sock_lastdata {
  struct netbuf *netbuf;
  struct pbuf *pbuf;
};

/** Contains all internal pointers and states used for a socket */
struct lwip_sock {
  /** sockets currently are built on netconns, each socket has one netconn */
  struct netconn *conn;
  /** data that was left from the previous read */
  union lwip_sock_lastdata lastdata;
#if LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL
  /** number of times data was received, set by event_callback(),
      tested by the receive and select functions */
  s16_t rcvevent;
  /** number of times data was ACKed (free send buffer), set by event_callback(),
      tested by select */
  u16_t sendevent;
  /** error happened for this socket, set by event_callback(), tested by select */
  u16_t errevent;
  /** counter of how many threads are waiting for this socket using select */
  SELWAIT_T select_waiting;
#endif /* LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL */
#if LWIP_NETCONN_FULLDUPLEX
  /* counter of how many threads are using a struct lwip_sock (not the 'int') */
  u8_t fd_used;
  /* status of pending close/delete actions */
  u8_t fd_free_pending;
#define LWIP_SOCK_FD_FREE_TCP  1
#define LWIP_SOCK_FD_FREE_FREE 2
#endif
};

#ifndef set_errno
#define set_errno(err) do { if (err) { errno = (err); } } while(0)
#endif

#if !LWIP_TCPIP_CORE_LOCKING
/** Maximum optlen used by setsockopt/getsockopt */
#define LWIP_SETGETSOCKOPT_MAXOPTLEN LWIP_MAX(16, sizeof(struct ifreq))

/** This struct is used to pass data to the set/getsockopt_internal
 * functions running in tcpip_thread context (only a void* is allowed) */
struct lwip_setgetsockopt_data {
  /** socket index for which to change options */
  int s;
  /** level of the option to process */
  int level;
  /** name of the option to process */
  int optname;
  /** set: value to set the option to
    * get: value of the option is stored here */
#if LWIP_MPU_COMPATIBLE
  u8_t optval[LWIP_SETGETSOCKOPT_MAXOPTLEN];
#else
  union {
    void *p;
    const void *pc;
  } optval;
#endif
  /** size of *optval */
  socklen_t optlen;
  /** if an error occurs, it is temporarily stored here */
  int err;
  /** semaphore to wake up the calling task */
  void* completed_sem;
};
#endif /* !LWIP_TCPIP_CORE_LOCKING */

#ifdef __cplusplus
}
#endif

struct lwip_sock* lwip_socket_dbg_get_socket(int fd);

#if LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL

#if LWIP_NETCONN_SEM_PER_THREAD
#define SELECT_SEM_T        sys_sem_t*
#define SELECT_SEM_PTR(sem) (sem)
#else /* LWIP_NETCONN_SEM_PER_THREAD */
#define SELECT_SEM_T        sys_sem_t
#define SELECT_SEM_PTR(sem) (&(sem))
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

/** Description for a task waiting in select */
struct lwip_select_cb {
  /** Pointer to the next waiting task */
  struct lwip_select_cb *next;
  /** Pointer to the previous waiting task */
  struct lwip_select_cb *prev;
#if LWIP_SOCKET_SELECT
  /** readset passed to select */
  fd_set *readset;
  /** writeset passed to select */
  fd_set *writeset;
  /** unimplemented: exceptset passed to select */
  fd_set *exceptset;
#endif /* LWIP_SOCKET_SELECT */
#if LWIP_SOCKET_POLL
  /** fds passed to poll; NULL if select */
  struct pollfd *poll_fds;
  /** nfds passed to poll; 0 if select */
  nfds_t poll_nfds;
#endif /* LWIP_SOCKET_POLL */
  /** don't signal the same semaphore twice: set to 1 when signalled */
  int sem_signalled;
  /** semaphore to wake up a task waiting for select */
  SELECT_SEM_T sem;
};
#endif /* LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL */

#endif /* LWIP_SOCKET */

#endif /* LWIP_HDR_SOCKETS_PRIV_H */
