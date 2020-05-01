void
eth_mac_irq()
{
  /* Service MAC IRQ here */

  /* Allocate pbuf from pool (avoid using heap in interrupts) */
  struct pbuf* p = pbuf_alloc(PBUF_RAW, eth_data_count, PBUF_POOL);

  if(p != NULL) {
    /* Copy ethernet frame into pbuf */
    pbuf_take(p, eth_data, eth_data_count);

    /* Put in a queue which is processed in main loop */
    if(!queue_try_put(&queue, p)) {
      /* queue is full -> packet loss */
      pbuf_free(p);
    }
  }
}

static err_t 
netif_output(struct netif *netif, struct pbuf *p)
{
  LINK_STATS_INC(link.xmit);

  /* Update SNMP stats (only if you use SNMP) */
  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  int unicast = ((p->payload[0] & 0x01) == 0);
  if (unicast) {
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  } else {
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  }

  lock_interrupts();
  pbuf_copy_partial(p, mac_send_buffer, p->tot_len, 0);
  /* Start MAC transmit here */
  unlock_interrupts();

  return ERR_OK;
}

static void 
netif_status_callback(struct netif *netif)
{
  printf("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
}

static err_t 
netif_init(struct netif *netif)
{
  netif->linkoutput = netif_output;
  netif->output     = etharp_output;
  netif->output_ip6 = ethip6_output;
  netif->mtu        = ETHERNET_MTU;
  netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 100000000);

  SMEMCPY(netif->hwaddr, your_mac_address_goes_here, sizeof(netif->hwaddr));
  netif->hwaddr_len = sizeof(netif->hwaddr);

  return ERR_OK;
}

void 
main(void)
{
  struct netif netif;

  lwip_init();

  netif_add(&netif, IP4_ADDR_ANY, IP4_ADDR_ANY, IP4_ADDR_ANY, NULL, netif_init, netif_input);
  netif.name[0] = 'e';
  netif.name[1] = '0';
  netif_create_ip6_linklocal_address(&netif, 1);
  netif.ip6_autoconfig_enabled = 1;
  netif_set_status_callback(&netif, netif_status_callback);
  netif_set_default(&netif);
  netif_set_up(&netif);
  
  /* Start DHCP and HTTPD */
  dhcp_start(&netif );
  httpd_init();

  while(1) {
    /* Check link state, e.g. via MDIO communication with PHY */
    if(link_state_changed()) {
      if(link_is_up()) {
        netif_set_link_up(&netif);
      } else {
        netif_set_link_down(&netif);
      }
    }

    /* Check for received frames, feed them to lwIP */
    lock_interrupts();
    struct pbuf* p = queue_try_get(&queue);
    unlock_interrupts();

    if(p != NULL) {
      LINK_STATS_INC(link.recv);
 
      /* Update SNMP stats (only if you use SNMP) */
      MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
      int unicast = ((p->payload[0] & 0x01) == 0);
      if (unicast) {
        MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
      } else {
        MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
      }

      if(netif.input(p, &netif) != ERR_OK) {
        pbuf_free(p);
      }
    }
     
    /* Cyclic lwIP timers check */
    sys_check_timeouts();
     
    /* your application goes here */
  }
}
