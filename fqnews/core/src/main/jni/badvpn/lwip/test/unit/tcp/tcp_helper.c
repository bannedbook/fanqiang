#include "tcp_helper.h"

#include "lwip/priv/tcp_priv.h"
#include "lwip/stats.h"
#include "lwip/pbuf.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip_addr.h"

#if !LWIP_STATS || !TCP_STATS || !MEMP_STATS
#error "This tests needs TCP- and MEMP-statistics enabled"
#endif

const ip_addr_t test_local_ip = IPADDR4_INIT_BYTES(192, 168, 1, 1);
const ip_addr_t test_remote_ip = IPADDR4_INIT_BYTES(192, 168, 1, 2);
const ip_addr_t test_netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);

/** Remove all pcbs on the given list. */
static void
tcp_remove(struct tcp_pcb* pcb_list)
{
  struct tcp_pcb *pcb = pcb_list;
  struct tcp_pcb *pcb2;

  while(pcb != NULL) {
    pcb2 = pcb;
    pcb = pcb->next;
    tcp_abort(pcb2);
  }
}

/** Remove all pcbs on listen-, active- and time-wait-list (bound- isn't exported). */
void
tcp_remove_all(void)
{
  tcp_remove(tcp_listen_pcbs.pcbs);
  tcp_remove(tcp_bound_pcbs);
  tcp_remove(tcp_active_pcbs);
  tcp_remove(tcp_tw_pcbs);
  fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
  fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB_LISTEN) == 0);
  fail_unless(MEMP_STATS_GET(used, MEMP_TCP_SEG) == 0);
  fail_unless(MEMP_STATS_GET(used, MEMP_PBUF_POOL) == 0);
}

/** Create a TCP segment usable for passing to tcp_input */
static struct pbuf*
tcp_create_segment_wnd(ip_addr_t* src_ip, ip_addr_t* dst_ip,
                   u16_t src_port, u16_t dst_port, void* data, size_t data_len,
                   u32_t seqno, u32_t ackno, u8_t headerflags, u16_t wnd)
{
  struct pbuf *p, *q;
  struct ip_hdr* iphdr;
  struct tcp_hdr* tcphdr;
  u16_t pbuf_len = (u16_t)(sizeof(struct ip_hdr) + sizeof(struct tcp_hdr) + data_len);
  LWIP_ASSERT("data_len too big", data_len <= 0xFFFF);

  p = pbuf_alloc(PBUF_RAW, pbuf_len, PBUF_POOL);
  EXPECT_RETNULL(p != NULL);
  /* first pbuf must be big enough to hold the headers */
  EXPECT_RETNULL(p->len >= (sizeof(struct ip_hdr) + sizeof(struct tcp_hdr)));
  if (data_len > 0) {
    /* first pbuf must be big enough to hold at least 1 data byte, too */
    EXPECT_RETNULL(p->len > (sizeof(struct ip_hdr) + sizeof(struct tcp_hdr)));
  }

  for(q = p; q != NULL; q = q->next) {
    memset(q->payload, 0, q->len);
  }

  iphdr = (struct ip_hdr*)p->payload;
  /* fill IP header */
  iphdr->dest.addr = ip_2_ip4(dst_ip)->addr;
  iphdr->src.addr = ip_2_ip4(src_ip)->addr;
  IPH_VHL_SET(iphdr, 4, IP_HLEN / 4);
  IPH_TOS_SET(iphdr, 0);
  IPH_LEN_SET(iphdr, htons(p->tot_len));
  IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, IP_HLEN));

  /* let p point to TCP header */
  pbuf_header(p, -(s16_t)sizeof(struct ip_hdr));

  tcphdr = (struct tcp_hdr*)p->payload;
  tcphdr->src   = htons(src_port);
  tcphdr->dest  = htons(dst_port);
  tcphdr->seqno = htonl(seqno);
  tcphdr->ackno = htonl(ackno);
  TCPH_HDRLEN_SET(tcphdr, sizeof(struct tcp_hdr)/4);
  TCPH_FLAGS_SET(tcphdr, headerflags);
  tcphdr->wnd   = htons(wnd);

  if (data_len > 0) {
    /* let p point to TCP data */
    pbuf_header(p, -(s16_t)sizeof(struct tcp_hdr));
    /* copy data */
    pbuf_take(p, data, (u16_t)data_len);
    /* let p point to TCP header again */
    pbuf_header(p, sizeof(struct tcp_hdr));
  }

  /* calculate checksum */

  tcphdr->chksum = ip_chksum_pseudo(p,
          IP_PROTO_TCP, p->tot_len, src_ip, dst_ip);

  pbuf_header(p, sizeof(struct ip_hdr));

  return p;
}

