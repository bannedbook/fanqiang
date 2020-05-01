#ifndef LWIP_HDR_TCP_HELPER_H
#define LWIP_HDR_TCP_HELPER_H

#include "../lwip_check.h"
#include "lwip/arch.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

/* counters used for test_tcp_counters_* callback functions */
struct test_tcp_counters {
  u32_t recv_calls;
  u32_t recved_bytes;
  u32_t recv_calls_after_close;
  u32_t recved_bytes_after_close;
  u32_t close_calls;
  u32_t err_calls;
  err_t last_err;
  char* expected_data;
  u32_t expected_data_len;
};

struct test_tcp_txcounters {
  u32_t num_tx_calls;
  u32_t num_tx_bytes;
  u8_t  copy_tx_packets;
  struct pbuf *tx_packets;
};

extern const ip_addr_t test_local_ip;
extern const ip_addr_t test_remote_ip;
extern const ip_addr_t test_netmask;
#define TEST_REMOTE_PORT    0x100
#define TEST_LOCAL_PORT     0x101

/* Helper functions */
void tcp_remove_all(void);

struct pbuf* tcp_create_segment(ip_addr_t* src_ip, ip_addr_t* dst_ip,
                   u16_t src_port, u16_t dst_port, void* data, size_t data_len,
                   u32_t seqno, u32_t ackno, u8_t headerflags);
struct pbuf* tcp_create_rx_segment(struct tcp_pcb* pcb, void* data, size_t data_len,
                   u32_t seqno_offset, u32_t ackno_offset, u8_t headerflags);
struct pbuf* tcp_create_rx_segment_wnd(struct tcp_pcb* pcb, void* data, size_t data_len,
                   u32_t seqno_offset, u32_t ackno_offset, u8_t headerflags, u16_t wnd);
void tcp_set_state(struct tcp_pcb* pcb, enum tcp_state state, const ip_addr_t* local_ip,
                   const ip_addr_t* remote_ip, u16_t local_port, u16_t remote_port);
void test_tcp_counters_err(void* arg, err_t err);
err_t test_tcp_counters_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err);

struct tcp_pcb* test_tcp_new_counters_pcb(struct test_tcp_counters* counters);

void test_tcp_input(struct pbuf *p, struct netif *inp);

void test_tcp_init_netif(struct netif *netif, struct test_tcp_txcounters *txcounters,
                         const ip_addr_t *ip_addr, const ip_addr_t *netmask);


#endif
