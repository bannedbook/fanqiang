#include "test_tcp.h"

#include "lwip/priv/tcp_priv.h"
#include "lwip/stats.h"
#include "tcp_helper.h"
#include "lwip/inet_chksum.h"

#ifdef _MSC_VER
#pragma warning(disable: 4307) /* we explicitly wrap around TCP seqnos */
#endif

#if !LWIP_STATS || !TCP_STATS || !MEMP_STATS
#error "This tests needs TCP- and MEMP-statistics enabled"
#endif
#if TCP_SND_BUF <= TCP_WND
#error "This tests needs TCP_SND_BUF to be > TCP_WND"
#endif

/* used with check_seqnos() */
#define SEQNO1 (0xFFFFFF00 - TCP_MSS)
#define ISS    6510
static u32_t seqnos[] = {
    SEQNO1,
    SEQNO1 + (1 * TCP_MSS),
    SEQNO1 + (2 * TCP_MSS),
    SEQNO1 + (3 * TCP_MSS),
    SEQNO1 + (4 * TCP_MSS),
    SEQNO1 + (5 * TCP_MSS) };

static u8_t test_tcp_timer;

/* our own version of tcp_tmr so we can reset fast/slow timer state */
static void
test_tcp_tmr(void)
{
  tcp_fasttmr();
  if (++test_tcp_timer & 1) {
    tcp_slowtmr();
  }
}

/* Setups/teardown functions */
static struct netif *old_netif_list;
static struct netif *old_netif_default;

static void
tcp_setup(void)
{
  old_netif_list = netif_list;
  old_netif_default = netif_default;
  netif_list = NULL;
  netif_default = NULL;
  /* reset iss to default (6510) */
  tcp_ticks = 0;
  tcp_ticks = 0 - (tcp_next_iss(NULL) - 6510);
  tcp_next_iss(NULL);
  tcp_ticks = 0;

  test_tcp_timer = 0;
  tcp_remove_all();
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
tcp_teardown(void)
{
  netif_list = NULL;
  netif_default = NULL;
  tcp_remove_all();
  /* restore netif_list for next tests (e.g. loopif) */
  netif_list = old_netif_list;
  netif_default = old_netif_default;
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}


/* Test functions */

/** Call tcp_new() and tcp_abort() and test memp stats */
START_TEST(test_tcp_new_abort)
{
  struct tcp_pcb* pcb;
  LWIP_UNUSED_ARG(_i);

  fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);

  pcb = tcp_new();
  fail_unless(pcb != NULL);
  if (pcb != NULL) {
    fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
    tcp_abort(pcb);
    fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
  }
}
END_TEST

/** Call tcp_new() and tcp_abort() and test memp stats */
START_TEST(test_tcp_listen_passive_open)
{
  struct tcp_pcb *pcb, *pcbl;
  struct tcp_pcb_listen *lpcb;
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct test_tcp_counters counters;
  struct pbuf *p;
  ip_addr_t src_addr;
  err_t err;
  LWIP_UNUSED_ARG(_i);

  fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);

  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  /* initialize counter struct */
  memset(&counters, 0, sizeof(counters));

  pcb = tcp_new();
  EXPECT_RET(pcb != NULL);
  err = tcp_bind(pcb, &netif.ip_addr, 1234);
  EXPECT(err == ERR_OK);
  pcbl = tcp_listen(pcb);
  EXPECT_RET(pcbl != NULL);
  EXPECT_RET(pcbl != pcb);
  lpcb = (struct tcp_pcb_listen *)pcbl;

  ip_addr_set_ip4_u32_val(src_addr, lwip_htonl(lwip_ntohl(ip_addr_get_ip4_u32(&lpcb->local_ip)) + 1));

  /* check correct syn packet */
  p = tcp_create_segment(&src_addr, &lpcb->local_ip, 12345,
    lpcb->local_port, NULL, 0, 12345, 54321, TCP_SYN);
  EXPECT(p != NULL);
  if (p != NULL) {
    /* pass the segment to tcp_input */
    test_tcp_input(p, &netif);
    /* check if counters are as expected */
    EXPECT(txcounters.num_tx_calls == 1);
  }

  /* chekc syn packet with short length */
  p = tcp_create_segment(&src_addr, &lpcb->local_ip, 12345,
    lpcb->local_port, NULL, 0, 12345, 54321, TCP_SYN);
  EXPECT(p != NULL);
  EXPECT(p->next == NULL);
  if ((p != NULL) && (p->next == NULL)) {
    p->len -= 2;
    p->tot_len -= 2;
    /* pass the segment to tcp_input */
    test_tcp_input(p, &netif);
    /* check if counters are as expected */
    EXPECT(txcounters.num_tx_calls == 1);
  }

  tcp_close(pcbl);
}
END_TEST

/** Create an ESTABLISHED pcb and check if receive callback is called */
START_TEST(test_tcp_recv_inseq)
{
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  char data[] = {1, 2, 3, 4};
  u16_t data_len;
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  LWIP_UNUSED_ARG(_i);

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  data_len = sizeof(data);
  /* initialize counter struct */
  memset(&counters, 0, sizeof(counters));
  counters.expected_data_len = data_len;
  counters.expected_data = data;

  /* create and initialize the pcb */
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);

  /* create a segment */
  p = tcp_create_rx_segment(pcb, counters.expected_data, data_len, 0, 0, 0);
  EXPECT(p != NULL);
  if (p != NULL) {
    /* pass the segment to tcp_input */
    test_tcp_input(p, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 1);
    EXPECT(counters.recved_bytes == data_len);
    EXPECT(counters.err_calls == 0);
  }

  /* make sure the pcb is freed */
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

/** Create an ESTABLISHED pcb and check if receive callback is called if a segment
 * overlapping rcv_nxt is received */