/** Create a TCP segment usable for passing to tcp_input */
struct pbuf*
tcp_create_segment(ip_addr_t* src_ip, ip_addr_t* dst_ip,
                   u16_t src_port, u16_t dst_port, void* data, size_t data_len,
                   u32_t seqno, u32_t ackno, u8_t headerflags)
{
  return tcp_create_segment_wnd(src_ip, dst_ip, src_port, dst_port, data,
    data_len, seqno, ackno, headerflags, TCP_WND);
}

/** Create a TCP segment usable for passing to tcp_input
 * - IP-addresses, ports, seqno and ackno are taken from pcb
 * - seqno and ackno can be altered with an offset
 */
struct pbuf*
tcp_create_rx_segment(struct tcp_pcb* pcb, void* data, size_t data_len, u32_t seqno_offset,
                      u32_t ackno_offset, u8_t headerflags)
{
  return tcp_create_segment(&pcb->remote_ip, &pcb->local_ip, pcb->remote_port, pcb->local_port,
    data, data_len, pcb->rcv_nxt + seqno_offset, pcb->lastack + ackno_offset, headerflags);
}

/** Create a TCP segment usable for passing to tcp_input
 * - IP-addresses, ports, seqno and ackno are taken from pcb
 * - seqno and ackno can be altered with an offset
 * - TCP window can be adjusted
 */
struct pbuf* tcp_create_rx_segment_wnd(struct tcp_pcb* pcb, void* data, size_t data_len,
                   u32_t seqno_offset, u32_t ackno_offset, u8_t headerflags, u16_t wnd)
{
  return tcp_create_segment_wnd(&pcb->remote_ip, &pcb->local_ip, pcb->remote_port, pcb->local_port,
    data, data_len, pcb->rcv_nxt + seqno_offset, pcb->lastack + ackno_offset, headerflags, wnd);
}

/** Safely bring a tcp_pcb into the requested state */
void
tcp_set_state(struct tcp_pcb* pcb, enum tcp_state state, const ip_addr_t* local_ip,
                   const ip_addr_t* remote_ip, u16_t local_port, u16_t remote_port)
{
  u32_t iss;

  /* @todo: are these all states? */
  /* @todo: remove from previous list */
  pcb->state = state;
  
  iss = tcp_next_iss(pcb);
  pcb->snd_wl2 = iss;
  pcb->snd_nxt = iss;
  pcb->lastack = iss;
  pcb->snd_lbb = iss;
  
  if (state == ESTABLISHED) {
    TCP_REG(&tcp_active_pcbs, pcb);
    ip_addr_copy(pcb->local_ip, *local_ip);
    pcb->local_port = local_port;
    ip_addr_copy(pcb->remote_ip, *remote_ip);
    pcb->remote_port = remote_port;
  } else if(state == LISTEN) {
    TCP_REG(&tcp_listen_pcbs.pcbs, pcb);
    ip_addr_copy(pcb->local_ip, *local_ip);
    pcb->local_port = local_port;
  } else if(state == TIME_WAIT) {
    TCP_REG(&tcp_tw_pcbs, pcb);
    ip_addr_copy(pcb->local_ip, *local_ip);
    pcb->local_port = local_port;
    ip_addr_copy(pcb->remote_ip, *remote_ip);
    pcb->remote_port = remote_port;
  } else {
    fail();
  }
}

void
test_tcp_counters_err(void* arg, err_t err)
{
  struct test_tcp_counters* counters = (struct test_tcp_counters*)arg;
  EXPECT_RET(arg != NULL);
  counters->err_calls++;
  counters->last_err = err;
}

static void
test_tcp_counters_check_rxdata(struct test_tcp_counters* counters, struct pbuf* p)
{
  struct pbuf* q;
  u32_t i, received;
  if(counters->expected_data == NULL) {
    /* no data to compare */
    return;
  }
  EXPECT_RET(counters->recved_bytes + p->tot_len <= counters->expected_data_len);
  received = counters->recved_bytes;
  for(q = p; q != NULL; q = q->next) {
    char *data = (char*)q->payload;
    for(i = 0; i < q->len; i++) {
      EXPECT_RET(data[i] == counters->expected_data[received]);
      received++;
    }
  }
  EXPECT(received == counters->recved_bytes + p->tot_len);
}

