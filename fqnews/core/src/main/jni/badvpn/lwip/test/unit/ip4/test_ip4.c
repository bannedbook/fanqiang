#include "test_ip4.h"

#include "lwip/ip4.h"
#include "lwip/inet_chksum.h"
#include "lwip/stats.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/ip4.h"

#include "lwip/tcpip.h"

#if !LWIP_IPV4 || !IP_REASSEMBLY || !MIB2_STATS || !IPFRAG_STATS
#error "This tests needs LWIP_IPV4, IP_REASSEMBLY; MIB2- and IPFRAG-statistics enabled"
#endif

/* Helper functions */
static void
create_ip4_input_fragment(u16_t ip_id, u16_t start, u16_t len, int last)
{
  struct pbuf *p;
  struct netif *input_netif = netif_list; /* just use any netif */
  fail_unless((start & 7) == 0);
  fail_unless(((len & 7) == 0) || last);
  fail_unless(input_netif != NULL);

  p = pbuf_alloc(PBUF_RAW, len + sizeof(struct ip_hdr), PBUF_RAM);
  fail_unless(p != NULL);
  if (p != NULL) {
    err_t err;
    struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
    IPH_VHL_SET(iphdr, 4, sizeof(struct ip_hdr) / 4);
    IPH_TOS_SET(iphdr, 0);
    IPH_LEN_SET(iphdr, lwip_htons(p->tot_len));
    IPH_ID_SET(iphdr, lwip_htons(ip_id));
    if (last) {
      IPH_OFFSET_SET(iphdr, lwip_htons(start / 8));
    } else {
      IPH_OFFSET_SET(iphdr, lwip_htons((start / 8) | IP_MF));
    }
    IPH_TTL_SET(iphdr, 5);
    IPH_PROTO_SET(iphdr, IP_PROTO_UDP);
    IPH_CHKSUM_SET(iphdr, 0);
    ip4_addr_copy(iphdr->src, *netif_ip4_addr(input_netif));
    iphdr->src.addr = lwip_htonl(lwip_htonl(iphdr->src.addr) + 1);
    ip4_addr_copy(iphdr->dest, *netif_ip4_addr(input_netif));
    IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, sizeof(struct ip_hdr)));

    err = ip4_input(p, input_netif);
    if (err != ERR_OK) {
      pbuf_free(p);
    }
    fail_unless(err == ERR_OK);
  }
}

/* Setups/teardown functions */

static void
ip4_setup(void)
{
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
ip4_teardown(void)
{
  if (netif_list->loop_first != NULL) {
    pbuf_free(netif_list->loop_first);
    netif_list->loop_first = NULL;
  }
  netif_list->loop_last = NULL;
  /* poll until all memory is released... */
  tcpip_thread_poll_one();
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}


/* Test functions */

START_TEST(test_ip4_reass)
{
  const u16_t ip_id = 128;
  LWIP_UNUSED_ARG(_i);

  memset(&lwip_stats.mib2, 0, sizeof(lwip_stats.mib2));

  create_ip4_input_fragment(ip_id, 8*200, 200, 1);
  fail_unless(lwip_stats.ip_frag.recv == 1);
  fail_unless(lwip_stats.ip_frag.err == 0);
  fail_unless(lwip_stats.ip_frag.memerr == 0);
  fail_unless(lwip_stats.ip_frag.drop == 0);
  fail_unless(lwip_stats.mib2.ipreasmoks == 0);

  create_ip4_input_fragment(ip_id, 0*200, 200, 0);
  fail_unless(lwip_stats.ip_frag.recv == 2);
  fail_unless(lwip_stats.ip_frag.err == 0);
  fail_unless(lwip_stats.ip_frag.memerr == 0);
  fail_unless(lwip_stats.ip_frag.drop == 0);
  fail_unless(lwip_stats.mib2.ipreasmoks == 0);

  create_ip4_input_fragment(ip_id, 1*200, 200, 0);
  fail_unless(lwip_stats.ip_frag.recv == 3);
  fail_unless(lwip_stats.ip_frag.err == 0);
  fail_unless(lwip_stats.ip_frag.memerr == 0);
  fail_unless(lwip_stats.ip_frag.drop == 0);
  fail_unless(lwip_stats.mib2.ipreasmoks == 0);

  create_ip4_input_fragment(ip_id, 2*200, 200, 0);
  fail_unless(lwip_stats.ip_frag.recv == 4);
  fail_unless(lwip_stats.ip_frag.err == 0);
  fail_unless(lwip_stats.ip_frag.memerr == 0);
  fail_unless(lwip_stats.ip_frag.drop == 0);
  fail_unless(lwip_stats.mib2.ipreasmoks == 0);

  create_ip4_input_fragment(ip_id, 3*200, 200, 0);
  fail_unless(lwip_stats.ip_frag.recv == 5);
  fail_unless(lwip_stats.ip_frag.err == 0);
  fail_unless(lwip_stats.ip_frag.memerr == 0);
  fail_unless(lwip_stats.ip_frag.drop == 0);
  fail_unless(lwip_stats.mib2.ipreasmoks == 0);

  create_ip4_input_fragment(ip_id, 4*200, 200, 0);
  fail_unless(lwip_stats.ip_frag.recv == 6);
  fail_unless(lwip_stats.ip_frag.err == 0);
  fail_unless(lwip_stats.ip_frag.memerr == 0);
  fail_unless(lwip_stats.ip_frag.drop == 0);
  fail_unless(lwip_stats.mib2.ipreasmoks == 0);

  create_ip4_input_fragment(ip_id, 7*200, 200, 0);
  fail_unless(lwip_stats.ip_frag.recv == 7);
  fail_unless(lwip_stats.ip_frag.err == 0);
  fail_unless(lwip_stats.ip_frag.memerr == 0);
  fail_unless(lwip_stats.ip_frag.drop == 0);
  fail_unless(lwip_stats.mib2.ipreasmoks == 0);

  create_ip4_input_fragment(ip_id, 6*200, 200, 0);
  fail_unless(lwip_stats.ip_frag.recv == 8);
  fail_unless(lwip_stats.ip_frag.err == 0);
  fail_unless(lwip_stats.ip_frag.memerr == 0);
  fail_unless(lwip_stats.ip_frag.drop == 0);
  fail_unless(lwip_stats.mib2.ipreasmoks == 0);

  create_ip4_input_fragment(ip_id, 5*200, 200, 0);
  fail_unless(lwip_stats.ip_frag.recv == 9);
  fail_unless(lwip_stats.ip_frag.err == 0);
  fail_unless(lwip_stats.ip_frag.memerr == 0);
  fail_unless(lwip_stats.ip_frag.drop == 0);
  fail_unless(lwip_stats.mib2.ipreasmoks == 1);
}
END_TEST


/** Create the suite including all tests for this module */
Suite *
ip4_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_ip4_reass),
  };
  return create_suite("IPv4", tests, sizeof(tests)/sizeof(testfunc), ip4_setup, ip4_teardown);
}
