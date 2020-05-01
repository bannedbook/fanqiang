/**
 * @file
 * Network Interface Sequential API module
 *
 * @defgroup netifapi NETIF API
 * @ingroup sequential_api
 * Thread-safe functions to be called from non-TCPIP threads
 *
 * @defgroup netifapi_netif NETIF related
 * @ingroup netifapi
 * To be called from non-TCPIP threads
 */

/*
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
 */

#include "lwip/opt.h"

#if LWIP_NETIF_API /* don't build if not configured for use in lwipopts.h */

#include "lwip/netifapi.h"
#include "lwip/memp.h"
#include "lwip/priv/tcpip_priv.h"

#include <string.h> /* strncpy */

#define NETIFAPI_VAR_REF(name)      API_VAR_REF(name)
#define NETIFAPI_VAR_DECLARE(name)  API_VAR_DECLARE(struct netifapi_msg, name)
#define NETIFAPI_VAR_ALLOC(name)    API_VAR_ALLOC(struct netifapi_msg, MEMP_NETIFAPI_MSG, name, ERR_MEM)
#define NETIFAPI_VAR_FREE(name)     API_VAR_FREE(MEMP_NETIFAPI_MSG, name)

/**
 * Call netif_add() inside the tcpip_thread context.
 */
static err_t
netifapi_do_netif_add(struct tcpip_api_call_data *m)
{
  /* cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  if (!netif_add( msg->netif,
#if LWIP_IPV4
                  API_EXPR_REF(msg->msg.add.ipaddr),
                  API_EXPR_REF(msg->msg.add.netmask),
                  API_EXPR_REF(msg->msg.add.gw),
#endif /* LWIP_IPV4 */
                  msg->msg.add.state,
                  msg->msg.add.init,
                  msg->msg.add.input)) {
    return ERR_IF;
  } else {
    return ERR_OK;
  }
}

#if LWIP_IPV4
/**
 * Call netif_set_addr() inside the tcpip_thread context.
 */
