#include "test_mqtt.h"

#include "lwip/pbuf.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/netif.h"

const ip_addr_t test_mqtt_local_ip = IPADDR4_INIT_BYTES(192, 168, 1, 1);
const ip_addr_t test_mqtt_remote_ip = IPADDR4_INIT_BYTES(192, 168, 1, 2);
const ip_addr_t test_mqtt_netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);

static err_t test_mqtt_netif_output(struct netif *netif, struct pbuf *p,
       const ip4_addr_t *ipaddr)
{
  LWIP_UNUSED_ARG(netif);
  LWIP_UNUSED_ARG(ipaddr);
  LWIP_UNUSED_ARG(p);
  return ERR_OK;
}

static void
test_mqtt_init_netif(struct netif *netif, const ip_addr_t *ip_addr, const ip_addr_t *netmask)
{
  struct netif *n;
  memset(netif, 0, sizeof(struct netif));
  netif->output = test_mqtt_netif_output;
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

/* Setups/teardown functions */
static struct netif *old_netif_list;
static struct netif *old_netif_default;

static void
mqtt_setup(void)
{
  old_netif_list = netif_list;
  old_netif_default = netif_default;
  netif_list = NULL;
  netif_default = NULL;
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
mqtt_teardown(void)
{
  netif_list = NULL;
  netif_default = NULL;
  /* restore netif_list for next tests (e.g. loopif) */
  netif_list = old_netif_list;
  netif_default = old_netif_default;
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void test_mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  LWIP_UNUSED_ARG(client);
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(status);
}

START_TEST(basic_connect)
{
  mqtt_client_t* client;
  struct netif netif;
  err_t err;
  struct mqtt_connect_client_info_t client_info = {
    "dumm",
    NULL, NULL,
    10,
    NULL, NULL, 0, 0
  };
  struct pbuf *p;
  unsigned char rxbuf[] = {0x20, 0x02, 0x00, 0x00};
  LWIP_UNUSED_ARG(_i);

  test_mqtt_init_netif(&netif, &test_mqtt_local_ip, &test_mqtt_netmask);

  client = mqtt_client_new();
  fail_unless(client != NULL);
  err = mqtt_client_connect(client, &test_mqtt_remote_ip, 1234, test_mqtt_connection_cb, NULL, &client_info);
  fail_unless(err == ERR_OK);

  client->conn->connected(client->conn->callback_arg, client->conn, ERR_OK);
  p = pbuf_alloc(PBUF_RAW, sizeof(rxbuf), PBUF_REF);
  fail_unless(p != NULL);
  p->payload = rxbuf;
  if (client->conn->recv(client->conn->callback_arg, client->conn, p, ERR_OK) != ERR_OK) {
    pbuf_free(p);
  }

  mqtt_disconnect(client);
  /* fixme: mqtt_client_fre() is missing... */
  mem_free(client);
}
END_TEST

Suite* mqtt_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(basic_connect),
  };
  return create_suite("MQTT", tests, sizeof(tests)/sizeof(testfunc), mqtt_setup, mqtt_teardown);
}