err_t
test_tcp_counters_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err)
{
  struct test_tcp_counters* counters = (struct test_tcp_counters*)arg;
  EXPECT_RETX(arg != NULL, ERR_OK);
  EXPECT_RETX(pcb != NULL, ERR_OK);
  EXPECT_RETX(err == ERR_OK, ERR_OK);

  if (p != NULL) {
    if (counters->close_calls == 0) {
      counters->recv_calls++;
      test_tcp_counters_check_rxdata(counters, p);
      counters->recved_bytes += p->tot_len;
    } else {
      counters->recv_calls_after_close++;
      counters->recved_bytes_after_close += p->tot_len;
    }
    pbuf_free(p);
  } else {
    counters->close_calls++;
  }
  EXPECT(counters->recv_calls_after_close == 0 && counters->recved_bytes_after_close == 0);
  return ERR_OK;
}

/** Allocate a pcb and set up the test_tcp_counters_* callbacks */
struct tcp_pcb*
test_tcp_new_counters_pcb(struct test_tcp_counters* counters)
{
  struct tcp_pcb* pcb = tcp_new();
  if (pcb != NULL) {
    /* set up args and callbacks */
    tcp_arg(pcb, counters);
    tcp_recv(pcb, test_tcp_counters_recv);
    tcp_err(pcb, test_tcp_counters_err);
    pcb->snd_wnd = TCP_WND;
    pcb->snd_wnd_max = TCP_WND;
  }
  return pcb;
}

/** Calls tcp_input() after adjusting current_iphdr_dest */
void test_tcp_input(struct pbuf *p, struct netif *inp)
{
  struct ip_hdr *iphdr = (struct ip_hdr*)p->payload;
  /* these lines are a hack, don't use them as an example :-) */
  ip_addr_copy_from_ip4(*ip_current_dest_addr(), iphdr->dest);
  ip_addr_copy_from_ip4(*ip_current_src_addr(), iphdr->src);
  ip_current_netif() = inp;
  ip_data.current_ip4_header = iphdr;

  /* since adding IPv6, p->payload must point to tcp header, not ip header */
  pbuf_header(p, -(s16_t)sizeof(struct ip_hdr));

  tcp_input(p, inp);

  ip_addr_set_zero(ip_current_dest_addr());
  ip_addr_set_zero(ip_current_src_addr());
  ip_current_netif() = NULL;
  ip_data.current_ip4_header = NULL;
}

static err_t test_tcp_netif_output(struct netif *netif, struct pbuf *p,
       const ip4_addr_t *ipaddr)
{
  struct test_tcp_txcounters *txcounters = (struct test_tcp_txcounters*)netif->state;
  LWIP_UNUSED_ARG(ipaddr);
  if (txcounters != NULL)
  {
    txcounters->num_tx_calls++;
    txcounters->num_tx_bytes += p->tot_len;
    if (txcounters->copy_tx_packets) {
      struct pbuf *p_copy = pbuf_alloc(PBUF_LINK, p->tot_len, PBUF_RAM);
      err_t err;
      EXPECT(p_copy != NULL);
      err = pbuf_copy(p_copy, p);
      EXPECT(err == ERR_OK);
      if (txcounters->tx_packets == NULL) {
        txcounters->tx_packets = p_copy;
      } else {
        pbuf_cat(txcounters->tx_packets, p_copy);
      }
    }
  }
  return ERR_OK;
}

void test_tcp_init_netif(struct netif *netif, struct test_tcp_txcounters *txcounters,
                         const ip_addr_t *ip_addr, const ip_addr_t *netmask)
{
  struct netif *n;
  memset(netif, 0, sizeof(struct netif));
  if (txcounters != NULL) {
    memset(txcounters, 0, sizeof(struct test_tcp_txcounters));
    netif->state = txcounters;
  }
  netif->output = test_tcp_netif_output;
  netif->flags |= NETIF_FLAG_UP | NETIF_FLAG_LINK_UP;
  ip_addr_copy_from_ip4(netif->netmask, *ip_2_ip4(netmask));
  ip_addr_copy_from_ip4(netif->ip_addr, *ip_2_ip4(ip_addr));
  for (n = netif_list; n != NULL; n = n->next) {
    if (n == netif) {
      return;
    }
  }
  netif->next = NULL;
  netif_list = netif;
}