static err_t
netifapi_do_netif_set_addr(struct tcpip_api_call_data *m)
{
  /* cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  netif_set_addr( msg->netif,
                  API_EXPR_REF(msg->msg.add.ipaddr),
                  API_EXPR_REF(msg->msg.add.netmask),
                  API_EXPR_REF(msg->msg.add.gw));
  return ERR_OK;
}
#endif /* LWIP_IPV4 */

/**
* Call netif_name_to_index() inside the tcpip_thread context.
*/
static err_t
netifapi_do_name_to_index(struct tcpip_api_call_data *m)
{
  /* cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  msg->msg.ifs.index = netif_name_to_index(msg->msg.ifs.name);
  return ERR_OK;
}

/**
* Call netif_index_to_name() inside the tcpip_thread context.
*/
static err_t
netifapi_do_index_to_name(struct tcpip_api_call_data *m)
{
  /* cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  if (!netif_index_to_name(msg->msg.ifs.index, msg->msg.ifs.name)) {
    /* return failure via empty name */
    msg->msg.ifs.name[0] = '\0';
  }
  return ERR_OK;
}

/**
 * Call the "errtfunc" (or the "voidfunc" if "errtfunc" is NULL) inside the
 * tcpip_thread context.
 */
static err_t
netifapi_do_netif_common(struct tcpip_api_call_data *m)
{
  /* cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  if (msg->msg.common.errtfunc != NULL) {
    return msg->msg.common.errtfunc(msg->netif);
  } else {
    msg->msg.common.voidfunc(msg->netif);
    return ERR_OK;
  }
}

/**
 * @ingroup netifapi_netif
 * Call netif_add() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @note for params @see netif_add()
 */
err_t
netifapi_netif_add(struct netif *netif,
#if LWIP_IPV4
                   const ip4_addr_t *ipaddr, const ip4_addr_t *netmask, const ip4_addr_t *gw,
#endif /* LWIP_IPV4 */
                   void *state, netif_init_fn init, netif_input_fn input)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

#if LWIP_IPV4
  if (ipaddr == NULL) {
    ipaddr = IP4_ADDR_ANY4;
  }
  if (netmask == NULL) {
    netmask = IP4_ADDR_ANY4;
  }
  if (gw == NULL) {
    gw = IP4_ADDR_ANY4;
  }
#endif /* LWIP_IPV4 */

  NETIFAPI_VAR_REF(msg).netif = netif;
#if LWIP_IPV4
  NETIFAPI_VAR_REF(msg).msg.add.ipaddr  = NETIFAPI_VAR_REF(ipaddr);
  NETIFAPI_VAR_REF(msg).msg.add.netmask = NETIFAPI_VAR_REF(netmask);
  NETIFAPI_VAR_REF(msg).msg.add.gw      = NETIFAPI_VAR_REF(gw);
#endif /* LWIP_IPV4 */
  NETIFAPI_VAR_REF(msg).msg.add.state   = state;
  NETIFAPI_VAR_REF(msg).msg.add.init    = init;
  NETIFAPI_VAR_REF(msg).msg.add.input   = input;
  err = tcpip_api_call(netifapi_do_netif_add, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

#if LWIP_IPV4
/**
 * @ingroup netifapi_netif
 * Call netif_set_addr() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @note for params @see netif_set_addr()
 */
err_t
netifapi_netif_set_addr(struct netif *netif,
                        const ip4_addr_t *ipaddr,
                        const ip4_addr_t *netmask,
                        const ip4_addr_t *gw)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  if (ipaddr == NULL) {
    ipaddr = IP4_ADDR_ANY4;
  }
  if (netmask == NULL) {
    netmask = IP4_ADDR_ANY4;
  }
  if (gw == NULL) {
    gw = IP4_ADDR_ANY4;
  }

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.add.ipaddr  = NETIFAPI_VAR_REF(ipaddr);
  NETIFAPI_VAR_REF(msg).msg.add.netmask = NETIFAPI_VAR_REF(netmask);
  NETIFAPI_VAR_REF(msg).msg.add.gw      = NETIFAPI_VAR_REF(gw);
  err = tcpip_api_call(netifapi_do_netif_set_addr, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_IPV4 */

/**
 * call the "errtfunc" (or the "voidfunc" if "errtfunc" is NULL) in a thread-safe
 * way by running that function inside the tcpip_thread context.
 *
 * @note use only for functions where there is only "netif" parameter.
 */
err_t
netifapi_netif_common(struct netif *netif, netifapi_void_fn voidfunc,
                      netifapi_errt_fn errtfunc)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.common.voidfunc = voidfunc;
  NETIFAPI_VAR_REF(msg).msg.common.errtfunc = errtfunc;
  err = tcpip_api_call(netifapi_do_netif_common, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

/**
* @ingroup netifapi_netif
* Call netif_name_to_index() in a thread-safe way by running that function inside the
* tcpip_thread context.
*
* @param name the interface name of the netif
* @param idx output index of the found netif
*/
err_t
netifapi_netif_name_to_index(const char *name, u8_t *idx)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  *idx = 0;

#if LWIP_MPU_COMPATIBLE
  strncpy(NETIFAPI_VAR_REF(msg).msg.ifs.name, name, NETIF_NAMESIZE - 1);
  NETIFAPI_VAR_REF(msg).msg.ifs.name[NETIF_NAMESIZE - 1] = '\0';
#else
  NETIFAPI_VAR_REF(msg).msg.ifs.name = LWIP_CONST_CAST(char *, name);
#endif /* LWIP_MPU_COMPATIBLE */
  err = tcpip_api_call(netifapi_do_name_to_index, &API_VAR_REF(msg).call);
  if (!err) {
    *idx = NETIFAPI_VAR_REF(msg).msg.ifs.index;
  }
  NETIFAPI_VAR_FREE(msg);
  return err;
}

/**
* @ingroup netifapi_netif
* Call netif_index_to_name() in a thread-safe way by running that function inside the
* tcpip_thread context.
*
* @param idx the interface index of the netif
* @param name output name of the found netif, empty '\0' string if netif not found.
*             name should be of at least NETIF_NAMESIZE bytes
*/
err_t
netifapi_netif_index_to_name(u8_t idx, char *name)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).msg.ifs.index = idx;
#if !LWIP_MPU_COMPATIBLE
  NETIFAPI_VAR_REF(msg).msg.ifs.name = name;
#endif /* LWIP_MPU_COMPATIBLE */
  err = tcpip_api_call(netifapi_do_index_to_name, &API_VAR_REF(msg).call);
#if LWIP_MPU_COMPATIBLE
  if (!err) {
    strncpy(name, NETIFAPI_VAR_REF(msg).msg.ifs.name, NETIF_NAMESIZE - 1);
    name[NETIF_NAMESIZE - 1] = '\0';
  }
#endif /* LWIP_MPU_COMPATIBLE */
  NETIFAPI_VAR_FREE(msg);
  return err;
}

#endif /* LWIP_NETIF_API */
