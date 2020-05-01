#include "test_sockets.h"

#include "lwip/mem.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/priv/sockets_priv.h"
#include "lwip/stats.h"

#include "lwip/tcpip.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/api.h"


static int
test_sockets_get_used_count(void)
{
  int used = 0;
  int i;

  for (i = 0; i < NUM_SOCKETS; i++) {
    struct lwip_sock* s = lwip_socket_dbg_get_socket(i);
    if (s != NULL) {
      if (s->fd_used) {
        used++;
      }
    }
  }
  return used;
}


/* Setups/teardown functions */

static void
sockets_setup(void)
{
  /* expect full free heap */
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
sockets_teardown(void)
{
  fail_unless(test_sockets_get_used_count() == 0);
  /* poll until all memory is released... */
  tcpip_thread_poll_one();
  while (tcp_tw_pcbs) {
    tcp_abort(tcp_tw_pcbs);
    tcpip_thread_poll_one();
  }
  tcpip_thread_poll_one();
  /* ensure full free heap */
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

#ifndef NUM_SOCKETS
#define NUM_SOCKETS MEMP_NUM_NETCONN
#endif

#if LWIP_SOCKET
static int
test_sockets_alloc_socket_nonblocking(int domain, int type)
{
  int s = lwip_socket(domain, type, 0);
  if (s >= 0) {
    int ret = lwip_fcntl(s, F_SETFL, O_NONBLOCK);
    fail_unless(ret == 0);
  }
  return s;
}

/* Verify basic sockets functionality
 */
START_TEST(test_sockets_basics)
{
  int s, i, ret;
  int s2[NUM_SOCKETS];
  LWIP_UNUSED_ARG(_i);

  s = lwip_socket(AF_INET, SOCK_STREAM, 0);
  fail_unless(s >= 0);
  lwip_close(s);

  for (i = 0; i < NUM_SOCKETS; i++) {
    s2[i] = lwip_socket(AF_INET, SOCK_STREAM, 0);
    fail_unless(s2[i] >= 0);
  }

  /* all sockets used, now it should fail */
  s = lwip_socket(AF_INET, SOCK_STREAM, 0);
  fail_unless(s == -1);
  /* close one socket */
  ret = lwip_close(s2[0]);
  fail_unless(ret == 0);
  /* now it should succeed */
  s2[0] = lwip_socket(AF_INET, SOCK_STREAM, 0);
  fail_unless(s2[0] >= 0);

  /* close all sockets */
  for (i = 0; i < NUM_SOCKETS; i++) {
    ret = lwip_close(s2[i]);
    fail_unless(ret == 0);
  }
}
END_TEST

static void test_sockets_allfunctions_basic_domain(int domain)
{
  int s, s2, s3, ret;
  struct sockaddr_storage addr, addr2;
  socklen_t addrlen, addr2len;
  /* listen socket */
  s = lwip_socket(domain, SOCK_STREAM, 0);
  fail_unless(s >= 0);

  ret = lwip_listen(s, 0);
  fail_unless(ret == 0);

  addrlen = sizeof(addr);
  ret = lwip_getsockname(s, (struct sockaddr*)&addr, &addrlen);
  fail_unless(ret == 0);

  s2 = test_sockets_alloc_socket_nonblocking(domain, SOCK_STREAM);
  fail_unless(s2 >= 0);
  /* nonblocking connect s2 to s (but use loopback address) */
  if (domain == AF_INET) {
#if LWIP_IPV4
    struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
    addr4->sin_addr.s_addr = PP_HTONL(INADDR_LOOPBACK);
#endif
  } else {
#if LWIP_IPV6
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
    struct in6_addr lo6 = IN6ADDR_LOOPBACK_INIT;
    addr6->sin6_addr = lo6;
#endif
  }
  ret = lwip_connect(s2, (struct sockaddr*)&addr, addrlen);
  fail_unless(ret == -1);
  fail_unless(errno == EINPROGRESS);
  ret = lwip_connect(s2, (struct sockaddr*)&addr, addrlen);
  fail_unless(ret == -1);
  fail_unless(errno == EALREADY);

  while(tcpip_thread_poll_one());

  s3 = lwip_accept(s, (struct sockaddr*)&addr2, &addr2len);
  fail_unless(s3 >= 0);

  ret = lwip_connect(s2, (struct sockaddr*)&addr, addrlen);
  fail_unless(ret == -1);
  fail_unless(errno == EISCONN);

  ret = lwip_close(s);
  fail_unless(ret == 0);
  ret = lwip_close(s2);
  fail_unless(ret == 0);
  ret = lwip_close(s3);
  fail_unless(ret == 0);
}

/* Try to step through all sockets functions once...
 */
START_TEST(test_sockets_allfunctions_basic)
{
  LWIP_UNUSED_ARG(_i);
#if LWIP_IPV4
  test_sockets_allfunctions_basic_domain(AF_INET);
#endif
#if LWIP_IPV6
  test_sockets_allfunctions_basic_domain(AF_INET6);
#endif
}
END_TEST

static void test_sockets_init_loopback_addr(int domain, struct sockaddr_storage *addr_st, socklen_t *sz)
{
  memset(addr_st, 0, sizeof(*addr_st));
  switch(domain) {
#if LWIP_IPV6
    case AF_INET6: {
      struct sockaddr_in6 *addr = (struct sockaddr_in6*)addr_st;
      struct in6_addr lo6 = IN6ADDR_LOOPBACK_INIT;
      addr->sin6_family = AF_INET6;
      addr->sin6_port = 0; /* use ephemeral port */
      addr->sin6_addr = lo6;
      *sz = sizeof(*addr);
   }
      break;
#endif /* LWIP_IPV6 */
#if LWIP_IPV4
    case AF_INET: {
      struct sockaddr_in *addr = (struct sockaddr_in*)addr_st;
      addr->sin_family = AF_INET;
      addr->sin_port = 0; /* use ephemeral port */
      addr->sin_addr.s_addr = PP_HTONL(INADDR_LOOPBACK);
      *sz = sizeof(*addr);
    }
      break;
#endif /* LWIP_IPV4 */
    default:
      *sz = 0;
      fail();
      break;
  }
}

static void test_sockets_msgapi_update_iovs(struct msghdr *msg, size_t bytes)
{
  int i;

  /* note: this modifies the underyling iov_base and iov_len for a partial
     read for an individual vector. This updates the msg->msg_iov pointer
     to skip fully consumed vecotrs */
  
  /* process fully consumed vectors */
  for (i = 0; i < msg->msg_iovlen; i++) {
    if (msg->msg_iov[i].iov_len <= bytes) {
      /* reduce bytes by amount of this vector */
      bytes -= msg->msg_iov[i].iov_len;
    } else {
      break; /* iov not fully consumed */
    }
  }

  /* slide down over fully consumed vectors */
  msg->msg_iov = &msg->msg_iov[i];
  msg->msg_iovlen -= i;

  /* update new first vector with any remaining amount */
  msg->msg_iov[0].iov_base = ((u8_t *)msg->msg_iov[0].iov_base + bytes);
  msg->msg_iov[0].iov_len -= bytes;
}

static void test_sockets_msgapi_tcp(int domain)
{
  #define BUF_SZ          (TCP_SND_BUF/4)
  #define TOTAL_DATA_SZ   (BUF_SZ*8) /* ~(TCP_SND_BUF*2) that accounts for integer rounding */
  #define NEED_TRAILER    (BUF_SZ % 4 != 0)
  int listnr, s1, s2, i, ret, opt;
  int bytes_written, bytes_read;
  struct sockaddr_storage addr_storage;
  socklen_t addr_size;
  struct iovec siovs[8];
  struct msghdr smsg;
  u8_t * snd_buf;
  struct iovec riovs[5];
  struct iovec riovs_tmp[5];
  struct msghdr rmsg;
  u8_t * rcv_buf;
  int    rcv_off;
  int    rcv_trailer = 0;
  u8_t val;

  test_sockets_init_loopback_addr(domain, &addr_storage, &addr_size);

  listnr = test_sockets_alloc_socket_nonblocking(domain, SOCK_STREAM);
  fail_unless(listnr >= 0);
  s1 = test_sockets_alloc_socket_nonblocking(domain, SOCK_STREAM);
  fail_unless(s1 >= 0);

  /* setup a listener socket on loopback with ephemeral port */
  ret = lwip_bind(listnr, (struct sockaddr*)&addr_storage, addr_size);
  fail_unless(ret == 0);
  ret = lwip_listen(listnr, 0);
  fail_unless(ret == 0);

  /* update address with ephemeral port */
  ret = lwip_getsockname(listnr, (struct sockaddr*)&addr_storage, &addr_size);
  fail_unless(ret == 0);

  /* connect, won't complete until we accept it */
  ret = lwip_connect(s1, (struct sockaddr*)&addr_storage, addr_size);
  fail_unless(ret == -1);
  fail_unless(errno == EINPROGRESS);

  while (tcpip_thread_poll_one());

  /* accept, creating the other side of the connection */
  s2 = lwip_accept(listnr, NULL, NULL);
  fail_unless(s2 >= 0);

  /* double check s1 is connected */
  ret = lwip_connect(s1, (struct sockaddr*)&addr_storage, addr_size);
  fail_unless(ret == -1);
  fail_unless(errno == EISCONN);

  /* set s2 to non-blocking, not inherited from listener */
  opt = lwip_fcntl(s2, F_GETFL, 0);
  fail_unless(opt == 6);
  opt = O_NONBLOCK;
  ret = lwip_fcntl(s2, F_SETFL, opt);
  fail_unless(ret == 0);

  /* we are done with listener, close it */
  ret = lwip_close(listnr);
  fail_unless(ret == 0);

  /* allocate a buffer for a stream of incrementing hex (0x00..0xFF) which we will use
     to create an input vector set that is larger than the TCP's send buffer. This will
     force execution of the partial IO vector send case */
  snd_buf = (u8_t*)mem_malloc(BUF_SZ);
  val = 0x00;
  fail_unless(snd_buf != NULL);
  for (i = 0; i < BUF_SZ; i++,val++) {
    snd_buf[i] = val;
  }

  /* send the buffer 8 times in one message, equating to TOTAL_DATA_SZ */
  for (i = 0; i < 8; i++) {
    siovs[i].iov_base = snd_buf;
    siovs[i].iov_len = BUF_SZ;
  }

  /* allocate a receive buffer, same size as snd_buf for easy verification */
  rcv_buf = (u8_t*)mem_calloc(1, BUF_SZ);
  fail_unless(rcv_buf != NULL);
  /* split across iovs */
  for (i = 0; i < 4; i++) {
    riovs[i].iov_base = &rcv_buf[i*(BUF_SZ/4)];
    riovs[i].iov_len = BUF_SZ/4;
  }
  /* handling trailing bytes if buffer doesn't evenly divide by 4 */
#if NEED_TRAILER
  if ((BUF_SZ % 4) != 0) {
    riovs[5].iov_base = &rcv_buf[4*(BUF_SZ/4)];
    riovs[5].iov_len = BUF_SZ - (4*(BUF_SZ/4));
    rcv_trailer = 1;
  }
#endif /* NEED_TRAILER */

  /* we use a copy of riovs since we'll be modifying base and len during
     receiving. This gives us an easy way to reset the iovs for next recvmsg */
  memcpy(riovs_tmp, riovs, sizeof(riovs));

  memset(&smsg, 0, sizeof(smsg));
  smsg.msg_iov = siovs;
  smsg.msg_iovlen = 8;

  memset(&rmsg, 0, sizeof(rmsg));
  rmsg.msg_iov = riovs_tmp;
  rmsg.msg_iovlen = (rcv_trailer ? 5 : 4);

  bytes_written = 0;
  bytes_read = 0;
  rcv_off = 0;

  while (bytes_written < TOTAL_DATA_SZ && (bytes_read < TOTAL_DATA_SZ)) {
    /* send data */
    if (bytes_written < TOTAL_DATA_SZ) {
      ret = lwip_sendmsg(s1, &smsg, 0);
      /* note: since we always receive after sending, there will be open
         space in the send buffer */
      fail_unless(ret > 0);
    
      bytes_written += ret;
      if (bytes_written < TOTAL_DATA_SZ) {
        test_sockets_msgapi_update_iovs(&smsg, (size_t)ret);
      }
    }

    while (tcpip_thread_poll_one());

    /* receive and verify data */
    do {
      if (bytes_read < TOTAL_DATA_SZ) {
        ret = lwip_recvmsg(s2, &rmsg, 0);
        fail_unless(ret > 0 || (ret == -1 && errno == EWOULDBLOCK));

        if (ret > 0) {
          rcv_off += ret;
          /* we have received a full buffer */
          if (rcv_off == BUF_SZ) {
            /* note: since iovs are just pointers, compare underlying buf */
            fail_unless(!memcmp(snd_buf, rcv_buf, BUF_SZ));
            bytes_read += BUF_SZ;
            /* reset receive state for next buffer */
            rcv_off = 0;
            memset(rcv_buf, 0, BUF_SZ);
            memcpy(riovs_tmp, riovs, sizeof(riovs));
            rmsg.msg_iov = riovs_tmp;
            rmsg.msg_iovlen = (rcv_trailer ? 5 : 4);
          } else { /* partial read */
            test_sockets_msgapi_update_iovs(&rmsg, (size_t)ret);
          }
        }
      } else {
        break;
      }
    } while(ret > 0);
  }
  
  ret = lwip_close(s1);
  fail_unless(ret == 0);
  ret = lwip_close(s2);
  fail_unless(ret == 0);
  mem_free(snd_buf);
  mem_free(rcv_buf);
}

static void test_sockets_msgapi_udp_send_recv_loop(int s, struct msghdr *smsg, struct msghdr *rmsg)
{
  int i, ret;

  /* send/receive our datagram of IO vectors 10 times */
  for (i = 0; i < 10; i++) {
    ret = lwip_sendmsg(s, smsg, 0);
    fail_unless(ret == 4);

    while (tcpip_thread_poll_one());

    /* receive the datagram split across 4 buffers */
    ret = lwip_recvmsg(s, rmsg, 0);
    fail_unless(ret == 4);

    /* verify data */
    fail_unless(*((u8_t*)rmsg->msg_iov[0].iov_base) == 0xDE);
    fail_unless(*((u8_t*)rmsg->msg_iov[1].iov_base) == 0xAD);
    fail_unless(*((u8_t*)rmsg->msg_iov[2].iov_base) == 0xBE);
    fail_unless(*((u8_t*)rmsg->msg_iov[3].iov_base) == 0xEF);

    /* clear rcv_buf to ensure no data is being skipped */
    *((u8_t*)rmsg->msg_iov[0].iov_base) = 0x00;
    *((u8_t*)rmsg->msg_iov[1].iov_base) = 0x00;
    *((u8_t*)rmsg->msg_iov[2].iov_base) = 0x00;
    *((u8_t*)rmsg->msg_iov[3].iov_base) = 0x00;
  }
}

static void test_sockets_msgapi_udp(int domain)
{
  int s, i, ret;
  struct sockaddr_storage addr_storage;
  socklen_t addr_size;
  struct iovec riovs[4];
  struct msghdr rmsg;
  u8_t rcv_buf[4];
  struct iovec siovs[4];
  struct msghdr smsg;
  u8_t snd_buf[4] = {0xDE, 0xAD, 0xBE, 0xEF};

  /* initialize IO vectors with data */
  for (i = 0; i < 4; i++) {
    siovs[i].iov_base = &snd_buf[i];
    siovs[i].iov_len = sizeof(u8_t);
    riovs[i].iov_base = &rcv_buf[i];
    riovs[i].iov_len = sizeof(u8_t);
  }

  test_sockets_init_loopback_addr(domain, &addr_storage, &addr_size);

  s = test_sockets_alloc_socket_nonblocking(domain, SOCK_DGRAM);
  fail_unless(s >= 0);

  ret = lwip_bind(s, (struct sockaddr*)&addr_storage, addr_size);
  fail_unless(ret == 0);

  /* Update addr with epehermal port */
  ret = lwip_getsockname(s, (struct sockaddr*)&addr_storage, &addr_size);
  fail_unless(ret == 0);
  switch(domain) {
#if LWIP_IPV6
    case AF_INET6:
      fail_unless(addr_size == sizeof(struct sockaddr_in6));
      break;
#endif /* LWIP_IPV6 */
#if LWIP_IPV4
    case AF_INET:
        fail_unless(addr_size == sizeof(struct sockaddr_in));
        break;
#endif /* LWIP_IPV6 */
    default:
      fail();
      break;
  }

  /* send and receive the datagram in 4 pieces */
  memset(&smsg, 0, sizeof(smsg));
  smsg.msg_iov = siovs;
  smsg.msg_iovlen = 4;
  memset(&rmsg, 0, sizeof(rmsg));
  rmsg.msg_iov = riovs;
  rmsg.msg_iovlen = 4;

  /* perform a sendmsg with remote host (self) */
  smsg.msg_name = &addr_storage;
  smsg.msg_namelen = addr_size;

  test_sockets_msgapi_udp_send_recv_loop(s, &smsg, &rmsg);

  /* Connect to self, allowing us to not pass message name */
  ret = lwip_connect(s, (struct sockaddr*)&addr_storage, addr_size);
  fail_unless(ret == 0);

  smsg.msg_name = NULL;
  smsg.msg_namelen = 0;

  test_sockets_msgapi_udp_send_recv_loop(s, &smsg, &rmsg);

  ret = lwip_close(s);
  fail_unless(ret == 0);
}

#if LWIP_IPV4
static void test_sockets_msgapi_cmsg(int domain)
{
  int s, ret, enable;
  struct sockaddr_storage addr_storage;
  socklen_t addr_size;
  struct iovec iov;
  struct msghdr msg;
  struct cmsghdr *cmsg;
  struct in_pktinfo *pktinfo;
  u8_t rcv_buf[4];
  u8_t snd_buf[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  u8_t cmsg_buf[CMSG_SPACE(sizeof(struct in_pktinfo))];

  test_sockets_init_loopback_addr(domain, &addr_storage, &addr_size);

  s = test_sockets_alloc_socket_nonblocking(domain, SOCK_DGRAM);
  fail_unless(s >= 0);

  ret = lwip_bind(s, (struct sockaddr*)&addr_storage, addr_size);
  fail_unless(ret == 0);

  /* Update addr with epehermal port */
  ret = lwip_getsockname(s, (struct sockaddr*)&addr_storage, &addr_size);
  fail_unless(ret == 0);

  enable = 1;
  ret = lwip_setsockopt(s, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable));
  fail_unless(ret == 0);

  /* Receive full message, including control message */
  iov.iov_base = rcv_buf;
  iov.iov_len = sizeof(rcv_buf);
  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof(cmsg_buf);
  msg.msg_flags = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = NULL;
  msg.msg_namelen = 0;

  memset(rcv_buf, 0, sizeof(rcv_buf));
  ret = lwip_sendto(s, snd_buf, sizeof(snd_buf), 0, (struct sockaddr*)&addr_storage, addr_size);
  fail_unless(ret == sizeof(snd_buf));
  
  tcpip_thread_poll_one();

  ret = lwip_recvmsg(s, &msg, 0);
  fail_unless(ret == sizeof(rcv_buf));
  fail_unless(!memcmp(rcv_buf, snd_buf, sizeof(rcv_buf)));
  
  /* Verify message header */
  cmsg = CMSG_FIRSTHDR(&msg);
  fail_unless(cmsg != NULL);
  fail_unless(cmsg->cmsg_len > 0);
  fail_unless(cmsg->cmsg_level == IPPROTO_IP);
  fail_unless(cmsg->cmsg_type == IP_PKTINFO);

  /* Verify message data */
  pktinfo = (struct in_pktinfo*)CMSG_DATA(cmsg);
  /* We only have loopback interface enabled */
  fail_unless(pktinfo->ipi_ifindex == 1);
  fail_unless(pktinfo->ipi_addr.s_addr == PP_HTONL(INADDR_LOOPBACK));

  /* Verify there are no additional messages */
  cmsg = CMSG_NXTHDR(&msg, cmsg);
  fail_unless(cmsg == NULL);

  /* Send datagram again, testing truncation */
  memset(rcv_buf, 0, sizeof(rcv_buf));
  ret = lwip_sendto(s, snd_buf, sizeof(snd_buf), 0, (struct sockaddr*)&addr_storage, addr_size);
  fail_unless(ret == sizeof(snd_buf));

  tcpip_thread_poll_one();

  msg.msg_controllen = 1;
  msg.msg_flags = 0;
  ret = lwip_recvmsg(s, &msg, 0);
  fail_unless(ret == sizeof(rcv_buf));
  fail_unless(!memcmp(rcv_buf, snd_buf, sizeof(rcv_buf)));
  /* Ensure truncation was returned */
  fail_unless(msg.msg_flags & MSG_CTRUNC);
  /* Ensure no control messages were returned */
  fail_unless(msg.msg_controllen == 0);

  ret = lwip_close(s);
  fail_unless(ret == 0);
}
#endif /* LWIP_IPV4 */

START_TEST(test_sockets_msgapis)
{
  LWIP_UNUSED_ARG(_i);
#if LWIP_IPV4
  test_sockets_msgapi_udp(AF_INET);
  test_sockets_msgapi_tcp(AF_INET);
  test_sockets_msgapi_cmsg(AF_INET);
#endif
#if LWIP_IPV6
  test_sockets_msgapi_udp(AF_INET6);
  test_sockets_msgapi_tcp(AF_INET6);
#endif
}
END_TEST

START_TEST(test_sockets_select)
{
#if LWIP_SOCKET_SELECT
  int s;
  int ret;
  fd_set readset;
  fd_set writeset;
  fd_set errset;
  struct timeval tv;

  fail_unless(test_sockets_get_used_count() == 0);

  s = lwip_socket(AF_INET, SOCK_STREAM, 0);
  fail_unless(s >= 0);
  fail_unless(test_sockets_get_used_count() == 0);

  FD_ZERO(&readset);
  FD_SET(s, &readset);
  FD_ZERO(&writeset);
  FD_SET(s, &writeset);
  FD_ZERO(&errset);
  FD_SET(s, &errset);

  tv.tv_sec = tv.tv_usec = 0;
  ret = lwip_select(s + 1, &readset, &writeset, &errset, &tv);
  fail_unless(ret == 0);
  fail_unless(test_sockets_get_used_count() == 0);

  ret = lwip_close(s);
  fail_unless(ret == 0);

#endif
  LWIP_UNUSED_ARG(_i);
}
END_TEST

START_TEST(test_sockets_recv_after_rst)
{
  int sl, sact;
  int spass = -1;
  int ret;
  struct sockaddr_in sa_listen;
  const u16_t port = 1234;
  int arg;
  const char txbuf[] = "something";
  char rxbuf[16];
  struct lwip_sock *sact_sock;
  int err;
  LWIP_UNUSED_ARG(_i);

  fail_unless(test_sockets_get_used_count() == 0);

  memset(&sa_listen, 0, sizeof(sa_listen));
  sa_listen.sin_family = AF_INET;
  sa_listen.sin_port = PP_HTONS(port);
  sa_listen.sin_addr.s_addr = PP_HTONL(INADDR_LOOPBACK);

  /* set up the listener */
  sl = lwip_socket(AF_INET, SOCK_STREAM, 0);
  fail_unless(sl >= 0);
  fail_unless(test_sockets_get_used_count() == 0);

  ret = lwip_bind(sl, (struct sockaddr *)&sa_listen, sizeof(sa_listen));
  fail_unless(ret == 0);
  ret = lwip_listen(sl, 0);
  fail_unless(ret == 0);

  /* set up the client */
  sact = lwip_socket(AF_INET, SOCK_STREAM, 0);
  fail_unless(sact >= 0);
  fail_unless(test_sockets_get_used_count() == 0);
  /* set the client to nonblocking to simplify this test */
  arg = 1;
  ret = lwip_ioctl(sact, FIONBIO, &arg);
  fail_unless(ret == 0);
  /* connect */
  do {
    ret = lwip_connect(sact, (struct sockaddr *)&sa_listen, sizeof(sa_listen));
    err = errno;
    fail_unless((ret == 0) || (ret == -1));
    if (ret != 0) {
      if (err == EISCONN) {
        /* Although this is not valid, use EISCONN as an indicator for successful connection.
           This marks us as "connect phase is done". On error, we would either have a different
           errno code or "send" fails later... -> good enough for this test. */
        ret = 0;
      } else {
        fail_unless(err == EINPROGRESS);
        if (err != EINPROGRESS) {
          goto cleanup;
        }
        /* we're in progress: little side check: test for EALREADY */
        ret = lwip_connect(sact, (struct sockaddr *)&sa_listen, sizeof(sa_listen));
        err = errno;
        fail_unless(ret == -1);
        fail_unless(err == EALREADY);
        if ((ret != -1) || (err != EALREADY)) {
          goto cleanup;
        }
      }
      tcpip_thread_poll_one();
      tcpip_thread_poll_one();
      tcpip_thread_poll_one();
      tcpip_thread_poll_one();
    }
  } while (ret != 0);
  fail_unless(ret == 0);

  /* accept the server connection part */
  spass = lwip_accept(sl, NULL, NULL);
  fail_unless(spass >= 0);

  /* write data from client */
  ret = lwip_send(sact, txbuf, sizeof(txbuf), 0);
  fail_unless(ret == sizeof(txbuf));

  tcpip_thread_poll_one();
  tcpip_thread_poll_one();

  /* issue RST (This is a HACK, don't try this in your own app!) */
  sact_sock = lwip_socket_dbg_get_socket(sact);
  fail_unless(sact_sock != NULL);
  if (sact_sock != NULL) {
    struct netconn *sact_conn = sact_sock->conn;
    fail_unless(sact_conn != NULL);
    if (sact_conn != NULL) {
      struct tcp_pcb *pcb = sact_conn->pcb.tcp;
      fail_unless(pcb != NULL);
      if (pcb != NULL) {
        tcp_rst(pcb, pcb->snd_nxt, pcb->rcv_nxt, &pcb->local_ip, &pcb->remote_ip,
                     pcb->local_port, pcb->remote_port);
      }
    }
  }
  tcpip_thread_poll_one();
  tcpip_thread_poll_one();

  /* expect to receive data first */
  ret = lwip_recv(spass, rxbuf, sizeof(rxbuf), 0);
  fail_unless(ret > 0);
  tcpip_thread_poll_one();
  tcpip_thread_poll_one();

  /* expect to receive RST indication */
  ret = lwip_recv(spass, rxbuf, sizeof(rxbuf), 0);
  fail_unless(ret == -1);
  err = errno;
  fail_unless(err == ECONNRESET);
  tcpip_thread_poll_one();
  tcpip_thread_poll_one();

  /* expect to receive ENOTCONN indication */
  ret = lwip_recv(spass, rxbuf, sizeof(rxbuf), 0);
  fail_unless(ret == -1);
  err = errno;
  fail_unless(err == ENOTCONN);
  tcpip_thread_poll_one();
  tcpip_thread_poll_one();

  /* expect to receive ENOTCONN indication */
  ret = lwip_recv(spass, rxbuf, sizeof(rxbuf), 0);
  fail_unless(ret == -1);
  err = errno;
  fail_unless(err == ENOTCONN);
  tcpip_thread_poll_one();
  tcpip_thread_poll_one();

cleanup:
  ret = lwip_close(sl);
  fail_unless(ret == 0);
  ret = lwip_close(sact);
  fail_unless(ret == 0);
  if (spass >= 0) {
    ret = lwip_close(spass);
    fail_unless(ret == 0);
  }
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
sockets_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_sockets_basics),
    TESTFUNC(test_sockets_allfunctions_basic),
    TESTFUNC(test_sockets_msgapis),
    TESTFUNC(test_sockets_select),
    TESTFUNC(test_sockets_recv_after_rst),
  };
  return create_suite("SOCKETS", tests, sizeof(tests)/sizeof(testfunc), sockets_setup, sockets_teardown);
}

#else /* LWIP_SOCKET */

Suite *
sockets_suite(void)
{
  return create_suite("SOCKETS", NULL, 0, NULL, NULL);
}
#endif /* LWIP_SOCKET */