START_TEST(test_tcp_recv_inseq_trim)
{
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  char data[PBUF_POOL_BUFSIZE*2];
  u16_t data_len;
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  const u32_t new_data_len = 40;
  LWIP_UNUSED_ARG(_i);

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  data_len = sizeof(data);
  memset(data, 0, sizeof(data));
  /* initialize counter struct */
  memset(&counters, 0, sizeof(counters));
  counters.expected_data_len = data_len;
  counters.expected_data = data;

  /* create and initialize the pcb */
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);

  /* create a segment (with an overlapping/old seqno so that the new data begins in the 2nd pbuf) */
  p = tcp_create_rx_segment(pcb, counters.expected_data, data_len, (u32_t)(0-(data_len-new_data_len)), 0, 0);
  EXPECT(p != NULL);
  if (p != NULL) {
    EXPECT(p->next != NULL);
    if (p->next != NULL) {
      EXPECT(p->next->next != NULL);
    }
  }
  if ((p != NULL) && (p->next != NULL) && (p->next->next != NULL)) {
    /* pass the segment to tcp_input */
    test_tcp_input(p, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 1);
    EXPECT(counters.recved_bytes == new_data_len);
    EXPECT(counters.err_calls == 0);
  }

  /* make sure the pcb is freed */
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

static err_t test_tcp_recv_expect1byte(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err);

static err_t
test_tcp_recv_expectclose(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err)
{
  EXPECT_RETX(pcb != NULL, ERR_OK);
  EXPECT_RETX(err == ERR_OK, ERR_OK);
  LWIP_UNUSED_ARG(arg);

  if (p != NULL) {
    fail();
    pbuf_free(p);
  } else {
    /* correct: FIN received; close our end, too */
    err_t err2 = tcp_close(pcb);
    fail_unless(err2 == ERR_OK);
    /* set back to some other rx function, just to not get here again */
    tcp_recv(pcb, test_tcp_recv_expect1byte);
  }
  return ERR_OK;
}

static err_t
test_tcp_recv_expect1byte(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err)
{
  EXPECT_RETX(pcb != NULL, ERR_OK);
  EXPECT_RETX(err == ERR_OK, ERR_OK);
  LWIP_UNUSED_ARG(arg);

  if (p != NULL) {
    if ((p->len == 1) && (p->tot_len == 1)) {
      tcp_recv(pcb, test_tcp_recv_expectclose);
    } else {
      fail();
    }
    pbuf_free(p);
  } else {
    fail();
  }
  return ERR_OK;
}

START_TEST(test_tcp_passive_close)
{
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  char data = 0x0f;
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  LWIP_UNUSED_ARG(_i);

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);

  /* initialize counter struct */
  memset(&counters, 0, sizeof(counters));
  counters.expected_data_len = 1;
  counters.expected_data = &data;

  /* create and initialize the pcb */
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);

  /* create a segment without data */
  p = tcp_create_rx_segment(pcb, &data, 1, 0, 0, TCP_FIN);
  EXPECT(p != NULL);
  if (p != NULL) {
    tcp_recv(pcb, test_tcp_recv_expect1byte);
    /* pass the segment to tcp_input */
    test_tcp_input(p, &netif);
  }
  /* don't free the pcb here (part of the test!) */
}
END_TEST

