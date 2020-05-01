#include "test_udp.h"

#include "lwip/udp.h"
#include "lwip/stats.h"

#if !LWIP_STATS || !UDP_STATS || !MEMP_STATS
#error "This tests needs UDP- and MEMP-statistics enabled"
#endif

/* Helper functions */
static void
udp_remove_all(void)
{
  struct udp_pcb *pcb = udp_pcbs;
  struct udp_pcb *pcb2;

  while(pcb != NULL) {
    pcb2 = pcb;
    pcb = pcb->next;
    udp_remove(pcb2);
  }
  fail_unless(MEMP_STATS_GET(used, MEMP_UDP_PCB) == 0);
}

/* Setups/teardown functions */

static void
udp_setup(void)
{
  udp_remove_all();
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
udp_teardown(void)
{
  udp_remove_all();
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}


/* Test functions */

START_TEST(test_udp_new_remove)
{
  struct udp_pcb* pcb;
  LWIP_UNUSED_ARG(_i);

  fail_unless(MEMP_STATS_GET(used, MEMP_UDP_PCB) == 0);

  pcb = udp_new();
  fail_unless(pcb != NULL);
  if (pcb != NULL) {
    fail_unless(MEMP_STATS_GET(used, MEMP_UDP_PCB) == 1);
    udp_remove(pcb);
    fail_unless(MEMP_STATS_GET(used, MEMP_UDP_PCB) == 0);
  }
}
END_TEST


/** Create the suite including all tests for this module */
Suite *
udp_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_udp_new_remove),
  };
  return create_suite("UDP", tests, sizeof(tests)/sizeof(testfunc), udp_setup, udp_teardown);
}