/** Check that we handle malformed tcp headers, and discard the pbuf(s) */
START_TEST(test_tcp_malformed_header)
{
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  char data[] = {1, 2, 3, 4};
  u16_t data_len, chksum;
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct tcp_hdr *hdr;
  LWIP_UNUSED_ARG(_i);

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  data_len = sizeof(data);
  /* initialize counter struct */
  memset(&counters, 0, sizeof(counters));
  counters.expected_data_len = data_len;
  counters.expected_data = data;

  /* create and initialize the pcb */
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);

  /* create a segment */
  p = tcp_create_rx_segment(pcb, counters.expected_data, data_len, 0, 0, 0);

  pbuf_header(p, -(s16_t)sizeof(struct ip_hdr));

  hdr = (struct tcp_hdr *)p->payload;
  TCPH_HDRLEN_FLAGS_SET(hdr, 15, 0x3d1);

  hdr->chksum = 0;

  chksum = ip_chksum_pseudo(p, IP_PROTO_TCP, p->tot_len,
                             &test_remote_ip, &test_local_ip);

  hdr->chksum = chksum;

  pbuf_header(p, sizeof(struct ip_hdr));

  EXPECT(p != NULL);
  EXPECT(p->next == NULL);
  if (p != NULL) {
    /* pass the segment to tcp_input */
    test_tcp_input(p, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
  }

  /* make sure the pcb is freed */
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST


/** Provoke fast retransmission by duplicate ACKs and then recover by ACKing all sent data.
 * At the end, send more data. */
START_TEST(test_tcp_fast_retx_recover)
{
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  char data1[] = { 1,  2,  3,  4};
  char data2[] = { 5,  6,  7,  8};
  char data3[] = { 9, 10, 11, 12};
  char data4[] = {13, 14, 15, 16};
  char data5[] = {17, 18, 19, 20};
  char data6[TCP_MSS] = {21, 22, 23, 24};
  err_t err;
  LWIP_UNUSED_ARG(_i);

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  memset(&counters, 0, sizeof(counters));

  /* create and initialize the pcb */
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  pcb->mss = TCP_MSS;
  /* disable initial congestion window (we don't send a SYN here...) */
  pcb->cwnd = pcb->snd_wnd;

  /* send data1 */
  err = tcp_write(pcb, data1, sizeof(data1), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  EXPECT_RET(txcounters.num_tx_calls == 1);
  EXPECT_RET(txcounters.num_tx_bytes == sizeof(data1) + sizeof(struct tcp_hdr) + sizeof(struct ip_hdr));
  memset(&txcounters, 0, sizeof(txcounters));
 /* "recv" ACK for data1 */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 4, TCP_ACK);
  EXPECT_RET(p != NULL);
  test_tcp_input(p, &netif);
  EXPECT_RET(txcounters.num_tx_calls == 0);
  EXPECT_RET(pcb->unacked == NULL);
  /* send data2 */
  err = tcp_write(pcb, data2, sizeof(data2), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  EXPECT_RET(txcounters.num_tx_calls == 1);
  EXPECT_RET(txcounters.num_tx_bytes == sizeof(data2) + sizeof(struct tcp_hdr) + sizeof(struct ip_hdr));
  memset(&txcounters, 0, sizeof(txcounters));
  /* duplicate ACK for data1 (data2 is lost) */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 0, TCP_ACK);
  EXPECT_RET(p != NULL);
  test_tcp_input(p, &netif);
  EXPECT_RET(txcounters.num_tx_calls == 0);
  EXPECT_RET(pcb->dupacks == 1);
  /* send data3 */
  err = tcp_write(pcb, data3, sizeof(data3), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  /* nagle enabled, no tx calls */
  EXPECT_RET(txcounters.num_tx_calls == 0);
  EXPECT_RET(txcounters.num_tx_bytes == 0);
  memset(&txcounters, 0, sizeof(txcounters));
  /* 2nd duplicate ACK for data1 (data2 and data3 are lost) */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 0, TCP_ACK);
  EXPECT_RET(p != NULL);
  test_tcp_input(p, &netif);
  EXPECT_RET(txcounters.num_tx_calls == 0);
  EXPECT_RET(pcb->dupacks == 2);
  /* queue data4, don't send it (unsent-oversize is != 0) */
  err = tcp_write(pcb, data4, sizeof(data4), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  /* 3nd duplicate ACK for data1 (data2 and data3 are lost) -> fast retransmission */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 0, TCP_ACK);
  EXPECT_RET(p != NULL);
  test_tcp_input(p, &netif);
  /*EXPECT_RET(txcounters.num_tx_calls == 1);*/
  EXPECT_RET(pcb->dupacks == 3);
  memset(&txcounters, 0, sizeof(txcounters));
  /* @todo: check expected data?*/
  
  /* send data5, not output yet */
  err = tcp_write(pcb, data5, sizeof(data5), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  /*err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);*/
  EXPECT_RET(txcounters.num_tx_calls == 0);
  EXPECT_RET(txcounters.num_tx_bytes == 0);
  memset(&txcounters, 0, sizeof(txcounters));
  {
    int i = 0;
    do
    {
      err = tcp_write(pcb, data6, TCP_MSS, TCP_WRITE_FLAG_COPY);
      i++;
    }while(err == ERR_OK);
    EXPECT_RET(err != ERR_OK);
  }
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  /*EXPECT_RET(txcounters.num_tx_calls == 0);
  EXPECT_RET(txcounters.num_tx_bytes == 0);*/
  memset(&txcounters, 0, sizeof(txcounters));

  /* send even more data */
  err = tcp_write(pcb, data5, sizeof(data5), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  /* ...and even more data */
  err = tcp_write(pcb, data5, sizeof(data5), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  /* ...and even more data */
  err = tcp_write(pcb, data5, sizeof(data5), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  /* ...and even more data */
  err = tcp_write(pcb, data5, sizeof(data5), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);

  /* send ACKs for data2 and data3 */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 12, TCP_ACK);
  EXPECT_RET(p != NULL);
  test_tcp_input(p, &netif);
  /*EXPECT_RET(txcounters.num_tx_calls == 0);*/

  /* ...and even more data */
  err = tcp_write(pcb, data5, sizeof(data5), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  /* ...and even more data */
  err = tcp_write(pcb, data5, sizeof(data5), TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);

#if 0
  /* create expected segment */
  p1 = tcp_create_rx_segment(pcb, counters.expected_data, data_len, 0, 0, 0);
  EXPECT_RET(p != NULL);
  if (p != NULL) {
    /* pass the segment to tcp_input */
    test_tcp_input(p, &netif);
    /* check if counters are as expected */
    EXPECT_RET(counters.close_calls == 0);
    EXPECT_RET(counters.recv_calls == 1);
    EXPECT_RET(counters.recved_bytes == data_len);
    EXPECT_RET(counters.err_calls == 0);
  }
#endif
  /* make sure the pcb is freed */
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

static u8_t tx_data[TCP_WND*2];

static void
check_seqnos(struct tcp_seg *segs, int num_expected, u32_t *seqnos_expected)
{
  struct tcp_seg *s = segs;
  int i;
  for (i = 0; i < num_expected; i++, s = s->next) {
    EXPECT_RET(s != NULL);
    EXPECT(s->tcphdr->seqno == htonl(seqnos_expected[i]));
  }
  EXPECT(s == NULL);
}

/** Send data with sequence numbers that wrap around the u32_t range.
 * Then, provoke fast retransmission by duplicate ACKs and check that all
 * segment lists are still properly sorted. */
START_TEST(test_tcp_fast_rexmit_wraparound)
{
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  err_t err;
  u16_t i, sent_total = 0;
  LWIP_UNUSED_ARG(_i);

  for (i = 0; i < sizeof(tx_data); i++) {
    tx_data[i] = (u8_t)i;
  }

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  memset(&counters, 0, sizeof(counters));

  /* create and initialize the pcb */
  tcp_ticks = SEQNO1 - ISS;
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  pcb->mss = TCP_MSS;
  /* disable initial congestion window (we don't send a SYN here...) */
  pcb->cwnd = 2*TCP_MSS;
  /* start in congestion advoidance */
  pcb->ssthresh = pcb->cwnd;

  /* send 6 mss-sized segments */
  for (i = 0; i < 6; i++) {
    err = tcp_write(pcb, &tx_data[sent_total], TCP_MSS, TCP_WRITE_FLAG_COPY);
    EXPECT_RET(err == ERR_OK);
    sent_total += TCP_MSS;
  }
  check_seqnos(pcb->unsent, 6, seqnos);
  EXPECT(pcb->unacked == NULL);
  err = tcp_output(pcb);
  EXPECT(txcounters.num_tx_calls == 2);
  EXPECT(txcounters.num_tx_bytes == 2 * (TCP_MSS + 40U));
  memset(&txcounters, 0, sizeof(txcounters));

  check_seqnos(pcb->unacked, 2, seqnos);
  check_seqnos(pcb->unsent, 4, &seqnos[2]);

  /* ACK the first segment */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, TCP_MSS, TCP_ACK);
  test_tcp_input(p, &netif);
  /* ensure this didn't trigger a retransmission. Only one
  segment should be transmitted because cwnd opened up by
  TCP_MSS and a fraction since we are in congestion avoidance */
  EXPECT(txcounters.num_tx_calls == 1);
  EXPECT(txcounters.num_tx_bytes == TCP_MSS + 40U);
  memset(&txcounters, 0, sizeof(txcounters));
  check_seqnos(pcb->unacked, 2, &seqnos[1]);
  check_seqnos(pcb->unsent, 3, &seqnos[3]);

  /* 3 dupacks */
  EXPECT(pcb->dupacks == 0);
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 0, TCP_ACK);
  test_tcp_input(p, &netif);
  EXPECT(txcounters.num_tx_calls == 0);
  EXPECT(pcb->dupacks == 1);
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 0, TCP_ACK);
  test_tcp_input(p, &netif);
  EXPECT(txcounters.num_tx_calls == 0);
  EXPECT(pcb->dupacks == 2);
  /* 3rd dupack -> fast rexmit */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 0, TCP_ACK);
  test_tcp_input(p, &netif);
  EXPECT(pcb->dupacks == 3);
  EXPECT(txcounters.num_tx_calls == 4);
  memset(&txcounters, 0, sizeof(txcounters));
  EXPECT(pcb->unsent == NULL);
  check_seqnos(pcb->unacked, 5, &seqnos[1]);

  /* make sure the pcb is freed */
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

/** Send data with sequence numbers that wrap around the u32_t range.
 * Then, provoke RTO retransmission and check that all
 * segment lists are still properly sorted. */
START_TEST(test_tcp_rto_rexmit_wraparound)
{
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  err_t err;
  u16_t i, sent_total = 0;
  LWIP_UNUSED_ARG(_i);

  for (i = 0; i < sizeof(tx_data); i++) {
    tx_data[i] = (u8_t)i;
  }

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  memset(&counters, 0, sizeof(counters));

  /* create and initialize the pcb */
  tcp_ticks = 0;
  tcp_ticks = 0 - tcp_next_iss(NULL);
  tcp_ticks = SEQNO1 - tcp_next_iss(NULL);
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  pcb->mss = TCP_MSS;
  /* disable initial congestion window (we don't send a SYN here...) */
  pcb->cwnd = 2*TCP_MSS;

  /* send 6 mss-sized segments */
  for (i = 0; i < 6; i++) {
    err = tcp_write(pcb, &tx_data[sent_total], TCP_MSS, TCP_WRITE_FLAG_COPY);
    EXPECT_RET(err == ERR_OK);
    sent_total += TCP_MSS;
  }
  check_seqnos(pcb->unsent, 6, seqnos);
  EXPECT(pcb->unacked == NULL);
  err = tcp_output(pcb);
  EXPECT(txcounters.num_tx_calls == 2);
  EXPECT(txcounters.num_tx_bytes == 2 * (TCP_MSS + 40U));
  memset(&txcounters, 0, sizeof(txcounters));

  check_seqnos(pcb->unacked, 2, seqnos);
  check_seqnos(pcb->unsent, 4, &seqnos[2]);

  /* call the tcp timer some times */
  for (i = 0; i < 10; i++) {
    test_tcp_tmr();
    EXPECT(txcounters.num_tx_calls == 0);
  }
  /* 11th call to tcp_tmr: RTO rexmit fires */
  test_tcp_tmr();
  EXPECT(txcounters.num_tx_calls == 1);
  check_seqnos(pcb->unacked, 1, seqnos);
  check_seqnos(pcb->unsent, 5, &seqnos[1]);

  /* fake greater cwnd */
  pcb->cwnd = pcb->snd_wnd;
  /* send more data */
  err = tcp_output(pcb);
  EXPECT(err == ERR_OK);
  /* check queues are sorted */
  EXPECT(pcb->unsent == NULL);
  check_seqnos(pcb->unacked, 6, seqnos);

  /* make sure the pcb is freed */
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

/** Provoke fast retransmission by duplicate ACKs and then recover by ACKing all sent data.
 * At the end, send more data. */
static void test_tcp_tx_full_window_lost(u8_t zero_window_probe_from_unsent)
{
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf *p;
  err_t err;
  u16_t sent_total, i;
  u8_t expected = 0xFE;

  for (i = 0; i < sizeof(tx_data); i++) {
    u8_t d = (u8_t)i;
    if (d == 0xFE) {
      d = 0xF0;
    }
    tx_data[i] = d;
  }
  if (zero_window_probe_from_unsent) {
    tx_data[TCP_WND] = expected;
  } else {
    tx_data[0] = expected;
  }

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  memset(&counters, 0, sizeof(counters));

  /* create and initialize the pcb */
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  pcb->mss = TCP_MSS;
  /* disable initial congestion window (we don't send a SYN here...) */
  pcb->cwnd = pcb->snd_wnd;

  /* send a full window (minus 1 packets) of TCP data in MSS-sized chunks */
  sent_total = 0;
  if ((TCP_WND - TCP_MSS) % TCP_MSS != 0) {
    u16_t initial_data_len = (TCP_WND - TCP_MSS) % TCP_MSS;
    err = tcp_write(pcb, &tx_data[sent_total], initial_data_len, TCP_WRITE_FLAG_COPY);
    EXPECT_RET(err == ERR_OK);
    err = tcp_output(pcb);
    EXPECT_RET(err == ERR_OK);
    EXPECT(txcounters.num_tx_calls == 1);
    EXPECT(txcounters.num_tx_bytes == initial_data_len + 40U);
    memset(&txcounters, 0, sizeof(txcounters));
    sent_total += initial_data_len;
  }
  for (; sent_total < (TCP_WND - TCP_MSS); sent_total += TCP_MSS) {
    err = tcp_write(pcb, &tx_data[sent_total], TCP_MSS, TCP_WRITE_FLAG_COPY);
    EXPECT_RET(err == ERR_OK);
    err = tcp_output(pcb);
    EXPECT_RET(err == ERR_OK);
    EXPECT(txcounters.num_tx_calls == 1);
    EXPECT(txcounters.num_tx_bytes == TCP_MSS + 40U);
    memset(&txcounters, 0, sizeof(txcounters));
  }
  EXPECT(sent_total == (TCP_WND - TCP_MSS));

  /* now ACK the packet before the first */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 0, TCP_ACK);
  test_tcp_input(p, &netif);
  /* ensure this didn't trigger a retransmission */
  EXPECT(txcounters.num_tx_calls == 0);
  EXPECT(txcounters.num_tx_bytes == 0);

  EXPECT(pcb->persist_backoff == 0);
  /* send the last packet, now a complete window has been sent */
  err = tcp_write(pcb, &tx_data[sent_total], TCP_MSS, TCP_WRITE_FLAG_COPY);
  sent_total += TCP_MSS;
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  EXPECT(txcounters.num_tx_calls == 1);
  EXPECT(txcounters.num_tx_bytes == TCP_MSS + 40U);
  memset(&txcounters, 0, sizeof(txcounters));
  EXPECT(pcb->persist_backoff == 0);

  if (zero_window_probe_from_unsent) {
    /* ACK all data but close the TX window */
    p = tcp_create_rx_segment_wnd(pcb, NULL, 0, 0, TCP_WND, TCP_ACK, 0);
    test_tcp_input(p, &netif);
    /* ensure this didn't trigger any transmission */
    EXPECT(txcounters.num_tx_calls == 0);
    EXPECT(txcounters.num_tx_bytes == 0);
    /* window is completely full, but persist timer is off since send buffer is empty */
    EXPECT(pcb->snd_wnd == 0);
    EXPECT(pcb->persist_backoff == 0);
  }

  /* send one byte more (out of window) -> persist timer starts */
  err = tcp_write(pcb, &tx_data[sent_total], 1, TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT_RET(err == ERR_OK);
  EXPECT(txcounters.num_tx_calls == 0);
  EXPECT(txcounters.num_tx_bytes == 0);
  memset(&txcounters, 0, sizeof(txcounters));
  if (!zero_window_probe_from_unsent) {
    /* no persist timer unless a zero window announcement has been received */
    EXPECT(pcb->persist_backoff == 0);
  } else {
    EXPECT(pcb->persist_backoff == 1);

    /* call tcp_timer some more times to let persist timer count up */
    for (i = 0; i < 4; i++) {
      test_tcp_tmr();
      EXPECT(txcounters.num_tx_calls == 0);
      EXPECT(txcounters.num_tx_bytes == 0);
    }

    /* this should trigger the zero-window-probe */
    txcounters.copy_tx_packets = 1;
    test_tcp_tmr();
    txcounters.copy_tx_packets = 0;
    EXPECT(txcounters.num_tx_calls == 1);
    EXPECT(txcounters.num_tx_bytes == 1 + 40U);
    EXPECT(txcounters.tx_packets != NULL);
    if (txcounters.tx_packets != NULL) {
      u8_t sent;
      u16_t ret;
      ret = pbuf_copy_partial(txcounters.tx_packets, &sent, 1, 40U);
      EXPECT(ret == 1);
      EXPECT(sent == expected);
    }
    if (txcounters.tx_packets != NULL) {
      pbuf_free(txcounters.tx_packets);
      txcounters.tx_packets = NULL;
    }
  }

  /* make sure the pcb is freed */
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}

START_TEST(test_tcp_tx_full_window_lost_from_unsent)
{
  LWIP_UNUSED_ARG(_i);
  test_tcp_tx_full_window_lost(1);
}
END_TEST

START_TEST(test_tcp_tx_full_window_lost_from_unacked)
{
  LWIP_UNUSED_ARG(_i);
  test_tcp_tx_full_window_lost(0);
}
END_TEST

START_TEST(test_tcp_rto_tracking)
{
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  err_t err;
  u16_t i, sent_total = 0;
  LWIP_UNUSED_ARG(_i);

  for (i = 0; i < sizeof(tx_data); i++) {
    tx_data[i] = (u8_t)i;
  }

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  memset(&counters, 0, sizeof(counters));

  /* create and initialize the pcb */
  tcp_ticks = SEQNO1 - ISS;
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  pcb->mss = TCP_MSS;
  /* Set congestion window large enough to send all our segments */
  pcb->cwnd = 5*TCP_MSS;

  /* send 5 mss-sized segments */
  for (i = 0; i < 5; i++) {
    err = tcp_write(pcb, &tx_data[sent_total], TCP_MSS, TCP_WRITE_FLAG_COPY);
    EXPECT_RET(err == ERR_OK);
    sent_total += TCP_MSS;
  }
  check_seqnos(pcb->unsent, 5, seqnos);
  EXPECT(pcb->unacked == NULL);
  err = tcp_output(pcb);
  EXPECT(txcounters.num_tx_calls == 5);
  EXPECT(txcounters.num_tx_bytes == 5 * (TCP_MSS + 40U));
  memset(&txcounters, 0, sizeof(txcounters));
  /* Check all 5 are in-flight */
  EXPECT(pcb->unsent == NULL);
  check_seqnos(pcb->unacked, 5, seqnos);

  /* Force us into retransmisson timeout */
  while (!(pcb->flags & TF_RTO)) {
    test_tcp_tmr();
  }
  /* Ensure 4 remaining segments are back on unsent, ready for retransmission */
  check_seqnos(pcb->unsent, 4, &seqnos[1]);
  /* Ensure 1st segment is on unacked (already retransmitted) */
  check_seqnos(pcb->unacked, 1, seqnos);
  EXPECT(txcounters.num_tx_calls == 1);
  EXPECT(txcounters.num_tx_bytes == TCP_MSS + 40U);
  memset(&txcounters, 0, sizeof(txcounters));
  /* Ensure rto_end points to next byte */
  EXPECT(pcb->rto_end == seqnos[5]);
  EXPECT(pcb->rto_end == pcb->snd_nxt);
  /* Check cwnd was reset */
  EXPECT(pcb->cwnd == pcb->mss);

  /* Add another segment to send buffer which is outside of RTO */
  err = tcp_write(pcb, &tx_data[sent_total], TCP_MSS, TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  sent_total += TCP_MSS;
  check_seqnos(pcb->unsent, 5, &seqnos[1]);
  /* Ensure no new data was sent */
  EXPECT(txcounters.num_tx_calls == 0);
  EXPECT(txcounters.num_tx_bytes == 0);
  EXPECT(pcb->rto_end == pcb->snd_nxt);

  /* ACK first segment */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, TCP_MSS, TCP_ACK);
  test_tcp_input(p, &netif);
  /* Next two retranmissions should go out, due to cwnd in slow start */
  EXPECT(txcounters.num_tx_calls == 2);
  EXPECT(txcounters.num_tx_bytes == 2 * (TCP_MSS + 40U));
  memset(&txcounters, 0, sizeof(txcounters));
  check_seqnos(pcb->unacked, 2, &seqnos[1]);
  check_seqnos(pcb->unsent, 3, &seqnos[3]);
  /* RTO should still be marked */
  EXPECT(pcb->flags & TF_RTO);
  /* cwnd should have only grown by 1 MSS */
  EXPECT(pcb->cwnd == (tcpwnd_size_t)(2 * pcb->mss));
  /* Ensure no new data was sent */
  EXPECT(pcb->rto_end == pcb->snd_nxt);

  /* ACK the next two segments */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 2*TCP_MSS, TCP_ACK);
  test_tcp_input(p, &netif);
  /* Final 2 retransmissions and 1 new data should go out */
  EXPECT(txcounters.num_tx_calls == 3);
  EXPECT(txcounters.num_tx_bytes == 3 * (TCP_MSS + 40U));
  memset(&txcounters, 0, sizeof(txcounters));
  check_seqnos(pcb->unacked, 3, &seqnos[3]);
  EXPECT(pcb->unsent == NULL);
  /* RTO should still be marked */
  EXPECT(pcb->flags & TF_RTO);
  /* cwnd should have only grown by 1 MSS */
  EXPECT(pcb->cwnd == (tcpwnd_size_t)(3 * pcb->mss));
  /* snd_nxt should have been advanced past rto_end */
  EXPECT(TCP_SEQ_GT(pcb->snd_nxt, pcb->rto_end));

  /* ACK the next two segments, finishing our RTO, leaving new segment unacked */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 2*TCP_MSS, TCP_ACK);
  test_tcp_input(p, &netif);
  EXPECT(!(pcb->flags & TF_RTO));
  check_seqnos(pcb->unacked, 1, &seqnos[5]);
  /* We should be in ABC congestion avoidance, so no change in cwnd */
  EXPECT(pcb->cwnd == (tcpwnd_size_t)(3 * pcb->mss));
  EXPECT(pcb->cwnd >= pcb->ssthresh);
  /* Ensure ABC congestion avoidance is tracking bytes acked */
  EXPECT(pcb->bytes_acked == (tcpwnd_size_t)(2 * pcb->mss));

  /* make sure the pcb is freed */
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

START_TEST(test_tcp_rto_timeout)
{
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct test_tcp_counters counters;
  struct tcp_pcb *pcb, *cur;
  err_t err;
  u16_t i;
  LWIP_UNUSED_ARG(_i);

  /* Setup data for a single segment */
  for (i = 0; i < TCP_MSS; i++) {
    tx_data[i] = (u8_t)i;
  }

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  memset(&counters, 0, sizeof(counters));

  /* create and initialize the pcb */
  tcp_ticks = SEQNO1 - ISS;
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  pcb->mss = TCP_MSS;
  pcb->cwnd = TCP_MSS;

  /* send our segment */
  err = tcp_write(pcb, &tx_data[0], TCP_MSS, TCP_WRITE_FLAG_COPY);
  EXPECT_RET(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT(txcounters.num_tx_calls == 1);
  EXPECT(txcounters.num_tx_bytes == 1 * (TCP_MSS + 40U));
  memset(&txcounters, 0, sizeof(txcounters));

  /* ensure no errors have been recorded */
  EXPECT(counters.err_calls == 0);
  EXPECT(counters.last_err == ERR_OK);

  /* Force us into retransmisson timeout */
  while (!(pcb->flags & TF_RTO)) {
    test_tcp_tmr();
  }

  /* check first rexmit */
  EXPECT(pcb->nrtx == 1);
  EXPECT(txcounters.num_tx_calls == 1);
  EXPECT(txcounters.num_tx_bytes == 1 * (TCP_MSS + 40U));

  /* still no error expected */
  EXPECT(counters.err_calls == 0);
  EXPECT(counters.last_err == ERR_OK);

  /* keep running the timer till we hit our maximum RTO */
  while (counters.last_err == ERR_OK) {
    test_tcp_tmr();
  }

  /* check number of retransmissions */
  EXPECT(txcounters.num_tx_calls == TCP_MAXRTX);
  EXPECT(txcounters.num_tx_bytes == TCP_MAXRTX * (TCP_MSS + 40U));

  /* check the connection (pcb) has been aborted */
  EXPECT(counters.err_calls == 1);
  EXPECT(counters.last_err == ERR_ABRT);
  /* check our pcb is no longer active */
  for (cur = tcp_active_pcbs; cur != NULL; cur = cur->next) {
    EXPECT(cur != pcb);
  }  
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

START_TEST(test_tcp_zwp_timeout)
{
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct test_tcp_counters counters;
  struct tcp_pcb *pcb, *cur;
  struct pbuf* p;
  err_t err;
  u16_t i;
  LWIP_UNUSED_ARG(_i);

  /* Setup data for two segments */
  for (i = 0; i < 2*TCP_MSS; i++) {
    tx_data[i] = (u8_t)i;
  }

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  memset(&counters, 0, sizeof(counters));

  /* create and initialize the pcb */
  tcp_ticks = SEQNO1 - ISS;
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  pcb->mss = TCP_MSS;
  pcb->cwnd = TCP_MSS;

  /* send first segment */
  err = tcp_write(pcb, &tx_data[0], TCP_MSS, TCP_WRITE_FLAG_COPY);
  EXPECT(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT(err == ERR_OK);
  
  /* verify segment is in-flight */
  EXPECT(pcb->unsent == NULL);
  check_seqnos(pcb->unacked, 1, seqnos);
  EXPECT(txcounters.num_tx_calls == 1);
  EXPECT(txcounters.num_tx_bytes == 1 * (TCP_MSS + 40U));
  memset(&txcounters, 0, sizeof(txcounters));

  /* ACK the segment and close the TX window */
  p = tcp_create_rx_segment_wnd(pcb, NULL, 0, 0, TCP_MSS, TCP_ACK, 0);
  test_tcp_input(p, &netif);
  EXPECT(pcb->unacked == NULL);
  EXPECT(pcb->unsent == NULL);
  /* send buffer empty, persist should be off */
  EXPECT(pcb->persist_backoff == 0);
  EXPECT(pcb->snd_wnd == 0);

  /* send second segment, should be buffered */
  err = tcp_write(pcb, &tx_data[TCP_MSS], TCP_MSS, TCP_WRITE_FLAG_COPY);
  EXPECT(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT(err == ERR_OK);

  /* ensure it is buffered and persist timer started */
  EXPECT(pcb->unacked == NULL);
  check_seqnos(pcb->unsent, 1, &seqnos[1]);
  EXPECT(txcounters.num_tx_calls == 0);
  EXPECT(txcounters.num_tx_bytes == 0);
  EXPECT(pcb->persist_backoff == 1);

  /* ensure no errors have been recorded */
  EXPECT(counters.err_calls == 0);
  EXPECT(counters.last_err == ERR_OK);

  /* run timer till first probe */
  EXPECT(pcb->persist_probe == 0);
  while (pcb->persist_probe == 0) {
    test_tcp_tmr();
  }
  EXPECT(txcounters.num_tx_calls == 1);
  EXPECT(txcounters.num_tx_bytes == (1 + 40U));
  memset(&txcounters, 0, sizeof(txcounters));

  /* respond to probe with remote's current SEQ, ACK, and zero-window */
  p = tcp_create_rx_segment_wnd(pcb, NULL, 0, 0, 0, TCP_ACK, 0);
  test_tcp_input(p, &netif);
  /* ensure zero-window is still active, but probe count reset */
  EXPECT(pcb->persist_backoff > 1);
  EXPECT(pcb->persist_probe == 0);
  EXPECT(pcb->snd_wnd == 0);

  /* ensure no errors have been recorded */
  EXPECT(counters.err_calls == 0);
  EXPECT(counters.last_err == ERR_OK);

  /* now run the timer till we hit our maximum probe count */
  while (counters.last_err == ERR_OK) {
    test_tcp_tmr();
  }

  /* check maximum number of 1 byte probes were sent */
  EXPECT(txcounters.num_tx_calls == TCP_MAXRTX);
  EXPECT(txcounters.num_tx_bytes == TCP_MAXRTX * (1 + 40U));

  /* check the connection (pcb) has been aborted */
  EXPECT(counters.err_calls == 1);
  EXPECT(counters.last_err == ERR_ABRT);
  /* check our pcb is no longer active */
  for (cur = tcp_active_pcbs; cur != NULL; cur = cur->next) {
    EXPECT(cur != pcb);
  }  
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

START_TEST(test_tcp_persist_split)
{
  struct netif netif;
  struct test_tcp_txcounters txcounters;
  struct test_tcp_counters counters;
  struct tcp_pcb *pcb;
  struct pbuf* p;
  err_t err;
  u16_t i;
  LWIP_UNUSED_ARG(_i);

  /* Setup data for four segments */
  for (i = 0; i < 4 * TCP_MSS; i++) {
    tx_data[i] = (u8_t)i;
  }

  /* initialize local vars */
  test_tcp_init_netif(&netif, &txcounters, &test_local_ip, &test_netmask);
  memset(&counters, 0, sizeof(counters));

  /* create and initialize the pcb */
  tcp_ticks = SEQNO1 - ISS;
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  pcb->mss = TCP_MSS;
  /* set window to three segments */
  pcb->cwnd = 3 * TCP_MSS;
  pcb->snd_wnd = 3 * TCP_MSS;
  pcb->snd_wnd_max = 3 * TCP_MSS;

  /* send three segments */
  err = tcp_write(pcb, &tx_data[0], 3 * TCP_MSS, TCP_WRITE_FLAG_COPY);
  EXPECT(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT(err == ERR_OK);

  /* verify segments are in-flight */
  EXPECT(pcb->unsent == NULL);
  EXPECT(pcb->unacked != NULL);
  check_seqnos(pcb->unacked, 3, seqnos);
  EXPECT(txcounters.num_tx_calls == 3);
  EXPECT(txcounters.num_tx_bytes == 3 * (TCP_MSS + 40U));
  memset(&txcounters, 0, sizeof(txcounters));

  /* ACK the segments and update the window to only 1/2 TCP_MSS */
  p = tcp_create_rx_segment_wnd(pcb, NULL, 0, 0, 3 * TCP_MSS, TCP_ACK, TCP_MSS / 2);
  test_tcp_input(p, &netif);
  EXPECT(pcb->unacked == NULL);
  EXPECT(pcb->unsent == NULL);
  EXPECT(pcb->persist_backoff == 0);
  EXPECT(pcb->snd_wnd == TCP_MSS / 2);

  /* send fourth segment, which is larger than snd_wnd */
  err = tcp_write(pcb, &tx_data[3 * TCP_MSS], TCP_MSS, TCP_WRITE_FLAG_COPY);
  EXPECT(err == ERR_OK);
  err = tcp_output(pcb);
  EXPECT(err == ERR_OK);

  /* ensure it is buffered and persist timer started */
  EXPECT(pcb->unacked == NULL);
  EXPECT(pcb->unsent != NULL);
  check_seqnos(pcb->unsent, 1, &seqnos[3]);
  EXPECT(txcounters.num_tx_calls == 0);
  EXPECT(txcounters.num_tx_bytes == 0);
  EXPECT(pcb->persist_backoff == 1);

  /* ensure no errors have been recorded */
  EXPECT(counters.err_calls == 0);
  EXPECT(counters.last_err == ERR_OK);

  /* call tcp_timer some more times to let persist timer count up */
    for (i = 0; i < 4; i++) {
      test_tcp_tmr();
      EXPECT(txcounters.num_tx_calls == 0);
      EXPECT(txcounters.num_tx_bytes == 0);
    }

  /* this should be the first timer shot, which should split the
   * segment and send a runt (of the remaining window size) */
  txcounters.copy_tx_packets = 1;
  test_tcp_tmr();
  txcounters.copy_tx_packets = 0;
  /* persist will be disabled as RTO timer takes over */
  EXPECT(pcb->persist_backoff == 0);
  EXPECT(txcounters.num_tx_calls == 1);
  EXPECT(txcounters.num_tx_bytes == ((TCP_MSS /2) + 40U));
  /* verify half segment sent, half still buffered */
  EXPECT(pcb->unsent != NULL);
  EXPECT(pcb->unsent->len == TCP_MSS / 2);
  EXPECT(pcb->unacked != NULL);
  EXPECT(pcb->unacked->len == TCP_MSS / 2);

  /* verify first half segment */
  EXPECT(txcounters.tx_packets != NULL);
  if (txcounters.tx_packets != NULL) {
    u8_t sent[TCP_MSS / 2];
    u16_t ret;
    ret = pbuf_copy_partial(txcounters.tx_packets, &sent, TCP_MSS / 2, 40U);
    EXPECT(ret == TCP_MSS / 2);
    EXPECT(memcmp(sent, &tx_data[3 * TCP_MSS], TCP_MSS / 2) == 0);
  }
  if (txcounters.tx_packets != NULL) {
    pbuf_free(txcounters.tx_packets);
    txcounters.tx_packets = NULL;
  }
  memset(&txcounters, 0, sizeof(txcounters));

  /* ACK the half segment, leave window at half segment */
  p = tcp_create_rx_segment_wnd(pcb, NULL, 0, 0, TCP_MSS / 2, TCP_ACK, TCP_MSS / 2);
  txcounters.copy_tx_packets = 1;
  test_tcp_input(p, &netif);
  txcounters.copy_tx_packets = 0;
  /* ensure remaining half segment was sent */
  EXPECT(txcounters.num_tx_calls == 1);
  EXPECT(txcounters.num_tx_bytes == ((TCP_MSS /2 ) + 40U));
  EXPECT(pcb->unsent == NULL);
  EXPECT(pcb->unacked != NULL);
  EXPECT(pcb->unacked->len == TCP_MSS / 2);
  EXPECT(pcb->snd_wnd == TCP_MSS / 2);

  /* verify second half segment */
  EXPECT(txcounters.tx_packets != NULL);
  if (txcounters.tx_packets != NULL) {
    u8_t sent[TCP_MSS / 2];
    u16_t ret;
    ret = pbuf_copy_partial(txcounters.tx_packets, &sent, TCP_MSS / 2, 40U);
    EXPECT(ret == TCP_MSS / 2);
    EXPECT(memcmp(sent, &tx_data[(3 * TCP_MSS) + TCP_MSS / 2], TCP_MSS / 2) == 0);
  }
  if (txcounters.tx_packets != NULL) {
    pbuf_free(txcounters.tx_packets);
    txcounters.tx_packets = NULL;
  }

  /* ensure no errors have been recorded */
  EXPECT(counters.err_calls == 0);
  EXPECT(counters.last_err == ERR_OK);

  /* make sure the pcb is freed */
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
tcp_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_tcp_new_abort),
    TESTFUNC(test_tcp_listen_passive_open),
    TESTFUNC(test_tcp_recv_inseq),
    TESTFUNC(test_tcp_recv_inseq_trim),
    TESTFUNC(test_tcp_passive_close),
    TESTFUNC(test_tcp_malformed_header),
    TESTFUNC(test_tcp_fast_retx_recover),
    TESTFUNC(test_tcp_fast_rexmit_wraparound),
    TESTFUNC(test_tcp_rto_rexmit_wraparound),
    TESTFUNC(test_tcp_tx_full_window_lost_from_unacked),
    TESTFUNC(test_tcp_tx_full_window_lost_from_unsent),
    TESTFUNC(test_tcp_rto_tracking),
    TESTFUNC(test_tcp_rto_timeout),
    TESTFUNC(test_tcp_zwp_timeout),
    TESTFUNC(test_tcp_persist_split)
  };
  return create_suite("TCP", tests, sizeof(tests)/sizeof(testfunc), tcp_setup, tcp_teardown);
}
